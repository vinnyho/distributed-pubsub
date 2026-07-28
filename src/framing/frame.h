#pragma once

#include <cstdint>
#include <vector>

// Wire format: [ u32 BE length N ][ N bytes payload ]

bool write_frame(int fd, const std::vector<char>& payload);
bool read_frame(int fd, std::vector<char>& pending, std::vector<char>& payload);
