// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shCompiler.h"
#include "shaderTab.h"
#include "shCompiler.h"
#include "samplers.h"
#include "shTargetContext.h"
#include <generic/dag_tab.h>

class ShaderClass;
namespace shaders
{
struct RenderState;
}

bool get_file_time64(const char *fn, int64_t &ft);
CompilerAction check_scripted_shader(const char *filename, dag::ConstSpan<String> current_deps, const shc::CompilationContext &ctx,
  bool check_cppstcode);
bool load_scripted_shaders(const char *filename, bool check_dep, shc::TargetContext &out_ctx);

/// @brief Loads a per-source-file (local) refined block layout from a shader cache bindump.
/// Merges the refined block register entries from the cache file into @p context.rbVars().
/// @param filename Path to the local shader cache file (*.obj or equivalent).
/// @param context  Compilation context whose rbVars map is populated.
/// @return @c true on success, @c false if the file could not be read (error is logged).
bool load_local_rb_layout(const char *filename, shc::CompilationContext &context);

/// @brief Serialises the current refined block layout from @p comp into a global cache file.
/// Writes all rbVars entries as a bindump stream.
/// @param filename Path to the output global cache file.
/// @param comp     Compilation context whose rbVars map is serialised.
/// @return @c true on success, @c false if the file could not be opened for writing.
bool save_global_rb_layout(const char *filename, const shc::CompilationContext &comp);

/// @brief Loads the global refined block layout from a shared cache file produced by save_global_rb_layout().
/// Merges all stored refined block register entries into @p context.rbVars().
/// @param filename Path to the global cache file written by save_global_rb_layout().
/// @param context  Compilation context whose rbVars map is populated.
/// @return @c true on success, @c false if the file could not be read (error is logged).
bool load_global_rb_layout(const char *filename, shc::CompilationContext &context);
