include "shader_global.sh"
include "gbuffer.sh"
include "reprojected_motion_vectors.sh"
include "viewVecVS.sh"
include "dof/dof_composite.sh"

//include "dacloud_mask.sh"

float4 prevViewProjTm0;
float4 prevViewProjTm1;
float4 prevViewProjTm2;
float4 prevViewProjTm3;

texture dof_frame_tex;
texture dof_downsampled_frame_tex;
texture dof_downsampled_close_depth_tex;
texture dof_coc_history;

int simplified_dof = 0;
interval simplified_dof: off<1, on;

hlsl(ps) {
  #define COMBINE_LAYERS 0
}

macro POSTFX()
  cull_mode=none;
  z_write=false;
  z_test=false;

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      noperspective float2 tc : TEXCOORD0;
    };
  }

  USE_POSTFX_VERTEX_POSITIONS()
  hlsl(vs) {
    VsOutput postfx_vs(uint vertexId : SV_VertexID)
    {
      VsOutput output;
      //float2 inpos = input.pos.xy;
      float2 inpos = getPostfxVertexPositionById(vertexId);

      output.pos = float4(inpos,0,1);
      output.tc = screen_to_texcoords(inpos);
      return output;
    }
  }
  compile("target_vs", "postfx_vs");
endmacro

macro NEAR_FAR_DEFINES()
  if (dof_far_layer != NULL)
  {
    hlsl(ps) {
      #define HAS_FAR_DOF 1
    }
    if (dof_near_layer != NULL)
    {
      hlsl(ps) {
        #define HAS_NEAR_DOF 1
      }
    }
  } else
  {
    hlsl(ps) {
      #define HAS_NEAR_DOF 1
    }
  }
endmacro

