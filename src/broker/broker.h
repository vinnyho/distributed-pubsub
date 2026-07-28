#pragma once

#include <atomic>

class Broker {
public:
    explicit Broker(int port);
    int run();

private:
    int listen_fd{-1};
    int port{0};
    std::atomic<bool> running{true};

    void setup_listen_socket();
    void handle_client(int client_fd);
};
