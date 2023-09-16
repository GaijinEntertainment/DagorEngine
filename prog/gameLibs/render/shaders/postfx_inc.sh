macro SCREEN_TO_TC()
hlsl(vs) {
  float2 screen_to_texcoords(float2 pos)
  {
    return pos * RT_SCALE_HALF + 0.5;
  }
}
endmacro

macro DECL_POSTFX_TC_VS_DEP()
  DECL_POSTFX_TC_VS_SCR()
endmacro

macro USE_POSTFX_VERTEX_POSITIONS()

DECL_POSTFX_TC_VS_DEP()
SCREEN_TO_TC()

hlsl(vs) {
  float2 getPostfxVertexPositionById(uint id)
  {
    return get_fullscreen_inpos(id);
  }
}

endmacro

hlsl(vs) {
  float2 get_fullscreen_inpos(uint vertexId)
  {
    return float2((vertexId == 2) ? +3.0 : -1.0, (vertexId == 1) ? -3.0 : 1.0);
  }
}
macro POSTFX_VS(pos_z)

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      #ifdef USE_TEXCOORD
        noperspective float2 USE_TEXCOORD : TEXCOORD0;
      #endif
      #ifdef USE_VIEW_VEC
        noperspective float3 USE_VIEW_VEC : TEXCOORD1;
      #endif
    };
  }

  USE_POSTFX_VERTEX_POSITIONS()

  hlsl(vs) {
    VsOutput postfx_vs(uint vertexId : SV_VertexID)
    {
      VsOutput output;
      float2 inpos = getPostfxVertexPositionById(vertexId);

      output.pos = float4(inpos, pos_z, 1);

      #ifdef USE_TEXCOORD
        output.USE_TEXCOORD = screen_to_texcoords(inpos);
      #endif

      #ifdef USE_VIEW_VEC
        output.USE_VIEW_VEC = get_view_vec_by_vertex_id(vertexId);
      #endif
      return output;
    }
  }
  compile("target_vs", "postfx_vs");
endmacro

macro POSTFX_VS_TEXCOORD(pos_z, texcoord_name)
  hlsl {
    #define USE_TEXCOORD texcoord_name
  }
  POSTFX_VS(pos_z)
  hlsl {
    #undef USE_TEXCOORD
  }
endmacro

macro POSTFX_VS_VIEWVEC(pos_z, view_vec_name)
  hlsl {
    #define USE_VIEW_VEC view_vec_name
  }
  POSTFX_VS(pos_z)
  hlsl {
    #undef USE_VIEW_VEC
  }
endmacro

macro POSTFX_VS_TEXCOORD_VIEWVEC(pos_z, texcoord_name, view_vec_name)
  hlsl {
    #define USE_TEXCOORD texcoord_name
    #define USE_VIEW_VEC view_vec_name
  }
  POSTFX_VS(pos_z)
  hlsl {
    #undef USE_TEXCOORD
    #undef USE_VIEW_VEC
  }
endmacro