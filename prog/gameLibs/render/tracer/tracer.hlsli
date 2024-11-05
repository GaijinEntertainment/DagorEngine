struct TailParticle
{
  float2 perlin;
  float rv;
  float pad;
};

struct GPUFxTracer
{
  float4 tailColor;
  float2 tailParticleHalfSize;
  float tailDeviation;
  float unused0;
};

struct GPUFxTracerDynamic
{
  float3 pos;
  int partCountSegmentId;
  float3 pad;
  float amplifyScale;
};

// have to pad it because on metal float3 is 16-byte aligned, while in c++ counter part its not
struct GPUFxSegment
{
  float3 worldPos;
  float pad;
};

// have to pad it because on metal float3 is 16-byte aligned, while in c++ counter part its not
struct GPUFxSegmentProcessed
{
  float3 worldPos;
  float pad0;
  float3 worldDir;
  float pad1;
  float3 tmX;
  float pad2;
  float3 tmY;
  float pad3;
};

struct GPUFXHead
{
  int tracerId;
  int segmentId;
  int tracerType;
  float opacity;
  float lenScale;
  float time;
};

struct GPUFxTracerCreate
{
  GPUFxTracer data;
  uint id;
  float pad0;
  float pad1;
  float pad2;
};

// have to pad it because on metal float3 is 16-byte aligned, while in c++ counter part its not
struct GPUFxSegmentCreate
{
  GPUFxSegment data;
  uint id;
  float pad0;
  float pad1;
  float pad2;
};

struct GPUFXTracerType
{
  float3 tracerStartColor1;
  float tracerHeadTaper;
  float3 tracerStartColor2;
  float tracerStartTime;
  float4 tracerHeadColor1;
  float3 tracerHeadColor2;
  float headHalfSize;

  float headRadius;
  float beam;
  float minPixelSize;
  float pad1;
};

struct GPUFXHeadProcessed
{
  float3 worldDir;
  float worldSize;
  float3 worldPos;
  float radius;
  float3 tracerStartColor1;
  float3 tracerStartColor2;
  float tracerStartTime;
  float4 tracerHeadColor1;
  float3 tracerHeadColor2;
  float beam;
  float minPixelSize;
  float time;
  float tracerHeadTaper;
};



#define GET_FX_TRACER(instance_id, tail_len, num_part, tracer_buffer, tracer_dynamic_buffer, segment_buffer, tracer, tracerDynamic, segment) \
{ \
  uint tracerId = instance_id / MAX_FX_SEGMENTS; \
  tracer = tracer_buffer[tracerId]; \
  tracerDynamic = tracer_dynamic_buffer[tracerId]; \
  uint segmId = instance_id % MAX_FX_SEGMENTS; \
  segment.worldPos = segment_buffer[instance_id].worldPos; \
  int partCount = tracerDynamic.partCountSegmentId % FX_TRACER_PACK_BIT; \
  int segmentId = tracerDynamic.partCountSegmentId / FX_TRACER_PACK_BIT; \
  BRANCH \
  if (segmId == (segmentId % FX_TRACER_BITS_SEGMENT)) \
  { \
    segment.worldDir = tracerDynamic.pos - segment.worldPos; \
    float segmLen = length(segment.worldDir); \
    segment.worldDir /= segmLen; \
    segment.worldPos += max(segmLen - tail_len, (float)0) * segment.worldDir; \
  } \
  else \
  { \
    segment.worldDir = normalize(segment_buffer[instance_id + 1].worldPos - segment.worldPos); \
    if (segmId == (segmentId / FX_TRACER_BITS_SEGMENT)) \
      segment.worldPos = segment_buffer[instance_id + 1].worldPos - segment.worldDir * tail_len * (partCount / FX_TRACER_BITS_PART_COUNT) / num_part; \
  } \
  segment.tmX = tracer.tailDeviation * normalize(cross(segment.worldDir, float3(0.f, 1.f, 0.f))); \
  segment.tmY = tracer.tailDeviation * normalize(cross(segment.worldDir, segment.tmX)); \
}

#define UNPACK_FX_TRACER(td, tracer, tracerDynamic, segment, part_offs) \
{ \
  tracer.tailColor = td[0]; \
  tracer.tailParticleHalfSize = float2(td[1].x, td[1].y); \
  tracer.tailDeviation = td[1].z; \
  part_offs = td[1].w; \
  tracerDynamic.pos = float3(td[2].x, td[2].y, td[2].z); \
  tracerDynamic.partCountSegmentId = td[2].w; \
  segment.worldPos = float3(td[3].x, td[3].y, td[3].z); \
  segment.worldDir = float3(td[4].x, td[4].y, td[4].z); \
  segment.tmX = float3(td[5].x, td[5].y, td[3].w); \
  segment.tmY = float3(td[5].z, td[5].w, td[4].w); \
}

#define PACK_FX_TRACER(td, tracer, tracerDynamic, segment, part_offs) \
{ \
  td[0] = tracer.tailColor; \
  td[1] = float4(tracer.tailParticleHalfSize.x, tracer.tailParticleHalfSize.y, tracer.tailDeviation, part_offs); \
  td[2] = float4(tracerDynamic.pos.x, tracerDynamic.pos.y, tracerDynamic.pos.z, tracerDynamic.partCountSegmentId); \
  td[3] = float4(segment.worldPos.x, segment.worldPos.y, segment.worldPos.z, segment.tmX.z); \
  td[4] = float4(segment.worldDir.x, segment.worldDir.y, segment.worldDir.z, segment.tmY.z); \
  td[5] = float4(segment.tmX.x, segment.tmX.y, segment.tmY.x, segment.tmY.y); \
}

