#ifndef UNI_HANDSHAKE_H
#define UNI_HANDSHAKE_H

#include <stdbool.h>

#include "net/uni_connection.h"

bool uni_recv_handshake(UniConnection *conn);

#endif // UNI_HANDSHAKE_H
