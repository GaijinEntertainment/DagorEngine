
include "shader_global.sh"
include "volume_light_distant_common.sh"

texture distant_fog_reprojection_dist;
texture distant_fog_result_inscatter;

float4 distant_fog_reconstruction_resolution;


// TODO: merge this shader into DF reconstruction (use 2x2 texel per thread)
// mip generation of distant fog result and pseudo depth, for transparent stuff only
shader volume_light_distant_mip_gen_cs
{
  (cs) {
    distant_fog_reprojection_dist@smp2d = distant_fog_reprojection_dist;
    distant_fog_result_inscatter@smp2d = distant_fog_result_inscatter;
    mip_gen_output_tex_size@f2 = (distant_fog_reconstruction_resolution.x*0.5, distant_fog_reconstruction_resolution.y*0.5, 0, 0);
    mip_gen_tr@f4 = (
      2*distant_fog_reconstruction_resolution.z,
      2*distant_fog_reconstruction_resolution.w,
      distant_fog_reconstruction_resolution.z,
      distant_fog_reconstruction_resolution.w);
  }

  hlsl(cs) {
    RWTexture2D<float4>  OutputInscatterMip : register(u7);
    RWTexture2D<float>  OutputReprojectionDistMip : register(u6);


    half4 sample_inscatter(float2 texcoord)
    {
      return (half4)tex2Dlod(distant_fog_result_inscatter, float4(texcoord, 0, 0));
    }
    half4 load_inscatter(int2 pos)
    {
      return distant_fog_result_inscatter[pos];
    }

    half4 volfog_calc_z_weights(float4 depthDiff)
    {
      return (half4)rcp( 0.00001 + depthDiff);
    }

    half4 calc_inscatter_bilateral(float2 texcoord, float maxDist, float4 reprojDist)
    {
      half4 bilinearResult = sample_inscatter(texcoord);

      BRANCH
      if (all(abs(bilinearResult - half4(0.h,0.h,0.h,1.h)) < 0.0001h))
        return half4(0.h,0.h,0.h,1.h);

      float2 coordsCorner = texcoord*(int2)mip_gen_output_tex_size.xy - 0.5;
      int4 coordsI;
      coordsI.xy = (int2)clamp(floor(coordsCorner), 0, (int2)mip_gen_output_tex_size.xy - 1);
      coordsI.zw = coordsI.xy + 1;

      float4 depthDiff = maxDist - reprojDist;
      float maxDiff = max4(depthDiff.x, depthDiff.y, depthDiff.z, depthDiff.w);

      bool bTinyDiff = maxDiff < maxDist*0.05;
      BRANCH
      if (bTinyDiff)
        return bilinearResult;

      half4 linearLowResWeights = volfog_calc_z_weights(depthDiff);
      linearLowResWeights *= linearLowResWeights; // this sharpens the best candidate in case there is a big diff in depths
      linearLowResWeights /= dot(linearLowResWeights, 1);

      half4 result;
      result.r = dot(distant_fog_result_inscatter.GatherRed(distant_fog_result_inscatter_samplerstate, texcoord), linearLowResWeights);
      result.g = dot(distant_fog_result_inscatter.GatherGreen(distant_fog_result_inscatter_samplerstate, texcoord), linearLowResWeights);
      result.b = dot(distant_fog_result_inscatter.GatherBlue(distant_fog_result_inscatter_samplerstate, texcoord), linearLowResWeights);
      result.a = dot(distant_fog_result_inscatter.GatherAlpha(distant_fog_result_inscatter_samplerstate, texcoord), linearLowResWeights);
      return result;
    }

    [numthreads( RECONSTRUCT_WARP_SIZE, RECONSTRUCT_WARP_SIZE, 1 )]
    void MipGen( uint2 dtId : SV_DispatchThreadID, uint2 tid : SV_GroupThreadID )
    {
      if (any(dtId >= (uint2)mip_gen_output_tex_size.xy))
        return;

      float2 tc = float2(dtId) * mip_gen_tr.xy + mip_gen_tr.zw;

      float4 reprojDist = distant_fog_reprojection_dist.GatherRed(distant_fog_reprojection_dist_samplerstate, tc);
      float maxDist = max4(reprojDist.x, reprojDist.y, reprojDist.z, reprojDist.w);

      OutputInscatterMip[dtId] = calc_inscatter_bilateral(tc, maxDist, reprojDist);
      OutputReprojectionDistMip[dtId] = maxDist;
    }
  }
  compile("target_cs", "MipGen");
}
