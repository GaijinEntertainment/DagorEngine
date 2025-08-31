// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vehiclePhys/tireTracks.h>
#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_dynShaderBuf.h>
#include <3d/dag_ringDynBuf.h>
#include <generic/dag_tabUtils.h>
#include <generic/dag_range.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <3d/dag_materialData.h>
#include <shaders/dag_shaderMesh.h>
#include <gameRes/dag_gameResources.h>

namespace tires0
{
#define SHADER_NAME "tires_default"

typedef Range<real> RealRange;

static bool initialized = false;

static DynShaderMeshBuf buffer;
static RingDynamicVB meshVB;
static RingDynamicIB meshIB;

// global vertex counter
static int currentVertexCount = 0;
static bool pack_normal_into_color = false;

static __forceinline uint8_t real_to_uchar(real a)
{
  if (a <= 0)
    return 0;
  if (a >= 1)
    return 255;
  return real2int(a * 255);
}

//*************************************************
// struct TireTrackVertex
//*************************************************
struct TireTrackVertex
{
  Point3 pos;
  E3DCOLOR color;
  Point2 tex;
};

const static uint16_t defIndex[6] = {1, 3, 0, 0, 3, 2};

//*************************************************
// class TireTrack
//*************************************************
class TireTrack
{
public:
  bool locked;
  bool isVisible;
  bool used;

  BBox3 bbox;
  Point3 c;
  real r;

  TireTrack() : vertices(midmem), indices(midmem), usedIndices(0) { reset(); }

  // render track to buffer
  void render()
  {
    if (vertices.size() < 4)
      return;

    buffer.addFaces(vertices.data(), vertices.size(), indices.data(), usedIndices / 3);
  }

  // reset for future usage
  void reset()
  {
    isVisible = false;
    locked = false;
    used = false;
    usedIndices = 0;
    currentVertexCount -= vertices.size();
    vertices.clear();
    bbox.setempty();
    c = Point3(0, 0, 0);
    r = 0.0f;
  }

  // return vertex count
  inline int getVertCount() const { return vertices.size(); }

  // set last vertices to another values
  void replaceLastVert(const TireTrackVertex *tv)
  {
    int count = vertices.size();
    G_ASSERT(count >= 2);
    vertices[count - 2] = tv[0];
    vertices[count - 1] = tv[1];
    updateBBox(tv);
  }

  // add vertices & calculate indices
  void addVertices(const TireTrackVertex *tv)
  {
    append_items(vertices, 2, tv);

    currentVertexCount += 2;

    // if track has > 2 vertices, set indices
    if (vertices.size() > 2)
    {
      if (usedIndices == indices.size())
      {
        int indexBase = vertices.size() - 4;
        for (int i = 0; i < 6; i++)
        {
          indices.push_back(indexBase + defIndex[i]);
        }
      }

      usedIndices += 6;
    }

    updateBBox(tv);
  }

private:
  Tab<TireTrackVertex> vertices; // vertex data
  Tab<uint16_t> indices;         // index data
  int usedIndices;

