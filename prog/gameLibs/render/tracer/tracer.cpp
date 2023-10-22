#if !_TARGET_PC
#undef _DEBUG_TAB_
#endif
#include <render/tracer.h>
#include <shaders/dag_shaderBlock.h>
#include <gameRes/dag_gameResources.h>
#include <math/dag_noise.h>
#include <math/random/dag_random.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_render.h>
#include <3d/dag_drv3d_platform.h>
#include <3d/dag_materialData.h>
#include <3d/dag_drv3d.h>
#include <debug/dag_debug.h>
#include <util/dag_oaHashNameMap.h>
#include <supp/dag_prefetch.h>
#include <math/dag_frustum.h>
#include <math/dag_plane3.h>
#include <util/dag_threadPool.h>
#include <util/dag_lag.h>
#include <image/dag_texPixel.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_computeShaders.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_mathUtils.h>
#include <math/dag_hlsl_floatx.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_convar.h>
#include <osApiWrappers/dag_miscApi.h>
#include <3d/dag_ringDynBuf.h>
#include "tracer.hlsli"

struct TailVertex
{
  int16_t vertexId;
  int16_t divisor;
};

struct HeadVertex
{
  int16_t headId;
  int16_t posId;
};

struct TailVertexPerlin : TailVertex
{
  Point3 perlin;
};


struct TrailType
{
  String name;
  float particleHalfSizeFrom;
  float particleHalfSizeTo;
  float deviation;
  E3DCOLOR color;
};

#define TRACER_UNITIALIZED          0x501502f9
#define LOGLEVEL_DEBUG              _MAKE4C('[R] ')
#define RESERVE_FOR_PLAYER_FRACTION 0.15f

CONSOLE_BOOL_VAL("render", oldTracers, false);


static inline float point_to_segment_distance(const Point3 &point, const Point3 &segment_a, const Point3 &segment_b)
{
  Point3 diff = point - segment_a;
  Point3 dir = segment_b - segment_a;
  float t = diff * dir;
  if (t > 0.f)
  {
    float sqrLen = dir.lengthSq();
    if (t >= sqrLen)
    {
      t = 1.f;
      diff -= dir;
    }
    else
    {
      t /= sqrLen;
      diff -= t * dir;
    }
  }
  return diff.length();
}

struct TracerMgrThreadGuard
{
  TracerManager *locked = nullptr;
  TracerMgrThreadGuard(TracerManager *mgr)
  {
    if (mgr->isMTMode)
    {
      locked = mgr;
      mgr->mutexCS.lock();
    }
    else
      G_ASSERT(is_main_thread());
  }
  void unlock()
  {
    if (locked)
    {
      locked->mutexCS.unlock();
      locked = nullptr;
    }
  }
  ~TracerMgrThreadGuard() { unlock(); }
};


Tracer::Tracer(TracerManager *in_owner, uint32_t in_idx, const Point3 &start_pos, const Point3 &in_speed, int type_no,
  unsigned int mesh_no, float tail_particle_half_size_from, float tail_particle_half_size_to, float tail_deviation,
  const Color4 &tail_color, float in_caliber, float spawn_time) :
  owner(in_owner),
  idx(in_idx),
  startPos(start_pos),
  pos(start_pos),
  speed(in_speed.length()),
  timeLeft(1e10f),
  spawnTime(spawn_time),
  shouldDecay(false),
  delayedDecay(false),
  decayT(0.f),
  typeNo(type_no),
  meshNo(mesh_no),
  tailParticleHalfSizeFrom(tail_particle_half_size_from),
  tailParticleHalfSizeTo(tail_particle_half_size_to),
  tailDeviation(tail_deviation),
  tailColor(tail_color),
  segments(),
  partCount(0),
  firstSegment(0),
  lastSegment(0),
  wasPrepared(false),
  minLen(0)
{
  G_UNREFERENCED(in_caliber);
  G_ASSERT(TRACER_UNITIALIZED == *(uint32_t *)&timeLeft);
  segments[0].pos = start_pos;
  segmentsNum = 1;
  dir = normalizeDef(in_speed, Point3(1.0f, 0.0f, 0.0f));

  appendToTracerBuffer(idx);
  appendToSegmentBuffer(0);
}

void Tracer::setMinLen(float value) { minLen = value; }

void Tracer::setStartPos(const Point3 &value)
{
  G_ASSERT(!owner->preparing);

  if (delayedDecay || shouldDecay)
    return;

  startPos = value;
}

void Tracer::setPos(const Point3 &in_pos, const Point3 &in_dir, float time_left, float angle_eps)
{
  G_ASSERT(!owner->preparing);

  if (delayedDecay || shouldDecay)
    return;

  G_ASSERT(segmentsNum > 0); // first segment added in constructor
  if (lengthSq(in_pos - segments[segmentsNum - 1].pos) < REAL_EPS)
    return;

  G_ASSERT(time_left < 1e9f);
  pos = in_pos;
  dir = in_dir;
  timeLeft = time_left;

  Point3 lastDir = normalize(pos - segments[segmentsNum - 1].pos);
  float angle = lastDir * dir;
  if (fabs(angle) < angle_eps)
  {
    if (segmentsNum == MAX_FX_SEGMENTS)
    {
      for (int i = 0; i < segmentsNum - 1; ++i)
        segments[i] = segments[i + 1];
      --segmentsNum;
    }
    segments[segmentsNum++].pos = pos;
    appendToSegmentBuffer(segmentsNum - 1);
  }

  updateSegments();
}

void Tracer::decay(const Point3 &in_pos)
{
  G_ASSERT(!owner->preparing);

  if (delayedDecay || shouldDecay)
    return;

  delayedDecay = true;
  if (lengthSq(in_pos - pos) < REAL_EPS)
    return;
  pos = in_pos;
  updateSegments();
}

float Tracer::getCurLifeTime() const { return owner->curTime - spawnTime; }

void Tracer::appendToSegmentBuffer(uint32_t segment_id)
{
  GPUFxSegment segment;
  segment.worldPos = pos;
  TracerMgrThreadGuard guard(owner);
  owner->segmentBuffer.append(idx * MAX_FX_SEGMENTS + segment_id, dag::ConstSpan<uint8_t>((uint8_t *)&segment, sizeof(GPUFxSegment)));
}

void Tracer::appendToTracerBuffer(uint32_t tracer_id)
{
  GPUFxTracer tracer;
  tracer.tailColor.set_rgba(tailColor);
  tracer.tailParticleHalfSize = Point2(tailParticleHalfSizeTo, tailParticleHalfSizeFrom);
  tracer.tailDeviation = tailDeviation;
  TracerMgrThreadGuard guard(owner);
  owner->tracerBuffer.append(tracer_id, dag::ConstSpan<uint8_t>((uint8_t *)&tracer, sizeof(GPUFxTracer)));
}

