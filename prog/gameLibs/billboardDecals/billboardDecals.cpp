// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <billboardDecals/billboardDecals.h>
#include <decalMatrices/decalsMatrices.h>
#include <billboardDecals/matProps.h>
#include <shaders/dag_computeShaders.h>
#include <math/integer/dag_IPoint4.h>
#include <3d/dag_quadIndexBuffer.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_resPtr.h>
#include <perfMon/dag_statDrv.h>

static constexpr int MAX_HOLES_IN_IB = 16384;

static int billboard_decals_depth_maskVarId = -1;

BillboardDecals::BillboardDecals() :
  matrixManager(BILLBOARD_DECALS_MAX_MATRICES_COUNT, "billboard_decal_matrices", ECS_HASH("decals__bulletHoleMatrixId"), 0),
  numHoles(0),
  nextHoleNo(0),
  holesGenerator(0),
  clearSphereShader(0),
  holesTypes(1),
  maxHoles(0),
  softnessDistance(0.1),
  hardDistance(0.01)
{
  billboard_decals_depth_maskVarId = get_shader_variable_id("billboard_decals_depth_mask");
}

bool BillboardDecals::init_textures(dag::ConstSpan<const char *> diffuse, dag::ConstSpan<const char *> normal,
  const char *texture_name)
{
  if (!diffuse.size())
    return false;

  diffuseTex = dag::add_managed_array_texture(String(128, "%s_diff*", texture_name), diffuse, "billboard_decals_diff_tex");
  if (diffuseTex)
    diffuseTex->disableSampler();
  diffuseTex.setVar();
  if (!normal.empty())
  {
    bumpTex = dag::add_managed_array_texture(String(128, "%s_normal*", texture_name), normal, "billboard_decals_bump_tex");
    if (bumpTex)
      bumpTex->disableSampler();
    bumpTex.setVar();
  }
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    d3d::SamplerHandle sampler = d3d::request_sampler(smpInfo);
    ShaderGlobal::set_sampler(get_shader_variable_id("billboard_decals_diff_tex_samplerstate", true), sampler);
    ShaderGlobal::set_sampler(get_shader_variable_id("billboard_decals_bump_tex_samplerstate", true), sampler);
  }
  // Textures.

  TextureInfo tinfo;
  w = 128, h = 128;
  if (diffuseTex && diffuseTex->getinfo(tinfo, 0))
  {
    w = tinfo.w, h = tinfo.h;
  }

  holesTypes = diffuse.size();

  return true;
}

bool BillboardDecals::init(float hard_dist, float soft_dist, unsigned max_holes, const char *generator_shader_name,
  const char *clear_sphere_shader_name, const char *decal_shader_name)
{
  close();
  // Create material.
  maxHoles = min(max_holes, (unsigned)MAX_HOLES_IN_IB);
  matrixManager.setMaxItems(maxHoles);
  holesRenderer.init(decal_shader_name, NULL, 0, "billboard");
  freeDecalIds.resize(maxHoles);

  // todo: on dx10, we can just use texture and copy data there
  holesGenerator = new_compute_shader(generator_shader_name);
  if (!holesGenerator)
    return false;
  clearSphereShader = new_compute_shader(clear_sphere_shader_name);
  if (!clearSphereShader)
    return false;
  holesInstances =
    dag::buffers::create_ua_sr_structured(sizeof(BillboardDecalInstance), maxHoles, "holesInstances", dag::buffers::Init::Zero);
  if (!holesInstances)
    return false;

  // VB and IB.
  index_buffer::init_quads_16bit();

  softnessDistance = soft_dist;
  hardDistance = hard_dist;
  return true;
}

BillboardDecals *BillboardDecals::create(float hard_distance, float softness_distance, unsigned max_holes,
  const char *generator_shader_name, const char *clear_sphere_shader_name, const char *decal_shader_name)
{
  BillboardDecals *bh = new BillboardDecals();
  if (bh->init(hard_distance, softness_distance, max_holes, generator_shader_name, clear_sphere_shader_name, decal_shader_name))
    return bh;
  delete bh;
  return NULL;
}

