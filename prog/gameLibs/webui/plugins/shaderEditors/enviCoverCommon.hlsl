#define SELECT_ALBEDO_AO 0U
#define SELECT_NORMAL_MATERIAL 1U
#define SELECT_SRTMSD 2U
#define SELECT_MOTION_VECS 3U

//MASK VALUES FOR MIXING
#define ALBEDO_MASK (1U << 0U)
#define NORMAL_MASK (1U << 1U)
#define MOTION_MASK (1U << 2U)
#define REACTIVE_MASK (1U << 3U)
#define SMOOTHNESS_MASK (1U << 4U)
#define REFLECTANCE_MASK (1U << 5U)
#define SHADOW_MASK (1U << 6U)
#define AO_MASK (1U << 7U)
#define EMISSION_COLOR_MASK (1U << 8U)
#define METALNESS_MASK (1U << 9U)
#define TRANSLUCENCY_MASK (1U << 10U)
#define EMISSION_STRENGTH_MASK (1U << 11U)
#define SSS_PROF_MASK (1U << 12U)
#define SHEEN_MASK (1U << 13U)
#define DYNAMIC_MASK (1U << 14U)
#define AUTO_MOTION_VECS_MASK (1U << 15U)
#define IS_LANDSCAPE_MASK (1U << 16U)
#define IS_HERO_COCKPIT_MASK (1U << 17U)
#define MATERIAL_MASK (1U << 18U)

// Type defines for nbs graph
#define Layer_t UnpackedGbuffer
#define material_t uint
#define mask_t uint

bool isMemberMaskSet(uint flags, uint mask)
{
  return (flags & mask) == mask;
}

int isSetInt(uint flags, uint mask)
{
  return (flags & mask) == mask;
}

#ifndef NBS_GBUFF_GAMMA_PRECISE
  #define NBS_GBUFF_GAMMA_PRECISE 0
#endif

half3 convertGbuffSRGBToLinear(half3 albedo)
{
  #if ENVI_COVER_GAMMA_USE_PRECISE
    return pow(albedo, 2.2f);
  #else
    return pow2(albedo);
  #endif
}

half3 convertGbuffLinearToSRGB(half3 albedo)
{
  #if ENVI_COVER_GAMMA_USE_PRECISE
    return pow(albedo, 1 / 2.2f);
  #else
    return sqrt(albedo);
  #endif
}

static const float EPS = 0.0001;

uint createLayerMask(bool albedo, bool normal, bool motion, bool reactive, bool smoothness, bool reflectance, bool shadow, bool ao,
  bool emission_color, bool metalness, bool translucency, bool emission_strength, bool sss_profile, bool sheen, bool dynamic,
  bool autoMotionVecs, bool isLandscape, bool isHeroCockpit, bool material)
{
  uint flag = 0;
  flag += albedo ? ALBEDO_MASK : 0;
  flag += normal ? NORMAL_MASK : 0;
  flag += motion ? MOTION_MASK : 0;
  flag += reactive ? REACTIVE_MASK : 0;
  flag += smoothness ? SMOOTHNESS_MASK : 0;
  flag += reflectance ? REFLECTANCE_MASK : 0;
  flag += shadow ? SHADOW_MASK : 0;
  flag += ao ? AO_MASK : 0;
  flag += emission_color ? EMISSION_COLOR_MASK : 0;
  flag += metalness ? METALNESS_MASK : 0;
  flag += translucency ? TRANSLUCENCY_MASK : 0;
  flag += emission_strength ? EMISSION_STRENGTH_MASK : 0;
  flag += sss_profile ? SSS_PROF_MASK : 0;
  flag += sheen ? SHEEN_MASK : 0;
  flag += dynamic ? DYNAMIC_MASK : 0;
  flag += autoMotionVecs ? AUTO_MOTION_VECS_MASK : 0;
  flag += isLandscape ? IS_LANDSCAPE_MASK : 0;
  flag += isHeroCockpit ? IS_HERO_COCKPIT_MASK : 0;
  flag += material ? MATERIAL_MASK : 0;
  return flag;
}