void Tracer::updateSegments()
{
  const float tailLengthMeters = owner->tailLengthMeters;
  const float tailLengthMetersInv = 1.0f / tailLengthMeters;
  const uint32_t numTailParticles = owner->numTailParticles;

  float trailLength = 0;
  int segmentNo = segmentsNum - 1;
  partCount = 0;
  firstSegment = 0;
  lastSegment = 0;

  for (; segmentNo >= 0 && trailLength < tailLengthMeters; --segmentNo)
  {
    TracerSegment &segment = segments[segmentNo];

    Point3 endPos;
    Point3 firstPos = segment.pos;
    if (segmentNo == segmentsNum - 1)
      endPos = pos;
    else
      endPos = segments[segmentNo + 1].pos;

    Point3 segmDir = endPos - firstPos;
    float segmentLength = segmDir.length();
    if (segmentLength < FLT_EPSILON)
    {
      segment.partNum = 0;
      continue;
    }
    segmDir /= segmentLength;
    trailLength += segmentLength;

    float cutL = max(trailLength - tailLengthMeters, 0.0f);
    if (cutL > 0)
    {
      segmentLength -= cutL;
      firstPos = endPos - segmDir * segmentLength;
    }

    segment.partNum = min<int>(ceil(segmentLength * numTailParticles * tailLengthMetersInv), numTailParticles - partCount);
    segment.startPos = firstPos;
    segment.endPos = endPos;
    if (segment.partNum > 0)
      firstSegment = segmentNo;
    if (partCount == 0 && segment.partNum > 0)
      lastSegment = segmentNo;
    partCount += segment.partNum;
  }

  for (; segmentNo >= 0; --segmentNo)
    segments[segmentNo].partNum = 0;
}


TracerManager::DrawBuffer::DrawBuffer() : structSize(0), cmdSize(0), cmd() {}

void TracerManager::DrawBuffer::createGPU(int struct_size, int cmd_size, int elements, unsigned flags, unsigned texfmt,
  const char *name)
{
  structSize = struct_size;
  cmdSize = cmd_size;
  buf = dag::create_sbuffer(struct_size, elements, flags, texfmt, name);
}

void TracerManager::DrawBuffer::createData(int struct_size, int elements)
{
  structSize = struct_size;
  data.resize(struct_size * elements);
}

void TracerManager::DrawBuffer::create(bool gpu, int struct_size, int cmd_size, int elements, unsigned flags, unsigned texfmt,
  const char *name)
{
  if (gpu)
    createGPU(struct_size, cmd_size, elements, flags, texfmt, name);
  else
    createData(struct_size, elements);
}

void TracerManager::DrawBuffer::close()
{
  buf.close();
  clear_and_shrink(data);
  clear_and_shrink(cmds);
  cmd = 0;
}

int TracerManager::DrawBuffer::lock(uint32_t ofs_bytes, uint32_t size_bytes, void **p, int flags)
{
  if (buf)
    return buf->lock(ofs_bytes, size_bytes, p, flags);
  else
    *p = data.data();
  return 1;
}

void TracerManager::DrawBuffer::unlock()
{
  if (buf)
    buf->unlock();
}

void TracerManager::DrawBuffer::append(uint32_t id, dag::ConstSpan<uint8_t> elem)
{
  G_ASSERT(structSize == elem.size());

  if (buf)
  {
    G_ASSERT(cmdSize > 0 && (cmdSize % 16) == 0 && structSize + sizeof(uint32_t) <= cmdSize);
    append_items(cmds, structSize, elem.data());
    append_items(cmds, sizeof(id), (uint8_t *)&id);
    for (int pNo = structSize + sizeof(uint32_t); pNo < cmdSize; pNo += elem_size(cmds))
      cmds.push_back(0);
    ++cmd;
  }

  if (!data.empty())
  {
    memcpy(&data[id * structSize], elem.data(), structSize);
  }
}

void TracerManager::DrawBuffer::process(ComputeShaderElement *cs, int fx_create_cmd, int element_count)
{
  if (!buf || cmds.empty())
    return;

  G_ASSERT(is_main_thread());

  if (cs)
  {
    static int fxCreateCmdVarId = get_shader_variable_id("fx_create_cmd");
    ShaderGlobal::set_int(fxCreateCmdVarId, fx_create_cmd);

    d3d::set_rwbuffer(STAGE_CS, 0, buf.get());
    uint32_t elemSize = cmdSize;
    G_ASSERT((elemSize % 16) == 0);
    int elemCount = cmds.size() / elemSize;

    const int commandSizeInConsts = (elemSize + 15) / 16;
    const int reqSize = 1 + elemCount * commandSizeInConsts;
    const int cbufferSize = d3d::set_cs_constbuffer_size(reqSize);
    uint32_t v[4] = {0};

    G_ASSERT(elemCount >= cmd);

    for (int i = max(elemCount - cmd, 0); i < elemCount; i += (cbufferSize - 1) / commandSizeInConsts)
    {
      int batch_size = min(elemCount - i, (int)(cbufferSize - 1) / commandSizeInConsts);
      v[0] = batch_size;
      d3d::set_cs_const(10, (float *)v, 1);

      d3d::set_cs_const(11, (float *)&cmds[i * elemSize], batch_size * commandSizeInConsts);
      cs->dispatch((batch_size + FX_TRACER_COMMAND_WARP_SIZE - 1) / FX_TRACER_COMMAND_WARP_SIZE, 1, 1);
    }

    cmd = 0;
    erase_items(cmds, 0, elemSize * max(elemCount, 0));

    d3d::set_cs_constbuffer_size(0);
    d3d::set_rwbuffer(STAGE_CS, 0, 0);
  }
  else
  {
    G_ASSERT(!data.empty() && element_count * structSize < data_size(data));
    GPUFxTracerDynamic *bufData;
    if (!buf->lock(0, 0, (void **)&bufData, VBLOCK_WRITEONLY | VBLOCK_DISCARD))
      return;
    memcpy(bufData, data.data(), element_count * structSize);
    buf->unlock();
  }
}