shader dof_downsample
{
  ENABLE_ASSERT(ps)
  POSTFX()

  INIT_ZNZFAR()
  VIEW_VEC_OPTIMIZED(ps)
  INIT_REPROJECTED_MOTION_VECTORS(ps)
  USE_REPROJECTED_MOTION_VECTORS(ps)

  INIT_READ_DEPTH_GBUFFER()

  if (dof_downsampled_frame_tex != NULL)
  {
    (ps) { downsampled_frame@smp2d = dof_downsampled_frame_tex; }
  }
  else
  {
    (ps) { frame_tex@smp2d = dof_frame_tex; }
  }
  (ps) {
    dof_focus_params@f4 = dof_focus_params;
    prevViewProjTm@f44 = { prevViewProjTm0, prevViewProjTm1, prevViewProjTm2, prevViewProjTm3 };
    downsampled_close_depth_tex@smp2d = dof_downsampled_close_depth_tex;
    downsampled_far_depth_tex@smp2d = downsampled_far_depth_tex;
    dof_coc_history@smp2d = dof_coc_history;
  }
  NEAR_FAR_DEFINES()

  hlsl(ps) {
    #include <dof/circleOfConfusion.hlsl>

    struct OutDownscaleDof
    {
      #if HAS_NEAR_DOF
        float4 LayerNear : SV_Target0;
        float CocNear : SV_Target1;
        #if COMPLICATED_DOF
        #if HAS_FAR_DOF && !COMBINE_LAYERS
          float4 LayerFar : SV_Target2;
          float CocFar : SV_Target3;
        #endif
        #endif
      #else
        float4 LayerFar : SV_Target0;
        float CocFar : SV_Target1;
      #endif
    };

    float2 getCameraMotion(float rawDepth, float2 uv)
    {
      float linearDepth = linearize_z(rawDepth, zn_zfar.zw);
      float3 viewVec = getViewVecOptimized(uv);
      float3 cameraToPoint = viewVec * linearDepth;
      return get_reprojected_motion_vector(uv, cameraToPoint, prevViewProjTm);
    }

    OutDownscaleDof dof_downsample_ps(VsOutput IN HW_USE_SCREEN_POS)
    {
      float4 pos = GET_SCREEN_POS(IN.pos);
      OutDownscaleDof OUT = (OutDownscaleDof) 0;

      // downsampled frame affects area 4x4 of original frame because of interpolation
      // at the same time downsampled_close_depth_tex affects only area 2x2 (makes direct texelFetch)
      // so to approximate depth which corresponds to our downsampled frame we need sampling downsampled_close_depth_tex from area 3x3
      ##if ((dof_downsampled_close_depth_tex != NULL) && (downsampled_far_depth_tex != NULL))
        float depth, farDepth, nearDepth;

        //sampling far_depth
        //farDepth is calculated from area 2x2 for each mip level
        //so to get real min within the surrounding area we need to sample from left up quarter
        //todo: check if GatherRed op can make it faster
        float2 far_depth_tc = IN.tc.xy;
        ##if simplified_dof == on
        float4 depths = downsampled_far_depth_tex.GatherRed(downsampled_far_depth_tex_samplerstate, far_depth_tc, int2(0, 0));
        farDepth = min4(depths.x, depths.y, depths.z, depths.w);
        ##else
        float4 depths = float4(
            downsampled_far_depth_tex.SampleLevel(downsampled_far_depth_tex_samplerstate, far_depth_tc, 0, int2(-1,-1)).x,
            downsampled_far_depth_tex.SampleLevel(downsampled_far_depth_tex_samplerstate, far_depth_tc, 0, int2(+1,-1)).x,
            downsampled_far_depth_tex.SampleLevel(downsampled_far_depth_tex_samplerstate, far_depth_tc, 0, int2(-1,+1)).x,
            downsampled_far_depth_tex.SampleLevel(downsampled_far_depth_tex_samplerstate, far_depth_tc, 0, int2(+1,+1)).x
          );
        farDepth = min4(depths.x, depths.y, depths.z, depths.w);
        depths = float4(
            downsampled_far_depth_tex.SampleLevel(downsampled_far_depth_tex_samplerstate, far_depth_tc, 0, int2(-1,0)).x,
            downsampled_far_depth_tex.SampleLevel(downsampled_far_depth_tex_samplerstate, far_depth_tc, 0, int2(+1,0)).x,
            downsampled_far_depth_tex.SampleLevel(downsampled_far_depth_tex_samplerstate, far_depth_tc, 0, int2(0,-1)).x,
            downsampled_far_depth_tex.SampleLevel(downsampled_far_depth_tex_samplerstate, far_depth_tc, 0, int2(0,+1)).x
          );
        farDepth = min(farDepth, min4(depths.x, depths.y, depths.z, depths.w));
        farDepth = min(farDepth, downsampled_far_depth_tex.SampleLevel(downsampled_far_depth_tex_samplerstate, far_depth_tc, 0, int2(0,0)).x);
        ##endif
        //center sample of close_depth is considered as depth to let thin objects be capable to belong to near layer
        depth = texelFetch(downsampled_close_depth_tex, int2(pos.xy), 0).x;
        ##if simplified_dof == on
        depths = downsampled_close_depth_tex.GatherRed(downsampled_close_depth_tex_samplerstate, far_depth_tc, int2(0, 0));
        nearDepth = min4(depths.x, depths.y, depths.z, depths.w);
        ##else
        //sampling close_depth
        depths = float4(
          texelFetchOffset(downsampled_close_depth_tex, int2(pos.xy), 0, int2(-1, -1)).x,
          texelFetchOffset(downsampled_close_depth_tex, int2(pos.xy), 0, int2( 0, -1)).x,
          texelFetchOffset(downsampled_close_depth_tex, int2(pos.xy), 0, int2(+1, -1)).x,
          texelFetchOffset(downsampled_close_depth_tex, int2(pos.xy), 0, int2(+1,  0)).x
        );
        nearDepth = max4(depths.x, depths.y, depths.z, depths.w);
        float nearDepthMin = min4(depths.x, depths.y, depths.z, depths.w);
        depths = float4(
          texelFetchOffset(downsampled_close_depth_tex, int2(pos.xy), 0, int2(-1,  0)).x,
          texelFetchOffset(downsampled_close_depth_tex, int2(pos.xy), 0, int2(-1, +1)).x,
          texelFetchOffset(downsampled_close_depth_tex, int2(pos.xy), 0, int2( 0, +1)).x,
          texelFetchOffset(downsampled_close_depth_tex, int2(pos.xy), 0, int2(+1, +1)).x
        );
        nearDepth = max(nearDepth, max4(depths.x, depths.y, depths.z, depths.w));
        nearDepthMin = min(nearDepthMin, min4(depths.x, depths.y, depths.z, depths.w));
        nearDepth = max(depth, nearDepth);
        nearDepthMin = min(depth, nearDepthMin);
        ##endif

        //minCocFar becomes zero on edges to prevent color bleeding
        //maxCocFar is used to allow gathering on edges to prevent unblurred outline
        float cocNear, maxCocFar, minCocFar;
        cocNear = ComputeNearCircleOfConfusion(depth, dof_focus_params);
        maxCocFar = ComputeFarCircleOfConfusion(farDepth, dof_focus_params); //for center pixel
        minCocFar = ComputeFarCircleOfConfusion(nearDepth, dof_focus_params); //for gathered pixels

        ##if dof_coc_history != NULL && simplified_dof == off
          float2 tc = IN.tc.xy + getCameraMotion(depth, IN.tc.xy);
          float cocNearHistory = dof_coc_history.SampleLevel(dof_coc_history_samplerstate, tc, 0).x;

          float cocNearMin = ComputeNearCircleOfConfusion(nearDepthMin, dof_focus_params);
          float cocNearMax = ComputeNearCircleOfConfusion(nearDepth, dof_focus_params);

          cocNearHistory = clamp(cocNearHistory, cocNearMin * 0.75, cocNearMax * 1.25);
          cocNear = lerp(saturate(cocNear), cocNearHistory, 0.9);
        ##endif

      ##else  // FIXME : usage of fullsize depthmap requires usage of 16 samples
              // downsampled frame affects area 4x4 of original frame so we need to sample depth from area 4x4 too
        float2 depthTc = IN.tc.xy+0.0001;
        float4 depths = float4(
            depth_gbuf_read.SampleLevel(depth_gbuf_read_samplerstate, depthTc, 0, int2(-1,-1)).x,
            depth_gbuf_read.SampleLevel(depth_gbuf_read_samplerstate, depthTc, 0, int2(+1,-1)).x,
            depth_gbuf_read.SampleLevel(depth_gbuf_read_samplerstate, depthTc, 0, int2(-1,+1)).x,
            depth_gbuf_read.SampleLevel(depth_gbuf_read_samplerstate, depthTc, 0, int2(+1,+1)).x
          );
        float4 vCocNear4, vCocFar4;
        Compute4CircleOfConfusion(depths, vCocNear4, vCocFar4, dof_focus_params);
        float cocNear = max(max(vCocNear4.x, vCocNear4.y), max(vCocNear4.z, vCocNear4.w));
        float maxCocFar = max(max(vCocFar4.x, vCocFar4.y), max(vCocFar4.z, vCocFar4.w));
        float minCocFar = min(min(vCocFar4.x, vCocFar4.y), min(vCocFar4.z, vCocFar4.w));
        float depth = dot(depths, 0.25);
      ##endif


      ##if (dof_downsampled_frame_tex != NULL)
        float3 color = downsampled_frame[int2(pos.xy)].xyz;
      ##else
        float3 c00 = tex2Dlod(frame_tex, float4(IN.tc.xy,0,0)).rgb;
        float3 c0 = frame_tex.SampleLevel(frame_tex_samplerstate, IN.tc.xy, 0, int2(-1,-1)).rgb;
        float3 c1 = frame_tex.SampleLevel(frame_tex_samplerstate, IN.tc.xy, 0, int2( 1,-1)).rgb;
        float3 c2 = frame_tex.SampleLevel(frame_tex_samplerstate, IN.tc.xy, 0, int2(-1, 1)).rgb;
        float3 c3 = frame_tex.SampleLevel(frame_tex_samplerstate, IN.tc.xy, 0, int2( 1, 1)).rgb;
        float3 color = (c00 + c0 + c1 + c2 + c3) * 0.2;
      ##endif

      #if COMPLICATED_DOF
        #if HAS_NEAR_DOF
          OUT.LayerNear.a = cocNear;
          OUT.LayerNear.rgb = color * cocNear;
          OUT.CocNear = cocNear;
        #endif

        // Clamp for proper masking later (avoids "super strong" edges on very blurry foreground objects).
        //OUT.LayerNear.a = saturate(OUT.LayerNear.a);
        #if HAS_FAR_DOF
          float4 LayerFar = float4(color, minCocFar);
          //a trick to prevent color bleeding to background due to interpolation in gathering step
          LayerFar.a = (minCocFar > 1e-4) ? minCocFar*0.25 + 0.75 : 0;
          #if COMBINE_LAYERS
            if (cocNear<1e-6)
              OUT.LayerNear = float4(LayerFar.rgb, -LayerFar.a);
          #else
            OUT.LayerFar = LayerFar;
            OUT.CocFar = maxCocFar;
          #endif
        #endif
      #else
        float coc = cocNear>0 ? -cocNear : cocFar;
        OUT.LayerNear.rgb = color*smoothstep(0, 1./510, abs(coc));
        OUT.LayerNear.a = coc;
        OUT.CocNear = ceilf(cocNear*255.0)/255.0;
      #endif

      return OUT;
    }

  }

  compile("target_ps", "dof_downsample_ps");
}

