// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_bounds2.h>
#include <util/dag_globDef.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <perfMon/dag_statDrv.h>
#include <scene/dag_physMat.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>
#include <debug/dag_debug.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_mathUtils.h>
#include <3d/dag_render.h>
#include <image/dag_texPixel.h>
#include <3d/dag_ringCPUTextureLock.h>
#include <shaders/dag_shaderBlock.h>
#include <math/integer/dag_IBBox2.h>
#include <render/toroidalHeightmap.h>
#include <util/dag_convar.h>
#include <render/groundDetailsHandler.h>

#if DAGOR_DBGLEVEL > 0
#include <util/dag_console.h>
#endif

GroundDisplacementCPU::GroundDisplacementCPU()
{
  renderHeightAndDisplacement.init("copy_height_displacement");
  renderGPUHeightAndDisplacement.init("copy_height_displacement_GPU");
}

GroundDisplacementCPU::~GroundDisplacementCPU()
{
  ringTextures.close();
  clear_and_shrink(loadedDisplacement);
  groundPhysDetailsTex.close();
  GPUgroundPhysDetailsTex.close();
}

void GroundDisplacementCPU::init(int tex_size, float heightmap_texel_size, float invalidation_scale)
{
  bufferSize = tex_size;
  texelSize = heightmap_texel_size;
  worldSize = bufferSize * texelSize;
  invalidationScale = invalidation_scale;
  origin = Point2(0, 0);
  curOrigin = origin;
  physDetailsOrigin = Point3(0, 0, 0);
  physDetailsCurOrigin = physDetailsOrigin;
  worldToDisplacement = Point4(0, 0, 0, 0);

  // texture for CPU copy
  ringTextures.init(bufferSize, bufferSize, 1, "CPU_displacement",
    TEXFMT_R32F | TEXCF_RTARGET | TEXCF_LINEAR_LAYOUT | TEXCF_CPU_CACHED_MEMORY);

  // detailed tex for depth above render (for small rendinsts/static scene)
  groundPhysDetailsTex.set(
    d3d::create_tex(NULL, 4 * bufferSize, 4 * bufferSize, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "ground_microdetails_tex"),
    "ground_microdetails_tex");
  groundPhysDetailsTex.getTex2D()->disableSampler();
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    ShaderGlobal::set_sampler(::get_shader_glob_var_id("ground_physdetails_tex_samplerstate"), d3d::request_sampler(smpInfo));
  }

  // texture for gpu physics simulations
  GPUgroundPhysDetailsTex.set(
    d3d::create_tex(NULL, 2 * bufferSize, 2 * bufferSize, TEXCF_RTARGET | TEXFMT_R32F, 1, "GPUground_physdetails_tex"),
    "GPUground_physdetails_tex");

  loadedDisplacement.resize(bufferSize * bufferSize);
  mem_set_0(loadedDisplacement);
  forceUpdateCounter = 0;
}


void GroundDisplacementCPU::updateGroundPhysdetails(IRenderGroundPhysdetailsCB *render_depth_cb, VisibilityFinder &vf)
{
  if (Point3(physDetailsOrigin - physDetailsCurOrigin).length() < worldSize)
    return;

  physDetailsCurOrigin = physDetailsOrigin;

  SCOPE_RENDER_TARGET;
  SCOPE_VIEW_PROJ_MATRIX;

  const float add_height = 10;

  Point3 centerPos = Point3(texelSize * floorf(physDetailsCurOrigin.x / texelSize), physDetailsCurOrigin.y,
    texelSize * floorf(physDetailsCurOrigin.z / texelSize));

  TMatrix vtm = TMatrix::IDENT;
  vtm.setcol(0, 1, 0, 0);
  vtm.setcol(1, 0, 0, 1);
  vtm.setcol(2, 0, 1, 0);
  d3d::settm(TM_VIEW, vtm);

  d3d::set_render_target();
  d3d::set_render_target(0, (Texture *)NULL, 0);
  d3d::set_depth(groundPhysDetailsTex.getTex2D(), DepthAccess::RW);

  float regionSize = texelSize * bufferSize;

  BBox2 region(Point2(centerPos.x - regionSize, centerPos.z - regionSize), Point2(centerPos.x + regionSize, centerPos.z + regionSize));
  TMatrix4 proj;
  proj =
    matrix_ortho_off_center_lh(region[0].x, region[1].x, region[0].y, region[1].y, centerPos.y - add_height, centerPos.y + add_height);

  d3d::settm(TM_PROJ, &proj);

  mat44f culling_view_proj;
  d3d::getglobtm(culling_view_proj);

  d3d::setview(0, 0, 4 * bufferSize, 4 * bufferSize, 0, 1);
  d3d::clearview(CLEAR_ZBUFFER, 0, 0.0f, 0);

  if (render_depth_cb)
    render_depth_cb->renderGroundPhysdetails(centerPos, culling_view_proj, vf);

  static int world_to_ground_physdetailsVarId = get_shader_variable_id("world_to_ground_physdetails", true);
  ShaderGlobal::set_color4(world_to_ground_physdetailsVarId, 0.5f / regionSize, 0.5f / regionSize,
    0.5f - 0.5f * centerPos.x / regionSize, 0.5f - 0.5f * centerPos.z / regionSize);
  static int ground_physdetails_paramsVarId = get_shader_variable_id("ground_physdetails_params", true);
  ShaderGlobal::set_color4(ground_physdetails_paramsVarId, centerPos.y - add_height, 2.0f * add_height, 1.0f / (4 * bufferSize), 0);
}

