void unpackGbufAlbedoAO(half4 albedo_ao, out half3 albedo, out half ao)
{
  albedo = albedo_ao.xyz;
  ao = albedo_ao.w;
}

void unpackGbufTranslucencyChannelW(half input, out uint flags, out uint shadowOrSSSProfile)
{
  uint shadowAndDynamic = uint(input * 255 + 0.1);
  flags = shadowAndDynamic & 7;
  shadowOrSSSProfile = shadowAndDynamic & SSS_PROFILE_MASK;
}

half4 packGbufSmoothnessReflectancMetallTranslucencyShadow(half smoothness, half reflectance, half translucencyorMetalness, uint flags, uint shadowOrSSSProfile)
{
  half lastEncoded = ((flags & 7) + (shadowOrSSSProfile & SSS_PROFILE_MASK)) / 255.0h;
  return half4(smoothness, reflectance, translucencyorMetalness, lastEncoded);
}

#if GBUFFER_NORMALS_PACKED
  void unpackExtraBitsAndMaterial(float4 normal_material, out uint extra_bits, out uint material)
  {
    extra_bits = uint(normal_material.z * 1023.h);
    material = uint(normal_material.w*(BASE_MAT_CNT-0.50001f));
    uint advancedMaterialOffset = (extra_bits & ADV_MAT_MASK_IN_EXTRA) << ADV_MAT_CONVERT_SHIFT;
    material += advancedMaterialOffset;
  }
  uint packExtraBits(uint dynamic_mask, uint material)
  {
    uint advancedMaterialBit = ((material & ADV_MAT_MASK_IN_MAT) >> ADV_MAT_CONVERT_SHIFT);
    return dynamic_mask | advancedMaterialBit;
  }
  half4 packExtraBitsNormalAndMaterial(float3 normal, uint extra_bits, uint material)
  {
    return half4(pack_normal_so(half3(normal)), extra_bits / 1023.h, material * (1.h/3.0h));
  }
#else
  void unpackGbufNormalMaterial(float4 normal_material, out float3 normal, out uint material)
  {
    material = uint(normal_material.w*(BASE_MAT_CNT-0.50001f));
    normal = normal_material.xyz*2-1;
  }
  float4 packGbufNormalMaterial(float3 normal, uint material)
  {
    return float4(normal*0.5h+0.5h, material * (1.h/3.0h));
  }
#endif