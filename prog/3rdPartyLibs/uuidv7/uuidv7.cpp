#include <chrono>
#include <cstdint>
#include <cstdio>
#include <random>
#include <functional>
#include <algorithm>
#include "uuidv7.h"

void uuidv7(uint8_t value[16])
{
  // random bytes
  std::random_device rd;
  uint8_t random_bytes[16];
  std::generate(std::begin(random_bytes), std::end(random_bytes), std::ref(rd));

  std::copy(std::begin(random_bytes), std::end(random_bytes), value);

  // current timestamp in ms
  auto now = std::chrono::system_clock::now();
  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

  // timestamp
  value[0] = (millis >> 40) & 0xFF;
  value[1] = (millis >> 32) & 0xFF;
  value[2] = (millis >> 24) & 0xFF;
  value[3] = (millis >> 16) & 0xFF;
  value[4] = (millis >> 8) & 0xFF;
  value[5] = millis & 0xFF;

  // version and variant
  value[6] = (value[6] & 0x0F) | 0x70;
  value[8] = (value[8] & 0x3F) | 0x80;
}


static int8_t hex_nibble(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  c |= 0x20; // 'A'..'F' -> 'a'..'f'
  if (c >= 'a' && c <= 'f')
    return c - ('a' - 10);
  return -1;
}

static bool parse_hex_byte(const char *s, uint8_t &out)
{
  int8_t hi = hex_nibble(s[0]);
  int8_t lo = hex_nibble(s[1]);
  if ((hi | lo) < 0)
    return false;
  out = (uint8_t)((hi << 4) | lo);
  return true;
}

int uuidv7_snprintf(const uint8_t uuid[16], char *buffer, int buffer_length)
{
  if (buffer_length < UUID_STRING_BUFFER_LENGTH)
    return 0;

  return snprintf(buffer, buffer_length, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", uuid[0], uuid[1],
    uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14],
    uuid[15]);
}

// Expected format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (36 chars)
int uuid7_from_string(const char *str, int str_len, uint8_t uuid[16])
{
  if (str_len != UUID_STRING_BUFFER_LENGTH - 1)
    return 0;

  if (str[8] != '-' || str[13] != '-' || str[18] != '-' || str[23] != '-')
    return 0;

  // Fixed offsets -- UUID format is always xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
  if (!parse_hex_byte(str + 0, uuid[0]) || !parse_hex_byte(str + 2, uuid[1]) || !parse_hex_byte(str + 4, uuid[2]) ||
      !parse_hex_byte(str + 6, uuid[3]) || !parse_hex_byte(str + 9, uuid[4]) || !parse_hex_byte(str + 11, uuid[5]) ||
      !parse_hex_byte(str + 14, uuid[6]) || !parse_hex_byte(str + 16, uuid[7]) || !parse_hex_byte(str + 19, uuid[8]) ||
      !parse_hex_byte(str + 21, uuid[9]) || !parse_hex_byte(str + 24, uuid[10]) || !parse_hex_byte(str + 26, uuid[11]) ||
      !parse_hex_byte(str + 28, uuid[12]) || !parse_hex_byte(str + 30, uuid[13]) || !parse_hex_byte(str + 32, uuid[14]) ||
      !parse_hex_byte(str + 34, uuid[15]))
    return 0;

  return 1;
}