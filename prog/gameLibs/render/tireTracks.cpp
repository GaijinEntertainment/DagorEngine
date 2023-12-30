#include <generic/dag_tabWithLock.h>
#include <render/tireTracks.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_stlqsort.h>
// #include <shaders/dag_dynShaderBuf.h>
// #include <3d/dag_ringDynBuf.h>
// #include <generic/dag_tabUtils.h>
#include <shaders/dag_shaders.h>
#include <generic/dag_carray.h>
#include <generic/dag_range.h>
#include <generic/dag_tab.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <3d/dag_drv3d.h>
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


// TODO: boxes. fast and more correct (grid?) visibility for vtex. alttab.

namespace tire_tracks
{
static void init_emitters();
static void close_emitters();
static void erase_emitters();
typedef Range<real> RealRange;

static bool initialized = false;
static String shaderName;

static Vbuffer *vbuffer = NULL;
static bool has_vertex_normal = false;

static Tab<Point2> allVertices;
static Tab<BBox3> updated_regions;
struct TracksInvalidateCommand
{
  Point3 pos;
  float radius_sq = 0;
  int emitter_id = -1;
  int track_id = -1;
};
static eastl::deque<TracksInvalidateCommand> invalidateRegionsQueue;

static int default_texture_idx = 0;
static int maxTrackVertices = 800;
static bool have_orphan_proxy = false;
static float transparency = 1.f;

static constexpr int MIN_TRIANGLES_IN_BATCH = 8;
#define MIN_MOVE_DIST_TO_UPDATE_VB         0.05f
#define VTEX_UPDATE_BOX_WIDTH_THRESHOLD_SQ (0.2 * 0.2)
#define MIN_TRACK_VERTICES                 32
#define LQ_MAX_TRACK_VERTICES_MUL          0.5f
#define MAX_ALLOWED_TRACKS                 256

//*************************************************
// struct TireTrackVertex
//*************************************************

struct TireTrackVertexNorm;

struct TireTrackVertexNoNorm
{
  Point3 pos;
  Point4 tex;

  void operator=(const TireTrackVertexNorm &rh);
};
struct TireTrackVertexNorm : public TireTrackVertexNoNorm
{
  E3DCOLOR norm;
};

void TireTrackVertexNoNorm::operator=(const TireTrackVertexNorm &rh)
{
  pos = rh.pos;
  tex = rh.tex;
}

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

  shaders::UniqueOverrideStateId shaderOverride;
};

Tab<TrackType> trackTypes;

class TireTrack
{
public:
  const TireTrack *getPair() const { return pairedTrack; }

  // we need 5 pairs buffer for case - change texture with overlapping:
  // 2 pair for previous batch: update last pos and degenerate triangles
  // 3 pairs for new batch - 2 segments: ovelaped/transparent gradiend segment and one new
  static const int MAX_PAIRS_UPDATED_PER_FRAME = 5;

  TireTrack() : vofs(0), pairedTrack(NULL), maxVerts(0) { reset(); }
  void init(int vpos, int maxVertices, TireTrack *pair)
  {
    G_ASSERTF(vpos < allVertices.size(), "vofs = %d, allVertices.size() = %d", vpos, allVertices.size());
    G_ASSERTF(vpos + maxVertices <= allVertices.size(), "vofs = %d+maxVerts=%d, allVertices.size() = %d", vpos, maxVertices,
      allVertices.size());
    vofs = vpos;
    pairedTrack = pair;
    maxVerts = maxVertices;
    reset();
  }

  // render track to buffer
  bool hasBatches() const { return batches.size() > 0; }
  int getBatchCount() const { return batches.size(); }
  int getLastBatchVertCount() { return batches.size() > 0 ? batches.back().twoVerts : 0; }
  int getLastBatchSegmentCount() { return batches.size() > 0 ? batches.back().twoVerts - 1 : 0; }
  bool shouldRender() const { return batches.size() > 0 && batches[0].twoVerts >= 1; }
  bool wasChanged() const { return updatedPairsCount > 0; }

  void render()
  {
    // debug("rendering %d batches", batches.size());
    // debug("%d batch: %d faces from %d", i, (batches[i].twoVerts-1)<<1, vofs + batches[i].renderVofs);
    int startBatch = 0;
    int endBatch = batches.size() - 1;
    while (startBatch < batches.size() && batches[startBatch].twoVerts < 2)
      startBatch++;
    while (endBatch >= 0 && batches[endBatch].twoVerts < 2)
      endBatch--;
    if (endBatch < startBatch)
      return;

    d3d::draw(PRIM_TRISTRIP, vofs + batches[startBatch].renderVofs,
      (batches[endBatch].twoVerts << 1) + (batches[endBatch].renderVofs - batches[startBatch].renderVofs) - 2);
  }
  void recalculateEatenBBox()
  {
    if (!batches.size())
      return;
    BBox2 box;
    for (int i = 0; i < batches.size(); ++i)
    {
      int offset = vofs + batches[i].renderVofs;
      for (int j = 0; j < batches[i].twoVerts; ++j, offset += 2)
      {
        box += allVertices[offset + 0];
        box += allVertices[offset + 1];
      }
    }
    bbox.bmin = v_make_vec4f(box[0].x, v_extract_y(bbox.bmin), box[0].y, 0);
    bbox.bmax = v_make_vec4f(box[1].x, v_extract_y(bbox.bmax), box[1].y, 0);
  }
  bool eatVertices()
  {
    if (!batches.size())
      return false;
    if (batches[0].twoVerts >= 2)
    {
      int offset = batches[0].renderVofs + vofs;
      eaten += allVertices[offset + 0];
      eaten += allVertices[offset + 1];
      batches[0].twoVerts--;
      batches[0].renderVofs += 2;
      if (batches[0].twoVerts < 2)
      {
        eaten += allVertices[offset + 2];
        eaten += allVertices[offset + 3];
        erase_items(batches, 0, 1);
        if (!batches.size())
          reset();
        else
          recalculateEatenBBox();
      }
      else
      {
        if ((batches[0].twoVerts & 7) == 0)
          recalculateEatenBBox();
      }
    }
    return shouldRender();
  }
  // reset for future usage
  void reset()
  {
    v_bbox3_init_empty(bbox);
    v_bbox3_init_empty(updateBbox);
    eaten.setempty();
    batches.clear();
    updatedPairsCount = 0;
  }

