include "separate_depth_mips.sh"
include "globtm.sh"

int downsampled_depth_mip_count;

macro BASE_GPU_OCCLUSION(code)
  if (separate_depth_mips == no) {
    (code) {
      hzb_tex@smp2d = downsampled_far_depth_tex;
      downsampled_depth_mip_count@f1 = (downsampled_depth_mip_count);
    }
  } else {
    INIT_SEPARATE_DEPTH_MIPS(code)
    USE_SEPARATE_DEPTH_MIPS(code)
  }
  hlsl(code)
  {
    #ifndef optional_hyperbolize_downsampled_depth
      #define optional_hyperbolize_downsampled_depth(a, znzfar) (a)
    #endif
    #define OCCLUSION_RECT_SZ 4

    ##if separate_depth_mips == no
      #define OCCLUSION_SAMPLE_DEPTH(location, level) hzb_tex.SampleLevel(hzb_tex_samplerstate, location, level).x
    ##else
      #define OCCLUSION_SAMPLE_DEPTH(location, level) sample_depth_separate(level, location)
    ##endif

    #if OCCLUSION_RECT_SZ == 4
      #define OCCLUSION_SAMPLE_RECT(fun) \
      {\
        float2 wd = (sbox.zw-sbox.xy)*1./3.;\
        UNROLL for ( uint i = 0; i < 4; i++ ) \
        { \
          float4 rawDepth4;\
          rawDepth4.x = optional_hyperbolize_downsampled_depth(OCCLUSION_SAMPLE_DEPTH(float2(i, 0) * wd + sbox.xy, level), zn_zfar);\
          rawDepth4.y = optional_hyperbolize_downsampled_depth(OCCLUSION_SAMPLE_DEPTH(float2(i, 1) * wd + sbox.xy, level), zn_zfar);\
          rawDepth4.z = optional_hyperbolize_downsampled_depth(OCCLUSION_SAMPLE_DEPTH(float2(i, 2) * wd + sbox.xy, level), zn_zfar);\
          rawDepth4.w = optional_hyperbolize_downsampled_depth(OCCLUSION_SAMPLE_DEPTH(float2(i, 3) * wd + sbox.xy, level), zn_zfar);\
          rawDepth = fun( rawDepth, rawDepth4 );\
        }\
      }
    #else
      #define OCCLUSION_SAMPLE_RECT(fun) \
        rawDepth.x = optional_hyperbolize_downsampled_depth(OCCLUSION_SAMPLE_DEPTH(sbox.xy, level), zn_zfar);\
        rawDepth.y = optional_hyperbolize_downsampled_depth(OCCLUSION_SAMPLE_DEPTH(sbox.xw, level), zn_zfar);\
        rawDepth.z = optional_hyperbolize_downsampled_depth(OCCLUSION_SAMPLE_DEPTH(sbox.zy, level), zn_zfar);\
        rawDepth.w = optional_hyperbolize_downsampled_depth(OCCLUSION_SAMPLE_DEPTH(sbox.zw, level), zn_zfar);
    #endif

    //sbox.xyXY in screen TC
    //return closeset raw depth
    float check_box_occl_visible_tc_base(float4 sbox, out float lod)
    {
      uint2 dim;
    ##if separate_depth_mips == no
      hzb_tex.GetDimensions(dim.x, dim.y);
    ##else
      depth_mip_0.GetDimensions(dim.x, dim.y);
    ##endif
      float4 sbox_vp = sbox*dim.xyxy;

      float2 sboxSize = ( sbox_vp.zw - sbox_vp.xy );
      #if OCCLUSION_RECT_SZ == 4
        sboxSize *= 0.5;// 0.5 for 4x4
      #endif
      float level = ceil( log2( max(1, max( sboxSize.x, sboxSize.y )) ) );

      {
        float  level_lower = max(level - 1, 0);
        float4 lower_sbox_vp = sbox_vp*exp2(-level_lower);
        float2 dims = ceil(lower_sbox_vp.zw) - floor(lower_sbox_vp.xy);
        // Use the lower level if we only touch <= 2 texels in both dimensions
        if (dims.x <= OCCLUSION_RECT_SZ && dims.y <= OCCLUSION_RECT_SZ)
          level = level_lower;
      }
      lod = level;


      //minTc = (floor(sbox_vp.xy/exp2(level))+0.5)/(dim/exp2(level));

    ##if separate_depth_mips == no
      // We use external constant instead of a value from GetDimensions call, because it seems, that
      // usage of mip levels count from GetDimension is a reason of GPU hung on intel 630 HD
      const float MAX_LOD = downsampled_depth_mip_count;
    ##elif separate_depth_mips == two
      const float MAX_LOD = 2;
    ##else
      const float MAX_LOD = 3;
    ##endif

      if (level >= MAX_LOD)//we don't have all mips, and we are too close anyway
        return 0;
      //float screenRadius = max(saturate(maxTc.x) - minTc.x, saturate(maxTc.y) - minTc.y);
      //float lod = ceil(max(0, 6 + log2(screenRadius)));
      //mipIdim = rcp(floor(dim)/exp2(level));
      float4 rawDepth = 1;
      OCCLUSION_SAMPLE_RECT(min);
      return min(min(rawDepth.x, rawDepth.y), min(rawDepth.z, rawDepth.w));
    }
  }
