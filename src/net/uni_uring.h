#ifndef UNI_URING_H
#define UNI_URING_H

#include <stdbool.h>
#include <stdint.h>

#include "liburing.h"
#include <netinet/in.h>

#include "uni.h"

typedef struct {
    struct io_uring ring;
    int fd;
    struct sockaddr_in server_addr;
    socklen_t addr_len;
} UniNetworking;

bool uni_net_init(UniNetworking *net, uint16_t port, UniError *err);

bool uni_net_run(UniNetworking *net);

#endif // UNI_URING_H
