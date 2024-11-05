// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shaderTab.h"
#include <generic/dag_tab.h>

class ShaderClass;
namespace shaders
{
struct RenderState;
}

enum class CompilerAction
{
  NOTHING,
  COMPILE_ONLY,
  LINK_ONLY,
  COMPILE_AND_LINK
};

namespace loadedshaders
{
extern Tab<TabFsh> fsh;
extern Tab<TabVpr> vpr;
extern Tab<TabStcode> stCode;
extern Tab<shaders::RenderState> render_state;
extern Tab<ShaderClass *> shClass;
} // namespace loadedshaders

bool get_file_time64(const char *fn, int64_t &ft);
CompilerAction check_scripted_shader(const char *filename, dag::ConstSpan<String> current_deps);
bool load_scripted_shaders(const char *filename, bool check_dep);
void unload_scripted_shaders();
