#pragma once

class Broker {
public:
    explicit Broker(int port);
    int run();

private:
    int listen_fd{-1};
    int port{0};
    void setup_listen_socket();
};