TracerManager::TracerManager(const DataBlock *blk) :
  tracerTypes(midmem),
  trailTypes(midmem),
  mFrustum(NULL),
  numTracers(0),
  numVisibleTracers(0),
  segmentDrawCount(0),
  tailOpacity(0.0f),
  createCmdCs(NULL),
  headData(),
  maxTracerNo(0),
  headSpeedScale(0, 0, 1, 1),
  preparing(false),
  curTime(0),
  tracerLimitExceedMsg(false),
  eyeDistBlend(0, 0)
{
  tracerBlockId = ShaderGlobal::getBlockId("tracer_frame");

  // Get settings.

  const DataBlock *tracersBlk = blk->getBlockByNameEx("tracers");
  headFadeTime = tracersBlk->getReal("headFadeTime", 1.f);
  headSpeedScale = tracersBlk->getPoint4("headSpeedScale", Point4(300.0f, 900.0f, 0.33f, 1.0f));

  tracerBeamTimeVarId = ::get_shader_variable_id("tracer_beam_time");
  headVibratingRate = tracersBlk->getReal("headVibratingRate", 30.0f);
  tracerPrimTypeVarId = ::get_shader_variable_id("tracer_prim_type");

  ShaderGlobal::set_color4(::get_shader_variable_id("head_noise_params"),
    Color4(tracersBlk->getReal("headNoiseFreq", 0.3f), tracersBlk->getReal("headNoiseScale", 0.03f),
      tracersBlk->getReal("headNoiseViewScale", 50.0f), tracersBlk->getReal("headBorderPixelRadius", 1.0f)));

  headShapeParamsVarId = ::get_shader_variable_id("head_shape_params");
  headShapeParams = Point4(tracersBlk->getReal("headArrowLen", 0.05f), tracersBlk->getReal("headHdrMultiplierCompatibility", 2.0f),
    tracersBlk->getReal("headFadeLenInv", 2.0f), tracersBlk->getReal("headHdrMultiplier", 25.0f));
  ShaderGlobal::set_color4(headShapeParamsVarId, Color4::xyzw(headShapeParams));

  ShaderGlobal::set_real(get_shader_variable_id("tail_lighting_directional_scale"), tracersBlk->getReal("tailDirectionalScale", 1.f));

  tailParticleExtension = tracersBlk->getReal("tailParticleExtension", 0.5f);
  tailFadeInRcp = 1.f / tracersBlk->getReal("tailFadeInDistance", 10.f);

  ShaderGlobal::set_real(::get_shader_variable_id("tailParticleExtension"), tailParticleExtension);
  ShaderGlobal::set_real(::get_shader_variable_id("tailFadeInRcp"), tailFadeInRcp);

  numTailParticles = tracersBlk->getInt("numTailParticles", 2000);
  G_ASSERT(numTailParticles > 0 && numTailParticles * 2 < FX_TRACER_BITS_PART_COUNT);
  G_STATIC_ASSERT(MAX_FX_SEGMENTS <= FX_TRACER_BITS_SEGMENT);
  G_STATIC_ASSERT(MAX_FX_TRACERS <= FX_HEAD_BITS_TRACER);
  G_STATIC_ASSERT((MAX_TRACER_TYPES - 1) <= (UINT32_MAX / FX_TRACER_BITS_SEGMENT / FX_HEAD_BITS_TRACER));
  ShaderGlobal::set_real(get_shader_variable_id("tail_num_particles"), numTailParticles);

  numTailLods = tracersBlk->getInt("numTailLods", 5);
  tailFartherLodDist = tracersBlk->getReal("tailFartherLodDist", 1000.f);
  tailLengthMeters = tracersBlk->getReal("tailLengthMeters", 1000.f);
  tailNoisePeriodMeters = tracersBlk->getReal("tailNoisePeriodMeters", 3.f);
  tailDecayToDelete = tracersBlk->getReal("tailDecayToDelete", 0.99f);

  ShaderGlobal::set_real(get_shader_variable_id("tail_length_meters"), tailLengthMeters);

  const DataBlock *tracerColorsBlk = blk->getBlockByNameEx("tracerColors");
  G_ASSERT(tracerColorsBlk->blockCount() > 0);
  for (unsigned int blockNo = 0; blockNo < tracerColorsBlk->blockCount(); blockNo++)
  {
    const DataBlock *tracerColorBlk = tracerColorsBlk->getBlock(blockNo);
    TracerType &tracerType = tracerTypes.push_back();
    tracerType.name = tracerColorBlk->getBlockName();
    tracerType.halfSize = tracerColorBlk->getReal("halfSize", 2.f);
    tracerType.radius = tracerColorBlk->getReal("radius", 0.01f);
    tracerType.color1 = tracerColorBlk->getE3dcolor("color1");
    tracerType.color2 = tracerColorBlk->getE3dcolor("color2");
    tracerType.startColor1 = tracerColorBlk->getE3dcolor("startColor1", tracerType.color1);
    tracerType.startColor2 = tracerColorBlk->getE3dcolor("startColor2", tracerType.color2);
    tracerType.startTime = tracerColorBlk->getReal("startTime", 0.0f);
    tracerType.hdrK = tracerColorBlk->getReal("hdrK", 1.0f);
    tracerType.beam = tracerColorBlk->getBool("beam", false);
    tracerType.minPixelSize = tracerColorBlk->getReal("minPixelSize", 0.0f);
    tracerType.fxName = tracerColorBlk->getStr("tracerFxType", "");

    float distScaleStart = tracerColorBlk->getReal("distScaleStart", 0.f);
    float distScaleEnd = tracerColorBlk->getReal("distScaleEnd", 1.f);
    float scaleStart = tracerColorBlk->getReal("distScale", 1.f);
    float scaleEnd = 1.0;
    tracerType.distScale.x = 1.f / (distScaleEnd - distScaleStart);
    tracerType.distScale.y = -distScaleStart / (distScaleEnd - distScaleStart);
    tracerType.distScale.z = scaleEnd - scaleStart;
    tracerType.distScale.w = scaleStart;
  }

  const DataBlock *tracerTrailsBlk = blk->getBlockByNameEx("tracerTrails");
  for (unsigned int blockNo = 0; blockNo < tracerTrailsBlk->blockCount(); blockNo++)
  {
    const DataBlock *tracerTrailBlk = tracerTrailsBlk->getBlock(blockNo);
    TrailType &trailType = trailTypes.push_back();
    trailType.name = tracerTrailBlk->getBlockName();
    trailType.particleHalfSizeFrom = tracerTrailBlk->getReal("particleHalfSizeFrom", 0.2f);
    trailType.particleHalfSizeTo = tracerTrailBlk->getReal("particleHalfSizeTo", 1.f);
    trailType.deviation = tracerTrailBlk->getReal("deviation", 2.f);
    trailType.color = tracerTrailBlk->getE3dcolor("color", 0xFFFFFFFF);
  }

  computeSupported = d3d::get_driver_desc().shaderModel >= 5.0_sm;
  multiDrawIndirectSupported = computeSupported && d3d::get_driver_desc().caps.hasWellSupportedIndirect;
  instanceIdSupported = d3d::get_driver_desc().caps.hasInstanceID;

  ShaderGlobal::set_int(::get_shader_variable_id("fx_instancing_type", true),
    computeSupported ? FX_INSTANCING_SBUF : FX_INSTANCING_CONSTS);

  if (computeSupported)
    createCmdCs = new_compute_shader("fx_create_cmd_cs");

  initHeads();
  initTrails();

  clear();
}

TracerManager::~TracerManager()
{
  releaseRes();
  del_it(createCmdCs);
}

void TracerManager::clear()
{
  finishPreparingIfNecessary();

  for (uint32_t tn = 0, bit = 1, processed = 0; tn < tracers.size() && processed < numTracers; ++tn, bit = (bit << 1) | (bit >> 31))
  {
    if (!(trExist[tn >> 5] & bit))
      continue;
    ++processed;
  }
  mem_set_0(trExist);
  mem_set_0(trCulledOrNotInitialized);
  numTracers = numVisibleTracers = 0;
  curTime = 0;
}

void TracerManager::reset()
{
  G_ASSERT(!preparing);

  releaseRes();

  initHeads();
  initTrails();
}

void TracerManager::update(float dt)
{
  TIME_PROFILE(tracer_manager_update);
  G_ASSERT(!preparing);

  curTime += dt;
  float vibX = fmodf(curTime * headVibratingRate, 1.0f);
  ShaderGlobal::set_real(tracerBeamTimeVarId, 0.5 + 0.5 * (int(curTime * headVibratingRate) % 2 ? 1.0 - vibX : vibX));

  PREFETCH_DATA(0, &tracers);
  const float decayInc = dt / tailLengthMeters;
  for (uint32_t tn = 0, bit = 1, processed = 0; tn < tracers.size() && processed < numTracers; ++tn, bit = (bit << 1) | (bit >> 31))
  {
    if (!(trExist[tn >> 5] & bit))
      continue;
    ++processed;
    Tracer *__restrict t = &tracers[tn];
    if (!t->shouldDecay)
      continue;
    t->decayT += t->speed * decayInc;
    PREFETCH_DATA(sizeof(Tracer), t);
    if (t->decayT > tailDecayToDelete)
    {
      trExist[tn >> 5] &= ~bit;
      --numTracers;
      --processed;
    }
  }
}

