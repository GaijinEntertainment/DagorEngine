/*
Copyright (c) 2017, Insomniac Games
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "Precompiled.h"

#include "CacheSim.h"
#include "CacheSimInternals.h"
#include "CacheSimData.h"
#include "GenericHashTable.h"
#include "Md5.h"

#include <EASTL/algorithm.h>
#include <intrin.h>
#include <immintrin.h>
#include <stdarg.h>
#include <time.h>
#include <psapi.h>

#include "CacheSimCommon.inl"
/// By default we stomp ntdll!RtlpCallVectoredHandlers with a jump to our handler.
///
/// This is dirty, so at your option you can also attempt to use a regular vectored exception handler.
/// The problem with this is that there is a SRW lock internally in ntdll that protects the VEH list. 
/// When taking an exception for *every instruction* on *every thread* that lock is extremely contended.
/// This results in deadlocks on certain combinations of sleeps and syscalls, and only on some versions
/// of Windows. We haven't exactly figured that part out, so we developed this ugliness instead.
///
/// With USE_VEH_TRAMPOLINE enabled, the vectored exception handling code simply calls our routine directly
/// and doesn't bother with any locking or list walking. This can break things in theory, but seems OK
/// for us in practice because nothing else uses vectored exception handling. But YMMV.

#define USE_VEH_TRAMPOLINE 1

namespace CacheSim
{
    enum
    {
        kExceptionCodeFindHandlers = 0x10001,
        kExceptionCodeFindHandlersContext = 0x10002
    };

#if USE_VEH_TRAMPOLINE
    static uint8_t veh_stash[12];
#else
    static PVOID g_Handler = 0;
#endif
    static uintptr_t g_RaiseExceptionAddress;

}

static uintptr_t AdjustFsSegment(uintptr_t address)
{
  // FS is not used in Windows user space.
  return 0;
}

static uintptr_t AdjustGsSegment(uintptr_t address)
{
  return __readgsqword(FIELD_OFFSET(NT_TIB, Self)) + address;
}

static void empty_func()
{
}

static int Backtrace2(uintptr_t callstack[], const PCONTEXT ctx)
{
  int i = 0;
  ULONGLONG image_base;
  UNWIND_HISTORY_TABLE target_gp;

  memset(&target_gp, 0, sizeof target_gp);

  CONTEXT ctx_copy;
  ctx_copy = *ctx; // Overkill

  while (i < CacheSim::kMaxCalls)
  {
    callstack[i++] = ctx_copy.Rip;

    PRUNTIME_FUNCTION pfunc = RtlLookupFunctionEntry(ctx_copy.Rip, &image_base, &target_gp);

    if (NULL == pfunc)
    {
      // This is a leaf function.
      ctx_copy.Rip = *(uintptr_t*)ctx_copy.Rsp;
      ctx_copy.Rsp += 8;
    }
    else
    {
      PVOID handler_data;
      DWORD64 establisher_frame;

      RtlVirtualUnwind(0, image_base, ctx_copy.Rip, pfunc, &ctx_copy, &handler_data, &establisher_frame, nullptr);
    }

    if (!ctx_copy.Rip)
      break;
  }

  return i;
}

#if !USE_VEH_TRAMPOLINE
static LONG WINAPI StepFilter(_In_ struct _EXCEPTION_POINTERS* ExcInfo)
#else
static LONG WINAPI StepFilter(PEXCEPTION_RECORD ExceptionRecord, PCONTEXT ContextRecord)
#endif
{
#if USE_VEH_TRAMPOLINE
  _EXCEPTION_POINTERS ExcInfoStorage = { ExceptionRecord, ContextRecord };
  _EXCEPTION_POINTERS* ExcInfo = &ExcInfoStorage;
#endif
  using namespace CacheSim;
  if (STATUS_SINGLE_STEP == ExcInfo->ExceptionRecord->ExceptionCode)
  {

    // Make sure the thread state is up to date.
    LONG curr_gen = g_Generation;
    ud_t* ud = &s_ThreadState.m_Disassembler;

    if (s_ThreadState.m_Generation != curr_gen)
    {
      ud_init(ud);
      ud_set_mode(ud, 64);

      s_ThreadState.m_LogicalCoreIndex = FindLogicalCoreIndex(::GetCurrentThreadId());

      s_ThreadState.m_Generation = curr_gen;
      InvalidateStack();
    }


    const int core_index = s_ThreadState.m_LogicalCoreIndex;

    // Only trace threads we've mapped to cores. Ignore all others.
    if (g_TraceEnabled && core_index >= 0)
    {
      uintptr_t rip = ExcInfo->ContextRecord->Rip;

      if (rip == g_RaiseExceptionAddress)
      {
        // Patch any attempts to raise an exception so we don't crash.
        // This typically comes up trying to call OutputDebugString, which will raise an exception internally.
        rip = ExcInfo->ContextRecord->Rip = (uintptr_t) &empty_func;
        ExcInfo->ContextRecord->EFlags |= 0x100;
        return EXCEPTION_CONTINUE_EXECUTION;
      }

      if (~0u == s_ThreadState.m_StackIndex)
      {
        // Recompute call stack
        uintptr_t callstack[kMaxCalls];
        static_assert(sizeof(PVOID) == sizeof(uintptr_t), "64-bit required");
        int frame_count = Backtrace2(callstack, ExcInfo->ContextRecord);
        if (0 == frame_count || kMaxCalls == frame_count)
          DebugBreak();

        AutoSpinLock lock;

        // Take off one more frame as we're splitting in two parts, stack and current_rip
        s_ThreadState.m_StackIndex = InsertStack(callstack + 1, frame_count - 1);
      }

      ud_set_input_buffer(ud, (const uint8_t*) rip, 16);
      ud_set_pc(ud, rip);
      int ilen = ud_disassemble(ud);
      GenerateMemoryAccesses(core_index, ud, rip, ilen, ExcInfo->ContextRecord);

      // Keep trapping.
      ExcInfo->ContextRecord->EFlags |= 0x100;
    }

    return EXCEPTION_CONTINUE_EXECUTION;
  }

  return EXCEPTION_CONTINUE_SEARCH;
}

void CacheSim::Init()
{
  using namespace CacheSim;
  // Note that this heap *has* to be non-serialized, because we can't have it trying to take any locks.
  // Doing so will deadlock the recording. So we rely on spin locks and a non-serialized heap instead.
  g_Stats.Init();
  g_Stacks.Init();
  memset(&g_StackData, 0, sizeof g_StackData);

  HMODULE h = LoadLibraryA("kernelbase.dll");
  g_RaiseExceptionAddress = (uintptr_t) GetProcAddress(h, "RaiseException");
}

#if USE_VEH_TRAMPOLINE
static LONG WINAPI FindCallHandlers(struct _EXCEPTION_POINTERS* ExcInfo)
{
  using namespace CacheSim;
  if (ExcInfo->ExceptionRecord->ExceptionCode == kExceptionCodeFindHandlers)
  {
    // Raise another exception to get a context record that has RtlpCallVectoredHandlers in the stack:
    RaiseException(kExceptionCodeFindHandlersContext, 0, 1, ExcInfo->ExceptionRecord->ExceptionInformation);
    return EXCEPTION_CONTINUE_EXECUTION;
  }
  else if (ExcInfo->ExceptionRecord->ExceptionCode == kExceptionCodeFindHandlersContext)
  {
    // Grab a backtrace from the exception we raised above:
    uintptr_t callstack[kMaxCalls];
    int frame_count = Backtrace2(callstack, ExcInfo->ContextRecord);
    if (0 == frame_count || kMaxCalls == frame_count)
      DebugBreak();

    // The third Rip in that backtrace should be after a call to RtlpCallVectoredHandlers,
    // so resolve the Rip-relative offset in that call instruction:
    uint8_t* call_rip = (uint8_t*) callstack[3];
    int32_t ofs;
    memcpy(&ofs, call_rip - 4, 4);

    // Store the result to the argument we got in ExceptionInformation:
    uintptr_t* pResult = (uintptr_t*) ExcInfo->ExceptionRecord->ExceptionInformation[0];
    *pResult = (uintptr_t)(call_rip + ofs);
    return EXCEPTION_CONTINUE_EXECUTION;
  }
  
  return EXCEPTION_CONTINUE_SEARCH;
}
#endif

/*!
\remarks
This function contains a table describing the offset inside the ntdll module where we can find the start of the symbol
RtlpCallVectoredHandlers. This symbol is not exported, so it's not possible to get its location
with a call to GetProcAddress(). If you have a version of NTDLL not in this list and need to
use CacheSim, update the table in this function per the comment below.
*/
bool CacheSim::StartCapture()
{
  using namespace CacheSim;
  if (g_TraceEnabled)
  {
    return false;
  }

  if (IsDebuggerPresent())
  {
    OutputDebugStringA("CacheSimStartCapture: Refusing to trace when the debugger is attached.\n");
    DebugBreak();
    return false;
  }

  // Reset.
  g_Cache.Init();

  HANDLE thread_handles[ARRAY_SIZE(s_CoreMappings)];
  int thread_count = 0;

  DWORD my_thread_id = GetCurrentThreadId();
  for (int i = 0, count = s_CoreMappingCount; i < count; ++i)
  {
    const auto& mapping = s_CoreMappings[i];
    if (mapping.m_ThreadId == my_thread_id)
      continue;

    if (HANDLE h = OpenThread(THREAD_ALL_ACCESS, FALSE, (DWORD)mapping.m_ThreadId))
    {
      thread_handles[thread_count++] = h;
    }
  }

  // Suspend all threads that aren't this thread.
  for (int i = 0; i < thread_count; ++i)
  {
    DWORD suspend_count = SuspendThread(thread_handles[i]);
    if (int(suspend_count) < 0)
      DebugBreak(); // Failed to suspend thread
  }

  // Make reasonably sure they've all stopped.
  Sleep(1000);

  InterlockedIncrement((LONG*)&g_Generation);

  g_TraceEnabled = 1;

#if !USE_VEH_TRAMPOLINE
  // Install exception filter to do the tracing.
  if (!g_Handler)
  {
    g_Handler = AddVectoredExceptionHandler(1, StepFilter);
    if (!g_Handler)
    {
      DebugBreak(); // Failed to install vectored exception handler
    }
  }
#else
  {
    static bool trampoline_installed = false;

    if (!trampoline_installed)
    {
      PVOID find_handler = AddVectoredExceptionHandler(1, FindCallHandlers);
      if (!find_handler)
      {
        DebugBreak();
      }
      uintptr_t callveh_ptr = 0;
      uintptr_t *exception_args = &callveh_ptr;
      RaiseException(kExceptionCodeFindHandlers, 0, 1, (ULONG_PTR*)&exception_args);
      RemoveVectoredExceptionHandler(find_handler);
      if (callveh_ptr == 0)
      {
        DebugBreak();
      }

      uint8_t* addr = (uint8_t*)callveh_ptr;
      DWORD old_prot;
      if (!VirtualProtect((uint8_t*)(uintptr_t(addr) & ~4095ull), 8192, PAGE_EXECUTE_READWRITE, &old_prot))
      {
        DebugBreak();
      }

      uintptr_t target = (uintptr_t)&StepFilter;
      const uint8_t a0 = uint8_t(target >> 56);
      const uint8_t a1 = uint8_t(target >> 48);
      const uint8_t a2 = uint8_t(target >> 40);
      const uint8_t a3 = uint8_t(target >> 32);
      const uint8_t a4 = uint8_t(target >> 24);
      const uint8_t a5 = uint8_t(target >> 16);
      const uint8_t a6 = uint8_t(target >> 8);
      const uint8_t a7 = uint8_t(target >> 0);

      uint8_t replacement[] =
      {
        0x48, 0xb8, a7, a6, a5, a4, a3, a2, a1, a0,   // mov rax, addr (64-bit abs)
        0xff, 0xe0                                    // jmp rax
      };

      static_assert(sizeof replacement == sizeof veh_stash, "This needs to match in size");

      memcpy(veh_stash, addr, sizeof veh_stash);
      memcpy(addr, replacement, sizeof replacement);

      FlushInstructionCache(GetCurrentProcess(), addr, sizeof replacement);

      if (!VirtualProtect((uint8_t*)(uintptr_t(addr) & ~4095ull), 8192, old_prot, &old_prot))
      {
        DebugBreak();
      }

      trampoline_installed = true;
    }
  }

#endif

  for (int i = 0; i < thread_count; ++i)
  {
    CONTEXT ctx;
    ZeroMemory(&ctx, sizeof ctx);
    ctx.ContextFlags = CONTEXT_CONTROL;

    // Enable trap flag for thread.
    if (GetThreadContext(thread_handles[i], &ctx))
    {
      ctx.EFlags |= 0x100;
      SetThreadContext(thread_handles[i], &ctx);
    }
  }

  // Resume all other threads.
  for (int i = 0; i < thread_count; ++i)
  {
    ResumeThread(thread_handles[i]);
  }

  for (int i = 0; i < thread_count; ++i)
  {
    CloseHandle(thread_handles[i]);
  }

  // Finally enable trap flag for calling thread.
  __writeeflags(__readeflags() | 0x100);

  return true;
}

