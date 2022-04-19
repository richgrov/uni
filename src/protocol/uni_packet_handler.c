#include "uni_packet_handler.h"

#include "uni_packet.h"

#define UNI_PKT_HANDSHAKE 0x00
#define UNI_PKT_LOGIN_SUCCESS 0x00
#define UNI_STATE_LOGIN 2

bool uni_recv_handshake(UniConnection *conn) {
    int id;
    if (!uni_read_varint(conn, &id) || id != UNI_PKT_HANDSHAKE) {
        return false;
    }

    int protocol_ver;
    uint16_t port;
    int next_state;

    if (
        !uni_read_varint(conn, &protocol_ver) ||
        !uni_read_str_ignore(conn, 255) ||
        !uni_read_ushort(conn, &port) ||
        !uni_read_varint(conn, &next_state)
    ) {
        return false;
    }

    conn->handler = UNI_HANDLER_LOGIN_START;
    return next_state == UNI_STATE_LOGIN;
}

bool uni_recv_login_start(UniConnection *conn) {
    int id;
    if (!uni_read_varint(conn, &id) || id != UNI_PKT_LOGIN_SUCCESS) {
        return false;
    }

    if (!uni_read_str_ignore(conn, 16)) {
        return false;
    }

    return true;
}