void TracerManager::doJob()
{
  TIME_PROFILE(tracer_manager_cull);
  G_ASSERT(mFrustum);

  mem_set_0(trCulledOrNotInitialized);
  numVisibleTracers = 0;
  segmentDrawCount = 0;
  uint32_t totalSegmentCount = 0;

  GPUFxTracerDynamic *tracerDynamicData = NULL;
  DrawIndexedIndirectArgs *tailArgsData = NULL;

  for (uint32_t tracerNo = 0, bit = 1, processed = 0; tracerNo < tracers.size() && processed < numTracers;
       tracerNo++, bit = (bit << 1) | (bit >> 31))
  {
    if (!(trExist[tracerNo >> 5] & bit))
      continue;
    ++processed;

    Tracer *__restrict tracer = &tracers[tracerNo];
    if (tracer->delayedDecay)
      tracer->shouldDecay = true;

    if (tracer->itl == TRACER_UNITIALIZED ||
        mFrustum->testSweptSphere(tracer->startPos, 50.f, tracer->pos - tracer->startPos) == Frustum::OUTSIDE)
      continue;

    trCulledOrNotInitialized[tracerNo >> 5] |= bit;
    ++numVisibleTracers;

    const float trWidth = tracer->tailParticleHalfSizeTo + tracer->tailParticleHalfSizeFrom;
    if (trWidth > 0.001f && tracer->tailColor.a > 0.01f && tracer->segments[tracer->firstSegment].partNum != 0)
      totalSegmentCount += tracer->lastSegment - tracer->firstSegment + 1;
  }

  for (uint32_t tracerNo = 0, bit = 1, processed = 0; tracerNo < tracers.size() && processed < numTracers;
       tracerNo++, bit = (bit << 1) | (bit >> 31))
  {
    if (!(trCulledOrNotInitialized[tracerNo >> 5] & bit))
      continue;
    ++processed;

    Tracer *__restrict tracer = &tracers[tracerNo];
    const float trWidth = tracer->tailParticleHalfSizeTo + tracer->tailParticleHalfSizeFrom;
    if (trWidth > 0.001f && tracer->tailColor.a > 0.01f && tracer->segments[tracer->firstSegment].partNum != 0)
    {
      int partStart = 0;

      for (int segmentNo = tracer->firstSegment; segmentNo <= tracer->lastSegment; ++segmentNo)
      {
        const TracerSegment &tracerSegment = tracer->segments[segmentNo];
        G_ASSERT(tracerSegment.partNum > 0);

        float distance = point_to_segment_distance(::grs_cur_view.pos, tracerSegment.startPos, tracerSegment.endPos);
        float dot = ::grs_cur_view.itm.getcol(2) * tracer->dir;
        float angle = safe_sqrt(1.f - dot * dot);
        angle = angle * 0.5f + 0.5f;
        float visibility = angle * max(0.0f, 1.f - distance / tailFartherLodDist);

        uint32_t lodNo = min((unsigned int)((1.f - visibility) * numTailLods), numTailLods - 1);
        uint32_t maxLodNo = log(max((float)tracerSegment.partNum, 1.0f));
        lodNo = min(maxLodNo, lodNo);
        int divisor = 1 << lodNo;
        G_ASSERT(tracerSegment.partNum >= divisor);

        if (tailArgsData == NULL)
        {
          if (multiDrawIndirectSupported)
          {
            tailArgsData =
              (DrawIndexedIndirectArgs *)tailIndirectRingBuffer->lockData(DRAW_INDEXED_INDIRECT_NUM_ARGS * totalSegmentCount);
            ringBufferPos = tailIndirectRingBuffer->getPos();
          }
          else
          {
            if (!tailIndirect.lock(0, sizeof(DrawIndexedIndirectArgs) * totalSegmentCount, (void **)&tailArgsData, VBLOCK_WRITEONLY))
              tailArgsData = NULL;
          }
        }

        G_ASSERT(tailArgsData);
        if (!tailArgsData)
        {
          if (tracerDynamicData)
            tracerDynamicBuffer.unlock();
          return;
        }

        DrawIndexedIndirectArgs &tailArgs = tailArgsData[segmentDrawCount];
        tailArgs.indexCountPerInstance = max(tracerSegment.partNum / divisor, 1) * FX_INDICES_PER_PARTICLE;
        tailArgs.instanceCount = tracerBuffer.getSbuffer() ? 1 : partStart;
        tailArgs.startIndexLocation = 0;
        tailArgs.baseVertexLocation = (partStart / divisor + lodOffsList[lodNo]) * FX_VERTICES_PER_PARTICLE;
        tailArgs.startInstanceLocation = tracerNo * MAX_FX_SEGMENTS + segmentNo;

        partStart += tracerSegment.partNum;
        ++segmentDrawCount;
      }
    }

    if (tracerDynamicData == NULL && !tracerDynamicBuffer.lock(0, 0, (void **)&tracerDynamicData, VBLOCK_WRITEONLY | VBLOCK_DISCARD))
    {
      if (tailArgsData)
      {
        if (multiDrawIndirectSupported)
          tailIndirectRingBuffer->unlockData(DRAW_INDEXED_INDIRECT_NUM_ARGS * totalSegmentCount);
        else
          tailIndirect.unlock();
      }
      return;
    }
    G_ASSERT(tracerDynamicData);
    if (!tracerDynamicData)
      continue;

    GPUFxTracerDynamic &tracerDynamic = tracerDynamicData[tracerNo];
    tracerDynamic.pos = tracer->pos;
    int tailPartCount = tracer->partCount + numTailParticles * saturate(tracer->decayT);
    int firstPartCount = tracer->segments[tracer->firstSegment].partNum;
    tracerDynamic.partCountSegmentId = (tailPartCount + firstPartCount * FX_TRACER_BITS_PART_COUNT) +
                                       (tracer->lastSegment + tracer->firstSegment * FX_TRACER_BITS_SEGMENT) * FX_TRACER_PACK_BIT;

    tracer->wasPrepared = true;
  }
  G_ASSERT(segmentDrawCount == totalSegmentCount);

  if (tailArgsData)
  {
    if (multiDrawIndirectSupported)
      tailIndirectRingBuffer->unlockData(DRAW_INDEXED_INDIRECT_NUM_ARGS * totalSegmentCount);
    else
      tailIndirect.unlock();
  }
  if (tracerDynamicData)
    tracerDynamicBuffer.unlock();
}

