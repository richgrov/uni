#include "uni.h"

#include <stddef.h>

int main(int argc, char** argv) {
    UniServer *server = uni_create(25565);
    if (server != NULL) {
        uni_run(server);
    }
}
