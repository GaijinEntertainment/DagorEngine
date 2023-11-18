#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMesh.h>
#include <3d/dag_materialData.h>
#include <3d/dag_render.h>
#include <3d/dag_drv3d.h>
#include <math/dag_mathUtils.h>
#include <3d/dag_drv3dCmd.h>

PostFxRenderer::PostFxRenderer() = default;
PostFxRenderer::PostFxRenderer(PostFxRenderer &&other) = default;
PostFxRenderer &PostFxRenderer::operator=(PostFxRenderer &&other) = default;
PostFxRenderer::PostFxRenderer(const PostFxRenderer &other) = default;
PostFxRenderer &PostFxRenderer::operator=(const PostFxRenderer &other) = default;
PostFxRenderer::PostFxRenderer(const char *shader_name) { init(shader_name); }

PostFxRenderer::~PostFxRenderer() { clear(); }

void PostFxRenderer::clear()
{
  shmat = NULL;
  shElem = NULL;
}

void PostFxRenderer::init(const char *shader_name, bool is_optional)
{
  shmat = new_shader_material_by_name_optional(shader_name, nullptr);
  if (shmat.get())
    shElem = shmat->make_elem();
  if (!shmat.get() || !shElem.get())
    logmessage(is_optional ? LOGLEVEL_DEBUG : LOGLEVEL_ERR, "PostFxRenderer: shader '%s' not found.", shader_name);
}

void PostFxRenderer::drawInternal(int num_tiles) const
{
  if (!shElem)
    return;
  if (num_tiles > 1)
  {
    if (!shElem->setStates(0, true))
      return;
    const int stride = sizeof(float) * 2;
    for (int tileNo = 0; tileNo < num_tiles; tileNo++)
    {
      float left = cvt((float)tileNo, 0.f, (float)num_tiles, -1.f, 1.f);
      float right = cvt((float)(tileNo + 1), 0.f, (float)num_tiles, -1.f, 1.f);
      float verts[8] = {left, -1.f, right, -1.f, left, 1.f, right, 1.f};
      d3d::draw_up(PRIM_TRISTRIP, 2, verts, stride);
      d3d::driver_command(DRV3D_COMMAND_D3D_FLUSH, NULL, NULL, NULL);
    }
  }
  else // draw triangle
  {
    d3d::setvsrc(0, 0, 0);
    shElem->render(0, 0, RELEM_NO_INDEX_BUFFER, 1, 0, PRIM_TRILIST);
  }
};
