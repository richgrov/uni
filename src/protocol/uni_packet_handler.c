#include "uni_packet_handler.h"

#include <stdlib.h>

#include "uni_packet.h"
#include "uni_log.h"

#define UNI_PKT_HANDSHAKE 0x00
#define UNI_PKT_LOGIN_SUCCESS 0x00
#define UNI_PKT_LOGIN_PLUGIN_REQ 0x04
#define UNI_PKT_LOGIN_PLUGIN_RES 0x02

#define UNI_PLUGIN_REQ_ID "velocity:player_info"
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

    conn->plugin_req_id = rand();
    int pkt_size =
        uni_varint_size(UNI_PKT_LOGIN_PLUGIN_REQ) +
        uni_varint_size(conn->plugin_req_id) +
        uni_str_size(sizeof(UNI_PLUGIN_REQ_ID) - 1);

    UniPacketOut pkt = uni_alloc_packet(pkt_size);
    if (pkt.buf == NULL) {
        // TODO: More verbose error message
        UNI_LOG("Failed to allocate packet", 0);
        return false;
    }

    char *cursor = &pkt.buf[pkt.write_idx];
    cursor = uni_write_varint(cursor, UNI_PKT_LOGIN_PLUGIN_REQ);
    cursor = uni_write_varint(cursor, conn->plugin_req_id);
             uni_write_str(cursor, UNI_PLUGIN_REQ_ID, sizeof(UNI_PLUGIN_REQ_ID) - 1);

    uni_conn_write(conn, &pkt);

    conn->handler = UNI_HANDLER_PLUGIN_RES;
    return true;
}

bool uni_recv_plugin_res(UniConnection *conn) {
    int id;
    if (!uni_read_varint(conn, &id) || id != UNI_PKT_LOGIN_PLUGIN_RES) {
        return false;
    }

    return false;
}