  inline void updateBBox(const TireTrackVertex *tv)
  {
    bbox += tv[0].pos;
    bbox += tv[1].pos;
    c = (bbox.lim[0] + bbox.lim[1]) / 2;
    r = (bbox.lim[0] - c).length();
  }
};

//*************************************************
// constants
//*************************************************
// size of vertex buffer for all tracks (in vertices)
static int vertexCount = 8000;

// maximum count of tracks
static int maxTrackCount = 100;

// maximum visible distance for tracks (meters)
static real maxVisibleDist = 100.0f;

// minimal & maximal side car speed, when wheels make tracks
static RealRange carsideSpeed(0.005f, 0.010f);

// distance between tracks & clipmesh
static real trackSlope = 0.06f;

// track width (in meters)
static real trackWidth = 0.35f;

// minimal & maximal track segment length (in meters)
static RealRange segmentLength(0.2f, 0.5f);

// minimal horizontal angle (grad), when tracks must be split
static real hAngleLimit = DEG_TO_RAD * 30.0f;
static real vAngleLimit = DEG_TO_RAD * 20.0f;

// texture for rendering
static int frameCount = 0;
static real frameSpacing = 0.002f;

// time delay in seconds. after this timer track will be finalized
// (or car must countinue this track to avoid this)
static real finalizeTime = 0.5f;

// get new free track from track list & lock it
static TireTrack *lockNewTrack();

// free one of unused tracks. return true, if success free track
static bool freeUnusedTrack();

// free one or more unused tracks. return true, when free 2 or more vertices
static bool freeUnusedTrackVertices();

// place track to unused list
static void unuseTrack(TireTrack *track);

static const Range<int> alphaRange(190, 255);

//*************************************************
// class TrackEmitter
//*************************************************
class TrackEmitter : public ITrackEmitter
{
private:
  TireTrack *current;
  Point3 lastPos;      // previous emit pos
  Point3 segmentStart; // point of segment start
  Point3 lastNorm;

  int lastTex;   // last emitted texture index
  real totalLen; // total track length

  real waitTimer; // time since last spawn event

  Color3 lastLt; // last lighting
public:
  TrackEmitter() : current(NULL), lastLt(255, 255, 255) {}


  virtual ~TrackEmitter() { finalize(); }

  // emit new track (start new or continue existing)
  virtual void emit(const Point3 &norm, const Point3 &pos, const Point3 &movedir, const Point3 &wdir, const Color3 &lt,
    real side_speed, int tex_id)
  {
    lastNorm = normalize(norm);
    lastLt = lt;
    if (lastNorm * Point3(0, 1, 0) < 0)
      lastNorm = -lastNorm;

    if (side_speed < carsideSpeed.getMin())
    {
      finalize();
      return;
    }

    carsideSpeed.rangeIt(side_speed);

    int alpha = convertRangedValue(side_speed, carsideSpeed, alphaRange);

    bool addNewSegment = false;

    // texture changed - finalize last track
    if (lastTex != tex_id)
      finalize();
    lastTex = tex_id;

    if (!current)
    {
      // check for spawn new track
      if (!checkFreeVB())
        return;

      current = lockNewTrack();
      if (!current)
        return;

      lastPos = pos;
      segmentStart = pos;
      totalLen = 0;
      lastTex = tex_id;
      addNewSegment = true;
      alpha = 0;
    }

    Point3 len = pos - segmentStart;
    real segmentLen = len.length();

    Point3 segmentDir;
    if (segmentLen == 0)
    {
      segmentDir = normalize(movedir);
    }
    else
    {
      segmentDir = len;
    }

    // check for new segment
    if (!addNewSegment)
    {
      if (segmentLen > segmentLength.getMax())
      {
        // check length
        addNewSegment = true;
        segmentLen = segmentLength.getMax();
        endSegment(norm, pos, movedir, len, segmentLen, segmentDir);
      }
      else
      {
        // check for critical angles
      }
    }

    // calculate vertex params & fill vertices
    if (!addVertices(addNewSegment, pos, segmentDir, alpha, totalLen + segmentLen))
    {
      finalize();
    }
  }

  // finalize track
  virtual void finalize()
  {
    //      debug("finalize: %X", current);
    if (current)
    {
      // try to fadeout track
      Point3 len = lastPos - segmentStart;
      real prevLen = len.length();

      if (current->getVertCount() <= 4)
      {
        // no visible track segments generated - dismiss track
        current->locked = false;
        unuseTrack(current);
      }
      else
      {
        // add last vertices
        len.normalize();
        addVertices(true, len * segmentLength.getMin() + lastPos, len, 0, totalLen + prevLen + segmentLength.getMin());
        current->locked = false;
      }

      current = NULL;
      waitTimer = 0.0f;
    }
  }

