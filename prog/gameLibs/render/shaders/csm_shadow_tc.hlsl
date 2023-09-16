#ifndef  shadow_array_supported
#define shadow_array_supported 0
#endif

#ifndef MIN_SHADOW_SIZE
  #define MIN_SHADOW_SIZE 512
#endif

#define HALF (float3(0.5-2./MIN_SHADOW_SIZE, 0.5-2./MIN_SHADOW_SIZE, 0.5))//0.5-2/512 (512 is our smallest cascade size)

float3 get_csm_shadow_tc(float3 pointToEye, out uint cascade_id, out float shadowEffect)
{
  shadowEffect = 1.0f;
  pointToEye = -pointToEye;
  //to be moved out to const buffer
  float3 t0,t1,t2,t3;
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
  bool b3 = NUM_CASCADES > 3 && all(abs(t3)<HALF);
  bool b2 = NUM_CASCADES > 2 && all(abs(t2)<HALF);
  bool b1 = NUM_CASCADES > 1 && all(abs(t1)<HALF);
  bool b0 = all(abs(t0)<HALF);

  float3 t = 2;
  cascade_id = 4;

  t = b3 ? t3 : t;
  t = b2 ? t2 : t;
  t = b1 ? t1 : t;
  t = b0 ? t0 : t;
  #if NUM_CASCADES>=4
  cascade_id = b3 ? 3 : cascade_id;
  #endif
  #if NUM_CASCADES>=3
  cascade_id = b2 ? 2 : cascade_id;
  #endif
  #if NUM_CASCADES>=2
  cascade_id = b1 ? 1 : cascade_id;
  #endif
  #if NUM_CASCADES>=1
  cascade_id = b0 ? 0 : cascade_id;
  #endif

  if (cascade_id == 4)
    return t;

  t.xy = t.xy*shadow_cascade_tc_mul_offset[cascade_id].xy + shadow_cascade_tc_mul_offset[cascade_id].zw;
  t.z += 0.5;
  return t;
}

float3 get_csm_shadow_tc_scaled(float3 pointToEye, out uint cascade_id, out float csmEffect, float scale)
{
  pointToEye = -pointToEye;
  //to be moved out to const buffer
  float3 t0,t1,t2,t3;
  float3 t0_o,t1_o,t2_o,t3_o;
  t0 = pointToEye.x*shadow_cascade_tm_transp[4*0+0].xyz
     + pointToEye.y*shadow_cascade_tm_transp[4*0+1].xyz
     + pointToEye.z*shadow_cascade_tm_transp[4*0+2].xyz;
  t0_o = shadow_cascade_tm_transp[4*0+3].xyz;
  #if NUM_CASCADES>1
    t1 = pointToEye.x*shadow_cascade_tm_transp[4*1+0].xyz
       + pointToEye.y*shadow_cascade_tm_transp[4*1+1].xyz
       + pointToEye.z*shadow_cascade_tm_transp[4*1+2].xyz;
    t1_o = shadow_cascade_tm_transp[4*1+3].xyz;
  #endif
  #if NUM_CASCADES>2
    t2 = pointToEye.x*shadow_cascade_tm_transp[4*2+0].xyz
       + pointToEye.y*shadow_cascade_tm_transp[4*2+1].xyz
       + pointToEye.z*shadow_cascade_tm_transp[4*2+2].xyz;
    t2_o = shadow_cascade_tm_transp[4*2+3].xyz;
  #endif
  #if NUM_CASCADES>3
    t3 = pointToEye.x*shadow_cascade_tm_transp[4*3+0].xyz
       + pointToEye.y*shadow_cascade_tm_transp[4*3+1].xyz
       + pointToEye.z*shadow_cascade_tm_transp[4*3+2].xyz;
    t3_o = shadow_cascade_tm_transp[4*3+3].xyz;
  #endif

  t0 = (NUM_CASCADES==1) ? t0 + t0_o : t0;
  t1 = (NUM_CASCADES==2) ? t1 + t1_o : t1;
  t2 = (NUM_CASCADES==3) ? t2 + t2_o : t2;
  t3 = (NUM_CASCADES==4) ? t3 + t3_o : t3;
  bool b3 = NUM_CASCADES > 3 && all(abs(NUM_CASCADES == 4 ? t3 : t3_o + t3 * scale)<HALF);
  bool b2 = NUM_CASCADES > 2 && all(abs(NUM_CASCADES == 3 ? t2 : t2_o + t2 * scale)<HALF);
  bool b1 = NUM_CASCADES > 1 && all(abs(NUM_CASCADES == 2 ? t1 : t1_o + t1 * scale)<HALF);
  bool b0 = NUM_CASCADES > 0 && all(abs(NUM_CASCADES == 1 ? t0 : t0_o + t0 * scale)<HALF);
  t0 = (NUM_CASCADES==1) ? t0 : t0 + t0_o;
  t1 = (NUM_CASCADES==2) ? t1 : t1 + t1_o;
  t2 = (NUM_CASCADES==3) ? t2 : t2 + t2_o;
  t3 = (NUM_CASCADES==4) ? t3 : t3 + t3_o;

  float3 t = 2;
  cascade_id = 4;
  t = b3 ? t3 : t;
  t = b2 ? t2 : t;
  t = b1 ? t1 : t;
  t = b0 ? t0 : t;
  #if NUM_CASCADES>=4
  cascade_id = b3 ? 3 : cascade_id;
  #endif
  #if NUM_CASCADES>=3
  cascade_id = b2 ? 2 : cascade_id;
  #endif
  #if NUM_CASCADES>=2
  cascade_id = b1 ? 1 : cascade_id;
  #endif
  #if NUM_CASCADES>=1
  cascade_id = b0 ? 0 : cascade_id;
  #endif

  csmEffect = 0;
  if (cascade_id == 4)
    return t;

  csmEffect = (NUM_CASCADES==4 && cascade_id == NUM_CASCADES-1) ? max3(abs(t3.x), abs(t3.y), abs(t3.z))*2 : csmEffect;
  csmEffect = (NUM_CASCADES==3 && cascade_id == NUM_CASCADES-1) ? max3(abs(t2.x), abs(t2.y), abs(t2.z))*2 : csmEffect;
  csmEffect = (NUM_CASCADES==2 && cascade_id == NUM_CASCADES-1) ? max3(abs(t1.x), abs(t1.y), abs(t1.z))*2 : csmEffect;
  csmEffect = (NUM_CASCADES==1 && cascade_id == NUM_CASCADES-1) ? max3(abs(t0.x), abs(t0.y), abs(t0.z))*2 : csmEffect;

  t.xy = t.xy*shadow_cascade_tc_mul_offset[cascade_id].xy + shadow_cascade_tc_mul_offset[cascade_id].zw;
  t.z += 0.5;
  return t;
}

#undef HALF