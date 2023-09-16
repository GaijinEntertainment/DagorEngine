#pragma once

#include "shaderTab.h"
#include <generic/dag_tab.h>

class ShaderClass;
namespace shaders
{
struct RenderState;
}

namespace loadedshaders
{
extern Tab<TabFsh> fsh;
extern Tab<TabVpr> vpr;
extern Tab<TabStcode> stCode;
extern Tab<shaders::RenderState> render_state;
extern Tab<ShaderClass *> shClass;
} // namespace loadedshaders

bool get_file_time64(const char *fn, int64_t &ft);
bool check_scripted_shader(const char *filename, bool check_dep);
bool load_scripted_shaders(const char *filename, bool check_dep);
void unload_scripted_shaders();
