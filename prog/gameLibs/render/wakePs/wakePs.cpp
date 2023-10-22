#include <render/wakePs.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <shaders/dag_computeShaders.h>
#include <math/dag_hlsl_floatx.h>
#include <math/random/dag_random.h>
#include <gameRes/dag_gameResources.h>
#include <memory/dag_framemem.h>
#include "wakePs.hlsli"


#define VERIFY_EMITTER_INDEX      G_ASSERT(index < emitters.size())
#define MAX_EMITTERS              1024u
#define MAX_PARTICLES             16384
#define MAX_PARTICLES_PER_EMITTER 64
#define ALLOC_EMITTERS_STEP       16
G_STATIC_ASSERT((sizeof(GPUEmitter) % 16) == 0);
G_STATIC_ASSERT((sizeof(GPUEmitterGen) % 16) == 0);
G_STATIC_ASSERT((sizeof(GPUEmitterDynamic) % 16) == 0);

const int wfx::ParticleSystem::MOD_VELOCITY = MOD_VELOCITY_FLAG;
const int wfx::ParticleSystem::MOD_MATERIAL = MOD_MATERIAL_FLAG;
static const char *SHADER_NAMES[] = {
  "wfx_render_height", "wfx_render_height_distorted", "wfx_render_foam", "wfx_render_foam_distorted", "wfx_render_foam_mask"};
G_STATIC_ASSERT(wfx::ParticleSystem::RENDER_TYPE_END == countof(SHADER_NAMES));


