#ifndef  shadow_array_supported
#define shadow_array_supported 0
#endif

#ifndef MIN_SHADOW_SIZE
  #define MIN_SHADOW_SIZE 512
#endif

#define HALF (float3(0.5-2./MIN_SHADOW_SIZE, 0.5-2./MIN_SHADOW_SIZE, 0.5) - 1e-5)//0.5-2/512 (512 is our smallest cascade size)

#ifndef LAST_CASCADE
#define LAST_CASCADE NUM_CASCADES
#endif

float get_csm_shadow_effect( uint cascade_id, float3 t0, float3 t1, float3 t2, float3 t3, float3 t4, float3 t5 )
{
  float csmEffect = 0;
  #if LAST_CASCADE==6
    csmEffect = (cascade_id == LAST_CASCADE-1) ? max3(abs(t5.x), abs(t5.y), abs(t5.z))*2 : csmEffect;
  #elif LAST_CASCADE==5
    csmEffect = (cascade_id == LAST_CASCADE-1) ? max3(abs(t4.x), abs(t4.y), abs(t4.z))*2 : csmEffect;
  #elif LAST_CASCADE==4
    csmEffect = (cascade_id == LAST_CASCADE-1) ? max3(abs(t3.x), abs(t3.y), abs(t3.z))*2 : csmEffect;
  #elif LAST_CASCADE==3
    csmEffect = (cascade_id == LAST_CASCADE-1) ? max3(abs(t2.x), abs(t2.y), abs(t2.z))*2 : csmEffect;
  #elif LAST_CASCADE==2
    csmEffect = (cascade_id == LAST_CASCADE-1) ? max3(abs(t1.x), abs(t1.y), abs(t1.z))*2 : csmEffect;
  #elif LAST_CASCADE==1
    csmEffect = (cascade_id == LAST_CASCADE-1) ? max3(abs(t0.x), abs(t0.y), abs(t0.z))*2 : csmEffect;
  #endif
  return csmEffect;
}

#ifndef CSM_CASCADE_SELECTION
#define CSM_CASCADE_SELECTION CSM_CASCADE_SELECTION_PROJECTION_BOUNDS
#endif

