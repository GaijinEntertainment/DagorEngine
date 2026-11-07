#ifndef RI_SHADER_CONST_BUFFER_FLAGS
#define RI_SHADER_CONST_BUFFER_FLAGS

#define RI_CBUFFER_FLAGS__PER_DRAW_DATA_FROM_CONST_BUFFER 0x0
#define RI_CBUFFER_FLAGS__PER_DRAW_DATA_FROM_GLOBAL_BUFFER 0x1
#define RI_CBUFFER_FLAGS__HASH_VAL 0x2 // Available only for TM instances, but not for POS instances (see instancing_type = tm_vb or riPosInst)
#define RI_CBUFFER_FLAGS__INSTANCE_OFFSET_FROM_CBUFFER 0x4
#define RI_CBUFFER_FLAGS__INITIAL_POS 0x8
#define RI_CBUFFER_FLAGS__DAGDP_INSTANCES 0x10 // .w of the 4th instance float4 holds RI_INSTANCE_EXTRA__* bits (not for daGdp grass)

// Bits of the 4th instance float4 .w, written by the daGdp placers.
#define RI_INSTANCE_EXTRA__NO_OBSTACLES 0x1 // no vegetation obstacle in the grid cell of the instance

#define RI_CBUFFER_OPACITY__SCALE_BY_RI_DIST_MUL 1.0f

#endif
