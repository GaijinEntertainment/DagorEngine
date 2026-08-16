// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shBindumps.h>

class String;

// Editor-only: compiles an assembled texgen .dshl (see texgen_dshl_assemble.h) to a host-platform
// shader bindump via the offline dsc2 compiler, and loads/reloads it as an additional bindump.
// A near-verbatim adaptation of NodeBasedShaderManager::compileScriptedShaders, restricted to the
// editor host driver (dsc2-hlsl11 on Windows, dsc2-spirv on Linux) since texgen runs only in-editor.
struct TexgenDshlCompiler
{
  // Reads debug{toolsPathWin,rootPathWin} from dgs settings (same as NodeBasedShaderManager::initCompilation).
  // Returns false (with out_err) if the dsc2 toolchain path is not configured.
  static bool initPaths(String &out_err);

  // dsc2 subprocess ONLY: compiles `dshl_text` to a bindump on disk named `<dump_name_base>.ps50.shdump.bin`.
  // This is the SLOW part (multi-second) and MUST be called OUTSIDE d3d::GpuAutoLock so it never stalls
  // the main render thread. On success, `out_dump_base` receives the absolute dump base path for loadOrReload.
  // The generated .dshl is kept on failure (path in the debug log) for diagnosis.
  static bool compileToDump(const char *dshl_text, const char *dump_name_base, String &out_dump_base, String &out_err);

  // Loads (first time) or hot-reloads (live handle) the dump produced by compileToDump. Fast; safe to
  // call under GpuAutoLock. On success `handle` references the loaded additional bindump.
  static bool loadOrReload(const char *dump_base, ShaderBindumpHandle &handle, String &out_err);
};
