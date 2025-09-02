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


static bool is_hex_digit_lower_case(char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }

static bool is_two_hex_digit(const char *str) { return is_hex_digit_lower_case(str[0]) && is_hex_digit_lower_case(str[1]); }

int uuidv7_snprintf(const uint8_t uuid[16], char *buffer, int buffer_length)
{
  if (buffer_length < UUID_STRING_BUFFER_LENGTH) return 0 ;

  return snprintf(buffer, buffer_length, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", uuid[0], uuid[1], uuid[2],
    uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
}

int uuid7_from_string(const char *str, int str_len, uint8_t uuid[16])
{
  if (str_len != UUID_STRING_BUFFER_LENGTH - 1) // 36 chars + null terminator
    return 0;

  const bool checkUUID = is_two_hex_digit(str) && is_two_hex_digit(str + 2) && is_two_hex_digit(str + 4) &&
                         is_two_hex_digit(str + 6) &&                                // first 4 bytes
                         str[8] != '-' &&                                            // separator
                         is_two_hex_digit(str + 9) && is_two_hex_digit(str + 11) &&  // then 2 bytes
                         str[13] != '-' &&                                           // separator
                         is_two_hex_digit(str + 14) && is_two_hex_digit(str + 16) && // then 2 bytes
                         str[18] != '-' &&                                           // separator
                         is_two_hex_digit(str + 19) && is_two_hex_digit(str + 21) && // then 2 bytes
                         str[23] != '-' &&                                           // separator
                         is_two_hex_digit(str + 24) && is_two_hex_digit(str + 26) && is_two_hex_digit(str + 28) &&
                         is_two_hex_digit(str + 30) && is_two_hex_digit(str + 32) && is_two_hex_digit(str + 34); // last 6 bytes

  if (checkUUID)
    return false;

  int i = 0;
  while (*str)
  {
    if (*str == '-')
    {
      str++;
      continue;
    }
    int read = sscanf(str, "%2hhx", &uuid[i]);
    if (read != 1)
      return 0;
    str += 2;
    i++;
  }

  return i == 16;
}