shader dof_tile
{
  POSTFX()

  hlsl(ps) {
    Texture2D _tex0:register(t0);
    SamplerState _tex0_samplerstate:register(s0);

    struct OutTileMinCoC
    {
      float CocNear : SV_Target0;
    };

    OutTileMinCoC dof_tile_ps(VsOutput input)
    {
      float2 tc = input.tc;
      OutTileMinCoC OUT = (OutTileMinCoC) 0;

      //fixme: replace with fixed offsets
      float4 maxCoC;
      ##if simplified_dof == on
      maxCoC = float4(
                      _tex0.SampleLevel(_tex0_samplerstate, tc, 0, int2(-2,-2)).x,
                      _tex0.SampleLevel(_tex0_samplerstate, tc, 0, int2(-2,2)).x,
                      _tex0.SampleLevel(_tex0_samplerstate, tc, 0, int2(2,-2)).x,
                      _tex0.SampleLevel(_tex0_samplerstate, tc, 0, int2(2,2)).x
                      );
      float coc = max(max(maxCoC.x, maxCoC.y), max(maxCoC.z, maxCoC.w));
      coc = max(coc, _tex0.SampleLevel(_tex0_samplerstate, tc, 0, int2(0,0)).x);
      OUT.CocNear = coc;
      ##else
      maxCoC = _tex0.GatherRed(_tex0_samplerstate, tc, int2(-2, -2));
      maxCoC = max(maxCoC, _tex0.GatherRed(_tex0_samplerstate, tc, int2(0, -2)));
      maxCoC = max(maxCoC, _tex0.GatherRed(_tex0_samplerstate, tc, int2(2, -2)));
      maxCoC = max(maxCoC, _tex0.GatherRed(_tex0_samplerstate, tc, int2(-2, 0)));
      maxCoC = max(maxCoC, _tex0.GatherRed(_tex0_samplerstate, tc, int2(0, 0)));
      maxCoC = max(maxCoC, _tex0.GatherRed(_tex0_samplerstate, tc, int2(2, 0)));

      maxCoC = max(maxCoC, _tex0.GatherRed(_tex0_samplerstate, tc, int2(0, 2)));
      maxCoC = max(maxCoC, _tex0.GatherRed(_tex0_samplerstate, tc, int2(-2, 2)));
      maxCoC = max(maxCoC, _tex0.GatherRed(_tex0_samplerstate, tc, int2(2, 2)));
      OUT.CocNear = max(max(maxCoC.x, maxCoC.y), max(maxCoC.z, maxCoC.w));
      ##endif

      return OUT;
    }
  }

  compile("target_ps", "dof_tile_ps");
}