void TracerManager::initTrails()
{
  SharedTex texture = dag::get_tex_gameres("tracer_tail");
  if (!texture)
    texture = dag::get_tex_gameres("tracer_tail_tex");
  G_ASSERT(texture);
  tailTex = SharedTexHolder(eastl::move(texture), "tracer_tail_tex");
  Ptr<MaterialData> matNull = new MaterialData;
  matNull->className = "tracer_tail";
  tailMat = new_shader_material(*matNull);
  G_ASSERT(tailMat);
  tailMat->addRef();
  matNull = NULL;

  bool instancingSBufSupported = computeSupported;

  CompiledShaderChannelId *curChan = NULL;
  uint32_t numChanElements = 0;
  uint32_t vertexSize = 0;
  if (instancingSBufSupported)
  {
    static CompiledShaderChannelId chan[] = {{SCTYPE_SHORT2, SCUSAGE_POS, 0, 0}};
    curChan = chan;
    numChanElements = countof(chan);
    vertexSize = sizeof(TailVertex);
  }
  else
  {
    static CompiledShaderChannelId chan[] = {{SCTYPE_SHORT2, SCUSAGE_POS, 0, 0}, {SCTYPE_FLOAT3, SCUSAGE_TC, 0, 0}};
    curChan = chan;
    numChanElements = countof(chan);
    vertexSize = sizeof(TailVertexPerlin);
  }

  const float noisePeriodScale = tailLengthMeters / (float)tailNoisePeriodMeters;
  const float numTailParticlesInv = 1.0f / numTailParticles;
  tailParticles.create(instancingSBufSupported, sizeof(TailParticle), 0, numTailParticles,
    SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED | SBCF_MAYBELOST, 0, "tailBuffer");
  TailParticle *particleData;
  tailParticles.lock(0, 0, (void **)&particleData, VBLOCK_WRITEONLY);
  G_ASSERT(particleData);
  if (particleData)
    for (unsigned int particleNo = 0; particleNo < numTailParticles; particleNo++)
    {
      float t = (float)particleNo * numTailParticlesInv;
      TailParticle &particle = particleData[particleNo];
      particle.perlin = Point2(perlin_noise::noise1(t * noisePeriodScale), perlin_noise::noise1((t + 0.5f) * noisePeriodScale));
      particle.rv = gfrnd();
    }
  tailParticles.unlock();

  uint32_t vbTotalParticles = numTailParticles * (1 - pow(0.5f, (float)numTailLods)) / (1 - 0.5f);
  tailVb = dag::create_vb(vbTotalParticles * FX_VERTICES_PER_PARTICLE * vertexSize, SBCF_MAYBELOST, "tailVb");
  G_ASSERT(tailVb);

  uint8_t *tailVbData;
  tailVb->lock(0, 0, (void **)&tailVbData, VBLOCK_WRITEONLY);
  G_ASSERT(tailVbData);
  uint32_t lodOffs = 0;
  lodOffsList.resize(numTailLods);
  if (tailVbData)
    for (uint32_t lodNo = 0; lodNo < numTailLods; ++lodNo)
    {
      uint32_t divisor = 1 << lodNo;
      for (uint32_t particleNo = 0; particleNo < numTailParticles / divisor; particleNo++)
        for (uint32_t j = 0; j < FX_VERTICES_PER_PARTICLE; ++j)
        {
          uint32_t partId = particleNo * divisor;
          TailVertex &vert = (TailVertex &)(tailVbData[((lodOffs + particleNo) * FX_VERTICES_PER_PARTICLE + j) * vertexSize]);
          vert.vertexId = partId * FX_VERTICES_PER_PARTICLE + j;
          vert.divisor = divisor;
          if (!instancingSBufSupported)
          {
            TailParticle &particle = ((TailParticle *)tailParticles.getData())[partId];
            ((TailVertexPerlin &)vert).perlin = Point3(particle.perlin.x, particle.perlin.y, particle.rv);
          }
        }
      lodOffsList[lodNo] = lodOffs;
      lodOffs += numTailParticles / divisor;
    }
  tailVb->unlock();

  tailIb = dag::create_ib(numTailParticles * FX_INDICES_PER_PARTICLE * sizeof(uint16_t), SBCF_MAYBELOST, "tailIb");
  G_ASSERT(tailIb);

  uint16_t *tailIbData;
  tailIb->lock(0, 0, &tailIbData, VBLOCK_WRITEONLY);
  G_ASSERT(tailIbData);
  if (tailIbData)
    for (uint32_t particleNo = 0; particleNo < numTailParticles; particleNo++)
    {
      tailIbData[0] = particleNo * FX_VERTICES_PER_PARTICLE + 0;
      tailIbData[1] = particleNo * FX_VERTICES_PER_PARTICLE + 1;
      tailIbData[2] = particleNo * FX_VERTICES_PER_PARTICLE + 2;
      tailIbData[3] = particleNo * FX_VERTICES_PER_PARTICLE + 0;
      tailIbData[4] = particleNo * FX_VERTICES_PER_PARTICLE + 2;
      tailIbData[5] = particleNo * FX_VERTICES_PER_PARTICLE + 3;
      tailIbData += FX_INDICES_PER_PARTICLE;
    }
  tailIb->unlock();

  if (instancingSBufSupported)
  {
    static VSDTYPE vsdInstancing[] = {VSD_STREAM_PER_VERTEX_DATA(0), VSD_REG(VSDR_POS, VSDT_SHORT2), VSD_STREAM_PER_INSTANCE_DATA(1),
      VSD_REG(VSDR_TEXC0, VSDT_SHORT2), VSD_END};
    tailInstancingVdecl = d3d::create_vdecl(vsdInstancing);
    tailRendElem.vDecl = tailInstancingVdecl;
    tailInstancesIds = dag::create_vb(MAX_FX_TRACERS * MAX_FX_SEGMENTS * sizeof(uint32_t), SBCF_MAYBELOST, "tailInstancesId");
    d3d_err(!!tailInstancesIds);
    short *data;
    tailInstancesIds->lock(0, 0, (void **)&data, VBLOCK_WRITEONLY);
    G_ASSERT(data);
    if (data)
      for (int i = 0; i < MAX_FX_TRACERS * MAX_FX_SEGMENTS; ++i, data += 2)
        data[0] = data[1] = i;
    tailInstancesIds->unlock();
  }
  else
    tailParticles.close();

  const uint32_t bufferFlags = SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED | SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE | SBCF_MAYBELOST;
  tracerBuffer.create(instancingSBufSupported, sizeof(GPUFxTracer), sizeof(GPUFxTracerCreate), MAX_FX_TRACERS,
    createCmdCs ? SBCF_UA_SR_STRUCTURED : bufferFlags, 0, "tracerBuffer");
  if (instancingSBufSupported && !createCmdCs)
    tracerBuffer.createData(sizeof(GPUFxTracer), MAX_FX_TRACERS);
  tracerDynamicBuffer.create(instancingSBufSupported, sizeof(GPUFxTracerDynamic), 0, MAX_FX_TRACERS, bufferFlags, 0,
    "tracerDynamicBuffer");
  segmentBuffer.create(instancingSBufSupported, sizeof(GPUFxSegment), sizeof(GPUFxSegmentCreate), MAX_FX_TRACERS * MAX_FX_SEGMENTS,
    createCmdCs ? SBCF_UA_SR_STRUCTURED : bufferFlags, 0, "segmentBuffer");
  if (instancingSBufSupported && !createCmdCs)
    segmentBuffer.createData(sizeof(GPUFxSegment), MAX_FX_TRACERS * MAX_FX_SEGMENTS);
  if (multiDrawIndirectSupported)
  {
    tailIndirectRingBuffer.reset(new RingDynamicSB());
    tailIndirectRingBuffer->init(MAX_FX_TRACERS * MAX_FX_SEGMENTS * DRAW_INDEXED_INDIRECT_NUM_ARGS, INDIRECT_BUFFER_ELEMENT_SIZE,
      INDIRECT_BUFFER_ELEMENT_SIZE, SBCF_INDIRECT, 0, "tailIndirectRingBuffer");
  }
  else
  {
    tailIndirect.create(false, INDIRECT_BUFFER_ELEMENT_SIZE, 0, MAX_FX_TRACERS * MAX_FX_SEGMENTS * DRAW_INDEXED_INDIRECT_NUM_ARGS,
      /*SBCF_INDIRECT anyway it is a CPU buffer, so it is commented */ 0, 0, "tailIndirect");
  }

  tailRendElem.shElem = tailMat->make_elem();
  if (instancingSBufSupported)
    tailRendElem.shElem->replaceVdecl(tailRendElem.vDecl);
  else
    tailRendElem.vDecl = dynrender::addShaderVdecl(curChan, numChanElements);
  tailRendElem.stride = dynrender::getStride(curChan, numChanElements);
  G_ASSERT(tailRendElem.vDecl != BAD_VDECL);
  tailRendElem.minVert = 0;
  tailRendElem.numVert = numTailParticles * FX_VERTICES_PER_PARTICLE;
  tailRendElem.startIndex = 0;
  tailRendElem.numPrim = numTailParticles * FX_PRIMITIVES_PER_PARTICLE;
}

