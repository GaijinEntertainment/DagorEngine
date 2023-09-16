macro DECLARE_APPLY_TREE_WIND()
  static float4 wind_channel_strength = (1, 1, 0, 1);
endmacro

macro INIT_APPLY_TREE_WIND()
  (vs)
  {
    wind_channel_strength@f3 = wind_channel_strength;
  }
endmacro

macro USE_APPLY_TREE_WIND()

hlsl(vs) {
  float4 fast_cos(float4 x)
  {
    return (1 - 0.5*x*x);
  }
  float TriangleWave2(float x)
  {
    float4 CosParams = float4(0.03,0.09,0.027,0.039);
    return dot(fast_cos(frac(x*CosParams)),1);
  }
  float TriangleWave1(float x)
  {
    float y = abs(frac(x + 0.5) * 2. - 1.);
    return y * y * (3. - 2. * y);
  }
  float3 ApplyTreeWind(float3 vcol, float4 tree_wind_params, float3 worldLocalPos, float3 worldPos, float3 inputPos, float3 worldNormal,
                        float weight, float noise_speed_mult)
  {
    const float BASE_TREE_WIND_SPEED_MULT = 0.1;
    const float BASE_TREE_WAVE_AMP = 0.2;
    const float MIN_WIND_SPEED = 0.0001;
    float3 resPos = worldLocalPos;
    float fTime = tree_wind_params.x;
    float fBranchAmp = tree_wind_params.y;
    float fDetailAmp = tree_wind_params.z;
    float3 wind = BASE_TREE_WIND_SPEED_MULT*sampleWindCurrentTime(worldPos, noise_speed_mult,0);
    float fSpeedSq = dot(wind.xz, wind.xz);
    FLATTEN
    if (fSpeedSq < MIN_WIND_SPEED*MIN_WIND_SPEED)
      return float3(0, 0, 0);
    float fSpeed = sqrt(fSpeedSq);
    wind /= fSpeed;
    weight *= fSpeed;

    float fEdgeAtten = vcol.x;
    float fBranchPhase = vcol.y;
    float fBranchAtten = vcol.z;

    float fObjPhase = dot(worldLocalPos.xyz, 2);
    fBranchPhase += fObjPhase;
    float fVtxPhase = dot(BASE_TREE_WAVE_AMP*inputPos.xyz, fBranchPhase);
    float bendingStrength = weight * fDetailAmp *fEdgeAtten;
    float waves = TriangleWave1(BASE_TREE_WAVE_AMP * fTime + fVtxPhase);
    resPos.xz += bendingStrength * waves * worldNormal.xz;
    resPos.y += weight * fBranchAmp * fBranchAtten * waves;
    float fBF = bendingStrength * lerp(TriangleWave2(fTime+fObjPhase),0.5,0.5);

    float3 newPos = resPos-worldLocalPos;
    float fLength = length(newPos);
    newPos.xzy += wind.xzy * fBF;
    float len = length(newPos);
    if (len > 0.0001) //prevent zero vector normalize
      newPos /= len;
    return newPos * fLength;
  }
}
endmacro