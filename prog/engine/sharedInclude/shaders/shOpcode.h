// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_preprocessor.h>

#define SHCOD_NOP DAG_CONCAT(SHCOD_NOP_, __LINE__)

// opcodes are sorted in "most-used-first" order
enum
{
  SHCOD_GET_GVEC,        // 2p      | load global var to VEC4 reg         | p1=reg#  p2=varId
  SHCOD_GET_GREAL,       // 2p      | load global var to REAL reg         | p1=reg#  p2=varId
  SHCOD_GET_GTEX,        // 2p      | load global tex to reg              | p1=reg#  p2=varId
  SHCOD_GET_GINT,        // 2p      |                                     | p1=reg#  p2=varId
  SHCOD_GET_GINT_TOREAL, // 2p      | load int global var to REAL reg     | p1=reg#  p2=varId
  SHCOD_IMM_REAL1,       // 2p_8_16 | unpack imm to REAL reg              | p1=reg# (8 bits)  p2=upper 16 bits of float
  SHCOD_IMM_SVEC1,       // 2p_8_16 | unpack and splat imm into VEC4 reg  | p1=reg# (8 bits)  p2=upper 16 bits of float
  SHCOD_IMM_REAL,        // 1p + 1d | load imm to REAL reg                | p1=reg#  d1=imm data
  SHCOD_IMM_VEC,         // 1p + 4d | load 4 imm into VEC4 reg            | p1=dest#  d1..4=imm data
  SHCOD_GET_VEC,         // 2p      | load local var to VEC4 reg          | p1=reg#  p2=varId
  SHCOD_GET_REAL,        // 2p      | load local var to REAL reg          | p1=reg#  p2=varId
  SHCOD_GET_TEX,         // 2p      | load local tex to reg               | p1=reg#  p2=varId
  SHCOD_GET_INT,         // 2p      | load local var to INT reg           | p1=reg#  p2=varId
  SHCOD_GET_INT_TOREAL,  // 2p      | load INT local var to REAL reg      | p1=reg#  p2=varId

  SHCOD_VPR_CONST,    // 2p      | set VS const[ind] from VEC4 reg     | p1=ind p2=reg#
  SHCOD_FSH_CONST,    // 2p      | set PS const[ind] from VEC4 reg     | p1=ind p2=reg#
  SHCOD_TEXTURE,      // 2p      | set texture                         | p1=ind p2=reg#
  SHCOD_G_TM,         // 2p_8_16 | set 4xVEC4 const for GM/PM/VPM      | p1=type (8 bits)  p2=ind
  SHCOD_GLOB_SAMPLER, // 3p      | set sampler from global var         | p1=stage p2=ind p3=varId

  SHCOD_MUL_REAL, // 3p      | REAL: dest# = left# * right#        | p1=dest# p2=left# p3=right#
  SHCOD_DIV_REAL, // 3p      | REAL: dest# = left# / right#        | p1=dest# p2=left# p3=right#
  SHCOD_ADD_REAL, // 3p      | REAL: dest# = left# + right#        | p1=dest# p2=left# p3=right#
  SHCOD_SUB_REAL, // 3p      | REAL: dest# = left# - right#        | p1=dest# p2=left# p3=right#
  SHCOD_NOP,
  SHCOD_COPY_REAL, // 2p      | REAL: #dest = #src                  | p1=dest#  p2=src#

  SHCOD_MUL_VEC,  // 3p      | VEC4: dest# = left# * right# (CW)   | p1=dest# p2=left# p3=right#
  SHCOD_DIV_VEC,  // 3p      | VEC4: dest# = left# / right# (CW)   | p1=dest# p2=left# p3=right#
  SHCOD_ADD_VEC,  // 3p      | VEC4: dest# = left# + right#        | p1=dest# p2=left# p3=right#
  SHCOD_SUB_VEC,  // 3p      | VEC4: dest# = left# - right#        | p1=dest# p2=left# p3=right#
  SHCOD_MAKE_VEC, // 3p + 1d | load VEC4 reg from 4 REAL regs      | p1=dest# p2=x_reg# p3=y_reg# d1=z_reg# d2=w_reg#
  SHCOD_COPY_VEC, // 2p      | VEC4: #dest = #src                  | p1=dest#  p2=src#

