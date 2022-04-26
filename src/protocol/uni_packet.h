#ifndef UNI_PACKET_H
#define UNI_PACKET_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "net/uni_connection.h"

// Read a variable-length encoded 32-bit integer from the connection's read
// buffer. On success, true is returned and *result is set with the result. On
// failure, false is returned.
bool uni_read_varint(UniConnection *buf, int *result);

// Read an unsigned 16-bit integer from the connection's read buffer.
// On success, true is returned and *result is set with the result. On failure,
// false is returned.
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

// Read a string from the connection's read buffer, but ignore the actual data.
// On success, true is returned. On failure, false is returned.
bool uni_read_str_ignore(UniConnection *buf, int max_len);

// Calculate the size of a varint as if it were to be written to a connection.
static inline int uni_varint_size(int i) {
    if (i < 128) {
        return 1;
    } else if (i < 16384) {
        return 2;
    } else if (i < 2097152) {
        return 3;
    } else if (i < 268435456) {
        return 4;
    } else {
        return 5;
    }
}

// Calculate the size of a string as if it were to be written to a connection.
static inline int uni_str_size(int str_len) {
    return uni_varint_size(str_len) + str_len;
}

// Encodes/writes a varint to the specified buffer.
static inline char *uni_write_varint(char *dest, int val) {
    int i = 0;

    do {
        unsigned char temp = val & 0b01111111;

        val >>= 7;

        if (val != 0) {
            temp |= 0b10000000;
        }

        ((unsigned char *) dest)[i] = temp;
        i++;
    }
    while (val > 0);

    return (char *) dest + i;
}

// Encodes/writes a string to the specified buffer.
static inline char *uni_write_str(char *dest, char *const str, int len) {
    dest = uni_write_varint(dest, len);
    memcpy(dest, str, len);
    return dest + len;
}

// Allocates a packet with the given size on the heap. If the desired length of
// the packet exceeds the maximum length of a varint header (3 bytes), the 'buf'
// field of the returned value will be NULL.
UniPacketOut uni_alloc_packet(int size);

#endif // UNI_PACKET_H
