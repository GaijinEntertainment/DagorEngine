// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/ast/ast.h>
#include <util/dag_inttypes.h>

#include <runtime/runtime.h>
#include <render/daFrameGraph/das/registerBlobType.h>


// NOTE: das script loading is multi-threaded, but daFG runtime is not,
// so only some functions need to use the mutex.
static das::mutex typeDbMutex;
static bool builtinsRegistered;

#define BUILTIN_TYPES \
  X(bool)             \
  X(int8_t)           \
  X(uint8_t)          \
  X(int16_t)          \
  X(uint16_t)         \
  X(int32_t)          \
  X(uint32_t)         \
  X(int64_t)          \
  X(uint64_t)         \
  X(float)            \
  X(double)

template <class T>
static void register_builtin(dafg::TypeDb &typeDb, const das::ModuleLibrary &lib)
{
  auto mangledName = das::typeFactory<T>::make(lib)->getMangledName(true);
  typeDb.registerInteropType(mangledName.c_str(), dafg::tag_for<T>(), dafg::detail::make_rtti<T>());
}

void try_register_builtins(const das::ModuleLibrary &lib)
{
  if (eastl::exchange(builtinsRegistered, true))
    return;

  auto &typeDb = dafg::Runtime::get().getTypeDb();

#define X(T) register_builtin<T>(typeDb, lib);
  BUILTIN_TYPES
#undef X
}

void dafg::detail::register_das_interop_type(const char *mangled_name, dafg::ResourceSubtypeTag tag, RTTI &&rtti)
{
  ::das::lock_guard lock{typeDbMutex};
  dafg::Runtime::get().getTypeDb().registerInteropType(mangled_name, tag, eastl::move(rtti));
}
