//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <string.h>
#include <memory/dag_mem.h>
#include <generic/dag_span.h>
#include <dag/dag_relocatable.h>

#include <supp/dag_define_KRNLIMP.h>
extern "C" KRNLIMP void dd_simplify_fname_c(char *fn);
#include <supp/dag_undef_KRNLIMP.h>

class SimpleString
{
public:
  typedef char value_type;

  // constructors and destructor
  SimpleString() : string(NULL) {}
  SimpleString(const SimpleString &s) { copyStr(s.string); }
  SimpleString(SimpleString &&s) : string(s.string) { s.string = NULL; }
  explicit SimpleString(const char *s) { copyStr(s); }
  SimpleString(const char *s, int sz) : string(NULL) { copyBuf(s, sz); }
  ~SimpleString() { freeStr(); }
  void clear()
  {
    freeStr();
    string = NULL;
  }

  // assigment operators
  SimpleString &operator=(const char *s)
  {
    if (string == s)
      return *this;
    freeStr();
    copyStr(s);
    return *this;
  }
  SimpleString &operator=(const SimpleString &s) { return operator=(s.str()); }
  SimpleString &operator=(SimpleString &&s)
  {
    swap(s);
    return *this;
  }
  void swap(SimpleString &other)
  {
    char *tmp = string;
    string = other.string;
    other.string = tmp;
  }

  void setStr(const char *s, int sz)
  {
    clear();
    copyBuf(s, sz);
  }

  // string length and emptiness test
  int length() const { return string ? (int)strlen(string) : 0; }
  bool empty() const { return !string || !string[0]; }

  // explicit cast to char* and const char*: empty string always returns ""
  char *str() { return string ? string : (char *)""; }
  const char *str() const { return string ? string : ""; }
  /// aliases for EASTL compatibility
  char *c_str() { return str(); }
  const char *c_str() const { return str(); }

  // cast operators
  operator char *() { return str(); }
  operator const char *() const { return str(); }

  // space allocation (to be used as buffer)
  void allocBuffer(int sz)
  {
    freeStr();
    if (sz > 0)
    {
      string = (char *)strmem->alloc(sz);
      string[0] = string[sz - 1] = '\0';
    }
    else
      string = NULL;
  }

  // explicitly rewrite pointer
  void setStrRaw(char *s) { string = s; }

  // comparison operators
  bool operator==(const char *s) const { return strcmp(str(), s ? s : "") == 0; }
  friend bool operator==(const char *a, const SimpleString &s) { return strcmp(a ? a : "", s.str()) == 0; }
  bool operator!=(const char *s) const { return strcmp(str(), s ? s : "") != 0; }
  friend bool operator!=(const char *a, const SimpleString &s) { return strcmp(a ? a : "", s.str()) != 0; }
  bool operator>(const char *s) const { return strcmp(str(), s ? s : "") > 0; }
  bool operator<(const char *s) const { return strcmp(str(), s ? s : "") < 0; }
  bool operator>=(const char *s) const { return strcmp(str(), s ? s : "") >= 0; }
  bool operator<=(const char *s) const { return strcmp(str(), s ? s : "") <= 0; }

  bool operator==(const SimpleString &s) const { return strcmp(str(), s.str()) == 0; }
  bool operator!=(const SimpleString &s) const { return strcmp(str(), s.str()) != 0; }
  bool operator>(const SimpleString &s) const { return strcmp(str(), s.str()) > 0; }
  bool operator<(const SimpleString &s) const { return strcmp(str(), s.str()) < 0; }
  bool operator>=(const SimpleString &s) const { return strcmp(str(), s.str()) >= 0; }
  bool operator<=(const SimpleString &s) const { return strcmp(str(), s.str()) <= 0; }

private:
  char *string;

  void copyBuf(const char *s, int sz)
  {
    if (sz <= 0 || !s || !*s)
    {
      string = NULL;
      return;
    }
    allocBuffer(sz + 1);
    memcpy(string, s, sz);
  }

  void copyStr(const char *s) { string = (!s || !s[0]) ? NULL : str_dup(s, strmem); }
  void freeStr() { (string) ? strmem->free(string) : (void)0; }
};

DAG_DECLARE_RELOCATABLE(SimpleString);

inline const char *simplify_fname(SimpleString &s)
{
  if (!s.empty())
    dd_simplify_fname_c(s.str());
  return s.str();
}

template <>
inline dag::ConstSpan<char> make_span_const(const SimpleString &v)
{
  return make_span_const(v.str(), v.length());
}
