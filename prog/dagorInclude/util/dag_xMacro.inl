//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//

/* This inline file implement XMacro technique:
 * reliable maintenance of parallel lists, of code or data,
 * whose corresponding items must appear in the same order.
 *
 * This macro-file require XMACRO_TABLE defined.
 * XMACRO_TABLE contain table data and proper list items defined in XMACRO_ARGS.
 *
 * Macros like XMACRO_DEF1, XMACRO_DEF2, ..., XMACRO_DEF8 expanded to
 * it content with argument replaced to corresponding item list index.
 *
 * XMACRO_VAL (XMACRO_ARGN_VAL) can used to modify
 * specified arguments when insert to generated code.
 * For example it can used for addition comma after each argument value.
 *
 * Sample code:
 *   #define XMACRO_TABLE \
 *     XMACRO_ARGS(VAL1, "str1", 1) \
 *     XMACRO_ARGS(VAL2, "str2", 2) \
 *     XMACRO_ARGS(VAL3, "str3") \
 *     XMACRO_ARGS(VAL4, "str4")
 *   #define XMACRO_VAL(value) value,
 *   #define XMACRO_DEF1(data) enum { data };
 *   #define XMACRO_DEF2(data) const char *names[] = { data "extra" };
 *   #define XMACRO_DEF3(data) int values[] = { data };
 *   #include <util/dag_xMacro.inl>
 *
 * Expanded to:
 *   enum { VAL1 , VAL2 , VAL3 , VAL4 , };
 *   const char *names[] = { "str1" , "str2" , "str3" , "str4" , "extra" };
 *   int values[] = { 1 , 2 , };
 *
 * Macros XMACRO_TABLE became undefined after file inlcude,
 * so for reuse source data must be declared in other macros:
 *   #define DATA ..., XMACRO_ARGS(...), XMACRO_ARGS(...), ...
 *   #define XMACRO_TABLE DATA
 *   #define XMACRO_DEF1(data) ...
 *   #include <util/dag_xMacro.inl>
 *   #define XMACRO_TABLE DATA
 *   #define XMACRO_DEF2(data) ...
 *   #include <util/dag_xMacro.inl>
 */

#ifndef XMACRO_TABLE
#error "XMACRO_TABLE should defined before include this file"
#endif

#define XMACRO_CONCATENATE(arg1, arg2)                       arg1##arg2
#define XMACRO_EXPAND(expr)                                  expr
#define XMACRO_NARG(...)                                     XMACRO_NARG_(__VA_ARGS__, XMACRO_RSEQ_N)
#define XMACRO_NARG_(...)                                    XMACRO_EXPAND(XMACRO_ARG_N(__VA_ARGS__))
#define XMACRO_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define XMACRO_RSEQ_N                                        8, 7, 6, 5, 4, 3, 2, 1, 0

#define XMACRO_ARGS_(N, ...) XMACRO_EXPAND(XMACRO_CONCATENATE(XMACRO_ARGS_, N)(__VA_ARGS__))
#define XMACRO_ARGS(...)     XMACRO_ARGS_(XMACRO_NARG(__VA_ARGS__), __VA_ARGS__)


#ifndef XMACRO_VAL
#define XMACRO_VAL(value) value
#endif

#ifndef XMACRO_VAL1
#define XMACRO_VAL1 XMACRO_VAL
#endif

#ifndef XMACRO_VAL2
#define XMACRO_VAL2 XMACRO_VAL
#endif

#ifndef XMACRO_VAL3
#define XMACRO_VAL3 XMACRO_VAL
#endif

#ifndef XMACRO_VAL4
#define XMACRO_VAL4 XMACRO_VAL
#endif

#ifndef XMACRO_VAL5
#define XMACRO_VAL5 XMACRO_VAL
#endif

#ifndef XMACRO_VAL6
#define XMACRO_VAL6 XMACRO_VAL
#endif

#ifndef XMACRO_VAL7
#define XMACRO_VAL7 XMACRO_VAL
#endif

#ifndef XMACRO_VAL8
#define XMACRO_VAL8 XMACRO_VAL
#endif


#define XMACRO_ARGS_1(arg1, ...) XMACRO_ARG1(XMACRO_VAL1(arg1))

#define XMACRO_ARGS_2(arg1, arg2, ...) \
  XMACRO_ARGS_1(arg1)                  \
  XMACRO_ARG2(XMACRO_VAL2(arg2))

#define XMACRO_ARGS_3(arg1, arg2, arg3, ...) \
  XMACRO_ARGS_2(arg1, arg2)                  \
  XMACRO_ARG3(XMACRO_VAL3(arg3))

#define XMACRO_ARGS_4(arg1, arg2, arg3, arg4, ...) \
  XMACRO_ARGS_3(arg1, arg2, arg3)                  \
  XMACRO_ARG4(XMACRO_VAL4(arg4))

#define XMACRO_ARGS_5(arg1, arg2, arg3, arg4, arg5, ...) \
  XMACRO_ARGS_4(arg1, arg2, arg3, arg4)                  \
  XMACRO_ARG5(XMACRO_VAL5(arg5))

