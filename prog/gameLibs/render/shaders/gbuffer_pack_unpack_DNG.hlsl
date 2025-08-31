// This is for DNG only, but it is required for NBS, so it needs to stay in gameLibs
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
  half4 emission_color; // .a is emission_albedo_mult, when encoded: either ao, or emission
  half shadow;
  uint material;
  uint dynamic;
  uint autoMotionVecs;
  uint sss_profile;//&SSS_PROFILE_MASK!
  uint isLandscape;
  uint isHeroCockpit;

  half3 motion;
  half reactive;

  // Dummy
  half impostorMask;
  bool isTwoSided;
  bool isGlass;
  bool isGrass;
  bool isFoliage;
  bool isUnstable;
};

struct PackedGbuffer
{
  half4 albedo_ao;
  //float4 normal_smoothness_material;//change normal later
  //half4 reflectance_metallTranslucency_shadow;//processed
  float4 normal_material;//change normal later
  // Last two bits of 'shadow'
  // 0 - static, auto motion
  // 1 - dynamic, no auto motion
  // 2 - landscape
  // 3 - dynamic, auto motion
  half4 smoothness_reflectance_metallTranslucency_shadow;//processed
  half4 motion_reactive;
};

half packTranslucencyOrMetalness(UnpackedGbuffer gbuffer)
{
  half metalnessOrTranslucency = isSubSurfaceShader(gbuffer.material) ? (gbuffer.sss_profile == SSS_PROFILE_CLOTH ? gbuffer.sheen : gbuffer.translucency) : gbuffer.metalness;
  return isEmissiveShader(gbuffer.material) ? encodeEmissionStrength(gbuffer.emission_strength) : metalnessOrTranslucency;
}
half packReflectance(UnpackedGbuffer gbuffer)
{
  return isEmissiveShader(gbuffer.material) ? encodeReflectance(half2(gbuffer.reflectance, gbuffer.emission_color.a)) : gbuffer.reflectance;
}

half packShadowAndDynamic(UnpackedGbuffer gbuffer)
{
  uint dynamicLandscapeMotionVec =
    (gbuffer.isHeroCockpit << 2)
    | ((gbuffer.isLandscape | (gbuffer.autoMotionVecs & gbuffer.dynamic)) << 1) | gbuffer.dynamic;

  uint sss_profile_or_shadow = gbuffer.material == SHADING_SUBSURFACE ? gbuffer.sss_profile : uint(gbuffer.shadow*255+0.1h);

  return half(half((sss_profile_or_shadow&SSS_PROFILE_MASK) + dynamicLandscapeMotionVec) / 255.h);
}

half4 packGbufSmoothnessReflectancMetallTranslucencyShadow(UnpackedGbuffer gbuffer)
{
  return half4(
    gbuffer.smoothness,
    packReflectance(gbuffer),
    packTranslucencyOrMetalness(gbuffer),
    packShadowAndDynamic(gbuffer)
  );
}

PackedGbuffer pack_gbuffer(UnpackedGbuffer gbuffer)
{
  PackedGbuffer gbuf;
  half material = half(half(gbuffer.material)*(1.h/3.0h));

  gbuf.normal_material = half4(gbuffer.normal*0.5h+0.5h, material);
  gbuf.albedo_ao = half4(gbuffer.albedo, isEmissiveShader(gbuffer.material) ? encodeEmissionColor(gbuffer.emission_color.rgb) : gbuffer.ao);

  gbuf.smoothness_reflectance_metallTranslucency_shadow = packGbufSmoothnessReflectancMetallTranslucencyShadow(gbuffer);
  gbuf.motion_reactive = half4(gbuffer.motion, gbuffer.reactive);
  return gbuf;
}

bool isDynamic(uint flags) { return (flags & 1) == 1; }
bool isLandscape(uint flags) { return (flags & 3) == 2; }
bool isAutoMotionVecs(uint flags) { return (flags & 3) != 1; }
bool isHeroCockpit(uint flags) { return (flags & 4) == 4; }

void unpackGbufferNormalSmoothness(PackedGbuffer gbuf, out float3 normal, out half smoothness)
{
  //normal = decodeNormal(gbuf.normal_smoothness_material.xy*2-1, decode_normal_basis(gbuf.reflectance_metallTranslucency_shadow));
  //smoothness = gbuf.normal_smoothness_material.z;
  normal = normalize(gbuf.normal_material.xyz*2-1);
  smoothness = gbuf.smoothness_reflectance_metallTranslucency_shadow.x;
}

