// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_addressWait.h>
#include <windows.h> // synchapi.h
#include <debug/dag_assert.h>
#include <EASTL/type_traits.h>

#if _TARGET_XBOX || defined(_TARGET_ARCH_ARM64)
#pragma comment(lib, "Synchronization.lib")
#else
namespace windetails
{
static eastl::add_pointer_t<decltype(::WaitOnAddress)> WaitOnAddress;
static eastl::add_pointer_t<decltype(::WakeByAddressAll)> WakeByAddressAll;
static eastl::add_pointer_t<decltype(::WakeByAddressSingle)> WakeByAddressSingle;
} // namespace windetails
#endif

static_assert((uint32_t)-1 == INFINITE);

#if _TARGET_PC_WIN && !defined(_TARGET_ARCH_ARM64)
void init_win_wait_on_address()
{
  if (auto futexLib = !windetails::WaitOnAddress ? LoadLibraryA("API-MS-Win-Core-Synch-l1-2-0.dll") : nullptr) // For Win7
  {
    reinterpret_cast<FARPROC &>(windetails::WaitOnAddress) = GetProcAddress(futexLib, "WaitOnAddress");
    reinterpret_cast<FARPROC &>(windetails::WakeByAddressAll) = GetProcAddress(futexLib, "WakeByAddressAll");
    reinterpret_cast<FARPROC &>(windetails::WakeByAddressSingle) = GetProcAddress(futexLib, "WakeByAddressSingle");
  }
  G_ASSERT(!windetails::WaitOnAddress || windetails::WakeByAddressAll);
  G_ASSERT(!windetails::WaitOnAddress || windetails::WakeByAddressSingle);
}

os_wait_on_address_cb_t os_get_native_wait_on_address_impl()
{
  if (auto waitNative = windetails::WaitOnAddress)
    return [=](volatile uint32_t *a, const uint32_t *ca, int w) { waitNative(a, (void *)ca, sizeof(*a), w); };
  return {};
}

#define WaitOnAddress       windetails::WaitOnAddress
#define WakeByAddressAll    windetails::WakeByAddressAll
#define WakeByAddressSingle windetails::WakeByAddressSingle
#elif _TARGET_PC_WIN
void init_win_wait_on_address() {}
#endif

void os_wait_on_address(volatile uint32_t *addr, const uint32_t *cmpaddr, int wait_ms)
{
  if (os_get_native_wait_on_address_impl())
    WaitOnAddress(addr, (void *)cmpaddr, sizeof(*addr), wait_ms);
}

void os_wait_on_address_64(volatile uint64_t *addr, const uint64_t *cmpaddr, int wait_ms)
{
  if (os_get_native_wait_on_address_impl())
    WaitOnAddress(addr, (void *)cmpaddr, sizeof(*addr), wait_ms);
}

void os_wake_on_address_all(volatile uint32_t *addr)
{
  if (os_get_native_wait_on_address_impl())
    WakeByAddressAll((void *)addr);
}

void os_wake_on_address_one(volatile uint32_t *addr)
{
  if (os_get_native_wait_on_address_impl())
    WakeByAddressSingle((void *)addr);
}
