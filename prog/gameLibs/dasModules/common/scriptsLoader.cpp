#include <dasModules/dasScriptsLoader.h>
#include <util/dag_threadPool.h>

namespace bind_dascript
{
constexpr uint32_t stackSize = 16 * 1024;

das::StackAllocator &get_shared_stack()
{
  static thread_local das::StackAllocator sharedStack(stackSize);
  return sharedStack;
}
} // namespace bind_dascript