  SHCOD_LVIEW,   // 2p      | load local view M44.col to VEC4 reg | p1=reg#  p2=col
  SHCOD_TMWORLD, // 2p      | load world M44.col to VEC4 reg      | p1=reg#  p2=col
  SHCOD_SAMPLER, // 3p      | set sampler from local var          | p1=stage p2=ind p3=varId
  SHCOD_NOP,
  SHCOD_NOP,

  SHCOD_CALL_FUNCTION, // 3p + *d | call function                       | p1=func_id p2=dest# p3=param count d*=data

  SHCOD_GET_GIVEC_TOREAL, // 2p   | load global int4 var to VEC4 reg    | p1=reg#  p2=varId
  SHCOD_GET_IVEC_TOREAL,  // 2p   | load local int4 var to VEC4 reg     | p1=reg#  p2=varId

  SHCOD_TLAS,      // 3psso   | set TLAS                            | p1=stage,p2=slot p3=reg#
  SHCOD_GET_GTLAS, // 2p      | load global TLAS to reg             | p1=reg#  p2=varId
  SHCOD_NOP,
  SHCOD_NOP,
  SHCOD_NOP,
  SHCOD_NOP,
  SHCOD_NOP,
  SHCOD_NOP,
  SHCOD_NOP,
  SHCOD_NOP,
  SHCOD_NOP,
  SHCOD_NOP,
  SHCOD_NOP,
  SHCOD_NOP,

  SHCOD_INVERSE, // 2p      | reg# = -reg#                        | p1=reg# p2=reg_cnt
  SHCOD_NOP,
  SHCOD_RWBUF, // 2p      | set r/w buffer (UAV)                | p1=ind p2=reg#

  SHCOD_STATIC_BLOCK,  // 3p + 1d | static state block params           | p1=tex_cnt p2=vs_cnt p3=ps_cnt p4=tex_base p5=vs_base
                       // p6=ps_base
  SHCOD_BLK_ICODE_LEN, // 1p      | size of next perm block code        | p1=opcode_cnt

  SHCOD_DIFFUSE,  // 0p      |                                     |
  SHCOD_EMISSIVE, // 0p      |                                     |
  SHCOD_SPECULAR, // 0p      |                                     |
  SHCOD_AMBIENT,  // 0p      |                                     |

  SHCOD_RWTEX,         // 2p      | set r/w texture (UAV)               | p1=ind p2=reg#
  SHCOD_CS_CONST,      // 2p      | set CS const[ind] from VEC4 reg     | p1=ind p2=reg#
  SHCOD_TEXTURE_VS,    // 2p      | set VS texture                      | p1=ind p2=reg#
  SHCOD_BUFFER,        // 3psso   | set buffer                          | p1=stage,p2=slot p3=reg#
  SHCOD_CONST_BUFFER,  // 3psso   | set const buffer                    | p1=stage,p2=slot p3=reg#
  SHCOD_GET_GBUF,      // 2p      | load global buf to reg              | p1=reg#  p2=varId
  SHCOD_GET_GMAT44,    // 2p      | load global var to FLOAT4x4 reg     | p1=reg#  p2=varId
  SHCOD_REG_BINDLESS,  // 2p      | register bindless tex from reg and store its id to consts[ind] | p1=ind p2=reg#
  SHCOD_PACK_MATERIALS // 0p      | cbufs for stcode with this flag will be stored in one buffer |
};
enum
{
  P1_SHCOD_G_TM_GLOBTM = 0,
  P1_SHCOD_G_TM_PROJTM = 1,
  P1_SHCOD_G_TM_VIEWPROJTM = 2,
};

#undef SHCOD_NOP
