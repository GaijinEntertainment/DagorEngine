//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

/// @addtogroup containers
/// @{

/// @addtogroup strings
/// @{

/** @file
  text #String object, derived from #Tab template
  @sa Tab
*/

#include <stdarg.h>
#include <string.h>

#ifdef __cplusplus

#include <generic/dag_tab.h>
#include <util/dag_safeArg.h>
#include <supp/dag_define_COREIMP.h>
#include <debug/dag_assert.h>

extern "C" KRNLIMP void dd_simplify_fname_c(char *fn);


/**
  dynamic text string object, derived from #Tab template;
  it has useful printf() methods, including constructor,
  these methods take @b est_sz argument, set it to
  "expected" string size
*/
class String : public Tab<char>
{
public:
  typedef char value_type;

  String() : Tab<char>(strmem) {}
  String(const String &s) : Tab<char>(s) {}
  String(String &&s) : Tab<char>(static_cast<Tab<char> &&>(s)) {}
  explicit String(IMemAlloc *m) : Tab<char>(m) {}
  String(const typename Tab<char>::allocator_type &allocator) : Tab<char>(allocator) {}

  explicit String(const char *s, IMemAlloc *m = strmem) : Tab<char>(m)
  {
    if (s && *s)
    {
      int l = i_strlen(s) + 1;
      resize(l);
      memcpy(data(), s, l);
    }
  }

  String(const char *s, int sz) : Tab<char>(strmem) { setStr(s, sz); }

/// printf() constructor
#define DSA_OVERLOADS_PARAM_DECL int est_sz,
#define DSA_OVERLOADS_PARAM_PASS est_sz,
  DECLARE_DSA_OVERLOADS_FAMILY(String, KRNLIMP void ctor_vprintf, ctor_vprintf)
  DECLARE_DSA_OVERLOADS_FAMILY(int printf, KRNLIMP int vprintf, return vprintf);
  DECLARE_DSA_OVERLOADS_FAMILY(int aprintf, KRNLIMP int avprintf, return avprintf);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

#define DSA_OVERLOADS_PARAM_DECL int at, int est_sz,
#define DSA_OVERLOADS_PARAM_PASS at, est_sz,
  DECLARE_DSA_OVERLOADS_FAMILY(int printf, KRNLIMP int vprintf, return vprintf);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS


  void setStr(const char *s)
  {
    if (s && *s)
    {
      int l = i_strlen(s) + 1;
      resize(l);
      memcpy(data(), s, l);
    }
    else
    {
      resize(0);
      if (capacity() > 0)
        *data() = 0;
    }
  }
  void setStr(const char *s, int l)
  {
    if (l > 0 && s && *s)
    {
      resize(l + 1);
      memcpy(data(), s, l);
      *(data() + l) = '\0';
    }
    else
    {
      resize(0);
      if (capacity() > 0)
        *data() = 0;
    }
  }
  void setSubStr(const char *begin, const char *end) { setStr(begin, end - begin); }
  void setStrCat(const char *s1, const char *s2)
  {
    int l1 = s1 ? i_strlen(s1) : 0;
    int l2 = s2 ? i_strlen(s2) : 0;
    resize(l1 + l2 + 1);
    if (l1)
      memcpy(data(), s1, l1);
    if (l2)
      memcpy(data() + l1, s2, l2);
    *(data() + l1 + l2) = '\0';
  }
  void setStrCat3(const char *s1, const char *s2, const char *s3)
  {
    int l1 = i_strlen(s1);
    int l2 = i_strlen(s2);
    int l3 = i_strlen(s3);
    resize(l1 + l2 + l3 + 1);
    char *p = data();
    memcpy(p, s1, l1);
    p += l1;
    memcpy(p, s2, l2);
    p += l2;
    memcpy(p, s3, l3);
    p += l3;
    *p = '\0';
  }
  void setStrCat4(const char *s1, const char *s2, const char *s3, const char *s4)
  {
    int l1 = i_strlen(s1);
    int l2 = i_strlen(s2);
    int l3 = i_strlen(s3);
    int l4 = i_strlen(s4);
    resize(l1 + l2 + l3 + l4 + 1);
    char *p = data();
    memcpy(p, s1, l1);
    p += l1;
    memcpy(p, s2, l2);
    p += l2;
    memcpy(p, s3, l3);
    p += l3;
    memcpy(p, s4, l4);
    p += l4;
    *p = '\0';
  }
  int updateSz()
  {
    int l = i_strlen(data());
    Tab<char>::resize(l + 1);
    return l;
  };

