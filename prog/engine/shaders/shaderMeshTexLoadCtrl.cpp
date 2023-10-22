#include <shaders/dag_shaderMeshTexLoadCtrl.h>

IShaderMatVdataTexLoadCtrl *dagor_sm_tex_load_controller = NULL;
static thread_local int ctx_type = 0;
static thread_local const char *ctx_name = nullptr;

const char *IShaderMatVdataTexLoadCtrl::preprocess_tex_name(const char *original_tex_name, String &tmp_storage)
{
  if (!dagor_sm_tex_load_controller)
    return original_tex_name;
  return dagor_sm_tex_load_controller->preprocessMatVdataTexName(original_tex_name, ctx_type, ctx_name, tmp_storage);
}

void dagor_set_sm_tex_load_ctx_type(unsigned obj_type) { ctx_type = obj_type; }
void dagor_set_sm_tex_load_ctx_name(const char *obj_name) { ctx_name = obj_name; }
void dagor_reset_sm_tex_load_ctx()
{
  ctx_type = 0;
  ctx_name = NULL;
}
