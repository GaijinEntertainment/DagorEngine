// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_tabWithLock.h>
#include <render/tireTracks.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_stlqsort.h>
#include <3d/dag_quadIndexBuffer.h>
#include <3d/dag_sbufferIDHolder.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <generic/dag_carray.h>
#include <generic/dag_range.h>
#include <generic/dag_tab.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_buffers.h>
#include <3d/dag_render.h>
#include <3d/dag_materialData.h>
#include <math/dag_bounds2.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_mathUtils.h>
#include <shaders/dag_shaderMesh.h>
#include <gameRes/dag_gameResources.h>
#include <perfMon/dag_statDrv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <generic/dag_staticTab.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_direct.h>
#include <EASTL/deque.h>
#include <vecmath/dag_vecMath.h>

namespace tire_tracks
{
static void init_emitters();
static void close_emitters();
static void erase_emitters();
typedef Range<real> RealRange;

static bool initialized = false;
static String shaderName;

UniqueBufWithShaderVar renderDataVS;
UniqueBufWithShaderVar deformPressureBuf;

static Tab<BBox3> updated_regions;

static uint32_t encode_float2(Point2 scaled_side_vec)
{
  return (float_to_half(scaled_side_vec.x) << 16) | float_to_half(scaled_side_vec.y);
}

static Point2 decode_float2(uint32_t scaled_side_vec)
{
  return Point2(half_to_float(scaled_side_vec >> 16), half_to_float(scaled_side_vec));
}

struct TireTrackNode
{
  Point3 pos;
  uint32_t scaledSideVector; // side vector * width
  uint32_t material;
  uint32_t padding; // will be replaced by second material to blend
  uint32_t omnidirBlendOpacity;
  uint32_t tcWetness;

  void packData(float alpha = 0.0f, float wetness = 0.0f, float omnidirectional_tex_blend = 0.0f, float texture_coord = 0.0f,
    int current_matId = 0, int matCount = 1)
  {
    if (current_matId < 0) // invalid texcoords
    {
      alpha = 0;
      current_matId = 0;
    }

    tcWetness = encode_float2(Point2(texture_coord, wetness));
    omnidirBlendOpacity = encode_float2(Point2(omnidirectional_tex_blend, alpha));
    float materialTc = (0.5f + (real)current_matId) / (real)matCount;
    material = encode_float2(Point2(materialTc, 0));
  }

  void setOpacity(float new_opacity)
  {
    omnidirBlendOpacity = encode_float2(Point2(decode_float2(omnidirBlendOpacity).x, new_opacity));
  }

  float getOpacity() const { return decode_float2(omnidirBlendOpacity).y; }

  float getOmnidirblend() const { return decode_float2(omnidirBlendOpacity).x; }

  float getWetness() const { return decode_float2(tcWetness).y; }

  Point2 getSideVector() const { return decode_float2(scaledSideVector); }
};

static int default_texture_idx = 0;
static float transparency = 1.f;

#define MIN_MOVE_DIST_TO_UPDATE_VB         0.05f
#define VTEX_UPDATE_BOX_WIDTH_THRESHOLD_SQ (0.2 * 0.2)
#define MIN_TRACK_VERTICES                 32
#define LQ_MAX_TRACK_VERTICES_MUL          0.5f
#define MAX_ALLOWED_TRACKS                 128
#define MAX_NODES_PER_TRACK                256

static TEXTUREID driftTexId;

struct TrackType
{
  TEXTUREID normalTexId = BAD_TEXTUREID;
  TEXTUREID diffuseTexId = BAD_TEXTUREID;
  // texture for rendering
  int frameCount = 0;
  float reflectance = 0;
  float smoothness = 0;
  real frameSpacing = 0.002f;
  float textureLength = 10.f;
  float textureWidth = 0.125f;