bool isModified(uint flag, uint select)
{
  return (flag >> select) & 1;
}

void setModified(inout uint flag, uint select)
{
  flag = flag | (1 << select);
}

void setModifiedFromMix(inout uint nbs_flags, uint mix_flags)
{
  if ((mix_flags & (ALBEDO_MASK | AO_MASK | EMISSION_COLOR_MASK)) != 0)
    setModified(nbs_flags, SELECT_ALBEDO_AO);

  if ((mix_flags & (NORMAL_MASK | MATERIAL_MASK)) != 0)
    setModified(nbs_flags, SELECT_NORMAL_MATERIAL);

  if ((mix_flags & (MOTION_MASK | REACTIVE_MASK)) != 0)
    setModified(nbs_flags, SELECT_MOTION_VECS);

  uint srtmsd_mask = SMOOTHNESS_MASK | REFLECTANCE_MASK | SHADOW_MASK | EMISSION_COLOR_MASK | METALNESS_MASK | TRANSLUCENCY_MASK | IS_HERO_COCKPIT_MASK |
                    EMISSION_STRENGTH_MASK | SSS_PROF_MASK | SHEEN_MASK | DYNAMIC_MASK | AUTO_MOTION_VECS_MASK | IS_LANDSCAPE_MASK;

  if ((mix_flags & srtmsd_mask) != 0)
    setModified(nbs_flags, SELECT_SRTMSD);
}
struct NBSGbuffer
{
  uint flags;
  UnpackedGbuffer unpackedGbuffer;
  // PackedGbuffer packedGbuffer;
};

void writeNBSGbuffer(NBSGbuffer nbsGbuff, uint2 tc)
{
  BRANCH
  if (isModified(nbsGbuff.flags, SELECT_ALBEDO_AO))
  {
    half3 gammaCorrectedAlbedo = convertGbuffLinearToSRGB(nbsGbuff.unpackedGbuffer.albedo);
    half4 packed_albedo_ao = packGbufAlbedoAo(gammaCorrectedAlbedo, nbsGbuff.unpackedGbuffer.ao, nbsGbuff.unpackedGbuffer.emission_color.rgb,
                                              isEmissiveShader(nbsGbuff.unpackedGbuffer.material));
    writeGbuffAlbedoAO(tc, packed_albedo_ao);
  }

  BRANCH
  if (isModified(nbsGbuff.flags, SELECT_NORMAL_MATERIAL))
  {
    half4 packed_normal_mat = packGbufNormalMaterial(nbsGbuff.unpackedGbuffer.normal, nbsGbuff.unpackedGbuffer.material);
    writeGbuffNormalMat(tc, packed_normal_mat);
  }

  BRANCH
  if (isModified(nbsGbuff.flags, SELECT_SRTMSD))
  {
    half4 SRTMSD = packGbufSmoothnessReflectancMetallTranslucencyShadow(nbsGbuff.unpackedGbuffer);
    writeGbuffReflectance(tc, SRTMSD);
  }

  BRANCH
  if (isModified(nbsGbuff.flags, SELECT_MOTION_VECS))
  {
    half4 motion_reactive = packMotionReactive(nbsGbuff.unpackedGbuffer.motion, nbsGbuff.unpackedGbuffer.reactive);
    writeGbuffMotionVecs(tc, motion_reactive);
  }
}

NBSGbuffer init_NBSGbuffer(uint2 tc)
{
  NBSGbuffer nbsGbuffer = (NBSGbuffer)0;
  PackedGbuffer packedGbuffer = (PackedGbuffer)0;
  packedGbuffer.smoothness_reflectance_metallTranslucency_shadow = readGbuffReflectance(tc);
  packedGbuffer.albedo_ao = readGbuffAlbedoAO(tc);
  packedGbuffer.normal_material = readGbuffNormalMat(tc);
  nbsGbuffer.unpackedGbuffer = unpackGbuffer(packedGbuffer);
  nbsGbuffer.unpackedGbuffer.albedo = convertGbuffSRGBToLinear(nbsGbuffer.unpackedGbuffer.albedo);
  return nbsGbuffer;
}


