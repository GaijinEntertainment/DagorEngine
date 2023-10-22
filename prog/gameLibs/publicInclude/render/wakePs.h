//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_carray.h>
#include <generic/dag_tab.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <3d/dag_resPtr.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <EASTL/unique_ptr.h>

class ComputeShaderElement;

namespace wfx
{

class EffectManager;

class ComputeMultiBuffer
{
public:
  ComputeMultiBuffer();

  void createGPU(int struct_size, int elements, const char *gen_shader, int gen_warp, int cmd_size, unsigned flags, unsigned texfmt,
    const char *name);
  void createData(int struct_size, int elements);
  void createAll(int struct_size, int elements, bool gpu, const char *shader_name, int cs_warp, int cmd_size, unsigned flags,
    unsigned texfmt, const char *name);
  void close();
  int lock(uint32_t ofs_bytes, uint32_t size_bytes, void **p, int flags);
  void unlock();
  void append(uint32_t id, dag::ConstSpan<uint8_t> elem);
  void process();

  inline Sbuffer *getSbuffer() const { return buf.get(); }
  inline const uint8_t *getData() const { return data.data(); }
  int getNumElements() const;

private:
  int structSize;
  int cmdSize;
  uint32_t curDataNum;
  BufPtr buf;
  eastl::unique_ptr<ComputeShaderElement> genShader;
  int genWarp;
  Tab<uint8_t> data;
  Tab<uint8_t> cmds;
  int cmd;
};

class ParticleSystem
{
  friend EffectManager;

public:
  struct EmitterParams;
  struct Spawn;
  struct Pose;
  struct Velocity;

  uint32_t addEmitter(const EmitterParams &params, int capacity = 0);
  void removeEmitter(uint32_t index);
  void clearEmitters();
  void reserveEmitters(uint32_t count);
  void resetEmitterSpawnCounters(uint32_t index, bool jump = false);

  const Spawn &getEmitterSpawn(uint32_t index) const;
  void setEmitterSpawn(uint32_t index, const Spawn &spawn);

  const Pose &getEmitterPose(uint32_t index) const;
  void setEmitterPose(uint32_t index, const Pose &pose);

  const Velocity &getEmitterVelocity(uint32_t index) const;
  void setEmitterVelocity(uint32_t index, const Velocity &velocity);

  float getEmitterAlpha(uint32_t index) const;
  void setEmitterAlpha(uint32_t index, float value);

  float getEmitPerMeter(uint32_t index) const;
  void setEmitPerMeter(uint32_t index, float value);

  bool isActive() const;

  enum RenderType
  {
    RENDER_HEIGHT = 0,
    RENDER_HEIGHT_DISTORTED = 1,
    RENDER_FOAM = 2,
    RENDER_FOAM_DISTORTED = 3,
    RENDER_FOAM_MASK = 4,
    RENDER_TYPE_END
  };

  struct MainParams
  {
    RenderType renderType = RENDER_HEIGHT;
    TEXTUREID diffuseTexId = BAD_TEXTUREID;
    TEXTUREID distortionTexId = BAD_TEXTUREID;
    TEXTUREID gradientTexId = BAD_TEXTUREID;
  };

  struct Spawn
  {
    float emitPerSecond = 0.0f;
    float emitPerMeter = 0.0f;
    float lifeTime = 0.0f;
    float lifeDelay = 0.0f;
    float lifeSpread = 0.0f;
  };

  struct Velocity
  {
    float vel = 0.0f;
    float velRes = 0.0f;
    float velSpread = 0.0f;
    Point2 dir = Point2(0.0f, 0.0f);
    float dirSpread = 0.0f;
  };

  struct Pose
  {
    Point2 pos = Point2(0.0f, 0.0f);
    float posSpread = 0.0f;
    float rot = 0.0f;
    float rotSpread = 0.0f;
    float radius = 0.0f;
    float radiusSpread = 0.0f;
    float radiusScale = 0.0f; // over lifetime
    Point3 scale = Point3(1.0f, 1.0f, 1.0f);
  };

  struct Material
  {
    uint32_t texIndex = 0;
    Point4 uv = Point4(1.0f, 1.0f, 0.0f, 0.0f);
    IPoint2 uvNumFrames = IPoint2(1, 1);
    Point4 startColor = Point4(1.0f, 1.0f, 1.0f, 1.0f);
    Point4 endColor = Point4(1.0f, 1.0f, 1.0f, 1.0f);
    float alpha = 1.0f;
  };

  struct EmitterParams
  {
    Spawn spawn;
    Pose pose;
    Velocity velocity;
    Material material;

    uint32_t activeFlags = 0;
  };

  void setRenderType(RenderType renderType);

  static const int MOD_VELOCITY;
  static const int MOD_MATERIAL;

private:
  ParticleSystem(EffectManager *in_manager, const MainParams &main_params);
  ~ParticleSystem();

  void initRes();
  void releaseRes();
  void reset();

  void update(float dt);
  bool render();
  void emit(float dt);

  void addGPUEmiter(uint32_t id, const EmitterParams &params);

  void deleteEmitter(int index);

  const MainParams &getMainParams() { return mainParams; }

  struct Emitter
  {
    EmitterParams params;

    float accumTime;
    float accumDist;
    Point2 lastPos;

    int startSlot;
    int numSlots;
    int curSlotOffset;

    bool removed;
    bool spawned;
    float deleteCountdown;
  };

  EffectManager *manager;
  MainParams mainParams;
  Tab<Emitter> emitters;
  int lastEmitterIndex;

  ComputeMultiBuffer emitterBuffer;
  BufPtr particleBuffer;
  BufPtr particleRenderBuffer;
  BufPtr indirectBuffer;

  int diffuseTexVarId;
  int distortionTexVarId;
  int gradientTexVarId;
  int countVarId;
  int offsetVarId;
  int limitVarId;
};

class EffectManager
{
  friend ParticleSystem;

public:
  EffectManager();
  ~EffectManager();

  ParticleSystem *addPSystem(const ParticleSystem::MainParams &main_params);
  void clearPSystems();

  void reset();

  void update(float dt);
  bool render(ParticleSystem::RenderType render_type);

private:
  void initRes();
  void releaseRes();

  Tab<ParticleSystem *> pSystems;

  eastl::unique_ptr<ComputeShaderElement> emitShader;
  eastl::unique_ptr<ComputeShaderElement> updateShader;
  eastl::unique_ptr<ComputeShaderElement> clearShader;
  DynamicShaderHelper renderShaders[ParticleSystem::RENDER_TYPE_END];

  BufPtr particleQuadIB;
  TexPtr randomBuffer;

  int fTimeVarId;
};

} // namespace wfx
