#include <cstring>
#include <iostream>
#include <string>

#include "broker.h"

int main(int argc, char* argv[]) {
    int port = 9090;
    if (argc >= 3 && std::strcmp(argv[1], "--port") == 0) {
        port = std::stoi(argv[2]);
    }

    Broker broker(port);
    return broker.run();
}
