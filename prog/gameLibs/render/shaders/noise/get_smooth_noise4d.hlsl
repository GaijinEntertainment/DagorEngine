
float get_smooth_noise4d(float4 v)
{
  float4 i = floor(v.xyzw);
  float4 f = frac(v.xyzw);

  // using ease curve for w component, it causes frame stutter.
  f.xyz = f.xyz * f.xyz * (-2.0f * f.xyz + 3.0f);
  //f.xyz = f.xyz * f.xyz * f.xyz * (f.xyz * (f.xyz * 6.0f - 15.0f) + 10.0f);

  float2 uv = ((i.xy + float2(7.0f, 17.0f) * i.z) + float2(89.0f, 113.0f) * i.w) + f.xy;
  float lowz = tex2Dlod(noise_sampler, float4((uv.xy + 0.5f) / 64.0f, 0, 0)).x;
  uv.xy += float2(7.0f, 17.0f);//uv = ((i.xy + float2(7.0f, 17.0f) * (i.z + 1)) + float2(89.0f, 113.0f) * i.w) + f.xy;
  float highz = tex2Dlod(noise_sampler, float4((uv.xy + 0.5f) / 64.0f, 0, 0)).x;
  float r0 = lerp(lowz, highz, f.z);

  uv.xy += float2(89.0f, 113.0f);//uv = ((i.xy + float2(7.0f, 17.0f) * (i.z + 1)) + float2(89.0f, 113.0f) * (i.w + 1)) + f.xy;
  highz = tex2Dlod(noise_sampler, float4((uv.xy + 0.5f) / 64.0f, 0, 0)).x;
  uv.xy -= float2(7.0f, 17.0f);//uv = ((i.xy + float2(7.0f, 17.0f) * i.z) + float2(89.0f, 113.0f) * (i.w + 1)) + f.xy;
  lowz = tex2Dlod(noise_sampler, float4((uv.xy + 0.5f) / 64.0f, 0, 0)).x;
  float r1 = lerp(lowz, highz, f.z);

  uv.xy += float2(89.0f, 113.0f);//uv = ((i.xy + float2(7.0f, 17.0f) * i.z) + float2(89.0f, 113.0f) * (i.w + 2)) + f.xy;
  lowz = tex2Dlod(noise_sampler, float4((uv.xy + 0.5f) / 64.0f, 0, 0)).x;
  uv.xy += float2(7.0f, 17.0f);//uv = ((i.xy + float2(7.0f, 17.0f) * (i.z + 1)) + float2(89.0f, 113.0f) * (i.w + 2)) + f.xy;
  highz = tex2Dlod(noise_sampler, float4((uv.xy + 0.5f) / 64.0f, 0, 0)).x;
  float r2 = lerp(lowz, highz, f.z);

  // smooth the noise
  r0 = (r0 + r1) * 0.5f;
  r1 = (r1 + r2) * 0.5f;

  float r = lerp(r0, r1, f.w);

  return r;
}

float get_smooth_noise4d_fast(float4 v)
{
  float4 i = floor(v.xyzw);
  float4 f = frac(v.xyzw);

  f.xyz = f.xyz * f.xyz * (-2.0f * f.xyz + 3.0f);

  float2 uv = ((i.xy + float2(7.0f, 17.0f) * i.z) + float2(89.0f, 113.0f) * i.w) + f.xy;
  float lowz = tex2Dlod(noise_sampler, float4((uv.xy + 0.5f) / 64.0f, 0, 0)).x;
  uv.xy += float2(7.0f, 17.0f);
  float highz = tex2Dlod(noise_sampler, float4((uv.xy + 0.5f) / 64.0f, 0, 0)).x;
  float r0 = lerp(lowz, highz, f.z);

  uv.xy += float2(89.0f, 113.0f);
  highz = tex2Dlod(noise_sampler, float4((uv.xy + 0.5f) / 64.0f, 0, 0)).x;
  uv.xy -= float2(7.0f, 17.0f);
  lowz = tex2Dlod(noise_sampler, float4((uv.xy + 0.5f) / 64.0f, 0, 0)).x;
  float r1 = lerp(lowz, highz, f.z);

  float r = lerp(r0, r1, f.w);

  return r;
}

float get_smooth_noise3d(float3 v)
{
  float3 i = floor(v.xyz);
  float3 f = frac(v.xyz);

  f.xyz = f.xyz * f.xyz * (-2.0f * f.xyz + 3.0f);

  float2 uv = ((i.xy + float2(7.0f, 17.0f) * i.z)) + f.xy;
  float lowz = tex2Dlod(noise_sampler, float4((uv.xy + 0.5f) / 64.0f, 0, 0)).x;
  uv.xy += float2(7.0f, 17.0f);
  float highz = tex2Dlod(noise_sampler, float4((uv.xy + 0.5f) / 64.0f, 0, 0)).x;
  float r0 = lerp(lowz, highz, f.z);

  return r0;
}

float4 get_smooth_noise2d(float2 v)//just smoothstep on coord
{
  float2 i = floor(v.xy);
  float2 f = frac(v.xy);

  f.xy = f.xy * f.xy * (-2.0f * f.xy + 3.0f);

  float2 uv = i.xy + f.xy;
  return tex2Dlod(noise_sampler, float4(uv.xy*(1./64.0f) + 0.5f / 64.0f, 0, 0));
}
