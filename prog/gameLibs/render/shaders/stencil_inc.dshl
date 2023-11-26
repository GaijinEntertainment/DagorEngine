
macro INIT_LOAD_STENCIL_BASE(code, texture_name, shadervar_name)
  (code) { texture_name@smp = shadervar_name hlsl { Texture2D<uint2> texture_name@smp; } }
endmacro

macro INIT_LOAD_STENCIL_GBUFFER()
  INIT_LOAD_STENCIL_BASE(ps, depth_gbuf_stencil, depth_gbuf);
endmacro
