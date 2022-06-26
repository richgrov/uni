#ifndef UNI_H
#define UNI_H

#include <stdbool.h>
#include <stdint.h>

typedef struct UniServerImpl UniServer;
typedef struct UniConnectionImpl UniConnection;

typedef enum {
    // A socket with the same address and port is already being used.
    UNI_ERR_IN_USE,

    // The socket could not be initialized due to low memory, a system-imposed
    // limit, or insufficient permission.
    UNI_ERR_LIMITED,

    // The operating system does not support the protocol or feature required.
    UNI_ERR_UNSUPPORTED,

    UNI_ERR_UNKNOWN,
} UniError;

// Initializes a uni server.
// 'secret' is the shared secret configured in the proxy. It must be null-
// terminated.
// 'user_ptr' will be passed to all future callbacks. However, when making uni_*
// calls, use the pointer returned by this function.
// In case of error, NULL will be returned and *err will be set to the error
// which occurred.
// err is allowed to be NULL, if so, no attempt at returning an error code will
// be made.
UniServer *uni_create(uint16_t port, const char *secret, void *user_ptr, UniError *err);

// Cleans up memory related to a server handle. Only needs to be called if
// uni_create() succeeds.
void uni_free(UniServer *server);

// Starts accepting connections.
bool uni_listen(UniServer *server);

// Polls the server for new I/O events. Should be called repeatedly and will
// block until a new event occurs.
// See also uni_try_poll()
void uni_poll(UniServer *server);

// Similar to uni_poll(), but will return immediately if there are no new
// events.
void uni_try_poll(UniServer *server);

// Note: All the strings in UniLoginProperty and UniLoginData with the exception
// of player_name are pointing to data within a received packet. Be sure to copy
// away if you need to keep them. None of the strings are null-terminated. Use
// the _len field to determine their length.

typedef struct {
    const char *name;
    int name_len;

    const char *value;
    int value_len;

    // Will be NULL if no signature was provided.
    const char *signature;
    int signature_len;
} UniLoginProperty;

typedef struct {
    // The username of the user who logged in. Not guaranteed to be a valid
    // username.
    char player_name[17];

    // The IPv4 address of the client. E.g. "127.0.0.1"
    const char *address_str;
    int address_len;

    // The raw data of the player's UUID.
    unsigned char *uuid_raw;

    // Additional properties provided by Mojang authentication. See
    // UniLoginProperty for more information.
    UniLoginProperty *properties;
    int num_properties;
} UniLoginData;

// Called when a player logs in. See UniLoginData for more information. The
// returned pointer will be used to identify this player in future callbacks
// unless it is NULL, which will deny the player's login.
// Warning: The connection pointer provided is not yet in the PLAY state. Do not
// use it for any reason beyond saving it for later use.
extern void *uni_on_login(void *server_user_ptr, UniConnection *conn, UniLoginData *data);

extern void uni_on_join(void *server_user_ptr, void *conn_user_ptr);

typedef struct {
    char* buf;
    int len;
    int write_idx;
} UniPacketOut;

// Writes a packet to the connection. Warning: This function is not thread-safe,
// nor will it attempt to queue packets if the previous packet hasn't been
// fully written. Synchronization is the responsibility of the caller. See also
// uni_on_write_finish()
void uni_write(UniConnection *conn, UniPacketOut *packet);

// Called when a write operation concludes. This function is guaranteed to be
// called from the same thread which called uni_poll() or uni_try_poll().
extern void uni_on_write_finish(void *user_ptr);

// Mark a connection as "to-be-closed". Note that this will not disconnect the
// client immediately, 
void uni_release(UniConnection *conn);

#endif // !UNI_H
