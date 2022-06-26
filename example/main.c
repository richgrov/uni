#include "uni.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    UniConnection *conn;
} Player;

void *uni_on_login(void *server, UniConnection *conn, UniLoginData *data) {
    printf("Player %s logged in.\n", data->player_name);

    Player *player = malloc(sizeof(Player));
    player->conn = conn;
    return player;
}

void uni_on_join(void *server, void *player) {
    puts("Player joined.");

    // Disconnect the client.
    uni_release(((Player *) player)->conn);
    free(player);
}

void uni_on_write_finish(void *user_ptr) {
}

int main(int argc, char** argv) {
    UniError err;
    UniServer *server = uni_create(25566, "your-forwarding-secret", NULL, &err);

    if (server == NULL) {
        printf("Couldn't start server: ");

        switch (err) {
            case UNI_ERR_IN_USE:
                puts("Address already in use.");
                break;

            case UNI_ERR_LIMITED:
                puts("Low or restricted resources.");
                break;

            case UNI_ERR_UNSUPPORTED:
                puts("System not supported.");
                break;

            case UNI_ERR_UNKNOWN:
                puts("Unknown error.");
                break;
        }

        return 1;
    }

    if (!uni_listen(server)) {
        return 1;
    }

    while (true) {
        uni_poll(server);
    }

    uni_free(server);
    return 0;
}