#if CSM_CASCADE_SELECTION == CSM_CASCADE_SELECTION_PROJECTION_BOUNDS
float3 get_csm_shadow_tc(float3 pointToEye, out uint cascade_id, out float csmEffect)
{
  pointToEye = -pointToEye;
  //to be moved out to const buffer
  float3 t0,t1,t2,t3,t4,t5;
  float4 p4 = float4(pointToEye, 1);
  t0 = float3(dot(shadow_cascade_0_tm[0], p4), dot(shadow_cascade_0_tm[1], p4), dot(shadow_cascade_0_tm[2], p4));
  #if NUM_CASCADES>1
    t1 = t0 * shadow_cascade_relative_scale[1].xyz + shadow_cascade_relative_offset[1].xyz;
  #endif
  #if NUM_CASCADES>2
    t2 = t0 * shadow_cascade_relative_scale[2].xyz + shadow_cascade_relative_offset[2].xyz;
  #endif
  #if NUM_CASCADES>3
    t3 = t0 * shadow_cascade_relative_scale[3].xyz + shadow_cascade_relative_offset[3].xyz;
  #endif
  #if NUM_CASCADES>4
    t4 = t0 * shadow_cascade_relative_scale[4].xyz + shadow_cascade_relative_offset[4].xyz;
  #endif
  #if NUM_CASCADES>5
    t5 = t0 * shadow_cascade_relative_scale[5].xyz + shadow_cascade_relative_offset[5].xyz;
  #endif

  bool b5 = NUM_CASCADES > 5 && all(abs(t5) < HALF);
  bool b4 = NUM_CASCADES > 4 && all(abs(t4) < HALF);
  bool b3 = NUM_CASCADES > 3 && all(abs(t3) < HALF);
  bool b2 = NUM_CASCADES > 2 && all(abs(t2) < HALF);
  bool b1 = NUM_CASCADES > 1 && all(abs(t1) < HALF);
  bool b0 = all(abs(t0) < HALF);

  float3 t = 2;
  cascade_id = 6;

  t = b5 ? t5 : t;
  t = b4 ? t4 : t;
  t = b3 ? t3 : t;
  t = b2 ? t2 : t;
  t = b1 ? t1 : t;
  t = b0 ? t0 : t;

  #if NUM_CASCADES>=6
    cascade_id = b5 ? 5 : cascade_id;
  #endif
  #if NUM_CASCADES>=5
    cascade_id = b4 ? 4 : cascade_id;
  #endif
  #if NUM_CASCADES>=4
    cascade_id = b3 ? 3 : cascade_id;
  #endif
  #if NUM_CASCADES>=3
    cascade_id = b2 ? 2 : cascade_id;
  #endif
  #if NUM_CASCADES>=2
    cascade_id = b1 ? 1 : cascade_id;
  #endif

  #if HW_VULKAN
    BRANCH
    if (b0)
      cascade_id = 0;
  #elif NUM_CASCADES>=1
    cascade_id = b0 ? 0 : cascade_id;
  #endif

  csmEffect = 0;
  if (cascade_id == 6)
    return t;

  csmEffect = get_csm_shadow_effect( cascade_id, t0, t1, t2, t3, t4, t5 );

  t.xy = t.xy*shadow_cascade_tc_mul_offset[cascade_id].xy + shadow_cascade_tc_mul_offset[cascade_id].zw;
  t.z += 0.5;
  return t;
}
#elif CSM_CASCADE_SELECTION == CSM_CASCADE_SELECTION_VIEW_DEPTH
float3 get_csm_shadow_tc(float3 pointToEye, out uint cascade_id, out float csmEffect)
{
  pointToEye = -pointToEye;
  float viewDepth = dot(shadow_cascade_camera_near_plane.xyz, pointToEye);

  // shadow_cascade_splits stores per-cascade far distances:
  //   splits[0] = (cascade0_far, cascade1_far, cascade2_far, cascade3_far)
  //   splits[1] = (cascade4_far, cascade5_far, -, -)
  // bi = viewDepth < far[i] means the point is within cascade i's range.
  // The override chain picks the smallest (innermost) cascade.
  bool b5 = NUM_CASCADES > 5 && viewDepth < shadow_cascade_splits[1].y;
  bool b4 = NUM_CASCADES > 4 && viewDepth < shadow_cascade_splits[1].x;
  bool b3 = NUM_CASCADES > 3 && viewDepth < shadow_cascade_splits[0].w;
  bool b2 = NUM_CASCADES > 2 && viewDepth < shadow_cascade_splits[0].z;
  bool b1 = NUM_CASCADES > 1 && viewDepth < shadow_cascade_splits[0].y;
  bool b0 = viewDepth < shadow_cascade_splits[0].x;

  float3 t = 2;
  cascade_id = 6;

  #if NUM_CASCADES>=6
    cascade_id = b5 ? 5 : cascade_id;
  #endif
  #if NUM_CASCADES>=5
    cascade_id = b4 ? 4 : cascade_id;
  #endif
  #if NUM_CASCADES>=4
    cascade_id = b3 ? 3 : cascade_id;
  #endif
  #if NUM_CASCADES>=3
    cascade_id = b2 ? 2 : cascade_id;
  #endif
  #if NUM_CASCADES>=2
    cascade_id = b1 ? 1 : cascade_id;
  #endif

  #if HW_VULKAN
    BRANCH
    if (b0)
      cascade_id = 0;
  #elif NUM_CASCADES>=1
    cascade_id = b0 ? 0 : cascade_id;
  #endif

  csmEffect = 0;
  if (cascade_id == 6)
    return t;

  float4 p4 = float4(pointToEye, 1);
  float3 t0 = float3(dot(shadow_cascade_0_tm[0], p4), dot(shadow_cascade_0_tm[1], p4), dot(shadow_cascade_0_tm[2], p4));
  t = t0 * shadow_cascade_relative_scale[cascade_id].xyz + shadow_cascade_relative_offset[cascade_id].xyz;

  csmEffect = (cascade_id == NUM_CASCADES - 1) ? max3(abs(t.x), abs(t.y), abs(t.z)) * 2 : 0;

  t.xy = t.xy*shadow_cascade_tc_mul_offset[cascade_id].xy + shadow_cascade_tc_mul_offset[cascade_id].zw;
  t.z += 0.5;
  return t;
}
#endif

float3 get_csm_shadow_tc(float3 pointToEye, out uint cascade_id)
{
  float shadowEffect;
  return get_csm_shadow_tc(pointToEye, cascade_id, shadowEffect);
}