bool GroundDisplacementCPU::process(float *destData, uint32_t &frame)
{
  int stride;
  uint8_t *data = ringTextures.lock(stride, frame);
  if (!data)
    return false;
  for (int y = 0; y < bufferSize; ++y, destData += bufferSize, data += stride)
    memcpy(destData, data, bufferSize * sizeof(float));
  ringTextures.unlock();
  return true;
}

void GroundDisplacementCPU::invalidate()
{
  origin = Point2(10000000, 1000000);
  curOrigin = origin;
  physDetailsOrigin = Point3(1000000, 1000000, 1000000);
  physDetailsCurOrigin = physDetailsOrigin;
  forceUpdateCounter = 0;
}

void GroundDisplacementCPU::beforeRenderFrame(float dt) { deltaTime = dt; }

void GroundDisplacementCPU::update(const Point3 &center_pos, ToroidalHeightmap *hmap)
{
  forceUpdateCounter++;

  physDetailsOrigin = center_pos;
  uint32_t frame2;
  if (process(loadedDisplacement.data(), frame2)) // written data
  {
    curOrigin = origin;

    worldToDisplacement = Point4(1.0f / worldSize, 1.0f / worldSize, -curOrigin.x + 0.5f * worldSize, -curOrigin.y + 0.5f * worldSize);
  }

  if (Point2(Point2(center_pos.x, center_pos.z) - curOrigin).length() < worldSize * invalidationScale && forceUpdateCounter < 100)
    return;

  int frame;
  Texture *currentTarget = ringTextures.getNewTarget(frame);
  if (!currentTarget)
    return;
  {
    origin = Point2(texelSize * floorf(center_pos.x / texelSize), texelSize * floorf(center_pos.z / texelSize));
    SCOPE_RENDER_TARGET;

    hmap->setTexFilter(d3d::FilterMode::Point);
    hmap->setHeightmapTex();

    static int groundDetails_to_worldVarId = get_shader_variable_id("groundDetails_to_world", true);
    ShaderGlobal::set_color4(groundDetails_to_worldVarId, worldSize, worldSize, origin.x - 0.5f * worldSize,
      origin.y - 0.5f * worldSize);

    static int ground_physdetails_texVarId = ::get_shader_glob_var_id("ground_physdetails_tex");
    ShaderGlobal::set_texture_fast(ground_physdetails_texVarId, groundPhysDetailsTex.getId());

    d3d::set_render_target(currentTarget, 0);
    renderHeightAndDisplacement.render();

    // restore filtering
    hmap->setTexFilter(d3d::FilterMode::Linear);

    d3d::set_render_target(GPUgroundPhysDetailsTex.getTex2D(), 0);
    renderGPUHeightAndDisplacement.render();
    d3d::resource_barrier({GPUgroundPhysDetailsTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    GPUgroundPhysDetailsTex.setVar();

    ringTextures.startCPUCopy();

    Point4 gpuworldToDisplacement =
      Point4(1.0f / worldSize, 1.0f / worldSize, -origin.x + 0.5f * worldSize, -origin.y + 0.5f * worldSize);

    static int world_to_groundPhysDetailsVarId = get_shader_variable_id("world_to_groundPhysDetails", true);
    ShaderGlobal::set_color4(world_to_groundPhysDetailsVarId, gpuworldToDisplacement.x, gpuworldToDisplacement.y,
      gpuworldToDisplacement.z, gpuworldToDisplacement.w);

    forceUpdateCounter = 0;
  }
}

void GroundDisplacementCPU::setMaxPhysicsHeight(const float &height, const float &radius)
{
  static int displacement_max_phys_heightVarId = get_shader_variable_id("displacement_max_phys_height", true);
  ShaderGlobal::set_real(displacement_max_phys_heightVarId, height);

  static int wheel_static_radiusVarId = get_shader_variable_id("wheel_static_radius", true);
  ShaderGlobal::set_real(wheel_static_radiusVarId, radius);
}

bool GroundDisplacementCPU::isDisplaced(const Point3 &world_pos) const
{
  if (Point2(Point2::xz(world_pos) - curOrigin).lengthSq() > worldSize * worldSize * 0.5f)
    return false;
  else
    return true;
}

float GroundDisplacementCPU::interpolateOffset(float new_offset, float current_offset, float delta_time) const
{
  return current_offset + clamp((new_offset - current_offset), maxSpeedDown * delta_time, maxSpeedUp * delta_time);
}

float GroundDisplacementCPU::getDisplacementHeight(const Point3 &world_pos)
{
  Point2 texCoord = Point2((world_pos.x + worldToDisplacement.z) * worldToDisplacement.x,
    (world_pos.z + worldToDisplacement.w) * worldToDisplacement.y);

  if ((texCoord.x >= 1) || (texCoord.x < 0) || (texCoord.y >= 1) || (texCoord.y < 0))
    return -1000000;

  Point2 pixelPos = Point2(texCoord.x * bufferSize - 0.5f, texCoord.y * bufferSize - 0.5f);
  Point2 lerpWeight = Point2(1 - pixelPos.x + floorf(pixelPos.x), 1 - pixelPos.y + floorf(pixelPos.y));

  Point4 pixelWeight = Point4(lerpWeight.x * lerpWeight.y, lerpWeight.x * (1 - lerpWeight.y), (1 - lerpWeight.x) * lerpWeight.y,
    (1 - lerpWeight.x) * (1 - lerpWeight.y));

  float height = pixelWeight.x * getPixel(pixelPos.x, pixelPos.y) + pixelWeight.y * getPixel(pixelPos.x, pixelPos.y + 1) +
                 pixelWeight.z * getPixel(pixelPos.x + 1, pixelPos.y) + pixelWeight.w * getPixel(pixelPos.x + 1, pixelPos.y + 1);

  return height;
}

CONSOLE_BOOL_VAL("suspension", testRandom, false);

bool is_testing_suspension() { return testRandom; }

#define LIMIT_DISPLACEMENT_UP   0.8f // large limit in case of penetrations
#define LIMIT_DISPLACEMENT_DOWN -0.2f

float GroundDisplacementCPU::getYOffset(const Point3 &world_pos, const float &hmap_height, float tank_speed, float ground_normal_y,
  float current_offset) const
{
  if (testRandom)
    return 0.3f * sinf(world_pos.x + 3.0f * get_shader_global_time_phase(0, 0));

  Point2 texCoord = Point2((world_pos.x + worldToDisplacement.z) * worldToDisplacement.x,
    (world_pos.z + worldToDisplacement.w) * worldToDisplacement.y);

  if ((texCoord.x >= 1) || (texCoord.x < 0) || (texCoord.y >= 1) || (texCoord.y < 0))
    return interpolateOffset(0, current_offset, deltaTime);

  Point2 pixelPos = Point2(texCoord.x * bufferSize - 0.5f, texCoord.y * bufferSize - 0.5f);
  Point2 lerpWeight = Point2(1 - pixelPos.x + floorf(pixelPos.x), 1 - pixelPos.y + floorf(pixelPos.y));

  Point4 pixelWeight = Point4(lerpWeight.x * lerpWeight.y, lerpWeight.x * (1 - lerpWeight.y), (1 - lerpWeight.x) * lerpWeight.y,
    (1 - lerpWeight.x) * (1 - lerpWeight.y));

  float height = pixelWeight.x * getPixel(pixelPos.x, pixelPos.y) + pixelWeight.y * getPixel(pixelPos.x, pixelPos.y + 1) +
                 pixelWeight.z * getPixel(pixelPos.x + 1, pixelPos.y) + pixelWeight.w * getPixel(pixelPos.x + 1, pixelPos.y + 1);

  float delta = height - world_pos.y;

  // if speed is high, wheel is more sensitive to ground details (random shaking compenstation)
  float intolerance = max(20.0f - 3 * tank_speed, 5.0f);
  float weight = max(1.0f - intolerance * max(world_pos.y - hmap_height, 0.0f), 0.0f);
  // for inclined surfaces we make tank less sensitive to negative displacement
  float inclinedScale = (ground_normal_y - 0.75f) * 5.0f;
  weight *= saturate(inclinedScale);

  // if distance between bottom point of wheel and heightmap at this point is large
  // we use only positive offset
  const float mindelta = -weight * 0.1f;
  const float maxdelta = 0.2f;

  if ((delta > LIMIT_DISPLACEMENT_UP) || (delta < LIMIT_DISPLACEMENT_DOWN))
    return interpolateOffset(0, current_offset, deltaTime);

  return interpolateOffset(ground_normal_y * clamp(delta, mindelta, maxdelta), current_offset, deltaTime);
}

/////////////////////////////////
// Physmat block
/////////////////////////////////

LandmeshPhysmatsCPU::LandmeshPhysmatsCPU() {}

LandmeshPhysmatsCPU::~LandmeshPhysmatsCPU()
{
  ringTextures.close();
  clear_and_shrink(loadedPhysmats);
}

void LandmeshPhysmatsCPU::init(int tex_size, float texel_size, float invalidation_scale)
{
  bufferSize = tex_size;
  texelSize = texel_size;
  worldSize = bufferSize * texelSize;
  invalidationScale = invalidation_scale;
  origin = Point2(0, 0);
  curOrigin = origin;

  worldToPhysmats = Point4(0, 0, 0, 0);

  ringTextures.init(bufferSize, bufferSize, 1, "CPU_physmats",
    TEXFMT_L8 | TEXCF_RTARGET | TEXCF_LINEAR_LAYOUT | TEXCF_CPU_CACHED_MEMORY);

  loadedPhysmats.resize(bufferSize * bufferSize);
  mem_set_0(loadedPhysmats);
  forceUpdateCounter = 0;
}


bool LandmeshPhysmatsCPU::process(uint8_t *destData, uint32_t &frame)
{
  int stride;
  uint8_t *data = ringTextures.lock(stride, frame);
  if (!data)
    return false;

  G_UNUSED(destData);
  for (int y = 0; y < bufferSize; ++y, destData += bufferSize, data += stride)
    memcpy(destData, data, bufferSize * sizeof(uint8_t));
  ringTextures.unlock();
  return true;
}

void LandmeshPhysmatsCPU::invalidate()
{
  origin = Point2(10000000, 1000000);
  curOrigin = origin;
  forceUpdateCounter = 0;
}

void LandmeshPhysmatsCPU::update(const Point3 &center_pos, IRenderLandmeshPhysdetailsCB *render_physmats_cb)
{
  forceUpdateCounter++;

  uint32_t frame2;
  if (process(loadedPhysmats.data(), frame2)) // written data
  {
    curOrigin = origin;

    worldToPhysmats = Point4(1.0f / worldSize, 1.0f / worldSize, -curOrigin.x + 0.5f * worldSize, -curOrigin.y + 0.5f * worldSize);
  }

  if (Point2(Point2(center_pos.x, center_pos.z) - curOrigin).length() < worldSize * invalidationScale && forceUpdateCounter < 1000)
    return;

  int frame;
  Texture *currentTarget = ringTextures.getNewTarget(frame);
  if (!currentTarget)
    return;
  {
    origin = Point2(texelSize * floorf(center_pos.x / texelSize), texelSize * floorf(center_pos.z / texelSize));
    SCOPE_RENDER_TARGET;

    d3d::set_render_target(currentTarget, 0);

    BBox2 region(Point2(origin.x - 0.5f * worldSize, origin.y - 0.5f * worldSize),
      Point2(origin.x + 0.5f * worldSize, origin.y + 0.5f * worldSize));

    render_physmats_cb->renderLandmeshPhysdetails(Point3(origin.x, center_pos.y, origin.y), region);

    ringTextures.startCPUCopy();

    forceUpdateCounter = 0;
  }
}

int LandmeshPhysmatsCPU::getPhysmat(const Point3 &world_pos) const
{
  Point2 texCoord =
    Point2((world_pos.x + worldToPhysmats.z) * worldToPhysmats.x, (world_pos.z + worldToPhysmats.w) * worldToPhysmats.y);

  if ((texCoord.x >= 1) || (texCoord.x < 0) || (texCoord.y >= 1) || (texCoord.y < 0))
    return -1;

  Point2 pixelPos = Point2(texCoord.x * bufferSize, texCoord.y * bufferSize);

  uint8_t matId = getPixel(pixelPos.x, pixelPos.y);
  return matId == 0xff ? -1 : matId;
}
