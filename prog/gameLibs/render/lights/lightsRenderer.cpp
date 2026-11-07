// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/lightsRenderer.h>
#include <render/lights/lightsRenderer.hlsli>

#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_driver.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaders.h>
#include <render/primitiveObjects.h>
#include <3d/dag_lockSbuffer.h>
#include <3d/dag_resourceTags.h>

LightsRenderer::LightsRenderer(const LightsResourcesManager *lights_res_mgr) : lightsResMgr(lights_res_mgr) {}

LightsRenderer::~LightsRenderer() { close(); }

const char *LightsRenderer::getResName(const char *name) const { return lightsResMgr->getResName(name); }

void LightsRenderer::init()
{
  initConeSphere();
  initSpot();
  initOmni();
  initDebugOmni();
  initDebugSpot();
  omniLightsVarId = ::get_shader_variable_id("omni_lights", false);
  spotLightsVarId = ::get_shader_variable_id("spot_lights", false);
}

void LightsRenderer::close()
{
  closeConeSphere();
  closeOmni();
  closeSpot();
  closeDebugOmni();
  closeDebugSpot();
}

void LightsRenderer::beforeResetDevice() { closeConeSphere(); }

void LightsRenderer::afterResetDevice() { initConeSphere(); }

void LightsRenderer::initConeSphere()
{
  static constexpr uint32_t SLICES = 5;
  calc_sphere_vertex_face_count(SLICES, SLICES, false, v_count, f_count);
  coneSphereVb.close();
  coneSphereVb = dag::create_vb((v_count + 5) * sizeof(Point3), 0, getResName("coneSphereVb"), RESTAG_LIGHTS);
  d3d_err((bool)coneSphereVb);
  coneSphereIb.close();
  coneSphereIb = dag::create_ib((f_count + 6) * 6, 0, getResName("coneSphereIb"), RESTAG_LIGHTS);
  d3d_err((bool)coneSphereIb);

  LockedBuffer<uint16_t> indicesLocked = lock_sbuffer<uint16_t>(coneSphereIb.getBuf(), 0, 0, VBLOCK_WRITEONLY);
  if (!indicesLocked)
    return;
  uint16_t *indices = indicesLocked.get();
  LockedBuffer<Point3> verticesLocked = lock_sbuffer<Point3>(coneSphereVb.getBuf(), 0, 0, VBLOCK_WRITEONLY);
  if (!verticesLocked)
    return;
  Point3 *vertices = verticesLocked.get();

  create_sphere_mesh(dag::Span<uint8_t>((uint8_t *)vertices, v_count * sizeof(Point3)),
    dag::Span<uint8_t>((uint8_t *)indices, f_count * 6), 1.0f, SLICES, SLICES, sizeof(Point3), false, false, false, false);
  vertices += v_count;
  vertices[0] = Point3(0, 0, 0);
  vertices[1] = Point3(-1, -1, 1);
  vertices[2] = Point3(+1, -1, 1);
  vertices[3] = Point3(-1, +1, 1);
  vertices[4] = Point3(+1, +1, 1);

  indices += f_count * 3;
  indices[0] = v_count + 0;
  indices[1] = v_count + 2;
  indices[2] = v_count + 1;
  indices += 3;
  indices[0] = v_count + 0;
  indices[1] = v_count + 3;
  indices[2] = v_count + 4;
  indices += 3;
  indices[0] = v_count + 0;
  indices[1] = v_count + 1;
  indices[2] = v_count + 3;
  indices += 3;
  indices[0] = v_count + 0;
  indices[1] = v_count + 4;
  indices[2] = v_count + 2;
  indices += 3;
  indices[0] = v_count + 1;
  indices[1] = v_count + 2;
  indices[2] = v_count + 3;
  indices += 3;
  indices[0] = v_count + 3;
  indices[1] = v_count + 2;
  indices[2] = v_count + 4;
}

LightsRenderer::IndexRanges LightsRenderer::getIndexRanges() const
{
  return {.omniIndexCount = f_count * 3, .omniStartIndex = 0, .spotIndexCount = 6 * 3, .spotStartIndex = f_count * 3};
}

void LightsRenderer::closeConeSphere()
{
  coneSphereVb.close();
  coneSphereIb.close();
}

void LightsRenderer::initOmni()
{
  closeOmni();
  pointLightsMat = new_shader_material_by_name("point_lights");
  G_ASSERT_RETURN(pointLightsMat, );
  pointLightsMat->addRef();
  pointLightsElem = pointLightsMat->make_elem();
}