//gbuffer.material, gbuffer.normal, gbuffer.smoothness, gbuffer.albedo and gbuffer.ao has to be inited already
void unpackGbufferTranslucencyChannel(inout UnpackedGbuffer gbuffer, half4 smoothness_reflectance_metallTranslucency_shadow, out half emission_albedo_mult)
{
  half shadow = smoothness_reflectance_metallTranslucency_shadow.w;
  half metalnessOrTranslucency = smoothness_reflectance_metallTranslucency_shadow.z;

  half packedReflectance = smoothness_reflectance_metallTranslucency_shadow.y;
  half2 unpackedReflectance = isEmissiveShader(gbuffer.material) ? decodeReflectance(packedReflectance) : packedReflectance;
  emission_albedo_mult = unpackedReflectance.y;
  half reflectance = unpackedReflectance.x;

  uint shadowAndDynamic = uint(shadow*255+0.1);

  gbuffer.dynamic = isDynamic(shadowAndDynamic);
  gbuffer.isLandscape = isLandscape(shadowAndDynamic);
  gbuffer.autoMotionVecs = isAutoMotionVecs(shadowAndDynamic);
  gbuffer.isHeroCockpit = isHeroCockpit(shadowAndDynamic);


  bool isSubSurface = isSubSurfaceShader(gbuffer.material);
  gbuffer.metalness = isMetallicShader(gbuffer.material) ? metalnessOrTranslucency : 0;
  gbuffer.translucency = isSubSurface ? metalnessOrTranslucency : 0;
  gbuffer.emission_strength = isEmissiveShader(gbuffer.material) ? decodeEmissionStrength(metalnessOrTranslucency) : 0;
  uint sss_profile_or_shadow = (shadowAndDynamic&SSS_PROFILE_MASK);
  gbuffer.shadow = half(gbuffer.material == SHADING_SUBSURFACE ? 1.0h : half(sss_profile_or_shadow)/255.h);
  gbuffer.sss_profile = gbuffer.material == SHADING_SUBSURFACE ? sss_profile_or_shadow : 0;
  gbuffer.reflectance = reflectance;
  //gbuffer.diffuseColor = albedo*(1-gbuffer.metalness);
  //half fresnel0Dielectric = 0.04f;//lerp(0.16f,0.01f, smoothness);//sqr((1.0 - refractiveIndex)/(1.0 + refractiveIndex)) for dielectrics;
  //gbuffer.specularColor = lerp(half3(fresnel0Dielectric, fresnel0Dielectric, fresnel0Dielectric), albedo, gbuffer.metalness);
  gbuffer.isTwoSided = gbuffer.isGlass = gbuffer.isGrass = gbuffer.isUnstable = false;
  gbuffer.impostorMask = 0;
}
UnpackedGbuffer unpackGbuffer(PackedGbuffer gbuf)
{
  UnpackedGbuffer gbuffer = (UnpackedGbuffer)0;

  //gbuffer.material = uint(gbuf.normal_smoothness_material.w*3.4999f);
  //half shadow = gbuf.reflectance_metallTranslucency_shadow.z;
  //half metalnessOrTranslucency = gbuf.reflectance_metallTranslucency_shadow.y;
  //half reflectance = gbuf.reflectance_metallTranslucency_shadow.x;
  unpackGbufferNormalSmoothness(gbuf, gbuffer.normal, gbuffer.smoothness);
  gbuffer.material = uint(gbuf.normal_material.w*(MATERIAL_COUNT-0.50001f));
  gbuffer.albedo = gbuf.albedo_ao.xyz;
  gbuffer.ao = isEmissiveShader(gbuffer.material) ? 1 : gbuf.albedo_ao.w;
  half emission_albedo_mult;
  unpackGbufferTranslucencyChannel(gbuffer, gbuf.smoothness_reflectance_metallTranslucency_shadow, emission_albedo_mult);
  gbuffer.emission_color = isEmissiveShader(gbuffer.material) ? half4(decodeEmissionColor(gbuf.albedo_ao.w), emission_albedo_mult) : 0;

  #ifndef NEED_MOTION_VECTOR_UNPACKING
    #error Need to define NEED_MOTION_VECTOR_UNPACKING to either 0 or 1 depending if we want to unpack motion vectors!
  #endif

  #if NEED_MOTION_VECTOR_UNPACKING // For NBS
    gbuffer.motion = gbuf.motion_reactive.xyz;
    gbuffer.reactive = gbuf.motion_reactive.w;
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

