#ifndef UNI_SERVER_H
#define UNI_SERVER_H

// Contains implementation functions related accessing data on a specific server
// instance.

#include <stdbool.h>

#include "uni.h"

// Verifies that a HMAC-SHA256 digest of the provided data is equal to received signature. The signature must be exactly
// 32 bytes long.
bool uni_verify_hmac(UniServer *server, const unsigned char *data, int data_len, const unsigned char* signature);

#endif // UNI_SERVER_H
