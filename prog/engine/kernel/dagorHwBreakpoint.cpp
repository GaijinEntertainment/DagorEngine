// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_hwBreakpoint.h>

#include <debug/dag_log.h>
#include <osApiWrappers/dag_rwLock.h>
#include <osApiWrappers/dag_rwSpinLock.h>
#include <util/dag_finally.h>

#include <atomic>
#include <util/dag_bitwise_cast.h>

#include <MinHook.h>
#include <Windows.h>
#include <TlHelp32.h>


namespace dag::hwbrk
{
struct Dr6
{
  /*0*/ uint64_t dr0_bp_state : 1;
  /*1*/ uint64_t dr1_bp_state : 1;
  /*2*/ uint64_t dr2_bp_state : 1;
  /*3*/ uint64_t dr3_bp_state : 1;

  /*4 - 12*/ uint64_t reserved1 : 8;

  /*13*/ uint64_t BD : 1;

  /*14*/ uint64_t BS : 1;

  /*15*/ uint64_t BT : 1;

  /*16*/ uint64_t RTM : 1;
};

struct Dr7
{
  /*0*/ uint64_t local_dr0_bp : 1;
  /*1*/ uint64_t global_dr0_bp : 1;

  /*2*/ uint64_t local_dr1_bp : 1;
  /*3*/ uint64_t global_dr1_bp : 1;

  /*4*/ uint64_t local_dr2_bp : 1;
  /*5*/ uint64_t global_dr2_bp : 1;

  /*6*/ uint64_t local_dr3_bp : 1;
  /*7*/ uint64_t global_dr3_bp : 1;

  /*8*/ uint64_t local_exact_bp : 1;
  /*9*/ uint64_t global_exact_bp : 1;

  /*10*/ uint64_t unk1 : 1;

  /*11*/ uint64_t restricted_transactional_memory : 1;

  /*12*/ uint64_t unk2 : 1;

  /*13*/ uint64_t general_detect_enable : 1;

  /*14 & 15*/ uint64_t unk3 : 2;

  /*16 & 17*/ uint64_t dr0_permissions : 2;
  /*18 & 19*/ uint64_t dr0_length : 2;

  /*20 & 21*/ uint64_t dr1_permissions : 2;
  /*22 & 23*/ uint64_t dr1_length : 2;

  /*24 & 25*/ uint64_t dr2_permissions : 2;
  /*26 & 27*/ uint64_t dr2_length : 2;