  void clear()
  {
    reset();
    clear_and_shrink(batches);
  }

  // return vertex count
  inline int getTotalVertCount() const { return batches.size() ? batches.back().renderVofs + (batches.back().twoVerts << 1) : 0; }

  // set last vertices to another values
  void replaceLastVert(const TireTrackVertexNorm *tv)
  {
    if (!batches.size())
      return;
    if (updatedPairsCount >= MAX_PAIRS_UPDATED_PER_FRAME)
    {
      G_ASSERT(updatedPairsCount < MAX_PAIRS_UPDATED_PER_FRAME);
      return;
    }
    Batch &pTop = batches.back();
    if (pTop.twoVerts || pTop.renderVofs)
    {
      memcpy(&updatedPairs[updatedPairsCount].Vertices[0], tv, sizeof(TireTrackVertexNorm) * 2);
      updatedPairs[updatedPairsCount].Index = pTop.renderVofs + (pTop.twoVerts << 1) - 2;
      updatedPairsCount++;
      lastUpdatedVertex = tv[1];
    }
  }

  TireTrackVertexNorm *getLastSegment() { return &lastSegmentVertices[0]; }

  template <class VType>
  void invalidateVtxBuf(VType *vbufOrig, Point3 pos, float radius_sq)
  {
    TIME_D3D_PROFILE(tire_tracks_invalidate_region);
    VType *vbuf = vbufOrig + vofs;
    for (int i = 0; i < maxTrackVertices; i++)
    {
      if ((pos - vbuf[i].pos).lengthSq() <= radius_sq)
        vbuf[i].tex.w = 0;
    }
  }

  template <class VType>
  void updateVtxBuf(VType *vbufOrig, int emitterIndex, int batchId)
  {
    (void)batchId;
    (void)emitterIndex;
    for (int i = 0; i < updatedPairsCount; ++i)
    {
      int offset = vofs + updatedPairs[i].Index;

      if (offset + 2 > allVertices.size())
      {
        G_ASSERTF(0, "emitter #%i: offset %i out of range %i. tire vofs=%i, batch #%i (renderVofs=%i, twoVerts=%i)", emitterIndex,
          offset + 1, allVertices.size(), vofs, batchId, batches[batchId].renderVofs, batches[batchId].twoVerts);
        continue;
      }

      VType *vbuf = vbufOrig + offset;
      vbuf[0] = updatedPairs[i].Vertices[0];
      vbuf[1] = updatedPairs[i].Vertices[1];
      allVertices[offset + 0] = Point2::xz(updatedPairs[i].Vertices[0].pos);
      allVertices[offset + 1] = Point2::xz(updatedPairs[i].Vertices[1].pos);
    }
  }

  void updateVbuf(void *vbufOrig, int emitterIndex)
  {
    (void)emitterIndex;
    if (!batches.size())
      return;
    int batchId = batches.size() - 1;
    while (batchId >= 0 && !batches[batchId].twoVerts)
      batchId--;
    if (batchId < 0)
    {
      updatedPairsCount = 0;
      return;
    }

    if (tire_tracks::has_vertex_normal)
    {
      TireTrackVertexNorm *dstBuf = (TireTrackVertexNorm *)vbufOrig;
      updateVtxBuf(dstBuf, emitterIndex, batchId);
    }
    else
    {
      TireTrackVertexNoNorm *dstBuf = (TireTrackVertexNoNorm *)vbufOrig;
      updateVtxBuf(dstBuf, emitterIndex, batchId);
    }

    for (int i = 0; i < updatedPairsCount; ++i)
      updateBBox(&updatedPairs[i].Vertices[0]);
    v_bbox3_add_box(bbox, updateBbox);
    updatedPairsCount = 0;
  }
  bool addBatch()
  {
    if (batches.size())
    {
      if (!batches.back().twoVerts) // just restart previous incomplete batch
      {
        return true;
      }
    }
    G_STATIC_ASSERT(MIN_TRIANGLES_IN_BATCH >= 8); // we need at least 6 vertices reserved
    if (getTotalVertCount() + MIN_TRIANGLES_IN_BATCH + 2 > maxVerts)
      return false;
    int batchId = append_items(batches, 1);
    batches[batchId].twoVerts = 0;
    batches[batchId].renderVofs = batchId > 0 ? batches[batchId - 1].renderVofs + (batches[batchId - 1].twoVerts << 1) + 2 : 0;
    return true;
  }
  bool addVertices(TireTrackVertexNorm *pVertices)
  {
    if (!batches.size())
      return false;
    if (getTotalVertCount() + 4 <= maxVerts)
    {
      if (batches.size() > 1 && batches.back().twoVerts == 0)
      {
        // degenerative tris
        TireTrackVertexNorm degenTris[2];
        degenTris[0] = lastUpdatedVertex;
        degenTris[1] = pVertices[0];
        replaceLastVert(degenTris);
      }

      memcpy(lastSegmentVertices, pVertices, sizeof(lastSegmentVertices));
      batches.back().twoVerts++;
      replaceLastVert(pVertices);
      pairedTrack->eatVertices();
      return true;
    }
    return false;
  }
  bbox3f getBbox() const { return bbox; }
  BBox3 getUpdateBbox() const
  {
    BBox3 box;
    v_stu_bbox3(box, updateBbox);
    if (!eaten.isempty())
    {
      float posY = as_point3(&bbox.bmin).y;
      BBox3 eatenBox(Point3(eaten[0].x, posY, eaten[0].y), Point3(eaten[1].x, posY, eaten[1].y));
      box += eatenBox;
    }
    return box;
  }
  BBox3 getEatenBbox() const
  {
    BBox3 box;
    if (!eaten.isempty())
    {
      float posY = as_point3(&updateBbox.bmin).y;
      box[0] = Point3(eaten[0].x, posY, eaten[0].y);
      box[1] = Point3(eaten[1].x, posY, eaten[1].y);
    }
    return box;
  }
  void invalidateUpdateBbox() { v_bbox3_init_empty(updateBbox); }
  void invalidateEatenBbox() { eaten.setempty(); }

private:
  BBox2 eaten;
  bbox3f bbox;       // full bounding box
  bbox3f updateBbox; // only last changed vertices
  struct Batch
  {
    int twoVerts, renderVofs;
  };
  int vofs, maxVerts;
  Tab<Batch> batches;

