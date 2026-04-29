//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_d3dResource.h>

#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/optional.h>
#include <dag/dag_vector.h>
#include <dag/dag_vectorSet.h>
#include <generic/dag_fixedMoveOnlyFunction.h>
#include <generic/dag_span.h>

namespace shaders_internal
{
class ConstSetter;
}

namespace stcode
{

using SpecialBlkTagInterpreter = dag::FixedMoveOnlyFunction<64, eastl::optional<eastl::string>(eastl::string_view)>;

#if CPP_STCODE

enum class ExecutionMode
{
  CPP,
  BYTECODE,
#if VALIDATE_CPP_STCODE
  TEST_CPP_AGAINST_BYTECODE,
#endif
};

#if CPP_STCODE_DYNAMIC_BRANCHING
inline constexpr bool USE_BRANCHED_DYNAMIC_ROUTINES = true;
#else
inline constexpr bool USE_BRANCHED_DYNAMIC_ROUTINES = false;
#endif

struct Context
{
  using DynRoutinePtr = void (*)(const void *, uint32_t);
  using StRoutinePtr = void (*)(void *);

  void *dllHandle = nullptr;

  dag::ConstSpan<DynRoutinePtr> dynRoutineStorageView{};
  dag::ConstSpan<StRoutinePtr> stRoutineStorageView{};
  ExecutionMode dynstcodeExMode = ExecutionMode::CPP;
  ExecutionMode stblkcodeExMode = ExecutionMode::CPP;

  SpecialBlkTagInterpreter specialTagInterpreter = [](auto) { return eastl::nullopt; };

#if _TARGET_PC && DAGOR_DBGLEVEL > 0
  dag::VectorSet<eastl::string> tmpFilesToDelete{};
  uint16_t tmpFilesGen = 0;
  bool attemptedInitialCleanup = false;
  bool generationsWrappedAround = false;
#endif
};

/// Module init/deinit ///

// The path to dll will be deduced from shbindump path, they are required to be in the same dir
bool load(Context &ctx, const char *shbindump_path_base);
void unload(Context &ctx);
bool is_loaded(const Context &ctx);

void disable(Context &ctx);

void set_special_tag_interp(Context &ctx, SpecialBlkTagInterpreter &&interp);

/// Execution ///

void run_dyn_routine(const Context &ctx, size_t id, const void *vars, uint32_t dyn_offset = -1, int req_tex_level = 15,
  const char *new_name = nullptr);
void run_st_routine(const Context &ctx, size_t id, void *execution_context);

/// Callbacks for resource interaction ///

// Can be called once if such a callback is required
using OnBeforeResourceUsedCb = void (*)(const D3dResource *, const char *);
void set_on_before_resource_used_cb(OnBeforeResourceUsedCb new_cb = nullptr);

void set_custom_const_setter(shaders_internal::ConstSetter *setter);

class ScopedCustomConstSetter
{
public:
  ScopedCustomConstSetter(shaders_internal::ConstSetter *);
  ~ScopedCustomConstSetter();

private:
  shaders_internal::ConstSetter *prev;
};

#else

inline constexpr bool USE_BRANCHED_DYNAMIC_ROUTINES = false;

struct Context
{};

inline bool load(Context &, const char *) { return true; }
inline void unload(Context &) {}
inline bool is_loaded(const Context &) { return false; }

inline void disable(Context &) {}

inline void set_special_tag_interp(Context &, SpecialBlkTagInterpreter &&) {}

inline void run_dyn_routine(const Context &, size_t, const void *, uint32_t = -1, int = 15, const char * = nullptr) {}
inline void run_st_routine(const Context &, size_t, void *) {}

template <class... Args>
inline void set_on_before_resource_used_cb(Args...)
{}

inline void set_custom_const_setter(shaders_internal::ConstSetter *) {}

class ScopedCustomConstSetter
{
  ScopedCustomConstSetter(shaders_internal::ConstSetter *) {}
};

#endif

} // namespace stcode