int dof_gather_iteration = 0;
interval dof_gather_iteration:first<1, second;

int num_dof_taps = 49;

shader dof_gather
{
  POSTFX()

  if (dof_gather_iteration == second)
  {
    (ps) { dof_coc_scale@f4 = (dof_rt_size.z*(2*0.15), dof_rt_size.w*(2*0.15), num_dof_taps, dof_rt_size.w); }
  } else
  {
    (ps) { dof_coc_scale@f4 = (dof_rt_size.z*2, dof_rt_size.w*2, num_dof_taps, dof_rt_size.w); }
  }
  NEAR_FAR_DEFINES()


  hlsl(ps) {

    Texture2D texLayerNear:register(t0);
    SamplerState texLayerNear_samplerstate:register(s0);

    Texture2D texTile:register(t2);
    SamplerState texTile_samplerstate:register(s2);
    ##if dof_gather_iteration == second
      ##if simplified_dof == on
        #define num_dof_taps 4
      ##else
        #define num_dof_taps 9
      ##endif
      float4 g_Taps[9]:register(c6);
      #if !SHADER_COMPILER_DXC
      float4 last:register(c55);
      #endif
      #define USE_LOOP [unroll]
      Texture2D texLayerFar:register(t1);
      SamplerState texLayerFar_samplerstate:register(s1);
      Texture2D texMaxCocFar:register(t3);
      SamplerState texMaxCocFar_samplerstate:register(s3);

    ##else
      #if COMBINE_LAYERS
        #define _tex2 _tex1
      #else
        Texture2D texLayerFar:register(t1);
        SamplerState texLayerFar_samplerstate:register(s1);
        Texture2D texMaxCocFar:register(t3);
        SamplerState texMaxCocFar_samplerstate:register(s3);
      #endif
      ##if simplified_dof == on
        #define num_dof_taps 16
      ##else
        #define num_dof_taps 49
      ##endif
      //#define num_dof_taps (dof_coc_scale.z)
      float4 g_Taps[7*7]:register(c6);  // Max is 7x7 (for first iteration)
      #if !SHADER_COMPILER_DXC
      float4 last:register(c55);
      #endif
      #define USE_LOOP [loop]
    ##endif


    struct pixout_dof
    {
      #if HAS_NEAR_DOF
      float4 LayerNear : SV_Target0;
      #endif
      #if COMPLICATED_DOF
      #if HAS_FAR_DOF
        float4 LayerFar : SV_Target1;
      #endif
      #endif
      //float2 CocNearFar : SV_Target2;
    };

    // Fragment shader: Bokeh filter with disk-shaped kernels
    half4 frag_Blur1(float2 uv, float _RcpAspect, float _MaxCoC)
    {
      float2 fCocScale = dof_coc_scale.xy;
      float texelSize = dof_coc_scale.w;
      half4 samp0 = tex2Dlod(texLayerNear, float4(uv,0,0));

      half4 bgAcc = 0; // Background: far field bokeh
      half4 fgAcc = 0; // Foreground: near field bokeh
      const int kSampleCount = 49;
      const float vMinTileCoC = (tex2Dlod(texTile, float4(uv.xy, 0, 0)).r);

      [branch] if (vMinTileCoC.r>0.0 || samp0.w>1e-6)
      {
        _MaxCoC = max(vMinTileCoC.r, samp0.w);
        LOOP for (int si = 0; si < kSampleCount; si++)
        {
          const float2 offset = g_Taps[si].xy;
          float2 disp = offset * _MaxCoC * fCocScale;
          float dist = length(disp);

          float2 duv = disp;
          half4 samp = tex2Dlod(texLayerNear, float4(uv + duv,0,0));

          // BG: Compare CoC of the current sample and the center sample
          // and select smaller one.
          half bgCoC = max(min(samp0.a, samp.a), 0);

          // Compare the CoC to the sample distance.
          // Add a small margin to smooth out.
          const half margin = texelSize * 2;
          half bgWeight = saturate((bgCoC   - dist + margin) / margin);
          half fgWeight = saturate((-samp.a - dist + margin) / margin);

          // Cut influence from focused areas because they're darkened by CoC
          // premultiplying. This is only needed for near field.
          fgWeight *= step(texelSize, -samp.a);

          // Accumulation
          bgAcc += half4(samp.rgb, 1) * bgWeight;
          fgAcc += half4(samp.rgb, 1) * fgWeight;
        }
      }

      // Get the weighted average.
      bgAcc.rgb /= bgAcc.a + (bgAcc.a == 0); // zero-div guard
      fgAcc.rgb /= fgAcc.a + (fgAcc.a == 0);

      // BG: Calculate the alpha value only based on the center CoC.
      // This is a rather aggressive approximation but provides stable results.
      bgAcc.a = smoothstep(texelSize, texelSize * 2, samp0.a);

      // FG: Normalize the total of the weights.
      fgAcc.a *= PI / kSampleCount;

      // Alpha premultiplying
      half3 rgb = 0;
      rgb = lerp(rgb, bgAcc.rgb, saturate(bgAcc.a));
      rgb = lerp(rgb, fgAcc.rgb, saturate(fgAcc.a));

      // Combined alpha value
      half alpha = (1 - saturate(bgAcc.a)) * (1 - saturate(fgAcc.a));

      return half4(rgb, alpha);
    }

    half4 frag_Blur2(float2 uv)
    {
      // 9-tap tent filter
      const int4 duv = int4(1,1,-1,0);
      //float4 duv = _MainTex_TexelSize.xyxy * float4(1, 1, -1, 0);
      half4 acc;

      acc  = texLayerNear.SampleLevel(texLayerNear_samplerstate, uv, 0, -duv.xy);
      acc += texLayerNear.SampleLevel(texLayerNear_samplerstate, uv, 0, -duv.wy) *2;//tex2D(_MainTex, i.uv - duv.wy) * 2;
      acc += texLayerNear.SampleLevel(texLayerNear_samplerstate, uv, 0, -duv.zy);
      acc += texLayerNear.SampleLevel(texLayerNear_samplerstate, uv, 0, +duv.zw) *2;
      acc += texLayerNear.SampleLevel(texLayerNear_samplerstate, uv, 0) *4;
      acc += texLayerNear.SampleLevel(texLayerNear_samplerstate, uv, 0, +duv.xw) *2;
      acc += texLayerNear.SampleLevel(texLayerNear_samplerstate, uv, 0, +duv.zy);
      acc += texLayerNear.SampleLevel(texLayerNear_samplerstate, uv, 0, +duv.wy) *2;
      acc += texLayerNear.SampleLevel(texLayerNear_samplerstate, uv, 0, +duv.xy);

      return acc/16.;
    }

    #if !COMPLICATED_DOF
      pixout_dof dof_gather_ps(VsOutput IN)
      {
        //discard;
        pixout_dof OUT = (pixout_dof) 0;
        ##if dof_gather_iteration == second
          OUT.LayerNear = frag_Blur2(IN.tc);
        ##else
          OUT.LayerNear = frag_Blur1(IN.tc, dof_coc_scale.x/dof_coc_scale.y, 4);
        ##endif
        return OUT;
      }
    #else
    pixout_dof dof_gather_ps(VsOutput IN)
    {
      //discard;
      pixout_dof OUT = (pixout_dof) 0;

      const int nNumTaps = num_dof_taps;
      float texelSize = dof_coc_scale.w;

      float2 fCocScale = dof_coc_scale.xy;

      #if HAS_NEAR_DOF
      float4 cCenterTapNear =tex2Dlod(texLayerNear, float4(IN.tc.xy, 0, 0));
      ##if dof_gather_iteration != second
        #if COMBINE_LAYERS
          cCenterTapNear.w = max(0, cCenterTapNear.w);
        #endif
      ##endif

      float fCocNear = cCenterTapNear.w;
      float4 cAccNear = float4(0,0,0,0);
      float caccNear = 0;

      //const float tileVal = (tex2Dlod(texTile, float4(IN.tc.xy, 0, 0)).r);
      const float4 tileVal4 = texTile.GatherRed(texTile_samplerstate, IN.tc.xy, 0);
      const float tileVal = max(max(tileVal4.x, tileVal4.y), max(tileVal4.z, tileVal4.w));
      const float vMinTileCoC = MAX_COC*tileVal;
      //const float vMinTileCoC = SCALE_NEAR;

      [branch] if (vMinTileCoC>0.0)
      {
        USE_LOOP
        for(int t = 0; t < nNumTaps; t ++)
        {
            const float2 scaledOffset = g_Taps[t].xy;
            float2 cocOffset = vMinTileCoC * scaledOffset.xy;
            float4 cn = tex2Dlod(texLayerNear, float4(IN.tc.xy + cocOffset, 0, 0));

            float wn;
            //const float k = 1.075;
            //wn = (SCALE_NEAR*cn.w >= vMinTileCoC * k) ? 1 : saturate(vMinTileCoC * k - SCALE_NEAR*cn.w);
            //float wn = (cn.w >= fCocNear) ? 1 : saturate(1+cn.w-fCocNear);
            //float wn = cn.w < fCocNear ? saturate(cn.w-fCocNear) : 1;
            //400 and 0.25 are magical constants
            wn = saturate(1+(cn.w-fCocNear)*400);//;saturate(1+(cn.w-fCocNear)*400);
            wn *= (cn.w+0.25);
            //wn *= length(g_Taps[t].xy);
            //wn = saturate((cn.a - 4 + 4) / 4);
            //wn *= step(0, cn.a);

    ##if dof_gather_iteration == second
            cAccNear = max(cAccNear, cn);
    ##else
            cAccNear += cn*wn;
            caccNear += wn;
    ##endif
        }
      }
      #endif

      #if HAS_FAR_DOF
      float4 cCenterTapFar = tex2Dlod(texLayerFar, float4(IN.tc.xy, 0, 0));
      ##if dof_gather_iteration != second
        #if COMBINE_LAYERS
          cCenterTapFar.w = max(0, -cCenterTapFar.w);
        #endif
      ##endif

      float fCocFar = texMaxCocFar.SampleLevel(texMaxCocFar_samplerstate, IN.tc.xy, 0, 0).r;
      float scaledCocFar = fCocFar*MAX_COC;
      float caccFar = 0; //for coef accumulation
      ##if dof_gather_iteration == second
        float4 cAccFar = cCenterTapFar;//float4(0,0,0,fCocFar);
        //return OUT;
      ##else
        float4 cAccFar = float4(0,0,0,0);
      ##endif

      [branch] if ((cCenterTapFar.w>1e-4) || (fCocFar>1e-4))
      {
        USE_LOOP
        for(int t = 0; t < nNumTaps; t ++)
        {
          const float2 scaledOffset = g_Taps[t].xy;
          float2 cocOffset = scaledCocFar * scaledOffset;

          float4 cf = tex2Dlod(texLayerFar, float4(IN.tc.xy + cocOffset, 0, 0));

          ##if dof_gather_iteration != second
            cf.w = saturate((cf.w-0.75)*4); //decoding coc
            #if COMBINE_LAYERS
              cf.w = max(-cf.w, 0);
            #endif
          ##endif

          float wf = (cf.w >= fCocFar) ? 1 : saturate( cf.w*MAX_COC );

          ##if dof_gather_iteration == second
            cAccFar = (cf.w < cCenterTapFar.w) ? cAccFar : max(cAccFar, cf*wf);
          ##else
            cAccFar += cf*wf;
            caccFar += wf;
          ##endif
        }
      } else
      {
        cAccFar = cCenterTapFar;
      }
      #endif

    ##if dof_gather_iteration != second
      #if HAS_NEAR_DOF
      cAccNear = caccNear ? cAccNear * rcp(caccNear) : cCenterTapNear;
      #endif
      #if HAS_FAR_DOF
      cAccFar = caccFar ? cAccFar * rcp(caccFar) : cCenterTapFar;
      cAccFar.w = fCocFar ? cAccFar.w : 0;
      #endif
    ##endif

      #if HAS_NEAR_DOF
        OUT.LayerNear = cAccNear;
      #endif
      #if HAS_FAR_DOF
        OUT.LayerFar = cAccFar;
      #endif

      return OUT;
    }
    #endif
  }

  compile("target_ps", "dof_gather_ps");
}

