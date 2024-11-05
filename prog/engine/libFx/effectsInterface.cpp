// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <startup/dag_restart.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <3d/dag_ringDynBuf.h>
#include <generic/dag_tab.h>
#include <generic/dag_qsort.h>
#include <util/dag_simpleString.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_debug.h>
#include <shaders/dag_dynShaderBuf.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_shaderMesh.h>
#include <fx/dag_fxInterface.h>
#include <math/dag_curveParams.h>

void *(*CubicCurveSampler::mem_allocator)(size_t) = NULL;
void (*CubicCurveSampler::mem_free)(void *) = NULL;

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


#define MAX_PARTS 1000

#define NUM_TEXTURE_SLOTS 4


class NamedDynamicShaderHelper : public DynamicShaderHelper
{
public:
  SimpleString name;

  void init(const char *shader_name, CompiledShaderChannelId *channels, int num_channels, const char *module_info,
    bool optional = false)
  {
    name = shader_name;
    DynamicShaderHelper::init(shader_name, channels, num_channels, module_info, optional);
  }
};


static Tab<NamedDynamicShaderHelper> fx_shaders(inimem_ptr());

// compatibility: new premultalpha shaders are optional
static const char *shaderNames[FX__NUM_STD_SHADERS * 2] = {
  "ablend_particles",
  "additive_trails",
  "addsmooth_particles",
  "atest_particles",
  "premultalpha_particles",
  "gbuffer_atest_particles",
  "atest_particles_refraction",
  "gbuffer_patch",

  "ablend_2d",
  "additive_2d",
  "addsmooth_2d",
  "atest_2d",
  "premultalpha_2d",
  "atest_2d",
  "atest_2d",
  "ablend_2d",
};


static int textureVarId[NUM_TEXTURE_SLOTS];
static int effects_tex_c_no = -1;
static int effects_nm_tex_c_no = -1;

static DynShaderMeshBuf *stdPartsMeshBuffer = NULL;
static DynShaderQuadBuf *stdPartsBuffer = NULL;

static int currentShaderId = 0;

static int lighting_power_vid = VariableMap::BAD_ID;
static int sun_power_vid = VariableMap::BAD_ID, amb_power_vid = VariableMap::BAD_ID;

static TEXTUREID currentTextureId[NUM_TEXTURE_SLOTS] = {
  BAD_TEXTUREID,
  BAD_TEXTUREID,
  BAD_TEXTUREID,
  BAD_TEXTUREID,
};


static CompiledShaderChannelId stdPartsChannels[4] = {{SCTYPE_FLOAT3, SCUSAGE_POS, 0, 0}, {SCTYPE_E3DCOLOR, SCUSAGE_VCOL, 0, 0},
  {SCTYPE_FLOAT3, SCUSAGE_TC, 0, 0}, {SCTYPE_FLOAT3, SCUSAGE_TC, 1, 0}};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static real colorScale = 1;


real EffectsInterface::getColorScale() { return ::colorScale; }


void EffectsInterface::setColorScale(real s) { colorScale = s; }


void EffectsInterface::setD3dWtm(const TMatrix &tm) { d3d::settm(TM_WORLD, (TMatrix &)tm); }


int EffectsInterface::registerStdParticleCustomShader(const char *name, bool optional /*= false*/)
{
  int id;
  for (id = 0; id < ::fx_shaders.size(); ++id)
    if (strcmp(::fx_shaders[id].name, name) == 0)
    {
      if (!::fx_shaders[id].material)
        ::fx_shaders[id].init(name, stdPartsChannels, countof(stdPartsChannels), "FxInterface", optional);

      return id;
    }

  id = append_items(::fx_shaders, 1);

  ::fx_shaders[id].init(name, stdPartsChannels, countof(stdPartsChannels), "FxInterface", optional);

  return id;
}


