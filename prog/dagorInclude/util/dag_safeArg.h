//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#if _TARGET_SIMD_SSE
#ifdef __clang__
typedef float __vec4f __attribute__((__vector_size__(16), __aligned__(16)));
typedef long long __vec4i __attribute__((__vector_size__(16), __aligned__(16)));
#elif defined(_MSC_VER)
typedef union __declspec(intrin_type) __declspec(align(16)) __m128 __vec4f;
typedef union __declspec(intrin_type) __declspec(align(16)) __m128i __vec4i;
#elif defined(__GNUC__)
typedef float __vec4f __attribute__((__vector_size__(16), __may_alias__));
typedef long long __vec4i __attribute__((__vector_size__(16), __may_alias__));
#endif
#elif _TARGET_SIMD_NEON
typedef __attribute__((neon_vector_type(4))) float __vec4f;
typedef __attribute__((neon_vector_type(4))) int __vec4i;
#endif

#ifdef __cplusplus
#include <util/dag_stdint.h>

#if _TARGET_STATIC_LIB || !_TARGET_PC_WIN
#define DSA_KRNLIMP
#else
#ifdef __B_MINICORE
#define DSA_KRNLIMP __declspec(dllexport)
#else
#define DSA_KRNLIMP __declspec(dllimport)
#endif
#endif

namespace eastl
{
class allocator;
template <typename T, typename A>
class basic_string;
template <typename T>
class basic_string_view;
} // namespace eastl
class DebugPrinter
{
public:
  virtual void print(char *s, int &len) const = 0;
  virtual int getLength() const = 0;
};
template <class T>
struct DebugConverter
{
  static const T &getDebugType(const T &v) { return v; }
};
class String;
class SimpleString;
struct Color4;
struct Color3;
struct E3DCOLOR;
class Point2;
class Point3;
class Point4;
class IPoint2;
class IPoint3;
class IPoint4;
class TMatrix;
class BBox2;
class BBox3;
class IBBox2;
class IBBox3;
#ifndef __OBJC__
template <class T>
class Ptr;
template <class T>
class PatchablePtr;
#endif
#define DSA DagorSafeArg
struct DSA
{
  DSA()
  {
    varValue.i = 0;
    varType = TYPE_VOID;
  }
  void set(void)
  {
    varValue.i = 0;
    varType = TYPE_VOID;
  }
  void set(float v)
  {
    varValue.d = v;
    varType = TYPE_DOUBLE;
  }
  void set(double v)
  {
    varValue.d = v;
    varType = TYPE_DOUBLE;
  }
  void set(int v)
  {
    varValue.i = (int64_t)v;
    varType = TYPE_INT;
  }
  void set(long v)
  {
    varValue.i = (int64_t)v;
    varType = TYPE_INT;
  }
  void set(long long v)
  {
    varValue.i = (int64_t)v;
    varType = TYPE_INT;
  }
  void set(unsigned int v)
  {
    varValue.i = (uint64_t)v;
    varType = TYPE_INT;
  }
  void set(unsigned long v)
  {
    varValue.i = (uint64_t)v;
    varType = TYPE_INT;
  }
  void set(unsigned long long v)
  {
    varValue.i = (uint64_t)v;
    varType = TYPE_INT;
  }

  DSA(const void *p)
  {
    varValue.p = p;
    varType = TYPE_PTR;
  }
  void set(const void *p)
  {
    varValue.p = p;
    varType = TYPE_PTR;
  }
#ifndef __OBJC__
  template <class T>
  void set(const Ptr<T> &v)
  {
    varValue.p = v.get();
    varType = TYPE_PTR;
  }
  template <class T>
  void set(const PatchablePtr<T> &v)
  {
    varValue.p = v.get();
    varType = TYPE_PTR;
  }
#endif

  DSA(const char *s)
  {
    varValue.s = s;
    varType = TYPE_STR;
  }
  void set(const char *s)
  {
    varValue.s = s;
    varType = TYPE_STR;
  }
  DSA_KRNLIMP void set(const ::String &s);
  DSA_KRNLIMP void set(const ::SimpleString &s);
  DSA_KRNLIMP void set(const eastl::basic_string<char, eastl::allocator> &s);
  DSA_KRNLIMP void set(const eastl::basic_string_view<char> &s); // Note: string_view is assumed to be null-terminated

