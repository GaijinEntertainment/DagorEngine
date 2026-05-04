#ifndef RI_SHADER_CONST_BUFFER_FLAGS
#define RI_SHADER_CONST_BUFFER_FLAGS

#define RI_CBUFFER_FLAGS__PER_DRAW_DATA_FROM_CONST_BUFFER 0x0
#define RI_CBUFFER_FLAGS__PER_DRAW_DATA_FROM_GLOBAL_BUFFER 0x1
#define RI_CBUFFER_FLAGS__HASH_VAL 0x2 // Available only for TM instances, but not for POS instances (see instancing_type = tm_vb or riPosInst)
#define RI_CBUFFER_FLAGS__INSTANCE_OFFSET_FROM_CBUFFER 0x4

#endif