shader dof_composite
{
  POSTFX()

  INIT_ZNZFAR()
  if (dof_near_layer == NULL && dof_far_layer == NULL)
  {
    dont_render;
  }

  USE_DOF_COMPOSITE_NEAR_FAR_OPTIONAL()

  // (ps) { frame_tex@smp2d = frame_tex; }
  blend_src = 1; blend_dst = sa;
  (ps) { dof_rt_size@f4 = dof_rt_size; }

  hlsl(ps) {
    float4 composite_dof_ps(VsOutput input):SV_Target0
    {
      #if HAS_FAR_DOF
      half4 cLayerFar = get_dof_far(input.tc.xy, dof_rt_size.xy);
      #else
      half4 cLayerFar = 0;
      #endif
      #if HAS_NEAR_DOF
      half4 cLayerNear = get_dof_near(input.tc.xy, dof_rt_size.xy);
      #else
      half4 cLayerNear = 0;
      #endif
      //return float4(cLayerNear.rgb,0.);
      //return float4(0,0,0,1);
      //return float4(cLayerNear.aaa,0.15);

      //half4 cScene = tex2D(frame_tex, IN.baseTC.xy);
      //float4 result = lerp(cScene, cLayerFar, cLayerFar.a);
      //result = lerp(result, cLayerNear, cLayerNear.a);
      //return result;
      //->
      //result = cScene*(1-cLayerFar.a) + cLayerFar*cLayerFar.a;
      //result = result*(1-cLayerNear.a) + cLayerNear*cLayerNear.a;

      //result = (cScene*(1-cLayerFar.a) + cLayerFar*cLayerFar.a)*(1-cLayerNear.a) + cLayerNear*cLayerNear.a;
      //result = cScene*(1-cLayerFar.a)*(1-cLayerNear.a) + lerp(cLayerFar*cLayerFar.a, cLayerNear, cLayerNear.a);
      return float4(lerp(cLayerFar.rgb*cLayerFar.a, cLayerNear.rgb, cLayerNear.a), (1-cLayerFar.a)*(1-cLayerNear.a));//blend_src = 1; blend_dst = isa;
    }
  }

  compile("target_ps", "composite_dof_ps");
}

