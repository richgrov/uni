#ifndef UNI_CONNECTION_H
#define UNI_CONNECTION_H

#include <stddef.h>

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
} UniPacketHandler;

typedef struct {
#ifdef UNI_OS_LINUX
    int fd;
    struct __kernel_timespec timeout;
    void *timeout_usr_data;
#endif // UNI_OS_LINUX

    UniReadState state;
    UniPacketHandler handler;
    int refcount;

    char *packet_buf;
    int packet_len;

    int header_len_limit;
    union {
        struct {
            char header_buf[2];
            int header_size;
        };
        int write_idx;
        int read_idx;
    };
} UniConnection;

static inline void uni_init_conn(UniConnection *conn) {
    conn->handler = UNI_HANDLER_HANDSHAKE;
    conn->refcount = 0;
    conn->packet_buf = NULL;
    conn->header_len_limit = 1;
    conn->header_size = 0;
}

static inline void uni_conn_prep_header(UniConnection *conn) {
    conn->state = UNI_READING_HEADER;
    conn->header_size = 0;
    conn->packet_len = 0;
}

static inline void uni_conn_prep_body(UniConnection *conn) {
    conn->state = UNI_READING_BODY;
    conn->write_idx = 0;
}

static inline void uni_conn_prep_handle(UniConnection *conn) {
    conn->read_idx = 0;
}

#endif // UNI_CONNECTION_H
