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
CompilerAction check_scripted_shader(const char *filename, dag::ConstSpan<String> current_deps, const ShCompilationInfo &comp,
  bool check_cppstcode);
bool load_scripted_shaders(const char *filename, bool check_dep, shc::TargetContext &out_ctx);
