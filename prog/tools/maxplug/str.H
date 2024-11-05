// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/// @addtogroup containers
/// @{

/// @addtogroup strings
/// @{

/** @file
  text #String object, derived from #Tab template
  @sa Tab DynTab
*/

#include <stdarg.h>
#include <string.h>
#include <max.h>
#include "debug.h"
// #include "define_COREIMPORT.h"

#ifdef __cplusplus

/**
  dynamic text string object, derived from #Tab template;
  it has useful printf() methods, including constructor,
  these methods take @b maxsz argument that is not used now,
  set it to "expected" string size
*/
class String : public Tab<char>
{
public:
  String() {}
  // String(int maxsz,char *s,va_list);
  /// printf() constructor
  String(int maxsz, char *s, ...)
  {
    if (!Resize(1))
      return;
    operator[](0) = 0;
    /*ptr[0]=0;
    va_list ap;
    va_start(ap,s);
    vprintf(maxsz,s,ap);
    va_end(ap);*/
  }
  String(const char *s)
  {
    if (s)
    {
      int l = (int)strlen(s);
      if (Resize(l + 1))
      {
        char *str = (char *)malloc(strlen(s) + 1);
        strcpy(str, s);
        Append(l + 1, str);
      }
    }
  }

  String(const char *first, const char *after_last)
  {
    char *str = (char *)malloc(strlen(first) + 1);
    strcpy(str, first);
    Append(after_last - first, str);
  }

  String &operator=(const char *s)
  {
    if (s)
    {
      int l = (int)strlen(s);
      if (Resize(l + 1))
      {
        char *str = (char *)malloc(strlen(s) + 1);
        strcpy(str, s);
        Append(l + 1, str);
      }
      //{memcpy(this[0],s,l+1);}
    }
    else if (Resize(1))
      operator[](0) = 0;
    return *this;
  }
  String &operator=(const String &s)
  {
    operator=((const char *)s);
    return *this;
  }
  /// text string length
  int length() const { return Count() - 1; }
  /// in case of empty empty/NULL string returns ""
  operator const char *() const
  {
    if (!Count())
      return "";
    return &operator[](0);
  }
  operator char *()
  {
    if (!Count())
      return (char *)"";
    return &operator[](0);
  }

  /// insert text - returns position or -1 on error
  int insert(int at, const char *p, int n);
  /// insert text - returns position or -1 on error
  int insert(int at, const char *p);
  /// append text - returns position or -1 on error
  int append(const char *p, int n);
  /// append text - returns position or -1 on error
  int append(const char *p);

  /// converts \ to / in whole string
  void cvtSlashes();
  /// converts / to \ in whole string
  void cvtSlashesRev();

  int insert(int at, int n, char *p = NULL, int step = 8) { return Tab<char>::Insert(at, n, p); }
  int append(int n, char *p = NULL) { return Tab<char>::Append(n, p); }

  int vprintf(int maxsz, char *s, va_list);         ///< replaces string
  int printf(int maxsz, char *s, ...);              ///< replaces string
  int vprintf(int at, int maxsz, char *s, va_list); ///< inserts text
  int printf(int at, int maxsz, char *s, ...);      ///< inserts text
  int vaprintf(int maxsz, char *s, va_list);        ///< appends text
  int aprintf(int maxsz, char *s, ...);             ///< appends text
  String operator+(const String &s) const;
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
  String &operator<<(const char *a)
  {
    *this += a;
    return *this;
  } ///< appends text
  bool operator==(const char *a) const { return strcmp((const char *)*this, a) == 0; }
  bool operator!=(const char *a) const { return strcmp((const char *)*this, a) != 0; }
  bool operator==(const String &s) const { return strcmp((const char *)*this, (const char *)s) == 0; }
  bool operator!=(const String &s) const { return strcmp((const char *)*this, (const char *)s) != 0; }

  String &toUpper();
  String &toLower();
  const char *begin() const { return operator const char *(); }
  const char *end() const { return begin() + length(); } // return pointer to '\0'
  char *begin() { return operator char *(); }
  char *end() { return begin() + length(); } // return pointer to '\0'
  bool prefix(const char *pref) const;
  bool suffix(const char *suff) const;
  String &replace(const String &old_str, const String &new_str);
  const char *find(const char *simple, const char *pos = NULL, bool forward = true) const;
  const char *find(char ch, const char *pos = NULL, bool forward = true) const;
  static unsigned char upChar(char sym) { return (sym >= 'a' && sym <= 'z' ? sym - 'a' + 'A' : sym); }
  static unsigned char loChar(char sym) { return (sym >= 'A' && sym <= 'Z' ? sym - 'A' + 'a' : sym); }

  String &operator+=(char ch)
  {
    append(1, &ch);
    return *this;
  }
};

inline bool operator==(const char *a, const String &s) { return strcmp(a, s) == 0; }
inline bool operator!=(const char *a, const String &s) { return strcmp(a, s) != 0; }

inline String operator+(char *p, String s) { return String(p) += s; }

#endif

// #include "undef_COREIMPORT.h"

/// @}

/// @}
