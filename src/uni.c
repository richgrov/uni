#include "uni_server.h"

#include <stdlib.h>
#include <string.h>

#include "hmac_sha256.h"

#include "net/uni_networking.h"

struct UniServerImpl {
    UniNetworking net;
    const char *secret;
    int secret_len;
};

UniServer *uni_create(uint16_t port, const char *secret, UniError *err) {
    UniServer *server = malloc(sizeof(UniServer));

    server->net.server = server;
    server->secret = secret;
    server->secret_len = (int) strlen(secret);

    if (!uni_net_init(&server->net, port, err)) {
        free(server);
        return NULL;
    }

    return server;
}

bool uni_run(UniServer *server) {
    return uni_net_run(&server->net);
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