  struct UpdatedPair
  {
    TireTrackVertexNorm Vertices[2];
    int Index; // index in local vertices. add vpos to map to vbuffer
  };

  carray<UpdatedPair, MAX_PAIRS_UPDATED_PER_FRAME> updatedPairs;
  int updatedPairsCount;

  TireTrackVertexNorm lastSegmentVertices[2]; // last added segment, used for segment overlapping
  TireTrackVertexNorm lastUpdatedVertex;

  TireTrack *pairedTrack;

  inline void updateBBox(const TireTrackVertexNorm *tv)
  {
    v_bbox3_add_pt(updateBbox, v_ldu(&tv[0].pos.x));
    v_bbox3_add_pt(updateBbox, v_ldu(&tv[1].pos.x));
  }
};

//*************************************************
// constants
//*************************************************

// maximum count of tracks
static int maxTrackCount = 32 * 2; // 32 tanks by 2 tracks (by double buffer)

// maximum visible distance for tracks (meters)
static real maxVisibleDist = 100.0f;

// size of vertex buffer for all tracks (in vertices)
static int vertexCount = maxTrackVertices * maxTrackCount * 2;

// distance between tracks & clipmesh
static real trackSlope = 0.06f;

// default track width (in meters)
static real defaultTrackWidth = 0.35f;

// 1 / part of texture width
static real trackTextureWidthFactor = 3.f;

// minimal & maximal track segment length (in meters)
static RealRange segmentLength(0.5f, 1.0f);

// minimal horizontal angle (grad), when tracks must be split
static real hAngleLimit = DEG_TO_RAD * 30.0f;
static real vAngleLimit = DEG_TO_RAD * 20.0f;

//*************************************************
// class TrackEmitter
//*************************************************
class TrackEmitter
{
private:
  TireTrack track[2]; // double buffering
  TireTrack *current;
  real totalBatchLen; // total track length
  Point3 lastNorm;
  float trackWidth;
  int lastTex; // last emitted texture index
  Point3 lastPos;
  Point3 lastSegmentPos;

public:
  TrackEmitter() :
    lastNorm(0, 1, 0),
    trackWidth(defaultTrackWidth),
    current(&track[0]),
    lastTex(-1),
    lastPos(-1e+10F, -1e+10F, -1e+10F),
    trackTypeNo(-1)
  {}
  TireTrack *getCurrentTrack() { return current; }
  TireTrack *getTrack(int j) { return &track[j]; }
  int trackTypeNo;
  void clearTracks()
  {
    track[0].clear();
    track[1].clear();
  }
  void init(int id)
  {
    track[0].init(id * maxTrackVertices, maxTrackVertices, &track[1]);
    track[1].init(id * maxTrackVertices + maxTrackCount * maxTrackVertices, maxTrackVertices, &track[0]);
    current = &track[0];
    lastTex = -1;
  }
  void start(float track_width)
  {
    lastNorm = Point3(0, 1, 0);
    totalBatchLen = 0;
    trackWidth = track_width > 0 ? track_width : defaultTrackWidth;
  }
  void swapTracks()
  {
    current = (current == &track[0]) ? &track[1] : &track[0]; // swap tracks
    current->reset();
  }
  void addBatch(float batchStartLen = 0.f)
  {
    if (!current->addBatch())
      swapTracks();
    totalBatchLen = batchStartLen;
  }