  shaders::UniqueOverrideStateId shaderOverride;
};

Tab<TrackType> trackTypes;

Tab<int> renderCount;

//*************************************************
// constants
//*************************************************

// maximum count of tracks
static int maxTrackCount = 32 * 2;

// maximum visible distance for tracks (meters)
static real maxVisibleDist = 100.0f;

// default track width (in meters)
static real defaultTrackWidth = 0.35f;

// 1 / part of texture width
static real trackTextureWidthFactor = 3.f;

// minimal & maximal track segment length (in meters)
static RealRange segmentLength(0.5f, 1.0f);

// minimal horizontal angle (grad), when tracks must be split
static real hAngleLimit = DEG_TO_RAD * 30.0f;
static real vAngleLimit = DEG_TO_RAD * 20.0f;

static float maxOmnidirTexBlendDelta = 1.0f;

// reduce opacity on tracks' ends
static bool fadeTrackEnds = false;

static real tcYstep = 200.0f;

//*************************************************
// class TrackEmitter
//*************************************************
class TrackEmitter
{
private:
  Point3 lastNorm;
  float trackWidth;
  float totalBatchLen;
  int lastTex; // last emitted texture index
  float prevOmnidirBlend;
  int currentIdx;
  Point3 lastPos;
  Point3 secondLastPos;
  bbox3f bbox;       // full bounding box
  bbox3f updateBbox; // only last changed vertices
public:
  Tab<TireTrackNode> track;
  bool alive;
  float deformPressureMult = -1.f;

  // start node of track
  int getStart() const
  {
    int ret;
    if (track.size() == MAX_NODES_PER_TRACK)
      ret = currentIdx;
    else
      ret = 0;

    return clamp(ret, 0, max((int)track.size() - 1, 0));
  }

  TrackEmitter() :
    lastNorm(0, 1, 0),
    trackWidth(defaultTrackWidth),
    lastTex(-1),
    lastPos(-1e+10F, -1e+10F, -1e+10F),
    secondLastPos(-1e+10F, -1e+10F, -1e+10F),
    trackTypeNo(-1),
    currentIdx(0),
    totalBatchLen(0.0f),
    alive(false)
  {}

  int trackTypeNo;
  void clearTracks() { track.clear(); }

  void init(float track_width, int track_type_no)
  {
    track.clear();
    track.reserve(MAX_NODES_PER_TRACK);
    currentIdx = 0;
    lastTex = -1;
    trackWidth = track_width;

    trackTypeNo = track_type_no;
    alive = true;

    v_bbox3_init_empty(updateBbox);
  }

  // emit new track (start new or continue existing)
  bool emit(const Point3 &norm, const Point3 &pos, const Point3 &movedir, real opacity, real wetness, int tex_id,
    float additional_width, float omnidirectional_tex_blend, bool correct_previous_node, bool direction_changed)
  {
    lastNorm = normalize(norm);
    if (lastNorm.y < 0)
      lastNorm = -lastNorm;

    if ((pos - lastPos).lengthSq() < sqr(segmentLength.getMin()))
    {
      // todo: change last node position
      return false;
    }

    lastTex = tex_id;

    totalBatchLen += (pos - lastPos).length() / trackTypes[trackTypeNo].textureLength;
    totalBatchLen = fmodf(totalBatchLen, tcYstep);

    int prevIdx = currentIdx > 0 ? currentIdx - 1 : track.size() - 1;
    if (maxOmnidirTexBlendDelta < 1.0f && prevIdx >= 0 && track[prevIdx].getOpacity() > 0.f)
    {
      omnidirectional_tex_blend =
        clamp(omnidirectional_tex_blend, prevOmnidirBlend - maxOmnidirTexBlendDelta, prevOmnidirBlend + maxOmnidirTexBlendDelta);
    }
    TireTrackNode node = genNewNode(pos, normalize(movedir), opacity, wetness, additional_width, omnidirectional_tex_blend,
      correct_previous_node, tex_id);

    prevOmnidirBlend = omnidirectional_tex_blend;

    if (track.size() == MAX_NODES_PER_TRACK)
      track[currentIdx] = node;
    else
      track.push_back(node);

    if (correct_previous_node && node.getOpacity() > 0.f && track.size() > 2)
    {
      int secondPrevIdx = prevIdx > 0 ? prevIdx - 1 : track.size() - 1;

      if (direction_changed)
        track[prevIdx].setOpacity(0.f);

      Point3 prevNodePos = track[prevIdx].pos;
      Point3 secondPrevNodePos = track[secondPrevIdx].pos;

      Point2 lastSegmentDir = normalize(Point2::xz(node.pos) - Point2::xz(prevNodePos));
      Point2 prevLastSegmentDir = normalize(Point2::xz(prevNodePos) - Point2::xz(secondPrevNodePos));
      Point2 segmentBisector = lastSegmentDir + prevLastSegmentDir;

      float segmentBisectorLengthSquared = lengthSq(segmentBisector);
      if (segmentBisectorLengthSquared > 1.f) // a turn is less than 120 degrees
      {
        Point2 sideVector = decode_float2(track[prevIdx].scaledSideVector);
        // A bisector of an angle between 2 polygonal chain segments is orthogonal to the bisector of corresponding vectors:
        Point2 newSideVector =
          Point2(segmentBisector.y, -segmentBisector.x) * safeinv(sqrtf(segmentBisectorLengthSquared)) * length(sideVector);
        if (dot(newSideVector, sideVector) < 0.f)
          newSideVector *= -1.f;

        track[prevIdx].scaledSideVector = encode_float2(newSideVector);

        // We have to invalidate previous node with the corrected side vector
        Point3 newSideVector3D = Point3::x0y(newSideVector);
        Point3 pPos = lastPos + newSideVector3D;
        v_bbox3_add_pt(updateBbox, v_ldu(&pPos.x));
        pPos = lastPos - newSideVector3D;
        v_bbox3_add_pt(updateBbox, v_ldu(&pPos.x));
      }
    }

    secondLastPos = lastPos;
    lastPos = pos;
    currentIdx = (currentIdx + 1) % MAX_NODES_PER_TRACK;
    return true;
  }