UnpackedGbuffer createMetallicLayer(half3 albedo, float3 normal, half3 motion, half smoothness, half metalness, half reflectance, half ao, half shadow, half reactive,
  bool dynamic, bool autoMotionVecs, bool isLandscape, bool isHeroCockpit)
{
  UnpackedGbuffer gbuff = (UnpackedGbuffer)0;
  gbuff.albedo = albedo;
  gbuff.normal = normal;
  gbuff.motion = motion;
  gbuff.smoothness = smoothness;
  gbuff.metalness = metalness;
  gbuff.reflectance = reflectance;
  gbuff.ao = ao;
  gbuff.shadow = shadow;
  gbuff.reactive = reactive;
  gbuff.material = SHADING_NORMAL;
  gbuff.dynamic = uint(dynamic);
  gbuff.autoMotionVecs = uint(autoMotionVecs);
  gbuff.isLandscape = uint(isLandscape);
  gbuff.isHeroCockpit = uint(isHeroCockpit);
  return gbuff;
}

UnpackedGbuffer createFoliageLayer(half3 albedo, float3 normal, half3 motion, half smoothness, half translucency, half reflectance, half ao, half shadow, half reactive,
  bool dynamic, bool autoMotionVecs, bool isLandscape, bool isHeroCockpit)
{
  UnpackedGbuffer gbuff = (UnpackedGbuffer)0;
  gbuff.albedo = albedo;
  gbuff.normal = normal;
  gbuff.motion = motion;
  gbuff.smoothness = smoothness;
  gbuff.translucency = translucency;
  gbuff.reflectance = reflectance;
  gbuff.ao = ao;
  gbuff.shadow = shadow;
  gbuff.reactive = reactive;
  gbuff.material = SHADING_FOLIAGE;
  gbuff.dynamic = uint(dynamic);
  gbuff.autoMotionVecs = uint(autoMotionVecs);
  gbuff.isLandscape = uint(isLandscape);
  gbuff.isHeroCockpit = uint(isHeroCockpit);
  return gbuff;
}

UnpackedGbuffer createSubsurfaceLayer(half3 albedo, float3 normal, half3 motion, half smoothness, half translucency, half reflectance, half sheen, half ao, half reactive,
  bool dynamic, bool autoMotionVecs, uint sss_profile, bool isLandscape, bool isHeroCockpit)
{
  UnpackedGbuffer gbuff = (UnpackedGbuffer)0;
  gbuff.albedo = albedo;
  gbuff.normal = normal;
  gbuff.motion = motion;
  gbuff.smoothness = smoothness;
  gbuff.translucency = translucency;
  gbuff.reflectance = reflectance;
  gbuff.sheen = sheen;
  gbuff.ao = ao;
  gbuff.reactive = reactive;
  gbuff.material = SHADING_SUBSURFACE;
  gbuff.dynamic = uint(dynamic);
  gbuff.autoMotionVecs = uint(autoMotionVecs);
  gbuff.sss_profile = sss_profile;
  gbuff.isLandscape = uint(isLandscape);
  gbuff.isHeroCockpit = uint(isHeroCockpit);
  return gbuff;
}

