#include <gui/dag_visualLog.h>
#include <util/dag_string.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_overrideStateId.h>
#include <shaders/dag_renderStateId.h>
#include <math/dag_curveParams.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>

void visuallog::logmsg(char const *, bool (*)(int, visuallog::LogItem *), void *, struct E3DCOLOR, int) { DAG_FATAL("stub"); }

namespace wingw
{
String file_open_dlg(void *phandle, const char caption[], const char filter[], const char def_ext[], const char init_path[],
  const char init_fn[])
{
  DAG_FATAL("stub");
  return String();
}

int message_box(int flags, const char *caption, const char *text, const DagorSafeArg *arg, int anum)
{
  DAG_FATAL("stub");
  return 0;
}
} // namespace wingw

void dagor_reset_spent_work_time() {}

void *(*CubicCurveSampler::mem_allocator)(size_t) = NULL;
void (*CubicCurveSampler::mem_free)(void *) = NULL;

#if !_TARGET_STATIC_LIB
void ShaderGlobal::setBlock(int block_id, int layer, bool flush_cache) {}
int ShaderGlobal::getBlock(int layer) { return -1; }
void ShaderElement::invalidate_cached_state_block() {}
ShaderMaterial *new_shader_material_by_name_optional(const char *, const char *, bool) { return nullptr; }

shaders::OverrideStateId shaders::overrides::create(const shaders::OverrideState &) { return shaders::OverrideStateId{}; }
bool shaders::overrides::destroy(shaders::OverrideStateId &) { return false; }
bool shaders::overrides::exists(shaders::OverrideStateId) { return false; }
bool shaders::overrides::set(shaders::OverrideStateId) { return false; }

shaders::RenderStateId shaders::render_states::create(const shaders::RenderState &) { return shaders::RenderStateId{}; }
void shaders::render_states::set(shaders::RenderStateId) {}
uint32_t shaders::render_states::get_count() { return 0; }
#endif
