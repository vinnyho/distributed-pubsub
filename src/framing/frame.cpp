#include "framing/frame.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>

namespace {

constexpr uint32_t kMaxFrameSize = 1024 * 1024;

bool send_all(int fd, const char* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, data + sent, len - sent, 0);
        if (n <= 0) {
            return false;
        }
        sent += static_cast<size_t>(n);
    }
    return true;
}

bool recv_some(int fd, std::vector<char>& pending) {
    char chunk[4096];
    ssize_t n = recv(fd, chunk, sizeof(chunk), 0);
    if (n <= 0) {
        return false;
    }
    pending.insert(pending.end(), chunk, chunk + n);
    return true;
}

bool try_extract_frame(std::vector<char>& pending, std::vector<char>& payload) {
    if (pending.size() < 4) {
        return false;
    }

    uint32_t net_len{};
    std::memcpy(&net_len, pending.data(), 4);
    uint32_t len = ntohl(net_len);
    if (len > kMaxFrameSize) {
        return false;
    }

    const size_t frame_size = 4 + len;
    if (pending.size() < frame_size) {
        return false;
    }

    payload.assign(pending.begin() + 4, pending.begin() + frame_size);
    pending.erase(pending.begin(), pending.begin() + static_cast<std::ptrdiff_t>(frame_size));
    return true;
}

}  // namespace

bool write_frame(int fd, const std::vector<char>& payload) {
    const uint32_t len = static_cast<uint32_t>(payload.size());
    const uint32_t net_len = htonl(len);

    if (!send_all(fd, reinterpret_cast<const char*>(&net_len), 4)) {
        return false;
    }
    if (len == 0) {
        return true;
    }
    return send_all(fd, payload.data(), payload.size());
}

bool read_frame(int fd, std::vector<char>& pending, std::vector<char>& payload) {
    while (!try_extract_frame(pending, payload)) {
        if (pending.size() >= 4) {
            uint32_t net_len{};
            std::memcpy(&net_len, pending.data(), 4);
            if (ntohl(net_len) > kMaxFrameSize) {
                return false;
            }
        }
        if (!recv_some(fd, pending)) {
            return false;
        }
    }
    return true;
}
