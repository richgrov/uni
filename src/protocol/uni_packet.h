#ifndef UNI_PACKET_H
#define UNI_PACKET_H

#include <stdbool.h>
#include <stdint.h>

#include "net/uni_connection.h"

bool uni_read_varint(UniConnection *buf, int *result);

static inline bool uni_read_ushort(UniConnection *buf, uint16_t *result) {
    if (buf->read_idx + sizeof(uint16_t) >= buf->packet_len) {
        return false;
    }

    uint16_t *val_ptr = (uint16_t *) &buf->packet_buf[buf->read_idx];

#ifdef UNI_BIG_ENDIAN
    *result = *val_ptr;
#else // UNI_BIG_ENDIAN
    *result = (*val_ptr >> 8) | (*val_ptr << 8);
#endif // !UNI_BIG_ENDIAN

    buf->read_idx += sizeof(uint16_t);

    return true;
}

bool uni_read_str_ignore(UniConnection *buf, int max_len);

#endif // UNI_PACKET_H
