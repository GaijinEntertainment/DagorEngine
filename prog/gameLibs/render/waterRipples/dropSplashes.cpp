// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_materialData.h>
#include <3d/dag_render.h>
#include <gameRes/dag_gameResources.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaders.h>

#include <render/dropSplashes.h>

static float splash_vertices[] = {
  0,
  0,
  0,
  // Tall part
  0.5f * cosf(0 * 2 * PI / 4),
  1,
  0.5f * sinf(0 * 2 * PI / 4),
  0.5f * cosf(1 * 2 * PI / 4),
  1,
  0.5f * sinf(1 * 2 * PI / 4),
  0.5f * cosf(2 * 2 * PI / 4),
  1,
  0.5f * sinf(2 * 2 * PI / 4),
  0.5f * cosf(3 * 2 * PI / 4),
  1,
  0.5f * sinf(3 * 2 * PI / 4),

  // Wide part
  -cosf(0 * 2 * PI / 6),
  0.4f,
  sinf(0 * 2 * PI / 6),
  -cosf(1 * 2 * PI / 6),
  0.4f,
  sinf(1 * 2 * PI / 6),
  -cosf(2 * 2 * PI / 6),
  0.4f,
  sinf(2 * 2 * PI / 6),
  -cosf(3 * 2 * PI / 6),
  0.4f,
  sinf(3 * 2 * PI / 6),
  -cosf(4 * 2 * PI / 6),
  0.4f,
  sinf(4 * 2 * PI / 6),
  -cosf(5 * 2 * PI / 6),
  0.4f,
  sinf(5 * 2 * PI / 6),
};

const static uint16_t splash_indices[] = {
  // Tall part
  0,
  1,
  2,
  0,
  2,
  3,
  0,
  3,
  4,
  0,
  4,
  1,

  // Wide part
  0,
  5,
  6,
  0,
  6,
  7,
  0,
  7,
  8,
  0,
  8,
  9,
  0,
  9,
  10,
  0,
  10,
  5,
};

const uint32_t VERTICES_PER_SPLASH = countof(splash_vertices) / 3;
const uint32_t INDICES_PER_SPLASH = countof(splash_indices);
const uint32_t PRIMITIVES_PER_SPLASH = INDICES_PER_SPLASH / 3;
const uint32_t VERTICES_PER_SPRITE = 4;
const uint32_t PRIMITIVES_PER_SPRITE = 2;


TEXTUREID DropSplashes::initSplashTexture(const DataBlock &blk, const char *tex_name, const char *res_default_name)
{
  const char *splashResourceName = blk.getStr(tex_name, res_default_name);
  const TEXTUREID splashTexId = ::get_tex_gameres(splashResourceName);
  G_ASSERTF(splashTexId != BAD_TEXTUREID, "texture '%s' not found.", splashResourceName);
  ShaderGlobal::set_texture(get_shader_variable_id(tex_name), splashTexId);
  return splashTexId;
}

void DropSplashes::initSplashTextures(const DataBlock &blk)
{
  volumetricSplashTextureId = initSplashTexture(blk, "volumetric_splash_tex", "water_surface_foam_tex");
  spriteSplashTextureId = initSplashTexture(blk, "sprite_splash_tex", "droplet_splash");
}

using RenderPair = eastl::pair<eastl::unique_ptr<dynrender::RElem>, eastl::unique_ptr<ShaderMaterial>>;


static RenderPair init_shader(const char *shader_name, const uint32_t count_of_channels, const uint32_t vertices_count,
  const uint32_t primitives_count)
{
  eastl::unique_ptr<MaterialData> matData = eastl::make_unique<MaterialData>();
  matData->className = shader_name;
  eastl::unique_ptr<ShaderMaterial> material;
  material.reset(new_shader_material(*matData));
  G_ASSERT(material);

  static CompiledShaderChannelId chan[] = {{SCTYPE_FLOAT3, SCUSAGE_POS, 0, 0}};
  if (!material->checkChannels(chan, count_of_channels))
    DAG_FATAL("invalid channels for this material!");

  eastl::unique_ptr<dynrender::RElem> rendElem = eastl::make_unique<dynrender::RElem>();
  rendElem->vDecl = dynrender::addShaderVdecl(chan, count_of_channels);
  rendElem->stride = dynrender::getStride(chan, count_of_channels);
  G_ASSERT(rendElem->vDecl != BAD_VDECL);
  rendElem->minVert = 0;
  rendElem->numVert = vertices_count;
  rendElem->startIndex = 0;
  rendElem->numPrim = primitives_count;
  rendElem->shElem = material->make_elem();
  return eastl::make_pair(eastl::move(rendElem), eastl::move(material));
}


void DropSplashes::initSplashShader()
{
  RenderPair pair = init_shader("drop_splashes", 1, VERTICES_PER_SPLASH, PRIMITIVES_PER_SPLASH);
  splashRendElem = eastl::move(pair.first);
  splashMaterial = eastl::move(pair.second);
}


void DropSplashes::initSpriteShader()
{
  RenderPair pair = init_shader("splash_sprite", 0, VERTICES_PER_SPRITE, PRIMITIVES_PER_SPRITE);
  spriteRendElem = eastl::move(pair.first);
  spriteMaterial = eastl::move(pair.second);
}

