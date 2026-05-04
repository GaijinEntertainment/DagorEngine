// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if USE_BCRYPT
#include <debug/dag_assert.h>

#include <windows.h>
#if _TARGET_PC_WIN
#include <winternl.h>
#endif
#include <bcrypt.h>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#if DAGOR_DBGLEVEL > 0
#define BCRYPT_DO(expr)                                                                                                              \
  do                                                                                                                                 \
  {                                                                                                                                  \
    NTSTATUS ntstat = expr;                                                                                                          \
    if (!NT_SUCCESS(ntstat) && dgs_assertion_handler)                                                                                \
      if (dgs_assertion_handler_inl(false, __FILE__, __LINE__, __FUNCTION__, #expr, "failed with %08X (see ntstatus.h for details)", \
            ntstat))                                                                                                                 \
        G_DEBUG_BREAK_FORCED;                                                                                                        \
  } while (0)
#else
#define BCRYPT_DO(expr) expr
#endif

#endif
