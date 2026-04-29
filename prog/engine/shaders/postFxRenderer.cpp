// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMesh.h>
#include <3d/dag_materialData.h>
#include <3d/dag_render.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_driver.h>
#include <math/dag_mathUtils.h>
#include <drv/3d/dag_commands.h>

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

void PostFxRenderer::render() const
{
  if (!shElem)
    return;

  d3d::setvsrc(0, 0, 0);
  shElem->render(0, 0, RELEM_NO_INDEX_BUFFER, 1, 0, PRIM_TRILIST);
};