void EffectsInterface::setStdParticleCustomShader(int id)
{
  if (currentShaderId == id)
    return;

  currentShaderId = id;
  G_ASSERT(currentShaderId >= 0 && currentShaderId < ::fx_shaders.size());
  if (stdPartsBuffer)
    ::stdPartsBuffer->setCurrentShader(::fx_shaders[currentShaderId].shader);
  if (stdPartsMeshBuffer)
    ::stdPartsMeshBuffer->setCurrentShader(::fx_shaders[currentShaderId].shader);

  for (int i = 0; i < NUM_TEXTURE_SLOTS; ++i)
    currentTextureId[i] = BAD_TEXTUREID;
}


void EffectsInterface::setStdParticleShader(int id)
{
  if (id < 0 || id >= FX__NUM_STD_SHADERS)
    return;

  setStdParticleCustomShader(id);
}


void EffectsInterface::setStd2dParticleShader(int id)
{
  if (id < 0 || id >= FX__NUM_STD_SHADERS)
    return;

  setStdParticleCustomShader(id + FX__NUM_STD_SHADERS);
}


void EffectsInterface::setStdParticleTexture(TEXTUREID tex_id, int slot)
{
  if (slot < 0 || slot >= NUM_TEXTURE_SLOTS)
    return;

  if (tex_id == currentTextureId[slot])
    return;

  G_ASSERT(currentShaderId >= 0 && currentShaderId < ::fx_shaders.size());
  currentTextureId[slot] = tex_id;

  if (stdPartsBuffer)
    stdPartsBuffer->flush();
  if (stdPartsMeshBuffer)
    stdPartsMeshBuffer->flush();
  if (tex_id != BAD_TEXTUREID && effects_tex_c_no >= 0)
  {
    // fixme: not optimal
    mark_managed_tex_lfu(tex_id);
    d3d::settex(effects_tex_c_no, D3dResManagerData::getBaseTex(tex_id));
  }
  else
    ::fx_shaders[currentShaderId].material->set_texture_param(textureVarId[slot], tex_id);
}

void EffectsInterface::setStdParticleNormalMap(TEXTUREID tex_id, int shading_type)
{
  if (effects_nm_tex_c_no >= 0)
  {
    mark_managed_tex_lfu(tex_id);
    d3d::settex(effects_nm_tex_c_no, D3dResManagerData::getBaseTex(tex_id));
  }
  static int useEffectsNormalMapVarId = get_shader_variable_id("use_effects_normal_map", true);
  ShaderGlobal::set_int_fast(useEffectsNormalMapVarId, tex_id == BAD_TEXTUREID ? 0 : 1);

  static int effectsShadingTypeVarId = get_shader_variable_id("effects_shading_type", true);
  ShaderGlobal::set_int_fast(effectsShadingTypeVarId, shading_type);
}

void EffectsInterface::setLightingParams(real lighting_power, real sun_power, real amb_power)
{
  if (VariableMap::isVariablePresent(lighting_power_vid))
    ::fx_shaders[currentShaderId].material->set_real_param(lighting_power_vid, lighting_power);
  if (VariableMap::isVariablePresent(sun_power_vid))
    ::fx_shaders[currentShaderId].material->set_real_param(sun_power_vid, sun_power);
  if (VariableMap::isVariablePresent(amb_power_vid))
    ::fx_shaders[currentShaderId].material->set_real_param(amb_power_vid, amb_power);
}

class StandardFxParticleCompare
{
public:
  static StandardFxParticle *array;

  static int compare(int a, int b)
  {
    real ao = array[a].sortOrder;
    real bo = array[b].sortOrder;

    if (ao > bo)
      return -1;
    return +1;
  }
};

StandardFxParticle *StandardFxParticleCompare::array = NULL;

#if _TARGET_PC && !_TARGET_STATIC_LIB
bool particles_debug_render = false;

