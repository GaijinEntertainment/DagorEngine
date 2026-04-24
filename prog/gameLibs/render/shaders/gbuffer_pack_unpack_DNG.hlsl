#ifndef USE_GBUF_DEBUG_TARGET
  #define USE_GBUF_DEBUG_TARGET 0
#endif

#ifndef USE_GBUF_EMISSION_PART
  #define USE_GBUF_EMISSION_PART 0
#endif

#ifndef USE_GBUF_BVH_TARGET
  #define USE_GBUF_BVH_TARGET 0
#endif

#ifndef USE_GBUF_IMPOSTOR_MASK
  #define USE_GBUF_IMPOSTOR_MASK 0
#endif

struct UnpackedGbuffer
{
  half3 albedo;
  half smoothness;
  float3 normal;

  half metalness;//either translucent or metallic or emission
  half translucency;//either translucent or metallic or emission
  half emission_strength; // encoded HS(V) color (V==1) //either translucent or metallic or emission
  half reflectance;
  half sheen;

  half ao;//either ao, or emission
  #if USE_GBUF_EMISSION_PART
    half emission_part;
  #endif
  half4 emission_color; // .a is emission_albedo_mult, when encoded: either ao, or emission
  half shadow;
  uint material;
  uint dynamic;
  uint sss_profile;//&SSS_PROFILE_MASK!
  uint isLandscape;
  uint isHeroCockpit;
  uint dynamicMask;

  half3 motion;
  half reactive;

  #if USE_GBUF_DEBUG_TARGET
    float debug_info;
  #endif

  // Dummy
  half impostorMask;
  bool isGlass;
  bool isGrass;
  bool isFoliage;
  bool isFacingSun;
  bool isSnow;
};

struct PackedGbuffer
{
  half4 albedo_ao;
  //float4 normal_smoothness_material;//change normal later
  //half4 reflectance_metallTranslucency_shadow;//processed
  half4 normal_material;//change normal later
  // Last two bits of 'shadow'
  // 0 - static, auto motion
  // 1 - dynamic, no auto motion
  // 2 - landscape
  // 3 - dynamic, auto motion
  half4 smoothness_reflectance_metallTranslucency_shadow;//processed
  half4 motion_reactive;

  #if USE_GBUF_DEBUG_TARGET
    float debug_info;
  #endif

  #if USE_GBUF_BVH_TARGET
    uint bvh_flags;
  #endif
};

half2 packEmissionStrenghHiLo(UnpackedGbuffer gbuffer)
{
  return isNoLightEmissiveShader(gbuffer.material)
         ? encodeEmissionStrengthTwoParts(gbuffer.emission_strength)
         : encodeEmissionStrength(gbuffer.emission_strength);
}

half packEmissionInAoChannel(UnpackedGbuffer gbuffer, half emission_strength_hi)
{
  return isNoLightEmissiveShader(gbuffer.material)
         ? emission_strength_hi
         : encodeEmissionColor(gbuffer.emission_color.rgb);
}

half4 packAlbedoAo(UnpackedGbuffer gbuffer, half emission_strength_hi)
{
  half aoChannel = isEmissiveShader(gbuffer.material)
                 ? packEmissionInAoChannel(gbuffer, emission_strength_hi)
                 : gbuffer.ao;

  return half4(gbuffer.albedo, aoChannel);
}


half4 packNormalMaterialExtra(UnpackedGbuffer gbuffer, bool packing_active)
{
  uint advancedMaterialBit = (gbuffer.material & ADV_MAT_MASK_IN_MAT) >> ADV_MAT_CONVERT_SHIFT;
  uint extraBits = gbuffer.dynamicMask | advancedMaterialBit;
  half extra = half(extraBits) / 1023.h;
  half baseMaterial = half(half(gbuffer.material & BASE_MAT_MASK) * (1.h/3.0h));

  return packing_active
          ? half4(pack_normal_so(half3(gbuffer.normal)), extra, baseMaterial)
          : half4(gbuffer.normal * 0.5h + 0.5h, baseMaterial);
}