UnpackedGbuffer createEmissiveLayer(half4 emission_color, half3 albedo, float3 normal, half3 motion, half smoothness, half emission_strength, half reflectance, half shadow, half reactive,
  bool dynamic, bool autoMotionVecs, bool isLandscape, bool isHeroCockpit)
{
  UnpackedGbuffer gbuff = (UnpackedGbuffer)0;
    gbuff.emission_color = emission_color;
    gbuff.albedo = albedo;
    gbuff.normal = normal;
    gbuff.motion = motion;
    gbuff.smoothness = smoothness;
    gbuff.emission_strength = emission_strength;
    gbuff.reflectance = reflectance;
    gbuff.shadow = shadow;
    gbuff.reactive = reactive;
    gbuff.material = SHADING_SELFILLUM;
    gbuff.dynamic = uint(dynamic);
    gbuff.autoMotionVecs = uint(autoMotionVecs);
    gbuff.isLandscape = uint(isLandscape);
    gbuff.isHeroCockpit = uint(isHeroCockpit);
    return gbuff;
}


// Single overwrite mixers:
void overwrite_albedo(inout NBSGbuffer gbuff, float3 value)
{
  gbuff.unpackedGbuffer.albedo = value;
  setModified(gbuff.flags, SELECT_ALBEDO_AO);
}
void overwrite_ao(inout NBSGbuffer gbuff, float value)
{
  gbuff.unpackedGbuffer.ao = value;
  setModified(gbuff.flags, SELECT_ALBEDO_AO);
}
void overwrite_normal(inout NBSGbuffer gbuff, float3 value)
{
  gbuff.unpackedGbuffer.normal = value;
  setModified(gbuff.flags, SELECT_NORMAL_MATERIAL);
}
void overwrite_material(inout NBSGbuffer gbuff, uint value)
{
  gbuff.unpackedGbuffer.material = value;
  setModified(gbuff.flags, SELECT_NORMAL_MATERIAL);
}
void overwrite_motion(inout NBSGbuffer gbuff, float3 value)
{
  gbuff.unpackedGbuffer.motion = value;
  setModified(gbuff.flags, SELECT_MOTION_VECS);
}
void overwrite_reactive(inout NBSGbuffer gbuff, float value)
{
  gbuff.unpackedGbuffer.reactive = value;
  setModified(gbuff.flags, SELECT_MOTION_VECS);
}
void overwrite_emission_color(inout NBSGbuffer gbuff, float4 value)
{
  gbuff.unpackedGbuffer.emission_color = value;
  setModified(gbuff.flags, SELECT_SRTMSD);    // .rgb stored here
  setModified(gbuff.flags, SELECT_ALBEDO_AO); // .a stored here
}
void overwrite_smoothness(inout NBSGbuffer gbuff, float value)
{
  gbuff.unpackedGbuffer.smoothness = value;
  setModified(gbuff.flags, SELECT_SRTMSD);
}
void overwrite_metalness(inout NBSGbuffer gbuff, float value)
{
  gbuff.unpackedGbuffer.metalness = value;
  setModified(gbuff.flags, SELECT_SRTMSD);
}
void overwrite_translucency(inout NBSGbuffer gbuff, float value)
{
  gbuff.unpackedGbuffer.translucency = value;
  setModified(gbuff.flags, SELECT_SRTMSD);
}
void overwrite_emission_strength(inout NBSGbuffer gbuff, float value)
{
  gbuff.unpackedGbuffer.emission_strength = value;
  setModified(gbuff.flags, SELECT_SRTMSD);
}
void overwrite_reflectance(inout NBSGbuffer gbuff, float value)
{
  gbuff.unpackedGbuffer.reflectance = value;
  setModified(gbuff.flags, SELECT_SRTMSD);
}
void overwrite_sheen(inout NBSGbuffer gbuff, float value)
{
  gbuff.unpackedGbuffer.sheen = value;
  setModified(gbuff.flags, SELECT_SRTMSD);
}
void overwrite_shadow(inout NBSGbuffer gbuff, float value)
{
  gbuff.unpackedGbuffer.shadow = value;
  setModified(gbuff.flags, SELECT_SRTMSD);
}
void overwrite_dynamic(inout NBSGbuffer gbuff, bool value)
{
  gbuff.unpackedGbuffer.dynamic = (uint)value;
  setModified(gbuff.flags, SELECT_SRTMSD);
}
void overwrite_autoMotionVecs(inout NBSGbuffer gbuff, bool value)
{
  gbuff.unpackedGbuffer.autoMotionVecs = (uint)value;
  setModified(gbuff.flags, SELECT_SRTMSD);
}
void overwrite_sss_profile(inout NBSGbuffer gbuff, uint value)
{
  gbuff.unpackedGbuffer.sss_profile = value;
  setModified(gbuff.flags, SELECT_SRTMSD);
}
void overwrite_isLandscape(inout NBSGbuffer gbuff, bool value)
{
  gbuff.unpackedGbuffer.isLandscape = (uint)value;
  setModified(gbuff.flags, SELECT_SRTMSD);
}
void overwrite_isHeroCockpit(inout NBSGbuffer gbuff, bool value)
{
  gbuff.unpackedGbuffer.isHeroCockpit = (uint)value;
  setModified(gbuff.flags, SELECT_SRTMSD);
}

