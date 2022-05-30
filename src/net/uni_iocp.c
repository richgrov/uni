#include "uni_networking.h"

#include <MSWSock.h>
#include <WinSock2.h>

bool uni_net_init(UniServer *server, uint16_t port, UniError *err) {
    WSADATA wsa_data;
    int ret = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (ret != 0) {
        if (err != NULL) {
            switch (ret) {
                case WSAVERNOTSUPPORTED:
                    *err = UNI_ERR_UNSUPPORTED;
                    break;

                case WSAEPROCLIM:
                    *err = UNI_ERR_LIMITED;
                    break;

                default:
                    *err = UNI_ERR_UNKNOWN;
                    break;
            }
        }
        return false;
    }

    server->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    server->socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (server->socket == INVALID_SOCKET) {
        if (err != NULL) {
            switch (WSAGetLastError()) {
                case WSAEMFILE:
                case WSAENOBUFS:
                    *err = UNI_ERR_LIMITED;
                    break;

                case WSAEPROTONOSUPPORT:
                case WSAESOCKTNOSUPPORT:
                    *err = UNI_ERR_UNSUPPORTED;
                    break;

                default:
                    *err = UNI_ERR_UNKNOWN;
                    break;
            }
        }
        return false;
    }

    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server->socket, &addr, sizeof(addr)) == SOCKET_ERROR) {
        if (err != NULL) {
            switch (WSAGetLastError()) {
                case WSAEACCES:
                case WSAENOBUFS:
                    *err = UNI_ERR_LIMITED;
                    break;

                case WSAEADDRINUSE:
                    *err = UNI_ERR_IN_USE;
                    break;

                default:
                    *err = UNI_ERR_UNKNOWN;
                    break;
            }
        }
        return false;
    }

    return true;
}

bool uni_listen(UniServer *server) {
    return listen(server->socket, UNI_CONN_BACKLOG) != -1;
}

static void uni_do_poll(UniServer *server) {
    // TODO
}

void uni_poll(UniServer *server) {
    uni_do_poll(server);
}

void uni_try_poll(UniServer *server) {
    uni_do_poll(server);
}
