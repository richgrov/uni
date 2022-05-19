#include "uni_networking.h"

#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include "liburing.h"
#include <unistd.h>

#include "uni_connection.h"
#include "protocol/uni_packet_handler.h"
#include "uni_log.h"

#define UNI_CONN_BACKLOG 16

typedef enum {
    UNI_ACT_READ,
    UNI_ACT_WRITE,
    UNI_ACT_ACCEPT,
    UNI_ACT_TIMEOUT,
    UNI_ACT_TIMEOUT_CANCEL
} UniUringAction;

typedef struct {
    UniUringAction action;
    UniConnection *conn;
} UniUringEntry;

void uni_dump_conn(UniConnection *conn) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
    UNI_LOG("UniConnection: {", 0);
    UNI_LOG("  fd = %d", conn->fd);
    UNI_LOG("  state = %d", conn->state);
    UNI_LOG("  handler = %d", conn->handler);
    UNI_LOG("  packet buf, len = %p, %d", conn->packet_buf, conn->packet_len);
    UNI_LOG("  header_len_limit = %d", conn->header_len_limit);
    UNI_LOG("  header_buf [0], [1] = %x, %x", conn->header_buf[0], conn->header_buf[1]);
    UNI_LOG("  header_size = %d", conn->header_size);
    UNI_LOG("  read/write idx = %d", conn->write_idx);
    UNI_LOG("}", 0);
#pragma clang diagnostic pop
}

static void uni_dump_net_err(const char *type, int res) {
    UNI_LOG("-- UNI %s ERROR --", type);
    UNI_LOG("res: %d, res string: %s, errno: %d, errno string: %s", res, strerror(-res), errno, strerror(errno));
}

static void uni_dump_conn_err(const char *type, UniConnection *conn, int res) {
    uni_dump_net_err(type, res);
    uni_dump_conn(conn);
}

void uni_uring_accept(UniServer *server, int socket, struct sockaddr *addr, socklen_t *addr_len) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&server->ring);
    io_uring_prep_accept(sqe, socket, addr, addr_len, 0);

    UniUringEntry *entry = malloc(sizeof(UniUringEntry));
    entry->action = UNI_ACT_ACCEPT;
    sqe->user_data = (__u64) entry;
}

// Queue a read operation.
void uni_uring_read(UniServer *server, UniConnection *conn, unsigned char* buf, int max_len) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&server->ring);
    io_uring_prep_recv(sqe, conn->fd, buf, max_len, 0);

    UniUringEntry *entry = malloc(sizeof(UniUringEntry));
    entry->action = UNI_ACT_READ;
    entry->conn = conn;
    sqe->user_data = (__u64) entry;
    conn->refcount++;
}

// Queue a write operation.
void uni_uring_write(UniServer *server, UniConnection *conn) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&server->ring);
    io_uring_prep_send(sqe, conn->fd, conn->out_pkt.buf, conn->out_pkt.len, 0);

    UniUringEntry *entry = malloc(sizeof(UniUringEntry));
    entry->action = UNI_ACT_WRITE;
    entry->conn = conn;
    sqe->user_data = (__u64) entry;
    conn->refcount++;
}

// Set a timeout and await its completion.
void uni_uring_timeout(UniServer *server, UniConnection *conn, int secs) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&server->ring);
    conn->timeout.tv_sec = secs;
    conn->timeout.tv_nsec = 0;
    io_uring_prep_timeout(sqe, &conn->timeout, 0, 0);

    UniUringEntry *entry = malloc(sizeof(UniUringEntry));
    entry->action = UNI_ACT_TIMEOUT;
    entry->conn = conn;
    sqe->user_data = (__u64) entry;
    conn->timeout_usr_data = entry;
    conn->refcount++;
}

