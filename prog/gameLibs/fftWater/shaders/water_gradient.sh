include "water_gradient_common.sh"

shader water_gradient, water_foam_blur
{
  cull_mode=none;
  z_test=false;
  z_write=false;

  hlsl {

    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 tc : TEXCOORD0;
    };
  }

  USE_POSTFX_VERTEX_POSITIONS()
  (vs) { texsz_consts@f2 = (water_texture_size.z * 0.5, water_texture_size.w * 0.5, 0, 0); }
  hlsl(vs) {
    VsOutput water_gradient_vs(uint vertex_id : SV_VertexID)
    {
      VsOutput output;
      float2 pos = getPostfxVertexPositionById(vertex_id);
      output.pos = float4(pos.x, pos.y, 1, 1);
      output.tc = pos*RT_SCALE_HALF+float2(0.5, 0.5);
      ##if shader == water_foam_blur
        output.tc -= texsz_consts.xy;
      ##endif
      return output;
    }
  }

  (ps) { texsz_consts@f3 = (water_texture_size.z, water_texture_size.w, 0, 0); }

  if (shader == water_gradient)
  {
    if (support_texture_array == on)
    {
      (ps) { water_displacement_texture0@smpArray = water_displacement_texture0; }
    }
    else
    {
      (ps) { water_displacement_texture1@smp2d = water_displacement_texture1; }
      if (water_cascades != two)
      {
        (ps) { water_displacement_texture2@smp2d = water_displacement_texture2; }
        if (water_cascades != three)
        {
          (ps) { water_displacement_texture3@smp2d = water_displacement_texture3; }
          if (water_cascades != four)
          {
            (ps) { water_displacement_texture4@smp2d = water_displacement_texture4; }
          }
        }
      }
    }

    (ps) {
      water_gradient_scales0@f2 = water_gradient_scales0;
      water_gradient_scales1@f2 = water_gradient_scales1;
    }
    if (water_cascades != two && water_cascades != one_to_four)
    {
      (ps) { water_gradient_scales2@f2 = water_gradient_scales2; }
      if (water_cascades != three)
      {
        (ps) { water_gradient_scales3@f2 = water_gradient_scales3; }
        if (water_cascades != four)
        {
          (ps) { water_gradient_scales4@f2 = water_gradient_scales4; }
        }
      }
    }
    hlsl(ps) {
      struct PsOut
      {
        half4 color0 : SV_Target0;
        ##if water_cascades != one_to_four
          half4 color1 : SV_Target1;
          ##if (water_cascades != two)
            half4 color2 : SV_Target2;
            ##if (water_cascades != three)
              half4 color3 : SV_Target3;
              ##if (water_cascades != four)
                half4 color4 : SV_Target4;
              ##endif
            ##endif
          ##endif
        ##endif
      };
      PsOut water_gradient_ps(VsOutput input)
      {
        // Sample neighbour texels
        float2 gradient0,gradient1, gradient2, gradient3, gradient4;
        float J0, J1, J2, J3, J4;

        ##if (support_texture_array == on)
        #define GET_GRADIENT(water_displacement_texture, no, water_gradient_scales, gradient, J)\
        {\
          float3 displace_left  = tex3D(water_displacement_texture0, float3(input.tc.xy - texsz_consts.xz, no).yxz).rgb;\
          float3 displace_right = tex3D(water_displacement_texture0, float3(input.tc.xy + texsz_consts.xz, no).yxz).rgb;\
          float3 displace_back  = tex3D(water_displacement_texture0, float3(input.tc.xy - texsz_consts.zy, no).yxz).rgb;\
          float3 displace_front = tex3D(water_displacement_texture0, float3(input.tc.xy + texsz_consts.zy, no).yxz).rgb;\
          gradient = float2(-(displace_right.z - displace_left.z) / max(0.01,1.0 + water_gradient_scales.y*(displace_right.x - displace_left.x)), -(displace_front.z - displace_back.z) / max(0.01,1.0+water_gradient_scales.y*(displace_front.y - displace_back.y)));\
          float2 Dx = (displace_right.xy - displace_left.xy) * water_gradient_scales.x;\
          float2 Dy = (displace_front.xy - displace_back.xy) * water_gradient_scales.x;\
          J = (1.0f + Dx.x) * (1.0f + Dy.y) - Dx.y * Dy.x;\
        }
        ##else
        #define GET_GRADIENT(water_displacement_texture, no, water_gradient_scales, gradient, J)\
        {\
          float3 displace_left  = tex2D(water_displacement_texture##no, (input.tc.xy - texsz_consts.xz).yx).rgb;\
          float3 displace_right = tex2D(water_displacement_texture##no, (input.tc.xy + texsz_consts.xz).yx).rgb;\
          float3 displace_back  = tex2D(water_displacement_texture##no, (input.tc.xy - texsz_consts.zy).yx).rgb;\
          float3 displace_front = tex2D(water_displacement_texture##no, (input.tc.xy + texsz_consts.zy).yx).rgb;\
          gradient = float2(-(displace_right.z - displace_left.z) / max(0.01,1.0 + water_gradient_scales.y*(displace_right.x - displace_left.x)), -(displace_front.z - displace_back.z) / max(0.01,1.0+water_gradient_scales.y*(displace_front.y - displace_back.y)));\
          float2 Dx = (displace_right.xy - displace_left.xy) * water_gradient_scales.x;\
          float2 Dy = (displace_front.xy - displace_back.xy) * water_gradient_scales.x;\
          J = (1.0f + Dx.x) * (1.0f + Dy.y) - Dx.y * Dy.x;\
        }
        ##endif

        // Output
        GET_GRADIENT(water_displacement_texture, 0, water_gradient_scales0, gradient0, J0);
        ##if water_cascades != one_to_four
          GET_GRADIENT(water_displacement_texture, 1, water_gradient_scales1, gradient1, J1);
          ##if (water_cascades != two)
            GET_GRADIENT(water_displacement_texture, 2, water_gradient_scales2, gradient2, J2);
            ##if (water_cascades != three)
              GET_GRADIENT(water_displacement_texture, 3, water_gradient_scales3, gradient3, J3);
              ##if (water_cascades != four)
                GET_GRADIENT(water_displacement_texture, 4, water_gradient_scales4, gradient4, J4);
              ##endif
            ##endif
          ##endif
        ##endif

        PsOut result;
        result.color0.xyzw = float4(gradient0, J0, 0);
        ##if water_cascades != one_to_four
          result.color1.xyzw = float4(gradient1, J1, 0);
          ##if (water_cascades != two)
            result.color2.xyzw = float4(gradient2, J2, 0);
            ##if (water_cascades != three)
              result.color3.xyzw = float4(gradient3, J3, 0);
              ##if (water_cascades != four)
                result.color4.xyzw = float4(gradient4, J4, 0);
              ##endif
            ##endif
          ##endif
        ##endif
        return result;
      }
    }
  } else
  {
    (ps)
    {
      foam_blur_extents0123@f4 = foam_blur_extents0123;
    }
    if (water_cascades == five)
    {
      (ps) { foam_blur_extents4567@f4 = foam_blur_extents4567; }
    }

    if (water_foam_pass == first)
    {
      if (support_texture_array == on)
      {
        (ps) { water_foam_texture0@smpArray = water_foam_texture0; }
      }
      else
      {
        (ps) {
          water_foam_texture0@smp2d = water_foam_texture0;
          water_foam_texture1@smp2d = water_foam_texture1;
        }
        if (water_cascades != two && water_cascades != one_to_four)
        {
          (ps) { water_foam_texture2@smp2d = water_foam_texture2; }
          if (water_cascades != three)
          {
            (ps) { water_foam_texture3@smp2d = water_foam_texture3; }
            if (water_cascades != four)
            {
              (ps) { water_foam_texture4@smp2d = water_foam_texture4; }
            }
          }
        }
      }

      (ps) { foam_dissipation@f3 = (foam_dissipation.x*0.25, foam_dissipation.y/0.25, foam_dissipation.z, 0); }

      hlsl(ps) {
        struct PsOut
        {
          half4 color0 : SV_Target0;
          ##if water_cascades == five
            half4 color1 : SV_Target1;
          ##endif
        };
      }
    }
    else
    {
      (ps) { water_foam_texture0@smpArray = water_foam_texture0; }
      hlsl(ps) {
        struct PsOut
        {
          half4 color0 : SV_Target0;
          ##if water_cascades != one_to_four
            half4 color1 : SV_Target1;
            ##if water_cascades != two
              half4 color2 : SV_Target2;
              ##if water_cascades != three
                half4 color3 : SV_Target3;
                ##if water_cascades != four
                  half4 color4 : SV_Target4;
                ##endif
              ##endif
            ##endif
          ##endif
        };
      }
    }

    hlsl(ps) {
      PsOut water_gradient_ps(VsOutput input)
      {
        // Sample neighbour texels

        // blur with variable size kernel is done by doing 4 bilinear samples, 
        // each sample is slightly offset from the center point
        half4 energy0123;
        half4 energy4567;
        ##if (water_foam_pass == first)
  ##if support_texture_array == on
          #define GET_ENERGY(water_foam_texture, cascade_group, attr, no)\
          {\
            float2 UVoffset = float2(0, foam_blur_extents##cascade_group.attr);\
            half foamenergy1  = tex3D(water_foam_texture0, float3(input.tc.xy + UVoffset, no)).w;\
            half foamenergy2  = tex3D(water_foam_texture0, float3(input.tc.xy - UVoffset, no)).w;\
            half foamenergy3  = tex3D(water_foam_texture0, float3(input.tc.xy + UVoffset*2.0, no)).w;\
            half foamenergy4  = tex3D(water_foam_texture0, float3(input.tc.xy - UVoffset*2.0, no)).w;\
            half folding = max(0, tex3D(water_foam_texture0, float3(input.tc.xy, no)).z);\
            energy##cascade_group.attr = foam_dissipation.x*((foamenergy1 + foamenergy2 + foamenergy3 + foamenergy4) + max(0,(1.0-folding-foam_dissipation.z))*foam_dissipation.y);\
          }
  ##else
          #define GET_ENERGY(water_foam_texture, cascade_group, attr, no)\
          {\
            float2 UVoffset = float2(0, foam_blur_extents##cascade_group.attr);\
            half foamenergy1  = tex2D(water_foam_texture##no, input.tc.xy + UVoffset).w;\
            half foamenergy2  = tex2D(water_foam_texture##no, input.tc.xy - UVoffset).w;\
            half foamenergy3  = tex2D(water_foam_texture##no, input.tc.xy + UVoffset*2.0).w;\
            half foamenergy4  = tex2D(water_foam_texture##no, input.tc.xy - UVoffset*2.0).w;\
            half folding = max(0, tex2D(water_foam_texture##no, input.tc.xy).z);\
            energy##cascade_group.attr = foam_dissipation.x*((foamenergy1 + foamenergy2 + foamenergy3 + foamenergy4) + max(0,(1.0-folding-foam_dissipation.z))*foam_dissipation.y);\
          }
  ##endif
          energy0123 = float4(0.0, 0.0, 0.0, 0.0);
          energy4567 = float4(0.0, 0.0, 0.0, 0.0);
          GET_ENERGY(water_foam_texture, 0123, x, 0);
          ##if water_cascades != one_to_four
            GET_ENERGY(water_foam_texture, 0123, y, 1);
            ##if water_cascades != two
              GET_ENERGY(water_foam_texture, 0123, z, 2);
              ##if water_cascades != three
                GET_ENERGY(water_foam_texture, 0123, w, 3);
                ##if water_cascades != four
                  GET_ENERGY(water_foam_texture, 4567, x, 4);
                ##endif
              ##endif
            ##endif
          ##endif

          PsOut result;
          result.color0.xyzw = energy0123;
          ##if water_cascades == five
            result.color1.xyzw = energy4567;
          ##endif
        ##else
          #define GET_ENERGY(water_foam_texture, slice, cascade_group, attr)\
          {\
            float2 UVoffset = float2(foam_blur_extents##cascade_group.attr, 0);\
            half foamenergy1 = tex3Dlod(water_foam_texture, float4(input.tc.xy + UVoffset, slice, 0)).attr;\
            half foamenergy2 = tex3Dlod(water_foam_texture, float4(input.tc.xy - UVoffset, slice, 0)).attr;\
            half foamenergy3 = tex3Dlod(water_foam_texture, float4(input.tc.xy + UVoffset*2.0, slice, 0)).attr;\
            half foamenergy4 = tex3Dlod(water_foam_texture, float4(input.tc.xy - UVoffset*2.0, slice, 0)).attr;\
            energy##cascade_group.attr = 0.25*(foamenergy1+foamenergy2+foamenergy3+foamenergy4);\
          }
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

          PsOut result;
          result.color0.xyzw = energy0123.x;
          ##if water_cascades != one_to_four
            result.color1.xyzw = energy0123.y;
            ##if water_cascades != two
              result.color2.xyzw = energy0123.z;
              ##if water_cascades != three
                result.color3.xyzw = energy0123.w;
                ##if water_cascades != four
                  result.color4.xyzw = energy4567.x;
                ##endif
              ##endif
            ##endif
          ##endif

        ##endif

        // Output
        return result;
      }
    }
  }

  compile("target_vs", "water_gradient_vs");
  compile("target_ps", "water_gradient_ps");
}

shader water_gradient_mip
{
  if (support_texture_array != on)
  {
    dont_render;
  }
  cull_mode=none;
  z_test=false;
  z_write=false;


  POSTFX_VS_TEXCOORD(1, tc)

  (ps) { water_gradients_tex0@smpArray = water_gradients_tex0; }

  hlsl(ps) {
    struct PsOut
    {
      half4 color0 : SV_Target0;
      ##if water_cascades != one_to_four
        half4 color1 : SV_Target1;
        ##if (water_cascades != two)
          half4 color2 : SV_Target2;
          ##if (water_cascades != three)
            half4 color3 : SV_Target3;
            ##if (water_cascades != four)
              half4 color4 : SV_Target4;
            ##endif
          ##endif
        ##endif
      ##endif
    };
    PsOut water_gradient_mip_ps(VsOutput input)
    {
      float attenuation = 0.91;
      PsOut result;
      result.color0 = tex3D(water_gradients_tex0, float3(input.tc.xy, 0));
      result.color0.a*=attenuation;
      ##if water_cascades != one_to_four
        result.color1 = tex3D(water_gradients_tex0, float3(input.tc.xy, 1));
        result.color1.a*=attenuation;
        ##if (water_cascades != two)
          result.color2 = tex3D(water_gradients_tex0, float3(input.tc.xy, 2));
          result.color2.a*=attenuation;
          ##if (water_cascades != three)
            result.color3 = tex3D(water_gradients_tex0, float3(input.tc.xy, 3));
            result.color3.a*=attenuation;
            ##if (water_cascades != four)
              result.color4 = tex3D(water_gradients_tex0, float3(input.tc.xy, 4));
              result.color4.a*=attenuation;
            ##endif
          ##endif
        ##endif
      ##endif
      return result;
    }
  }

  compile("target_ps", "water_gradient_mip_ps");
}
