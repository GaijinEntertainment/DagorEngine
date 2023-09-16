/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2007                *
 * by the Xiph.Org Foundation and contributors http://www.xiph.org/ *
 *                                                                  *
 ********************************************************************

  function:
      fragment.c 15469 2008-10-30 12:49:42Z tterribe

 ********************************************************************/

#define DBG_CHECK_ALTIVEC 0

#include "../internal.h"
#include "ppc_altivec.h"

#if defined(USE_ASM_ALTIVEC)
vec_short8_decl_static(c128, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
vec_uchar16_decl_static(pc, 0x11, 0x0, 0x11, 0x1, 0x11, 0x2, 0x11, 0x3, 0x11, 0x4, 0x11, 0x5, 0x11, 0x6, 0x11, 0x7);

#define A16_DISP(x) (((int)(x)) & 0xF)
#define A8_DISP(x)  (((int)(x)) & 0x7)

#define REPEAT4(macro) macro(); macro(); macro(); macro()
#define REPEAT8(macro) REPEAT4(macro); REPEAT4(macro)


#define DST_DISP0_LOOP2() \
  LOOP2(); i += 2; \
  vec_stvrx(dr1, 8, _dst); _dst+=_dst_ystride; \
  vec_stvrx(dr2, 8, _dst); _dst+=_dst_ystride;

#define DST_DISP8_LOOP2() \
  LOOP2(); i += 2; \
  vec_stvlx(dr1, 0, _dst); _dst+=_dst_ystride; \
  vec_stvlx(dr2, 0, _dst); _dst+=_dst_ystride; \

#define DST_DISP0_LOOP1() \
  LOOP1(); i ++; \
  vec_stvrx(dr, 8, _dst); _dst+=_dst_ystride;

#define DST_DISP8_LOOP1() \
  LOOP1(); i ++; \
  vec_stvlx(dr, 0, _dst); _dst+=_dst_ystride;


#define DO_4_LOOP2() \
  i = 0; \
  if (A16_DISP(_dst) == 0) { REPEAT4(DST_DISP0_LOOP2); } else { REPEAT4(DST_DISP8_LOOP2); }

#define DO_8_LOOP1() \
  i = 0; \
  if (A16_DISP(_dst) == 0) { REPEAT8(DST_DISP0_LOOP1); } else { REPEAT8(DST_DISP8_LOOP1); }

#endif

#if defined(USE_ASM_ALTIVEC) && DBG_CHECK_ALTIVEC
#include <debug/dag_debug.h>
#include <debug/dag_fatal.h>
void oc_frag_recon_intra_check(const unsigned char *_dst,int _dst_ystride,const ogg_int16_t *_residue)
{
  int i, j, res, bugs = 0;
  for (i=0;i<8;i++, _dst+=_dst_ystride)
    for (j=0;j<8;j++)
    {
      res=*_residue++;
      if (_dst[j]!=OC_CLAMP255(res+128))
      {
        cdebug("dst[%d,%d]: res=%d dst=%d != %d", i, j, res, _dst[j], OC_CLAMP255(res+128));
        bugs ++;
      }
    }
  if (bugs)
    fatal("bugs=%d dst=%p pitch=%p", bugs, _dst-_dst_ystride*8, _dst_ystride);
}

void oc_frag_recon_inter_check(const unsigned char *_dst,int _dst_ystride,
  const unsigned char *_src,int _src_ystride, const ogg_int16_t *_residue)
{
  int i, j, res, bugs = 0;
  for (i=0;i<8;i++, _dst+=_dst_ystride, _src+=_src_ystride)
    for(j=0;j<8;j++)
    {
      res=*_residue++;
      if (_dst[j]!=OC_CLAMP255(res+_src[j]))
      {
        cdebug("dst[%d,%d]: res=%d src=%d  dst=%d != %d",
          i, j, res, _src[j], _dst[j], OC_CLAMP255(res+_src[j]));
        bugs ++;
      }
    }
  if (bugs)
    fatal("bugs=%d src=%p dst=%p pitch=(%p %p)",
      bugs, _src-_src_ystride*8, _dst-_dst_ystride*8, _src_ystride, _dst_ystride);
}

void oc_frag_recon_inter2_check(const unsigned char *_dst,int _dst_ystride,
  const unsigned char *_src1,int _src1_ystride, const unsigned char *_src2, int _src2_ystride,
  const ogg_int16_t *_residue)
{
  int i, j, res, s1, bugs = 0;
  for (i=0;i<8;i++, _dst+=_dst_ystride, _src1+=_src1_ystride, _src2+=_src2_ystride)
    for(j=0;j<8;j++)
    {
      res=*_residue++;
      s1 = _src1[j] > 0 ? (int)_src1[j]-1 : 0;
      if (_dst[j]!=OC_CLAMP255(res+((s1+_src2[j]+1)>>1)))
      {
        cdebug("dst[%d,%d]: res=%d src1=%d src2=%d  dst=%d != %d",
          i, j, res, _src1[j], _src2[j], _dst[j], OC_CLAMP255(res+((s1+_src2[j]+1)>>1)));
        bugs ++;
      }
    }
  if (bugs)
    fatal("bugs=%d src1=%p src2=%p dst=%p pitch=(%p %p %p)",
      bugs, _src1-_src1_ystride*8, _src2-_src2_ystride*8, _dst-_dst_ystride*8,
      _src1_ystride, _src2_ystride, _dst_ystride);
}
#endif

void oc_frag_recon_intra_c(unsigned char *_dst,int _dst_ystride,
 const ogg_int16_t *_residue){
#if defined(USE_ASM_ALTIVEC)
  vec_short8_ld_static(c128);
  vec_short8 rr1, rr2, tr1, tr2;
  vec_uchar16 dr1, dr2;
  int i;

  #if DBG_CHECK_ALTIVEC
  if (A16_DISP(_residue) | A8_DISP(_dst) | (_dst_ystride & 0xF))
    fatal("non-aligned: _residue=%p, _dst=%p, stride=%d", _residue, _dst, _dst_ystride);
  #endif

  #define LOOP2() \
    rr1 = vec_lvx(i*16, _residue); \
    rr2 = vec_lvx(i*16+16, _residue); \
    tr1 = vec_vaddshs(rr1, c128); \
    tr2 = vec_vaddshs(rr2, c128); \
    dr1 = vec_vpkshus(tr1, tr1);  \
    dr2 = vec_vpkshus(tr2, tr2)

  DO_4_LOOP2();
  #undef LOOP2

  #if DBG_CHECK_ALTIVEC
  _dst -= _dst_ystride*8;
  oc_frag_recon_intra_check(_dst,_dst_ystride,_residue);
  #endif

#else
  int i;
  for(i=0;i<8;i++){
    int j;
    for(j=0;j<8;j++){
      int res;
      res=*_residue++;
      _dst[j]=OC_CLAMP255(res+128);
    }
    _dst+=_dst_ystride;
  }
#endif
}


void oc_frag_recon_inter_c(unsigned char *_dst,int _dst_ystride,
 const unsigned char *_src,int _src_ystride,const ogg_int16_t *_residue){
#if defined(USE_ASM_ALTIVEC)
  vec_uchar16_ld_static(pc);
  vec_short8 rr1, rr2, tr1, tr2, pr1, pr2;
  vec_uchar16 sr1, sr2, sr1r, sr2r;
  vec_uchar16 dr1, dr2;
  int dst_disp = A16_DISP(_dst);
  int src_disp = A16_DISP(_src);
  const unsigned char *__restrict _src_next = _src+_src_ystride;
  int i;

  #if DBG_CHECK_ALTIVEC
  if (A16_DISP(_residue) | A8_DISP(_dst) | (_dst_ystride & 0xF) | (_src_ystride & 0xF))
    fatal("non-aligned: _residue=%p, _dst=%p, stride=%p, _src=%p _src_ystride=%p",
      _residue, _dst, _dst_ystride, _src, _src_ystride);
  #endif

  vec_prefetch(_src_next);
  _src_ystride = _src_ystride*2;

  #define LOOP2_MATH() \
    vec_prefetch(_src); vec_prefetch(_src_next); \
    rr1 = vec_lvx(i*16, _residue); \
    rr2 = vec_lvx(i*16+16, _residue); \
    pr1 = vec_permsh(sr1, pc, pc); \
    pr2 = vec_permsh(sr2, pc, pc); \
    tr1 = vec_vaddshs(rr1, pr1); \
    tr2 = vec_vaddshs(rr2, pr2); \
    dr1 = vec_vpkshus(tr1, tr1); \
    dr2 = vec_vpkshus(tr2, tr2)

  if (src_disp <= 8)
  {
    #define LOOP2() \
      sr1 = vec_lvlx(0, _src);      _src+=_src_ystride; \
      sr2 = vec_lvlx(0, _src_next); _src_next+=_src_ystride; \
      LOOP2_MATH() 

    DO_4_LOOP2();
    #undef LOOP2
  }
  else
  {
    #define LOOP2() \
      sr1 = vec_lvlx(0, _src); \
      sr2 = vec_lvlx(0, _src_next); \
      if (src_disp > 8) \
      { \
        sr1r = vec_lvrx(16, _src); \
        sr2r = vec_lvrx(16, _src_next); \
        sr1 = vec_vor(sr1, sr1r); \
        sr2 = vec_vor(sr2, sr2r); \
      } \
      _src+=_src_ystride; _src_next+=_src_ystride; \
      LOOP2_MATH()

    DO_4_LOOP2();
    #undef LOOP2
  }
  #undef LOOP2_MATH

  #if DBG_CHECK_ALTIVEC
  _src-=_src_ystride*4;
  _dst-=_dst_ystride*8;
  oc_frag_recon_inter_check(_dst,_dst_ystride,_src,_src_ystride/2,_residue);
  #endif

#else
  int i;
  for(i=0;i<8;i++){
    int j;
    for(j=0;j<8;j++){
      int res;
      res=*_residue++;
      _dst[j]=OC_CLAMP255(res+_src[j]);
    }
    _dst+=_dst_ystride;
    _src+=_src_ystride;
  }
#endif
}

void oc_frag_recon_inter2_c(unsigned char *_dst,int _dst_ystride,
 const unsigned char *_src1,int _src1_ystride,const unsigned char *_src2,
 int _src2_ystride,const ogg_int16_t *_residue){
#if defined(USE_ASM_ALTIVEC)
  vec_uchar16_ld_static(pc);
  vec_uchar16 c1 = vec_splat_u8(1);
  vec_short8 rr;
  vec_uchar16 sr1, sr2, sr1m;
  vec_uchar16 avg;
  vec_uchar16 dr;
  int dst_disp = A16_DISP(_dst);
  int src1_disp = A16_DISP(_src1);
  int src2_disp = A16_DISP(_src2);
  int i;

  #if DBG_CHECK_ALTIVEC
  if (A16_DISP(_residue) | A8_DISP(_dst) | (_dst_ystride & 0xF) | (_src1_ystride & 0xF) | (_src2_ystride & 0xF))
    fatal("non-aligned: _residue=%p, _dst=%p, stride=%p, _src1=%p _src1_ystride=%p, _src2=%p _src2_ystride=%p",
      _residue, _dst, _dst_ystride, _src1, _src1_ystride, _src2, _src2_ystride);
  #endif

  #define LOOP1_MATH() \
    vec_prefetch(_src1); vec_prefetch(_src2); \
    sr1m = vec_vsububs(sr1,c1); \
    rr = vec_lvx(i*16, _residue); \
    avg = vec_vavgub(sr1m, sr2); \
    rr = vec_vaddshs(rr, vec_permsh(avg, pc, pc)); \
    dr = vec_vpkshus(rr, rr) \

  if (src1_disp <= 8 && src2_disp <= 8)
  {
    #define LOOP1() \
      sr1 = vec_lvlx(0, _src1); _src1+=_src1_ystride; \
      sr2 = vec_lvlx(0, _src2); _src2+=_src2_ystride; \
      LOOP1_MATH() 

    DO_8_LOOP1();
    #undef LOOP1
  }
  else if (src1_disp > 8 && src2_disp > 8)
  {
    #define LOOP1() \
      sr1 = vec_vor(vec_lvlx(0, _src1), vec_lvrx(16, _src1)); _src1+=_src1_ystride; \
      sr2 = vec_vor(vec_lvlx(0, _src2), vec_lvrx(16, _src2)); _src2+=_src2_ystride; \
      LOOP1_MATH() 

    DO_8_LOOP1();
    #undef LOOP1
  }
  else if (src1_disp > 8)
  {
    #define LOOP1() \
      sr1 = vec_vor(vec_lvlx(0, _src1), vec_lvrx(16, _src1)); _src1+=_src1_ystride; \
      sr2 = vec_lvlx(0, _src2); _src2+=_src2_ystride; \
      LOOP1_MATH() 

    DO_8_LOOP1();
    #undef LOOP1
  }
  else
  {
    #define LOOP1() \
      sr1 = vec_lvlx(0, _src1); _src1+=_src1_ystride; \
      sr2 = vec_vor(vec_lvlx(0, _src2), vec_lvrx(16, _src2)); _src2+=_src2_ystride; \
      LOOP1_MATH() 

    DO_8_LOOP1();
    #undef LOOP1
  }
  #undef LOOP2_MATH

  #if DBG_CHECK_ALTIVEC
  _src1-=_src1_ystride*8;
  _src2-=_src2_ystride*8;
  _dst-=_dst_ystride*8;
  oc_frag_recon_inter2_check(_dst,_dst_ystride,_src1,_src1_ystride,_src2,_src2_ystride,_residue);
  #endif

#else
  int i;
  for(i=0;i<8;i++){
    int j;
    for(j=0;j<8;j++){
      int res;
      res=*_residue++;
      _dst[j]=OC_CLAMP255(res+((int)_src1[j]+_src2[j]>>1));
    }
    _dst+=_dst_ystride;
    _src1+=_src1_ystride;
    _src2+=_src2_ystride;
  }
#endif
}

/*Computes the predicted DC value for the given fragment.
  This requires that the fully decoded DC values be available for the left,
   upper-left, upper, and upper-right fragments (if they exist).
  _frag:      The fragment to predict the DC value for.
  _fplane:    The fragment plane the fragment belongs to.
  _x:         The x-coordinate of the fragment.
  _y:         The y-coordinate of the fragment.
  _pred_last: The last fully-decoded DC value for each predictor frame
               (OC_FRAME_GOLD, OC_FRAME_PREV and OC_FRAME_SELF).
              This should be initialized to 0's for the first fragment in each
               color plane.
  Return: The predicted DC value for this fragment.*/
int oc_frag_pred_dc(const oc_fragment *_frag,
 const oc_fragment_plane *_fplane,int _x,int _y,int _pred_last[3]){
  static const int PRED_SCALE[16][4]={
    /*0*/
    {0,0,0,0},
    /*OC_PL*/
    {1,0,0,0},
    /*OC_PUL*/
    {1,0,0,0},
    /*OC_PL|OC_PUL*/
    {1,0,0,0},
    /*OC_PU*/
    {1,0,0,0},
    /*OC_PL|OC_PU*/
    {1,1,0,0},
    /*OC_PUL|OC_PU*/
    {0,1,0,0},
    /*OC_PL|OC_PUL|PC_PU*/
    {29,-26,29,0},
    /*OC_PUR*/
    {1,0,0,0},
    /*OC_PL|OC_PUR*/
    {75,53,0,0},
    /*OC_PUL|OC_PUR*/
    {1,1,0,0},
    /*OC_PL|OC_PUL|OC_PUR*/
    {75,0,53,0},
    /*OC_PU|OC_PUR*/
    {1,0,0,0},
    /*OC_PL|OC_PU|OC_PUR*/
    {75,0,53,0},
    /*OC_PUL|OC_PU|OC_PUR*/
    {3,10,3,0},
    /*OC_PL|OC_PUL|OC_PU|OC_PUR*/
    {29,-26,29,0}
  };
  static const int PRED_SHIFT[16]={0,0,0,0,0,1,0,5,0,7,1,7,0,7,4,5};
  static const int PRED_RMASK[16]={0,0,0,0,0,1,0,31,0,127,1,127,0,127,15,31};
  static const int BC_MASK[8]={
    /*No boundary condition.*/
    OC_PL|OC_PUL|OC_PU|OC_PUR,
    /*Left column.*/
    OC_PU|OC_PUR,
    /*Top row.*/
    OC_PL,
    /*Top row, left column.*/
    0,
    /*Right column.*/
    OC_PL|OC_PUL|OC_PU,
    /*Right and left column.*/
    OC_PU,
    /*Top row, right column.*/
    OC_PL,
    /*Top row, right and left column.*/
    0
  };
  /*Predictor fragments, left, up-left, up, up-right.*/
  const oc_fragment *predfr[4];
  /*The frame used for prediction for this fragment.*/
  int                pred_frame;
  /*The boundary condition flags.*/
  int                bc;
  /*DC predictor values: left, up-left, up, up-right, missing values
     skipped.*/
  int                p[4];
  /*Predictor count.*/
  int                np;
  /*Which predictor constants to use.*/
  int                pflags;
  /*The predicted DC value.*/
  int                ret;
  int                i;
  pred_frame=OC_FRAME_FOR_MODE[_frag->mbmode];
  bc=(_x==0)+((_y==0)<<1)+((_x+1==_fplane->nhfrags)<<2);
  predfr[0]=_frag-1;
  predfr[1]=_frag-_fplane->nhfrags-1;
  predfr[2]=predfr[1]+1;
  predfr[3]=predfr[2]+1;
  np=0;
  pflags=0;
  for(i=0;i<4;i++){
    int pflag;
    pflag=1<<i;
    if((BC_MASK[bc]&pflag)&&predfr[i]->coded&&
     OC_FRAME_FOR_MODE[predfr[i]->mbmode]==pred_frame){
      p[np++]=predfr[i]->dc;
      pflags|=pflag;
    }
  }
  if(pflags==0)return _pred_last[pred_frame];
  else{
    ret=PRED_SCALE[pflags][0]*p[0];
    /*LOOP VECTORIZES.*/
    for(i=1;i<np;i++)ret+=PRED_SCALE[pflags][i]*p[i];
    ret=OC_DIV_POW2(ret,PRED_SHIFT[pflags],PRED_RMASK[pflags]);
  }
  if((pflags&(OC_PL|OC_PUL|OC_PU))==(OC_PL|OC_PUL|OC_PU)){
    if(abs(ret-p[2])>128)ret=p[2];
    else if(abs(ret-p[0])>128)ret=p[0];
    else if(abs(ret-p[1])>128)ret=p[1];
  }
  return ret;
}