// Lerp mixers:
void lerp_albedo(inout NBSGbuffer gbuff, float3 value, float weight)
{
  gbuff.unpackedGbuffer.albedo = lerp(gbuff.unpackedGbuffer.albedo, value, weight);
  if (weight > EPS)
    setModified(gbuff.flags, SELECT_ALBEDO_AO);
}
void lerp_ao(inout NBSGbuffer gbuff, float value, float weight)
{
  gbuff.unpackedGbuffer.ao = lerp(gbuff.unpackedGbuffer.ao, value, weight);
  if (weight > EPS)
    setModified(gbuff.flags, SELECT_ALBEDO_AO);
}
void lerp_normal(inout NBSGbuffer gbuff, float3 value, float weight)
{
  gbuff.unpackedGbuffer.normal = lerp(gbuff.unpackedGbuffer.normal, value, weight);
  if (weight > EPS)
    setModified(gbuff.flags, SELECT_NORMAL_MATERIAL);
}
void lerp_motion(inout NBSGbuffer gbuff, float3 value, float weight)
{
  gbuff.unpackedGbuffer.motion = lerp(gbuff.unpackedGbuffer.motion, value, weight);
  if (weight > EPS)
    setModified(gbuff.flags, SELECT_MOTION_VECS);
}
void lerp_reactive(inout NBSGbuffer gbuff, float value, float weight)
{
  gbuff.unpackedGbuffer.reactive = lerp(gbuff.unpackedGbuffer.reactive, value, weight);
  if (weight > EPS)
    setModified(gbuff.flags, SELECT_MOTION_VECS);
}
void lerp_emission_color(inout NBSGbuffer gbuff, float4 value, float weight)
{
  gbuff.unpackedGbuffer.emission_color = lerp(gbuff.unpackedGbuffer.emission_color, value, weight);
  if (weight > EPS)
  {
    setModified(gbuff.flags, SELECT_SRTMSD);    // .rgb stored here
    setModified(gbuff.flags, SELECT_ALBEDO_AO); // .a stored here
  }
}
void lerp_smoothness(inout NBSGbuffer gbuff, float value, float weight)
{
  gbuff.unpackedGbuffer.smoothness = lerp(gbuff.unpackedGbuffer.smoothness, value, weight);
  if (weight > EPS)
    setModified(gbuff.flags, SELECT_SRTMSD);
}
void lerp_metalness(inout NBSGbuffer gbuff, float value, float weight)
{
  gbuff.unpackedGbuffer.metalness = lerp(gbuff.unpackedGbuffer.metalness, value, weight);
  if (weight > EPS)
    setModified(gbuff.flags, SELECT_SRTMSD);
}
void lerp_translucency(inout NBSGbuffer gbuff, float value, float weight)
{
  gbuff.unpackedGbuffer.translucency = lerp(gbuff.unpackedGbuffer.translucency, value, weight);
  if (weight > EPS)
    setModified(gbuff.flags, SELECT_SRTMSD);
}
void lerp_emission_strength(inout NBSGbuffer gbuff, float value, float weight)
{
  gbuff.unpackedGbuffer.emission_strength = lerp(gbuff.unpackedGbuffer.emission_strength, value, weight);
  if (weight > EPS)
    setModified(gbuff.flags, SELECT_SRTMSD);
}
void lerp_reflectance(inout NBSGbuffer gbuff, float value, float weight)
{
  gbuff.unpackedGbuffer.reflectance = lerp(gbuff.unpackedGbuffer.reflectance, value, weight);
  if (weight > EPS)
    setModified(gbuff.flags, SELECT_SRTMSD);
}
void lerp_sheen(inout NBSGbuffer gbuff, float value, float weight)
{
  gbuff.unpackedGbuffer.sheen = lerp(gbuff.unpackedGbuffer.sheen, value, weight);
  if (weight > EPS)
    setModified(gbuff.flags, SELECT_SRTMSD);
}
void lerp_shadow(inout NBSGbuffer gbuff, float value, float weight)
{
  gbuff.unpackedGbuffer.shadow = lerp(gbuff.unpackedGbuffer.shadow, value, weight);
  if (weight > EPS)
    setModified(gbuff.flags, SELECT_SRTMSD);
}

