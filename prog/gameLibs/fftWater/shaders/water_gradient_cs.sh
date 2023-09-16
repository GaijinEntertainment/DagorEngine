include "water_gradient_common.sh"

float4 water_texture_size_mip = (512, 512, 1/512, 1/512);

int water_gradient_cs_color_uav_slot = 0 always_referenced;
int water_foam_blur_cs_color_uav_slot = 0 always_referenced;
int water_gradient_mip_cs_color_uav_slot = 0 always_referenced;

shader water_gradient_cs
{
  (cs) {
    water_displacement_texture0@smpArray = water_displacement_texture0;
    texsz_consts@f4 = water_texture_size;
    water_gradient_scales0@f2 = water_gradient_scales0;
    water_gradient_scales1@f2 = water_gradient_scales1;
  }
  if (water_cascades != two && water_cascades != one_to_four)
  {
    (cs) { water_gradient_scales2@f2 = water_gradient_scales2; }
    if (water_cascades != three)
    {
      (cs) { water_gradient_scales3@f2 = water_gradient_scales3; }
      if (water_cascades != four)
      {
        (cs) { water_gradient_scales4@f2 = water_gradient_scales4; }
      }
    }
  }

  hlsl(cs) {
    #include "water_gradient_cs_const.hlsli"

    RWTexture2DArray<float4> color0 : register(u0);

    [numthreads( WATER_GRADIENT_CS_WARP_SIZE, WATER_GRADIENT_CS_WARP_SIZE, 1 )]
    void water_gradient_cs(uint2 id : SV_DispatchThreadID)
    {
      if (any(id > (uint2)texsz_consts.xy))
        return;

      #define GET_GRADIENT(no, gradient, J)\
      {\
        float3 displace_left = tex3Dlod(water_displacement_texture0, float4((id + int2(-1, 0)) * texsz_consts.zw, no, 0).yxzw).rgb;\
        float3 displace_right = tex3Dlod(water_displacement_texture0, float4((id + int2(1, 0)) * texsz_consts.zw, no, 0).yxzw).rgb;\
        float3 displace_back = tex3Dlod(water_displacement_texture0, float4((id + int2(0, -1)) * texsz_consts.zw, no, 0).yxzw).rgb;\
        float3 displace_front = tex3Dlod(water_displacement_texture0, float4((id + int2(0, 1)) * texsz_consts.zw, no, 0).yxzw).rgb;\
        gradient = float2(-(displace_right.z - displace_left.z) / max(0.01,1.0 + water_gradient_scales##no.y*(displace_right.x - displace_left.x)), -(displace_front.z - displace_back.z) / max(0.01,1.0+water_gradient_scales##no.y*(displace_front.y - displace_back.y)));\
        float2 Dx = (displace_right.xy - displace_left.xy) * water_gradient_scales##no.x;\
        float2 Dy = (displace_front.xy - displace_back.xy) * water_gradient_scales##no.x;\
        J = (1.0f + Dx.x) * (1.0f + Dy.y) - Dx.y * Dy.x;\
      }
      #define OUTPUT_GRADIENT(no, pos, gradient, J) color0[uint3(pos, no)] = float4(gradient, J, 0)

      // Sample neighbour texels
      float2 gradient0, gradient1, gradient2, gradient3, gradient4;
      float J0, J1, J2, J3, J4;

      GET_GRADIENT(0, gradient0, J0);
      ##if (water_cascades != one_to_four)
        GET_GRADIENT(1, gradient1, J1);
        ##if (water_cascades != two)
          GET_GRADIENT(2, gradient2, J2);
          ##if (water_cascades != three)
            GET_GRADIENT(3, gradient3, J3);
            ##if (water_cascades != four)
              GET_GRADIENT(4, gradient4, J4);
            ##endif
          ##endif
        ##endif
      ##endif

      // Output
      uint2 outputPos = id; // only if resolution of water_displacement_texture and output rwtextures are matching!
      OUTPUT_GRADIENT(0, outputPos, gradient0, J0);
      ##if (water_cascades != one_to_four)
        OUTPUT_GRADIENT(1, outputPos, gradient1, J1);
        ##if (water_cascades != two)
          OUTPUT_GRADIENT(2, outputPos, gradient2, J2);
          ##if (water_cascades != three)
            OUTPUT_GRADIENT(3, outputPos, gradient3, J3);
            ##if (water_cascades != four)
              OUTPUT_GRADIENT(4, outputPos, gradient4, J4);
            ##endif
          ##endif
        ##endif
      ##endif
    }
  }
  compile("cs_5_0", "water_gradient_cs");
}

shader water_foam_blur_cs
{
  (cs) {
    foam_blur_extents0123@f4 = foam_blur_extents0123;
    texsz_consts@f4 = water_texture_size;
    water_foam_texture0@smpArray = water_foam_texture0;
    foam_dissipation@f3 = (foam_dissipation.x*0.25, foam_dissipation.y/0.25, foam_dissipation.z, 0);
  }
  if (water_cascades == five)
  {
    (cs) { foam_blur_extents4567@f4 = foam_blur_extents4567; }
  }

  hlsl(cs) {
    RWTexture2DArray<float4> color0 : register(u0);
  }

  hlsl(cs) {
    #include "water_gradient_cs_const.hlsli"

    [numthreads( WATER_FOAM_BLUR_CS_WARP_SIZE, WATER_FOAM_BLUR_CS_WARP_SIZE, 1 )]
    void water_foam_blur_cs(uint2 id : SV_DispatchThreadID)
    {
      if (any(id > (uint2)texsz_consts.xy))
        return;

      float2 tc = (id + 0.5f) * texsz_consts.zw;
      uint2 outputPos = id; // only if resolution of water_displacement_texture and output rwtextures are matching!

      // Sample neighbour texels

      // blur with variable size kernel is done by doing 4 bilinear samples,
      // each sample is slightly offset from the center point

      half4 energy0123 = 0, energy4567 = 0;

      #define GET_ENERGY(water_foam_texture, cascade_group, attr, no)\
      {\
        float2 UVoffset = float2(0, foam_blur_extents##cascade_group.attr);\
        half foamenergy1  = tex3Dlod(water_foam_texture0, float4(tc + UVoffset, no, 0)).w;\
        half foamenergy2  = tex3Dlod(water_foam_texture0, float4(tc - UVoffset, no, 0)).w;\
        half foamenergy3  = tex3Dlod(water_foam_texture0, float4(tc + UVoffset*2.0, no, 0)).w;\
        half foamenergy4  = tex3Dlod(water_foam_texture0, float4(tc - UVoffset*2.0, no, 0)).w;\
        half folding = max(0, tex3Dlod(water_foam_texture0, float4(tc, no, 0)).z);\
        energy##cascade_group.attr = foam_dissipation.x*((foamenergy1 + foamenergy2 + foamenergy3 + foamenergy4) + max(0,(1.0-folding-foam_dissipation.z))*foam_dissipation.y);\
      }

      GET_ENERGY(water_foam_texture, 0123, x, 0);
      ##if (water_cascades != one_to_four)
        GET_ENERGY(water_foam_texture, 0123, y, 1);
        ##if (water_cascades != two)
          GET_ENERGY(water_foam_texture, 0123, z, 2);
          ##if (water_cascades != three)
            GET_ENERGY(water_foam_texture, 0123, w, 3);
            ##if (water_cascades != four)
              GET_ENERGY(water_foam_texture, 4567, x, 4);
            ##endif
          ##endif
        ##endif
      ##endif
      energy0123 = min(half4(1.0,1.0,1.0,1.0), energy0123);
      ##if water_cascades == five
        energy4567 = min(half4(1.0,1.0,1.0,1.0), energy4567);
      ##endif

      color0[uint3(outputPos, 0)] = energy0123;
      ##if water_cascades == five
        color0[uint3(outputPos, 1)] = energy4567;
      ##endif
    }
  }

  compile("cs_5_0", "water_foam_blur_cs");
}

shader water_gradient_foam_combine_cs
{
  (cs) {
    foam_blur_extents0123@f4 = foam_blur_extents0123;
    texsz_consts@f4 = water_texture_size;

    water_displacement_texture0@smpArray = water_displacement_texture0;
    water_foam_texture0@smpArray = water_foam_texture0;

    water_gradient_scales0@f2 = water_gradient_scales0;
    water_gradient_scales1@f2 = water_gradient_scales1;
  }

  if (water_cascades != two && water_cascades != one_to_four)
  {
    (cs) { water_gradient_scales2@f2 = water_gradient_scales2; }
    if (water_cascades != three)
    {
      (cs) { water_gradient_scales3@f2 = water_gradient_scales3; }
      if (water_cascades != four)
      {
        (cs) {
          foam_blur_extents4567@f4 = foam_blur_extents4567;
          water_gradient_scales4@f2 = water_gradient_scales4;
        }
      }
    }
  }

  hlsl(cs) {
    #include "water_gradient_cs_const.hlsli"

    RWTexture2DArray<float4> color0 : register(u0);

    [numthreads( WATER_GRADIENT_FOAM_COMBINE_WARP_SIZE, WATER_GRADIENT_FOAM_COMBINE_WARP_SIZE, 1 )]
    void water_gradient_foam_combine_cs(uint2 id : SV_DispatchThreadID)
    {
      if (any(id > (uint2)texsz_consts.xy))
        return;

      float2 tc = (id + 0.5f) * texsz_consts.zw;

      float4 result0;
      ##if (water_cascades != one_to_four)
        float4 result1;
        ##if (water_cascades != two)
          float4 result2;
          ##if (water_cascades != three)
            float4 result3;
            ##if (water_cascades != four)
              float4 result4;
            ##endif
          ##endif
        ##endif
      ##endif

      // Calculate gradients and store them in result.rgb
      {
        // Sample neighbour texels
        #define GET_GRADIENT(no, gradient, J)\
        {\
          float3 displace_left = tex3Dlod(water_displacement_texture0, float4((id + int2(-1, 0)) * texsz_consts.zw, no, 0).yxzw).rgb;\
          float3 displace_right = tex3Dlod(water_displacement_texture0, float4((id + int2(1, 0)) * texsz_consts.zw, no, 0).yxzw).rgb;\
          float3 displace_back = tex3Dlod(water_displacement_texture0, float4((id + int2(0, -1)) * texsz_consts.zw, no, 0).yxzw).rgb;\
          float3 displace_front = tex3Dlod(water_displacement_texture0, float4((id + int2(0, 1)) * texsz_consts.zw, no, 0).yxzw).rgb;\
          gradient = float2(-(displace_right.z - displace_left.z) / max(0.01,1.0 + water_gradient_scales##no.y*(displace_right.x - displace_left.x)), -(displace_front.z - displace_back.z) / max(0.01,1.0+water_gradient_scales##no.y*(displace_front.y - displace_back.y)));\
          float2 Dx = (displace_right.xy - displace_left.xy) * water_gradient_scales##no.x;\
          float2 Dy = (displace_front.xy - displace_back.xy) * water_gradient_scales##no.x;\
          J = (1.0f + Dx.x) * (1.0f + Dy.y) - Dx.y * Dy.x;\
        }

        float2 gradient0, gradient1, gradient2, gradient3, gradient4;
        float J0, J1, J2, J3, J4;

        GET_GRADIENT(0, gradient0, J0);
        ##if (water_cascades != one_to_four)
          GET_GRADIENT(1, gradient1, J1);
          ##if (water_cascades != two)
            GET_GRADIENT(2, gradient2, J2);
            ##if (water_cascades != three)
              GET_GRADIENT(3, gradient3, J3);
              ##if (water_cascades != four)
                GET_GRADIENT(4, gradient4, J4);
              ##endif
            ##endif
          ##endif
        ##endif

        result0.rgb = float3(gradient0, J0);
        ##if (water_cascades != one_to_four)
          result1.rgb = float3(gradient1, J1);
          ##if (water_cascades != two)
            result2.rgb = float3(gradient2, J2);
            ##if (water_cascades != three)
              result3.rgb = float3(gradient3, J3);
              ##if (water_cascades != four)
                result4.rgb = float3(gradient4, J4);
              ##endif
            ##endif
          ##endif
        ##endif
      }

      // Calculate foam and store them in result.a
      {
        #define GET_ENERGY(water_foam_texture, slice, cascade_group, attr)\
        {\
          float2 UVoffset = float2(foam_blur_extents##cascade_group.attr, 0);\
          half foamenergy1 = tex3Dlod(water_foam_texture, float4(tc + UVoffset, slice, 0)).attr;\
          half foamenergy2 = tex3Dlod(water_foam_texture, float4(tc - UVoffset, slice, 0)).attr;\
          half foamenergy3 = tex3Dlod(water_foam_texture, float4(tc + UVoffset*2.0, slice, 0)).attr;\
          half foamenergy4 = tex3Dlod(water_foam_texture, float4(tc - UVoffset*2.0, slice, 0)).attr;\
          energy##cascade_group.attr = 0.25*(foamenergy1+foamenergy2+foamenergy3+foamenergy4);\
        }

        half4 energy0123;
        half4 energy4567;



        GET_ENERGY(water_foam_texture0, 0, 0123, x);
        ##if (water_cascades != one_to_four)
          GET_ENERGY(water_foam_texture0, 0, 0123, y);
          ##if (water_cascades != two)
            GET_ENERGY(water_foam_texture0, 0, 0123, z);
            ##if (water_cascades != three)
              GET_ENERGY(water_foam_texture0, 0, 0123, w);
              ##if (water_cascades != four)
                GET_ENERGY(water_foam_texture0, 1, 4567, x);
              ##endif
            ##endif
          ##endif
        ##endif

        result0.a = energy0123.x;
        ##if (water_cascades != one_to_four)
          result1.a = energy0123.y;
          ##if (water_cascades != two)
            result2.a = energy0123.z;
            ##if (water_cascades != three)
              result3.a = energy0123.w;
              ##if (water_cascades != four)
                result4.a = energy4567.x;
              ##endif
            ##endif
          ##endif
        ##endif
      }

      // Output result
      color0[uint3(id, 0)] = result0;
      ##if (water_cascades != one_to_four)
      color0[uint3(id, 1)] = result1;
      ##endif
      ##if (water_cascades != two && water_cascades != one_to_four)
        color0[uint3(id, 2)] = result2;
        ##if (water_cascades != three)
          color0[uint3(id, 3)] = result3;
          ##if (water_cascades != four)
            color0[uint3(id, 4)] = result4;
          ##endif
        ##endif
      ##endif
    }
  }
  compile("cs_5_0", "water_gradient_foam_combine_cs");
}

shader water_gradient_mip_cs
{
  (cs) {
    water_gradients_tex0@smpArray = water_gradients_tex0
    texsz_consts@f4 = water_texture_size_mip;
  }

  hlsl(cs) {
    #include "water_gradient_cs_const.hlsli"

    RWTexture2DArray<float4> color0 : register(u0);

    [numthreads( WATER_GRADIENT_MIP_CS_WARP_SIZE, WATER_GRADIENT_MIP_CS_WARP_SIZE, 1 )]
    void water_gradient_mip_cs(uint2 id : SV_DispatchThreadID)
    {
      if (any(id > texsz_consts.xy))
        return;

      float2 tc = (id + 0.5f) * texsz_consts.zw;

      const float attenuation = 0.91;

      #define WRITE_GRADIENT(cascadeNo)                                                          \
        float4 cascade##cascadeNo = tex3Dlod(water_gradients_tex0, float4(tc.xy, cascadeNo, 0)); \
        cascade##cascadeNo.a *= attenuation;                                                     \
        color0[uint3(id, cascadeNo)] = cascade##cascadeNo;

      WRITE_GRADIENT(0)

      ##if (water_cascades != one_to_four)
        WRITE_GRADIENT(1)
        ##if (water_cascades != two)
          WRITE_GRADIENT(2)
          ##if (water_cascades != three)
            WRITE_GRADIENT(3)
            ##if (water_cascades != four)
              WRITE_GRADIENT(4)
            ##endif
          ##endif
        ##endif
      ##endif
    }
  }

  compile("cs_5_0", "water_gradient_mip_cs");
}
