#include "uni.h"

#include <stdio.h>

void *uni_on_login(UniLoginData *data) {
    printf("Player %.*s logged in.\n", data->name_len, data->player_name);
    return (void *) 1;
}

int main(int argc, char** argv) {
    UniError err;
    UniServer *server = uni_create(25566, "your-forwarding-secret", &err);

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

    uni_run(server);
    return 0;
}
