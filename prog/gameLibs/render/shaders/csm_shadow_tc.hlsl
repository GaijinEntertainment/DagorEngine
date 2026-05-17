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

#if CSM_CASCADE_SELECTION == CSM_CASCADE_SELECTION_PROJECTION_BOUNDS
float3 get_csm_shadow_tc(float3 pointToEye, out uint cascade_id, out float csmEffect)
{
  pointToEye = -pointToEye;
  //to be moved out to const buffer
  float3 t0,t1,t2,t3,t4,t5;
  float4 p4 = float4(pointToEye, 1);
  t0 = float3(dot(shadow_cascade_0_tm[0], p4), dot(shadow_cascade_0_tm[1], p4), dot(shadow_cascade_0_tm[2], p4));
  #if NUM_CASCADES>1
    t1 = t0 * shadow_cascade_relative_scale_and_near[1].xyz + shadow_cascade_relative_offset_and_far[1].xyz;
  #endif
  #if NUM_CASCADES>2
    t2 = t0 * shadow_cascade_relative_scale_and_near[2].xyz + shadow_cascade_relative_offset_and_far[2].xyz;
  #endif
  #if NUM_CASCADES>3
    t3 = t0 * shadow_cascade_relative_scale_and_near[3].xyz + shadow_cascade_relative_offset_and_far[3].xyz;
  #endif
  #if NUM_CASCADES>4
    t4 = t0 * shadow_cascade_relative_scale_and_near[4].xyz + shadow_cascade_relative_offset_and_far[4].xyz;
  #endif
  #if NUM_CASCADES>5
    t5 = t0 * shadow_cascade_relative_scale_and_near[5].xyz + shadow_cascade_relative_offset_and_far[5].xyz;
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
  float4 p4 = float4(pointToEye, 1);
  float3 t0 = float3(dot(shadow_cascade_0_tm[0], p4), dot(shadow_cascade_0_tm[1], p4), dot(shadow_cascade_0_tm[2], p4));

  // Regular cascades pick by viewDepth < far[i]; the override chain
  // takes the smallest (innermost) matching cascade.
  //
  // Manual (anchored) cascades can occupy first N slots and can represent any box in world space
  // so they're selected using projection bounds.

#if HAS_MANUAL_CASCADES
  uint manualCount = uint(shadow_cascade_manual_cascade_count);
  // Compute ti for cascades that may be manual at runtime.
  // The NUM_CASCADES > i guard on each bi short-circuits the read when ti is unused.
  float3 t1 = 0, t2 = 0, t3 = 0, t4 = 0, t5 = 0;
  #if NUM_CASCADES > 1
    t1 = t0 * shadow_cascade_relative_scale_and_near[1].xyz + shadow_cascade_relative_offset_and_far[1].xyz;
  #endif
  #if NUM_CASCADES > 2
    t2 = t0 * shadow_cascade_relative_scale_and_near[2].xyz + shadow_cascade_relative_offset_and_far[2].xyz;
  #endif
  #if NUM_CASCADES > 3
    t3 = t0 * shadow_cascade_relative_scale_and_near[3].xyz + shadow_cascade_relative_offset_and_far[3].xyz;
  #endif
  #if NUM_CASCADES > 4
    t4 = t0 * shadow_cascade_relative_scale_and_near[4].xyz + shadow_cascade_relative_offset_and_far[4].xyz;
  #endif
  #if NUM_CASCADES > 5
    t5 = t0 * shadow_cascade_relative_scale_and_near[5].xyz + shadow_cascade_relative_offset_and_far[5].xyz;
  #endif

  bool b0 = all(abs(t0) < HALF);
  bool b1 = NUM_CASCADES > 1 && ((manualCount > 1u) ? all(abs(t1) < HALF) : (viewDepth < shadow_cascade_relative_offset_and_far[1].w));
  bool b2 = NUM_CASCADES > 2 && ((manualCount > 2u) ? all(abs(t2) < HALF) : (viewDepth < shadow_cascade_relative_offset_and_far[2].w));
  bool b3 = NUM_CASCADES > 3 && ((manualCount > 3u) ? all(abs(t3) < HALF) : (viewDepth < shadow_cascade_relative_offset_and_far[3].w));
  bool b4 = NUM_CASCADES > 4 && ((manualCount > 4u) ? all(abs(t4) < HALF) : (viewDepth < shadow_cascade_relative_offset_and_far[4].w));
  bool b5 = NUM_CASCADES > 5 && ((manualCount > 5u) ? all(abs(t5) < HALF) : (viewDepth < shadow_cascade_relative_offset_and_far[5].w));
#else
  bool b5 = NUM_CASCADES > 5 && viewDepth < shadow_cascade_relative_offset_and_far[5].w;
  bool b4 = NUM_CASCADES > 4 && viewDepth < shadow_cascade_relative_offset_and_far[4].w;
  bool b3 = NUM_CASCADES > 3 && viewDepth < shadow_cascade_relative_offset_and_far[3].w;
  bool b2 = NUM_CASCADES > 2 && viewDepth < shadow_cascade_relative_offset_and_far[2].w;
  bool b1 = NUM_CASCADES > 1 && viewDepth < shadow_cascade_relative_offset_and_far[1].w;
  bool b0 = viewDepth < shadow_cascade_relative_offset_and_far[0].w;
#endif

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

  t = t0 * shadow_cascade_relative_scale_and_near[cascade_id].xyz + shadow_cascade_relative_offset_and_far[cascade_id].xyz;

  BRANCH
  if (cascade_id == NUM_CASCADES - 1)
  {
    float near = shadow_cascade_relative_scale_and_near[NUM_CASCADES - 1].w;
    float far = shadow_cascade_relative_offset_and_far[NUM_CASCADES - 1].w;
    float mid = (near + far) * 0.5;
    csmEffect = saturate((viewDepth - mid) * 2.0 / (far - near));
  }

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
    t1 = t0 * shadow_cascade_relative_scale_and_near[1].xyz;
    t1_o = t0_o * shadow_cascade_relative_scale_and_near[1].xyz + shadow_cascade_relative_offset_and_far[1].xyz;
  #endif
  #if NUM_CASCADES>2
    t2 = t0 * shadow_cascade_relative_scale_and_near[2].xyz;
    t2_o = t0_o * shadow_cascade_relative_scale_and_near[2].xyz + shadow_cascade_relative_offset_and_far[2].xyz;
  #endif
  #if NUM_CASCADES>3
    t3 = t0 * shadow_cascade_relative_scale_and_near[3].xyz;
    t3_o = t0_o * shadow_cascade_relative_scale_and_near[3].xyz + shadow_cascade_relative_offset_and_far[3].xyz;
  #endif
  #if NUM_CASCADES>4
    t4 = t0 * shadow_cascade_relative_scale_and_near[4].xyz;
    t4_o = t0_o * shadow_cascade_relative_scale_and_near[4].xyz + shadow_cascade_relative_offset_and_far[4].xyz;
  #endif
  #if NUM_CASCADES>5
    t5 = t0 * shadow_cascade_relative_scale_and_near[5].xyz;
    t5_o = t0_o * shadow_cascade_relative_scale_and_near[5].xyz + shadow_cascade_relative_offset_and_far[5].xyz;
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
float3 get_csm_shadow_tc_scaled(float3 pointToEye, out uint cascade_id, out float csmEffect, float randomTransitionOffset)
{
  pointToEye = -pointToEye;
  float viewDepth = dot(shadow_cascade_camera_near_plane.xyz, pointToEye);

#if HAS_MANUAL_CASCADES
  // randomTransitionOffset should be in range [-0.5, 0.5].
  // scale for projection bounds shrinkage was in range [1, 1.1]
  float scale = randomTransitionOffset * 0.1 + 1.05;
  uint manualCount = uint(shadow_cascade_manual_cascade_count);
  float3 t0 = float3(
    dot(shadow_cascade_0_tm[0].xyz, pointToEye),
    dot(shadow_cascade_0_tm[1].xyz, pointToEye),
    dot(shadow_cascade_0_tm[2].xyz, pointToEye));
  float3 t0_o = float3(shadow_cascade_0_tm[0].w, shadow_cascade_0_tm[1].w, shadow_cascade_0_tm[2].w);
  float3 t1 = 0, t1_o = 0;
  float3 t2 = 0, t2_o = 0;
  float3 t3 = 0, t3_o = 0;
  float3 t4 = 0, t4_o = 0;
  float3 t5 = 0, t5_o = 0;
  #if NUM_CASCADES > 1
    t1 = t0 * shadow_cascade_relative_scale_and_near[1].xyz;
    t1_o = t0_o * shadow_cascade_relative_scale_and_near[1].xyz + shadow_cascade_relative_offset_and_far[1].xyz;
  #endif
  #if NUM_CASCADES > 2
    t2 = t0 * shadow_cascade_relative_scale_and_near[2].xyz;
    t2_o = t0_o * shadow_cascade_relative_scale_and_near[2].xyz + shadow_cascade_relative_offset_and_far[2].xyz;
  #endif
  #if NUM_CASCADES > 3
    t3 = t0 * shadow_cascade_relative_scale_and_near[3].xyz;
    t3_o = t0_o * shadow_cascade_relative_scale_and_near[3].xyz + shadow_cascade_relative_offset_and_far[3].xyz;
  #endif
  #if NUM_CASCADES > 4
    t4 = t0 * shadow_cascade_relative_scale_and_near[4].xyz;
    t4_o = t0_o * shadow_cascade_relative_scale_and_near[4].xyz + shadow_cascade_relative_offset_and_far[4].xyz;
  #endif
  #if NUM_CASCADES > 5
    t5 = t0 * shadow_cascade_relative_scale_and_near[5].xyz;
    t5_o = t0_o * shadow_cascade_relative_scale_and_near[5].xyz + shadow_cascade_relative_offset_and_far[5].xyz;
  #endif

  t0 = (NUM_CASCADES == 1) ? t0 + t0_o : t0;
  t1 = (NUM_CASCADES == 2) ? t1 + t1_o : t1;
  t2 = (NUM_CASCADES == 3) ? t2 + t2_o : t2;
  t3 = (NUM_CASCADES == 4) ? t3 + t3_o : t3;
  t4 = (NUM_CASCADES == 5) ? t4 + t4_o : t4;
  t5 = (NUM_CASCADES == 6) ? t5 + t5_o : t5;

  float3 scale3 = float3(scale, scale, 1); // don't shrink frustum depth, it causes popping

  bool b0 = all(abs(NUM_CASCADES == 1 ? t0 : t0_o + t0 * scale3) < HALF);
  bool b1 = NUM_CASCADES > 1 && ((manualCount > 1u)
    ? all(abs(NUM_CASCADES == 2 ? t1 : t1_o + t1 * scale3) < HALF)
    : (viewDepth  < shadow_cascade_relative_offset_and_far[1].w));
  bool b2 = NUM_CASCADES > 2 && ((manualCount > 2u)
    ? all(abs(NUM_CASCADES == 3 ? t2 : t2_o + t2 * scale3) < HALF)
    : (viewDepth < shadow_cascade_relative_offset_and_far[2].w));
  bool b3 = NUM_CASCADES > 3 && ((manualCount > 3u)
    ? all(abs(NUM_CASCADES == 4 ? t3 : t3_o + t3 * scale3) < HALF)
    : (viewDepth < shadow_cascade_relative_offset_and_far[3].w));
  bool b4 = NUM_CASCADES > 4 && ((manualCount > 4u)
    ? all(abs(NUM_CASCADES == 5 ? t4 : t4_o + t4 * scale3) < HALF)
    : (viewDepth < shadow_cascade_relative_offset_and_far[4].w));
  bool b5 = NUM_CASCADES > 5 && ((manualCount > 5u)
    ? all(abs(NUM_CASCADES == 6 ? t5 : t5_o + t5 * scale3) < HALF)
    : (viewDepth < shadow_cascade_relative_offset_and_far[5].w));

  // Promote t0 to combined for post-selection (no-op when NUM_CASCADES == 1
  // since the first promotion already combined it).
  t0 = (NUM_CASCADES == 1) ? t0 : t0 + t0_o;
#else
  bool b5 = NUM_CASCADES > 5 && viewDepth < shadow_cascade_relative_offset_and_far[5].w;
  bool b4 = NUM_CASCADES > 4 && viewDepth < shadow_cascade_relative_offset_and_far[4].w;
  bool b3 = NUM_CASCADES > 3 && viewDepth < shadow_cascade_relative_offset_and_far[3].w;
  bool b2 = NUM_CASCADES > 2 && viewDepth < shadow_cascade_relative_offset_and_far[2].w;
  bool b1 = NUM_CASCADES > 1 && viewDepth < shadow_cascade_relative_offset_and_far[1].w;
  bool b0 = viewDepth < shadow_cascade_relative_offset_and_far[0].w;
#endif

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

  bool shouldTransition = cascade_id != NUM_CASCADES - 1;
#if HAS_MANUAL_CASCADES
  shouldTransition = shouldTransition && cascade_id >= manualCount;
#endif

  BRANCH
  if (shouldTransition)
  {
    uint nextCascadeId = cascade_id + 1;
    float near = shadow_cascade_relative_scale_and_near[nextCascadeId].w;
    float far = shadow_cascade_relative_offset_and_far[cascade_id].w;

    float transition = saturate((viewDepth - near) / (far - near));
    cascade_id = cascade_id + round(transition + randomTransitionOffset);
  }

#if !HAS_MANUAL_CASCADES
  float4 p4 = float4(pointToEye, 1);
  float3 t0 = float3(dot(shadow_cascade_0_tm[0], p4), dot(shadow_cascade_0_tm[1], p4), dot(shadow_cascade_0_tm[2], p4));
#endif
  t = t0 * shadow_cascade_relative_scale_and_near[cascade_id].xyz + shadow_cascade_relative_offset_and_far[cascade_id].xyz;

  BRANCH
  if (cascade_id == NUM_CASCADES - 1)
  {
    float near = shadow_cascade_relative_scale_and_near[NUM_CASCADES - 1].w;
    float far = shadow_cascade_relative_offset_and_far[NUM_CASCADES - 1].w;
    float mid = (near + far) * 0.5;
    csmEffect = saturate((viewDepth - mid) * 2.0 / (far - near));
  }

  t.xy = t.xy*shadow_cascade_tc_mul_offset[cascade_id].xy + shadow_cascade_tc_mul_offset[cascade_id].zw;
  t.z += 0.5;
  return t;
}
#endif

float get_csm_shadow_last_cascade_vignette(float3 pointToEye)
{
#if NUM_CASCADES == 0
  return 1;
#endif
  pointToEye = -pointToEye;
#if CSM_CASCADE_SELECTION == CSM_CASCADE_SELECTION_PROJECTION_BOUNDS
  float4 p4 = float4(pointToEye, 1);
  float3 t0 = float3(dot(shadow_cascade_0_tm[0], p4), dot(shadow_cascade_0_tm[1], p4), dot(shadow_cascade_0_tm[2], p4));
  float3 tLast = t0 * shadow_cascade_relative_scale_and_near[LAST_CASCADE-1].xyz + shadow_cascade_relative_offset_and_far[LAST_CASCADE-1].xyz;
  float v = max3(abs(tLast.x), abs(tLast.y), saturate(tLast.z))*2.f; // ignore upper z, since we want depth from all cascades to be included
#elif CSM_CASCADE_SELECTION == CSM_CASCADE_SELECTION_VIEW_DEPTH
  float viewDepth = dot(shadow_cascade_camera_near_plane.xyz, pointToEye);
  float near = shadow_cascade_relative_scale_and_near[LAST_CASCADE - 1].w;
  float far = shadow_cascade_relative_offset_and_far[LAST_CASCADE - 1].w;
  float mid = (near + far) * 0.5;
  float v = saturate((viewDepth - mid) * 2.0 / (far - near));
#endif
  return saturate(v);
}

#undef HALF
