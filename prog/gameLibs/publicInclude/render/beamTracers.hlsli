#define MAX_TRACERS 2048

#define TRACER_RIBBON_SEGMENTS_COUNT 512
#define TRACER_SEGMENTS_COUNT 16
#define TRACER_COMMAND_WARP_SIZE 16
#define TRACER_CULL_WARP_SIZE 16
#define TRACER_BEAM_MAX_UPDATE_COMMANDS ((4096-4) / 2)
#define TRACER_MAX_CREATE_COMMANDS ((4096-4)/4)
#define TRACER_MAX_CULL_COMMANDS (4096-14)

#ifndef GPU_TARGET
#pragma pack(push, 1)
#endif

struct GPUBeamTracer//todo: compress.
{
  float3 p0;
  float pad0;

  float3 dir; //changes on create only, needed for update&culling
  float pad1;

  float3 head_color;//changes on create only, needed for culling only (creates head)
  float burnTime;// for head

  float4 smoke_color_density;//changes on create only, needed for culling only(creates tail)

  float ttl, radiusStart;//changes on create only, needed for update&culling
  float startTime;//changes on create only, needed for update&culling
  float fadeDist;
  float begin_fade_time;
  float end_fade_time;

  uint isRay;
  float scroll_speed;
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
  float fadeDist;
  float padding1;
};

struct BeamTracerUpdateCommand
{
  float3 pos;
  uint id;
  float3 startPos;
  float padding;
  #ifndef GPU_TARGET
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
  float scrollSpeed;
  float fadeTimeRatio;
  float fadeDist;
};

struct BeamTracerCreateCommand
{
  float3 pos0; float ttl;
  float3 dir; uint id;
  float4 head_color__burnTime;
  float begin_fade_time;
  float end_fade_time;
  float scroll_speed;
  float fade_dist;
  #ifndef GPU_TARGET
  BeamTracerCreateCommand() = default;
  BeamTracerCreateCommand(int id_, const float3 &p0, const float3 &d, float ttl_,
                          const float4 &head_color_, float fade_dist_,
                          float begin_fade_time_, float end_fade_time_, float scroll_speed_):
    id(id_), pos0(p0), dir(d), ttl(ttl_), head_color__burnTime(head_color_),
    fade_dist(fade_dist_), begin_fade_time(begin_fade_time_), end_fade_time(end_fade_time_),
    scroll_speed(scroll_speed_)
    {}
  #endif
};

#ifdef GPU_TARGET
float get_tracers_max_turbulence_radius(float radiusStart) {return (radiusStart*2+0.25);}

#define TRACERS_WIND_RADIUS_STR 0.1
float get_tracers_max_wind_radius(float radiusStart){return TRACERS_WIND_RADIUS_STR*radiusStart;}

float3 get_tracers_wind(float time)
{
  return -time*TRACERS_WIND_RADIUS_STR*float3(1,0,1);//todo: unified wind solution (particles, volumetrics, tracers)
}

#endif

#ifndef GPU_TARGET
#pragma pack(pop)
#endif