  /*28 & 29*/ uint64_t dr3_permissions : 2;
  /*30 & 31*/ uint64_t dr3_length : 2;
};

struct DebugRegisters
{
  size_t dr0;
  size_t dr1;
  size_t dr2;
  size_t dr3;
  Dr7 dr7;
};

using PFN_RtlUserThreadStart = VOID(WINAPI *)(PVOID, PVOID);

static std::atomic<uint8_t> used_slots = 0;
static DebugRegisters debug_registers{};
static SpinLockReadWriteLock register_mutex{};
static bool mh_initialized = false;
static PFN_RtlUserThreadStart real_rtl_user_thread_start = nullptr;


static void update_context(CONTEXT &ctx)
{
  ctx.Dr0 = debug_registers.dr0;
  ctx.Dr1 = debug_registers.dr1;
  ctx.Dr2 = debug_registers.dr2;
  ctx.Dr3 = debug_registers.dr3;
  ctx.Dr7 = dag::bit_cast<DWORD64>(debug_registers.dr7);
}

static void update_thread(HANDLE thread)
{
  CONTEXT ctx;
  ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
  if (::GetThreadContext(thread, &ctx) == FALSE)
  {
    return;
  }

  update_context(ctx);

  if (::SetThreadContext(thread, &ctx) == FALSE)
  {
    //
  }
}

static void update_threads()
{
  const auto currentProcessId = ::GetCurrentProcessId();
  const auto currentThreadId = ::GetCurrentThreadId();
  const auto threadSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (threadSnap == INVALID_HANDLE_VALUE)
    return;

  FINALLY([&] { ::CloseHandle(threadSnap); });

  THREADENTRY32 te32;
  te32.dwSize = sizeof(THREADENTRY32);
  if (!::Thread32First(threadSnap, &te32))
    return;

  do
  {
    if (te32.th32OwnerProcessID != currentProcessId)
      continue;

    if (te32.th32ThreadID == currentThreadId)
      continue;

    const auto currentThread = ::OpenThread(THREAD_ALL_ACCESS, false, te32.th32ThreadID);

    if (currentThread == NULL)
      continue;

    update_thread(currentThread);
    ::CloseHandle(currentThread);
  } while (::Thread32Next(threadSnap, &te32));

  update_thread(::GetCurrentThread());
}

static void WINAPI hooked_rtl_user_thread_start(PVOID start, PVOID param)
{
  if (used_slots.load() != 0) [[unlikely]]
  {
    ScopedLockReadTemplate debugRegisterLock{register_mutex};
    update_thread(::GetCurrentThread());
  }

  real_rtl_user_thread_start(start, param);
}

void set(const void *address, Size size, Slot slot, When when)
{
  {
    ScopedLockWriteTemplate debugRegisterLock{register_mutex};

    if (!mh_initialized)
    {
      if (auto result = MH_Initialize(); result == MH_OK || result == MH_ERROR_ALREADY_INITIALIZED)
      {
        PFN_RtlUserThreadStart pRtlUserThreadStart = nullptr;
        result = MH_CreateHookApiEx(L"ntdll", "RtlUserThreadStart", hooked_rtl_user_thread_start,
          reinterpret_cast<void **>(&real_rtl_user_thread_start), reinterpret_cast<void **>(&pRtlUserThreadStart));
        G_ASSERT(result == MH_OK);
        MH_EnableHook(pRtlUserThreadStart);
      }
      else
      {
        G_ASSERT_RETURN(false, );
      }

      mh_initialized = true;
    }

    switch (slot)
    {
      case Slot::_0:
      {
        debug_registers.dr0 = dag::bit_cast<size_t>(address);
        debug_registers.dr7.dr0_permissions = eastl::to_underlying(when);
        debug_registers.dr7.dr0_length = eastl::to_underlying(size);
        debug_registers.dr7.local_dr0_bp = 1;
      }
      break;
      case Slot::_1:
      {
        debug_registers.dr1 = dag::bit_cast<size_t>(address);
        debug_registers.dr7.dr1_permissions = eastl::to_underlying(when);
        debug_registers.dr7.dr1_length = eastl::to_underlying(size);
        debug_registers.dr7.local_dr1_bp = 1;
      }
      break;
      case Slot::_2:
      {
        debug_registers.dr2 = dag::bit_cast<size_t>(address);
        debug_registers.dr7.dr2_permissions = eastl::to_underlying(when);
        debug_registers.dr7.dr2_length = eastl::to_underlying(size);
        debug_registers.dr7.local_dr2_bp = 1;
      }
      break;
      case Slot::_3:
      {
        debug_registers.dr3 = dag::bit_cast<size_t>(address);
        debug_registers.dr7.dr3_permissions = eastl::to_underlying(when);
        debug_registers.dr7.dr3_length = eastl::to_underlying(size);
        debug_registers.dr7.local_dr3_bp = 1;
      }
      break;
    }

    used_slots.fetch_or(eastl::to_underlying(slot));
  }

  logdbg("Set data breakpoint on 0x%p", address);
  ScopedLockReadTemplate debugRegisterLock{register_mutex};
  update_threads();
}

void remove(Slot slot)
{
  {
    ScopedLockWriteTemplate debugRegisterLock{register_mutex};

    switch (slot)
    {
      case Slot::_0:
      {
        debug_registers.dr7.local_dr0_bp = 0;
      }
      break;
      case Slot::_1:
      {
        debug_registers.dr7.local_dr1_bp = 0;
      }
      break;
      case Slot::_2:
      {
        debug_registers.dr7.local_dr2_bp = 0;
      }
      break;
      case Slot::_3:
      {
        debug_registers.dr7.local_dr3_bp = 0;
      }
      break;
    }

    used_slots.fetch_and(~(eastl::to_underlying(slot)));
  }

  ScopedLockReadTemplate debugRegisterLock{register_mutex};
  update_threads();
}

static eastl::function<void(void *, Slot)> action;
static void *handle = nullptr;

static long CALLBACK exception_handler(PEXCEPTION_POINTERS ex)
{
  G_ASSERT(action);

  if (ex->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP)
    return EXCEPTION_CONTINUE_SEARCH;

  auto dr6 = dag::bit_cast<Dr6>(ex->ContextRecord->Dr6);
  auto dr7 = dag::bit_cast<Dr7>(ex->ContextRecord->Dr7);
  Slot slot;
  if (dr6.dr0_bp_state)
  {
    dr6.dr0_bp_state = 0;
    slot = Slot::_0;

    if (dr7.dr0_permissions == 0)
      ex->ContextRecord->EFlags |= (1 << 16);
  }
  else if (dr6.dr1_bp_state)
  {
    dr6.dr1_bp_state = 0;
    slot = Slot::_1;

    if (dr7.dr1_permissions == 0)
      ex->ContextRecord->EFlags |= (1 << 16);
  }
  else if (dr6.dr2_bp_state)
  {
    dr6.dr2_bp_state = 0;
    slot = Slot::_2;

    if (dr7.dr2_permissions == 0)
      ex->ContextRecord->EFlags |= (1 << 16);
  }
  else if (dr6.dr3_bp_state)
  {
    dr6.dr3_bp_state = 0;
    slot = Slot::_3;

    if (dr7.dr3_permissions == 0)
      ex->ContextRecord->EFlags |= (1 << 16);
  }
  else
  {
    return EXCEPTION_CONTINUE_EXECUTION;
  }

  action(dag::bit_cast<void *>(ex->ContextRecord->Rip), slot);

  return EXCEPTION_CONTINUE_EXECUTION;
}

void on_break(eastl::function<void(void *, Slot)> callable)
{
  action = eastl::move(callable);
  if (action)
    handle = ::AddVectoredExceptionHandler(1, exception_handler);
  else
    ::RemoveVectoredExceptionHandler(handle);
}
} // namespace dag::hwbrk

#define EXPORT_PULL dll_pull_kernel_dagorHwBreakpoint
#include <supp/exportPull.h>