void BillboardDecals::close()
{
  index_buffer::release_quads_16bit();
  maxHoles = 0;
  numFreeDecals = 0;
  freeDecalIds.clear();
}

BillboardDecals::~BillboardDecals() { close(); }

void BillboardDecals::prepareRender(const Frustum & /*frustum*/, const Point3 & /*origin*/)
{
  G_STATIC_ASSERT(sizeof(holesToUpdate[0]) % 16 == 0);
  G_STATIC_ASSERT(sizeof(spheresToClear[0]) % 16 == 0);

  while (eastl::optional<uint32_t> decalId = matrixManager.popReusableItem())
    removeHole(decalId.value());

  if (numHoles == 0 && holesToUpdate.size() == 0 && spheresToClear.size() == 0)
    return;
  if (holesToUpdate.size())
  {
    // todo: on DX9/Dx10 replace that with shader that writes to texture (rendering points to texture).

    IPoint4 params(nextHoleNo, holesToUpdate.size(), maxHoles, 0);
    d3d::set_cs_const(0, (float *)&params.x, 1);
    d3d::set_cs_const(1, (float *)&holesToUpdate[0], holesToUpdate.size() * ((elem_size(holesToUpdate) + 15) / 16));
    d3d::set_rwbuffer(STAGE_CS, 0, holesInstances.getBuf());
    holesGenerator->dispatch(1, (holesToUpdate.size() + DECALS_WARP_X * DECALS_WARP_X - 1) / (DECALS_WARP_X * DECALS_WARP_X), 1);
    d3d::set_rwbuffer(STAGE_CS, 0, 0);
    d3d::resource_barrier({holesInstances.getBuf(), RB_RO_SRV | RB_STAGE_VERTEX});

    holesToUpdate.clear();
  }
  if (spheresToClear.size())
  {
    int spheresToClearPerFrame = eastl::min(MAX_SPHERES_TO_CLEAR, (int)spheresToClear.size());
    if (numHoles > 0)
    {
      IPoint4 param_holes = IPoint4(numHoles, spheresToClearPerFrame, 0, 0);
      // Point4 param_area = Point4::xyzV(pos, radius);
      d3d::set_cs_const(0, (float *)&param_holes.x, 1);
      // d3d::set_cs_const(1, (float*)&param_area.x, 1);
      d3d::set_cs_const(1, (float *)&spheresToClear[0], spheresToClearPerFrame * ((elem_size(spheresToClear) + 15) / 16));
      d3d::set_rwbuffer(STAGE_CS, 0, holesInstances.getBuf());
      clearSphereShader->dispatch(1, (numHoles + DECALS_WARP_X * DECALS_WARP_X - 1) / (DECALS_WARP_X * DECALS_WARP_X), 1);
      d3d::set_rwbuffer(STAGE_CS, 0, 0);
      d3d::resource_barrier({holesInstances.getBuf(), RB_RO_SRV | RB_STAGE_VERTEX});
    }
    spheresToClear.erase(spheresToClear.begin(), spheresToClear.begin() + spheresToClearPerFrame);
  }

  // todo: on dx11 make dispatch for frustum culling and decals matrix transform.
  // currently, vertex processing of all 10000 billboards is about ~0.03msec on my laptop. If we reach maximum of 16384 it is still
  // 0.05msec. of course, compute shader is saving 4 times of time for transforming vertices and about the same for frustum culling
  // (plus we can add distance based fade out) minus obvious bandwidth (to write-read memory), and memory footprint in general however,
  // maximum gain will be 0.04msec (0.05->0.01), and may be a bit more than that for more complicated decals may be I will do that
  // later, it is still -0.04msec, not plus 0.04msec. it makes much more sense for more tesselated decals, though.
}