void renderParticlesDebug(StandardFxParticle *parts, int count, int *indices)
{
  if (!particles_debug_render)
    return;

  for (int pi = 0; pi < count; ++pi)
  {
    StandardFxParticle &p = parts[indices ? indices[pi] : pi];
    E3DCOLOR color = E3DCOLOR(128, 0, 0);
    ::draw_cached_debug_line(p.pos - p.uvec, p.pos + p.uvec, color);
    ::draw_cached_debug_line(p.pos - p.vvec, p.pos + p.vvec, color);
  }
}
#endif


void EffectsInterface::renderStdFlatParticles(StandardFxParticle *parts, int count, int *indices)
{
  if (count <= 0)
    return;

  if (!stdPartsBuffer)
    return;

  d3d::settm(TM_WORLD, TMatrix::IDENT);

  StdParticleVertex *verts = NULL;
  bool visibilityChecked = false;

  if (indices)
  {
    // extract only visible paricles for sorting and rendering
    int visibleCount = 0;
    for (int i = 0; i < count; ++i)
    {
      if (parts[indices[i]].isVisible())
      {
        // swap elements to prevent duplicating array items (needed for example by FlowPs)
        int t = indices[visibleCount];
        indices[visibleCount] = indices[i];
        indices[i] = t;
        visibleCount++;
      }
    };
    count = visibleCount;
    visibilityChecked = true;

    StandardFxParticleCompare::array = parts;
    SimpleQsort<int, StandardFxParticleCompare> qsort;
    qsort.sort(indices, count);
  }

  for (int pi = 0; pi < count; ++pi)
  {
    StandardFxParticle &p = parts[indices ? indices[pi] : pi];

    if (!visibilityChecked && !p.isVisible())
      continue;

    ::stdPartsBuffer->fillRawQuads((void **)&verts, 1);

    verts[0].tc.set_xyV(p.tco, p.specularFade);
    verts[1].tc.set_xyV(p.tco + p.tcu, p.specularFade);
    verts[2].tc.set_xyV(p.tco + p.tcu + p.tcv, p.specularFade);
    verts[3].tc.set_xyV(p.tco + p.tcv, p.specularFade);

    verts[0].next_tc_blend.set_xyV(p.next_tco, p.frameBlend);
    verts[1].next_tc_blend.set_xyV(p.next_tco + p.tcu, p.frameBlend);
    verts[2].next_tc_blend.set_xyV(p.next_tco + p.tcu + p.tcv, p.frameBlend);
    verts[3].next_tc_blend.set_xyV(p.next_tco + p.tcv, p.frameBlend);

    verts[0].pos = p.pos + (-p.vvec - p.uvec);
    verts[0].color = p.color;

    verts[1].pos = p.pos + (-p.vvec + p.uvec);
    verts[1].color = p.color;

    verts[2].pos = p.pos + (+p.vvec + p.uvec);
    verts[2].color = p.color;

    verts[3].pos = p.pos + (+p.vvec - p.uvec);
    verts[3].color = p.color;
  }

  ::stdPartsBuffer->flush();

#if _TARGET_PC && !_TARGET_STATIC_LIB
  renderParticlesDebug(parts, count, indices);
#endif
}

