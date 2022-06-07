#ifndef UNI_CONNECTION_H
#define UNI_CONNECTION_H

#include <stddef.h>

#include "uni.h"
#include "uni_networking.h"
#include "uni_os_constants.h"

#ifdef UNI_OS_LINUX
#include "liburing.h"
#endif // UNI_OS_LINUX

typedef enum {
    UNI_READING_HEADER,
    UNI_READING_BODY
} UniReadState;

typedef enum {
    UNI_HANDLER_HANDSHAKE,
    UNI_HANDLER_LOGIN_START,
    UNI_HANDLER_PLUGIN_RES
} UniPacketHandler;

typedef struct {
    char* buf;
    int len;

    // Used for two reasons:
    // 1. Each packet requires a varint header at the beginning to indicate its
    // length. This is the index AFTER the header so the rest of the packet can
    // be written properly.
    // 2. When writing the packet, not all data may be writen at once. This
    // tracks how much has, and hasn't been written.
    int write_idx;
} UniPacketOut;

struct UniConnectionImpl {
    UniServer *server;

#ifdef UNI_OS_LINUX
    int fd;
    struct __kernel_timespec timeout;
    void *timeout_usr_data;
#endif // UNI_OS_LINUX

    UniReadState state;
    UniPacketHandler handler;
    int refcount;

    unsigned char *packet_buf;
    int packet_len;
    UniPacketOut out_pkt;

    int header_len_limit;
    union {
        struct {
            unsigned char header_buf[2];
            int header_size;
        };
        int write_idx;
        int read_idx;
    };

    union {
        int plugin_req_id;
        void *user_ptr;
    };
};

static inline void uni_init_conn(UniServer *server, UniConnection *conn) {
    conn->server = server;
    conn->handler = UNI_HANDLER_HANDSHAKE;
    conn->refcount = 0;
    conn->packet_buf = NULL;
    conn->header_len_limit = 1;
    conn->header_size = 0;
}

// Prepares the connection's state so it is ready to read a packet's header.
static inline void uni_conn_prep_header(UniConnection *conn) {
    conn->state = UNI_READING_HEADER;
    conn->header_size = 0;
    conn->packet_len = 0;
}

// Prepares the connection's state so it is ready to read a packet's body.
static inline void uni_conn_prep_body(UniConnection *conn) {
    conn->state = UNI_READING_BODY;
    conn->write_idx = 0;
}

// Prepares the connection's state so it is ready to call a packet handler
// function.
static inline void uni_conn_prep_handle(UniConnection *conn) {
    conn->read_idx = 0;
}

// Prepares the connection's state for writing a packet + queues the write
// operation.
void uni_conn_write(UniConnection *conn, UniPacketOut *packet);

#endif // !UNI_CONNECTION_H