  __forceinline const char &operator[](int idx) const { return data()[idx]; }
  __forceinline char &operator[](int idx) { return data()[idx]; }
  String &operator=(const char *s)
  {
    setStr(s);
    return *this;
  }
  String &operator=(const String &s)
  {
    setStr(s);
    return *this;
  }
  String &operator=(String &&s)
  {
    Tab<char>::swap(s);
    return *this;
  }

  /// text string length
  bool empty() const { return (size() < 1) || !front(); }
  int length() const { return (size() < 1) ? 0 : int(size() - 1); }

  /// explicit cast to char* and const char*: empty string always returns ""
  char *str() { return size() > 0 ? data() : (char *)""; }
  const char *str() const { return size() > 0 ? data() : ""; }
  /// aliases for EASTL compatibility
  char *c_str() { return str(); }
  const char *c_str() const { return str(); }

  /// cast operator for convenience
  operator char *() { return str(); }
  operator const char *() const { return str(); }

  inline void pop_back()
  {
    Tab<char>::pop_back();
    if (!Tab<char>::empty())
      back() = '\0';
  }
  inline void pop() { String::pop_back(); }
  inline void chop(int n)
  {
    if (n + 1 < size())
      erase(Tab<char>::end() - n - 1, Tab<char>::end() - 1);
    else if (n >= 0)
      setStr(nullptr);
  }

  /// insert text - returns position or -1 on error
  KRNLIMP int insert(int at, const char *p, int n);
  KRNLIMP int insert(int at, const char *p) { return insert(at, p, p ? i_strlen(p) : 0); }

  /// append text - returns position or -1 on error
  KRNLIMP int append(const char *p, int n);
  KRNLIMP int append(const char *p) { return append(p, p ? i_strlen(p) : 0); }

  KRNLIMP int cvprintf(int est_sz, const char *fmt, va_list);         ///< replaces string
  KRNLIMP int cvprintf(int at, int est_sz, const char *fmt, va_list); ///< inserts text
  KRNLIMP int cvaprintf(int est_sz, const char *fmt, va_list);        ///< appends text

  String operator+(const String &s) const
  {
    if (empty())
      return s;
    if (s.empty())
      return *this;

    String sum;
    size_t l1 = length(), l2 = s.length();

    sum.resize(l1 + l2 + 1);
    memcpy(sum.data(), data(), l1);
    memcpy(sum.data() + l1, s.data(), l2);
    sum.back() = '\0';
    return sum;
  }

  String operator+(const char *s) const
  {
    if (empty())
      return String(s);
    if (!s || !s[0])
      return *this;

    String sum;
    size_t l1 = length(), l2 = strlen(s);

    sum.resize(l1 + l2 + 1);
    memcpy(sum.data(), data(), l1);
    memcpy(sum.data() + l1, s, l2);
    sum.back() = '\0';
    return sum;
  }

  /// appends text
  String &operator+=(char ch)
  {
    append(&ch, 1);
    return *this;
  }
  String &operator+=(const String &s)
  {
    append(s, s.length());
    return *this;
  }
  String &operator+=(const char *s)
  {
    append(s);
    return *this;
  }
  String &operator<<(const char *a) { return operator+=(a); }


