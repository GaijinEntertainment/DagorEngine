#define PACK_R32G32B32_TGSM 0
#define PACK_R11G11B10_TGSM 1
#define PACK_R16G16B16_TGSM 2
#define PACK_R16G16B16_TGSM_TEST 3
#define PACK_TGSM PACK_R32G32B32_TGSM//PACK_R11G11B10_TGSM//PACK_R16G16B16_TGSM//
#if PACK_TGSM == PACK_R11G11B10_TGSM || PACK_TGSM == PACK_R16G16B16_TGSM_TEST
  
  #if PACK_TGSM == PACK_R11G11B10_TGSM
  #define encode_tgsm_color(a) encode_color(a)
  #define decode_tgsm_color(a) decode_color(a)
  groupshared uint thread_result[AVERAGE_CUBE_WARP_SIZE*6];
  #else
  uint3 encode_tgsm_color(float3 c0) { return f32tof16(c0); }
  float3 decode_tgsm_color(uint3 c0) { return f16tof32(c0); }
  groupshared uint3 thread_result[AVERAGE_CUBE_WARP_SIZE*6];
  #endif
  void average_tgsm(uint a, uint b)
  {
    a*=6;
    b = a + b*6;
    thread_result[a+0] = encode_tgsm_color(0.5*decode_tgsm_color(thread_result[a+0]) + 0.5*decode_tgsm_color(thread_result[b+0]));
    thread_result[a+1] = encode_tgsm_color(0.5*decode_tgsm_color(thread_result[a+1]) + 0.5*decode_tgsm_color(thread_result[b+1]));
    thread_result[a+2] = encode_tgsm_color(0.5*decode_tgsm_color(thread_result[a+2]) + 0.5*decode_tgsm_color(thread_result[b+2]));
    thread_result[a+3] = encode_tgsm_color(0.5*decode_tgsm_color(thread_result[a+3]) + 0.5*decode_tgsm_color(thread_result[b+3]));
    thread_result[a+4] = encode_tgsm_color(0.5*decode_tgsm_color(thread_result[a+4]) + 0.5*decode_tgsm_color(thread_result[b+4]));
    thread_result[a+5] = encode_tgsm_color(0.5*decode_tgsm_color(thread_result[a+5]) + 0.5*decode_tgsm_color(thread_result[b+5]));
  }
  void init_tgsm(uint a, float3 c0, float3 c1, float3 c2, float3 c3, float3 c4, float3 c5)
  {
    a*=6;
    thread_result[a+0] = encode_tgsm_color(c0);
    thread_result[a+1] = encode_tgsm_color(c1);
    thread_result[a+2] = encode_tgsm_color(c2);
    thread_result[a+3] = encode_tgsm_color(c3);
    thread_result[a+4] = encode_tgsm_color(c4);
    thread_result[a+5] = encode_tgsm_color(c5);
  }
  void read_tgsm(float numRays, out float3 c0, out float3 c1, out float3 c2, out float3 c3, out float3 c4, out float3 c5)
  {
    c0 = 4*decode_tgsm_color(thread_result[0]);
    c1 = 4*decode_tgsm_color(thread_result[1]);
    c2 = 4*decode_tgsm_color(thread_result[2]);
    c3 = 4*decode_tgsm_color(thread_result[3]);
    c4 = 4*decode_tgsm_color(thread_result[4]);
    c5 = 4*decode_tgsm_color(thread_result[5]);
  }
