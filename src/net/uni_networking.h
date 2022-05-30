#ifndef UNI_NETWORKING_H
#define UNI_NETWORKING_H

#include <stdbool.h>
#include <stdint.h>

#include "uni_server.h"

#define UNI_CONN_BACKLOG 16

bool uni_net_init(UniServer *server, uint16_t port, UniError *err);

#endif // !UNI_NETWORKING_H