void DisableTrapFlag()
{
  __writeeflags(__readeflags() & ~0x100);

  // Give the thread a few instructions to run so we're definitely not tracing.
  //Thread::NanoPause();
  Sleep(0);
  Sleep(0);
  Sleep(0);
}

void GetFilenameForSave(char* filename, size_t bufferSize)
{
  char executable_filename[512];
  const char* executable_name = "unknown";
  if (0 != GetModuleFileNameA(NULL, executable_filename, ARRAY_SIZE(executable_filename)))
  {
    if (char* p = strrchr(executable_filename, '\\'))
    {
      ++p;
      if (char* dot = strchr(p, '.'))
      {
        executable_name = p;
        *dot = '\0';
      }
    }
  }

  _snprintf_s(filename, bufferSize, bufferSize, "%s_%u.csim", executable_name, (uint32_t)time(nullptr));
}

void GetModuleList(ModuleList* moduleList)
{
  HMODULE modules[1024];
  DWORD bytes_needed = 0;
  if (EnumProcessModules(GetCurrentProcess(), modules, sizeof modules, &bytes_needed))
  {
     size_t bytes = eastl::min(sizeof modules, size_t(bytes_needed));
     size_t nmods = bytes / sizeof modules[0];

    for (size_t i = 0; i < nmods; ++i)
    {
      HMODULE mod = modules[i];
      MODULEINFO modinfo;
      if (GetModuleInformation(GetCurrentProcess(), mod, &modinfo, sizeof modinfo))
      {
        if (DWORD namelen = GetModuleFileNameExA(GetCurrentProcess(), mod, moduleList->m_Infos[i].m_Filename, sizeof moduleList->m_Infos[i].m_Filename))
        {
          moduleList->m_Infos[i].m_StartAddrInMemory = static_cast<void*>(mod);
          moduleList->m_Infos[i].m_SegmentOffset = 0; // Only used on linux
          moduleList->m_Infos[i].m_Length = modinfo.SizeOfImage;
          moduleList->m_Count++;
        }
      }
    }
  }

}

void CacheSim::RemoveHandler()
{
#if !USE_VEH_TRAMPOLINE
  if (CacheSim::g_Handler)
  {
    RemoveVectoredExceptionHandler(CacheSim::g_Handler);
    CacheSim::g_Handler = nullptr;
  }
#endif
}

uint64_t CacheSim::GetCurrentThreadId()
{
  return ::GetCurrentThreadId();
}