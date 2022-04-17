#include "uni_packet.h"

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

bool uni_read_str_ignore(UniConnection *buf, int max_len) {
    int str_len;
    if (!uni_read_varint(buf, &str_len)) {
        return false;
    }

    if (str_len < 0 || str_len > max_len) {
        return false;
    }

    buf->read_idx += str_len;
    return buf->read_idx < buf->packet_len;
}
