#include "uni_packet.h"

#include <stdlib.h>

bool uni_read_varint(UniConnection *buf, int *result) {
    *result = 0;

    for (int i = 0; i < 6; i++) {
        if (buf->read_idx >= buf->packet_len) {
            return false;
        }

        char byte = buf->packet_buf[buf->read_idx];
        buf->read_idx++;
 
        *result |= (byte & 0b01111111) << (7 * i);

        if ((byte & 0b10000000) == 0) {
            return true;
        }
    }

    return false;
}

char *uni_read_str(UniConnection *conn, int max_len, int *out_len) {
    if (!uni_read_varint(conn, out_len)) {
        return NULL;
    }

    if (*out_len < 0 || *out_len > max_len) {
        return NULL;
    }

    char *str = &conn->packet_buf[conn->read_idx];

    conn->read_idx += *out_len;
    if (conn->read_idx <= conn->packet_len) {
        return str;
    } else {
        return NULL;
    }
}

bool uni_read_str_ignore(UniConnection *buf, int max_len) {
    int str_len;
    if (!uni_read_varint(buf, &str_len)) {
        return false;
    }

    if (str_len < 0 || str_len > max_len) {
        return false;
    }

    buf->read_idx += str_len;
    return buf->read_idx <= buf->packet_len;
}

UniPacketOut uni_alloc_packet(int size) {
    UniPacketOut packet;

    int header_size = uni_varint_size(size);
    if (header_size > 3) {
        // Packet length headers can't be longer than 3 bytes.
        // (i.e. packets must be less than 2097152 bytes in length)
        packet.buf = NULL;
    } else {
        packet.buf = malloc(header_size + size);
        packet.len = header_size + size;
        packet.write_idx = header_size;
        uni_write_varint(packet.buf, size);
    }
    return packet;
}
