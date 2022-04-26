#ifndef UNI_PACKET_HANDLER_H
#define UNI_PACKET_HANDLER_H

#include <stdbool.h>

#include "net/uni_connection.h"

// Handle a 'Handshake' packet from the client.
bool uni_recv_handshake(UniConnection *conn);

// Handle a 'Login Start' packet from the client.
bool uni_recv_login_start(UniConnection *conn);

// Handle a 'Login Plugin Response' packet from the client.
bool uni_recv_plugin_res(UniConnection *conn);

#endif // UNI_PACKET_HANDLER_H
