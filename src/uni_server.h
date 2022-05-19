#ifndef UNI_SERVER_H
#define UNI_SERVER_H

// Contains implementation functions related accessing data on a specific server
// instance.

#include <stdbool.h>

#include "uni_os_constants.h"
#include "uni.h"

#ifdef UNI_OS_LINUX
#include "liburing.h"
#include <netinet/in.h>
#endif // UNI_OS_LINUX

struct UniServerImpl {
    const char *secret;
    int secret_len;

#ifdef UNI_OS_LINUX
    struct io_uring ring;
    int fd;
    struct sockaddr_in server_addr;
    socklen_t addr_len;
#endif // UNI_OS_LINUX
};

// Verifies that a HMAC-SHA256 digest of the provided data is equal to received signature. The signature must be exactly
// 32 bytes long.
bool uni_verify_hmac(UniServer *server, const unsigned char *data, int data_len, const unsigned char* signature);

#endif // UNI_SERVER_H