#if CSM_CASCADE_SELECTION == CSM_CASCADE_SELECTION_PROJECTION_BOUNDS
float3 get_csm_shadow_tc_scaled(float3 pointToEye, out uint cascade_id, out float csmEffect, float scale)
{
  pointToEye = -pointToEye;
  //to be moved out to const buffer
  float3 t0,t1,t2,t3,t4,t5;
  float3 t0_o,t1_o,t2_o,t3_o,t4_o,t5_o;
  t0 = float3(dot(shadow_cascade_0_tm[0].xyz, pointToEye), dot(shadow_cascade_0_tm[1].xyz, pointToEye), dot(shadow_cascade_0_tm[2].xyz, pointToEye));
  t0_o = float3(shadow_cascade_0_tm[0].w, shadow_cascade_0_tm[1].w, shadow_cascade_0_tm[2].w);
  #if NUM_CASCADES>1
    t1 = t0 * shadow_cascade_relative_scale[1].xyz;
    t1_o = t0_o * shadow_cascade_relative_scale[1].xyz + shadow_cascade_relative_offset[1].xyz;
  #endif
  #if NUM_CASCADES>2
    t2 = t0 * shadow_cascade_relative_scale[2].xyz;
    t2_o = t0_o * shadow_cascade_relative_scale[2].xyz + shadow_cascade_relative_offset[2].xyz;
  #endif
  #if NUM_CASCADES>3
    t3 = t0 * shadow_cascade_relative_scale[3].xyz;
    t3_o = t0_o * shadow_cascade_relative_scale[3].xyz + shadow_cascade_relative_offset[3].xyz;
  #endif
  #if NUM_CASCADES>4
    t4 = t0 * shadow_cascade_relative_scale[4].xyz;
    t4_o = t0_o * shadow_cascade_relative_scale[4].xyz + shadow_cascade_relative_offset[4].xyz;
  #endif
  #if NUM_CASCADES>5
    t5 = t0 * shadow_cascade_relative_scale[5].xyz;
    t5_o = t0_o * shadow_cascade_relative_scale[5].xyz + shadow_cascade_relative_offset[5].xyz;
  #endif

  t0 = (NUM_CASCADES==1) ? t0 + t0_o : t0;
  t1 = (NUM_CASCADES==2) ? t1 + t1_o : t1;
  t2 = (NUM_CASCADES==3) ? t2 + t2_o : t2;
  t3 = (NUM_CASCADES==4) ? t3 + t3_o : t3;
  t4 = (NUM_CASCADES==5) ? t4 + t4_o : t4;
  t5 = (NUM_CASCADES==6) ? t5 + t5_o : t5;

  float3 scale3 = float3(scale, scale, 1); // don't shrink frustum depth, it causes popping
  bool b5 = NUM_CASCADES > 5 && all(abs(NUM_CASCADES == 6 ? t5 : t5_o + t5 * scale3)<HALF);
  bool b4 = NUM_CASCADES > 4 && all(abs(NUM_CASCADES == 5 ? t4 : t4_o + t4 * scale3)<HALF);
  bool b3 = NUM_CASCADES > 3 && all(abs(NUM_CASCADES == 4 ? t3 : t3_o + t3 * scale3)<HALF);
  bool b2 = NUM_CASCADES > 2 && all(abs(NUM_CASCADES == 3 ? t2 : t2_o + t2 * scale3)<HALF);
  bool b1 = NUM_CASCADES > 1 && all(abs(NUM_CASCADES == 2 ? t1 : t1_o + t1 * scale3)<HALF);
  bool b0 = NUM_CASCADES > 0 && all(abs(NUM_CASCADES == 1 ? t0 : t0_o + t0 * scale3)<HALF);

  t0 = (NUM_CASCADES==1) ? t0 : t0 + t0_o;
  t1 = (NUM_CASCADES==2) ? t1 : t1 + t1_o;
  t2 = (NUM_CASCADES==3) ? t2 : t2 + t2_o;
  t3 = (NUM_CASCADES==4) ? t3 : t3 + t3_o;
  t4 = (NUM_CASCADES==5) ? t4 : t4 + t4_o;
  t5 = (NUM_CASCADES==6) ? t5 : t5 + t5_o;

  float3 t = 2;
  cascade_id = 6;

  t = b5 ? t5 : t;
  t = b4 ? t4 : t;
  t = b3 ? t3 : t;
  t = b2 ? t2 : t;
  t = b1 ? t1 : t;
  t = b0 ? t0 : t;

  #if NUM_CASCADES>=6
    cascade_id = b5 ? 5 : cascade_id;
  #endif
  #if NUM_CASCADES>=5
    cascade_id = b4 ? 4 : cascade_id;
  #endif
  #if NUM_CASCADES>=4
    cascade_id = b3 ? 3 : cascade_id;
  #endif
  #if NUM_CASCADES>=3
    cascade_id = b2 ? 2 : cascade_id;
  #endif
  #if NUM_CASCADES>=2
    cascade_id = b1 ? 1 : cascade_id;
  #endif

  #if HW_VULKAN
    BRANCH
    if (b0)
      cascade_id = 0;
  #elif NUM_CASCADES>=1
    cascade_id = b0 ? 0 : cascade_id;
  #endif

  csmEffect = 0;
  if (cascade_id == 6)
    return t;

  csmEffect = get_csm_shadow_effect( cascade_id, t0, t1, t2, t3, t4, t5 );

  t.xy = t.xy*shadow_cascade_tc_mul_offset[cascade_id].xy + shadow_cascade_tc_mul_offset[cascade_id].zw;
  t.z += 0.5;
  return t;
}
#elif CSM_CASCADE_SELECTION == CSM_CASCADE_SELECTION_VIEW_DEPTH
float3 get_csm_shadow_tc_scaled(float3 pointToEye, out uint cascade_id, out float csmEffect, float scale)
{
  pointToEye = -pointToEye;
  float viewDepth = dot(shadow_cascade_camera_near_plane.xyz, pointToEye);
  // Scale shrinks the acceptance range for non-last cascades, creating a dithered transition zone.
  // The last cascade is unscaled (matches the projection-bounds version where scale only affects xy,
  // and the last cascade uses the full unscaled bounds).
  float scaledViewDepth = viewDepth * scale;

  bool b5 = NUM_CASCADES > 5 && (NUM_CASCADES == 6 ? viewDepth : scaledViewDepth) < shadow_cascade_splits[1].y;
  bool b4 = NUM_CASCADES > 4 && (NUM_CASCADES == 5 ? viewDepth : scaledViewDepth) < shadow_cascade_splits[1].x;
  bool b3 = NUM_CASCADES > 3 && (NUM_CASCADES == 4 ? viewDepth : scaledViewDepth) < shadow_cascade_splits[0].w;
  bool b2 = NUM_CASCADES > 2 && (NUM_CASCADES == 3 ? viewDepth : scaledViewDepth) < shadow_cascade_splits[0].z;
  bool b1 = NUM_CASCADES > 1 && (NUM_CASCADES == 2 ? viewDepth : scaledViewDepth) < shadow_cascade_splits[0].y;
  bool b0 = (NUM_CASCADES == 1 ? viewDepth : scaledViewDepth) < shadow_cascade_splits[0].x;

  float3 t = 2;
  cascade_id = 6;

  #if NUM_CASCADES>=6
    cascade_id = b5 ? 5 : cascade_id;
  #endif
  #if NUM_CASCADES>=5
    cascade_id = b4 ? 4 : cascade_id;
  #endif
  #if NUM_CASCADES>=4
    cascade_id = b3 ? 3 : cascade_id;
  #endif
  #if NUM_CASCADES>=3
    cascade_id = b2 ? 2 : cascade_id;
  #endif
  #if NUM_CASCADES>=2
    cascade_id = b1 ? 1 : cascade_id;
  #endif

  #if HW_VULKAN
    BRANCH
    if (b0)
      cascade_id = 0;
  #elif NUM_CASCADES>=1
    cascade_id = b0 ? 0 : cascade_id;
  #endif

  csmEffect = 0;
  if (cascade_id == 6)
    return t;

  float4 p4 = float4(pointToEye, 1);
  float3 t0 = float3(dot(shadow_cascade_0_tm[0], p4), dot(shadow_cascade_0_tm[1], p4), dot(shadow_cascade_0_tm[2], p4));
  t = t0 * shadow_cascade_relative_scale[cascade_id].xyz + shadow_cascade_relative_offset[cascade_id].xyz;

  csmEffect = (cascade_id == NUM_CASCADES - 1) ? max3(abs(t.x), abs(t.y), abs(t.z)) * 2 : 0;

  t.xy = t.xy*shadow_cascade_tc_mul_offset[cascade_id].xy + shadow_cascade_tc_mul_offset[cascade_id].zw;
  t.z += 0.5;
  return t;
}
#endif

#undef HALF
