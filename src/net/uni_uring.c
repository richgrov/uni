#include "uni_uring.h"

#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include "liburing.h"
#include <unistd.h>

#include "uni_connection.h"
#include "protocol/uni_handshake.h"

#define UNI_CONN_BACKLOG 16

typedef enum {
    UNI_ACT_READ,
    UNI_ACT_WRITE,
    UNI_ACT_ACCEPT
} UniUringAction;

typedef struct {
    UniUringAction action;
    UniConnection *conn;
} UniUringEntry;

void uni_uring_accept(UniNetworking *net, int socket, struct sockaddr *addr, socklen_t *addr_len) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&net->ring);
    io_uring_prep_accept(sqe, socket, addr, addr_len, 0);
    io_uring_sqe_set_flags(sqe, 0); // TODO: Needed?

    UniUringEntry *entry = malloc(sizeof(UniUringEntry));
    entry->action = UNI_ACT_ACCEPT;
    sqe->user_data = (__u64) entry;
}

void uni_uring_read(UniNetworking *net, UniConnection *conn, char* buf, int max_len) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&net->ring);
    io_uring_prep_recv(sqe, conn->fd, buf, max_len, 0);
    io_uring_sqe_set_flags(sqe, 0); // TODO: Needed?

    UniUringEntry *entry = malloc(sizeof(UniUringEntry));
    entry->action = UNI_ACT_READ;
    entry->conn = conn;
    sqe->user_data = (__u64) entry;
}

static void uni_conn_free(UniConnection *conn) {
    free(conn->packet_buf);
    free(conn);
}

bool uni_net_init(UniNetworking *net, uint16_t port) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    net->server_addr = server_addr;
    net->addr_len = sizeof(server_addr);

    net->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (net->fd == -1) {
        // TODO: Better way of signifying the exact error
        return false;
    }

    if (bind(
        net->fd,
        (struct sockaddr *) &server_addr,
        sizeof(server_addr)
    ) == -1) {
        return false;
    }

    struct io_uring_params params;
    memset(&params, 0, sizeof((params)));

    if (io_uring_queue_init_params(2048 /* TODO: Is this a good amount? */, &net->ring, &params) < 0) {
        return false;
    }

    if (!(params.features & IORING_FEAT_FAST_POLL)) {
        return false;
    }

    struct io_uring_probe *probe;
    probe = io_uring_get_probe_ring(&net->ring);
    if (!probe | !io_uring_opcode_supported(probe, IORING_OP_PROVIDE_BUFFERS)) {
        return false;
    }
    free(probe);

    uni_uring_accept(net, net->fd, (struct sockaddr *) &net->server_addr, &net->addr_len);

    return true;
}

bool uni_net_run(UniNetworking *net) {
    if (listen(net->fd, UNI_CONN_BACKLOG) == -1) {
        return false;
    }

    while (/* TODO: is_running */true) {
        io_uring_submit_and_wait(&net->ring, 1);

        unsigned head;
        struct io_uring_cqe *cqe;
        unsigned count = 0;

        io_uring_for_each_cqe(&net->ring, head, cqe) {
            count++;

            UniUringEntry *entry = (UniUringEntry*) cqe->user_data;
            switch (entry->action) {
                case UNI_ACT_ACCEPT:
                    if (cqe->res != -1) {
                        UniConnection *conn = malloc(sizeof(UniConnection));
                        uni_init_conn(conn);
                        uni_conn_prep_header(conn);
                        conn->fd = cqe->res;

                        uni_uring_read(net, conn, conn->header_buf, sizeof(conn->header_buf));
                    } else {
                        // TODO: Error
                    }

                    uni_uring_accept(net, net->fd, (struct sockaddr *) &net->server_addr, &net->addr_len);
                    break;

                case UNI_ACT_READ:
                    if (cqe->res > 0) {
                        UniConnection *conn = entry->conn;

                        if (entry->conn->state == UNI_READING_HEADER) {
                            for (int i = 0; i < cqe->res; i++) {
                                char b = conn->header_buf[i];
                                conn->packet_len |= (b & 0b01111111) << (7 * conn->header_size++);

                                if (conn->header_size > conn->header_len_limit) {
                                    close(conn->fd);
                                    uni_conn_free(conn);
                                    goto nothing;
                                } else if ((b & 0b10000000) == 0) {
                                    conn->packet_buf = realloc(conn->packet_buf, conn->packet_len);
                                    if (conn->packet_buf == NULL) {
                                        close(conn->fd);
                                        uni_conn_free(conn);
                                        goto nothing;
                                    }

                                    if (i == 0 && cqe->res == 2) {
                                        conn->packet_buf[0] = conn->header_buf[1];
                                        uni_conn_prep_body(conn);
                                        conn->write_idx++;
                                    } else {
                                        uni_conn_prep_body(conn);
                                    }

                                    uni_uring_read(net, conn, &conn->packet_buf[conn->write_idx], conn->packet_len - conn->write_idx);
                                    goto nothing;
                                }
                            }

                            uni_uring_read(net, conn, conn->header_buf, 1);

                            nothing: ;
                        } else {
                            conn->write_idx += cqe->res;
                            if (conn->write_idx == conn->packet_len) {
                                uni_conn_prep_handle(conn);

                                bool success;
                                switch (conn->handler) {
                                    case UNI_HANDLER_HANDSHAKE:
                                        success = uni_recv_handshake(conn);
                                        break;
                                }

                                if (!success) {
                                    close(conn->fd);
                                    uni_conn_free(conn);
                                } else {
                                    uni_conn_prep_header(conn);
                                    uni_uring_read(net, conn, conn->header_buf, sizeof(conn->header_buf));
                                }
                            } else {
                                uni_uring_read(net, conn, &conn->packet_buf[conn->write_idx], conn->packet_len - conn->write_idx);
                            }
                        }
                    } else if (cqe->res == 0) {
                        uni_conn_free(entry->conn);
                    } else {
                        // TODO: Should close() be called for res <= 0?
                    }
                    break;

                case UNI_ACT_WRITE:
                    break;
            }

            free(entry);
        }

        io_uring_cq_advance(&net->ring, count);
    }

    return true;
}
