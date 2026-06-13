#include <chrono>
#include <cstdint>
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

// String offsets of each UUID byte in xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
static const uint8_t kUuidHexOffsets[16] = {0, 2, 4, 6, 9, 11, 14, 16, 19, 21, 24, 26, 28, 30, 32, 34};
static const uint8_t kUuidDashOffsets[4] = {8, 13, 18, 23};

static void write_hex_byte(char *p, uint8_t v)
{
  static const char hex[] = "0123456789abcdef";
  p[0] = hex[v >> 4];
  p[1] = hex[v & 0xF];
}

int uuidv7_snprintf(const uint8_t uuid[16], char *buffer, int buffer_length)
{
  if (buffer_length < UUID_STRING_BUFFER_LENGTH)
    return 0;

  for (int i = 0; i < 16; i++)
    write_hex_byte(buffer + kUuidHexOffsets[i], uuid[i]);
  for (int i = 0; i < 4; i++)
    buffer[kUuidDashOffsets[i]] = '-';
  buffer[36] = '\0';
  return 36;
}

// Expected format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (36 chars)
int uuid7_from_string(const char *str, int str_len, uint8_t uuid[16])
{
  if (str_len != UUID_STRING_BUFFER_LENGTH - 1)
    return 0;

  for (int i = 0; i < 4; i++)
    if (str[kUuidDashOffsets[i]] != '-')
      return 0;

  for (int i = 0; i < 16; i++)
    if (!parse_hex_byte(str + kUuidHexOffsets[i], uuid[i]))
      return 0;

  return 1;
}