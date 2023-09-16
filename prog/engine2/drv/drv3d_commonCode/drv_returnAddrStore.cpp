#include "drv_returnAddrStore.h"
#include <EASTL/vector_set.h>

thread_local const void *saved_return_address = nullptr;

// Some d3d functions call other d3d functions, so have to ignore those
ScopedReturnAddressStore::ScopedReturnAddressStore(const void *return_address)
{
#if _TARGET_PC_WIN || _TARGET_XBOX || _TARGET_C1
  G_ASSERT(return_address);
#endif
  localAddress = return_address;
  if (saved_return_address == nullptr)
    saved_return_address = localAddress;
}

ScopedReturnAddressStore::~ScopedReturnAddressStore()
{
  if (saved_return_address == localAddress)
    saved_return_address = nullptr;
}

const void *ScopedReturnAddressStore::get_threadlocal_saved_address() { return saved_return_address; }

namespace lockreturnaddr
{
static eastl::vector_set<const void *> global_ptrs;
thread_local const void *thread_local_ptr = nullptr;

void before_lock(const void *return_address)
{
  if (thread_local_ptr == nullptr)
    thread_local_ptr = return_address;
}
void after_successful_lock(const void *return_address)
{
  if (return_address && thread_local_ptr == return_address)
  {
    global_ptrs.insert(return_address);
    thread_local_ptr = nullptr;
  }
}
void after_failed_lock(const void *return_address)
{
  if (thread_local_ptr == return_address)
  {
    thread_local_ptr = nullptr;
  }
}
void before_unlock(int lock_count)
{
  if (lock_count == 0)
    global_ptrs.clear();
}
eastl::string ptrs_to_string()
{
  eastl::string result = "";
  for (const void *ptr : global_ptrs)
    result.append_sprintf("%p ", ptr);
  return result;
}
bool validate_addr_store(bool is_locked) { return global_ptrs.empty() != is_locked; }
} // namespace lockreturnaddr