  bbox3f getBbox() const { return bbox; }

  BBox3 getUpdateBbox() const
  {
    BBox3 box;
    v_stu_bbox3(box, updateBbox);
    return box;
  }

  void invalidateUpdateBbox() { v_bbox3_init_empty(updateBbox); }

  void invalidateRegion(const BBox2 &box)
  {
    for (int i = 0; i < track.size(); i++)
    {
      Point2 posXZ = Point2(track[i].pos.x, track[i].pos.z);
      if (box & posXZ)
        track[i].setOpacity(0);
    }
  }

private:
  TireTrackNode genNewNode(const Point3 &pos, const Point3 &segment_dir, float alpha, float wetness, float additional_width,
    float omnidirectional_tex_blend, bool correct_previous_node, int tex_id)
  {
    const Point3 b = normalize(lastNorm % -segment_dir);

    TireTrackNode result;

    // real width = trackWidth / 2 + (1.0 - fabsf(movedir * segment_dir)) * (segmentLength.getMin() / 2);
    real trackTextureWidth = trackWidth * trackTextureWidthFactor;
    real width = (trackTextureWidth + additional_width) / 2;

    Point3 pwidth = b * width;

    result.pos = pos;
    result.scaledSideVector = encode_float2(Point2::xz(pwidth));
    result.packData(alpha * transparency, wetness, omnidirectional_tex_blend, totalBatchLen, tex_id,
      trackTypes[trackTypeNo].frameCount);

    // generate invalidation box

    // track moved too far, likely teleport or very big lag
    if ((pos - lastPos).lengthSq() > sqr(2 * segmentLength.getMax()))
    {
      int prevIdx = currentIdx - 1;
      if (prevIdx < 0 && track.size() == MAX_NODES_PER_TRACK)
        prevIdx = track.size() - 1;

      // hide previous and current nodes from render
      if (prevIdx >= 0)
        track[prevIdx].setOpacity(0);

      result.setOpacity(0);
    }
    else
    {
      if (track.size() > 1)
      {
        // generate invalidation box from all corner points
        Point3 pPos = pos + pwidth;
        v_bbox3_add_pt(updateBbox, v_ldu(&pPos.x));
        pPos = pos - pwidth;
        v_bbox3_add_pt(updateBbox, v_ldu(&pPos.x));

        if (!correct_previous_node)
        {
          pPos = lastPos + pwidth;
          v_bbox3_add_pt(updateBbox, v_ldu(&pPos.x));
          pPos = lastPos - pwidth;
          v_bbox3_add_pt(updateBbox, v_ldu(&pPos.x));
        }

        if (fadeTrackEnds)
        {
          pPos = secondLastPos + pwidth;
          v_bbox3_add_pt(updateBbox, v_ldu(&pPos.x));
          pPos = secondLastPos - pwidth;
          v_bbox3_add_pt(updateBbox, v_ldu(&pPos.x));
        }
      }
    }

    return result;
  }

