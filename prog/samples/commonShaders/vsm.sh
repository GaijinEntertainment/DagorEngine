macro INIT_VSM()
endmacro

macro USE_VSM()
hlsl(ps) {
  #define def_vsm_shadow_blurred(a) 1
}
endmacro