  void act(real dt)
  {
    // if no spawn events in some time, finalize track
    if (current)
    {
      waitTimer += dt;
      if (waitTimer >= finalizeTime)
        finalize();
    }
  }

private:
  // check for vbuffer overrun. return false, if cannot free nessesary vertices
  bool checkFreeVB()
  {
    // check for vertex buffer overrun
    if (currentVertexCount + 2 > vertexCount)
    {
      // free one of unused tracks. exit, if failed
      if (!freeUnusedTrackVertices())
        return false;
    }

    return true;
  }

  // add new vertices
  bool addVertices(bool is_new_segment, const Point3 &pos, const Point3 &segment_dir, int alpha, real tuv)
  {
    if ((is_new_segment || current->getVertCount() < 4) && !checkFreeVB())
      return false;

    const Point3 b = normalize(lastNorm % -segment_dir);
    const Point3 slope = lastNorm * trackSlope;

    // real width = trackWidth / 2 + (1.0 - fabsf(movedir * segment_dir)) * (segmentLength.getMin() / 2);
    real width = trackWidth / 2;

    Point3 pwidth = b * width;

    TireTrackVertex tv[2];
    tv[0].pos = pos - pwidth + slope;
    tv[1].pos = pos + pwidth + slope;

    //      debug("segment=%d len=%.4f w=%.4f %.4f " FMT_P3 " " FMT_P3 "", addNewSegment, segmentLen, width,
    //        (tv[1].pos - tv[0].pos).length(), P3D(normalize(norm)), P3D(normalize(segmentDir)));

    //      tv[0].color = E3DCOLOR(255, 0, 0, 255);
    //      tv[1].color = E3DCOLOR(0, 255, 0, 255);

    //        tv[0].color = E3DCOLOR(255, 255, 255, 255);
    //        tv[1].color = E3DCOLOR(255, 255, 255, 255);
    //
    //      if (!alpha)
    //      {
    //        tv[0].color = E3DCOLOR(255, 255, 255, 255);
    //        tv[1].color = E3DCOLOR(255, 255, 255, 255);
    //        alpha = 255;
    //      }
    if (pack_normal_into_color)
    {
      tv[0].color.r = tv[1].color.r = real_to_uchar(lastNorm.x * 0.5f + 0.5f);
      tv[0].color.g = tv[1].color.g = real_to_uchar(lastNorm.y * 0.5f + 0.5f);
      tv[0].color.b = tv[1].color.b = real_to_uchar(lastNorm.z * 0.5f + 0.5f);
    }
    else
      tv[0].color = tv[1].color = e3dcolor(lastLt);

    tv[0].color.a = tv[1].color.a = int(alpha);
    tv[0].tex = Point2((real)lastTex / (real)frameCount + frameSpacing, tuv);
    tv[1].tex = Point2((real)(lastTex + 1) / (real)frameCount - frameSpacing, tuv);

    if (is_new_segment || current->getVertCount() < 4)
    {
      current->addVertices(tv);
    }
    else
    {
      current->replaceLastVert(tv);
    }

    lastPos = pos;
    waitTimer = 0.0f;
    return true;
  }

