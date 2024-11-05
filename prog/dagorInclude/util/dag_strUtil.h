//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <util/dag_baseDef.h>
#include <generic/dag_tab.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

inline bool str_ends_with_c(const char *str, const char *suffix, int str_len = -1, int suffix_len = -1)
{
  if (str_len < 0)
    str_len = (int)strlen(str);
  if (suffix_len < 0)
    suffix_len = (int)strlen(suffix);
  return str_len >= suffix_len && strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}

template <size_t N>
inline bool str_ends_with(const char *str, const char (&suff)[N], int slen = -1)
{
  return str_ends_with_c(str, &suff[0], slen, (int)N - 1);
}

//! convert binary data to string in lowercased hexadecimal format
template <typename S>
static inline const char *data_to_str_hex(S &dest, const void *src_data, size_t src_data_len)
{
  dest.allocBuffer(int(src_data_len * 2 + 1));
  for (size_t i = 0; i < src_data_len; i++)
    SNPRINTF((&dest[i * 2]), 3, "%02x", ((unsigned char *)src_data)[i]);
  return dest.str();
}
static inline const char *data_to_str_hex_buf(char *dest, size_t dest_len, const void *src_data, size_t src_data_len)
{
  if (src_data_len * 2 + 1 > dest_len)
    return NULL;
  for (size_t i = 0; i < src_data_len; i++)
    SNPRINTF((&dest[i * 2]), 3, "%02x", ((unsigned char *)src_data)[i]);
  dest[src_data_len * 2] = '\0';
  return dest;
}

//! convert string in hexadecimal format to binary data
static inline const void *str_hex_to_data_buf(void *dest_buf, size_t dest_buf_len, const char *s, size_t *out_len = NULL,
  int s_len = -1)
{
  size_t len = s_len >= 0 ? s_len : strlen(s);
  if (len & 1) // we require even number of characters (each 2 characters combine 1 byte of data)
  {
    if (out_len)
      *out_len = 0;
    return NULL;
  }
  len /= 2;
  if (out_len)
    *out_len = len;
  if (len > dest_buf_len)
    len = dest_buf_len;
  for (unsigned char *p = (unsigned char *)dest_buf, *pe = p + len; p < pe; p++)
  {
    unsigned char c = 0;
    if (*s >= '0' && *s <= '9')
      c = *s - '0';
    else if (*s >= 'a' && *s <= 'f')
      c = *s - 'a' + 10;
    else if (*s >= 'A' && *s <= 'F')
      c = *s - 'A' + 10;
    s++;
    c <<= 4;
    if (*s >= '0' && *s <= '9')
      c |= *s - '0';
    else if (*s >= 'a' && *s <= 'f')
      c |= *s - 'a' + 10;
    else if (*s >= 'A' && *s <= 'F')
      c |= *s - 'A' + 10;
    s++;
    *p = c;
  }
  return dest_buf;
}
template <typename T>
static inline const void *str_hex_to_data(T &dest, const char *s)
{
  size_t len = strlen(s);
  if (len & 1) // we require even number of characters (each 2 characters combine 1 byte of data)
    return NULL;
  if ((len / 2) % elem_size(dest))
    return NULL;
  dest.resize(len / 2 / elem_size(dest));
  return str_hex_to_data_buf(dest.data(), data_size(dest), s, NULL, (int)len);
}

template <class Func>
bool str_split(const char *str, char term, Func func)
{
  const char *sPtr = str, *ePtr = str, *cPtr = str;
  while (cPtr)
  {
    if (*cPtr == term || !*cPtr)
    {
      if (!func(sPtr, (ePtr - sPtr) + 1))
        return false;
      ePtr = sPtr = cPtr + 1;
    }
    else if (*cPtr == ' ')
    {
      if (*sPtr == ' ')
        ++sPtr;
    }
    else
      ePtr = cPtr;
    if (!*cPtr)
      break;
    ++cPtr;
  }
  return true;
}

static inline bool parse_url(const char *url, char *host_buffer, int host_buffer_size, int &port)
{
  G_ASSERT(url);

  size_t actualHostSize;

  const char *delimiter = strrchr(url, ':');
  if (delimiter)
  {
    actualHostSize = delimiter - url;

    char *endptr = NULL;
    port = strtol(delimiter + 1, &endptr, 10);
    if (endptr && *endptr)
      return false;
  }
  else
  {
    actualHostSize = strlen(url);
  }

  if (actualHostSize > host_buffer_size - 1)
    return false;


  strncpy(host_buffer, url, actualHostSize);
  host_buffer[actualHostSize] = 0;

  return true;
}

static inline int parse_string_list(Tab<String> &list, const char *str, const char *prefix = "", const char *suffix = "")
{
  list.clear();
  while (const char *p = strchr(str, '|'))
  {
    if (p - str > 0)
      list.push_back().printf(0, "%s%.*s%s", prefix, p - str, str, suffix);
    str = p + 1;
  }
  if (*str)
    list.push_back().printf(0, "%s%s%s", prefix, str, suffix);
  return list.size();
}

static inline String urlencode(const String &src)
{
  String ret;
  char sym[8] = {0};

  for (uint32_t i = 0; i < src.length(); ++i)
    if (isalnum(src[i]) || src[i] == '.')
      ret += src[i];
    else if (src[i] == ' ')
      ret += '+';
    else
    {
      snprintf(sym, sizeof(sym), "%%%02x", src[i] & 0xFF);
      ret += sym;
    }

  return ret;
}