namespace wfx
{

ComputeMultiBuffer::ComputeMultiBuffer() : structSize(0), cmdSize(0), cmd(0), curDataNum(0), genWarp(0) {}

void ComputeMultiBuffer::createGPU(int struct_size, int elements, const char *gen_shader, int gen_warp, int cmd_size, unsigned flags,
  unsigned texfmt, const char *name)
{
  G_ASSERT(cmd_size > 0 && (cmd_size % 16) == 0 && struct_size + sizeof(uint32_t) <= cmd_size);
  structSize = struct_size;
  cmdSize = cmd_size;
  buf = dag::create_sbuffer(struct_size, elements, flags, texfmt, name);
  genShader.reset(gen_shader ? new_compute_shader(gen_shader) : NULL);
  genWarp = gen_warp;
}

void ComputeMultiBuffer::createData(int struct_size, int elements)
{
  structSize = struct_size;
  data.resize(struct_size * elements);
}

void ComputeMultiBuffer::createAll(int struct_size, int elements, bool gpu, const char *gen_shader, int gen_warp, int cmd_size,
  unsigned flags, unsigned texfmt, const char *name)
{
  if (gpu)
    createGPU(struct_size, elements, gen_shader, gen_warp, cmd_size, flags, texfmt, name);
  else
    createData(struct_size, elements);
}

void ComputeMultiBuffer::close()
{
  curDataNum = 0;
  cmd = 0;
  genShader.reset();
  clear_and_shrink(cmds);
  buf.close();
  clear_and_shrink(data);
}

int ComputeMultiBuffer::lock(uint32_t ofs_bytes, uint32_t size_bytes, void **p, int flags)
{
  if (buf)
    return buf->lock(ofs_bytes, size_bytes, p, flags);
  else
    *p = data.data();
  return 1;
}

void ComputeMultiBuffer::unlock()
{
  if (buf)
    buf->unlock();
}

void ComputeMultiBuffer::append(uint32_t id, dag::ConstSpan<uint8_t> elem)
{
  G_ASSERT(structSize == elem.size());

  if (buf)
  {
    append_items(cmds, structSize, elem.data());
    append_items(cmds, sizeof(id), (uint8_t *)&id);
    for (int pNo = structSize + sizeof(uint32_t); pNo < cmdSize; pNo += elem_size(cmds))
      cmds.push_back(0);
    ++cmd;
  }

  if (!data.empty())
  {
    memcpy(&data[id * structSize], elem.data(), structSize);
    curDataNum = max(curDataNum, id + 1);
  }
}

void ComputeMultiBuffer::process()
{
  if (!buf || cmds.empty())
    return;

  if (genShader)
  {
    d3d::set_rwbuffer(STAGE_CS, 0, buf.get());
    uint32_t elemSize = cmdSize;
    G_ASSERT((elemSize % 16) == 0);
    int elemCount = cmds.size() / elemSize;

    const int commandSizeInConsts = (elemSize + 15) / 16;
    const int reqSize = 1 + elemCount * commandSizeInConsts;
    const int cbufferSize = d3d::set_cs_constbuffer_size(reqSize);
    uint32_t v[4] = {0};

    for (int i = elemCount - cmd; i < elemCount; i += (cbufferSize - 1) / commandSizeInConsts)
    {
      int batch_size = min(elemCount - i, (int)(cbufferSize - 1) / commandSizeInConsts);
      v[0] = batch_size;
      d3d::set_cs_const(0, (float *)v, 1);

      d3d::set_cs_const(1, (float *)&cmds[i * elemSize], batch_size * commandSizeInConsts);
      genShader->dispatch((batch_size + genWarp - 1) / genWarp, 1, 1);
    }

    cmd = 0;
    erase_items(cmds, 0, elemSize * elemCount);

    d3d::set_cs_constbuffer_size(0);
    d3d::set_rwbuffer(STAGE_CS, 0, 0);
    d3d::resource_barrier({buf.get(), RB_RO_SRV | RB_STAGE_COMPUTE});
  }
  else
  {
    G_ASSERT(!data.empty() && curDataNum * structSize < data_size(data));
    buf->updateData(0, curDataNum * structSize, data.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  }
}

int ComputeMultiBuffer::getNumElements() const { return buf ? buf->getNumElements() : safediv(data.size(), (float)structSize); }

ParticleSystem::ParticleSystem(EffectManager *in_manager, const MainParams &main_params) :
  manager(in_manager), mainParams(main_params), lastEmitterIndex(-1)
{
  G_ASSERT(main_params.diffuseTexId != BAD_TEXTUREID);

  diffuseTexVarId = ::get_shader_variable_id("wfx_diffuse");
  distortionTexVarId = ::get_shader_variable_id("wfx_distortion");
  gradientTexVarId = ::get_shader_variable_id("wfx_gradient");
  countVarId = ::get_shader_variable_id("wfx_count");
  offsetVarId = ::get_shader_variable_id("wfx_offset");
  limitVarId = ::get_shader_variable_id("wfx_limit");

  initRes();
}

ParticleSystem::~ParticleSystem()
{
  releaseRes();
  clearEmitters();
}

uint32_t ParticleSystem::addEmitter(const EmitterParams &params, int capacity)
{
  uint32_t emitterIndex = emitters.size();
  for (uint32_t emitterNo = 0; emitterNo < emitters.size(); ++emitterNo)
    if (emitters[emitterNo].removed)
    {
      emitterIndex = emitterNo;
      break;
    }

  G_ASSERTF(capacity == 0, "not supported yet");
  G_UNREFERENCED(capacity);
  int numSlots = MAX_PARTICLES_PER_EMITTER;
  int startSlot = numSlots * emitterIndex;

  Emitter &emitter = emitterIndex < emitters.size() ? emitters[emitterIndex] : emitters.push_back();
  emitter.removed = false;
  emitter.params = params;
  emitter.accumTime = 0;
  emitter.accumDist = 0;
  emitter.lastPos = Point2(0, 0);
  emitter.spawned = false;

  emitter.startSlot = startSlot;
  emitter.numSlots = numSlots;
  emitter.curSlotOffset = 0;

  reserveEmitters(emitterIndex + 1);
  addGPUEmiter(emitterIndex, emitter.params);

  lastEmitterIndex = max<int>(lastEmitterIndex, emitterIndex);

  return emitterIndex;
}

void ParticleSystem::removeEmitter(uint32_t index)
{
  VERIFY_EMITTER_INDEX;
  emitters[index].removed = true;

  // removed emitters stay for 5 more seconds to fade the emitter out
  emitters[index].deleteCountdown = 5;
}

void ParticleSystem::deleteEmitter(int index)
{
  if (lastEmitterIndex == index)
  {
    lastEmitterIndex = -1;
    for (int emitterNo = emitters.size() - 1; emitterNo >= 0; --emitterNo)
      if (!emitters[emitterNo].removed)
      {
        lastEmitterIndex = emitterNo;
        break;
      }
  }
}

void ParticleSystem::clearEmitters()
{
  clear_and_shrink(emitters);
  lastEmitterIndex = -1;
}

void ParticleSystem::reserveEmitters(uint32_t count)
{
  G_ASSERT(count < MAX_EMITTERS);
  int newCount = min(count, MAX_EMITTERS);
  int lastCount = emitterBuffer.getNumElements();

  if (newCount > lastCount)
  {
    emitterBuffer.close();
    particleBuffer.close();
    particleRenderBuffer.close();

    newCount = ((newCount - 1) / ALLOC_EMITTERS_STEP + 1) * ALLOC_EMITTERS_STEP;

    emitterBuffer.createGPU(sizeof(GPUEmitter), newCount, "wfx_gen_emitter", EMITTER_GEN_WARP, sizeof(GPUEmitterGen),
      SBCF_UA_SR_STRUCTURED, 0, "wfxEmitterBuffer");
    for (uint32_t emitterNo = 0; emitterNo < lastCount; ++emitterNo)
      addGPUEmiter(emitterNo, emitters[emitterNo].params);

    particleBuffer = dag::buffers::create_ua_sr_structured(sizeof(GPUParticle), newCount * MAX_PARTICLES_PER_EMITTER,
      "wfxParticleBuffer", dag::buffers::Init::Zero);
    particleRenderBuffer = dag::buffers::create_ua_sr_structured(sizeof(GPUParticleRender), newCount * MAX_PARTICLES_PER_EMITTER,
      "wfxParticleRenderBuffer");
  }
}

void ParticleSystem::resetEmitterSpawnCounters(uint32_t index, bool jump)
{
  VERIFY_EMITTER_INDEX;
  if (jump)
    emitters[index].lastPos = emitters[index].params.pose.pos;
  else
  {
    emitters[index].spawned = false;
    emitters[index].accumDist = 0;
    emitters[index].accumTime = 0;
  }
}

const ParticleSystem::Spawn &ParticleSystem::getEmitterSpawn(uint32_t index) const
{
  VERIFY_EMITTER_INDEX;
  return emitters[index].params.spawn;
}

void ParticleSystem::setEmitterSpawn(uint32_t index, const ParticleSystem::Spawn &spawn)
{
  VERIFY_EMITTER_INDEX;
  emitters[index].params.spawn = spawn;
}

const ParticleSystem::Pose &ParticleSystem::getEmitterPose(uint32_t index) const
{
  VERIFY_EMITTER_INDEX;
  return emitters[index].params.pose;
}

void ParticleSystem::setEmitterPose(uint32_t index, const Pose &pose)
{
  VERIFY_EMITTER_INDEX;
  emitters[index].params.pose = pose;
}

const ParticleSystem::Velocity &ParticleSystem::getEmitterVelocity(uint32_t index) const
{
  VERIFY_EMITTER_INDEX;
  return emitters[index].params.velocity;
}

void ParticleSystem::setEmitterVelocity(uint32_t index, const Velocity &velocity)
{
  VERIFY_EMITTER_INDEX;
  emitters[index].params.velocity = velocity;
}

float ParticleSystem::getEmitterAlpha(uint32_t index) const
{
  VERIFY_EMITTER_INDEX;
  return emitters[index].params.material.alpha;
}

void ParticleSystem::setEmitterAlpha(uint32_t index, float value)
{
  VERIFY_EMITTER_INDEX;
  emitters[index].params.material.alpha = value;
}

float ParticleSystem::getEmitPerMeter(uint32_t index) const { return emitters[index].params.spawn.emitPerMeter; }

void ParticleSystem::setEmitPerMeter(uint32_t index, float value) { emitters[index].params.spawn.emitPerMeter = value; }

bool ParticleSystem::isActive() const { return lastEmitterIndex > -1; }

void ParticleSystem::setRenderType(RenderType renderType) { mainParams.renderType = renderType; }

void ParticleSystem::initRes()
{
  indirectBuffer = dag::buffers::create_ua_indirect(dag::buffers::Indirect::DrawIndexed, 1, "wfxIndirectBuffer");
  DrawIndexedIndirectArgs *indirectData;
  if (indirectBuffer->lock(0, 0, (void **)&indirectData, VBLOCK_WRITEONLY) && indirectData)
  {
    indirectData->indexCountPerInstance = INDICIES_PER_PARTICLE;
    indirectData->instanceCount = 0;
    indirectData->startIndexLocation = 0;
    indirectData->baseVertexLocation = 0;
    indirectData->startInstanceLocation = 0;
    indirectBuffer->unlock();
  }
}

void ParticleSystem::releaseRes()
{
  emitterBuffer.close();
  particleBuffer.close();
  indirectBuffer.close();
  particleRenderBuffer.close();
}

void ParticleSystem::reset()
{
  releaseRes();
  initRes();
  reserveEmitters(emitters.size());

  for (int emitterNo = 0; emitterNo < emitters.size(); ++emitterNo)
  {
    emitters[emitterNo].curSlotOffset = 0;
    addGPUEmiter(emitterNo, emitters[emitterNo].params);
  }
}

void ParticleSystem::update(float dt)
{
  if (lastEmitterIndex == -1)
    return;

  STATE_GUARD_NULLPTR(d3d::set_buffer(STAGE_CS, EMITTER_BUFFER_REGISTER, VALUE), emitterBuffer.getSbuffer());
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, PARTICLE_BUFFER_REGISTER, VALUE), particleBuffer.get());

  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, PARTICLE_RENDER_BUFFER_REGISTER, VALUE), particleRenderBuffer.get());
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, INDIRECT_REGISTER, VALUE), indirectBuffer.get());

  manager->clearShader->dispatch(1, 1, 1);
  d3d::resource_barrier({indirectBuffer.get(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});

  int startSlot = emitters[0].startSlot;
  int numSlots = emitters[lastEmitterIndex].startSlot + emitters[lastEmitterIndex].numSlots - startSlot;

  ShaderGlobal::set_int(countVarId, numSlots);
  ShaderGlobal::set_int(offsetVarId, startSlot);
  manager->updateShader->dispatch((numSlots + UPDATE_WARP - 1) / UPDATE_WARP, 1, 1);

  for (int emitterIx = lastEmitterIndex; emitterIx >= 0; --emitterIx)
  {
    Emitter &em = emitters[emitterIx];
    if (em.removed && em.deleteCountdown > 0)
    {
      em.deleteCountdown -= dt;
      if (em.deleteCountdown <= 0)
        deleteEmitter(emitterIx);
    }
  }
  d3d::resource_barrier({particleBuffer.get(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
  d3d::resource_barrier({particleRenderBuffer.get(), RB_RO_SRV | RB_STAGE_VERTEX});
  d3d::resource_barrier({indirectBuffer.get(), RB_RO_INDIRECT_BUFFER});
}

bool ParticleSystem::render()
{
  if (lastEmitterIndex == -1)
    return false;

  d3d::set_buffer(STAGE_VS, PARTICLE_RENDER_BUFFER_REGISTER, particleRenderBuffer.get());
  ShaderGlobal::set_texture(diffuseTexVarId, mainParams.diffuseTexId);
  if ((mainParams.renderType == RENDER_HEIGHT_DISTORTED) || (mainParams.renderType == RENDER_FOAM_DISTORTED) ||
      (mainParams.renderType == RENDER_FOAM_MASK))
  {
    ShaderGlobal::set_texture(distortionTexVarId, mainParams.distortionTexId);
    ShaderGlobal::set_texture(gradientTexVarId, mainParams.gradientTexId);
  }

  manager->renderShaders[mainParams.renderType].shader->setStates(0, true);
  d3d::draw_indexed_indirect(PRIM_TRILIST, indirectBuffer.get(), 0);

  d3d::set_buffer(STAGE_VS, PARTICLE_RENDER_BUFFER_REGISTER, NULL);

  return true;
}

void ParticleSystem::emit(float dt)
{
  if (lastEmitterIndex == -1)
    return;

  const int regPerEmitter = sizeof(GPUEmitterDynamic) / 16;
  Tab<GPUEmitterDynamic> emitterDynData(framemem_ptr());

  for (int emitterNo = 0; emitterNo <= lastEmitterIndex; ++emitterNo)
  {
    Emitter &emitter = emitters[emitterNo];
    if (emitter.removed)
      continue;

    float maxLifetime = emitter.params.spawn.lifeTime + emitter.params.spawn.lifeSpread;
    float maxAllowedEmissionFrequency = MAX_PARTICLES_PER_EMITTER / maxLifetime * 0.9f;
    float distance = length(emitter.lastPos - emitter.params.pose.pos);
    float emitPerSecond = min(emitter.params.spawn.emitPerSecond, maxAllowedEmissionFrequency);
    float emitPerMeter = emitter.params.spawn.emitPerMeter;
    float distanceEmissionFrequency = safediv(emitPerMeter * distance, dt);
    if (distanceEmissionFrequency > maxAllowedEmissionFrequency)
      emitPerMeter *= safediv(maxAllowedEmissionFrequency, distanceEmissionFrequency);

    emitter.accumTime = min(emitter.accumTime + dt * emitPerSecond, (float)EMIT_PER_FRAME_LIMIT);
    if (emitter.spawned)
      emitter.accumDist = min(emitter.accumDist + emitPerMeter * distance, (float)EMIT_PER_FRAME_LIMIT);
    emitter.lastPos = emitter.params.pose.pos;
    emitter.spawned = true;

    int numSlots = min(emitter.numSlots, (MAX_EMITTER_GEN_COMMANDS - EMITTER_DYN_BUFFER_REGISTER) / regPerEmitter - 3);
    int numTimeParticles = min<int>(emitter.accumTime, numSlots);
    int numDistParticles = min<int>(emitter.accumDist, numSlots - numTimeParticles);
    int numParticles = numTimeParticles + numDistParticles;
    if (numParticles <= 0)
      continue;
    emitter.accumTime -= numTimeParticles;
    emitter.accumDist -= numDistParticles;

    GPUEmitterDynamic &emitterDyn = emitterDynData.push_back();
    G_STATIC_ASSERT(MAX_PARTICLES_PER_EMITTER <= 2048 && MAX_EMITTERS <= 1024);
    PACK_ID_OFFSET_NUM(emitterDyn.id_coff, emitterNo, emitter.curSlotOffset, numParticles);
    emitterDyn.pos = emitter.params.pose.pos;
    emitterDyn.scaleZ = emitter.params.pose.scale.z;
    emitterDyn.rot = emitter.params.pose.rot;
    emitterDyn.vel = emitter.params.velocity.vel;
    emitterDyn.dir = emitter.params.velocity.dir;
    emitterDyn.alpha = emitter.params.material.alpha;
    emitterDyn.startSlot = emitter.startSlot;
    emitterDyn.numSlots = emitter.numSlots;

    emitter.curSlotOffset = (emitter.curSlotOffset + numParticles) % emitter.numSlots;
  }

  if (!emitterDynData.empty())
  {
    const int reqSize = EMITTER_DYN_BUFFER_REGISTER + emitterDynData.size() * regPerEmitter;
    const int cbufferSize = d3d::set_cs_constbuffer_size(reqSize);
    const int batchSize = (cbufferSize - EMITTER_DYN_BUFFER_REGISTER) / regPerEmitter;

    d3d::set_buffer(STAGE_CS, EMITTER_BUFFER_REGISTER, emitterBuffer.getSbuffer());
    d3d::set_rwbuffer(STAGE_CS, PARTICLE_BUFFER_REGISTER, particleBuffer.get());

    for (int batchNo = 0; batchNo < emitterDynData.size(); batchNo += batchSize)
    {
      if (batchNo > 0)
        d3d::resource_barrier({particleBuffer.get(), RB_NONE});
      int batchCount = min<int>(emitterDynData.size() - batchNo, batchSize);
      G_ASSERT(batchCount > 0);
      d3d::set_cs_const(EMITTER_DYN_BUFFER_REGISTER, (float *)&emitterDynData[batchNo], batchCount * regPerEmitter);
      ShaderGlobal::set_int(countVarId, batchCount * EMIT_PER_FRAME_LIMIT);
      manager->emitShader->dispatch((batchCount * EMIT_PER_FRAME_LIMIT + EMIT_WARP - 1) / EMIT_WARP, 1, 1);
    }

    d3d::set_buffer(STAGE_CS, EMITTER_BUFFER_REGISTER, NULL);
    d3d::set_rwbuffer(STAGE_CS, PARTICLE_BUFFER_REGISTER, NULL);
    d3d::resource_barrier({particleBuffer.get(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});

    d3d::set_cs_constbuffer_size(0);
  }
}

void ParticleSystem::addGPUEmiter(uint32_t id, const EmitterParams &params)
{
  G_ASSERT(id < MAX_EMITTERS);
  if (id >= MAX_EMITTERS)
    return;

  GPUEmitter gpuEmitter;
  gpuEmitter.lifeTime = params.spawn.lifeTime;
  gpuEmitter.lifeDelay = params.spawn.lifeDelay;
  gpuEmitter.lifeSpread = params.spawn.lifeSpread;
  gpuEmitter.velSpread = params.velocity.velSpread;
  gpuEmitter.dirSpread = params.velocity.dirSpread;
  gpuEmitter.posSpread = params.pose.posSpread;
  gpuEmitter.rotSpread = params.pose.rotSpread;
  gpuEmitter.radius = params.pose.radius;
  gpuEmitter.radiusSpread = params.pose.radiusSpread;
  gpuEmitter.radiusScale = params.pose.radiusScale;
  gpuEmitter.velRes = params.velocity.velRes;
  gpuEmitter.scaleXY = Point2::xy(params.pose.scale);
  gpuEmitter.uvNumFrames.x = max(params.material.uvNumFrames.x, 1);
  gpuEmitter.uvNumFrames.y = max(params.material.uvNumFrames.y, 1);
  gpuEmitter.startColor = params.material.startColor;
  gpuEmitter.endColor = params.material.endColor;
  gpuEmitter.uv = params.material.uv;
  gpuEmitter.activeFlags = params.activeFlags;
  emitterBuffer.append(id, dag::ConstSpan<uint8_t>((uint8_t *)&gpuEmitter, sizeof(gpuEmitter)));
}


EffectManager::EffectManager()
{
  fTimeVarId = ::get_shader_variable_id("wfx_ftime");

  initRes();
}

EffectManager::~EffectManager()
{
  releaseRes();
  clearPSystems();
}

void EffectManager::initRes()
{
  emitShader.reset(new_compute_shader("wfx_emit"));
  updateShader.reset(new_compute_shader("wfx_update"));
  clearShader.reset(new_compute_shader("wfx_clear"));
  G_ASSERT(emitShader && updateShader && clearShader);

  for (int renderType = 0; renderType < ParticleSystem::RENDER_TYPE_END; renderType++)
    renderShaders[renderType].init(SHADER_NAMES[renderType], NULL, 0, SHADER_NAMES[renderType]);

  particleQuadIB = dag::create_ib(MAX_PARTICLES * INDICIES_PER_PARTICLE * sizeof(uint16_t), 0, "particleQuadIB");
  G_ASSERT(particleQuadIB);

  uint16_t *particleQuadIBData;
  if (!particleQuadIB->lock(0, 0, &particleQuadIBData, VBLOCK_WRITEONLY) || !particleQuadIBData)
    return;
  for (uint32_t particleNo = 0; particleNo < MAX_PARTICLES; particleNo++)
  {
    particleQuadIBData[0] = particleNo * VERTICES_PER_PARTICLE + 0;
    particleQuadIBData[1] = particleNo * VERTICES_PER_PARTICLE + 1;
    particleQuadIBData[2] = particleNo * VERTICES_PER_PARTICLE + 2;
    particleQuadIBData[3] = particleNo * VERTICES_PER_PARTICLE + 0;
    particleQuadIBData[4] = particleNo * VERTICES_PER_PARTICLE + 2;
    particleQuadIBData[5] = particleNo * VERTICES_PER_PARTICLE + 3;
    particleQuadIBData += INDICIES_PER_PARTICLE;
  }
  particleQuadIB->unlock();

  randomBuffer = dag::create_tex(NULL, RANDOM_BUFFER_RESOLUTION_X, RANDOM_BUFFER_RESOLUTION_Y, TEXFMT_A8R8G8B8 | TEXCF_MAYBELOST, 1,
    "wfxRandomBuffer");
  uint8_t *randomBufferData = NULL;
  int randomBufferStride = 0;
  if (!randomBuffer->lockimg((void **)&randomBufferData, randomBufferStride, 0, TEXLOCK_WRITE) || !randomBufferData)
    return;
  for (int y = 0; y < RANDOM_BUFFER_RESOLUTION_Y; ++y, randomBufferData += randomBufferStride)
    for (int x = 0; x < RANDOM_BUFFER_RESOLUTION_X; ++x)
      for (int c = 0; c < 4; ++c)
        randomBufferData[x * 4 + c] = clamp(gauss_rnd() * 0.5f + 0.5f, 0.0f, 1.0f) * 255;
  randomBuffer->unlockimg();
  randomBuffer->texaddr(TEXADDR_WRAP);
}

void EffectManager::releaseRes()
{
  emitShader.reset();
  updateShader.reset();
  clearShader.reset();

  for (int renderType = 0; renderType < ParticleSystem::RENDER_TYPE_END; renderType++)
    renderShaders[renderType].close();

  particleQuadIB.close();
  randomBuffer.close();
}

ParticleSystem *EffectManager::addPSystem(const ParticleSystem::MainParams &main_params)
{
  ParticleSystem *pSystem = new ParticleSystem(this, main_params);
  pSystems.push_back(pSystem);
  return pSystem;
}

void EffectManager::clearPSystems()
{
  for (auto &pSystem : pSystems)
    del_it(pSystem);
  clear_and_shrink(pSystems);
}

void EffectManager::reset()
{
  releaseRes();
  initRes();

  for (auto &pSystem : pSystems)
    pSystem->reset();
}

void EffectManager::update(float dt)
{
  if (pSystems.empty())
    return;

  for (auto &pSystem : pSystems)
    pSystem->emitterBuffer.process();

  ShaderGlobal::set_real(fTimeVarId, dt);

  for (auto &pSystem : pSystems)
  {
    d3d::set_tex(STAGE_CS, RANDOM_BUFFER_REGISTER, randomBuffer.get(), false);
    pSystem->emit(dt);
    d3d::set_tex(STAGE_CS, RANDOM_BUFFER_REGISTER, NULL, false);

    pSystem->update(dt);
  }
}

bool EffectManager::render(ParticleSystem::RenderType render_type)
{
  if (pSystems.empty())
    return false;

  d3d::setvsrc(0, 0, 0);
  d3d::setind(particleQuadIB.get());

  bool renderedAnything = false;
  for (auto &pSystem : pSystems)
  {
    if (pSystem->getMainParams().renderType == render_type)
      renderedAnything |= pSystem->render();
  }
  return renderedAnything;
}

} // namespace wfx