  // finalize segment. update variables
  void endSegment(const Point3 &norm, const Point3 &pos, const Point3 &movedir, Point3 &len, real &segmentLen, Point3 &segmentDir)
  {
    segmentStart = pos;
    totalLen += segmentLen;

    len = pos - lastPos;
    segmentLen = len.length();

    if (segmentLen == 0)
    {
      segmentDir = normalize(movedir);
    }
    else
    {
      segmentDir = len;
    }
  }
};

//*************************************************
// internal parameters
//*************************************************
// shaders
static Ptr<ShaderMaterial> currentMaterial = NULL;
static Ptr<ShaderElement> currentElem = NULL;

// track table
static Tab<TireTrack *> tracks(midmem_ptr());
static Tab<TireTrack *> freeTracks(midmem_ptr());

// get new free track from track list
static TireTrack *lockNewTrack()
{
  if (!freeTracks.size() && (tracks.size() >= maxTrackCount) && !freeUnusedTrack())
  {
    //      debug("cannot lockNewTrack()");
    //      for (int i = 0; i < tracks.size(); i++)
    //      {
    //        debug("%d: u=%d v=%d l=%d", i, tracks[i]->used, tracks[i]->isVisible, tracks[i]->locked);
    //      }
    return NULL;
  }

  TireTrack *newTrack = NULL;
  if (!freeTracks.size())
  {
    newTrack = new (midmem) TireTrack();
    tracks.push_back(newTrack);
  }
  else
  {
    newTrack = freeTracks.back();
    freeTracks.pop_back();
  }

  newTrack->isVisible = true;
  newTrack->locked = true;
  newTrack->used = true;
  return newTrack;
}

// place track to unused list
static inline void unuseTrack(TireTrack *track)
{
  G_ASSERT(track);
  G_ASSERT(!track->locked);

  track->reset();
  freeTracks.push_back(track);
}

// free one of unused tracks. return true, if success free track
static bool freeUnusedTrack()
{
  for (int i = tracks.size() - 1; i >= 0; i--)
  {
    TireTrack *track = tracks[i];
    if (track->used && !track->locked && !track->isVisible)
    {
      unuseTrack(track);
      return true;
    }
  }

  return false;
}

// free one or more unused tracks. return true, when free 2 or more vertices
static bool freeUnusedTrackVertices()
{
  int vertCount = 0;
  for (int i = tracks.size() - 1; i >= 0; i--)
  {
    TireTrack *track = tracks[i];
    if (track->used && !track->locked && !track->isVisible)
    {
      vertCount += track->getVertCount();
      unuseTrack(track);

      if (vertCount >= 2)
        return true;
    }
  }
  return false;
}

//*************************************************
// emitters
//*************************************************
static Tab<TrackEmitter *> emitters(midmem_ptr());

// create new track emitter
ITrackEmitter *create_emitter()
{
  if (!initialized)
    return NULL;

  TrackEmitter *e = new (midmem) TrackEmitter();
  emitters.push_back(e);

  return e;
}


// delete track emitter.
void delete_emitter(ITrackEmitter *e) { tabutils::deleteElement(emitters, (TrackEmitter *)e); }

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

// init system. load settings from blk-file
void init(const char *blk_file, bool pack_normal_into_color_)
{
  G_ASSERT(!initialized);
  initialized = true;
  pack_normal_into_color = pack_normal_into_color_;

  DataBlock blk(blk_file);

  // load spawn params
  DataBlock *params = blk.getBlockByName("SpawnParams");
  if (params)
  {
    loadRange(params, "carside_speed", carsideSpeed);

    finalizeTime = params->getReal("finalizeTime", finalizeTime);
  }

  // load visual params
  params = blk.getBlockByName("VisualParams");
  if (params)
  {
    trackSlope = params->getReal("slope", trackSlope);
    vertexCount = params->getInt("vertexCount", vertexCount);
    if (vertexCount <= 32)
      DAG_FATAL("Vertex count (%d) for tire tracks must be > 32!", vertexCount);

    DataBlock *angles = params->getBlockByName("critical_angle");
    if (angles)
    {
      hAngleLimit = params->getReal("horizontal", hAngleLimit);
      vAngleLimit = params->getReal("vertical", vAngleLimit);
    }

    trackWidth = params->getReal("width", trackWidth);

    loadRange(params, "length", segmentLength);

    maxTrackCount = params->getInt("maxTrackCount", maxTrackCount);
    maxVisibleDist = params->getReal("maxVisibleDist", maxVisibleDist);
  }

  // load texture params
  const char *texName = NULL;

  params = blk.getBlockByName("Texture");
  if (params)
  {
    texName = params->getStr("name", NULL);

    frameCount = params->getInt("frames", frameCount);
    frameSpacing = params->getReal("spacing", frameSpacing);
  }

  // init channels
  Tab<CompiledShaderChannelId> channels(tmpmem_ptr());
  append_items(channels, 3);

  // pos
  channels[0].t = SCTYPE_FLOAT3;
  channels[0].vbu = SCUSAGE_POS;
  channels[0].vbui = 0;

  // color
  channels[1].t = SCTYPE_E3DCOLOR;
  channels[1].vbu = SCUSAGE_VCOL;
  channels[1].vbui = 0;

  // tex UV
  channels[2].t = SCTYPE_FLOAT2;
  channels[2].vbu = SCUSAGE_TC;
  channels[2].vbui = 0;

  // create shader buffer
  G_ASSERT(vertexCount < 65536);
  meshVB.init(vertexCount, dynrender::getStride(channels.data(), channels.size()), "tireTracksVB");
  meshIB.init((vertexCount - 2) * 3, "tireTracksIB");
  meshVB.addRef();
  meshIB.addRef();
  buffer.setRingBuf(&meshVB, &meshIB);
  buffer.init(channels, vertexCount, vertexCount - 2);

  // create material & shader
  Ptr<MaterialData> matNull = new MaterialData;
  matNull->className = SHADER_NAME;

  if (texName != NULL)
  {
    matNull->mtex[0] = ::get_tex_gameres(texName);
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = d3d::AddressMode::Wrap;

    if (!set_texture_separate_sampler(matNull->mtex[0], smpInfo))
      logerr("Can't set a sampler for tiretrack texture - '%s'", texName);
  }

  currentMaterial = new_shader_material(*matNull);

  if (currentMaterial == NULL)
  {
    DAG_FATAL("StdGuiRender - shader '%s' not found!", SHADER_NAME);
  }

  if (!currentMaterial->checkChannels(channels.data(), channels.size()))
  {
    DAG_FATAL("TireTracks::init - invalid channels for shader '%s'!", (char *)currentMaterial->getShaderClassName());
  }

  currentElem = currentMaterial->make_elem();

  // set shader to buffer
  buffer.setCurrentShader(currentElem);

  currentVertexCount = 0;
}

// release system
void release()
{
  G_ASSERT(initialized);
  initialized = false;

  tabutils::deleteAll(emitters);

  clear_and_shrink(freeTracks);
  tabutils::deleteAll(tracks);

  buffer.close();
  meshVB.close();
  meshIB.close();
  currentMaterial = NULL;
  currentElem = NULL;
  currentVertexCount = 0;
}

// remove all tire tracks from screen
void clear(bool completeClear)
{
  // finish drawing tracks
  for (int i = 0; i < emitters.size(); i++)
    emitters[i]->finalize();

  // remove tracks from render
  for (int i = 0; i < tracks.size(); i++)
    if (tracks[i]->used)
      unuseTrack(tracks[i]);
  if (completeClear)
  {
    freeTracks.clear();
    clear_all_ptr_items(tracks);
  }
}

//*************************************************
// act/render
//*************************************************
void act(real dt)
{
  if (!tracks.size())
    return;
  for (int i = 0; i < emitters.size(); i++)
    emitters[i]->act(dt);
}

// before render tires
void before_render(const Frustum &frustum, const Point3 &pos)
{
  // check visibility
  for (auto track : tracks)
  {
    if (track->used && !track->locked)
    {
      real dist = (track->c - pos).length() - track->r;
      if (dist >= maxVisibleDist)
        track->isVisible = false;
      else
        track->isVisible = frustum.testSphereB(track->c, track->r);
    }
  }
}

// render tires
void render()
{
  if (!tracks.size())
    return;

  // render
  for (TireTrack *track : tracks)
  {
    if (track->used && track->isVisible)
      track->render();
  }

  buffer.flush();
}

void reset_per_frame_dynbuf_pos()
{
  int q1 = meshVB.getProcessedCount();
  int q2 = meshIB.getProcessedCount();
  if (q1 > meshVB.bufSize())
    debug("insufficient tiretrack meshVB size: %d used, %d available", q1, meshVB.bufSize());
  if (q2 > meshIB.bufSize())
    debug("insufficient tiretrack meshIB size: %d used, %d available", q2, meshIB.bufSize());
  meshVB.resetCounters();
  meshIB.resetCounters();
}
} // namespace tires0
