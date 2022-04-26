#ifndef UNI_PACKET_HANDLER_H
#define UNI_PACKET_HANDLER_H

#include <stdbool.h>

#include "net/uni_connection.h"

bool uni_recv_handshake(UniConnection *conn);

bool uni_recv_login_start(UniConnection *conn);

bool uni_recv_plugin_req(UniConnection *conn);

#endif // UNI_PACKET_HANDLER_H
