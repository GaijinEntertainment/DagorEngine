//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stdint.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <vecmath/dag_vecMathDecl.h>
#include <3d/dag_resId.h>

class Point4;
struct Frustum;

namespace pfx
{
typedef uint32_t CullingId;
typedef uint32_t ContextId;
typedef uint32_t SystemId;
typedef uint32_t InstanceId;

const uint32_t BAD_ID = 0xffffffff;

enum class EmissionType : int
{
  UNDEFINED = 0, // for debug purposes
  FIXED = 1,     // emmit X at once and keep them alive
  BURST = 2,     // emmit X every Y period, Z times
  LINEAR = 3,    // emmit with constant spawn rate (X parts / Y lifetime)
};

struct EmissionData
{
  EmissionType type = EmissionType::UNDEFINED;

  struct FixedData
  {
    unsigned int count = 0; // total particle count
  };

  struct BurstData
  {
    unsigned int count = 0;     // per burst
    unsigned int cycles = 0;    // max burst cycles (0 means no limit)
    float period = 0;           // burst period
    float lifeLimit = 0;        // negative = immortal
    unsigned int elemLimit = 0; // if elems reach this value, emission will be suspended. Can be 0, which means it calculate itself.
  };

  struct LinearData
  {
    unsigned int count = 0; // total particle count
    float lifeLimit = 0;    // must be > 0
  };

  float delay = 0; // delay for the first burst

  FixedData fixedData;
  BurstData burstData;
  LinearData linearData;
};

struct RenderDesc
{
  uint32_t tag;
  eastl::string shader;
};

struct ParticleGroupDesc
{
  EmissionData emissionData;
  eastl::string emissionShader;   // optional
  eastl::string simulationShader; // optional
  eastl::vector<RenderDesc> rendersDesc;
  eastl::vector<TEXTUREID> texturesVs;
  eastl::vector<TEXTUREID> texturesPs;
  eastl::vector<TEXTUREID> texturesCs;

  unsigned int elemSize = 0; // 1 element size (in float4)
};

struct EmitterDesc
{
  eastl::string emissionShader;   // optional
  eastl::string simulationShader; // optional

  eastl::vector<float> params; // can be rewritten with emission/simulation
  eastl::vector<ParticleGroupDesc> particleGroupsDesc;
};

struct SystemDesc
{
  SystemDesc();

  bbox3f localBbox;
  EmissionData emissionData;
  eastl::vector<EmitterDesc> emittersDesc;
  eastl::vector<float> params;

  int posParamId = -1; // system param idx for position (optional)
};

ContextId create_context();
void release_context(ContextId cid);
void on_reset_device(ContextId cid);

SystemId register_system(ContextId cid, const SystemDesc &desc, const eastl::string &name);
void release_system(ContextId cid, SystemId sid);

InstanceId create_instance(ContextId cid, SystemId sid);
InstanceId create_instance(ContextId cid, eastl::string &name);
void destroy_instance(ContextId cid, InstanceId iid);

CullingId create_culling_state(ContextId cid);
void destroy_culling_state(ContextId ctx_id, CullingId cull_id);

int get_system_ref_count(ContextId cid, SystemId sid);
int get_active_system_ref_count(ContextId cid, SystemId sid);

void register_global_param_key(ContextId cid, const eastl::string &key, size_t key_id);
void register_system_param_key(ContextId cid, SystemId sid, const eastl::string &key, size_t key_id);

void set_global_param(ContextId cid, size_t key_id, float v);
void set_global_params(ContextId cid, size_t key_id, const float *values, size_t values_count);
void set_global_param(ContextId cid, const eastl::string &key, float v);
void set_global_params(ContextId cid, const eastl::string &key, const float *values, size_t values_count);
void set_global_param4(ContextId cid, size_t key_id, float v0, float v1, float v2, float v3);
void set_global_param4f(ContextId cid, size_t key_id, const Point4 &v);
void set_global_param4v(ContextId cid, size_t key_id, const vec4f &v);

// float get_global_param( ContextId cid, const eastl::string &key );
// float get_global_param( ContextId cid, size_t key_id );
// void get_global_param( ContextId cid, const eastl::string &key, float *values, size_t values_count );
// void get_global_param( ContextId cid, size_t key_id, float *values, size_t values_count );

void set_global_tex_ps(ContextId cid, int slot, BaseTexture *tex);
void set_global_tex_vs(ContextId cid, int slot, BaseTexture *tex);
void set_global_tex_cs(ContextId cid, int slot, BaseTexture *tex);

// void set_instance_pos( ContextId cid, InstanceId iid, const vec4f &v );
void set_instance_status(ContextId cid, InstanceId iid, bool enabled);

void set_instance_param(ContextId cid, InstanceId iid, const eastl::string &key, float v);
void set_instance_params(ContextId cid, InstanceId iid, const eastl::string &key, const float *values, size_t values_count);
void set_instance_param(ContextId cid, InstanceId iid, size_t key_id, float v);
void set_instance_params(ContextId cid, InstanceId iid, size_t key_id, const float *values, size_t values_count);
void set_instance_param4(ContextId cid, InstanceId iid, size_t key_id, float v0, float v1, float v2, float v3);
void set_instance_param4f(ContextId cid, size_t key_id, const Point4 &v);
void set_instance_param4v(ContextId cid, InstanceId iid, size_t key_id, const vec4f &v);

// float get_instance_param( ContextId cid, InstanceId iid, const eastl::string &key );
// float get_instance_param( ContextId cid, InstanceId iid, size_t key_id );
// void get_instance_param( ContextId cid, const eastl::string &key, float *values, size_t values_count );
// void get_instance_param( ContextId cid, size_t key_id, float *values, size_t values_count );

void update(ContextId cid, float dt);
void update_culling_state(ContextId ctx_id, CullingId cull_id, const vec4f &view_pos, const Frustum &frustum);
void render(ContextId ctx_id, CullingId cull_id, uint32_t tag);

// TODO: this is temporary helpers for cross acesfx and pfx render and sorting
// after current cpu-effects will migrate to pfx, remove it (it is very very very slow to render per instance)
int render_task_begin(ContextId ctx_id, CullingId cull_id);
int render_task_end(ContextId ctx_id, CullingId cull_id);
vec4f render_task_pos(ContextId ctx_id, CullingId cull_id, int task_id);
void render_task_perform(ContextId ctx_id, CullingId cull_id, int task_id, uint32_t tag);
} // namespace pfx
