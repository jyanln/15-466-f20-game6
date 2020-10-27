#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>

static inline void serialize_int(int32_t in_x, std::vector<char>::iterator buffer) {
  uint32_t x = htonl((uint32_t) in_x);
  buffer[0] = x >> 24;
  buffer[1] = x >> 16;
  buffer[2] = x >> 8;
  buffer[3] = x;
}

static inline void serialize_float(float in_x, std::vector<char>::iterator buffer) {
  int32_t x = reinterpret_cast<int32_t&>(in_x);
  return serialize_int(x, buffer);
}

static inline int32_t deserialize_int(std::vector<char>::iterator buffer) {
  return ntohl(
        (((uint32_t) buffer[0]) << 24) |
        (((uint32_t) buffer[1]) << 16) |
        (((uint32_t) buffer[2]) << 8) |
        ((uint32_t) buffer[3]));
}

static inline float deserialize_float(std::vector<char>::iterator buffer) {
  int32_t x = deserialize_int(buffer);
  return reinterpret_cast<float&>(x);
}
