#include "uni_packet_handler.h"

#include "uni_packet.h"

typedef struct {
    int protocol_ver;
    int next_state;
} UniPktHandshake;

#define UNI_PKT_HANDSHAKE 0x00
#define UNI_STATE_LOGIN 2

bool uni_pkt_handshake(UniPktHandshake *packet, UniConnection *conn) {
    uint16_t port;

    return
        uni_read_varint(conn, &packet->protocol_ver) &&
        uni_read_str_ignore(conn, 255) &&
        uni_read_ushort(conn, &port) &&
        uni_read_varint(conn, &packet->next_state);
}

bool uni_recv_handshake(UniConnection *conn) {
    int id;
    if (!uni_read_varint(conn, &id) || id != UNI_PKT_HANDSHAKE) {
        return false;
    }

    UniPktHandshake packet;
    if (!uni_pkt_handshake(&packet, conn)) {
        return false;
    }

    return packet.next_state == UNI_STATE_LOGIN;
}