  /// comparison operators
  bool operator==(const char *a) const { return strcmp(str(), a) == 0; }
  bool operator==(const String &s) const { return strcmp(str(), s.str()) == 0; }
  bool operator!=(const char *a) const { return strcmp(str(), a) != 0; }
  bool operator!=(const String &s) const { return strcmp(str(), s.str()) != 0; }

  KRNLIMP String &toUpper();
  KRNLIMP String &toLower();
  char *begin(int offset_plus = 0)
  {
    if (size() > 0)
      return data() + offset_plus;
    else
    {
      G_ASSERT(offset_plus == 0);
      return (char *)"";
    }
  }
  char *end(int offset_minus = 0)
  {
    if (size() > 0)
      return data() + length() - offset_minus;
    else
    {
      G_ASSERT(offset_minus == 0);
      return (char *)"";
    }
  }
  const char *begin(int offset_plus = 0) const { return const_cast<String *>(this)->begin(offset_plus); }
  const char *end(int offset_minus = 0) const { return const_cast<String *>(this)->end(offset_minus); }

  void allocBuffer(int sz)
  {
    resize(sz);
    if (sz > 0)
      front() = back() = '\0';
  }

  KRNLIMP bool prefix(const char *pref) const;
  KRNLIMP bool suffix(const char *suff) const;
  KRNLIMP String &replace(const char *old_str, const char *new_str);
  KRNLIMP String &replaceAll(const char *old_str, const char *new_str);
  KRNLIMP const char *find(const char *simple, const char *pos = NULL, bool forward = true) const;
  KRNLIMP const char *find(char ch, const char *pos = NULL, bool forward = true) const;

  static unsigned char upChar(char sym) { return sym >= 'a' && sym <= 'z' ? sym - 'a' + 'A' : sym; }
  static unsigned char loChar(char sym) { return sym >= 'A' && sym <= 'Z' ? sym - 'A' + 'a' : sym; }

  static String mk_str_cat(const char *s1, const char *s2)
  {
    String s;
    s.setStrCat(s1, s2);
    return s;
  }
  static String mk_sub_str(const char *sb, const char *se)
  {
    String s;
    s.setStr(sb, se - sb);
    return s;
  }
  static String mk_str(const char *str, int len)
  {
    String s;
    s.setStr(str, len);
    return s;
  }
};

inline bool operator==(const char *a, const String &s) { return strcmp(a, s.str()) == 0; }
inline bool operator!=(const char *a, const String &s) { return strcmp(a, s.str()) != 0; }

inline String operator+(const char *p, const String &s)
{
  if (!p || !p[0])
    return s;
  if (s.empty())
    return String(p);

  String sum;
  size_t l1 = strlen(p), l2 = s.length();

  sum.resize(l1 + l2 + 1);
  memcpy(sum.data(), p, l1);
  memcpy(sum.data() + l1, s.data(), l2);
  sum.back() = '\0';
  return sum;
}

// specialize insert_items/insert_item_at/append_items for String (optimal perf)
template <>
inline uint32_t insert_items(String &v, uint32_t at, uint32_t n, const char *p)
{
  v.insert_default(v.begin() + at, n);
  if (p)
    memcpy(v.begin() + at, p, n);
  return at;
}
template <>
inline uint32_t insert_item_at(String &v, uint32_t at, const char &item)
{
  v.Tab<char>::insert(v.begin() + at, item);
  return at;
}
template <>
inline uint32_t append_items(String &v, uint32_t n, const char *p)
{
  uint32_t at = v.Tab<char>::size();
  v.append_default(n);
  if (p)
    memcpy(v.begin() + at, p, n);
  return at;
}

#include <supp/dag_undef_COREIMP.h>

inline const char *simplify_fname(String &s)
{
  if (s.empty())
    return s.str();
  dd_simplify_fname_c(s.str());
  s.updateSz();
  return s.str();
}

#endif

/// @}

/// @}
