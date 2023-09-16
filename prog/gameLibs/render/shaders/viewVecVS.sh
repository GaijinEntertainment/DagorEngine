float4 view_vecLT;
float4 view_vecRT;
float4 view_vecLB;
float4 view_vecRB;

macro INIT_VIEW_VEC_STAGE(code)
  (code) {
  view_vecLT@f3=view_vecLT;
  view_vecRT@f3=view_vecRT;
  view_vecLB@f3=view_vecLB;
  view_vecRB@f3=view_vecRB;
  }
endmacro

macro USE_VIEW_VEC_STAGE(code)
  hlsl(code) {
    float3 restore_view_vec( float2 pos )
    {
      return
        (pos.y > 0 ?
          ( pos.x < 0 ? view_vecLT : view_vecRT ) :
          ( pos.x < 0 ? view_vecLB : view_vecRB )).xyz;
    }

    float3 get_view_vec_by_vertex_id(uint vertex_id)
    {
      float3 vect = (vertex_id == 0 ? view_vecLT : (vertex_id == 1 ? view_vecLB : view_vecRT)).xyz;
      return 2 * vect - view_vecLT.xyz;
    }

    float3 lerp_view_vec(float2 tc)
    {
      return lerp(lerp(view_vecLT, view_vecRT, tc.x), lerp(view_vecLB, view_vecRB, tc.x), tc.y).xyz;
    }
  }
endmacro

macro VIEW_VEC_OPTIMIZED(code)
  (code) {
    view_vecLT@f3 = view_vecLT;
    view_vecRT_minus_view_vecLT@f3 = (view_vecRT-view_vecLT);
    view_vecLB_minus_view_vecLT@f3 = (view_vecLB-view_vecLT);
  }
  hlsl(code) {
    float3 getViewVecOptimized(float2 tc) {return view_vecLT + view_vecRT_minus_view_vecLT*tc.x + view_vecLB_minus_view_vecLT*tc.y;}
  }
endmacro

macro USE_VIEW_VEC_VS()
  USE_VIEW_VEC_STAGE(vs)
endmacro

macro USE_AND_INIT_VIEW_VEC(code)
  INIT_VIEW_VEC_STAGE(code)
  USE_VIEW_VEC_STAGE(code)
endmacro

macro USE_AND_INIT_VIEW_VEC_VS()
  USE_AND_INIT_VIEW_VEC(vs)
endmacro

macro USE_AND_INIT_VIEW_VEC_CS()
  USE_AND_INIT_VIEW_VEC(cs)
endmacro

macro USE_AND_INIT_VIEW_VEC_PS()
  USE_AND_INIT_VIEW_VEC(ps)
endmacro