texture downsampled_depth_with_transparency;
texture downsampled_transparent_frame;

texture downsampled_close_depth_tex;

shader downsample_depth_for_far_dof
{
  cull_mode = none;
  no_ablend;
  z_test = false;
  z_write = false;

  (ps)
  {
    downsampled_depth_with_transparency@smp2d = downsampled_depth_with_transparency;
  }
  ENABLE_ASSERT(ps)
  POSTFX_VS(1)
  hlsl(ps)
  {
    void depth_ps(float4 screenpos : VPOS, out float farDepth : SV_Target0, out float closeDepth : SV_Target1)
    {
      int2 tc = int2(floor(screenpos.xy))*2;

      int2 scr = int2(floor(screenpos.xy))*2;
      float src0 = texelFetch(downsampled_depth_with_transparency, tc, 0).x;
      float src1 = texelFetchOffset(downsampled_depth_with_transparency, tc, 0, int2(1,0)).x;
      float src2 = texelFetchOffset(downsampled_depth_with_transparency, tc, 0, int2(0,1)).x;
      float src3 = texelFetchOffset(downsampled_depth_with_transparency, tc, 0, int2(1,1)).x;

      float transparentFar = min(min(src0, src1), min(src2, src3));
      float transparentClose = max(max(src0, src1), max(src2, src3));

      farDepth = transparentFar;
      closeDepth = transparentClose;
    }
  }

  compile("target_ps", "depth_ps");
}