//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>
#include <util/dag_stdint.h>
#include <windows.h>
#include <dbghelp.h>

namespace DagorHwException
{
struct MemoryRange
{
  uintptr_t from = 0;
  uintptr_t to = 0;
  bool contain(uint64_t addr) const { return addr >= from && addr < to; }
};

struct MinidumpThreadData
{
  uintptr_t threadId = 0;
  bool needScanMem = false;
  bool needDumpRet = false;
  uint32_t scannedRegs = 0;
#if _TARGET_64BIT
  static const uint32_t regsCount = 16;
#else
  static const uint32_t regsCount = 7;
#endif
  uintptr_t regs[16]{};
  uintptr_t stackPtr = 0;
  MemoryRange stack;

  void setRegisters(const CONTEXT &ctx)
  {
#if _TARGET_64BIT && _M_ARM64
    stackPtr = ctx.Pc;
    const int64_t *regsSrc = (int64_t *)&ctx.X0;
#elif _TARGET_64BIT
    stackPtr = ctx.Rsp;
    const int64_t *regsSrc = (int64_t *)&ctx.Rax;
#else
    stackPtr = ctx.Esp;
    const int32_t *regsSrc = (int32_t *)&ctx.Edi;
#endif
    for (uint32_t i = 0; i < regsCount; i++)
      regs[i] = regsSrc[i];
  }
};

struct MinidumpExceptionData
{
  MinidumpExceptionData(EXCEPTION_POINTERS *_eptr, uintptr_t exc_thread_id);

  EXCEPTION_POINTERS *eptr = nullptr;
  MinidumpThreadData excThread;
  MinidumpThreadData mainThread;
  MemoryRange image; // main module containing this file
  MemoryRange code;

  MemoryRange ntdll;
  MemoryRange user32;
  MemoryRange kernel32;
  MemoryRange kernelBase;
  MemoryRange videoDriver;

  uint32_t memAddQueueSize = 0;
  MemoryRange memAddQueue[16];
  uint32_t memRemoveQueueSize = 0;
  MemoryRange memRemoveQueue[128];
  bool memCallbackCalled = false;
};

KRNLIMP BOOL CALLBACK minidump_callback(PVOID callback_param, const PMINIDUMP_CALLBACK_INPUT callback_input,
  PMINIDUMP_CALLBACK_OUTPUT callback_output);
KRNLIMP int CALLBACK write_minidump_fallback(HANDLE hProcess, DWORD dwProcessId, HANDLE hDumpFile, EXCEPTION_POINTERS *eptr,
  MINIDUMP_USER_STREAM_INFORMATION *user_streams);
} // namespace DagorHwException

#include <supp/dag_undef_KRNLIMP.h>
