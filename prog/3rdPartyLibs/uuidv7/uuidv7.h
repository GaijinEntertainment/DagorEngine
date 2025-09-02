#pragma once
#include <stdint.h>

// UUID string buffer length including null terminator
// format: "XXXX-XX-XX-XX-XXXXXX" (36 chars + null terminator)
const int UUID_STRING_BUFFER_LENGTH = 37;

// return new uuidv7 in 16 byte array
void uuidv7(uint8_t uuid[16]);

// convert uuid to string in format "XXXX-XX-XX-XX-XXXXXX", buffer must be at least UUID_STRING_BUFFER_LENGTH
int uuidv7_snprintf(const uint8_t uuid[16], char *buffer, int buffer_length);

// parse 36 char string to uuidv7, return 1 if success and 0 if string is invalid (e.g. not 36 chars or not in valid format)
int uuid7_from_string(const char *str, int str_len, uint8_t uuid[16]);