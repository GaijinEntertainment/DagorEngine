macro USE_PIXEL_TANGENT_SPACE()
  hlsl(ps) {
    //#define PERTURB_NORMAL_DISABLE 1
    //#define PERTURB_NORMAL_PRECISE_FAST 1
    #include <psh_tangent.hlsl>
  }
endmacro