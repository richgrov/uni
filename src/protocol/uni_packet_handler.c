#include "uni_packet_handler.h"

#include <stdlib.h>

#include "uni_packet.h"
#include "uni_log.h"
#include "uni_server.h"

#define UNI_PKT_HANDSHAKE 0x00
#define UNI_PKT_LOGIN_SUCCESS 0x00
#define UNI_PKT_LOGIN_PLUGIN_REQ 0x04
#define UNI_PKT_LOGIN_PLUGIN_RES 0x02

#define UNI_PLUGIN_REQ_ID "velocity:player_info"
#define UNI_STATE_LOGIN 2

// Handle a 'Handshake' packet from the client.
static bool uni_recv_handshake(UniConnection *conn) {
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

// Handle a 'Login Start' packet from the client.
static bool uni_recv_login_start(UniConnection *conn) {
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
    conn->header_len_limit = 2;
    return true;
}

// Handle a 'Login Plugin Response' packet from the client.
static bool uni_recv_plugin_res(UniConnection *conn) {
    int id;
    if (!uni_read_varint(conn, &id) || id != UNI_PKT_LOGIN_PLUGIN_RES) {
        return false;
    }

    // https://forums.velocitypowered.com/t/velocity-modern-player-forwarding-protocol-information/52

    int message_id;
    if (!uni_read_varint(conn, &message_id) || message_id != conn->plugin_req_id) {
        return false;
    }

    bool understood;
    if (!uni_read_bool(conn, &understood) || !understood) {
        return false;
    }

    unsigned char* hmac = uni_read_bytes(conn, 32);
    if (hmac == NULL) {
        return false;
    }

    if (!uni_verify_hmac(conn->net->server, &conn->packet_buf[conn->read_idx], conn->packet_len - conn->read_idx, hmac)) {
        return false;
    }

    int protocol_ver;
    if (!uni_read_varint(conn, &protocol_ver)) {
        return false;
    }

    int addr_len;
    char *address = uni_read_str(conn, 15, &addr_len);
    if (address == NULL) {
        return false;
    }

    unsigned char *uuid = uni_read_bytes(conn, 16);
    if (uuid == NULL) {
        return false;
    }

    int name_len;
    char *name = uni_read_str(conn, 16, &name_len);
    if (name == NULL) {
        return false;
    }

    int num_properties;
    if (!uni_read_varint(conn, &num_properties)) {
        return false;
    }

    for (int i = 0; i < num_properties; i++) {
        int property_name_len;
        char *property = uni_read_str(conn, 32, &property_name_len);
        if (property == NULL) {
            return false;
        }

        int val_len;
        char *value = uni_read_str(conn, 1024, &val_len);
        if (value == NULL) {
            return false;
        }

        bool has_sig;
        if (!uni_read_bool(conn, &has_sig)) {
            return false;
        }

        if (has_sig) {
            int sig_len;
            char *signature = uni_read_str(conn, 1024, &sig_len);
            if (signature == NULL) {
                return false;
            }
        }
    }

    return true;
}

bool uni_handle_packet(UniConnection *conn) {
    switch (conn->handler) {
        case UNI_HANDLER_HANDSHAKE:
            return uni_recv_handshake(conn);

        case UNI_HANDLER_LOGIN_START:
            return uni_recv_login_start(conn);

        case UNI_HANDLER_PLUGIN_RES:
            return uni_recv_plugin_res(conn);
    }

    UNI_UNREACHABLE();
}
