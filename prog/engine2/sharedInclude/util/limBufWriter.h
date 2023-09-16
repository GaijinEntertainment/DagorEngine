
#ifndef __DAGOR_LIMBUFWRITER_H
#define __DAGOR_LIMBUFWRITER_H
#pragma once

#include <stdarg.h>
#include <string.h>
#include <util/dag_safeArg.h>

#include <supp/dag_define_COREIMP.h>

//! this class allows safe formatted printing to fixed-sized buffer
class LimitedBufferWriter
{
  char *buf;
  int bufLeft;

public:
  LimitedBufferWriter(char *_buf, int buf_size) { reset(_buf, buf_size); }

  inline void reset(char *_buf, int buf_size)
  {
    buf = _buf;
    bufLeft = buf_size;
    if (bufLeft > 0)
      buf[0] = '\0';
  }

  inline void done()
  {
    if (bufLeft > 0)
    {
      buf[0] = '\0';
      bufLeft = 0;
    }
  }

// these always write terminating zero
#define DSA_OVERLOADS_PARAM_DECL
#define DSA_OVERLOADS_PARAM_PASS
  DECLARE_DSA_OVERLOADS_FAMILY(inline void aprintf, inline void avprintf, avprintf);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

  KRNLIMP void avprintf(const char *fmt, va_list ap);
  inline void mixed_avprintf(const char *fmt, const void *arg, int anum)
  {
    if (bufLeft <= 0)
      return;
    int cnt = DagorSafeArg::mixed_print_fmt(buf, bufLeft, fmt, arg, anum);
    bufLeft -= cnt;
    buf += cnt;
  }

  // these don't write terminating zero
  inline void append(const char *str) { append(str, i_strlen(str)); }
  void append(const char *str, int char_count);

  inline int getBufLeft() const { return bufLeft; }
  inline char *getBufPos() const { return buf; }

  inline void incPos(int buf_used)
  {
    if (buf_used < bufLeft)
    {
      buf += buf_used;
      bufLeft -= buf_used;
    }
    else if (bufLeft > 0)
    {
      buf[bufLeft - 1] = '\0';
      bufLeft = 0;
    }
  }
};

inline void LimitedBufferWriter::avprintf(const char *fmt, const DagorSafeArg *arg, int anum)
{
  if (bufLeft <= 0)
    return;
  int cnt = DagorSafeArg::print_fmt(buf, bufLeft, fmt, arg, anum);
  bufLeft -= cnt;
  buf += cnt;
}

#include <supp/dag_undef_COREIMP.h>

#endif