void BillboardDecals::clearInSphere(const Point3 &pos, float sphere_radius)
{
  spheresToClear.push_back(Point4::xyzV(pos, sphere_radius));
}

void BillboardDecals::render()
{
  TIME_D3D_PROFILE(BillboardDecals_render);
  if (numHoles == 0 || !diffuseTex)
    return;

  matrixManager.beforeRender();
  index_buffer::use_quads_16bit();

  ShaderGlobal::set_color4(billboard_decals_depth_maskVarId, 1.f + hardDistance / softnessDistance, -1.f / softnessDistance, w, h);

  holesRenderer.shader->setStates(0, true);
  // todo: on Dx9 we need vertex buffer. It can be as simple as vertexId in each vertex (4 bytes per vertices)
  d3d::setvsrc(0, 0, 0);
  // todo: on Dx9/DX10 replace holesInstances with texture
  d3d::set_buffer(STAGE_VS, 0, holesInstances.getBuf());
  d3d::drawind(PRIM_TRILIST, 0, numHoles * 2, 0);
  d3d::set_buffer(STAGE_VS, 0, 0);
}

int32_t BillboardDecals::addHole(const Point3 &pos, const Point3 &norm, const Point3 &up, float size, uint32_t texture_id,
  uint32_t matrix_id)
{
  uint32_t hole_id = nextHoleNo;
  if (numFreeDecals > 0)
    hole_id = freeDecalIds[numFreeDecals - 1];
  if (!updateHole(pos, norm, up, size, hole_id, texture_id, matrix_id, false))
    return -1;
  if (hole_id == nextHoleNo)
  {
    nextHoleNo = (nextHoleNo + 1) % maxHoles;
    numHoles = min(numHoles + 1, maxHoles);
  }
  else
    --numFreeDecals;
  return hole_id;
}

bool BillboardDecals::updateHole(const Point3 &pos, const Point3 &norm, const Point3 &up, float size, uint32_t id, uint32_t texture_id,
  uint32_t matrix_id, bool allow_rapid_update)
{
  if (texture_id >= holesTypes)
    return false;
#if DAGOR_DBGLEVEL > 0
  // Same decal cannot be updated twice in the same frame
  if (!allow_rapid_update)
    for (unsigned int i = 0; i < holesToUpdate.size(); ++i)
      G_ASSERT(holesToUpdate[i].id != id);
#endif
  BillboardToUpdate *decalItr = holesToUpdate.end();
  if (allow_rapid_update)
  {
    // A bullet hole can be created and deleted in the same frame, when an object is destroyed by the bullet
    decalItr = eastl::find_if(holesToUpdate.begin(), holesToUpdate.end(), [id](const BillboardToUpdate &b) { return b.id == id; });
  }
  if (decalItr == holesToUpdate.end())
  {
    if (holesToUpdate.size() < holesToUpdate.capacity())
      append_items(holesToUpdate, 1);
    else
      return false;
    decalItr = holesToUpdate.end() - 1;
  }
  decalItr->pos_size = Point4::xyzV(pos, size);
  decalItr->normal = norm;
  decalItr->texture_id = texture_id;
  decalItr->up = up;
  decalItr->matrix_id = matrix_id;
  decalItr->id = id;
  matrixManager.useMatrixId(matrix_id, id);
  return true;
}
void BillboardDecals::removeHole(uint32_t id)
{
  G_ASSERT(numFreeDecals < freeDecalIds.size()); // Probably a decal has been deleted mulptiple times
  freeDecalIds[numFreeDecals++] = id;
  updateHole(Point3(0, 0, 0), Point3(0, 0, 0), Point3(0, 0, 0), 0, id, 0, DecalsMatrices::INVALID_MATRIX_ID, true);
}

void BillboardDecals::afterReset() { clear(); }
void BillboardDecals::clear()
{
  numHoles = nextHoleNo = 0;
  holesToUpdate.clear();
  matrixManager.clearItems();
}
