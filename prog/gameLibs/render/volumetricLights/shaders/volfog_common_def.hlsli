#ifndef VOLFOG_COMMON_DEF_INCLUDED
#define VOLFOG_COMMON_DEF_INCLUDED 1


#define RESULT_WARP_SIZE 8
#define INITIAL_WARP_SIZE_X 4
#define INITIAL_WARP_SIZE_Y 4
#define INITIAL_WARP_SIZE_Z 8
#define MEDIA_WARP_SIZE_X 4
#define MEDIA_WARP_SIZE_Y 4
#define MEDIA_WARP_SIZE_Z 4

#define VOLFOG_DITHERING_POISSON_SAMPLE_CNT 8


#define RAYMARCH_WARP_SIZE 8
#define RECONSTRUCT_WARP_SIZE 8




// for debugging:
#define DEBUG_DISTANT_FOG_RECONSTRUCT 0
#define DEBUG_DISTANT_FOG_RAYMARCH 0
#define DEBUG_DISTANT_FOG_DISABLE_4_WAY_RECONSTRUCTION 0 // very expensive, only for testing
#define DEBUG_DISTANT_FOG_MEDIA_EXTRA_MUL 1 // for testing, should be turned off in production

// with bilateral upsampling and raw depth + transmittance weight, a tiny 3x3 kernel is fine
#define DISTANT_FOG_NEIGHBOR_KERNEL_SIZE 1 //(DEBUG_DISTANT_FOG_DISABLE_4_WAY_RECONSTRUCTION ? 1 : 2)

#define VOLFOG_MEDIA_DENSITY_EPS (1e-32)

#define DISTANT_FOG_SUBSTEP_CNT 2 // set it to <= 1 turns it off, 2 is optimal (tiny performance overhead, visible change in result)


#define MAX_RAYMARCH_FOG_DIST ((((1<<5)-1)*32 + (1<<5))*256.) // this is a hard limit, but it should be tested with smaller ones


#define DISTANT_FOG_FX_DIST_BIAS 10.0


#endif