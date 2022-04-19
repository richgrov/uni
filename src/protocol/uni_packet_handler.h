#ifndef UNI_PACKET_HANDLER_H
#define UNI_PACKET_HANDLER_H

#include <stdbool.h>

#include "net/uni_connection.h"

bool uni_recv_handshake(UniConnection *conn);

#endif // UNI_PACKET_HANDLER_H