endmacro

macro GPU_OCCLUSION(code)
  INIT_AND_USE_GLOBTM(code)
  BASE_GPU_OCCLUSION(code)
  hlsl(code) {
    #define UNOCCLUDED 2
    #define VISIBLE 1
    #define OCCLUDED 0
    uint check_point_occl_visible(float3 checkPos)
    {
      float4 screenPos = mul(float4(checkPos, 1), globtm);
      if (screenPos.w<0.01)
        return UNOCCLUDED;
      screenPos.xyz/=screenPos.w;
      float2 tc = screenPos.xy*float2(0.5,-0.5) + 0.5;

      //todo: check depth if in screen, otherwise, also raycast voxels (but more sparse)
      if (any(abs(tc*2-1) > 1.0))
        return UNOCCLUDED;
    ##if separate_depth_mips == no
      float rawDepth = optional_hyperbolize_downsampled_depth(tex2Dlod(hzb_tex, float4(tc,0,0)).x, zn_zfar);
    ##else
      float rawDepth = optional_hyperbolize_downsampled_depth(tex2Dlod(depth_mip_0, float4(tc,0,0)).x, zn_zfar);
    ##endif
      if (rawDepth>=screenPos.z)
        return OCCLUDED;
      return VISIBLE;
    }

    bool check_box_occl_visible_base(float3 minb, float3 maxb, out float4 sbox, out float lod, out float2 minMaxRawDepth)
    {
      //todo: we can speed it up
      float4 screenPos[8];
      #define CALC_CORNER(i) screenPos[i] = mul(float4(float3((i&1) ? maxb.x : minb.x, (i&2) ? maxb.y : minb.y, (i&4) ? maxb.z : minb.z), 1), globtm)
      CALC_CORNER(0);
      CALC_CORNER(1);
      CALC_CORNER(2);
      CALC_CORNER(3);
      CALC_CORNER(4);
      CALC_CORNER(5);
      CALC_CORNER(6);
      CALC_CORNER(7);
      #define fun(i) screenPos[i].w
      #define find_min_max(minF) minF(minF(minF(fun(0), fun(1)), minF(fun(2), fun(3))), minF(minF(fun(4), fun(5)), minF(fun(6), fun(7))))

      float minScreenW = find_min_max(min);
      if (minScreenW<0.1)
        return true;

      #undef fun
      #define fun(i) screenPos[i].xyz/screenPos[i].w

      float3 minScreen = find_min_max(min), maxScreen = find_min_max(max);
      float4 minMaxTc = float4(minScreen.xy, maxScreen.xy)*float4(0.5,-0.5, 0.5, -0.5) + 0.5;

      if (min(minMaxTc.x, minMaxTc.w) > 1 || max(minMaxTc.z, minMaxTc.y) < 0)
        return false;
      minMaxRawDepth.x = minScreen.z;
      minMaxRawDepth.y = maxScreen.z;
      sbox = saturate(minMaxTc.xwzy);

      if (check_box_occl_visible_tc_base(sbox, lod) > maxScreen.z)
        return false;
      return true;
    }
    bool check_box_occl_visible(float3 minb, float3 maxb)
    {
      float4 sbox; float lod; float2 minMaxRawDepth;
      minMaxRawDepth = 0;
      return check_box_occl_visible_base(minb, maxb, sbox, lod, minMaxRawDepth);
    }
  }
endmacro
