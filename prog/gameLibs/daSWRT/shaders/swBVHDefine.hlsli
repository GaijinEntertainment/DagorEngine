#ifndef BVH_DEFINE_HLSLI
#define BVH_DEFINE_HLSLI 1

#define BVH_MAX_BLAS_DEPTH 32
#define BVH_MAX_TLAS_DEPTH 32

#define BVH_BLAS_ELEM_SIZE 1
#define BVH_BLAS_NODE_SIZE 16
#define BVH_BLAS_LEAF_SIZE 32

#define TLAS_NODE_ELEM_SIZE (4*4)
#define TLAS_LEAF_ELEM_SIZE (11*4)

#define SWRT_TILE_MIP 3 // we use that mip from closed depth, so that is tile size
#define SWRT_TILE_SIZE (1<<SWRT_TILE_MIP)


#endif