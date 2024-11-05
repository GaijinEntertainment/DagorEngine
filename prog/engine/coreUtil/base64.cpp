// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_base64.h>
#include <util/dag_globDef.h>

#include <math/dag_mathBase.h>


static const char sixtet_to_base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


void Base64::operator=(const char *s)
{
  int l = i_strlen(s);
  ensureAlloced(l + 1);
  strcpy((char *)data, s);
  len = l;
}

static void sixtetsForInt(uint8_t *out, int src)
{
  uint8_t *b = (uint8_t *)&src;
  out[0] = (b[0] & 0xfc) >> 2;
  out[1] = ((b[0] & 0x3) << 4) + ((b[1] & 0xf0) >> 4);
  out[2] = ((b[1] & 0xf) << 2) + ((b[2] & 0xc0) >> 6);
  out[3] = b[2] & 0x3f;
}

static int intForSixtets(uint8_t *in)
{
  int ret = 0;
  uint8_t *b = (uint8_t *)&ret;
  b[0] |= in[0] << 2;
  b[0] |= (in[1] & 0x30) >> 4;
  b[1] |= (in[1] & 0xf) << 4;
  b[1] |= (in[2] & 0x3c) >> 2;
  b[2] |= (in[2] & 0x3) << 6;
  b[2] |= in[3];
  return ret;
}


void Base64::encode(const uint8_t *from, int size)
{
  int i, j;
  unsigned long w;
  uint8_t *to;

  ensureAlloced(4 * (size + 3) / 3 + 2); // ratio and padding + trailing \0
  to = data;

  w = 0;
  i = 0;
  while (size > 0)
  {
    w |= *from << (i * 8);
    ++from;
    --size;
    ++i;
    if (size == 0 || i == 3)
    {
      uint8_t out[4];
      sixtetsForInt(out, w);
      for (j = 0; j * 6 < i * 8; ++j)
      {
        *to++ = sixtet_to_base64[out[j]];
      }
      if (size == 0)
      {
        for (j = i; j < 3; ++j)
        {
          *to++ = '=';
        }
      }
      w = 0;
      i = 0;
    }
  }

  *to++ = '\0';
  len = to - data;
}

int Base64::decodeLength(void) const
{
  if (len)
  {
    int encLen = len;
    while (encLen && data[encLen - 1] == '=')
      --encLen;

    return encLen * 3 / 4;
  }

  return 0;
}

int Base64::decode(uint8_t *to) const
{
  unsigned long w;
  int i, j;
  int n;
  static char base64_to_sixtet[256];
  static int tab_init = 0;
  uint8_t *from = data;

  if (!tab_init)
  {
    memset(base64_to_sixtet, 0, 256);
    for (i = 0; (j = sixtet_to_base64[i]) != '\0'; ++i)
    {
      base64_to_sixtet[j] = i;
    }
    tab_init = 1;
  }

  w = 0;
  i = 0;
  n = 0;
  uint8_t in[4] = {0, 0, 0, 0};
  while (*from != '\0' && *from != '=')
  {
    if (*from == ' ' || *from == '\n')
    {
      ++from;
      continue;
    }
    in[i] = base64_to_sixtet[*(unsigned char *)from];
    ++i;
    ++from;
    if (*from == '\0' || *from == '=' || i == 4)
    {
      w = intForSixtets(in);
      for (j = 0; j < i - 1; ++j)
      {
        *to++ = w & 0xff;
        ++n;
        w >>= 8;
      }
      i = 0;
      w = 0;
    }
  }
  return n;
}

// void Base64::encode( const String& src )
//{
//   encode( (const uint8_t *)src.c_str(), src.length() );
// }

void Base64::decode(String &dest) const
{
  int decLen = decodeLength();
  uint8_t *buf = new uint8_t[decLen + 1];

  int decodeLen = decode(buf);
  int out = min(decLen, decodeLen);

  buf[out] = '\0';
  dest = (const char *)buf;

  delete[] buf;
}
