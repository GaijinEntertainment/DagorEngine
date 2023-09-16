
float4x4 globtm_inv;
macro INIT_INV_GLOBTM(code)
(code) {
  globtm_inv@f44 = globtm_inv;
}
endmacro

macro USE_AND_INIT_INV_GLOBTM_PS()
  INIT_INV_GLOBTM(ps)
endmacro
