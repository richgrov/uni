#ifndef UNI_PACKET_HANDLER_H
#define UNI_PACKET_HANDLER_H

#include <stdbool.h>

#include "net/uni_connection.h"

// Processes the current packet in the connection's read buffer.
bool uni_handle_packet(UniConnection *conn);

bool uni_recv_play(UniConnection *conn);

#endif // !UNI_PACKET_HANDLER_H