void EffectsInterface::renderBatchFlatParticles(StandardFxParticle *parts, int count, int *indices)
{
  if (count <= 0)
    return;

  if (!stdPartsBuffer)
    return;

  d3d::settm(TM_WORLD, TMatrix::IDENT);

  StdParticleVertex *verts = NULL;
  bool visibilityChecked = false;

  if (indices)
  {
    // extract only visible paricles for sorting and rendering
    int visibleCount = 0;
    for (int i = 0; i < count; ++i)
    {
      if (parts[indices[i]].isVisible())
      {
        // swap elements to prevent duplicating array items (needed for example by FlowPs)
        int t = indices[visibleCount];
        indices[visibleCount] = indices[i];
        indices[i] = t;
        visibleCount++;
      }
    };
    count = visibleCount;
    visibilityChecked = true;

    StandardFxParticleCompare::array = parts;
    SimpleQsort<int, StandardFxParticleCompare> qsort;
    qsort.sort(indices, count);
  }

  for (int pi = 0; pi < count; ++pi)
  {
    StandardFxParticle &p = parts[indices ? indices[pi] : pi];

    if (!visibilityChecked && !p.isVisible())
      continue;

    ::stdPartsBuffer->fillRawQuads((void **)&verts, 1);

    verts[0].tc.set_xyV(p.tco, p.specularFade);
    verts[1].tc.set_xyV(p.tco + p.tcu, p.specularFade);
    verts[2].tc.set_xyV(p.tco + p.tcu + p.tcv, p.specularFade);
    verts[3].tc.set_xyV(p.tco + p.tcv, p.specularFade);

    verts[0].next_tc_blend.set_xyV(p.next_tco, p.frameBlend);
    verts[1].next_tc_blend.set_xyV(p.next_tco + p.tcu, p.frameBlend);
    verts[2].next_tc_blend.set_xyV(p.next_tco + p.tcu + p.tcv, p.frameBlend);
    verts[3].next_tc_blend.set_xyV(p.next_tco + p.tcv, p.frameBlend);

    verts[0].pos = p.pos + (-p.vvec - p.uvec);
    verts[0].color = p.color;

    verts[1].pos = p.pos + (-p.vvec + p.uvec);
    verts[1].color = p.color;

    verts[2].pos = p.pos + (+p.vvec + p.uvec);
    verts[2].color = p.color;

    verts[3].pos = p.pos + (+p.vvec - p.uvec);
    verts[3].color = p.color;
  }

#if _TARGET_PC && !_TARGET_STATIC_LIB
  renderParticlesDebug(parts, count, indices);
#endif
}

static bool isRenderStarted = false;


void EffectsInterface::startStdRender()
{
  G_ASSERT(!isRenderStarted);
  isRenderStarted = true;

  if (!stdPartsBuffer)
    return;

  d3d::settm(TM_WORLD, TMatrix::IDENT);
}


void EffectsInterface::endStdRender()
{
  G_ASSERT(isRenderStarted);
  isRenderStarted = false;

  if (!stdPartsBuffer)
    return;

  ::stdPartsBuffer->flush();
  ::stdPartsMeshBuffer->flush();
}


DynShaderMeshBuf *EffectsInterface::getStdRenderBuffer() { return stdPartsMeshBuffer; }


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static RingDynamicVB fxMeshVB;
static RingDynamicIB fxMeshIB;

static int maxQuadDynBufSz = 4096;
static int maxFaceDynBufSz = 1024;


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


int EffectsInterface::getMaxQuadCount()
{
  return MAX_PARTS; // same as in stdPartsBuffer->init()
}


void EffectsInterface::beginQuads(StdParticleVertex **ptr, int num)
{
  G_ASSERT(ptr);
  G_ASSERT(num > 0);
  G_ASSERT(::stdPartsBuffer);

  G_VERIFY(::stdPartsBuffer->fillRawQuads((void **)ptr, num));
}


void EffectsInterface::endQuads() { ::stdPartsBuffer->flush(); }


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void EffectsInterface::initDynamicBuffers(int max_quad, int max_faces)
{
  maxQuadDynBufSz = max_quad;
  maxFaceDynBufSz = max_faces;
}