half4 packGbuf2(UnpackedGbuffer gbuffer, half emission_strength_lo)
{
  half reflectance = isEmissiveShader(gbuffer.material)
                   ? encodeReflectance(half2(gbuffer.reflectance, gbuffer.emission_color.a))
                   : gbuffer.reflectance;

  half metalnessOrEmissionStrength = isEmissiveShader(gbuffer.material) ? emission_strength_lo : gbuffer.metalness;
  half translucencyOrSheen = gbuffer.sss_profile == SSS_PROFILE_CLOTH ? gbuffer.sheen : gbuffer.translucency;
  half channelB = isSubSurfaceShader(gbuffer.material) ? translucencyOrSheen : metalnessOrEmissionStrength;


  #if USE_GBUF_IMPOSTOR_MASK
    uint extraFlags = ((gbuffer.isHeroCockpit || gbuffer.impostorMask) << 2) | (gbuffer.isLandscape << 1) | gbuffer.dynamic;
  #else
    uint extraFlags = (gbuffer.isHeroCockpit << 2) | (gbuffer.isLandscape << 1) | gbuffer.dynamic;
  #endif

  #if USE_GBUF_EMISSION_PART
    uint sss_profile_or_shadow = gbuffer.material == SHADING_SUBSURFACE
                               ? gbuffer.sss_profile
                               : uint((isEmissiveShader(gbuffer.material) ? (half(1.0) - gbuffer.emission_part) : gbuffer.shadow) * 255 + 0.1h);
  #else
    uint sss_profile_or_shadow = gbuffer.material == SHADING_SUBSURFACE ? gbuffer.sss_profile : uint(gbuffer.shadow*255+0.1h);
  #endif

  half shadowAndDynamic = half((sss_profile_or_shadow&SSS_PROFILE_MASK) + extraFlags) / 255.h;

  return half4(gbuffer.smoothness, reflectance, channelB, shadowAndDynamic);
}

half4 packGbufMotionReactive(half3 motion_vecs, half reactive)
{
  return half4(motion_vecs, reactive);
}

uint packGbufBvhFlags(UnpackedGbuffer gbuffer)
{
  return (gbuffer.isGlass ? BVH_GLASS : 0)
        | (gbuffer.isGrass ? BVH_GRASS : 0)
        | (gbuffer.isFoliage ? BVH_FOLIAGE : 0)
        | (gbuffer.isFacingSun ? BVH_FACING_SUN : 0);
}

PackedGbuffer pack_gbuffer(UnpackedGbuffer gbuffer)
{
  PackedGbuffer gbuf;
  //Simplify material if no normal packing is active
  gbuffer.material = packed_gbuf_normals ? gbuffer.material : gbuffer.material & BASE_MAT_MASK;

  half2 emission_hi_lo = packEmissionStrenghHiLo(gbuffer);

  gbuf.albedo_ao = packAlbedoAo(gbuffer, emission_hi_lo.x);
  gbuf.normal_material = packNormalMaterialExtra(gbuffer, (bool)packed_gbuf_normals);
  gbuf.smoothness_reflectance_metallTranslucency_shadow = packGbuf2(gbuffer, emission_hi_lo.y);
  gbuf.motion_reactive = packGbufMotionReactive(gbuffer.motion, gbuffer.reactive);

  #if USE_GBUF_DEBUG_TARGET
    gbuf.debug_info = gbuffer.debug_info;
  #endif

  #if USE_GBUF_BVH_TARGET
    gbuf.bvh_flags = packGbufBvhFlags(gbuffer);
  #endif

  return gbuf;
}

bool isDynamic(uint flags) { return (flags & 1) == 1; }
bool isLandscape(uint flags) { return (flags & 2) == 2; }
bool isHeroCockpit(uint flags) { return (flags & 4) == 4; }

void unpackMaterialExtra(half4 normal_material, out uint extra, out uint material)
{
  extra = packed_gbuf_normals ? uint(normal_material.z * 1023.h + 0.5h) : 0u;

  uint baseMaterial = uint(normal_material.w*(BASE_MAT_CNT-0.50001f));
  uint advancedMaterialOffset = (extra & ADV_MAT_MASK_IN_EXTRA) << ADV_MAT_CONVERT_SHIFT;
  material = baseMaterial + advancedMaterialOffset;
}

void unpackNormal(half4 normal_material, uint material, out float3 normal)
{
  half3 packedVersionNormals = isNoLightEmissiveShader(material)
                             ? half3(0, 0, 0)
                             : unpack_normal_so(normal_material.xy);

  normal = packed_gbuf_normals ? packedVersionNormals : normalize(normal_material.xyz * 2.0h - 1.h);
}

