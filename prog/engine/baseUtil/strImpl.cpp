// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_string.h>
#include <util/dag_globDef.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <debug/dag_debug.h>

int String::insert(int at, const char *p, int n)
{
  if (empty())
  {
    if (n <= 0)
      return 0;
    resize(n + 1);
    (p) ? memcpy(data(), p, n) : memset(data(), ' ', n);
    *(data() + n) = '\0';
    return 0;
  }

  if (at > size() - 1)
    at = size() - 1;
  else if (at < 0)
    at = 0;
  if (n <= 0)
    return at;

  int idx = insert_items(*this, at, n);
  (p) ? memcpy(data() + idx, p, n) : memset(data() + idx, ' ', n);
  return idx;
}

int String::append(const char *p, int n)
{
  if (n <= 0)
    return size() - 1;

  if (empty())
  {
    resize(n + 1);
    (p) ? memcpy(data(), p, n) : memset(data(), ' ', n);
    *(data() + n) = '\0';
    return 0;
  }

  int idx = insert_items(*this, size() - 1, n);
  (p) ? memcpy(data() + idx, p, n) : memset(data() + idx, ' ', n);
  return idx;
}

int String::cvprintf(int est_sz, const char *fmt, va_list ap)
{
  resize(est_sz);

  DAG_VACOPY(args_copy, ap);
  int cnt = vsnprintf(data(), capacity(), fmt, args_copy);
  DAG_VACOPYEND(args_copy);

  if (cnt >= capacity())
  {
    resize(cnt + 1);
    cnt = vsnprintf(data(), capacity(), fmt, ap);
  }
#ifdef _MSC_VER
  else if (cnt < 0)
  {
    DAG_VACOPY(args_copy2, ap);
    cnt = _vscprintf(fmt, args_copy2);
    DAG_VACOPYEND(args_copy2);
    if (cnt >= 0)
    {
      resize(cnt + 1);
      cnt = _vsnprintf(data(), capacity(), fmt, ap);
    }
  }
#endif

  if (cnt >= 0)
  {
    resize(cnt + 1);
    if (size() < capacity())
      shrink_to_fit();
  }
  else
    resize(0);

  return cnt;
}

int String::cvprintf(int at, int est_sz, const char *fmt, va_list ap)
{
  String s(dag::get_allocator(*this));
  int cnt = s.cvprintf(est_sz, fmt, ap);
  if (cnt < 0)
    return cnt;

  insert(at, &s[0], cnt);
  return cnt;
}

int String::cvaprintf(int est_sz, const char *fmt, va_list ap) { return cvprintf(length(), est_sz, fmt, ap); }

void String::ctor_vprintf(int est_sz, const char *fmt, const DagorSafeArg *arg, int anum)
{
  dag::set_allocator(*this, strmem);
  vprintf(est_sz, fmt, arg, anum);
}
int String::vprintf(int, const char *fmt, const DagorSafeArg *arg, int anum)
{
  int sz = DagorSafeArg::count_len(fmt, arg, anum);
  resize(sz + 1);
  return DagorSafeArg::print_fmt(data(), sz + 1, fmt, arg, anum);
}
int String::vprintf(int at, int est_sz, const char *fmt, const DagorSafeArg *arg, int anum)
{
  String s;
  int cnt = s.vprintf(est_sz, fmt, arg, anum);
  insert(at, &s[0], cnt);
  return cnt;
}
int String::avprintf(int, const char *fmt, const DagorSafeArg *arg, int anum)
{
  size_t prev_len = length() ? strlen(data()) : 0;
  int sz = DagorSafeArg::count_len(fmt, arg, anum);
  resize(prev_len + sz + 1);
  return DagorSafeArg::print_fmt(data() + prev_len, sz + 1, fmt, arg, anum);
}


String &String::toLower()
{
  dd_strlwr(data());
  return *this;
}

String &String::toUpper()
{
  dd_strupr(data());
  return *this;
}