  // emit new track (start new or continue existing)
  void emit(const Point3 &norm, const Point3 &pos, const Point3 &movedir, real opacity, int tex_id, float additional_width,
    float omnidirectional_tex_blend)
  {
    if (current->wasChanged())
      return;

    lastNorm = normalize(norm);
    if (lastNorm.y < 0)
      lastNorm = -lastNorm;

    TireTrackVertexNorm tv[2];
    int lastBatchVtxs = current->getLastBatchVertCount();
    Point3 segmentDir = lastBatchVtxs ? pos - lastSegmentPos : normalize(movedir);
    float lastSegmLen = lastBatchVtxs ? (pos - lastSegmentPos).length() : 0.f;

    if ((pos - lastPos).lengthSq() > sqr(segmentLength.getMax()))
    {
      finalize();
    }

    if (lastTex != tex_id)
    {
      if (current->getLastBatchSegmentCount() >= 2)
      {
        // can do fadein from last segment, if there at least 2 of them
        genVertices(tv, pos, segmentDir, 0.f, lastSegmLen, additional_width, omnidirectional_tex_blend);
        current->replaceLastVert(tv);
        lastTex = tex_id;

        // prev segm vtxs
        TireTrackVertexNorm *lastSegm = current->getLastSegment();
        TireTrackVertexNorm nv[2];
        memcpy(nv, lastSegm, sizeof(nv));
        totalBatchLen = 0.f;
        genTexCoord(nv, 0.f, 0.f, omnidirectional_tex_blend);

        // start new
        addBatch();
        addVertices(pos, 0.f, nv);
        // cur pos
        genVertices(tv, pos, segmentDir, opacity, lastSegmLen, additional_width, omnidirectional_tex_blend);
        addVertices(pos, lastSegmLen, tv);
        addVertices(pos, 0.f, tv);
      }
      else
      {
        genVertices(tv, pos, segmentDir, opacity, lastSegmLen, additional_width, omnidirectional_tex_blend);
        if (lastTex != -1)
          current->replaceLastVert(tv);

        addBatch();
        lastTex = tex_id;
        genVertices(tv, pos, segmentDir, lastBatchVtxs ? opacity : 0.f, 0.f, additional_width, omnidirectional_tex_blend);
        addVertices(pos, 0.f, tv);
      }
    }
    else
    {
      lastTex = tex_id;
      if ((pos - lastPos).length() > MIN_MOVE_DIST_TO_UPDATE_VB)
      {
        genVertices(tv, pos, segmentDir, opacity, lastSegmLen, additional_width, omnidirectional_tex_blend);

        if (lastBatchVtxs < 2)
          addVertices(pos, 0.f, tv);
        else
        {
          current->replaceLastVert(tv);
          lastPos = pos;

          if (lastSegmLen > segmentLength.getMax())
          {
            // finish
            totalBatchLen += lastSegmLen;
            // new
            addBatch(fmodf(totalBatchLen / trackTypes[trackTypeNo].textureLength, 1.f));

            genVertices(tv, pos, segmentDir, opacity, 0.f, additional_width, omnidirectional_tex_blend);
            addVertices(pos, 0.f, tv);
          }
          else if (lastSegmLen > segmentLength.getMin())
          {
            addVertices(pos, lastSegmLen, tv);
          }
        }
      }
    }
  }

  // finalize track
  void finalize()
  {
    addBatch();
    lastTex = -1;
  }

private:
  void genTexCoord(TireTrackVertexNorm *dstPair, float segmentLen, float alpha, float omnidirectional_tex_blend)
  {
    float longitudinalPosition = totalBatchLen + segmentLen;
    float textureCoord = longitudinalPosition / trackTypes[trackTypeNo].textureLength;
    dstPair[0].tex = Point4((real)lastTex / (real)trackTypes[trackTypeNo].frameCount + trackTypes[trackTypeNo].frameSpacing,
      textureCoord, omnidirectional_tex_blend, alpha);
    dstPair[1].tex = Point4((real)(lastTex + 1) / (real)trackTypes[trackTypeNo].frameCount - trackTypes[trackTypeNo].frameSpacing,
      textureCoord, omnidirectional_tex_blend, alpha);
  }
  void genVertices(TireTrackVertexNorm *dstPair, const Point3 &pos, const Point3 &segment_dir, float alpha, float segmentLen,
    float additional_width, float omnidirectional_tex_blend)
  {
    const Point3 b = normalize(lastNorm % -segment_dir);
    const Point3 slope = lastNorm * trackSlope;

    // real width = trackWidth / 2 + (1.0 - fabsf(movedir * segment_dir)) * (segmentLength.getMin() / 2);
    real trackTextureWidth = trackWidth * trackTextureWidthFactor;
    real width = (trackTextureWidth + additional_width) / 2;

    Point3 pwidth = b * width;

    dstPair[0].pos = pos - pwidth + slope;
    dstPair[1].pos = pos + pwidth + slope;
    E3DCOLOR colorNorm(128.f + 127.f * lastNorm.x, 128.f + 127.f * lastNorm.y, 128.f + 127.f * lastNorm.z);
    dstPair[0].norm = colorNorm;
    dstPair[1].norm = colorNorm;

    genTexCoord(dstPair, segmentLen, alpha * transparency, omnidirectional_tex_blend);
  }
  // add new vertices
  void addVertices(const Point3 &pos, float segmentLen, TireTrackVertexNorm *pVertices)
  {
    if (current->addVertices(pVertices))
      totalBatchLen += segmentLen;
    else
    {
      addBatch();
      current->addVertices(pVertices);
    }

    lastPos = pos;
    lastSegmentPos = pos;
  }
};

//*************************************************
// internal parameters
//*************************************************
// shaders
static Ptr<ShaderMaterial> currentMaterial = NULL;
static Ptr<ShaderElement> currentElem = NULL;

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

static int tires_drift_texVarId = -1, tires_diffuse_texVarId = -1, tires_normal_texVarId = -1, track_tex_frame_mulVarId = -1,
           track_smoothness_reflectanceVarId = -1;
;

