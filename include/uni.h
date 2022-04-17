#ifndef UNI_H
#define UNI_H

#include <stdbool.h>
#include <stdint.h>

typedef struct UniServerImpl UniServer;

typedef enum {
    // A socket with the same address and port is already being used.
    UNI_ERR_IN_USE,

    // The socket could not be initialized due to low memory, a system-imposed
    // limit, or insufficient permission.
    UNI_ERR_LIMITED,

    // The operating system does not support the protocol or feature required.
    UNI_ERR_UNSUPPORTED,

    UNI_ERR_UNKNOWN
} UniError;

// Initializes a uni server.
// In case of error, NULL will be returned and *err will be set to the error
// which occurred.
// err is allowed to be NULL, if so, no attempt at returning an error code will
// be made.
UniServer *uni_create(uint16_t port, UniError *err);

bool uni_run(UniServer *server);

#endif // UNI_H
