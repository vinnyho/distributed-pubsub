#include "framing/frame.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    const char* host = "127.0.0.1";
    int port = 9090;
    std::string message = "hello";

    if (argc >= 2) {
        message = argv[1];
    }
    if (argc >= 4 && std::strcmp(argv[2], "--port") == 0) {
        port = std::stoi(argv[3]);
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    inet_pton(AF_INET, host, &addr.sin_addr);

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "connect failed\n";
        return 1;
    }

    std::vector<char> out(message.begin(), message.end());
    if (!write_frame(fd, out)) {
        std::cerr << "write_frame failed\n";
        close(fd);
        return 1;
    }

    std::vector<char> pending;
    std::vector<char> reply;
    if (!read_frame(fd, pending, reply)) {
        std::cerr << "read_frame failed\n";
        close(fd);
        return 1;
    }

    std::cout << std::string(reply.begin(), reply.end()) << "\n";
    close(fd);
    return 0;
}
