#define MAX_EMITTER_GEN_COMMANDS ((512-8)/4)
#define EMIT_PER_FRAME_LIMIT 4
#define EMITTER_GEN_WARP 128
#define EMIT_WARP 128
#define UPDATE_WARP 128

#define EMITTER_BUFFER_REGISTER 0
#define EMITTER_DYN_BUFFER_REGISTER 2
#define RANDOM_BUFFER_REGISTER 1
#define PARTICLE_BUFFER_REGISTER 1
#define PARTICLE_RENDER_BUFFER_REGISTER 3
#define PARTICLE_RENDER_BUFFER_REGISTER_FOR_UPDATE 2
#define INDIRECT_REGISTER 3

#define VERTICES_PER_PARTICLE 4
#define INDICIES_PER_PARTICLE 6
#define RANDOM_BUFFER_RESOLUTION_X 256
#define RANDOM_BUFFER_RESOLUTION_Y 2

#define MOD_VELOCITY_FLAG 0x1
#define MOD_MATERIAL_FLAG 0x2

struct GPUEmitter
{
  // reg0
  float lifeTime;
  float lifeDelay;
  float lifeSpread;
  float velSpread;
  // reg1
  float radiusSpread;
  float dirSpread;
  float posSpread;
  float rotSpread;
  // reg2
  float2 scaleXY;
  float radius;
  float radiusScale;
  // reg3
  uint2 uvNumFrames;
  float velRes;
  uint activeFlags;
  // reg4
  float4 startColor;
  // reg5
  float4 endColor;
  // reg6
  float4 uv;
};

struct GPUEmitterGen
{
  GPUEmitter emitter;
  uint emitterId;
  float unused0;
  float unused1;
  float unused2;
};

struct GPUEmitterDynamic
{
  // reg0
  float2 pos;
  uint id_coff;
  float rot;
  // reg1
  float2 dir;
  float vel;
  float alpha;
  // reg2
  uint startSlot;
  uint numSlots;
  float scaleZ;
  uint unused;
};

struct GPUParticle
{
  uint emitterId;
  float timeLeft;
  float lifeTime;
  float radiusVar;
  float3 pos;
  float rot;
  float3 dir;
  float vel;
  float alpha;
  float vel0;
  float scaleZ;
  float unused;
};

struct GPUParticleRender
{
  float3 pos;
  float tAlpha;
  float2 dirX;
  float2 dirY;
  float4 color;
  float4 uv;
  float4 uv2;
  float3 scale;
  float fAlpha;
};

#define PACK_ID_OFFSET_NUM(out_data, id, offset, num) \
{ \
  out_data = (id << 22) + (offset << 11) + num; \
}

#define UNPACK_ID_OFFSET_NUM(in_data, id, offset, num) \
{ \
  id = in_data >> 22; \
  offset = (in_data >> 11) & 0x7FF; \
  num = in_data & 0x7FF; \
}