void LightsRenderer::initSpot()
{
  closeSpot();
  spotLightsMat = new_shader_material_by_name("spot_lights");
  G_ASSERT_RETURN(spotLightsMat, );
  spotLightsMat->addRef();
  spotLightsElem = spotLightsMat->make_elem();
}

void LightsRenderer::closeOmni()
{
  pointLightsElem = NULL;
  del_it(pointLightsMat);
}

void LightsRenderer::closeSpot()
{
  spotLightsElem = NULL;
  del_it(spotLightsMat);
}

void LightsRenderer::initDebugOmni()
{
  closeDebugOmni();
  pointLightsDebugMat = new_shader_material_by_name_optional("debug_lights");
  if (!pointLightsDebugMat)
    return;
  pointLightsDebugMat->addRef();
  pointLightsDebugElem = pointLightsDebugMat->make_elem();
}

void LightsRenderer::initDebugSpot()
{
  closeDebugSpot();
  spotLightsDebugMat = new_shader_material_by_name_optional("debug_spot_lights");
  if (!spotLightsDebugMat)
    return;
  spotLightsDebugMat->addRef();
  spotLightsDebugElem = spotLightsDebugMat->make_elem();
}

void LightsRenderer::closeDebugOmni()
{
  pointLightsDebugElem = NULL;
  del_it(pointLightsDebugMat);
}

void LightsRenderer::closeDebugSpot()
{
  spotLightsDebugElem = NULL;
  del_it(spotLightsDebugMat);
}

void LightsRenderer::renderPrims(ShaderElement *elem, int buffer_var_id, D3DRESID lights_cb_id, Sbuffer *indirect_args,
  uint32_t args_byte_offset)
{
  d3d::setind(coneSphereIb.getBuf());
  d3d::setvsrc(0, coneSphereVb.getBuf(), sizeof(Point3));
  D3DRESID old_buffer = ShaderGlobal::get_buf(buffer_var_id);
  ShaderGlobal::set_buffer(buffer_var_id, lights_cb_id);
  elem->setStates(0, true);
  d3d::draw_indexed_indirect(PRIM_TRILIST, indirect_args, args_byte_offset);
  ShaderGlobal::set_buffer(buffer_var_id, old_buffer);
}

void LightsRenderer::renderFarOmniLights(const OmniLightsCB &far_omni_lights_cb, Sbuffer *indirect_args)
{
  if (!pointLightsElem)
    return;
  TIME_D3D_PROFILE(renderFarOmniLights);
  renderPrims(pointLightsElem, omniLightsVarId, far_omni_lights_cb.getId(), indirect_args,
    LIGHTS_RENDERER_INDIRECT_ARGS_FAR_OMNI_DRAW_OFFSET);
}

void LightsRenderer::renderFarSpotLights(const SpotLightsCB &far_spot_lights_cb, Sbuffer *indirect_args)
{
  if (!spotLightsElem)
    return;
  TIME_D3D_PROFILE(renderFarSpotLights);
  renderPrims(spotLightsElem, spotLightsVarId, far_spot_lights_cb.getId(), indirect_args,
    LIGHTS_RENDERER_INDIRECT_ARGS_FAR_SPOT_DRAW_OFFSET);
}

void LightsRenderer::renderDebugOmniLights(const OmniLightsCBs &omni_lights_cb, Sbuffer *indirect_args)
{
  if (!pointLightsDebugElem)
    return;
  TIME_D3D_PROFILE(renderDebugOmniLights);
  renderPrims(pointLightsDebugElem, omniLightsVarId, omni_lights_cb.clustered->getId(), indirect_args,
    LIGHTS_RENDERER_INDIRECT_ARGS_CLUSTERED_OMNI_DRAW_OFFSET);
  renderPrims(pointLightsDebugElem, omniLightsVarId, omni_lights_cb.far->getId(), indirect_args,
    LIGHTS_RENDERER_INDIRECT_ARGS_FAR_OMNI_DRAW_OFFSET);
}

void LightsRenderer::renderDebugSpotLights(const SpotLightsCBs &spot_lights_cb, Sbuffer *indirect_args)
{
  if (!spotLightsDebugElem)
    return;
  TIME_D3D_PROFILE(renderDebugSpotLights);
  renderPrims(spotLightsDebugElem, spotLightsVarId, spot_lights_cb.clustered->getId(), indirect_args,
    LIGHTS_RENDERER_INDIRECT_ARGS_CLUSTERED_SPOT_DRAW_OFFSET);
  renderPrims(spotLightsDebugElem, spotLightsVarId, spot_lights_cb.far->getId(), indirect_args,
    LIGHTS_RENDERER_INDIRECT_ARGS_FAR_SPOT_DRAW_OFFSET);
}