void unpackSmoothness(half4 smoothness_reflectance_metallTranslucency_shadow, out half smoothness)
{
  smoothness = smoothness_reflectance_metallTranslucency_shadow.x;
}

void unpackSSSProfileShadowAndBitflied(half4 smoothness_reflectance_metallTranslucency_shadow, uint material, out uint sss_profile, out half shadow, out uint bit_filed, out uint sss_profile_or_shadow)
{
  half shadowChannel = smoothness_reflectance_metallTranslucency_shadow.w;
  uint shadowAndDynamic = uint(shadowChannel*255+0.1);
  sss_profile_or_shadow = (shadowAndDynamic&SSS_PROFILE_MASK);

  bit_filed = shadowAndDynamic & (~SSS_PROFILE_MASK);
  #if USE_GBUF_EMISSION_PART
    shadow = material == SHADING_SUBSURFACE ? half(1.0) :
                (isEmissiveShader(material) ? half(1.0) : half(sss_profile_or_shadow)/255.h);
  #else
    shadow = material == SHADING_SUBSURFACE ? 1.0h : half(sss_profile_or_shadow)/255.h;
  #endif
  sss_profile = material == SHADING_SUBSURFACE ? sss_profile_or_shadow : 0;
}

void unpackTranslucency(half4 smoothness_reflectance_metallTranslucency_shadow, uint material, uint sss_profile, out half translucency)
{
  translucency = isSubSurfaceShader(material) && sss_profile != SSS_PROFILE_CLOTH ? smoothness_reflectance_metallTranslucency_shadow.z : 0;
}

void unpackShadowAndDynamicChannel(PackedGbuffer gbuf, inout UnpackedGbuffer gbuffer)
{
  uint bitfield, sss_profile_or_shadow;
  unpackSSSProfileShadowAndBitflied(gbuf.smoothness_reflectance_metallTranslucency_shadow, gbuffer.material, gbuffer.sss_profile, gbuffer.shadow, bitfield, sss_profile_or_shadow);
  gbuffer.dynamic = isDynamic(bitfield);
  gbuffer.isLandscape = isLandscape(bitfield);

  #if USE_GBUF_IMPOSTOR_MASK
    bool heroCockpitOrImpostorMask = isHeroCockpit(bitfield);
    gbuffer.isHeroCockpit = heroCockpitOrImpostorMask && gbuffer.dynamic;
    gbuffer.impostorMask = heroCockpitOrImpostorMask && !gbuffer.dynamic;
  #else
    gbuffer.isHeroCockpit = isHeroCockpit(bitfield);
  #endif

  #if USE_GBUF_EMISSION_PART
    gbuffer.emission_part = gbuffer.material == SHADING_SUBSURFACE ? half(0.0) :
                         (isEmissiveShader(gbuffer.material) ? half(1.0) - half(sss_profile_or_shadow) / 255.h : half(0.0));
  #endif
}

void unpackEmissionStrengthAndColor(PackedGbuffer gbuf, inout UnpackedGbuffer gbuffer)
{
  half gbuf2Z = gbuf.smoothness_reflectance_metallTranslucency_shadow.z;
  half emissionStrength = isNoLightEmissiveShader(gbuffer.material)
                         ? decodeEmissionStrengthFromTwoParts(half2(gbuf.albedo_ao.w, gbuf2Z))
                         : decodeEmissionStrength(gbuf2Z);

  gbuffer.emission_strength = isEmissiveShader(gbuffer.material) ? emissionStrength : 0;

  half3 emissionColor = isNoLightEmissiveShader(gbuffer.material) ? half3(1, 1, 1) : decodeEmissionColor(gbuf.albedo_ao.w);
  gbuffer.emission_color.rgb = isEmissiveShader(gbuffer.material) ? emissionColor : half3(0, 0, 0);
}