#define XMACRO_ARGS_6(arg1, arg2, arg3, arg4, arg5, arg6, ...) \
  XMACRO_ARGS_5(arg1, arg2, arg3, arg4, arg5)                  \
  XMACRO_ARG6(XMACRO_VAL6(arg6))

#define XMACRO_ARGS_7(arg1, arg2, arg3, arg4, arg5, arg6, arg7, ...) \
  XMACRO_ARGS_6(arg1, arg2, arg3, arg4, arg5, arg6)                  \
  XMACRO_ARG7(XMACRO_VAL7(arg7))

#define XMACRO_ARGS_8(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, ...) \
  XMACRO_ARGS_7(arg1, arg2, arg3, arg4, arg5, arg6, arg7)                  \
  XMACRO_ARG8(XMACRO_VAL8(arg8))


#define XMACRO_ARG1(...)
#define XMACRO_ARG2(...)
#define XMACRO_ARG3(...)
#define XMACRO_ARG4(...)
#define XMACRO_ARG5(...)
#define XMACRO_ARG6(...)
#define XMACRO_ARG7(...)
#define XMACRO_ARG8(...)


#ifdef XMACRO_DEF1
#undef XMACRO_ARG1
#define XMACRO_ARG1(...) __VA_ARGS__
XMACRO_DEF1(XMACRO_TABLE)
#undef XMACRO_ARG1
#define XMACRO_ARG1(...)
#endif

#ifdef XMACRO_DEF2
#undef XMACRO_ARG2
#define XMACRO_ARG2(...) __VA_ARGS__
XMACRO_DEF2(XMACRO_TABLE)
#undef XMACRO_ARG2
#define XMACRO_ARG2(...)
#endif

#ifdef XMACRO_DEF3
#undef XMACRO_ARG3
#define XMACRO_ARG3(...) __VA_ARGS__
XMACRO_DEF3(XMACRO_TABLE)
#undef XMACRO_ARG3
#define XMACRO_ARG3(...)
#endif

#ifdef XMACRO_DEF4
#undef XMACRO_ARG4
#define XMACRO_ARG4(...) __VA_ARGS__
XMACRO_DEF4(XMACRO_TABLE)
#undef XMACRO_ARG4
#define XMACRO_ARG4(...)
#endif

#ifdef XMACRO_DEF5
#undef XMACRO_ARG5
#define XMACRO_ARG5(...) __VA_ARGS__
XMACRO_DEF5(XMACRO_TABLE)
#undef XMACRO_ARG5
#define XMACRO_ARG5(...)
#endif

#ifdef XMACRO_DEF6
#undef XMACRO_ARG6
#define XMACRO_ARG6(...) __VA_ARGS__
XMACRO_DEF6(XMACRO_TABLE)
#undef XMACRO_ARG6
#define XMACRO_ARG6(...)
#endif

#ifdef XMACRO_DEF7
#undef XMACRO_ARG7
#define XMACRO_ARG7(...) __VA_ARGS__
XMACRO_DEF7(XMACRO_TABLE)
#undef XMACRO_ARG7
#define XMACRO_ARG7(...)
#endif

#ifdef XMACRO_DEF8
#undef XMACRO_ARG8
#define XMACRO_ARG8(...) __VA_ARGS__
XMACRO_DEF8(XMACRO_TABLE)
#undef XMACRO_ARG8
#define XMACRO_ARG8(...)
#endif


#undef XMACRO_ARG1
#undef XMACRO_ARG2
#undef XMACRO_ARG3
#undef XMACRO_ARG4
#undef XMACRO_ARG5
#undef XMACRO_ARG6
#undef XMACRO_ARG7
#undef XMACRO_ARG8

#undef XMACRO_ARGS_1
#undef XMACRO_ARGS_2
#undef XMACRO_ARGS_3
#undef XMACRO_ARGS_4
#undef XMACRO_ARGS_6
#undef XMACRO_ARGS_6
#undef XMACRO_ARGS_7
#undef XMACRO_ARGS_8
#undef XMACRO_ARGS
#undef XMACRO_ARGS_
#undef XMACRO_NARG
#undef XMACRO_NARG_
#undef XMACRO_ARG_N
#undef XMACRO_RSEQ_N
#undef XMACRO_CONCATENATE
#undef XMACRO_EXPAND

#ifndef XMACRO_TABLE_SAVE
#undef XMACRO_TABLE
#endif

#ifndef XMACRO_DEF_SAVE
#undef XMACRO_DEF1
#undef XMACRO_DEF2
#undef XMACRO_DEF3
#undef XMACRO_DEF4
#undef XMACRO_DEF5
#undef XMACRO_DEF6
#undef XMACRO_DEF7
#undef XMACRO_DEF8
#endif

#ifndef XMACRO_VAL_SAVE
#undef XMACRO_VAL
#undef XMACRO_VAL1
#undef XMACRO_VAL2
#undef XMACRO_VAL3
#undef XMACRO_VAL4
#undef XMACRO_VAL5
#undef XMACRO_VAL6
#undef XMACRO_VAL7
#undef XMACRO_VAL8
#endif
