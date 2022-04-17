#include "uni.h"

#include <stdlib.h>

#include "net/uni_networking.h"

struct UniServerImpl {
    UniNetworking net;
};

UniServer *uni_create(uint16_t port, UniError *err) {
    UniServer *server = malloc(sizeof(UniServer));

    if (!uni_net_init(&server->net, port, err)) {
        free(server);
        return NULL;
    }

    return server;
}

bool uni_run(UniServer *server) {
    uni_net_run(&server->net);
}