// Cancel an ongoing timeout. This will result in both the timeout completing
// with code -ECANCELED and the cancellation operation itself reporting that it
// has completed.
void uni_uring_cancel_timeout(UniServer *server, UniConnection *conn) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&server->ring);
    io_uring_prep_timeout_remove(sqe, (__u64) conn->timeout_usr_data, 0);

    UniUringEntry *entry = malloc(sizeof(UniUringEntry));
    entry->action = UNI_ACT_TIMEOUT_CANCEL;
    entry->conn = conn;
    sqe->user_data = (__u64) entry;
    conn->refcount++;
}

// Shutdown all read/write operations and cancel the current timeout on a
// connection.
static void uni_conn_shutdown(UniServer *server, UniConnection *conn) {
    uni_uring_cancel_timeout(server, conn);
    shutdown(conn->fd, SHUT_RDWR);
}

// Attempt to free and close a connection if its reference count is zero.
// Returns true on success, false otherwise.
static bool uni_conn_gc(UniConnection *conn) {
    if (conn->refcount == 0) {
        close(conn->fd);
        free(conn->packet_buf);
        free(conn);
        return true;
    }

    return false;
}

bool uni_net_init(UniServer *server, uint16_t port, UniError *err) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server->server_addr = server_addr;
    server->addr_len = sizeof(server_addr);

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->fd == -1) {
        if (err != NULL) {
            switch (errno) {
                case EAFNOSUPPORT:
                case EINVAL:
                case EPROTONOSUPPORT:
                    *err = UNI_ERR_UNSUPPORTED;
                    break;

                case EACCES:
                case EMFILE:
                case ENFILE:
                case ENOBUFS:
                case ENOMEM:
                    *err = UNI_ERR_LIMITED;
                    break;

                default:
                    *err = UNI_ERR_UNKNOWN;
                    break;
            }
        }
        return false;
    }

    if (bind(server->fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        if (err != NULL) {
            switch (errno) {
                case EACCES:
                case EFAULT:
                case ENOMEM:
                case EROFS:
                    *err = UNI_ERR_LIMITED;
                    break;

                case EADDRINUSE:
                    *err = UNI_ERR_IN_USE;
                    break;

                default:
                    *err = UNI_ERR_UNKNOWN;
                    break;
            }
        }
        return false;
    }

    struct io_uring_params params;
    memset(&params, 0, sizeof((params)));

    if (io_uring_queue_init_params(2048 /* TODO: Is this a good amount? */, &server->ring, &params) < 0) {
        // TODO: Determine cause of error
        if (err != NULL) {
            *err = UNI_ERR_UNSUPPORTED;
        }
        return false;
    }

    if (!(params.features & IORING_FEAT_FAST_POLL)) {
        if (err != NULL) {
            *err = UNI_ERR_UNSUPPORTED;
        }
        return false;
    }

    uni_uring_accept(server, server->fd, (struct sockaddr *) &server->server_addr, &server->addr_len);

    return true;
}