void EffectsInterface::startup()
{
  currentShaderId = 0;

  int max_vcount = maxFaceDynBufSz * 2;
  max_vcount += maxQuadDynBufSz * 4;
  fxMeshVB.init(max_vcount, dynrender::getStride(stdPartsChannels, countof(stdPartsChannels)), "fx_mesh_vb");
  if (maxFaceDynBufSz > 0)
    fxMeshIB.init(maxFaceDynBufSz * 3, "fx_mesh_ib");
  fxMeshVB.addRef();
  fxMeshIB.addRef();

  for (int id = 0; id < ::fx_shaders.size(); ++id)
    if (!::fx_shaders[id].material)
      ::fx_shaders[id].DynamicShaderHelper::init(::fx_shaders[id].name, stdPartsChannels, countof(stdPartsChannels), "FxInterface");

  for (int i = 0; i < NUM_TEXTURE_SLOTS; ++i)
    currentTextureId[i] = BAD_TEXTUREID;

  ::stdPartsBuffer = new DynShaderQuadBuf;
  ::stdPartsMeshBuffer = new DynShaderMeshBuf;
  ::stdPartsBuffer->setRingBuf(&fxMeshVB);
  ::stdPartsMeshBuffer->setRingBuf(&fxMeshVB, &fxMeshIB);
  ::stdPartsBuffer->init(dag::Span<CompiledShaderChannelId>(stdPartsChannels, countof(stdPartsChannels)), MAX_PARTS);

  int max_faces = maxFaceDynBufSz < MAX_PARTS * 2 ? maxFaceDynBufSz : MAX_PARTS * 2;
  ::stdPartsMeshBuffer->init(dag::Span<CompiledShaderChannelId>(stdPartsChannels, countof(stdPartsChannels)), max_faces * 2,
    max_faces);

  ::textureVarId[0] = get_shader_variable_id("tex");
  int effects_tex_c_no_varId = get_shader_variable_id("effects_tex_c_no", true);
  if (VariableMap::isGlobVariablePresent(effects_tex_c_no_varId))
    ::effects_tex_c_no = ShaderGlobal::get_int_fast(effects_tex_c_no_varId);
  else
    ::effects_tex_c_no = -1;

  for (int i = 1; i < NUM_TEXTURE_SLOTS; ++i)
  {
    char name[32];
    snprintf(name, sizeof(name), "tex%d", i);
    ::textureVarId[i] = get_shader_variable_id(name, true);
  }

  if (::textureVarId[1] == -1)
    ::textureVarId[1] = get_shader_variable_id("tex_nm", true);
  int effects_nm_tex_c_no_varId = get_shader_variable_id("effects_nm_tex_c_no", true);
  if (VariableMap::isGlobVariablePresent(effects_nm_tex_c_no_varId))
    ::effects_nm_tex_c_no = ShaderGlobal::get_int_fast(effects_nm_tex_c_no_varId);
  else
    ::effects_nm_tex_c_no = -1;


  for (int i = 0; i < countof(shaderNames); ++i)
  {
    registerStdParticleCustomShader(shaderNames[i], true);
  }

  lighting_power_vid = get_shader_variable_id("lighting_power", true);
  sun_power_vid = get_shader_variable_id("sun_light_power", true);
  amb_power_vid = get_shader_variable_id("ambient_light_power", true);

  ::stdPartsBuffer->setCurrentShader(::fx_shaders[currentShaderId].shader);
  ::stdPartsMeshBuffer->setCurrentShader(::fx_shaders[currentShaderId].shader);
}


void EffectsInterface::shutdown()
{
  del_it(::stdPartsBuffer);
  del_it(::stdPartsMeshBuffer);

  for (int i = 0; i < ::fx_shaders.size(); ++i)
    ::fx_shaders[i].close();
  clear_and_shrink(fx_shaders);

  currentShaderId = 0;
  fxMeshVB.close();
  fxMeshIB.close();
}

int EffectsInterface::resetPerFrameDynamicBufferPos()
{
  int q1 = fxMeshVB.getProcessedCount();
  int q2 = fxMeshIB.getProcessedCount();
  if (q1 > fxMeshVB.bufSize())
    debug("insufficient fxMeshVB size: %d used, %d available", q1, fxMeshVB.bufSize());
  if (q2 > fxMeshIB.bufSize())
    debug("insufficient fxMeshIB size: %d used, %d available", q2, fxMeshIB.bufSize());
  fxMeshVB.resetCounters();
  fxMeshIB.resetCounters();
  q1 /= 4;
  q2 /= 6;
  return q1 > q2 ? q1 : q2;
}
