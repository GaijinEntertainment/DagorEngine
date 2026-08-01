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
static void register_builtin(dafg::TypeDb &type_db, const das::ModuleLibrary &lib)
{
  auto mangledName = das::typeFactory<T>::make(lib)->getMangledName(true);

  auto rtti = dafg::detail::make_rtti<T>();
#if DAFG_DEBUG_RTTI
  rtti.dasName = String{mangledName.c_str()};
#endif
  type_db.registerInteropType(mangledName.c_str(), dafg::tag_for<T>(), eastl::move(rtti));
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

void dafg::detail::register_das_interop_type(const char *mangled_name, RTTI::TaggerRef tag_for, RTTI::MakerRef make_rtti,
  RTTI::RecursiveMakerRef make_rtti_rec)
{
  ::das::lock_guard lock{typeDbMutex};
  auto &typeDb = dafg::Runtime::get().getTypeDb();

  if (auto rttiPtr = typeDb.getRTTI(tag_for()); rttiPtr == nullptr)
  {
    dafg::detail::RTTI::TempStorage subRttis;
    auto [rootTag, rootRtti] = make_rtti_rec(subRttis);
#if DAFG_DEBUG_RTTI
    rootRtti.dasName = String{mangled_name};
#endif
    typeDb.registerInteropType(mangled_name, rootTag, eastl::move(rootRtti));
    for (auto &[tag, rtti] : subRttis)
      typeDb.registerNativeType(tag, eastl::move(rtti));
  }
  else
  {
    // if type is already registered, we make non-recursive rtti to update its das name
    auto rootRtti = make_rtti();
#if DAFG_DEBUG_RTTI
    rootRtti.dasName = String{mangled_name};
#endif
    typeDb.registerInteropType(mangled_name, tag_for(), eastl::move(rootRtti));
  }
}