// Overwrite with Layer Node
void overwriteWithLayer(inout NBSGbuffer gbuff, UnpackedGbuffer layer, uint flags, bool shouldOverwrite)
{
  BRANCH
  if (shouldOverwrite)
  {
    gbuff.unpackedGbuffer.albedo = isMemberMaskSet(flags, ALBEDO_MASK) ?  layer.albedo : gbuff.unpackedGbuffer.albedo;
    gbuff.unpackedGbuffer.normal = isMemberMaskSet(flags, NORMAL_MASK) ?  layer.normal : gbuff.unpackedGbuffer.normal;
    gbuff.unpackedGbuffer.motion = isMemberMaskSet(flags, MOTION_MASK) ?  layer.motion : gbuff.unpackedGbuffer.motion;
    gbuff.unpackedGbuffer.reactive = isMemberMaskSet(flags, REACTIVE_MASK) ?  layer.reactive : gbuff.unpackedGbuffer.reactive;
    gbuff.unpackedGbuffer.smoothness = isMemberMaskSet(flags, SMOOTHNESS_MASK) ?  layer.smoothness : gbuff.unpackedGbuffer.smoothness;
    gbuff.unpackedGbuffer.reflectance = isMemberMaskSet(flags, REFLECTANCE_MASK) ?  layer.reflectance : gbuff.unpackedGbuffer.reflectance;
    gbuff.unpackedGbuffer.shadow = isMemberMaskSet(flags, SHADOW_MASK) ?  layer.shadow : gbuff.unpackedGbuffer.shadow;
    gbuff.unpackedGbuffer.ao = isMemberMaskSet(flags, AO_MASK) ?  layer.ao : gbuff.unpackedGbuffer.ao;
    gbuff.unpackedGbuffer.emission_color = isMemberMaskSet(flags, EMISSION_COLOR_MASK) ?  layer.emission_color : gbuff.unpackedGbuffer.emission_color;
    gbuff.unpackedGbuffer.metalness = isMemberMaskSet(flags, METALNESS_MASK) ?  layer.metalness : gbuff.unpackedGbuffer.metalness;
    gbuff.unpackedGbuffer.translucency = isMemberMaskSet(flags, TRANSLUCENCY_MASK) ?  layer.translucency : gbuff.unpackedGbuffer.translucency;
    gbuff.unpackedGbuffer.emission_strength = isMemberMaskSet(flags, EMISSION_STRENGTH_MASK) ?  layer.emission_strength : gbuff.unpackedGbuffer.emission_strength;
    gbuff.unpackedGbuffer.sss_profile = isMemberMaskSet(flags, SSS_PROF_MASK) ?  layer.sss_profile : gbuff.unpackedGbuffer.sss_profile;
    gbuff.unpackedGbuffer.sheen = isMemberMaskSet(flags, SHEEN_MASK) ?  layer.sheen : gbuff.unpackedGbuffer.sheen;
    gbuff.unpackedGbuffer.dynamic = isMemberMaskSet(flags, DYNAMIC_MASK) ?  layer.dynamic : gbuff.unpackedGbuffer.dynamic;
    gbuff.unpackedGbuffer.autoMotionVecs = isMemberMaskSet(flags, AUTO_MOTION_VECS_MASK) ?  layer.autoMotionVecs : gbuff.unpackedGbuffer.autoMotionVecs;
    gbuff.unpackedGbuffer.isLandscape = isMemberMaskSet(flags, IS_LANDSCAPE_MASK) ?  layer.isLandscape : gbuff.unpackedGbuffer.isLandscape;
    gbuff.unpackedGbuffer.isHeroCockpit = isMemberMaskSet(flags, IS_HERO_COCKPIT_MASK) ?  layer.isHeroCockpit : gbuff.unpackedGbuffer.isHeroCockpit;
    gbuff.unpackedGbuffer.material = isMemberMaskSet(flags, MATERIAL_MASK) ?  layer.material : gbuff.unpackedGbuffer.material;
    setModifiedFromMix(gbuff.flags, flags);
  }
}

