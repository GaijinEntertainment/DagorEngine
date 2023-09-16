#ifndef  shadow_array_supported
#define shadow_array_supported 0
#endif

#ifndef MIN_SHADOW_SIZE
  #define MIN_SHADOW_SIZE 512
#endif

#define HALF (float3(0.5-2./MIN_SHADOW_SIZE, 0.5-2./MIN_SHADOW_SIZE, 0.5))//0.5-2/512 (512 is our smallest cascade size)

#ifndef LAST_CASCADE
#define LAST_CASCADE NUM_CASCADES
#endif

float get_csm_shadow_effect( uint cascade_id, float3 t0, float3 t1, float3 t2, float3 t3, float3 t4, float3 t5 )
{
  float csmEffect = 0;
  csmEffect = (LAST_CASCADE==6 && cascade_id == LAST_CASCADE-1) ? max3(abs(t5.x), abs(t5.y), abs(t5.z))*2 : csmEffect;
  csmEffect = (LAST_CASCADE==5 && cascade_id == LAST_CASCADE-1) ? max3(abs(t4.x), abs(t4.y), abs(t4.z))*2 : csmEffect;
  csmEffect = (LAST_CASCADE==4 && cascade_id == LAST_CASCADE-1) ? max3(abs(t3.x), abs(t3.y), abs(t3.z))*2 : csmEffect;
  csmEffect = (LAST_CASCADE==3 && cascade_id == LAST_CASCADE-1) ? max3(abs(t2.x), abs(t2.y), abs(t2.z))*2 : csmEffect;
  csmEffect = (LAST_CASCADE==2 && cascade_id == LAST_CASCADE-1) ? max3(abs(t1.x), abs(t1.y), abs(t1.z))*2 : csmEffect;
  csmEffect = (LAST_CASCADE==1 && cascade_id == LAST_CASCADE-1) ? max3(abs(t0.x), abs(t0.y), abs(t0.z))*2 : csmEffect;
  return csmEffect;
}

float3 get_csm_shadow_tc(float3 pointToEye, float sel_scale, out uint cascade_id, out float csmEffect, out float3 tlast)
{
  pointToEye = -pointToEye;
  //to be moved out to const buffer
  float3 t0,t1,t2,t3,t4,t5;
    t0 = pointToEye.x*shadow_cascade_tm_transp[4*0+0].xyz
       + pointToEye.y*shadow_cascade_tm_transp[4*0+1].xyz
       + pointToEye.z*shadow_cascade_tm_transp[4*0+2].xyz
       + shadow_cascade_tm_transp[4*0+3].xyz;
  #if NUM_CASCADES>1
    t1 = pointToEye.x*shadow_cascade_tm_transp[4*1+0].xyz
       + pointToEye.y*shadow_cascade_tm_transp[4*1+1].xyz
       + pointToEye.z*shadow_cascade_tm_transp[4*1+2].xyz
       + shadow_cascade_tm_transp[4*1+3].xyz;
  #endif
  #if NUM_CASCADES>2
    t2 = pointToEye.x*shadow_cascade_tm_transp[4*2+0].xyz
       + pointToEye.y*shadow_cascade_tm_transp[4*2+1].xyz
       + pointToEye.z*shadow_cascade_tm_transp[4*2+2].xyz
       + shadow_cascade_tm_transp[4*2+3].xyz;
  #endif
  #if NUM_CASCADES>3
    t3 = pointToEye.x*shadow_cascade_tm_transp[4*3+0].xyz
       + pointToEye.y*shadow_cascade_tm_transp[4*3+1].xyz
       + pointToEye.z*shadow_cascade_tm_transp[4*3+2].xyz
       + shadow_cascade_tm_transp[4*3+3].xyz;
  #endif
  #if NUM_CASCADES>4
    t4 = pointToEye.x*shadow_cascade_tm_transp[4*4+0].xyz
       + pointToEye.y*shadow_cascade_tm_transp[4*4+1].xyz
       + pointToEye.z*shadow_cascade_tm_transp[4*4+2].xyz
       + shadow_cascade_tm_transp[4*4+3].xyz;
  #endif
  #if NUM_CASCADES>5
    t5 = pointToEye.x*shadow_cascade_tm_transp[4*5+0].xyz
       + pointToEye.y*shadow_cascade_tm_transp[4*5+1].xyz
       + pointToEye.z*shadow_cascade_tm_transp[4*5+2].xyz
       + shadow_cascade_tm_transp[4*5+3].xyz;
  #endif

  tlast = float3(0, 0, 0);
  #if NUM_CASCADES==2
  tlast = t1;
  #elif NUM_CASCADES==3
  tlast = t2;
  #elif NUM_CASCADES==4
  tlast = t3;
  #elif NUM_CASCADES==5
  tlast = t4;
  #elif NUM_CASCADES==6
  tlast = t5;
  #endif
  float3 use_half = sel_scale * HALF;
  bool b5 = NUM_CASCADES > 5 && all(abs(t5)<use_half);
  bool b4 = NUM_CASCADES > 4 && all(abs(t4)<use_half);
  bool b3 = NUM_CASCADES > 3 && all(abs(t3)<use_half);
  bool b2 = NUM_CASCADES > 2 && all(abs(t2)<use_half);
  bool b1 = NUM_CASCADES > 1 && all(abs(t1)<use_half);
  bool b0 = all(abs(t0)<HALF);

  float3 t = 1;
  cascade_id = NUM_CASCADES - 1;

  t = b5 ? t5 : t;
  t = b4 ? t4 : t;
  t = b3 ? t3 : t;
  t = b2 ? t2 : t;
  t = b1 ? t1 : t;
  t = b0 ? t0 : t;
  cascade_id = b5 ? 5 : cascade_id;
  cascade_id = b4 ? 4 : cascade_id;
  cascade_id = b3 ? 3 : cascade_id;
  cascade_id = b2 ? 2 : cascade_id;
  cascade_id = b1 ? 1 : cascade_id;

#if HW_VULKAN
  BRANCH
  if (b0)
    cascade_id = 0;
#else
  cascade_id = b0 ? 0 : cascade_id;
#endif

  csmEffect = get_csm_shadow_effect( cascade_id, t0, t1, t2, t3, t4, t5 );

  t.xy = t.xy*shadow_cascade_tc_mul_offset[cascade_id].xy + shadow_cascade_tc_mul_offset[cascade_id].zw;
  t.z += 0.5;
  return t;
}

#undef HALF