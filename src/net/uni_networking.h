#ifndef UNI_NETWORKING_H
#define UNI_NETWORKING_H

#include <stdbool.h>
#include <stdint.h>

#include "uni.h"
#include "uni_os_constants.h"

#ifdef UNI_OS_LINUX
#include "liburing.h"
#include <netinet/in.h>
#endif // UNI_OS_LINUX

typedef struct {
    UniServer *server;

#ifdef UNI_OS_LINUX
    struct io_uring ring;
    int fd;
    struct sockaddr_in server_addr;
    socklen_t addr_len;
#endif // UNI_OS_LINUX
} UniNetworking;

bool uni_net_init(UniNetworking *net, uint16_t port, UniError *err);

bool uni_net_run(UniNetworking *net);

#endif // UNI_NETWORKING_H