// Overwrite with Layer Node
void lerpWithLayer(inout NBSGbuffer gbuff, UnpackedGbuffer layer, uint flags, float weight, bool allowOverwrite)
{
  BRANCH
  if (weight > EPS)
  {
    gbuff.unpackedGbuffer.albedo = lerp(gbuff.unpackedGbuffer.albedo, layer.albedo, weight * isSetInt(flags, ALBEDO_MASK));
    gbuff.unpackedGbuffer.normal = lerp(gbuff.unpackedGbuffer.normal, layer.normal, weight * isSetInt(flags, NORMAL_MASK));
    gbuff.unpackedGbuffer.motion = lerp(gbuff.unpackedGbuffer.motion, layer.motion, weight * isSetInt(flags, MOTION_MASK));
    gbuff.unpackedGbuffer.reactive = lerp(gbuff.unpackedGbuffer.reactive, layer.reactive, weight * isSetInt(flags, REACTIVE_MASK));
    gbuff.unpackedGbuffer.smoothness = lerp(gbuff.unpackedGbuffer.smoothness, layer.smoothness, weight * isSetInt(flags, SMOOTHNESS_MASK));
    gbuff.unpackedGbuffer.reflectance = lerp(gbuff.unpackedGbuffer.reflectance, layer.reflectance, weight * isSetInt(flags, REFLECTANCE_MASK));
    gbuff.unpackedGbuffer.shadow = lerp(gbuff.unpackedGbuffer.shadow, layer.shadow, weight * isSetInt(flags, SHADOW_MASK));
    gbuff.unpackedGbuffer.ao = lerp(gbuff.unpackedGbuffer.ao, layer.ao, weight * isSetInt(flags, AO_MASK));
    gbuff.unpackedGbuffer.emission_color = lerp(gbuff.unpackedGbuffer.emission_color, layer.emission_color, weight * isSetInt(flags, EMISSION_COLOR_MASK));
    gbuff.unpackedGbuffer.metalness = lerp(gbuff.unpackedGbuffer.metalness, layer.metalness, weight * isSetInt(flags, METALNESS_MASK));
    gbuff.unpackedGbuffer.translucency = lerp(gbuff.unpackedGbuffer.translucency, layer.translucency, weight * isSetInt(flags, TRANSLUCENCY_MASK));
    gbuff.unpackedGbuffer.emission_strength = lerp(gbuff.unpackedGbuffer.emission_strength, layer.emission_strength, weight * isSetInt(flags, EMISSION_STRENGTH_MASK));
    gbuff.unpackedGbuffer.sheen = lerp(gbuff.unpackedGbuffer.sheen, layer.sheen, weight * isSetInt(flags, SSS_PROF_MASK));
    gbuff.unpackedGbuffer.sss_profile = isMemberMaskSet(flags, SSS_PROF_MASK) && allowOverwrite ?  layer.sss_profile : gbuff.unpackedGbuffer.sss_profile;
    gbuff.unpackedGbuffer.dynamic = isMemberMaskSet(flags, DYNAMIC_MASK) && allowOverwrite ?  layer.dynamic : gbuff.unpackedGbuffer.dynamic;
    gbuff.unpackedGbuffer.autoMotionVecs = isMemberMaskSet(flags, AUTO_MOTION_VECS_MASK) && allowOverwrite ?  layer.autoMotionVecs : gbuff.unpackedGbuffer.autoMotionVecs;
    gbuff.unpackedGbuffer.isLandscape = isMemberMaskSet(flags, IS_LANDSCAPE_MASK) && allowOverwrite ?  layer.isLandscape : gbuff.unpackedGbuffer.isLandscape;
    gbuff.unpackedGbuffer.isHeroCockpit = isMemberMaskSet(flags, IS_HERO_COCKPIT_MASK) && allowOverwrite ?  layer.isHeroCockpit : gbuff.unpackedGbuffer.isHeroCockpit;
    gbuff.unpackedGbuffer.material = isMemberMaskSet(flags, MATERIAL_MASK) && allowOverwrite ?  layer.material : gbuff.unpackedGbuffer.material;
    setModifiedFromMix(gbuff.flags, flags);
  }
}

