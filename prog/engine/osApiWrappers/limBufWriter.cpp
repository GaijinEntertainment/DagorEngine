// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/limBufWriter.h>
#include <stdio.h>

void LimitedBufferWriter::avprintf(const char *fmt, va_list ap)
{
  if (bufLeft <= 0)
    return;

  int cnt = _vsnprintf(buf, bufLeft, fmt, ap);
  if (cnt >= 0)
  {
    bufLeft -= cnt;
    buf += cnt;
  }
  else
  {
    buf[bufLeft - 1] = '\0';
    bufLeft = 0;
  }
}

void LimitedBufferWriter::append(const char *str, int char_count)
{
  if (bufLeft <= 0)
    return;

  if (char_count + 1 > bufLeft)
  {
    char_count = bufLeft - 1;
    memcpy(buf, str, char_count);
    buf += char_count;
    *buf = '\0';
    bufLeft = 0;
  }
  else
  {
    memcpy(buf, str, char_count);
    buf += char_count;
    bufLeft -= char_count;
  }
}