// init system. load settings from blk-file
void init(const char *blk_file, bool has_normalmap, bool has_vertex_norm, bool is_low_quality, bool stub_render_mode)
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
    trackSlope = params->getReal("slope", trackSlope);
    maxTrackVertices = params->getInt("maxTrackVertices", maxTrackVertices);

    if (maxTrackVertices < MIN_TRACK_VERTICES)
      DAG_FATAL("Vertex count (%d) and max track vertices (%d) for tire tracks must be > %d!", vertexCount, maxTrackVertices,
        MIN_TRACK_VERTICES);

    if (is_low_quality)
      maxTrackVertices = max(int(maxTrackVertices * LQ_MAX_TRACK_VERTICES_MUL), MIN_TRACK_VERTICES);

    DataBlock *angles = params->getBlockByName("critical_angle");
    if (angles)
    {
      hAngleLimit = params->getReal("horizontal", hAngleLimit);
      vAngleLimit = params->getReal("vertical", vAngleLimit);
    }

    defaultTrackWidth = params->getReal("width", defaultTrackWidth);
    trackTextureWidthFactor = 1 / params->getReal("widthTexturePart", 1 / trackTextureWidthFactor);

    loadRange(params, "length", segmentLength);
    debug("segmentLength %g %g", segmentLength.getMin(), segmentLength.getMax());
    if (segmentLength.getMin() < 0.7f || segmentLength.getMax() < 3.0f)
      segmentLength = RealRange(0.7f, 3.f);

    maxTrackCount = params->getInt("maxTrackCount", maxTrackCount);
    debug("maxTrackCount = %d", maxTrackCount);
    vertexCount = maxTrackVertices * maxTrackCount * 2;
    maxVisibleDist = params->getReal("maxVisibleDist", maxVisibleDist);

    shaderName = params->getStr("shaderName", "tires_default");

    transparency = params->getReal("transparency", 1.f);
  }
  debug("allVertices.resize(%d), (%d)", vertexCount, maxTrackVertices * maxTrackCount * 2);
  // vertexCount = maxTrackVertices*maxTrackCount*2;//fixme: uncomment this line, if assert proven
  allVertices.resize(vertexCount);
  mem_set_0(allVertices);

  init_emitters();

  if (stub_render_mode)
    return;

  tires_drift_texVarId = get_shader_variable_id("tires_drift_tex");
  tires_diffuse_texVarId = get_shader_variable_id("tires_diffuse_tex");
  tires_normal_texVarId = get_shader_variable_id("tires_normal_tex", true);
  track_tex_frame_mulVarId = get_shader_variable_id("track_tex_frame_mul", true);
  track_smoothness_reflectanceVarId = get_shader_variable_id("track_smoothness_reflectance", true);

  // load texture params
  const char *texName = NULL;
  const char *omnidirectionalTexName = NULL;
  const char *texNormalName = NULL;

  params = blk.getBlockByName("Texture");
  if (params)
  {
    trackTypes.resize(params->blockCount());
    for (uint32_t trackTypeNo = 0; trackTypeNo < trackTypes.size(); trackTypeNo++)
    {
      const DataBlock *trackBlk = params->getBlock(trackTypeNo);

      texName = trackBlk->getStr("name", NULL);
      texNormalName = trackBlk->getStr("normalName", NULL);

      trackTypes[trackTypeNo].frameCount = trackBlk->getInt("frames", 0);
      trackTypes[trackTypeNo].frameSpacing = trackBlk->getReal("spacing", 0.002f);
      trackTypes[trackTypeNo].textureLength = trackBlk->getReal("textureLength", 10.0f);

      trackTypes[trackTypeNo].diffuseTexId = ::get_tex_gameres(texName);

      Texture *texture = (Texture *)(::acquire_managed_tex(trackTypes[trackTypeNo].diffuseTexId));
      G_ASSERTF(texture, "can not load texture <%s> for tire tracks", texName);
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
      unsigned writeMicrodetail = trackBlk->getBool("writeMaterial", false) ? (WRITEMASK_BLUE1) : 0;
      unsigned writeSmoothness = trackBlk->getBool("writeSmoothness", false) ? (WRITEMASK_GREEN2) : 0;
      unsigned writeReflectance = trackBlk->getBool("writeReflectance", false) ? (WRITEMASK_BLUE2) : 0;

      unsigned writemask = writeAlbedo | writeNormal | writeMicrodetail | writeSmoothness | writeReflectance;

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

  // create shader buffer
  tire_tracks::has_vertex_normal = has_vertex_norm;
  int vsize = has_vertex_norm ? sizeof(TireTrackVertexNorm) : sizeof(TireTrackVertexNoNorm);
  vbuffer = d3d::create_vb(vertexCount * vsize, SBCF_DYNAMIC, "tires");

  // create material & shader
  Ptr<MaterialData> matNull = new MaterialData;
  matNull->className = shaderName;
  G_ASSERT(omnidirectionalTexName != NULL);

  currentMaterial = new_shader_material(*matNull);

  if (currentMaterial == NULL)
  {
    DAG_FATAL("StdGuiRender - shader '%s' not found!", shaderName.str());
  }

  Tab<CompiledShaderChannelId> channels(framemem_ptr());
  channels.resize(has_vertex_norm ? 3 : 2);
  // pos
  channels[0].t = SCTYPE_FLOAT3;
  channels[0].vbu = SCUSAGE_POS;
  channels[0].vbui = 0;

  // tex UV
  channels[1].t = SCTYPE_FLOAT4; // TC and omnidirectional texture blend
  channels[1].vbu = SCUSAGE_TC;
  channels[1].vbui = 0;

  // normal
  if (has_vertex_norm)
  {
    channels[2].t = SCTYPE_E3DCOLOR;
    channels[2].vbu = SCUSAGE_VCOL;
    channels[2].vbui = 0;
  }

  if (!currentMaterial->checkChannels(channels.data(), channels.size()))
  {
    DAG_FATAL("TireTracks::init - invalid channels for shader '%s'!", (char *)currentMaterial->getShaderClassName());
  }

  currentElem = currentMaterial->make_elem();
}

// release system
void setShaderVars(int track_type_no)
{
  ShaderGlobal::set_texture(tires_normal_texVarId, trackTypes[track_type_no].normalTexId);
  ShaderGlobal::set_texture(tires_diffuse_texVarId, trackTypes[track_type_no].diffuseTexId);
  ShaderGlobal::set_real(track_tex_frame_mulVarId, trackTypes[track_type_no].frameCount);
  ShaderGlobal::set_color4(track_smoothness_reflectanceVarId, trackTypes[track_type_no].smoothness,
    trackTypes[track_type_no].reflectance, 0, 0);
  ShaderGlobal::set_texture(tires_drift_texVarId, driftTexId);
}
void release(bool closeEmitters)
{
  G_ASSERT(initialized);
  initialized = false;

  if (closeEmitters)
    close_emitters();
  else
    clear(true);

  del_d3dres(vbuffer);
  currentMaterial = NULL;
  currentElem = NULL;

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
  clear_and_shrink(allVertices);
  clear_and_shrink(trackTypes);
}

//*************************************************
// emitters
//*************************************************
static Tab<TrackEmitter> emitters(midmem_ptr());
static Tab<int16_t> free_emitters;
static TabWithLock<int16_t> used_emitters;
static Tab<int16_t> updated_emitters;

// remove all tire tracks from screen
void clear(bool completeClear)
{
  // finish drawing tracks
  if (!completeClear)
    for (int i = 0; i < emitters.size(); i++)
      emitters[i].finalize();

  // remove tracks from render
  if (completeClear)
  {
    erase_emitters();
    have_orphan_proxy = false;
  }
}

// before render tires
static void beforeRenderProxy(float dt, const Point3 &orig);

void beforeRender(float dt, const Point3 &origin)
{
  TracksInvalidateCommand inv;
  if (invalidateRegionsQueue.size() > 0)
  {
    inv = invalidateRegionsQueue.front();
    invalidateRegionsQueue.pop_front();
  }
  TIME_D3D_PROFILE(tire_tracks_update);
  if (updated_emitters.size() > used_emitters.size())
    updated_emitters.clear(); // for safety

  beforeRenderProxy(dt, origin);

  void *vertices = NULL;
  for (int i = 0; i < used_emitters.size(); i++)
  {
    bool bUpdated = false;
    for (int j = 0; j < 2; ++j)
    {
      bool invalidate_track = used_emitters[i] == inv.emitter_id && j == inv.track_id;
      TireTrack *track = emitters[used_emitters[i]].getTrack(j);
      if (track->wasChanged() || invalidate_track)
      {
        if (!vertices)
        {
          if (!vbuffer->lock(0, 0, (void **)&vertices, VBLOCK_NOOVERWRITE | VBLOCK_WRITEONLY | VBLOCK_NOSYSLOCK))
            return;
        }
        if (track->wasChanged())
        {
          track->updateVbuf(vertices, used_emitters[i]);
          bUpdated = true;
        }
        else // invalidate track
        {
          if (tire_tracks::has_vertex_normal)
            track->invalidateVtxBuf((TireTrackVertexNorm *)vertices, inv.pos, inv.radius_sq);
          else
            track->invalidateVtxBuf((TireTrackVertexNoNorm *)vertices, inv.pos, inv.radius_sq);
        }
      }
    }
    if (bUpdated)
      updated_emitters.push_back(used_emitters[i]);
  }
  if (vertices)
    vbuffer->unlock();
}

void invalidate_region(const BBox3 &bbox)
{
  // increase a radius a bit to make sure track marks are removed from region entirely
  float radius = max(bbox.width().x, max(bbox.width().y, bbox.width().z)) / 2 + 0.5;
  for (int i = 0; i < used_emitters.size(); i++)
  {
    for (int j = 0; j < 2; ++j)
    {
      TireTrack *track = emitters[used_emitters[i]].getTrack(j);
      bbox3f track_bbox = track->getBbox();
      if (v_bbox3_test_box_intersect(track_bbox, v_ldu_bbox3(bbox)))
        invalidateRegionsQueue.push_back({bbox.center(), radius * radius, used_emitters[i], j});
    }
  }
}

// render tires
void render(const Frustum &frustum, bool for_displacement)
{
  TIME_D3D_PROFILE(tire_tracks);
  bool inited = false;
  int lastTrackType = -1;
  // render
  shaders::OverrideStateId savedStateId = shaders::overrides::get_current();
  for (int i = 0; i < used_emitters.size(); i++)
  {
    for (int j = 0; j < 2; j++)
    {
      TireTrack *track = emitters[used_emitters[i]].getTrack(j);
      int currentTrackType = emitters[used_emitters[i]].trackTypeNo;
      if (track->shouldRender() && frustum.testBoxB(track->getBbox().bmin, track->getBbox().bmax))
      {
        inited = (lastTrackType == currentTrackType);
        if (!inited)
        {
          if (!for_displacement)
          {
            shaders::overrides::reset();
            shaders::overrides::set(trackTypes[currentTrackType].shaderOverride);
          }

          setShaderVars(currentTrackType);
          if (!currentElem->setStates(0, true))
            return;
          d3d::setvsrc_ex(0, vbuffer, 0, tire_tracks::has_vertex_normal ? sizeof(TireTrackVertexNorm) : sizeof(TireTrackVertexNoNorm));
          inited = true;
          lastTrackType = currentTrackType;
        }
        track->render();
      }
    }
  }
  if (!for_displacement)
  {
    shaders::overrides::reset();
    shaders::overrides::set(savedStateId);
  }
}

void set_current_params(const DataBlock *data)
{
  if (data)
    default_texture_idx = data->getInt("defaultTexIdx", 0);
}

const Tab<BBox3> &get_updated_regions()
{
  for (int i = 0; i < updated_emitters.size(); i++)
  {
    TireTrack *track = emitters[updated_emitters[i]].getCurrentTrack();
    if (!track)
      continue;
    if (track->shouldRender())
    {
      BBox3 box;
      box = track->getUpdateBbox();
      if (!box.isempty() && lengthSq(box.width()) > VTEX_UPDATE_BOX_WIDTH_THRESHOLD_SQ)
      {
        updated_regions.push_back(box);
        track->invalidateUpdateBbox();
      }
      box = track->getEatenBbox();
      if (!box.isempty() && lengthSq(box.width()) > VTEX_UPDATE_BOX_WIDTH_THRESHOLD_SQ)
      {
        updated_regions.push_back(box);
        track->invalidateEatenBbox();
      }
    }
  }
  updated_emitters.clear();
  return updated_regions;
}

void clear_updated_regions() { updated_regions.clear(); }

class ProxyEmitter
{
public:
  void init(float w, float mint, float dist_scale, int track_type_no)
  {
    width = w;
    min_time = mint;
    dist_scale_factor = fabsf(dist_scale);
    trackTypeNo = track_type_no;
  }

  ProxyEmitter() : emitterId(-1), timeToOwn(0), width(0.35), pos(0, 0, 0), min_time(10), trackTypeNo(-1), dist_scale_factor(1) {}
  void increaseOwnership(float time) { timeToOwn += time; }
  void own(int id)
  {
    emitterId = id;
    timeToOwn = min_time;
    emitters[emitterId].trackTypeNo = trackTypeNo;
    emitters[emitterId].start(width);
  }
  void loose()
  {
    emitterId = -1;
    timeToOwn = 0;
  }
  int getId() const { return emitterId; }
  Point3 getPos() const { return pos; }
  void setPos(const Point3 &p) { pos = p; }
  bool canBeLost() const { return timeToOwn < 0; }
  bool update(float dt)
  {
    timeToOwn -= dt;
    return canBeLost();
  }
  float getTime() { return timeToOwn; }
  int getPriority(const Point3 &origin) const
  {
    return bitwise_cast<int>(lengthSq(pos - origin) * dist_scale_factor); // effectively sqrt(dt_scale_factor)
  }

protected:
  int emitterId;
  float timeToOwn, dist_scale_factor, min_time;
  float width;
  int trackTypeNo;
  Point3 pos;
};

static Tab<ProxyEmitter> proxy_emitters(midmem_ptr());
static Tab<int16_t> free_proxy_emitters;
static TabWithLock<int16_t> used_proxy_emitters;

void check_consistency()
{
  for (int i = 0; i < proxy_emitters.size(); ++i)
  {
    if (proxy_emitters[i].getId() == -1)
      continue;
    for (int j = i + 1; j < proxy_emitters.size(); ++j)
    {
      G_ASSERT(proxy_emitters[i].getId() != proxy_emitters[j].getId());
    }
  }
}

static void init_emitters()
{
  // this function called after tires params changed, so we need to reset emitters indices in vertex buffer

  emitters.resize(maxTrackCount);
  free_emitters.resize(maxTrackCount);
  used_emitters.reserve(maxTrackCount);
  updated_emitters.reserve(maxTrackCount);
  updated_regions.reserve(maxTrackCount * 2);
  proxy_emitters.reserve(maxTrackCount * 2);

  for (int i = 0; i < emitters.size(); ++i)
  {
    emitters[i].init(i);
    free_emitters[i] = i;
  }
}

static void close_emitters()
{
  used_emitters.lock();
  clear_and_shrink(emitters);
  clear_and_shrink(free_emitters);
  used_emitters.clear();
  used_emitters.shrink_to_fit();
  clear_and_shrink(updated_emitters);
  clear_and_shrink(updated_regions);
  used_emitters.unlock();
  used_proxy_emitters.lock();
  clear_and_shrink(free_proxy_emitters);
  used_proxy_emitters.clear();
  used_proxy_emitters.shrink_to_fit();
  clear_and_shrink(proxy_emitters);
  used_proxy_emitters.unlock();
}

static void erase_emitters()
{
  updated_emitters.clear();
  updated_regions.clear();
  for (int i = 0; i < emitters.size(); i++)
    emitters[i].clearTracks();
  used_emitters.lock();
  for (int i = 0; i < proxy_emitters.size(); i++)
  {
    if (proxy_emitters[i].getId() >= 0)
      free_emitters.push_back(proxy_emitters[i].getId());
    proxy_emitters[i].loose();
  }
  used_emitters.unlock();
}
static bool get_free_ownership(int pid)
{
  if (free_emitters.size() <= 0)
    return false;
  used_emitters.lock();
  int emitterId = free_emitters.back();
  free_emitters.pop_back();
  used_emitters.push_back(emitterId);
  used_emitters.unlock();
  proxy_emitters[pid].own(emitterId);
  return true;
}
// create new track emitter
int create_emitter(float width, float /*texture_length_factor*/, float min_time, float prio_scale_factor, int track_type_no)
{
  if (!initialized)
    return -1;
  if (track_type_no < 0 || track_type_no >= trackTypes.size())
    return -1;
  used_proxy_emitters.lock();
  if (used_proxy_emitters.size() >= MAX_ALLOWED_TRACKS)
  {
    used_proxy_emitters.unlock();
    return -1;
  }
  int id;
  if (free_proxy_emitters.size())
  {
    id = free_proxy_emitters.back();
    free_proxy_emitters.pop_back();
  }
  else
    id = append_items(proxy_emitters, 1);
  used_proxy_emitters.push_back(id);
  proxy_emitters[id].init(width, min_time, prio_scale_factor, track_type_no);
  if (!get_free_ownership(id))
  {
    have_orphan_proxy |= true;
  }
  used_proxy_emitters.unlock();
  return id;
}

void emit(int pemitterId, const Point3 &norm, const Point3 &pos, const Point3 &movedir, real opacity, int tex_id,
  float additional_width, float omnidirectional_tex_blend)
{
  if (pemitterId < 0 || pemitterId >= proxy_emitters.size())
    return;
  proxy_emitters[pemitterId].setPos(pos);
  ProxyEmitter &pr = proxy_emitters[pemitterId];
  if (pr.getId() >= 0 && pr.getId() < emitters.size())
    emitters[pr.getId()].emit(norm, pos, movedir, opacity, tex_id, additional_width, omnidirectional_tex_blend);
}

void finalize_track(int pemitterId)
{
  if (pemitterId < 0 || pemitterId >= proxy_emitters.size())
    return;
  if (proxy_emitters[pemitterId].getId() >= 0)
    emitters[proxy_emitters[pemitterId].getId()].finalize();
}

// delete track emitter.
void delete_emitter(int pemitterId)
{
  if (pemitterId < 0 || pemitterId >= proxy_emitters.size())
    return;
  used_emitters.lock();
  int emitterId = proxy_emitters[pemitterId].getId();
  proxy_emitters[pemitterId].loose();
  if (emitterId >= 0 && emitterId < emitters.size())
  {
    emitters[emitterId].finalize();
    free_emitters.push_back(emitterId);
    bool deleted = false;
    for (int i = 0; i < used_emitters.size(); ++i)
    {
      if (used_emitters[i] == emitterId)
      {
        erase_items(used_emitters, i, 1);
        deleted = true;
        break;
      }
    }
    G_ASSERT(deleted);
  }
#if DAGOR_DBGLEVEL > 0
  for (int i = 0; i < free_proxy_emitters.size(); ++i)
  {
    G_ASSERT(free_proxy_emitters[i] != pemitterId);
  }
#endif
  used_emitters.unlock();
  used_proxy_emitters.lock();
  free_proxy_emitters.push_back(pemitterId);
  bool found = false;
  for (int i = 0; i < used_proxy_emitters.size(); ++i)
  {
    if (used_proxy_emitters[i] == pemitterId)
    {
      erase_items(used_proxy_emitters, i, 1);
      found = true;
      break;
    }
  }
  G_ASSERT(found);
#if DAGOR_DBGLEVEL > 0
  for (int i = 0; i < used_proxy_emitters.size(); ++i)
  {
    G_ASSERT(used_proxy_emitters[i] != pemitterId);
  }
#endif
  used_proxy_emitters.unlock();
}
struct SortByYAsc
{
  inline bool operator()(const IPoint2 &a, const IPoint2 &b) const { return a.y < b.y; }
};

static void beforeRenderProxy(float dt, const Point3 &origin)
{
  if (have_orphan_proxy && used_proxy_emitters.size() <= emitters.size())
  {
#if DAGOR_DBGLEVEL > 0
    used_proxy_emitters.lock();
    for (int i = 0; i < used_proxy_emitters.size(); ++i)
    {
      int pid = used_proxy_emitters[i];
      if (proxy_emitters[pid].getId() < 0)
      {
        G_VERIFY(get_free_ownership(pid));
      }
    }
    used_proxy_emitters.unlock();
#endif
    have_orphan_proxy = false;
    return;
  }
  StaticTab<IPoint2, MAX_ALLOWED_TRACKS> potentialFree;
  G_ASSERT(potentialFree.capacity() >= used_proxy_emitters.size());
  int alreadyFree = 0;
  used_proxy_emitters.lock();
  for (int i = 0; i < used_proxy_emitters.size(); ++i)
  {
    uint16_t pid = used_proxy_emitters[i];
    if (proxy_emitters[pid].getId() >= 0)
    {
      if (proxy_emitters[pid].update(dt))
        potentialFree.push_back(IPoint2(pid, proxy_emitters[pid].getPriority(origin)));
    }
    else
    {
      potentialFree.push_back(IPoint2(pid, proxy_emitters[pid].getPriority(origin)));
      alreadyFree++;
    }
  }
  used_proxy_emitters.unlock();
  if (alreadyFree <= 0)
  {
    return;
  }
  stlsort::sort(potentialFree.data(), potentialFree.data() + potentialFree.size(), SortByYAsc());

  have_orphan_proxy = false;
  int lastOwned = potentialFree.size() - 1;
  for (; lastOwned >= 0; --lastOwned)
    if (proxy_emitters[potentialFree[lastOwned].x].getId() >= 0)
      break;
    else
      have_orphan_proxy = true;

  if (lastOwned < 0)
    return;
  for (int i = 0; i < potentialFree.size() && i < lastOwned && alreadyFree >= 0; ++i)
  {
    int pid = potentialFree[i].x;
    if (proxy_emitters[pid].getId() >= 0)
    {
      // skip.
    }
    else
    {
      alreadyFree--;
      if (get_free_ownership(pid))
        continue;
      int eId = proxy_emitters[potentialFree[lastOwned].x].getId();
      emitters[eId].finalize();
      proxy_emitters[potentialFree[lastOwned].x].loose();
      proxy_emitters[pid].own(eId);
      for (; lastOwned >= i + 1; --lastOwned)
        if (proxy_emitters[potentialFree[lastOwned].x].getId() >= 0)
          break;
        else
          have_orphan_proxy = true;
    }
  }
}

bool is_initialized() { return initialized; }

} // namespace tire_tracks