void TracerManager::renderTrails()
{
  if (segmentDrawCount == 0)
    return;

  tailRendElem.shElem->setStates(0, true);
  d3d::setvdecl(tailRendElem.vDecl);

  if (tracerBuffer.getSbuffer())
  {
    d3d::set_buffer(STAGE_VS, FX_PARTICLES_REGISTER_NO, tailParticles.getSbuffer());
    d3d::set_buffer(STAGE_VS, FX_TRACER_REGISTER_NO, tracerBuffer.getSbuffer());

    d3d::setvsrc_ex(0, tailVb.get(), 0, tailRendElem.stride);
    d3d::setind(tailIb.get());
    d3d::setvsrc_ex(1, tailInstancesIds.get(), 0, sizeof(uint32_t));

    if (multiDrawIndirectSupported)
    {
      d3d::multi_draw_indexed_indirect(PRIM_TRILIST, tailIndirectRingBuffer->getRenderBuf(), segmentDrawCount,
        INDIRECT_BUFFER_ELEMENT_SIZE * DRAW_INDEXED_INDIRECT_NUM_ARGS, ringBufferPos * INDIRECT_BUFFER_ELEMENT_SIZE);
    }
    else
    {
      for (uint32_t drawNo = 0; drawNo < segmentDrawCount; ++drawNo)
      {
        DrawIndexedIndirectArgs &args = ((DrawIndexedIndirectArgs *)tailIndirect.getData())[drawNo];
        d3d::drawind_instanced(PRIM_TRILIST, args.startIndexLocation, args.indexCountPerInstance / FX_INDICES_PER_PRIMITIVE,
          args.baseVertexLocation, 1, args.startInstanceLocation);
      }
    }

    d3d::setvsrc_ex(1, NULL, 0, 0);

    d3d::set_buffer(STAGE_VS, FX_PARTICLES_REGISTER_NO, NULL);
    d3d::set_buffer(STAGE_VS, FX_TRACER_REGISTER_NO, NULL);
  }
  else
  {
    d3d::setvsrc(0, tailVb.get(), tailRendElem.stride);
    d3d::setind(tailIb.get());

    for (uint32_t drawNo = 0; drawNo < segmentDrawCount; ++drawNo)
    {
      DrawIndexedIndirectArgs &args = ((DrawIndexedIndirectArgs *)tailIndirect.getData())[drawNo];

      GPUFxTracer tracer;
      GPUFxTracerDynamic tracerDynamic;
      GPUFxSegmentProcessed segment;
      carray<Point4, FX_TRACER_DATA_NUM_REGISTERS> tracerData;
      GET_FX_TRACER(args.startInstanceLocation, tailLengthMeters, numTailParticles, ((GPUFxTracer *)tracerBuffer.getData()),
        ((GPUFxTracerDynamic *)tracerDynamicBuffer.getData()), ((GPUFxSegment *)segmentBuffer.getData()), tracer, tracerDynamic,
        segment);
      PACK_FX_TRACER(tracerData, tracer, tracerDynamic, segment, args.instanceCount);

      d3d::set_vs_const(FX_TRACER_DATA_REGISTER_NO, (float *)&tracerData, tracerData.size());
      d3d::drawind(PRIM_TRILIST, args.startIndexLocation, args.indexCountPerInstance / FX_INDICES_PER_PRIMITIVE,
        args.baseVertexLocation);
    }
  }
}

