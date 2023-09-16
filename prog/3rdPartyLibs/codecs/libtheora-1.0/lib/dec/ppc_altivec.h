#pragma once

#if defined(USE_ASM_ALTIVEC)
#if defined(__VEC__) && defined(__ALTIVEC__)
  #include <altivec.h>
  #include <vec_types.h>
  #include <ppu_intrinsics.h>

  #define vec_prefetch(p)    __dcbt(p)
  #define vec_permsh(a,b,c)  (vec_short8)vec_vperm(a,b,c)

  #if defined(__SNC__)
    #define vec_short8_decl_static(name, e0, e1, e2, e3, e4, e5, e6, e7) \
      static const vec_short8 name = (vec_short8){e0, e1, e2, e3, e4, e5, e6, e7}

    #define vec_uchar16_decl_static(name, e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10, e11, e12, e13, e14, e15) \
      static const vec_uchar16 name = (vec_uchar16){e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10, e11, e12, e13, e14, e15}

    #define vec_splats8h(a)    (vector signed short)(a)

  #else
    #define vec_short8_decl_static(name, e0, e1, e2, e3, e4, e5, e6, e7) \
      static const vec_short8 name = {e0, e1, e2, e3, e4, e5, e6, e7}

    #define vec_uchar16_decl_static(name, e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10, e11, e12, e13, e14, e15) \
      static const vec_uchar16 name = {e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10, e11, e12, e13, e14, e15}

    #define vec_splats8h(a)    vec_splats(a)

  #endif

  #define vec_short8_ld_static(name)
  #define vec_uchar16_ld_static(name)

#else
  #include <vectorintrinsics.h>
  #include <PPCIntrinsics.h>

  #define vec_short8  __vector4
  #define vec_uchar16 __vector4

  #define vec_prefetch(p)       __dcbt(0,p)
  #define vec_lvx(ofs,base)     __lvx(base,ofs)
  #define vec_lvlx(ofs,base)    __lvlx(base,ofs)
  #define vec_lvrx(ofs,base)    __lvrx(base,ofs)
  #define vec_stvlx(v,ofs,base) __stvlx(v,base,ofs)
  #define vec_stvrx(v,ofs,base) __stvrx(v,base,ofs)
  #define vec_vor(a,b)          __vor(a,b)
  #define vec_vaddshs(a,b)      __vaddshs(a,b)
  #define vec_vavgub(a,b)       __vavgub(a,b)
  #define vec_vsububs(a,b)      __vsububs(a,b)
  #define vec_vpkshus(a,b)      __vpkshus(a,b)
  #define vec_perm(a,b,c)       __vperm(a,b,c)
  #define vec_permsh(a,b,c)     __vperm(a,b,c)
  #define vec_splats8h(a)       __vsplth(__lvehx(&a,0), 0)
  #define vec_splat_u8(a)       __vspltisb(a)

  #define vec_short8_decl_static(name, e0, e1, e2, e3, e4, e5, e6, e7) \
    static const signed short __declspec(align(16)) __##name##_vmx[8] = {e0, e1, e2, e3, e4, e5, e6, e7}

  #define vec_uchar16_decl_static(name, e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10, e11, e12, e13, e14, e15) \
    static const unsigned char __declspec(align(16)) __##name##_vmx[16] = {e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10, e11, e12, e13, e14, e15}

  #define vec_short8_ld_static(name)  vec_short8  name = vec_lvlx(0, __##name##_vmx)
  #define vec_uchar16_ld_static(name) vec_uchar16 name = vec_lvlx(0, __##name##_vmx)

#endif
#endif
