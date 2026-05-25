#ifndef SOCKET_CONTROL_H
#define SOCKET_CONTROL_H

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#include <pthread.h>
#include <cstring>
#include "Logger.h"

extern bool hackMap;
extern bool hackCamXa;
extern float camXaValue;
extern bool hackESP;
extern bool hackAntiDialog;
static bool main_thread_flag = true;

inline void* socket_server_thread(void* arg) {
    int server_fd, new_socket;
    struct sockaddr_un address;
    socklen_t addrlen = sizeof(address);

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        return nullptr;
    }

    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    const char* socket_name = "StarcoolPRO_socket";
    address.sun_path[0] = '\0';
    strcpy(address.sun_path + 1, socket_name);

    socklen_t len = offsetof(struct sockaddr_un, sun_path) + 1 + strlen(socket_name);

    if (::bind(server_fd, (struct sockaddr *)&address, len) < 0) {
        close(server_fd);
        return nullptr;
    }

    if (listen(server_fd, 5) < 0) {
        close(server_fd);
        return nullptr;
    }

    char buffer[1024] = {0};
    while (main_thread_flag) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0) continue;

        int valread = read(new_socket, buffer, 1024);
        if (valread > 0) {
            buffer[valread] = '\0';
            std::string msg(buffer);
            msg.erase(msg.find_last_not_of(" \n\r\t") + 1);

            if (msg.find("MAP_HACK:") == 0) {
                hackMap = (msg.substr(9) == "1");
            }
            else if (msg.find("CAMXA_VAL:") == 0) {
                camXaValue = std::stof(msg.substr(10));
                hackCamXa = (camXaValue > 0.0f);
            }
            else if (msg.find("ESP_LINE:") == 0) {
                hackESP = (msg.substr(9) == "1");
            }
            else if (msg.find("ANTI_DIALOG:") == 0) {
                hackAntiDialog = (msg.substr(12) == "1");
            }
        }
        close(new_socket);
        memset(buffer, 0, 1024);
    }
    close(server_fd);
    return nullptr;
}

#endif
