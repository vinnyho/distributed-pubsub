#include "broker.h"

#include "framing/frame.h"

#include <arpa/inet.h>
#include <csignal>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <thread>
#include <vector>

namespace {

std::atomic<bool>* g_running = nullptr;
int* g_listen_fd = nullptr;

void on_sigint(int) {
    if (g_running != nullptr) {
        g_running->store(false);
    }
    if (g_listen_fd != nullptr && *g_listen_fd >= 0) {
        shutdown(*g_listen_fd, SHUT_RDWR);
    }
}

}  // namespace

Broker::Broker(int port) : port(port) {}

int Broker::run() {
    setup_listen_socket();

    g_running = &running;
    g_listen_fd = &listen_fd;
    signal(SIGINT, on_sigint);

    std::cout << "listening on port " << port << "...\n";

    while (running.load()) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        int client_fd =
            accept(listen_fd, reinterpret_cast<sockaddr*>(&client_addr), &len);

        if (client_fd < 0) {
            if (!running.load()) {
                break;
            }
            continue;
        }

        std::thread(&Broker::handle_client, this, client_fd).detach();
    }

    close(listen_fd);
    listen_fd = -1;
    g_listen_fd = nullptr;
    g_running = nullptr;

    std::cout << "shut down\n";
    return 0;
}

void Broker::handle_client(int client_fd) {
    std::vector<char> pending;
    while (true) {
        std::vector<char> payload;
        if (!read_frame(client_fd, pending, payload)) {
            break;
        }
        write_frame(client_fd, payload);
    }
    close(client_fd);
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