#elif PACK_TGSM == PACK_R16G16B16_TGSM
  
  groupshared uint3 thread_result[AVERAGE_CUBE_WARP_SIZE*3];

  void init_tgsm(uint a, float3 c0, float3 c1, float3 c2, float3 c3, float3 c4, float3 c5)
  {
    a*=3;
    thread_result[a+0].xyz = encode_two_colors16(c0,c1);
    thread_result[a+1].xyz = encode_two_colors16(c2,c3);
    thread_result[a+2].xyz = encode_two_colors16(c4,c5);
  }
  void read_tgsm(float numRays, out float3 c0, out float3 c1, out float3 c2, out float3 c3, out float3 c4, out float3 c5)
  {
    decode_two_colors16(thread_result[0], c0, c1);
    c0 *= 4; c1 *= 4;
    decode_two_colors16(thread_result[1], c2, c3);
    c2 *= 4; c3 *= 4;
    decode_two_colors16(thread_result[2], c4, c5);
    c4 *= 4; c5 *= 4;
  }
  void average_tgsm(uint a, uint b)
  {
    a*=3;
    b = a + b*3;
    float3 ac0,ac1;
    float3 bc0,bc1;
    decode_two_colors16(thread_result[a+0], ac0, ac1);decode_two_colors16(thread_result[b+0], bc0, bc1);
    thread_result[a+0] = encode_two_colors16((ac0+bc0)*0.5, (ac1+bc1)*0.5);
    
    decode_two_colors16(thread_result[a+1], ac0, ac1);decode_two_colors16(thread_result[b+1], bc0, bc1);
    thread_result[a+1] = encode_two_colors16((ac0+bc0)*0.5, (ac1+bc1)*0.5);

    decode_two_colors16(thread_result[a+2], ac0, ac1);decode_two_colors16(thread_result[b+2], bc0, bc1);
    thread_result[a+2] = encode_two_colors16((ac0+bc0)*0.5, (ac1+bc1)*0.5);
  }
#elif PACK_TGSM == PACK_R32G32B32_TGSM
  groupshared float3 thread_result[AVERAGE_CUBE_WARP_SIZE*6];
  void average_tgsm(uint a, uint b)
  {
    a*=6;
    b = a + b*6;
    thread_result[a+0] += thread_result[b+0];
    thread_result[a+1] += thread_result[b+1];
    thread_result[a+2] += thread_result[b+2];
    thread_result[a+3] += thread_result[b+3];
    thread_result[a+4] += thread_result[b+4];
    thread_result[a+5] += thread_result[b+5];
  }
  void init_tgsm(uint a, float3 c0, float3 c1, float3 c2, float3 c3, float3 c4, float3 c5)
  {
    a *= 6;
    thread_result[a+0] = c0;
    thread_result[a+1] = c1;
    thread_result[a+2] = c2;
    thread_result[a+3] = c3;
    thread_result[a+4] = c4;
    thread_result[a+5] = c5;
  }
  void read_tgsm(float w, out float3 c0, out float3 c1, out float3 c2, out float3 c3, out float3 c4, out float3 c5)
  {
    uint a = 0;
    c0 = thread_result[a*6+0]*w;
    c1 = thread_result[a*6+1]*w;
    c2 = thread_result[a*6+2]*w;
    c3 = thread_result[a*6+3]*w;
    c4 = thread_result[a*6+4]*w;
    c5 = thread_result[a*6+5]*w;
  }
#endif

#define AVERAGE(b2) average_tgsm(tid, b2);

#define PARALLEL_CUBE_AVERAGE \
{\
  init_tgsm(tid, col0, col1, col2, col3, col4, col5);\
  GroupMemoryBarrierWithGroupSync();\
  if (AVERAGE_CUBE_WARP_SIZE >= 256) { if (tid < 128) { AVERAGE(128) } GroupMemoryBarrierWithGroupSync();}\
  if (AVERAGE_CUBE_WARP_SIZE >= 128) { if (tid < 64) { AVERAGE(64) } GroupMemoryBarrierWithGroupSync();}\
  {\
    if (AVERAGE_CUBE_WARP_SIZE >= 64) { if (tid < 32) { AVERAGE(32)} }\
    if (AVERAGE_CUBE_WARP_SIZE >= 32) { if (tid < 16) { AVERAGE(16)} }\
    if (AVERAGE_CUBE_WARP_SIZE >= 16) { if (tid <  8) { AVERAGE(8) } }\
    if (AVERAGE_CUBE_WARP_SIZE >= 8)  { if (tid <  4) { AVERAGE(4) } }\
    if (AVERAGE_CUBE_WARP_SIZE >= 4)  { if (tid <  2) { AVERAGE(2) } }\
  }\
  if (tid == 0)\
  {\
    AVERAGE(1)\
    read_tgsm(parallel_weight, col0, col1, col2, col3, col4, col5);\
  }\
}