// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <hlslCompiler/hlslCompiler.h>

#include <osApiWrappers/dag_dynLib.h>
#include <debug/dag_assert.h>
#include <debug/dag_debug.h>
#include <debug/dag_logSys.h>
#include <debug/dag_log.h>

#include <EASTL/string.h>

#if _TARGET_PC_WIN
#include <windows.h>
#endif

namespace hlsl_compiler
{

using CompileFunction = bool (*)(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err);
using InitSettingsFunction = void (*)(debug_log_callback_t);

static void *g_dynlib_handle = nullptr;

#define TARGET_LIST                            \
  ITEM(DX11, dx11)                             \
  ITEM(PS4, ps4)                               \
  ITEM(VULKAN, spirv)                          \
  ITEM(DX12, dx12)                             \
  ITEM(DX12_XBOX_ONE, dx12_xbox_one)           \
  ITEM(DX12_XBOX_SCARLETT, dx12_xbox_scarlett) \
  ITEM(PS5, ps5)                               \
  ITEM(MTL, metal)                             \
  ITEM(VULKAN_BINDLESS, spirv_bindless)        \
  ITEM(MTL_BINDLESS, metal_bindless)

#define ITEM(_, suffix) static CompileFunction compile_compute_shader_##suffix = nullptr;
TARGET_LIST
#undef ITEM

bool is_initialized() { return g_dynlib_handle != nullptr; }

bool try_init_dynlib(const char *dll_root)
{
  if (g_dynlib_handle)
    return true;

  constexpr char dllNameBase[] = "hlslCompiler-dev";

#if _TARGET_PC_WIN

  eastl::string dllLookupPath = eastl::string(dll_root);
  for (char &c : dllLookupPath)
    if (c == '/')
      c = '\\';

  struct DllMount
  {
    DllMount(const char *path) { SetDllDirectoryA(path); }
    ~DllMount() { SetDllDirectoryA(nullptr); }
  } dllMount(dllLookupPath.c_str());

  eastl::string path(eastl::string::CtorSprintf{}, "%s" DAGOR_OS_DLL_SUFFIX, dllNameBase);

#else

  eastl::string path(eastl::string::CtorSprintf{}, "%s/%s" DAGOR_OS_DLL_SUFFIX, dll_root, dllNameBase);

#endif

  g_dynlib_handle = os_dll_load_deep_bind(path.c_str());

  if (!g_dynlib_handle)
    return false;

#define FETCH_FUNCTION(name)                                           \
  do                                                                   \
  {                                                                    \
    name = (CompileFunction)os_dll_get_symbol(g_dynlib_handle, #name); \
    if (name)                                                          \
      debug("[COMPUTE COMPILER] Loaded routine %s", #name);            \
    else                                                               \
      debug("[COMPUTE COMPILER] Failed to load routine %s!", #name);   \
  } while (0)

  auto init_settings = (InitSettingsFunction)os_dll_get_symbol(g_dynlib_handle, "init_settings");
  if (init_settings)
  {
    (*init_settings)(+[](int lev_tag, const char *fmt, const void *arg, int anum, const char *, int) -> int {
      logmessage(lev_tag, fmt, arg, anum);
      return 0; // false
    });
  }

#define ITEM(_, suffix) FETCH_FUNCTION(compile_compute_shader_##suffix);
  TARGET_LIST
#undef ITEM

#undef FETCH_FUNCTION

  return true;
}

void deinit_dynlib()
{
  G_ASSERT(is_initialized());
  os_dll_close(g_dynlib_handle);
  g_dynlib_handle = nullptr;
}

bool compile_compute_shader(Platform platform, dag::ConstSpan<char> hlsl_src, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err)
{
  G_ASSERT(is_initialized());

  CompileFunction compFunc = nullptr;

  switch (platform)
  {
#define ITEM(platform, suffix) \
  case Platform::platform: compFunc = compile_compute_shader_##suffix; break;
    TARGET_LIST
#undef ITEM

    default: G_ASSERT_RETURN(0, false);
  }

  if (!compFunc)
  {
    auto platformStr = [](Platform platform) {
      switch (platform)
      {
#define ITEM(platform, suffix) \
  case Platform::platform: return #suffix;
        // clang-format off
        TARGET_LIST
        // clang-format on
#undef ITEM

        default: G_ASSERT_RETURN(0, "");
      }
    };

    logerr("[COMPUTE COMPILER] Compute shader compilation for %s is not supported by the loaded dynlib", platformStr(platform));
    return false;
  }

  return compFunc(hlsl_src.cbegin(), hlsl_src.size(), entry, profile, shader_bin, out_err);
}

} // namespace hlsl_compiler