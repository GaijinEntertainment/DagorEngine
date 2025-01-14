//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_d3dResource.h>

namespace shaders_internal
{
class ConstSetter;
}

namespace stcode
{

#if CPP_STCODE

enum class ExecutionMode
{
  CPP,
#if VALIDATE_CPP_STCODE
  TEST_CPP_AGAINST_BYTECODE,
#elif STCODE_RUNTIME_CHOICE
  BYTECODE,
#endif
};

ExecutionMode execution_mode();

/// Module init/deinit ///

// The path to dll will be deduced from shbindump path, they are required to be in the same dir
bool load(const char *shbindump_path_base);
void unload();
bool is_loaded();

/// Execution ///

void run_routine(size_t id, const void *vars, bool is_compute, int req_tex_level = 15, const char *new_name = nullptr);

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

inline bool load(const char *) { return true; }
inline void unload() {}
inline bool is_loaded() { return false; }
inline void run_routine(size_t, const void *, bool, int, const char *) {}

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
