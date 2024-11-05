  half ComputeTextureLOD(in float2 texDim, float2 dx, float2 dy)
  {
    //float2 mag = ddx_ * ddx_ + ddy_ * ddy_;
    half2 mag = (abs(dx) + abs(dy))*texDim;

    //float lod = max(0.5 * log2(max(mag.x, mag.y)), 0);
    half lod = log2(max(mag.x, mag.y));
    return lod;
  }
  #ifndef parallax_tex_sampler
    #define parallax_tex_sampler(tex, tc) tex2Dlod(tex, tc)
  #endif
  float2 ParallaxOcclusionMap(in float4 baseTC, in half3 viewDirNrm, in int numSteps, in int numSteps_bil, in half displacement, out half4 outTex)
  {
    half stepSize =  1.0 / numSteps;
    half bumpScale = displacement;

    half2 delta = half2(viewDirNrm.x, viewDirNrm.y) * bumpScale / (-viewDirNrm.z * numSteps); // / max(-viewDirNrm.z * numSteps, 0.1)

    
    half height = 1 - stepSize;
    half4 offset = baseTC;
    offset.xy += delta;

    half4 NB0 = parallax_tex_sampler(PARALLAX_TEX, baseTC);
    outTex = parallax_tex_sampler(PARALLAX_TEX, offset);

    
    int i;
    for (i=0; i<numSteps; i++)
    {
        FLATTEN
        if (outTex.PARALLAX_ATTR >= height)
            break;

        NB0 = outTex;

        height -= stepSize;
        offset.xy += delta;

        outTex = parallax_tex_sampler(PARALLAX_TEX, offset);
    }
    
    half4 offsetBest = offset;
    half error = 1.0;

    half t1 = height;
    half t0 = t1 + stepSize;

    half delta1 = t1 - outTex.PARALLAX_ATTR;
    half delta0 = t0 - NB0.PARALLAX_ATTR;

    half4 intersect = half4(delta * numSteps, delta * numSteps + baseTC.xy);
    
    for (i=0; i<numSteps_bil; i++)
    {
        FLATTEN
        if (abs(error) <= 0.01)
            break;

        half denom = delta1 - delta0;
        half t = (t0 * delta1 - t1 * delta0) / denom;
        offsetBest.xy = -t * intersect.xy + intersect.zw;

        outTex = parallax_tex_sampler(PARALLAX_TEX, offsetBest);

        error = t - outTex.PARALLAX_ATTR;
        if (error < 0)
        {
            delta1 = error;
            t1 = t;
        }
        else
        {
            delta0 = error;
            t0 = t;
        }
    }
    return offsetBest.xy;
  }
  #ifndef num_parallax_iterations
   #define num_parallax_iterations 4
  #endif
  float2 get_parallax(float2 parallaxTS, float4 texCoord, float str)
  {
    //half2 btc = input.tc.xy;
    //half lod = ComputeTextureLOD(texCoord, half2(256,256), dx,dy);
    parallaxTS *= (str/num_parallax_iterations);
    float4 tc = texCoord;
    //half4 tc = half4(texCoord,0,lod);
    for (int i=0; i<num_parallax_iterations; ++i)
    {
      float height = 1-parallax_tex_sampler(PARALLAX_TEX, tc).PARALLAX_ATTR;
      tc.xy += parallaxTS.xy*height;
    }
    return tc.xy;
  }
