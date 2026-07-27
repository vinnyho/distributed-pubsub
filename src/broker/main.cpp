#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>

int main() {
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    std::cerr << "socket() failed: " << std::strerror(errno) << "\n";
    return 1;
  }

  int opt = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(9090);

  if (bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    std::cerr << "bind() failed: " << std::strerror(errno) << "\n";
    close(listen_fd);
    return 1;
  }

  if (listen(listen_fd, /*backlog=*/16) < 0) {
    std::cerr << "listen() failed: " << std::strerror(errno) << "\n";
    close(listen_fd);
    return 1;
  }

  std::cout << "listening on port 9090...\n";

  sockaddr_in client_addr{};
  socklen_t client_len = sizeof(client_addr);
  int client_fd = accept(listen_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
  if (client_fd < 0) {
    std::cerr << "accept() failed: " << std::strerror(errno) << "\n";
    close(listen_fd);
    return 1;
  }

  std::cout << "client connected\n";

  char buffer[1024];
  ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);
  if (n > 0) {
    std::cout << "received " << n << " bytes, echoing back\n";
    send(client_fd, buffer, static_cast<size_t>(n), 0);
  }

  close(client_fd);
  close(listen_fd);
  return 0;
}
