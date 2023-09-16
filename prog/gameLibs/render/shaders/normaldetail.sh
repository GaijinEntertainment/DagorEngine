include "psh_tangent.sh"
//reference!

macro USE_NORMAL_DETAIL()
//http://blog.selfshadow.com/publications/blending-in-detail/
hlsl(ps) {
  //#define PERTURB_NORMAL_PRECISE_FAST 1
  #include <psh_tangent.hlsl>
  #include <normaldetail.hlsl>
}
endmacro