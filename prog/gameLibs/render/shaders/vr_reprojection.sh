int use_vr_reprojection = 0;
interval use_vr_reprojection:off<1, on;
int use_bounding_vr_reprojection = 0;
interval use_bounding_vr_reprojection:off<1, on;

float4 vr_reprojection_matrix0 = (1,0,0,0);
float4 vr_reprojection_matrix1 = (0,1,0,0);
float4 vr_reprojection_matrix2 = (0,0,1,0);
float4 vr_reprojection_matrix3 = (0,0,0,1);

float4 vr_bounding_view_reprojection_matrix0 = (1,0,0,0);
float4 vr_bounding_view_reprojection_matrix1 = (0,1,0,0);
float4 vr_bounding_view_reprojection_matrix2 = (0,0,1,0);
float4 vr_bounding_view_reprojection_matrix3 = (0,0,0,1);

macro INIT_VR_REPROJECTION(stage)
  (stage) {
    vrReprojectionMatrix@f44 = {vr_reprojection_matrix0, vr_reprojection_matrix1, vr_reprojection_matrix2, vr_reprojection_matrix3};
    use_vr_reprojection@i1 = use_vr_reprojection;
  }
endmacro

macro USE_VR_REPROJECTION(stage)
  hlsl(stage) {
    float4 vr_reproject(float4 pos)
    {
      ##if use_vr_reprojection == on
        float4 reprojectedTexcoord = mul(pos, vrReprojectionMatrix);
        return reprojectedTexcoord;
      ##else
        return pos;
      ##endif
    }

    float2 vr_reproject(float2 texcoord, float rawDepth)
    {
      ##if use_vr_reprojection == on
        float4 pos = float4((texcoord - 0.5) * float2(2.0, -2.0), rawDepth, 1.0);
        pos = vr_reproject(pos);
        pos.xy /= pos.w;
        return pos.xy * float2(0.5, -0.5) + 0.5;
      ##else
        return texcoord;
      ##endif
    }
  }
endmacro

macro INIT_BOUNDING_VIEW_REPROJECTION(stage)
  (stage) {
    vrBoundingViewReprojectionMatrix@f44 = {vr_bounding_view_reprojection_matrix0,
                                            vr_bounding_view_reprojection_matrix1,
                                            vr_bounding_view_reprojection_matrix2,
                                            vr_bounding_view_reprojection_matrix3};
    use_bounding_vr_reprojection@i1 = use_bounding_vr_reprojection;
  }
endmacro

macro USE_BOUNDING_VIEW_REPROJECTION(stage)
  hlsl(stage) {
    float3 vr_bounding_view_reproject(float2 texcoord, float rawDepth)
    {
      float4 pos = float4(texcoord * float2(2.0, -2.0) + float2(-1, 1), rawDepth, 1.0);
      float4 reprojectedClipPos = mul(pos, vrBoundingViewReprojectionMatrix);
      reprojectedClipPos /= reprojectedClipPos.w;
      return float3(reprojectedClipPos.xy * float2(0.5, -0.5) + 0.5, reprojectedClipPos.z);
    }
    float2 vr_bounding_view_reproject_tc(float2 texcoord, float rawDepth)
    {
      return vr_bounding_view_reproject(texcoord, rawDepth).xy;
    }
  }
endmacro