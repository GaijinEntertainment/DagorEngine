#ifndef BEAM_TRACERS_HLSLI_INCLUDED
#define BEAM_TRACERS_HLSLI_INCLUDED

#define MAX_TRACERS 2048

#define TRACER_RIBBON_SEGMENTS_COUNT 512
#define TRACER_SEGMENTS_COUNT 16
#define TRACER_COMMAND_WARP_SIZE 16
#define TRACER_CULL_WARP_SIZE 16
#define CONST_BUF_MAX_SIZE 4096 /* in float4 */
#define TRACER_CMD_BUF_MAX_SIZE (CONST_BUF_MAX_SIZE-4)
#define TRACER_BEAM_UPDATE_COMMAND_SIZE 2 /* BeamTracerUpdateCommand in float4 */
#define TRACER_BEAM_MAX_UPDATE_COMMANDS (TRACER_CMD_BUF_MAX_SIZE / TRACER_BEAM_UPDATE_COMMAND_SIZE)
#define TRACER_BEAM_CREATE_COMMAND_SIZE 4 /* BeamTracerCreateCommand in float4 */
#define TRACER_BEAM_MAX_CREATE_COMMANDS (TRACER_CMD_BUF_MAX_SIZE / TRACER_BEAM_CREATE_COMMAND_SIZE)
#define TRACER_MAX_CULL_COMMANDS (CONST_BUF_MAX_SIZE-14)

#ifdef __cplusplus
#pragma pack(push, 1)
#endif

struct GPUBeamTracer//todo: compress.
{
  float3 p0;
  float pad0;

  float3 dir; //changes on create only, needed for update&culling
  float pad1;

  float3 headColor;//changes on create only, needed for culling only (creates head)
  float burnTime;// for head

  float4 smoke_color_density;//changes on create only, needed for culling only(creates tail)

  float ttl, radiusStart;//changes on create only, needed for update&culling
  float startTime;//changes on create only, needed for update&culling
  float beamFadeDist;
  float beamBeginFadeTime;
  float beamEndFadeTime;

  uint isRay;
  float beamScrollSpeed;
};

// TODO: do we need this? only need vertices if we want to support multi-segment lines (e.g. ricochet)
struct GPUBeamTracerVertices//todo: compress. needed for rendering
{
  float4 pos_dist;
  float time;
  float padding0;
  float padding1;
  float padding2;
};

struct GPUBeamTracerDynamic
{
  float3 movingDir;//changes on update, needed for update (normalized val)
  uint firstVert_totalVerts;//changes on update/culling, needed on update&culling
  float radius, len;//changes on update, needed for culling
  float beamFadeDist;
  float padding1;
};

struct BeamTracerUpdateCommand
{
  float3 pos;
  uint id;
  float3 startPos;
  float padding;
  #ifdef __cplusplus
  BeamTracerUpdateCommand() = default;
  BeamTracerUpdateCommand(int id_, const Point3 &p, const Point3 &start_pos):id(id_), pos(p), startPos(start_pos){}
  #endif
};

struct GPUBeamTracerTailRender//todo: compress me!
{
  float4 smoke_color_density;//can be compressed
  float ttl, radiusStart;//can be compressed
  float startTime;
  uint firstVert_totalVerts;
  uint id;
  float padding0;
  float padding1;
  float padding2;
};

struct GPUBeamTracerHeadRender//todo: compress me!
{
  float3 p0;
  float radius0;//can be compressed
  float3 p1;
  float radius1;
  float4 color;//can be compressed
  float resv1; //padding for 16 byte alignment
  float beamScrollSpeed;
  float fadeTimeRatio;
  float beamFadeDist;
};

struct BeamTracerCreateCommand
{
  float3 pos0; float ttl;
  float3 dir; uint id;
  float3 headColor; float burnTime;
  float beamBeginFadeTime;
  float beamEndFadeTime;
  float beamScrollSpeed;
  float beamFadeDist;
  #ifdef __cplusplus
  BeamTracerCreateCommand() = default;
  BeamTracerCreateCommand(int id_, const float3 &p0, const float3 &d, float ttl_,
                          const float3 &head_color, float burn_time, float fade_dist,
                          float begin_fade_time, float end_fade_time, float scroll_speed):
    id(id_), pos0(p0), dir(d), ttl(ttl_), headColor(head_color), burnTime(burn_time),
    beamFadeDist(fade_dist), beamBeginFadeTime(begin_fade_time), beamEndFadeTime(end_fade_time),
    beamScrollSpeed(scroll_speed)
    {}
  #endif
};

#ifndef __cplusplus
float get_tracers_max_turbulence_radius(float radiusStart) {return (radiusStart*2+0.25);}

#define TRACERS_WIND_RADIUS_STR 0.1
float get_tracers_max_wind_radius(float radiusStart){return TRACERS_WIND_RADIUS_STR*radiusStart;}

float3 get_tracers_wind(float time)
{
  return -time*TRACERS_WIND_RADIUS_STR*float3(1,0,1);//todo: unified wind solution (particles, volumetrics, tracers)
}

#endif

#ifdef __cplusplus
#pragma pack(pop)
#endif

#endif