#define PACK_FX_HEAD(hd, id, head) \
{ \
  int offset = id * FX_HEAD_H_NUM_REGISTERS; \
  int idSegment = (head.tracerId + head.tracerType * FX_HEAD_BITS_TRACER) * FX_HEAD_BITS_SEGMENT + head.segmentId; \
  hd[offset + 0] = float4(idSegment, head.time, head.opacity, head.lenScale); \
}

#define UNPACK_FX_HEAD(hd, id, head) \
{ \
  int offset = id * FX_HEAD_H_NUM_REGISTERS; \
  int idSegment = hd[offset + 0].x; \
  int idTracer = idSegment / FX_HEAD_BITS_SEGMENT; \
  head.tracerId = idTracer % FX_HEAD_BITS_TRACER; \
  head.segmentId = idSegment % FX_HEAD_BITS_SEGMENT; \
  head.tracerType = idTracer / FX_HEAD_BITS_TRACER; \
  head.time = hd[offset + 0].y; \
  head.opacity = hd[offset + 0].z; \
  head.lenScale = hd[offset + 0].w; \
}

#define GET_FX_HEAD_PROCESSED(head_h, tracer_type_buffer, tracer_dynamic_buffer, segment_buffer, head) \
{ \
  GPUFXTracerType ht = tracer_type_buffer[head_h.tracerType]; \
  GPUFxTracerDynamic tracerDynamic = tracer_dynamic_buffer[head_h.tracerId]; \
  head.worldDir = normalize(tracerDynamic.pos - segment_buffer[head_h.tracerId * MAX_FX_SEGMENTS + head_h.segmentId].worldPos); \
  head.worldSize = ht.headHalfSize * head_h.lenScale; \
  head.worldPos = tracerDynamic.pos - head.worldDir * head.worldSize; \
  head.radius = ht.headRadius * tracerDynamic.amplifyScale; \
  head.tracerHeadColor1 = ht.tracerHeadColor1 * tracerDynamic.amplifyScale; \
  head.tracerHeadColor2 = ht.tracerHeadColor2 * tracerDynamic.amplifyScale; \
  head.tracerHeadColor1.w *= head_h.opacity; \
  head.beam = ht.beam; \
  head.minPixelSize = ht.minPixelSize * tracerDynamic.amplifyScale; \
  head.time = head_h.time; \
  head.tracerStartColor1 = ht.tracerStartColor1 * tracerDynamic.amplifyScale; \
  head.tracerStartColor2 = ht.tracerStartColor2 * tracerDynamic.amplifyScale; \
  head.tracerStartTime = ht.tracerStartTime; \
  head.tracerHeadTaper = ht.tracerHeadTaper; \
}

#define PACK_FX_HEAD_PROCESSED(hd, id, head) \
{ \
  int offset = id * FX_HEAD_NUM_REGISTERS; \
  hd[offset + 0] = float4(head.worldPos.x, head.worldPos.y, head.worldPos.z, head.worldSize); \
  hd[offset + 1] = float4(head.worldDir.x, head.worldDir.y, head.worldDir.z, head.radius); \
  hd[offset + 2] = head.tracerHeadColor1; \
  hd[offset + 3] = float4(head.tracerHeadColor2.x, head.tracerHeadColor2.y, head.tracerHeadColor2.z, head.beam); \
  hd[offset + 4] = float4(head.minPixelSize, head.time, 0, 0); \
  if (FX_HEAD_NUM_REGISTERS >= 7) \
  { \
    hd[offset + 5] = float4(head.tracerStartColor1.x, head.tracerStartColor1.y, head.tracerStartColor1.z, head.tracerHeadTaper); \
    hd[offset + 6] = float4(head.tracerStartColor2.x, head.tracerStartColor2.y, head.tracerStartColor2.z, head.tracerStartTime); \
  } \
}

#define UNPACK_FX_HEAD_PROCESSED(hd, id, head) \
{ \
  int offset = id * FX_HEAD_NUM_REGISTERS; \
  head.worldPos = hd[offset + 0].xyz; \
  head.worldSize = hd[offset + 0].w; \
  head.worldDir = hd[offset + 1].xyz; \
  head.radius = hd[offset + 1].w; \
  head.tracerHeadColor1 = hd[offset + 2]; \
  head.tracerHeadColor2 = hd[offset + 3].xyz; \
  head.beam = hd[offset + 3].w; \
  head.minPixelSize = hd[offset + 4].x; \
  head.time = hd[offset + 4].y; \
  if (FX_HEAD_NUM_REGISTERS >= 7) \
  { \
    head.tracerStartColor1 = hd[offset + 5].xyz; \
    head.tracerHeadTaper = hd[offset + 5].w; \
    head.tracerStartColor2 = hd[offset + 6].xyz; \
    head.tracerStartTime = hd[offset + 6].w; \
  } \
}