void unpackTranslucencyChannelAndAlbedoAo(PackedGbuffer gbuf, inout UnpackedGbuffer gbuffer)
{
  unpackTranslucency(gbuf.smoothness_reflectance_metallTranslucency_shadow, gbuffer.material, gbuffer.sss_profile, gbuffer.translucency);

  half metalnessEmissionStrengthOrSheen = gbuf.smoothness_reflectance_metallTranslucency_shadow.z;
  gbuffer.metalness = isMetallicShader(gbuffer.material) ? metalnessEmissionStrengthOrSheen : 0;
  gbuffer.sheen = isSubSurfaceShader(gbuffer.material) && gbuffer.sss_profile == SSS_PROFILE_CLOTH ? metalnessEmissionStrengthOrSheen : 0;

  unpackEmissionStrengthAndColor(gbuf, gbuffer);

  gbuffer.albedo = gbuf.albedo_ao.rgb;
  gbuffer.ao = isEmissiveShader(gbuffer.material) ? 1.h : gbuf.albedo_ao.w;
}

void unpackReflectanceChannel(PackedGbuffer gbuf, inout UnpackedGbuffer gbuffer)
{
  half packedReflectance = gbuf.smoothness_reflectance_metallTranslucency_shadow.y;
  half2 unpackedReflectance = isEmissiveShader(gbuffer.material) ? decodeReflectance(packedReflectance) : packedReflectance;
  gbuffer.emission_color.a = isEmissiveShader(gbuffer.material) ? unpackedReflectance.y : 0;
  gbuffer.reflectance = unpackedReflectance.x;
}

void unpackMotionVecsForNBS(PackedGbuffer gbuf, inout UnpackedGbuffer gbuffer)
{
  #ifndef NEED_MOTION_VECTOR_UNPACKING
    #error Need to define NEED_MOTION_VECTOR_UNPACKING to either 0 or 1 depending if we want to unpack motion vectors!
  #endif

  #if NEED_MOTION_VECTOR_UNPACKING // For NBS
    gbuffer.motion = gbuf.motion_reactive.xyz;
    gbuffer.reactive = gbuf.motion_reactive.w;
  #endif
}

void unpackBvhFlags(PackedGbuffer gbuf, inout UnpackedGbuffer gbuffer)
{
  #if USE_GBUF_BVH_TARGET
    uint bvh_flags = gbuf.bvh_flags;
    gbuffer.isGlass = (bvh_flags & BVH_GLASS) != 0;
    gbuffer.isGrass = (bvh_flags & BVH_GRASS) != 0;
    gbuffer.isFoliage = (bvh_flags & BVH_FOLIAGE) != 0;
    gbuffer.isFacingSun = (bvh_flags & BVH_FACING_SUN) != 0;
    if (gbuffer.isGlass)
      gbuffer.smoothness = 1;
  #endif
}

UnpackedGbuffer unpackGbuffer(PackedGbuffer gbuf)
{
  UnpackedGbuffer gbuffer = (UnpackedGbuffer)0;

  uint extra;
  unpackMaterialExtra(gbuf.normal_material, extra, gbuffer.material);
  gbuffer.dynamicMask = extra & 1u;
  #if !ENVI_COVER_NBS_WITH_PACKED_NORMS
    unpackNormal(gbuf.normal_material, gbuffer.material, gbuffer.normal);
  #endif
  unpackSmoothness(gbuf.smoothness_reflectance_metallTranslucency_shadow, gbuffer.smoothness);
  unpackShadowAndDynamicChannel(gbuf, gbuffer);
  unpackTranslucencyChannelAndAlbedoAo(gbuf, gbuffer);
  unpackReflectanceChannel(gbuf, gbuffer);
  unpackMotionVecsForNBS(gbuf, gbuffer);
  unpackBvhFlags(gbuf, gbuffer);

  #if USE_GBUF_DEBUG_TARGET
    gbuffer.debug_info = gbuf.debug_info;
  #endif

  return gbuffer;
}

#ifndef NEUTRAL_SSS_FACTOR
#define NEUTRAL_SSS_FACTOR half3(1,1,1)
#endif

#ifndef BLOOD_SSS_FACTOR
#define BLOOD_SSS_FACTOR half3(0.8,0.4,0.3)
#endif

#ifndef LEAVES_SSS_FACTOR
#define LEAVES_SSS_FACTOR half3(1.0,0.8,0.5)//todo:make me global variable!
#endif

#ifndef SHEEN_SSS_FACTOR
#define SHEEN_SSS_FACTOR NEUTRAL_SSS_FACTOR
#endif

