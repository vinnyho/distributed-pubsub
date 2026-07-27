#include "broker.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

Broker::Broker(int port) : port(port) {}

int Broker::run() {
    setup_listen_socket();
    std::cout << "listening on port " << port << "...\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        int client_fd =
            accept(listen_fd, reinterpret_cast<sockaddr*>(&client_addr), &len);

        char buffer[1024];
        ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);
        if (n > 0) {
            send(client_fd, buffer, static_cast<size_t>(n), 0);
        }
        close(client_fd);
    }

    return 0;
}

void Broker::setup_listen_socket() {
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    listen(listen_fd, 16);
}