  friend void tire_tracks::render_debug(bool);
};

//*************************************************
// internal parameters
//*************************************************
// shaders
DynamicShaderHelper trackMaterial;
static DynamicShaderHelper tiresProjectiveDecalShader;

//*************************************************
// init/release
//*************************************************
static void loadRange(DataBlock *blk, const char *name, RealRange &result)
{
  if (!blk)
    return;
  DataBlock *value = blk->getBlockByName(name);

  if (value)
  {
    result.setMin(value->getReal("min", result.getMin()));
    result.setMax(value->getReal("max", result.getMax()));
  }
}

static int tires_drift_texVarId = -1, tires_diffuse_texVarId = -1, tires_normal_texVarId = -1, track_smoothness_reflectanceVarId = -1,
           tires_texture_widthVarId = -1, tires_start_instVarId = -1, tires_frame_countVarId = -1, tires_tcY_stepVarId = -1;

static uint32_t nodeCount = MAX_ALLOWED_TRACKS * (4 + MAX_NODES_PER_TRACK);

void update_samplers()
{
  if (!initialized)
    return;

  d3d::SamplerHandle tireSampler = d3d::request_sampler({.anisotropic_max = (float)::dgs_tex_anisotropy});
  ShaderGlobal::set_sampler(get_shader_variable_id("tires_drift_tex_samplerstate"), tireSampler);
  ShaderGlobal::set_sampler(get_shader_variable_id("tires_diffuse_tex_samplerstate"), tireSampler);
  ShaderGlobal::set_sampler(get_shader_variable_id("tires_normal_tex_samplerstate", true), tireSampler);
}

// init system. load settings from blk-file
void init(const char *blk_file, bool has_normalmap, bool stub_render_mode)
{
  G_ASSERT(!initialized);
  DataBlock blk;
  if (dd_file_exists(blk_file))
    initialized = blk.load(blk_file);
  if (!initialized)
    return;

  // load visual params
  DataBlock *params = blk.getBlockByName("VisualParams");
  DataBlock *override = params->getBlockByName(::get_platform_string_id());
  if (override)
    params = override;

  if (params)
  {
    DataBlock *angles = params->getBlockByName("critical_angle");
    if (angles)
    {
      hAngleLimit = params->getReal("horizontal", hAngleLimit);
      vAngleLimit = params->getReal("vertical", vAngleLimit);
    }

    defaultTrackWidth = params->getReal("width", defaultTrackWidth);
    trackTextureWidthFactor = 1 / params->getReal("widthTexturePart", 1 / trackTextureWidthFactor);
    ShaderGlobal::set_float(get_shader_variable_id("tires_texture_width_factor", true), trackTextureWidthFactor);

    loadRange(params, "length", segmentLength);
    debug("segmentLength %g %g", segmentLength.getMin(), segmentLength.getMax());
    if (segmentLength.getMin() < 0.7f || segmentLength.getMax() < 3.0f)
      segmentLength = RealRange(0.7f, 3.f);

    maxTrackCount = min(params->getInt("maxTrackCount", maxTrackCount), MAX_ALLOWED_TRACKS);
    debug("maxTrackCount = %d", maxTrackCount);
    maxVisibleDist = params->getReal("maxVisibleDist", maxVisibleDist);

    shaderName = params->getStr("shaderName", "tires_default");

    tcYstep = params->getReal("tcYstep", 50.f);

    transparency = params->getReal("transparency", 1.f);

    fadeTrackEnds = params->getBool("fadeTrackEnds", false);
    maxOmnidirTexBlendDelta = params->getReal("maxOmnidirTexBlendDelta", 1.0f);
  }

  init_emitters();

  if (stub_render_mode)
    return;

  tires_drift_texVarId = get_shader_variable_id("tires_drift_tex");
  tires_diffuse_texVarId = get_shader_variable_id("tires_diffuse_tex");
  tires_normal_texVarId = get_shader_variable_id("tires_normal_tex", true);
  track_smoothness_reflectanceVarId = get_shader_variable_id("track_smoothness_reflectance", true);
  tires_texture_widthVarId = get_shader_variable_id("tires_texture_width", true);
  tires_frame_countVarId = get_shader_variable_id("tires_frame_count", true);
  tires_start_instVarId = get_shader_variable_id("tires_start_inst", true);
  tires_tcY_stepVarId = get_shader_variable_id("tires_tcY_step", true);

  ShaderGlobal::set_float(tires_tcY_stepVarId, tcYstep);

  update_samplers();

  // load texture params
  const char *texName = NULL;
  const char *omnidirectionalTexName = NULL;
  const char *texNormalName = NULL;

  params = blk.getBlockByName("Texture");
  if (params)
  {
    trackTypes.resize(params->blockCount());
    renderCount.resize(params->blockCount());
    for (uint32_t trackTypeNo = 0; trackTypeNo < trackTypes.size(); trackTypeNo++)
    {
      const DataBlock *trackBlk = params->getBlock(trackTypeNo);

      texName = trackBlk->getStr("name", NULL);
      texNormalName = trackBlk->getStr("normalName", NULL);

      trackTypes[trackTypeNo].frameCount = trackBlk->getInt("frames", 0);
      trackTypes[trackTypeNo].frameSpacing = trackBlk->getReal("spacing", 0.002f);
      trackTypes[trackTypeNo].textureLength = trackBlk->getReal("textureLength", 10.0f);
      trackTypes[trackTypeNo].textureWidth =
        0.5f / trackTypes[trackTypeNo].frameCount - trackTypes[trackTypeNo].frameCount * trackTypes[trackTypeNo].frameSpacing;

      trackTypes[trackTypeNo].diffuseTexId = ::get_tex_gameres(texName);

      Texture *texture = (Texture *)(::acquire_managed_tex(trackTypes[trackTypeNo].diffuseTexId));
      if (!texture)
        logerr("can not load texture <%s> for tire tracks", texName);
      G_UNUSED(texture);
      release_managed_tex(trackTypes[trackTypeNo].diffuseTexId);

      if (texNormalName && has_normalmap && tires_normal_texVarId >= 0)
        trackTypes[trackTypeNo].normalTexId = ::get_tex_gameres(texNormalName);
      else
        trackTypes[trackTypeNo].normalTexId = BAD_TEXTUREID;

      G_ASSERT(!has_normalmap || !VariableMap::isGlobVariablePresent(tires_normal_texVarId) || texNormalName != NULL);

      trackTypes[trackTypeNo].reflectance = trackBlk->getReal("reflectance", 0.5f);
      trackTypes[trackTypeNo].smoothness = trackBlk->getReal("smoothness", 0.3f);

      unsigned writeAlbedo = trackBlk->getBool("writeAlbedo", true) ? (WRITEMASK_RED0 | WRITEMASK_GREEN0 | WRITEMASK_BLUE0) : 0;
      unsigned writeNormal = trackBlk->getBool("writeNormal", true) ? (WRITEMASK_RED1 | WRITEMASK_GREEN1) : 0;
      unsigned writeSmoothness = trackBlk->getBool("writeSmoothness", false) ? (WRITEMASK_BLUE1) : 0;
      unsigned writeMicrodetail = trackBlk->getBool("writeMaterial", false) ? (WRITEMASK_BLUE2) : 0;
      unsigned writeReflectance = trackBlk->getBool("writeReflectance", false) ? (WRITEMASK_GREEN2) : 0;
      unsigned writeAO = trackBlk->getBool("writeAO", false) ? (WRITEMASK_RED2) : 0;

      unsigned writemask = writeAlbedo | writeNormal | writeMicrodetail | writeSmoothness | writeReflectance | writeAO;

      shaders::OverrideState state;
      state.colorWr = writemask;
      trackTypes[trackTypeNo].shaderOverride = shaders::overrides::create(state);
    }
  }
  else
  {
    logerr("Texture block not found");
  }

  omnidirectionalTexName = blk.getStr("omnidirectionalTextureName", NULL);

  driftTexId = ::get_tex_gameres(omnidirectionalTexName);

  nodeCount = maxTrackCount * (4 + MAX_NODES_PER_TRACK);

  renderDataVS = dag::create_sbuffer(sizeof(Point4), nodeCount * sizeof(TireTrackNode) / sizeof(Point4),
    SBCF_DYNAMIC | SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE, TEXFMT_A32B32G32R32F, String(0, "tire_tracks_data_vs"), RESTAG_TRACK);

  trackMaterial.init(shaderName, NULL, 0, "tire_track shader");
  tiresProjectiveDecalShader.init("tires_projective_decal", NULL, 0, "tire_track shader", true);
  index_buffer::init_box();
}

// release system
void setShaderVars(int track_type_no)
{
  ShaderGlobal::set_texture(tires_normal_texVarId, trackTypes[track_type_no].normalTexId);
  ShaderGlobal::set_texture(tires_diffuse_texVarId, trackTypes[track_type_no].diffuseTexId);
  ShaderGlobal::set_texture(tires_drift_texVarId, driftTexId);

  ShaderGlobal::set_int(tires_frame_countVarId, trackTypes[track_type_no].frameCount);
  ShaderGlobal::set_float(tires_texture_widthVarId, trackTypes[track_type_no].textureWidth);

  ShaderGlobal::set_float4(track_smoothness_reflectanceVarId, trackTypes[track_type_no].smoothness,
    trackTypes[track_type_no].reflectance, 0, 0);
}
void release()
{
  G_ASSERT(initialized);
  initialized = false;

  close_emitters();

  ShaderGlobal::set_texture(tires_normal_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(tires_diffuse_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(tires_drift_texVarId, BAD_TEXTUREID);

  if (driftTexId != BAD_TEXTUREID)
    release_managed_tex(driftTexId); // for get_tex_gameres

  for (uint32_t i = 0; i < trackTypes.size(); i++)
  {
    if (trackTypes[i].normalTexId != BAD_TEXTUREID)
      release_managed_tex(trackTypes[i].normalTexId); // for get_tex_gameres

    if (trackTypes[i].diffuseTexId != BAD_TEXTUREID)
      release_managed_tex(trackTypes[i].diffuseTexId); // for get_tex_gameres

    trackTypes[i].diffuseTexId = trackTypes[i].normalTexId = BAD_TEXTUREID;
    shaders::overrides::destroy(trackTypes[i].shaderOverride);
  }
  driftTexId = BAD_TEXTUREID;
  clear_and_shrink(trackTypes);
  renderDataVS.close();
  deformPressureBuf.close();
  trackMaterial.close();
  tiresProjectiveDecalShader.close();
  index_buffer::release_box();
}

//*************************************************
// emitters
//*************************************************
static Tab<TrackEmitter> emitters(midmem_ptr());

// remove all tire tracks from screen
void clear(bool completeClear)
{
  // remove tracks from render
  if (completeClear)
  {
    erase_emitters();
  }
}

void before_render(float /*dt*/, bool need_deform_pressure_buffer)
{
  if (!renderDataVS.getBuf())
    return;

  FRAMEMEM_REGION;
  Tab<TireTrackNode> renderData(framemem_ptr());
  Tab<float> deformPressure(framemem_ptr());

  int lastSize = 0;
  int trackStartIndex = 0;
  for (int renderType = 0; renderType < trackTypes.size(); renderType++)
  {
    for (const auto &tr : emitters)
    {
      if (tr.trackTypeNo != renderType || tr.track.size() == 0)
        continue;

      if (!fadeTrackEnds)
      {
        // empty node before track
        int idx = tr.getStart() > tr.track.size() ? 0 : tr.getStart();
        TireTrackNode plugNode = tr.track[idx];
        plugNode.packData();
        renderData.push_back(plugNode);
      }

      // todo: memset instead of push?
      // per-point data
      for (int j = tr.getStart(); j < tr.track.size(); j++)
        renderData.push_back(tr.track[j]);

      for (int j = 0; j < tr.getStart(); j++)
        renderData.push_back(tr.track[j]);

      if (!fadeTrackEnds)
      {
        // empty node after track
        int idx = tr.getStart() > 0 ? (tr.getStart() - 1) : (tr.track.size() - 1);
        TireTrackNode plugNode = tr.track[idx];
        plugNode.packData();
        renderData.push_back(plugNode);
      }
      else
      {
        renderData[trackStartIndex].setOpacity(0);
        renderData.back().setOpacity(0);
      }
      if (need_deform_pressure_buffer)
        deformPressure.resize(renderData.size(), tr.deformPressureMult);
      trackStartIndex = renderData.size();
    }
    renderCount[renderType] = renderData.size() - lastSize;
    lastSize = renderData.size();
  }

  uint32_t lockCount = min(renderData.size(), nodeCount);

  if (lockCount > 0)
    renderDataVS.getBuf()->updateDataWithLock(0, sizeof(TireTrackNode) * lockCount, renderData.data(), VBLOCK_DISCARD);
  renderDataVS.setVar();
  if (need_deform_pressure_buffer && lockCount > 0)
  {
    // todo: it would be better to rework this buffer if more per emitter params is needed in future
    if (!deformPressureBuf)
      deformPressureBuf = dag::buffers::create_one_frame_sr_structured(sizeof(float), nodeCount, "tire_tracks_deform_pressure");
    if (deformPressureBuf)
      deformPressureBuf.getBuf()->updateDataWithLock(0, sizeof(float) * lockCount, deformPressure.data(), VBLOCK_DISCARD);
  }
}

void invalidate_region(const BBox3 &bbox)
{
  BBox2 box(Point2(bbox.lim[0].x, bbox.lim[0].z), Point2(bbox.lim[1].x, bbox.lim[1].z));
  for (int i = 0; i < emitters.size(); i++)
  {
    TrackEmitter &track = emitters[i];

    track.invalidateRegion(box);
  }
}

void set_current_params(const DataBlock *data)
{
  if (data)
    default_texture_idx = data->getInt("defaultTexIdx", 0);
}

const Tab<BBox3> &get_updated_regions()
{
  for (int i = 0; i < emitters.size(); i++)
  {
    TrackEmitter &track = emitters[i];

    if (!track.alive)
      continue;

    BBox3 box;
    box = track.getUpdateBbox();
    if (!box.isempty() && lengthSq(box.width()) > VTEX_UPDATE_BOX_WIDTH_THRESHOLD_SQ)
    {
      updated_regions.push_back(box);
      track.invalidateUpdateBbox();
    }
  }
  return updated_regions;
}

void clear_updated_regions() { updated_regions.clear(); }

static void init_emitters()
{
  // this function called after tires params changed, so we need to reset emitters indices in vertex buffer

  emitters.reserve(maxTrackCount);
  updated_regions.reserve(maxTrackCount);
}

static int found_or_create_free_emitter(int track_type_no)
{
  for (int i = 0; i < emitters.size(); i++)
    if (!emitters[i].alive && emitters[i].trackTypeNo == track_type_no)
      return i;

  if (emitters.size() >= maxTrackCount)
    return -1;

  emitters.push_back();
  return emitters.size() - 1;
}

static void close_emitters()
{
  clear_and_shrink(emitters);
  clear_and_shrink(updated_regions);
}

static void erase_emitters()
{
  close_emitters();
  for (int i = 0; i < emitters.size(); i++)
    emitters[i].track.clear();
}

// create new track emitter
int create_emitter(float width, float /*texture_length_factor*/, float min_time, float prio_scale_factor, int track_type_no)
{
  G_UNUSED(prio_scale_factor);
  G_UNUSED(min_time);

  if (!initialized)
    return -1;
  if (track_type_no < 0 || track_type_no >= trackTypes.size())
    return -1;

  int id = found_or_create_free_emitter(track_type_no);

  if (id < 0)
    return -1;

  emitters[id].init(width, track_type_no);

  return id;
}

bool emit(int emitterId, const Point3 &norm, const Point3 &pos, const Point3 &movedir, real opacity, real wetness, int tex_id,
  float additional_width, float omnidirectional_tex_blend, bool correct_previous_node, bool direction_changed)
{
  if (emitterId < 0 || emitterId >= emitters.size())
    return false;

  return emitters[emitterId].emit(norm, pos, movedir, opacity, wetness, tex_id, additional_width, omnidirectional_tex_blend,
    correct_previous_node, direction_changed);
}

void set_emitter_deform_pressure(int emitterId, float pressure_mult)
{
  if (emitterId < 0 || emitterId >= emitters.size())
    return;
  emitters[emitterId].deformPressureMult = pressure_mult;
}

// delete track emitter.
void delete_emitter(int emitterId)
{
  if (emitterId < 0 || emitterId >= emitters.size())
    return;

  emitters[emitterId].alive = false;
}

void render_strips(bool apply_clipmap_writemask)
{
  if (!renderDataVS.getBuf())
    return;

  index_buffer::Quads32BitUsageLock quads32Lock;

  int rendered = 0;
  // render
  shaders::OverrideStateId savedStateId = shaders::overrides::get_current();
  for (int renderType = 0; renderType < trackTypes.size(); renderType++)
  {
    if (renderCount[renderType] < 2)
      continue;

    setShaderVars(renderType);
    ShaderGlobal::set_int(tires_start_instVarId, rendered);

    if (apply_clipmap_writemask)
    {
      shaders::overrides::reset();
      shaders::overrides::set(trackTypes[renderType].shaderOverride);
    }

    d3d::setvsrc_ex(0, NULL, 0, 0);

    if (!trackMaterial.shader->setStates(0, true))
      return;

    d3d::drawind_instanced(PRIM_TRILIST, 0, 2, 0, renderCount[renderType] - 1);

    rendered += renderCount[renderType];
  }

  if (apply_clipmap_writemask)
  {
    shaders::overrides::reset();
    shaders::overrides::set(savedStateId);
  }
}

void render_projective_decals()
{
  if (!tiresProjectiveDecalShader.shader || !renderDataVS.getBuf())
    return;
  TIME_D3D_PROFILE(tire_tracks_decals);
  index_buffer::use_box();
  int startInstance = 0;
  for (int renderType = 0; renderType < trackTypes.size(); renderType++)
  {
    if (renderCount[renderType] < 2)
      continue;
    setShaderVars(renderType);
    ShaderGlobal::set_int(tires_start_instVarId, startInstance);
    d3d::setvsrc_ex(0, NULL, 0, 0);
    if (!tiresProjectiveDecalShader.shader->setStates())
      return;
    d3d::drawind_instanced(PRIM_TRILIST, 0, 12, 0, renderCount[renderType] - 1);
    startInstance += renderCount[renderType];
  }
}

void render_debug(bool show_nodes_data)
{
  begin_draw_cached_debug_lines(false, false, false);
  for (const auto &tr : emitters)
  {
    if (tr.track.empty())
      continue;
    int startNodeIdx = tr.getStart();
    Point3 prevPos = tr.track[startNodeIdx].pos;
    E3DCOLOR trackDebugColor = E3DCOLOR(255, 0, 0);
    E3DCOLOR textureExtendColor = E3DCOLOR(255, 255, 255);
    for (int i = 0; i < tr.track.size(); ++i)
    {
      int nodeIdx = (startNodeIdx + i) % tr.track.size();

      const TireTrackNode &node = tr.track[nodeIdx];

      Point3 pos = node.pos;
      Point3 sideVector = Point3::x0y(node.getSideVector());
      draw_cached_debug_line(pos - sideVector, pos + sideVector, textureExtendColor);
      Point3 trackHalfWidthVector = normalize(sideVector) * tr.trackWidth * 0.5f;
      draw_cached_debug_line(pos - trackHalfWidthVector, pos + trackHalfWidthVector, trackDebugColor);
      draw_cached_debug_line(prevPos, pos, trackDebugColor);
      prevPos = pos;
      if (show_nodes_data)
      {
        int texId = decode_float2(node.material).x * trackTypes[tr.trackTypeNo].frameCount;
        float omnidirBlend = node.getOmnidirblend();
        float opacity = node.getOpacity();
        float wetness = node.getWetness();
        add_debug_text_mark(pos, String(0, "texId: %d", texId), -1, 0.f);
        add_debug_text_mark(pos, String(0, "omnidir: %f", omnidirBlend), -1, 1.f);
        add_debug_text_mark(pos, String(0, "opacity: %f", opacity), -1, 2.f);
        add_debug_text_mark(pos, String(0, "wetness: %f", wetness), -1, 3.f);
      }
    }
  }
  end_draw_cached_debug_lines();
}

bool is_initialized() { return initialized; }

} // namespace tire_tracks
