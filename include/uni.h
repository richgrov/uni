#ifndef UNI_H
#define UNI_H

#include <stdbool.h>
#include <stdint.h>

typedef struct UniServerImpl UniServer;

UniServer *uni_create(uint16_t port);

bool uni_run(UniServer *server);

#endif // UNI_H
