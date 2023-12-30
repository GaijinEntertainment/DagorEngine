#include <render/computeShaderFallback/voltexRenderer.h>
#include <3d/dag_drv3d_multi.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <util/dag_string.h>

static int dispatchSizeVarId = VariableMap::BAD_ID;
static int sliceOffsetVarId = VariableMap::BAD_ID;

void VoltexRenderer::init(const char *compute_shader, const char *pixel_shader)
{
  voxelPerGroupX = voxelPerGroupY = voxelPerGroupZ = 0;
  if (is_compute_supported())
  {
    shaderCs.reset(new_compute_shader(compute_shader, false));
    G_ASSERTF(shaderCs, "shader %s is missed, were shaders rebuilded?", compute_shader);
  }
  if (!shaderCs)
  {
    if (sliceOffsetVarId == VariableMap::BAD_ID || dispatchSizeVarId == VariableMap::BAD_ID)
    {
      dispatchSizeVarId = get_shader_variable_id("dispatch_size");
      sliceOffsetVarId = get_shader_variable_id("slice_offset");
    }
    shaderPs.init(pixel_shader);
  }
}

void VoltexRenderer::init(const char *compute_shader, const char *pixel_shader, int voxel_per_group_x, int voxel_per_group_y,
  int voxel_per_group_z)
{
  init(compute_shader, pixel_shader);
  voxelPerGroupX = voxel_per_group_x;
  voxelPerGroupY = voxel_per_group_y;
  voxelPerGroupZ = voxel_per_group_z;
}

void VoltexRenderer::render(const ManagedTex &voltex, int mip_level, IPoint3 shape, IPoint3 offset)
{
  if (!voltex.getVolTex())
    return;
  TextureInfo tinfo;
  voltex.getVolTex()->getinfo(tinfo);
  if (shape.x == -1)
    shape.x = tinfo.w >> mip_level;
  if (shape.y == -1)
    shape.y = tinfo.h >> mip_level;
  if (shape.z == -1)
    shape.z = tinfo.d >> mip_level;

  if (shaderCs)
  {
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, mip_level), voltex.getVolTex());
    if (voxelPerGroupX > 0 && voxelPerGroupY > 0 && voxelPerGroupZ > 0)
      shaderCs->dispatch((shape.x + voxelPerGroupX - 1) / voxelPerGroupX, (shape.y + voxelPerGroupY - 1) / voxelPerGroupY,
        (shape.z + voxelPerGroupZ - 1) / voxelPerGroupZ);
    else
      shaderCs->dispatchThreads(shape.x, shape.y, shape.z);
  }
  else if (shaderPs.getElem())
  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(0, voltex.getVolTex(), d3d::RENDER_TO_WHOLE_ARRAY, mip_level);
    if (shape.x != (tinfo.w >> mip_level) || shape.y != (tinfo.h >> mip_level) || shape.z != (tinfo.d >> mip_level))
      d3d::setview(offset.x, offset.y, shape.x, shape.y, 0.0, 1.0);
    ShaderGlobal::set_real(sliceOffsetVarId, offset.z);
    ShaderGlobal::set_color4(dispatchSizeVarId, Color4(shape.x, shape.y, shape.z, 0.0));
    shaderPs.getElem()->setStates();
    d3d::draw_instanced(PRIM_TRILIST, 0, 1, shape.z);
    ShaderGlobal::set_real(sliceOffsetVarId, 0);
  }
  else
  {
    DAG_FATAL("can't render not inited VoltexRenderer");
  }
}

bool VoltexRenderer::is_compute_supported() { return d3d::get_driver_desc().shaderModel >= 5.0_sm; }

bool VoltexRenderer::isComputeLoaded() const { return bool(shaderCs); }