void DropSplashes::fillBuffers()
{
  splashVb->updateData(0, VERTICES_PER_SPLASH * sizeof(float) * 3, splash_vertices, 0);
  splashIb->updateData(0, INDICES_PER_SPLASH * sizeof(uint16_t), splash_indices, 0);
}

DropSplashes::DropSplashes(const DataBlock &blk) : splashRendElem(nullptr), spriteRendElem(nullptr), currentTime(0), splashesCount(0)
{
  initSplashShader();
  initSpriteShader();

  splashVb = dag::create_vb(VERTICES_PER_SPLASH * sizeof(float) * 3, 0, "splashVb");
  G_ASSERT(splashVb);

  splashIb = dag::create_ib(INDICES_PER_SPLASH * sizeof(uint16_t), 0, "splashIb");
  G_ASSERT(splashIb);
  fillBuffers();

  setSplashesCount(blk.getInt("splashesCount", 10000));
  setDistance(blk.getReal("distance", 50.f));
  setTimeToLive(blk.getReal("timeToLive", 0.3f));
  setIterationTime(blk.getReal("iterationTime", 2.f));
  setVolumetricSplashScale(blk.getReal("volumetricSplashScale", 1.f));
  setSpriteSplashScale(blk.getReal("spriteSplashScale", 7.5f));
  setPartOfSprites(blk.getReal("partOfSprites", 0.2f));
  setSpriteYPos(blk.getReal("spriteYPos", 0.2f));

  initSplashTextures(blk);
}

void DropSplashes::setTimeToLive(const float time_to_live)
{
  splashTimeToLive = time_to_live;
  ShaderGlobal::set_real_fast(::get_shader_variable_id("splash_time_to_live"), splashTimeToLive);
}

void DropSplashes::setIterationTime(const float iteration_time)
{
  iterationTime = iteration_time;
  ShaderGlobal::set_real_fast(::get_shader_variable_id("splash_iteration_time"), iterationTime);
}

void DropSplashes::setDistance(const float dist)
{
  distance = dist;
  ShaderGlobal::set_real_fast(::get_shader_variable_id("splashes_distance"), distance);
}

void DropSplashes::update(const float dt, const Point3 &view_pos)
{
  static int hero_position_currentVarId = ::get_shader_variable_id("camera_position_current");
  static int hero_position_oldVarId = ::get_shader_variable_id("camera_position_old");
  static int splash_current_timeVarId = ::get_shader_variable_id("splash_current_time");

  currentTime += dt;
  if (currentTime > iterationTime)
  {
    currentTime = currentTime - floor(currentTime / iterationTime) * iterationTime;
    ShaderGlobal::set_color4(hero_position_oldVarId, ShaderGlobal::get_color4_fast(hero_position_currentVarId));
    ShaderGlobal::set_color4(hero_position_currentVarId, Color4(view_pos.x, view_pos.z, 0, 0));
  }
  ShaderGlobal::set_real_fast(splash_current_timeVarId, currentTime);
}

void DropSplashes::render()
{
  if (!splashesCount)
    return;

  ShaderGlobal::set_texture(get_shader_variable_id("volumetric_splash_tex"), volumetricSplashTextureId);
  ShaderGlobal::set_texture(get_shader_variable_id("sprite_splash_tex"), spriteSplashTextureId);

  static int splash_scaleVarId = ::get_shader_variable_id("splash_scale");

  {
    TIME_D3D_PROFILE(render_volumetric_drop_splashes);
    ShaderGlobal::set_real_fast(splash_scaleVarId, volumetricSplashScale);

    splashRendElem->shElem->setStates(0, true);
    d3d::setvdecl(splashRendElem->vDecl);

    d3d::setvsrc(0, splashVb.get(), splashRendElem->stride);
    d3d::setind(splashIb.get());

    d3d::drawind_instanced(PRIM_TRILIST, 0, PRIMITIVES_PER_SPLASH, 0, splashesCount);
  }

  {
    TIME_D3D_PROFILE(render_sprite_drop_splashes);
    const uint32_t spritesCount = getSpritesCount();
    if (spritesCount > 0)
    {
      ShaderGlobal::set_real_fast(splash_scaleVarId, spriteSplashScale);
      spriteRendElem->shElem->setStates(0, true);
      d3d::setvdecl(spriteRendElem->vDecl);

      d3d::draw_instanced(PRIM_TRISTRIP, 0, 2, spritesCount);
    }
  }
}

void DropSplashes::setPartOfSprites(const float part)
{
  G_ASSERT(part >= 0 && part <= 1);
  partOfSprites = part;
}

uint32_t DropSplashes::getSpritesCount() const { return splashesCount * partOfSprites; }

void DropSplashes::setSpriteYPos(const float pos) { ShaderGlobal::set_real_fast(::get_shader_variable_id("sprite_y_pos"), pos); }

float DropSplashes::getSpriteYPos() const { return ShaderGlobal::get_real_fast(::get_shader_variable_id("sprite_y_pos")); }

DropSplashes::~DropSplashes()
{
  release_managed_tex(volumetricSplashTextureId);
  release_managed_tex(spriteSplashTextureId);
}