  DSA_KRNLIMP void set(const E3DCOLOR &c);
  void set(const Color4 &c)
  {
    varValue.c4 = &c;
    varType = TYPE_COL4;
  }
  void set(const Color3 &c)
  {
    varValue.c3 = &c;
    varType = TYPE_COL3;
  }
  void set(const Point2 &p)
  {
    varValue.p2 = &p;
    varType = TYPE_P2;
  }
  void set(const Point3 &p)
  {
    varValue.p3 = &p;
    varType = TYPE_P3;
  }
  void set(const Point4 &p)
  {
    varValue.p4 = &p;
    varType = TYPE_P4;
  }
  void set(const IPoint2 &p)
  {
    varValue.ip2 = &p;
    varType = TYPE_IP2;
  }
  void set(const IPoint3 &p)
  {
    varValue.ip3 = &p;
    varType = TYPE_IP3;
  }
  void set(const IPoint4 &p)
  {
    varValue.ip4 = &p;
    varType = TYPE_IP4;
  }
  void set(const TMatrix &tm)
  {
    varValue.tm = &tm;
    varType = TYPE_TM;
  }
  void set(const BBox2 &bb)
  {
    varValue.bb2 = &bb;
    varType = TYPE_BB2;
  }
  void set(const BBox3 &bb)
  {
    varValue.bb3 = &bb;
    varType = TYPE_BB3;
  }
  void set(const IBBox2 &bb)
  {
    varValue.ibb2 = &bb;
    varType = TYPE_IBB2;
  }
  void set(const IBBox3 &bb)
  {
    varValue.ibb3 = &bb;
    varType = TYPE_IBB3;
  }
  // It's mostly intended for dev/debug; use these with caution: pointer to SIMD vectors may become invalid for some compilers
  void set(const __vec4f &v)
  {
    varValue.p4 = (const Point4 *)&v;
    varType = TYPE_P4;
  }
  void set(const __vec4i &v)
  {
    varValue.ip4 = (const IPoint4 *)&v;
    varType = TYPE_P4;
  }

  DSA(const DebugPrinter &v)
  {
    varValue.printer = &v;
    varType = TYPE_CUSTOM;
  }

  template <typename T>
  DSA(const T &v)
  {
    set(DebugConverter<T>::getDebugType(v));
  }
  DSA_KRNLIMP static int count_len(const char *fmt, const DSA *arg, int anum);
  DSA_KRNLIMP static int print_fmt(char *buf, int len, const char *fmt, const DSA *arg, int anum);

  DSA_KRNLIMP static int mixed_print_fmt(char *buf, int len, const char *fmt, const void *valist_or_dsa, int dsa_anum);

#if _TARGET_STATIC_LIB || defined(__B_MINICORE) // to prevent mix 2 different RTL contexts
  DSA_KRNLIMP static int fprint_fmt(void *fp, const char *fmt, const DSA *arg, int anum);
  static int mixed_fprint_fmt(void *fp, const char *fmt, const void *valist_or_dsa, int dsa_num);
#endif

private:
  DSA(const DSA *); // prevent using *printf instead of *vprintf
  DSA(DSA *);       // prevent using *printf instead of *vprintf

public:
  enum Type
  {
    TYPE_VOID = 0,
    TYPE_STR,
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_COL,
    TYPE_PTR,
    TYPE_COL4,
    TYPE_COL3,
    TYPE_P2,
    TYPE_P3,
    TYPE_P4,
    TYPE_IP2,
    TYPE_IP3,
    TYPE_IP4,
    TYPE_TM,
    TYPE_BB2,
    TYPE_BB3,
    TYPE_IBB2,
    TYPE_IBB3,
    TYPE_CUSTOM,
    TYPE_PTR_END,
  };
  Type varType;
  union
  {
    int64_t i;
    double d;
    const void *p;
    const char *s;
    const Color4 *c4;
    const Color3 *c3;
    const Point2 *p2;
    const Point3 *p3;
    const Point4 *p4;
    const IPoint2 *ip2;
    const IPoint3 *ip3;
    const IPoint4 *ip4;
    const TMatrix *tm;
    const BBox2 *bb2;
    const BBox3 *bb3;
    const IBBox2 *ibb2;
    const IBBox3 *ibb3;
    const DebugPrinter *printer;
  } varValue;
};
#undef DSA
#undef DSA_KRNLIMP

#define DECLARE_DSA_OVERLOADS_FAMILY_LT(PREFIX, FUNC_CALL)                                        \
  template <typename... Args>                                                                     \
  PREFIX(DSA_OVERLOADS_PARAM_DECL const char *fmt, const Args &...args)                           \
  {                                                                                               \
    const DagorSafeArg a[sizeof...(args) ? sizeof...(args) : 1] = {args...};                      \
    FUNC_CALL(DSA_OVERLOADS_PARAM_PASS fmt, sizeof...(args) ? a : nullptr, (int)sizeof...(args)); \
  }

#define DECLARE_DSA_OVERLOADS_FAMILY(PREFIX, FUNC_DECL, FUNC_CALL)                      \
  FUNC_DECL(DSA_OVERLOADS_PARAM_DECL const char *f, const DagorSafeArg *arg, int anum); \
  DECLARE_DSA_OVERLOADS_FAMILY_LT(PREFIX, FUNC_CALL)

#endif
