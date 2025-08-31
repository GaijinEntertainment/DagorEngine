// This is for DNG only, but it is required for NBS, so it needs to stay in gameLibs

#ifndef RW_PREFIX
  #error RW_PREFIX needs to be defined to either be empty, or have globallycoherent!
#endif

#define GBUFFER_RW_DEFINED // Used for NBS error msg creation

RW_PREFIX RWTexture2D<float4>  NBSGbuf0 : register(u4);
RW_PREFIX RWTexture2D<float4>  NBSGbuf1 : register(u5);
RW_PREFIX RWTexture2D<float4>  NBSGbuf2 : register(u6);
RW_PREFIX RWTexture2D<float4>  NBSGbuf3 : register(u7);

void writeGbuffAlbedoAO(uint2 tc, half4 albedo_ao) { NBSGbuf0[tc] = albedo_ao; }
void writeGbuffNormalMat(uint2 tc, float4 normal_mat) { NBSGbuf1[tc] = normal_mat; }
void writeGbuffReflectance(uint2 tc, half4 reflectance) { NBSGbuf2[tc] = reflectance; }
void writeGbuffMotionVecs(uint2 tc, half4 motion_vecs) { NBSGbuf3[tc] = motion_vecs; }
float4 readGbuffAlbedoAO(uint2 tc) { return NBSGbuf0[tc]; }
float4 readGbuffNormalMat(uint2 tc) { return NBSGbuf1[tc]; }
float4 readGbuffReflectance(uint2 tc) { return NBSGbuf2[tc]; }
float4 readGbuffMotionVecs(uint2 tc) { return NBSGbuf3[tc]; }