bool uni_run(UniServer *server) {
    if (listen(server->fd, UNI_CONN_BACKLOG) == -1) {
        return false;
    }

    while (/* TODO: is_running */true) {
        io_uring_submit_and_wait(&server->ring, 1);

        unsigned head;
        struct io_uring_cqe *cqe;
        unsigned count = 0;

        io_uring_for_each_cqe(&server->ring, head, cqe) {
            count++;

            UniUringEntry *entry = (UniUringEntry*) cqe->user_data;
            UniConnection *conn = entry->conn;
            switch (entry->action) {
                case UNI_ACT_ACCEPT:
                    if (cqe->res >= -1) {
                        conn = malloc(sizeof(UniConnection));
                        uni_init_conn(server, conn);
                        uni_conn_prep_header(conn);
                        conn->fd = cqe->res;

                        uni_uring_timeout(server, conn, 2);
                        uni_uring_read(server, conn, conn->header_buf, sizeof(conn->header_buf));
                    } else {
                        uni_dump_net_err("ACCEPT", cqe->res);
                    }

                    uni_uring_accept(server, server->fd, (struct sockaddr *) &server->server_addr, &server->addr_len);
                    break;

                case UNI_ACT_READ:
                    conn->refcount--;
                    if (uni_conn_gc(conn)) {
                        break;
                    }

                    if (cqe->res > 0) {
                        if (conn->state == UNI_READING_HEADER) {
                            for (int i = 0; i < cqe->res; i++) {
                                unsigned char b = conn->header_buf[i];
                                conn->packet_len |= (b & 0b01111111) << (7 * conn->header_size++);

                                if (conn->header_size > conn->header_len_limit) {
                                    UNI_DLOG("Disconnect: Header size %d > %d", conn->header_size, conn->header_len_limit);
                                #ifdef UNI_DEBUG
                                    uni_dump_conn(conn);
                                #endif // UNI_DEBUG

                                    uni_conn_shutdown(server, conn);
                                    goto nothing;
                                } else if ((b & 0b10000000) == 0) {
                                    conn->packet_buf = realloc(conn->packet_buf, conn->packet_len);
                                    if (conn->packet_buf == NULL) {
                                        UNI_DLOG("Disconnect: realloc(%d) failed", conn->packet_len);
                                        uni_dump_conn(conn);

                                        uni_conn_shutdown(server, conn);
                                        goto nothing;
                                    }

                                    if (i == 0 && cqe->res == 2) {
                                        conn->packet_buf[0] = conn->header_buf[1];
                                        uni_conn_prep_body(conn);
                                        conn->write_idx++;
                                    } else {
                                        uni_conn_prep_body(conn);
                                    }

                                    uni_uring_read(server, conn, &conn->packet_buf[conn->write_idx], conn->packet_len - conn->write_idx);
                                    goto nothing;
                                }
                            }

                            uni_uring_read(server, conn, conn->header_buf, 1);

                            nothing: ;
                        } else {
                            conn->write_idx += cqe->res;
                            if (conn->write_idx == conn->packet_len) {
                                uni_conn_prep_handle(conn);

                                if (!uni_handle_packet(conn)) {
                                    // Something went wrong trying to process this packet.
                                    uni_conn_shutdown(server, conn);
                                } else {
                                    uni_conn_prep_header(conn);
                                    uni_uring_read(server, conn, conn->header_buf, sizeof(conn->header_buf));
                                }
                            } else {
                                uni_uring_read(server, conn, &conn->packet_buf[conn->write_idx], conn->packet_len - conn->write_idx);
                            }
                        }
                    } else if (cqe->res == 0) {
                        uni_conn_shutdown(server, conn);
                    } else {
                        uni_dump_conn_err("READ", conn, cqe->res);
                    }
                    break;

                case UNI_ACT_WRITE:
                    conn->refcount--;
                    if (uni_conn_gc(conn)) {
                        break;
                    }

                    if (cqe->res > 0) {
                        conn->out_pkt.write_idx += cqe->res;

                        if (conn->out_pkt.write_idx == conn->out_pkt.len) {
                            free(conn->out_pkt.buf);
                        } else {
                            uni_uring_write(server, conn);
                        }
                    } else {
                        uni_dump_conn_err("WRITE", conn, cqe->res);
                    }
                    break;

                case UNI_ACT_TIMEOUT:
                    conn->refcount--;
                    if (cqe->res != -ECANCELED) {
                        if (!uni_conn_gc(conn)) {
                            shutdown(conn->fd, SHUT_RDWR);
                        }
                    }
                    break;

                case UNI_ACT_TIMEOUT_CANCEL:
                    conn->refcount--;
                    uni_conn_gc(conn);
                    break;
            }

            free(entry);
        }

        io_uring_cq_advance(&server->ring, count);
    }

    return true;
}

void uni_conn_write(UniConnection *conn, UniPacketOut *packet) {
    packet->write_idx = 0;
    conn->out_pkt = *packet;
    uni_uring_write(conn->server, conn);
}
