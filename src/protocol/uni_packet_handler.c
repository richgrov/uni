#include "uni_packet_handler.h"

#include <stdlib.h>

#include "uni_packet.h"
#include "uni_log.h"
#include "uni_server.h"

#define UNI_PKT_HANDSHAKE 0x00
#define UNI_PKT_LOGIN_START 0x00
#define UNI_PKT_LOGIN_PLUGIN_REQ 0x04
#define UNI_PKT_LOGIN_PLUGIN_RES 0x02
#define UNI_PKT_LOGIN_SUCCESS 0x02

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
    if (!uni_read_varint(conn, &id) || id != UNI_PKT_LOGIN_START) {
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
        UNI_LOG("Disconnect: uni_alloc_packet(%d) failed", pkt_size);
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

    if (!uni_verify_hmac(conn->server, &conn->packet_buf[conn->read_idx], conn->packet_len - conn->read_idx, hmac)) {
        return false;
    }

    int protocol_ver;
    if (!uni_read_varint(conn, &protocol_ver)) {
        return false;
    }

    UniLoginData data;

    data.address_str = uni_read_str(conn, 15, &data.address_len);
    if (data.address_str == NULL) {
        return false;
    }

    data.uuid_raw = uni_read_bytes(conn, 16);
    if (data.uuid_raw == NULL) {
        return false;
    }

    data.player_name = uni_read_str(conn, 16, &data.name_len);
    if (data.player_name == NULL) {
        return false;
    }

    if (!uni_read_varint(conn, &data.num_properties) || data.num_properties < 0) {
        return false;
    }

    data.properties = malloc(sizeof(UniLoginProperty) * data.num_properties);

    for (int i = 0; i < data.num_properties; i++) {
        UniLoginProperty *prop = &data.properties[i];
        prop->name = uni_read_str(conn, 32, &prop->name_len);
        if (prop->name == NULL) {
            goto property_fail;
        }

        prop->value = uni_read_str(conn, 1024, &prop->value_len);
        if (prop->value == NULL) {
            goto property_fail;
        }

        bool has_sig;
        if (!uni_read_bool(conn, &has_sig)) {
            return false;
        }

        if (has_sig) {
            prop->signature = uni_read_str(conn, 1024, &prop->signature_len);
            if (prop->signature == NULL) {
                goto property_fail;
            }
        } else {
            prop->signature = NULL;
        }
    }

    void *user_ptr = uni_on_login(&data);
    free(data.properties);

    if (user_ptr == NULL) {
        return false;
    }

    conn->user_ptr = user_ptr;

    int pkt_size =
        uni_varint_size(UNI_PKT_LOGIN_SUCCESS) +
        16 + /* UUID */
        uni_str_size(data.name_len);

    UniPacketOut pkt = uni_alloc_packet(pkt_size);
    if (pkt.buf == NULL) {
        UNI_LOG("Failed to allocate packet", 0);
        return false;
    }

    char *cursor = &pkt.buf[pkt.write_idx];
    cursor = uni_write_varint(cursor, UNI_PKT_LOGIN_SUCCESS);
    cursor = uni_write_bytes(cursor, data.uuid_raw, 16);
             uni_write_str(cursor, data.player_name, data.name_len);

    uni_conn_write(conn, &pkt);
    return true;

property_fail:
    free(data.properties);
    return false;
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