bool String::prefix(const char *pref) const
{
  const char *s = str();
  while (*pref)
    if (*(s++) != *(pref++))
      return false;
  return true;
}

bool String::suffix(const char *suff) const
{
  const char *s = str();
  size_t suflen = strlen(suff);
  size_t slen = strlen(s);
  if (slen < suflen)
    return false;

  return strcmp(s + slen - suflen, suff) == 0;
}

String &String::replace(const char *old_str, const char *new_str)
{
  int old_str_len = i_strlen(old_str), new_str_len = i_strlen(new_str);
  String dst;
  const char *first = begin(), *last = begin(), *p = old_str;

  while (*last)
  {
    if (*p == 0)
    {
      dst += String::mk_sub_str(first, last - old_str_len);
      dst.append(new_str, new_str_len);
      first = last;
      p = old_str;
    }
    else if (*last != *p)
      p = old_str;
    else
      p++;
    last++;
  }

  if (*p == 0)
  {
    dst += String::mk_sub_str(first, last - old_str_len);
    dst.append(new_str, new_str_len);
  }
  else
    dst += String::mk_sub_str(first, last);

  return operator=(dst);
}

String &String::replaceAll(const char *old_str, const char *new_str)
{
  String dst;
  dst.reserve(size());
  const char *s = str();
  int oldStrLen = i_strlen(old_str);
  int newStrLen = i_strlen(new_str);
  int copyCount = 0;

  while (*s)
  {
    if (*s == *old_str)
    {
      bool found = true;
      for (int i = 0; i < oldStrLen; i++)
        if (s[i] != old_str[i])
        {
          found = false;
          break;
        }

      if (found)
      {
        if (copyCount > 0)
        {
          dst.append(s - copyCount, copyCount);
          copyCount = 0;
        }

        dst.append(new_str, newStrLen);
        s += oldStrLen;
      }
      else
      {
        s++;
        copyCount++;
      }
    }
    else
    {
      s++;
      copyCount++;
    }
  }

  if (copyCount > 0)
    dst.append(s - copyCount, copyCount);

  dst.push_back(0);

  return operator=(dst);
}


const char *String::find(const char *simple, const char *pos, bool forward) const
{
  G_VERIFY(forward == true);
  return strstr(pos ? pos : begin(), simple);
}

const char *String::find(char ch, const char *pos, bool forward) const
{
  if (forward)
  {
    if (pos == NULL)
      pos = begin();
    for (; pos < end(); pos++)
      if (*pos == ch)
        return pos;
  }
  else
  {
    if (pos == NULL)
      pos = end();
    for (; --pos >= begin();)
      if (*pos == ch)
        return pos;
  }
  return NULL;
}

String stackhlp_get_call_stack_str(const void *const *stack, unsigned max_size)
{
  static constexpr int MAX_SZ = 8192 - 4;
  String s;
  s.resize(MAX_SZ);
  stackhlp_get_call_stack(s.data(), MAX_SZ - 1, stack, max_size);
  s.shrink_to_fit();
  return s;
}

String stackhelp::ext::get_call_stack_str(stackhelp::CallStackInfo stack, stackhelp::ext::CallStackInfo ext_stack)
{
  static constexpr unsigned max_size = 8 * 1024;
  String s;
  s.resize(max_size);
  get_call_stack(s.data(), unsigned(s.size()), stack, ext_stack);
  return s;
}

const char *stackhelp::ext::get_call_stack(char *buf, unsigned max_buf, stackhelp::CallStackInfo stack,
  stackhelp::ext::CallStackInfo ext_stack)
{
  auto len = i_strlen(get_call_stack(buf, max_buf, stack));
  len += ext_stack(buf + len, max_buf - len);
  buf[len < (max_buf - 1) ? len : (max_buf - 1)] = '\0';
  return buf;
}

#define EXPORT_PULL dll_pull_baseutil_strImpl
#include <supp/exportPull.h>