void TracerManager::initHeads()
{
  G_ASSERT(tracerTypes.size() > 0);

  Ptr<MaterialData> matNull = new MaterialData;
  matNull->className = "tracer_head2";
  headMat = new_shader_material(*matNull);
  G_ASSERT(headMat);
  headMat->addRef();
  matNull = NULL;

  if (!instanceIdSupported)
  {
    static CompiledShaderChannelId chan[] = {{SCTYPE_SHORT2, SCUSAGE_POS, 0, 0}};
    if (!headMat->checkChannels(chan, countof(chan)))
      fatal("invalid channels for this material!");
    headRendElem.vDecl = dynrender::addShaderVdecl(chan, countof(chan));
    headRendElem.stride = dynrender::getStride(chan, countof(chan));
    G_ASSERT(headRendElem.vDecl != BAD_VDECL);
    headRendElem.minVert = 0;
    headRendElem.numVert = FX_VERTICES_PER_PARTICLE;
    headRendElem.startIndex = 0;
    headRendElem.numPrim = FX_PRIMITIVES_PER_PARTICLE;

    headVb = dag::create_vb(MAX_FX_TRACERS * FX_HEAD_VERTICES_PER_PARTICLE * sizeof(HeadVertex), SBCF_MAYBELOST, "headVb");
    G_ASSERT(headVb);

    uint8_t *headVbData;
    headVb->lock(0, 0, (void **)&headVbData, VBLOCK_WRITEONLY);
    if (headVbData)
      for (int headNo = 0; headNo < MAX_FX_TRACERS; ++headNo)
      {
        for (int vNo = 0; vNo < FX_HEAD_VERTICES_PER_PARTICLE; ++vNo)
        {
          HeadVertex &v = ((HeadVertex *)headVbData)[headNo * FX_HEAD_VERTICES_PER_PARTICLE + vNo];
          v.headId = headNo;
          v.posId = vNo;
        }
      }
    headVb->unlock();
  }

  headIb = dag::create_ib(MAX_FX_TRACERS * FX_HEAD_INDICES_PER_PARTICLE * sizeof(uint16_t), SBCF_MAYBELOST, "headIb");
  G_ASSERT(headIb);

  uint16_t *headIbData;
  headIb->lock(0, 0, &headIbData, VBLOCK_WRITEONLY);
  if (headIbData)
    for (uint32_t headNo = 0; headNo < MAX_FX_TRACERS; headNo++)
    {
      headIbData[0] = headNo * FX_HEAD_VERTICES_PER_PARTICLE + 0;
      headIbData[1] = headNo * FX_HEAD_VERTICES_PER_PARTICLE + 1;
      headIbData[2] = headNo * FX_HEAD_VERTICES_PER_PARTICLE + 2;
      headIbData[3] = headNo * FX_HEAD_VERTICES_PER_PARTICLE + 0;
      headIbData[4] = headNo * FX_HEAD_VERTICES_PER_PARTICLE + 2;
      headIbData[5] = headNo * FX_HEAD_VERTICES_PER_PARTICLE + 3;
      headIbData += FX_HEAD_INDICES_PER_PARTICLE;
    }
  headIb->unlock();

  headRendElem.shElem = headMat->make_elem();

  bool instancingSBufSupported = computeSupported;
  tracerTypeBuffer.create(instancingSBufSupported, sizeof(GPUFXTracerType), 0, tracerTypes.size(),
    SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED | SBCF_MAYBELOST, 0, "tracerTypeBuffer");
  GPUFXTracerType *tracerTypeData = NULL;
  tracerTypeBuffer.lock(0, 0, (void **)&tracerTypeData, VBLOCK_WRITEONLY);
  if (tracerTypeData)
    for (int typeNo = 0; typeNo < tracerTypes.size(); ++typeNo)
    {
      GPUFXTracerType &tracerType = tracerTypeData[typeNo];
      const TracerType &tracerData = tracerTypes[typeNo];
      const Point4 hdrK = Point4(tracerData.hdrK, tracerData.hdrK, tracerData.hdrK, 1.0f);
      tracerType.headHalfSize = tracerData.halfSize;
      tracerType.headRadius = tracerData.radius;
      tracerType.beam = tracerData.beam ? 1.0f : 0.0f;
      tracerType.minPixelSize = tracerData.minPixelSize;
      tracerType.tracerStartColor1 = Point3::rgb(color3(tracerData.startColor1));
      tracerType.tracerStartColor2 = Point3::rgb(color3(tracerData.startColor2));
      tracerType.tracerStartTime = tracerData.startTime;
      tracerType.tracerHeadColor1 = mul(Point4::rgba(color4(tracerData.color1)), hdrK);
      tracerType.tracerHeadColor2 = mul(Point3::rgb(color3(tracerData.color2)), Point3::xyz(hdrK));
    }
  tracerTypeBuffer.unlock();
}

void TracerManager::renderHeads()
{
  headRendElem.shElem->setStates(0, true);
  if (headVb)
  {
    d3d::setvdecl(headRendElem.vDecl);
    d3d::setvsrc(0, headVb.get(), headRendElem.stride);
  }
  else
    d3d::setvsrc(0, NULL, 0);
  d3d::setind(headIb.get());

  if (tracerTypeBuffer.getSbuffer())
    d3d::set_buffer(STAGE_VS, FX_TRACER_TYPE_REGISTER_NO, tracerTypeBuffer.getSbuffer());

  const int FX_HEAD_NUM_REGISTERS =
    tracerTypeBuffer.getSbuffer() ? FX_HEAD_MAX_SUPPORTED_NUM_REGISTERS : FX_HEAD_MIN_SUPPORTED_NUM_REGISTERS;
  int numRegisters = tracerTypeBuffer.getSbuffer() ? FX_HEAD_H_NUM_REGISTERS : FX_HEAD_NUM_REGISTERS;
  int batchInstNum =
    (d3d::set_vs_constbuffer_size(FX_HEAD_MAXIUM_REGISTERS + FX_TRACER_DATA_REGISTER_NO) - FX_TRACER_DATA_REGISTER_NO) / numRegisters;
  int curInstNum = 0;

  for (uint32_t tracerNo = 0, bit = 1, processed = 0; tracerNo < tracers.size() && processed < numVisibleTracers;
       tracerNo++, bit = (bit << 1) | (bit >> 31))
  {
    if (!(trCulledOrNotInitialized[tracerNo >> 5] & bit))
      continue;
    ++processed;
    Tracer *__restrict tracer = &tracers[tracerNo];
    if (tracer->shouldDecay || !tracer->wasPrepared)
      continue;

    if (tracerTypes[tracer->typeNo].color1.a <= 0.0f)
      continue;

    Point3 distVec = tracer->pos - ::grs_cur_view.pos;
    float eyeDist = distVec.length();
    float viewDist = dot(distVec, ::grs_cur_view.itm.getcol(2));
    float distAlpha = eyeDistBlend.x > 0 || eyeDistBlend.y > 0 ? cvt(eyeDist, eyeDistBlend.x, eyeDistBlend.y, 1.0f, 0.0f) : 1.0f;
    if (distAlpha <= 0.0f)
      continue;

    GPUFXHead headH;
    headH.tracerId = tracerNo;
    headH.segmentId = tracer->lastSegment;
    headH.tracerType = max(tracer->typeNo, 0);
    headH.opacity = clamp(tracer->timeLeft / headFadeTime, 0.f, 1.f) * distAlpha;
    headH.time = curTime - tracer->spawnTime;

    const TracerType &tType = tracerTypes[headH.tracerType];
    float viewScale = tType.distScale.w + tType.distScale.z * saturate(tType.distScale.y + tType.distScale.x * viewDist);
    float speedScale = cvt(tracer->speed, headSpeedScale.x, headSpeedScale.y, headSpeedScale.z, headSpeedScale.w);
    float maxHeadSize = length(tracer->pos - tracer->startPos);
    headH.lenScale = clamp(tType.halfSize * viewScale * speedScale, tracer->minLen * 0.5f, maxHeadSize * 0.5f) / tType.halfSize;

    if (tracerTypeBuffer.getSbuffer())
    {
      PACK_FX_HEAD(headData, curInstNum, headH);
    }
    else
    {
      GPUFXHeadProcessed head;
      GET_FX_HEAD_PROCESSED(headH, ((GPUFXTracerType *)tracerTypeBuffer.getData()),
        ((GPUFxTracerDynamic *)tracerDynamicBuffer.getData()), ((GPUFxSegment *)segmentBuffer.getData()), head);
      PACK_FX_HEAD_PROCESSED(headData, curInstNum, head);
    }

    ++curInstNum;
    if (curInstNum >= batchInstNum)
    {
      d3d::set_vs_const(FX_TRACER_DATA_REGISTER_NO, (float *)headData.data(), curInstNum * numRegisters);
      d3d::drawind(PRIM_TRILIST, 0, curInstNum * FX_HEAD_PRIMITIVES_PER_PARTICLE, 0);
      curInstNum = 0;
    }
  }

  if (curInstNum > 0)
  {
    d3d::set_vs_const(FX_TRACER_DATA_REGISTER_NO, (float *)headData.data(), curInstNum * numRegisters);
    d3d::drawind(PRIM_TRILIST, 0, curInstNum * FX_HEAD_PRIMITIVES_PER_PARTICLE, 0);
    curInstNum = 0;
  }

  d3d::set_vs_constbuffer_size(0);

  if (tracerTypeBuffer.getSbuffer())
    d3d::set_buffer(STAGE_VS, FX_TRACER_TYPE_REGISTER_NO, NULL);
}

