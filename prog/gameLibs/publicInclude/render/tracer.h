//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_carray.h>
#include <shaders/dag_dynSceneRes.h>
#include <math/dag_Point4.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include "tracerConsts.hlsli"
#include <3d/dag_drv3dConsts.h>
#include <3d/dag_resPtr.h>
#include <EASTL/functional.h>
#include <3d/dag_ringDynBuf.h>
#include <osApiWrappers/dag_critSec.h>

#define MAX_FX_TRACERS 2048

struct Frustum;
class AcesEffect;
class ComputeShaderElement;
class RingDynamicSB;
struct TrailType;

struct TracerSegment
{
  Point3 pos;
  int partNum;
  Point3 startPos;
  Point3 endPos;
};

class TracerManager;

class Tracer
{
  friend TracerManager;

public:
  Tracer() { memset(this, 0, sizeof(*this)); }
  Tracer(TracerManager *owner, uint32_t in_idx, const Point3 &start_pos, const Point3 &in_speed, int type_no, unsigned int mesh_no,
    float tail_particle_half_size_from, float tail_particle_half_size_to, float tail_deviation, const Color4 &tail_color,
    float in_caliber, float spawn_time);

  void setMinLen(float value);
  void setStartPos(const Point3 &value);
  void setPos(const Point3 &in_pos, const Point3 &in_dir, float time_left, float angle_eps = cosf(0.6f * PI / 180.0f));
  void decay(const Point3 &in_pos);
  float getCurLifeTime() const;

protected:
  void appendToSegmentBuffer(uint32_t segment_id);
  void appendToTracerBuffer(uint32_t tracer_id);
  void updateSegments();

  TracerManager *owner;

  union
  {
    float timeLeft;
    unsigned itl;
  };
  float spawnTime;
  bool shouldDecay;
  bool delayedDecay;

  Point3 startPos;
  float minLen;
  float speed;
  Point3 pos;
  Point3 dir;
  float decayT;
  int typeNo;
  uint32_t idx;
  unsigned int meshNo;
  float tailParticleHalfSizeFrom;
  float tailParticleHalfSizeTo;
  float tailDeviation;
  Color4 tailColor;

  carray<TracerSegment, MAX_FX_SEGMENTS> segments;
  int segmentsNum;
  int partCount;
  int firstSegment;
  int lastSegment;
  bool wasPrepared;
};


class TracerManager : public cpujobs::IJob
{
  friend Tracer;

public:
  enum HeadPrimType
  {
    HEAD_PRIM_DIR = 0,
    HEAD_PRIM_CAPS = 1
  };

  TracerManager(const DataBlock *blk);
  ~TracerManager();

  void update(float dt);
  void beforeRender(const Frustum *frustum_);
  void renderTrans(bool heads = true, bool trails = true, const float *hk = NULL, HeadPrimType head_prim_type = HEAD_PRIM_DIR);
  void finishPreparingIfNecessary();

  Tracer *createTracer(const Point3 &start_pos, const Point3 &speed, int tracerType, int trailType, float caliber, bool force = false,
    bool forceNoTrail = false, float cur_life_time = 0.0f);

  int getTracerTypeNoByName(const char *name);
  int getTrailTypeNoByName(const char *name);
  Color4 getTraceColor(int no);

  const Point2 &getEyeDistBlend() const;
  void setEyeDistBlend(const Point2 &value);

  void clear();
  void reset();

  float getTrailSizeHeuristic(int trailType);
  void setMTMode(bool mt) { isMTMode = mt; }

protected:
  class DrawBuffer
  {
  public:
    DrawBuffer();

    void createGPU(int struct_size, int cmd_size, int elements, unsigned flags, unsigned texfmt, const char *name);
    void createData(int struct_size, int elements);
    void create(bool gpu, int struct_size, int cmd_size, int elements, unsigned flags, unsigned texfmt, const char *name);
    void close();
    int lock(uint32_t ofs_bytes, uint32_t size_bytes, void **p, int flags);
    void unlock();
    void append(uint32_t id, dag::ConstSpan<uint8_t> elem);
    void process(ComputeShaderElement *cs, int commands_array_const_no, int commands_count_const_no, int fx_create_cmd,
      int element_count);

    inline Sbuffer *getSbuffer() const { return buf.get(); }
    inline uint8_t *getData() { return data.data(); }
    inline const uint8_t *getData() const { return data.data(); }

  private:
    int structSize;
    int cmdSize;
    BufPtr buf;
    Tab<uint8_t> data;
    Tab<uint8_t> cmds;
    int cmd;
  };

  enum
  {
    FX_INSTANCING_CONSTS = 0,
    FX_INSTANCING_SBUF = 1
  };

  void doJob();
  void releaseJob() {}

  void initTrails();
  void renderTrails();
  void initHeads();
  void renderHeads();
  void releaseRes();

  uint32_t numTracers, numVisibleTracers, maxTracerNo;
  carray<Tracer, MAX_FX_TRACERS> tracers;
  carray<uint32_t, ((MAX_FX_TRACERS + 31) >> 5)> trExist;
  carray<uint32_t, ((MAX_FX_TRACERS + 31) >> 5)> trCulledOrNotInitialized;
  const Frustum *mFrustum;

  ShaderMaterial *headMat;
  dynrender::RElem headRendElem;
  BufPtr headVb;
  BufPtr headIb;
  DrawBuffer tracerTypeBuffer;
  carray<Point4, FX_HEAD_MAXIUM_REGISTERS> headData;

  int headShapeParamsVarId;
  int tracerBeamTimeVarId;
  int tracerPrimTypeVarId;

  Point4 headShapeParams;
  float headVibratingRate;
  Point2 eyeDistBlend;
  float headFadeTime;
  Point4 headSpeedScale;
  int tracerBlockId;
  int numTailParticles;
  uint32_t numTailLods;
  float tailFartherLodDist;
  float tailOpacity;
  float tailParticleExtension;
  float tailFadeInRcp;
  float tailLengthMeters;
  float tailNoisePeriodMeters;
  float tailDecayToDelete;
  ComputeShaderElement *createCmdCs;

  bool computeSupported;
  bool multiDrawIndirectSupported;
  bool instanceIdSupported;
  DrawBuffer tailIndirect;
  eastl::unique_ptr<RingDynamicSB> tailIndirectRingBuffer;
  volatile int ringBufferPos = 0;

  int commandsCountConstNo;
  int commandsArrayConstNo;

  DrawBuffer tracerBuffer;
  DrawBuffer tracerDynamicBuffer;
  DrawBuffer segmentBuffer;
  uint32_t segmentDrawCount;
  Tab<uint32_t> lodOffsList;
  float curTime;
  bool preparing;
  bool tracerLimitExceedMsg;

  SharedTexHolder tailTex;
  ShaderMaterial *tailMat;
  dynrender::RElem tailRendElem;
  BufPtr tailVb;
  BufPtr tailIb;
  BufPtr tailInstancesIds;
  VDECL tailInstancingVdecl = BAD_VDECL;
  DrawBuffer tailParticles;

  bool isMTMode = false;
  WinCritSec mutexCS;
  friend struct TracerMgrThreadGuard;

public:
  struct TracerType
  {
    String name;
    String fxName;
    float halfSize;
    float radius;
    Point4 distScale;
    E3DCOLOR startColor1;
    E3DCOLOR startColor2;
    float startTime;
    E3DCOLOR color1;
    E3DCOLOR color2;
    float hdrK;
    float minPixelSize;
    bool beam;
  };
  Tab<TracerType> tracerTypes;
  Tab<TrailType> trailTypes;
};
