// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_assert.h>

#define D3D_CONTRACT_FAILED_TEXT     "Incorrect D3D API usage"
#define D3D_CONTRACT_FAILED_FMT(fmt) D3D_CONTRACT_FAILED_TEXT "\n" fmt

#define D3D_CONTRACT_ASSERT_EX(expression, expr_str) G_ASSERTF_EX(expression, expr_str, D3D_CONTRACT_FAILED_TEXT)
#define D3D_CONTRACT_ASSERTF_EX(expression, expr_str, fmt, ...) \
  G_ASSERTF_EX(expression, expr_str, D3D_CONTRACT_FAILED_FMT(fmt), ##__VA_ARGS__)
#define D3D_CONTRACT_ASSERT_FAIL(fmt, ...)              G_ASSERT_FAIL(D3D_CONTRACT_FAILED_FMT(fmt), ##__VA_ARGS__)
#define D3D_CONTRACT_ASSERTF_ONCE(expression, fmt, ...) G_ASSERTF_ONCE(expression, D3D_CONTRACT_FAILED_FMT(fmt), ##__VA_ARGS__)
#define D3D_CONTRACT_ASSERT(expression)                 D3D_CONTRACT_ASSERT_EX(expression, #expression)
#define D3D_CONTRACT_ASSERTF(expression, fmt, ...)      D3D_CONTRACT_ASSERTF_EX(expression, #expression, fmt, ##__VA_ARGS__)

#define D3D_CONTRACT_VERIFY(expression)            G_VERIFYF(expression, D3D_CONTRACT_FAILED_TEXT)
#define D3D_CONTRACT_VERIFYF(expression, fmt, ...) G_VERIFYF(expression, D3D_CONTRACT_FAILED_FMT(fmt), ##__VA_ARGS__)

#define D3D_CONTRACT_ASSERT_AND_DO(expr, cmd)            G_ASSERTF_AND_DO(expr, cmd, D3D_CONTRACT_FAILED_TEXT)
#define D3D_CONTRACT_ASSERTF_AND_DO(expr, cmd, fmt, ...) G_ASSERTF_AND_DO(expr, cmd, D3D_CONTRACT_FAILED_FMT(fmt), ##__VA_ARGS__)

#define D3D_CONTRACT_ASSERT_RETURN(expr, returnValue) D3D_CONTRACT_ASSERT_AND_DO(expr, return returnValue)
#define D3D_CONTRACT_ASSERT_BREAK(expr)               G_ASSERTF_AND_DO_UNHYGIENIC(expr, break, D3D_CONTRACT_FAILED_TEXT)
#define D3D_CONTRACT_ASSERT_CONTINUE(expr)            G_ASSERTF_AND_DO_UNHYGIENIC(expr, continue, D3D_CONTRACT_FAILED_TEXT)

#define D3D_CONTRACT_ASSERTF_RETURN(expr, returnValue, fmt, ...) \
  D3D_CONTRACT_ASSERTF_AND_DO(expr, return returnValue, fmt, ##__VA_ARGS__)
#define D3D_CONTRACT_ASSERTF_BREAK(expr, fmt, ...) \
  G_ASSERTF_AND_DO_UNHYGIENIC(expr, break, D3D_CONTRACT_FAILED_FMT(fmt), ##__VA_ARGS__)
#define D3D_CONTRACT_ASSERTF_CONTINUE(expr, fmt, ...) \
  G_ASSERTF_AND_DO_UNHYGIENIC(expr, continue, D3D_CONTRACT_FAILED_FMT(fmt), ##__VA_ARGS__)

#define D3D_CONTRACT_ASSERT_FAIL_RETURN(returnValue, fmt, ...)  \
  {                                                             \
    G_ASSERT_FAIL(D3D_CONTRACT_FAILED_FMT(fmt), ##__VA_ARGS__); \
    return returnValue;                                         \
  }