void TracerManager::releaseRes()
{
  if (tailMat)
    tailMat->delRef();
  tailMat = NULL;

  if (headMat)
    headMat->delRef();
  headMat = NULL;

  clear();
  headIb.close();
  headVb.close();
  tailVb.close();
  tailIb.close();
  tailParticles.close();
  tailIndirect.close();
  tailIndirectRingBuffer.reset();
  ringBufferPos = 0;
  tailInstancesIds.close();
  d3d::delete_vdecl(tailInstancingVdecl);
  tracerBuffer.close();
  tracerDynamicBuffer.close();
  segmentBuffer.close();
}

void TracerManager::beforeRender(const Frustum *f)
{
  finishPreparingIfNecessary();

  if (!numTracers)
  {
    numVisibleTracers = 0;
    return;
  }

  G_ASSERT(f);
  mFrustum = f;


  tracerBuffer.process(createCmdCs, 0, maxTracerNo + 1);
  segmentBuffer.process(createCmdCs, 1, (maxTracerNo + 1) * MAX_FX_SEGMENTS);

  preparing = true;
  threadpool::add(this);
}

void TracerManager::renderTrans(bool heads, bool trails, float *hk, HeadPrimType head_prim_type)
{
  threadpool::wait(this);
  preparing = false;
  if (!numVisibleTracers)
    return;

  static int headProjectionHkVarId = ::get_shader_variable_id("head_projection_hk");
  if (hk)
  {
    ShaderGlobal::set_real(headProjectionHkVarId, *hk);
  }
  else
  {
    Driver3dPerspective p;
    d3d::getpersp(p);
    ShaderGlobal::set_real(headProjectionHkVarId, p.hk);
  }
  ShaderGlobal::set_int(tracerPrimTypeVarId, oldTracers ? -1 : head_prim_type);

  ShaderGlobal::setBlock(tracerBlockId, ShaderGlobal::LAYER_SCENE);
  if (tracerBuffer.getSbuffer())
  {
    d3d::set_buffer(STAGE_VS, FX_TRACER_DYNAMIC_REGISTER_NO, tracerDynamicBuffer.getSbuffer());
    d3d::set_buffer(STAGE_VS, FX_SEGMENT_REGISTER_NO, segmentBuffer.getSbuffer());
  }

  {
    TIME_PROFILE(render_trails_heads);
    if (trails)
      renderTrails();
    if (heads)
      renderHeads();
  }

  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
  if (tracerBuffer.getSbuffer())
  {
    d3d::set_buffer(STAGE_VS, FX_TRACER_DYNAMIC_REGISTER_NO, NULL);
    d3d::set_buffer(STAGE_VS, FX_SEGMENT_REGISTER_NO, NULL);
  }
}

void TracerManager::finishPreparingIfNecessary()
{
  if (preparing)
  {
    preparing = false;
    threadpool::wait(this);
  }
}

Tracer *TracerManager::createTracer(const Point3 &start_pos, const Point3 &speed, int tracerType, int trailType, float caliber,
  bool force, bool forceNoTrail, float cur_life_time)
{
  TracerMgrThreadGuard guard(this);
  G_ASSERT(!preparing);

  PREFETCH_DATA(0, &tracers);

  if (tracerType == -1 || trailType == -1)
    return NULL;

  G_ASSERT(tracerType < tracerTypes.size());
  G_ASSERT(trailType < trailTypes.size());

  if (trailTypes[trailType].color.a <= 0 ||
      (trailTypes[trailType].particleHalfSizeFrom + trailTypes[trailType].particleHalfSizeTo) < 0.001f)
    forceNoTrail = true;

  if (tracerTypes[tracerType].color1.a <= 0.0f && forceNoTrail)
    return NULL;

  if (numTracers == 0)
    tracerLimitExceedMsg = false;

  if (numTracers >= tracers.size())
  {
    if (!tracerLimitExceedMsg)
      logwarn("Can't create a new tracer. A total limit %d has been achived. force: %d", MAX_FX_TRACERS, force);
    tracerLimitExceedMsg = true;
    return NULL;
  }

  uint32_t fi = -1;
  for (uint32_t tracerNo = 0, bit = 1; tracerNo < tracers.size(); tracerNo++, bit = (bit << 1) | (bit >> 31))
  {
    if (!(trExist[tracerNo >> 5] & bit))
    {
      fi = tracerNo;
      goto space_found_;
    }
  }

  if (!tracerLimitExceedMsg)
    logwarn("Can't find space for a new tracer. Limit: %d, numTracers: %d, force: %d", MAX_FX_TRACERS, numTracers, force);
  tracerLimitExceedMsg = true;

  return NULL;
space_found_:
  // Drop tracers with a low priority if the total count achived reserved region in the common buffer
  if ((!force || tracerTypes[tracerType].color1.a <= 0.0f) && fi >= (int)(tracers.size() * (1.f - RESERVE_FOR_PLAYER_FRACTION)))
    return NULL;

  G_ASSERT(fi < tracers.size());
  trExist[fi >> 5] |= 1 << (fi & 31);
  numTracers++;

  float tailDeviation = trailTypes[trailType].deviation;
  maxTracerNo = numTracers == 1 ? fi : max(fi, maxTracerNo);

  Tracer *tracer = new (&tracers[fi], _NEW_INPLACE) Tracer(this, fi, start_pos, speed, tracerType, grnd(),
    forceNoTrail ? 0.f : trailTypes[trailType].particleHalfSizeFrom, forceNoTrail ? 0.f : trailTypes[trailType].particleHalfSizeTo,
    tailDeviation, color4(trailTypes[trailType].color), caliber, curTime - cur_life_time);
  return tracer;
}

int TracerManager::getTracerTypeNoByName(const char *name)
{
  if (!name)
    return -1;

  for (unsigned int tracerTypeNo = 0; tracerTypeNo < tracerTypes.size(); tracerTypeNo++)
  {
    if (!stricmp(tracerTypes[tracerTypeNo].name, name))
      return tracerTypeNo;
  }

  G_ASSERTF(0, "Invalid tracer type '%s'", name);
  return -1;
}

int TracerManager::getTrailTypeNoByName(const char *name)
{
  if (!name)
    return -1;

  for (unsigned int trailTypeNo = 0; trailTypeNo < trailTypes.size(); trailTypeNo++)
  {
    if (!stricmp(trailTypes[trailTypeNo].name, name))
      return trailTypeNo;
  }

  G_ASSERTF(0, "Invalid trail type '%s'", name);
  return -1;
}

Color4 TracerManager::getTraceColor(int no)
{
  G_ASSERTF_RETURN((no >= 0 && no < tracerTypes.size()), {}, "Invalid tracer index '%d'", no);
  return tracerTypes[no].color1;
}

const Point2 &TracerManager::getEyeDistBlend() const { return eyeDistBlend; }

void TracerManager::setEyeDistBlend(const Point2 &value) { eyeDistBlend = value; }

float TracerManager::getTrailSizeHeuristic(int trailType)
{
  if (trailType == -1)
    return 0.0001f;

  G_ASSERT(trailType >= 0 && trailType < trailTypes.size());

  return trailTypes[trailType].particleHalfSizeTo;
}