void ditherWithLayer(inout NBSGbuffer gbuff, UnpackedGbuffer layer, uint flags, float weight, float bias, uint2 tc)
{
  BRANCH
  if (weight > EPS && envi_cover_is_temporal_aa != 0)
  {
    half dither_limit = half(((tc.x + (envi_cover_frame_idx & 1)) % 2) + (((envi_cover_frame_idx >> 1) + tc.y) % 2)) / 3;

    if(weight >= dither_limit + bias)
      overwriteWithLayer(gbuff, layer, flags, true);
  }
  else if (weight > EPS && envi_cover_is_temporal_aa == 0)
  {
    lerpWithLayer(gbuff, layer, flags, weight, true);
  }
}


void applySparkles(inout NBSGbuffer gbuff, float3 sparkle_albedo, float sparkle_reflectance, float sparkle_smoothness, float3 world_pos, float3 world_view_pos, float max_alpha,
  float size, float density, bool should_calculate, bool apply_to_normal, bool apply_to_smoothness, bool apply_to_albedo, bool apply_to_reflectance)
{
  if (!should_calculate)
    return;

  float3 normal = gbuff.unpackedGbuffer.normal;
  half smoothness = gbuff.unpackedGbuffer.smoothness;
  half3 albedo = gbuff.unpackedGbuffer.albedo;
  half reflectance = gbuff.unpackedGbuffer.reflectance;
  computeSnowSparkle_impl(world_pos, world_view_pos, max_alpha, size, density, sparkle_albedo, sparkle_reflectance, normal, albedo, smoothness, reflectance, sparkle_smoothness);
  if (apply_to_normal)
  {
    gbuff.unpackedGbuffer.normal = normal;
    setModified(gbuff.flags, SELECT_NORMAL_MATERIAL);
  }
  if(apply_to_smoothness)
  {
    gbuff.unpackedGbuffer.smoothness = smoothness;
    setModified(gbuff.flags, SELECT_SRTMSD);
  }
  if(apply_to_albedo)
  {
    gbuff.unpackedGbuffer.albedo = albedo;
    setModified(gbuff.flags, SELECT_ALBEDO_AO);
  }
  if(apply_to_reflectance)
  {
    gbuff.unpackedGbuffer.reflectance = reflectance;
    setModified(gbuff.flags, SELECT_SRTMSD);
  }

}