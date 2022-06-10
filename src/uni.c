#include "uni_server.h"

#include <stdlib.h>
#include <string.h>

#include "hmac_sha256.h"

#include "net/uni_networking.h"

UniServer *uni_create(uint16_t port, const char *secret, void *user_ptr, UniError *err) {
    UniServer *server = malloc(sizeof(UniServer));

    server->secret_len = (int) strlen(secret);
    server->secret = malloc(server->secret_len);
    memcpy(server->secret, secret, server->secret_len);
    server->user_ptr = user_ptr;

    if (!uni_net_init(server, port, err)) {
        free(server);
        free(server->secret);
        return NULL;
    }

    return server;
}

void uni_free(UniServer *server) {
    free(server->secret);
}

bool uni_verify_hmac(UniServer *server, const unsigned char *data, int data_len, const unsigned char* signature) {
    unsigned char out[32];
    if (hmac_sha256(server->secret, server->secret_len, data, data_len, out, 32) != 32) {
        return false;
    }

    // Constant-time comparison to prevent timing attacks
    int res = 0;
    for (int i = 0; i < 32; i++) {
        res |= out[i] ^ signature[i];
    }

    return res == 0;
}
