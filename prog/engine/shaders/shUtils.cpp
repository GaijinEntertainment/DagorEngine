// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaderCommon.h>
#include <shaders/shUtils.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <shaders/shFunc.h>
#include "constRemap.h"
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include "shadersBinaryData.h"
#include "mapBinarySearch.h"

namespace ShUtils
{
const char *fsh_version(d3d::shadermodel::Version vertex_shader_model)
{
  auto str = d3d::as_string(vertex_shader_model);
  G_ASSERT(str[0] != '\0');
  return str;
}


const char *channel_type_name(int t)
{
  switch (t)
  {
    case SCTYPE_FLOAT1: return "float1";
    case SCTYPE_FLOAT2: return "float2";
    case SCTYPE_FLOAT3: return "float3";
    case SCTYPE_FLOAT4: return "float4";
    case SCTYPE_E3DCOLOR: return "e3dcolor";
    case SCTYPE_UBYTE4: return "ubyte4";
    case SCTYPE_SHORT2: return "short2";
    case SCTYPE_SHORT4: return "short4";
    case SCTYPE_USHORT2N: return "ushort2n";
    case SCTYPE_USHORT4N: return "ushort4n";
    case SCTYPE_SHORT2N: return "short2n";
    case SCTYPE_SHORT4N: return "short4n";
    case SCTYPE_HALF2: return "half2";
    case SCTYPE_HALF4: return "half4";
    case SCTYPE_DEC3N: return "dec3n";
    case SCTYPE_UDEC3: return "udec3";
  }
  return "???";
}

const char *channel_usage_name(int u)
{
  switch (u)
  {
    case SCUSAGE_POS: return "pos";
    case SCUSAGE_NORM: return "norm";
    case SCUSAGE_VCOL: return "vcol";
    case SCUSAGE_TC: return "tc";
    case SCUSAGE_LIGHTMAP: return "lightmap";
    case SCUSAGE_EXTRA: return "extra";
  }
  return "???";
}

const char *shader_var_type_name(int t)
{
  switch (t)
  {
    case SHVT_INT: return "int";
    case SHVT_REAL: return "real";
    case SHVT_COLOR4: return "color4";
    case SHVT_TEXTURE: return "texture";
    case SHVT_BUFFER: return "buffer";
    case SHVT_TLAS: return "tlas";
    case SHVT_INT4: return "int4";
    case SHVT_FLOAT4X4: return "float4x4";
  }
  return "???";
}


const char *shcod_tokname(int t)
{
  switch (t)
  {
    case SHCOD_DIFFUSE: return "DIFFUSE";
    case SHCOD_EMISSIVE: return "EMISSIVE";
    case SHCOD_SPECULAR: return "SPECULAR";
    case SHCOD_AMBIENT: return "AMBIENT";
    case SHCOD_FSH_CONST: return "FSH_CONST";
    case SHCOD_CS_CONST: return "CS_CONST";
    case SHCOD_GET_INT: return "GET_INT";
    case SHCOD_GET_INT_TOREAL: return "GET_INT(real)";
    case SHCOD_GET_IVEC_TOREAL: return "GET_IVEC(color4)";
    case SHCOD_GET_REAL: return "GET_REAL";
    case SHCOD_GET_TEX: return "GET_TEX";
    case SHCOD_GET_VEC: return "GET_VEC";
    case SHCOD_GET_GINT: return "GET_GINT";
    case SHCOD_GET_GINT_TOREAL: return "GET_GINT(real)";
    case SHCOD_GET_GIVEC_TOREAL: return "GET_GIVEC(color4)";
    case SHCOD_GET_GREAL: return "GET_GREAL";
    case SHCOD_GET_GTEX: return "GET_GTEX";
    case SHCOD_GET_GBUF: return "GET_GBUF";
    case SHCOD_GET_GTLAS: return "GET_GTLAS";
    case SHCOD_BUFFER: return "BUFFER";
    case SHCOD_CONST_BUFFER: return "CONST_BUFFER";
    case SHCOD_TLAS: return "TLAS";
    case SHCOD_GET_GVEC: return "GET_GVEC";
    case SHCOD_GET_GMAT44: return "GET_GMAT44";
    case SHCOD_G_TM: return "G_TM";
    case SHCOD_IMM_REAL: return "IMM_REAL";
    case SHCOD_IMM_VEC: return "IMM_VEC";
    case SHCOD_IMM_REAL1: return "IMM_REAL1";
    case SHCOD_IMM_SVEC1: return "IMM_SVEC1";
    case SHCOD_LVIEW: return "LVIEW";
    case SHCOD_TMWORLD: return "WTM";
    case SHCOD_MAKE_VEC: return "MAKE_VEC";
    case SHCOD_GLOB_SAMPLER: return "GLOB_SAMPLER";
    case SHCOD_SAMPLER: return "SAMPLER";
    case SHCOD_TEXTURE: return "TEXTURE";
    case SHCOD_TEXTURE_VS: return "TEXTURE_VS";
    case SHCOD_VPR_CONST: return "VPR_CONST";
    case SHCOD_ADD_REAL: return "ADD_REAL";
    case SHCOD_SUB_REAL: return "SUB_REAL";
    case SHCOD_MUL_REAL: return "MUL_REAL";
    case SHCOD_DIV_REAL: return "DIV_REAL";
    case SHCOD_ADD_VEC: return "ADD_VEC";
    case SHCOD_SUB_VEC: return "SUB_VEC";
    case SHCOD_MUL_VEC: return "MUL_VEC";
    case SHCOD_DIV_VEC: return "DIV_VEC";
    case SHCOD_CALL_FUNCTION: return "CALL_FUNCTION";
    case SHCOD_INVERSE: return "INVERSE";
    case SHCOD_STATIC_BLOCK: return "STATIC_BLOCK";
    case SHCOD_BLK_ICODE_LEN: return "BLK_ICODE_LEN";
    case SHCOD_COPY_REAL: return "COPY_REAL";
    case SHCOD_COPY_VEC: return "COPY_VEC";
    case SHCOD_REG_BINDLESS: return "REG_BINDLESS";
    case SHCOD_PACK_MATERIALS: return "PACK_MATERIALS";
    case SHCOD_RWTEX: return "RWTEX";
    case SHCOD_RWBUF: return "RWBUF";
  }
  logerr("unknown <%d>", t);
  return "?";
}

static int resolve_local_var(int ofs, dag::ConstSpan<uint32_t> stVarMap, const shaderbindump::VarList &locals)
{
  for (int j = 0; j < stVarMap.size(); j++)
    if (mapbinarysearch::unpackData(stVarMap[j]) == ofs)
      return mapbinarysearch::unpackKey(stVarMap[j]);
  return -1;
}

// dump code table to debug
void shcod_dump(dag::ConstSpan<int> cod, const shaderbindump::VarList *globals, const shaderbindump::VarList *locals,
  dag::ConstSpan<uint32_t> stVarMap, bool embrace_dump)
{
  if (embrace_dump)
    debug("dump shcode (%p)-----", cod.data());

  (void)(globals);
  (void)(locals);
  (void)(stVarMap);

  int break_idx = -1;
  for (int i = 0; i < cod.size(); i++)
  {
    int opcode = shaderopcode::getOp(cod[i]);
    String str(128, "%4d: %s: ", i, shcod_tokname(opcode));
    switch (opcode)
    {
      //************************************************************************
      //* math
      //************************************************************************
      case SHCOD_ADD_REAL:
      case SHCOD_SUB_REAL:
      case SHCOD_MUL_REAL:
      case SHCOD_DIV_REAL:
      case SHCOD_ADD_VEC:
      case SHCOD_SUB_VEC:
      case SHCOD_MUL_VEC:
      case SHCOD_DIV_VEC:
      {
        int regDst = shaderopcode::getOp3p1(cod[i]);
        int regL = shaderopcode::getOp3p2(cod[i]);
        int regR = shaderopcode::getOp3p3(cod[i]);
        str.aprintf(128, "dest=%d left=%d right=%d", regDst, regL, regR);
      }
      break;
      //************************************************************************
      //* standart
      //************************************************************************
      case SHCOD_IMM_REAL1:
      {
        int reg = shaderopcode::getOp2p1_8(cod[i]);
        int val = int(shaderopcode::getOp2p2_16(cod[i])) << 16;
        str.aprintf(128, "reg=%d val=%.4f", reg, *(real *)&val);
      }
      break;
      case SHCOD_IMM_SVEC1:
      {
        int reg = shaderopcode::getOp2p1_8(cod[i]);
        int val = int(shaderopcode::getOp2p2_16(cod[i])) << 16;
        str.aprintf(128, "reg=%d val=%.4f %.4f %.4f %.4f", reg, *(real *)&val, *(real *)&val, *(real *)&val, *(real *)&val);
      }
      break;
      case SHCOD_IMM_REAL:
      {
        int reg = shaderopcode::getOp1p1(cod[i]);
        const int *val = &cod[++i];
        str.aprintf(128, "reg=%d val=%.4f", reg, *(real *)val);
      }
      break;
      case SHCOD_IMM_VEC:
      {
        int ro = shaderopcode::getOp1p1(cod[i]);
        str.aprintf(128, "reg=%d val=%.4f %.4f %.4f %.4f", ro, *(real *)&cod[i + 1], *(real *)&cod[i + 2], *(real *)&cod[i + 3],
          *(real *)&cod[i + 4]);
        i += 4;
      }
      break;
      case SHCOD_GET_INT:
      case SHCOD_GET_INT_TOREAL:
      case SHCOD_GET_IVEC_TOREAL:
      case SHCOD_GET_REAL:
      case SHCOD_GET_VEC:
      case SHCOD_GET_TEX:
      {
        int ro = shaderopcode::getOp2p1(cod[i]);
        int ofs = shaderopcode::getOp2p2(cod[i]);
#if DAGOR_DBGLEVEL > 0
        int v = locals ? resolve_local_var(ofs, stVarMap, *locals) : -1;
        if (v >= 0)
        {
          debug_("%sreg=%d var_ofs=%d  |", str.str(), ro, ofs);
          shaderbindump::dumpVar(*locals, v);
          continue;
        }
        else
#endif
          str.aprintf(128, "reg=%d var_ofs=%d", ro, ofs);
      }
      break;
      case SHCOD_GET_GINT:
      case SHCOD_GET_GINT_TOREAL:
      case SHCOD_GET_GIVEC_TOREAL:
      case SHCOD_GET_GREAL:
      case SHCOD_GET_GVEC:
      case SHCOD_GET_GMAT44:
      case SHCOD_GET_GTEX:
      case SHCOD_GET_GBUF:
      case SHCOD_GET_GTLAS:
      {
        int ro = shaderopcode::getOp2p1(cod[i]);
        int ofs = shaderopcode::getOp2p2(cod[i]);
#if DAGOR_DBGLEVEL > 0
        if (globals && (uint32_t)ofs < (uint32_t)globals->v.size())
        {
          debug_("%sreg=%d var_ofs=%d  |", str.str(), ro, ofs);
          shaderbindump::dumpVar(*globals, ofs);
          continue;
        }
        else
#endif
          str.aprintf(128, "reg=%d var_ofs=%d", ro, ofs);
      }
      break;

      case SHCOD_CALL_FUNCTION:
      {
        int functionName = shaderopcode::getOp3p1(cod[i]);
        int rOut = shaderopcode::getOp3p2(cod[i]);
        int paramCount = shaderopcode::getOp3p3(cod[i]);

        str.aprintf(128, "func=%s out=%d", functional::getFuncName(functional::FunctionId(functionName)), rOut);
        for (int j = 0; j < paramCount; j++)
        {
          str.aprintf(128, " in=%d", cod[i + j + 1]);
        }
        i += paramCount;
      }
      break;

      case SHCOD_MAKE_VEC:
      {
        int ro = shaderopcode::getOp3p1(cod[i]);
        int r1 = shaderopcode::getOp3p2(cod[i]);
        int r2 = shaderopcode::getOp3p3(cod[i]);
        int r3 = shaderopcode::getData2p1(cod[i + 1]);
        int r4 = shaderopcode::getData2p2(cod[i + 1]);
        str.aprintf(128, "out=%d r=%d %d %d %d", ro, r1, r2, r3, r4);
        i++;
      }
      break;
      case SHCOD_LVIEW:
      case SHCOD_TMWORLD:
      case SHCOD_INVERSE: str.aprintf(128, "reg=%u  c=%u", shaderopcode::getOp2p1(cod[i]), shaderopcode::getOp2p2(cod[i])); break;

      case SHCOD_G_TM:
        str.aprintf(128, "type=%d  dest vpr_const[%u]", shaderopcode::getOp2p1_8(cod[i]), shaderopcode::getOp2p2_16(cod[i]));
        break;
      case SHCOD_BUFFER:
      case SHCOD_CONST_BUFFER:
      case SHCOD_TLAS:
      {
        int stage = shaderopcode::getOpStageSlot_Stage(cod[i]);
        int slot = shaderopcode::getOpStageSlot_Slot(cod[i]);
        int ofs = shaderopcode::getOpStageSlot_Reg(cod[i]);
        str.aprintf(128, "stage = %s slot=%d ofs=%d",
          stage == STAGE_CS ? "cs"
                            : (stage == STAGE_PS    ? "ps"
                                : stage == STAGE_VS ? "vs"
                                                    : "?"),
          slot, ofs);
      }
      break;
      case SHCOD_FSH_CONST:
      case SHCOD_VPR_CONST:
      case SHCOD_CS_CONST:
      case SHCOD_TEXTURE:
      case SHCOD_TEXTURE_VS:
      case SHCOD_RWTEX:
      case SHCOD_RWBUF:
      case SHCOD_REG_BINDLESS:
      {
        int ind = shaderopcode::getOp2p1(cod[i]);
        int ofs = shaderopcode::getOp2p2(cod[i]);
        str.aprintf(128, "ind=%d ofs=%d", ind, ofs);
      }
      break;
      case SHCOD_SAMPLER:
      {
        int ro = shaderopcode::getOpStageSlot_Slot(cod[i]);
        int ofs = shaderopcode::getOpStageSlot_Reg(cod[i]);
#if DAGOR_DBGLEVEL > 0
        int v = locals ? resolve_local_var(ofs, stVarMap, *locals) : -1;
        if (v >= 0)
        {
          debug_("%sreg=%d var_ofs=%d  |", str.str(), ro, ofs);
          shaderbindump::dumpVar(*locals, v);
          continue;
        }
        else
#endif
          str.aprintf(128, "reg=%d var_ofs=%d", ro, ofs);
      }
      break;
      case SHCOD_GLOB_SAMPLER:
      {
        int ofs = shaderopcode::getOpStageSlot_Reg(cod[i]);
        if ((uint32_t)ofs < (uint32_t)globals->v.size())
        {
#if DAGOR_DBGLEVEL > 0
          int stage = shaderopcode::getOpStageSlot_Stage(cod[i]);
          int ind = shaderopcode::getOpStageSlot_Slot(cod[i]);
          const char *stageStr = stage == STAGE_CS ? "cs" : (stage == STAGE_PS ? "ps" : stage == STAGE_VS ? "vs" : "?");
          debug_("%sstage=%s reg=%d var_ofs=%d  |", str.str(), stageStr, ind, ofs);
          shaderbindump::dumpVar(*globals, ofs);
#endif
          continue;
        }
      }
      break;
      case SHCOD_STATIC_BLOCK:
        str.aprintf(128, "texCnt=%u vsCnt=%u psCnt=%u texBase=%d vsBase=%d psBase=%d", shaderopcode::getOp3p1(cod[i]),
          shaderopcode::getOp3p2(cod[i]), shaderopcode::getOp3p3(cod[i]), (cod[i + 1] >> 16) & 0xFF, (cod[i + 1] >> 8) & 0xFF,
          cod[i + 1] & 0xFF);
        i += 1;
        break;

      case SHCOD_BLK_ICODE_LEN:
        str.aprintf(128, " %d opcodes", shaderopcode::getOp1p1(cod[i]));
        break_idx = i + 1 + shaderopcode::getOp1p1(cod[i]);
        break;

      case SHCOD_COPY_REAL:
      case SHCOD_COPY_VEC: str.aprintf(128, "dest=%d src=%d", shaderopcode::getOp2p1(cod[i]), shaderopcode::getOp2p2(cod[i])); break;
      case SHCOD_PACK_MATERIALS: str.aprintf(128, "// Shader supports packed materials."); break;

      default: str.aprintf(128, " opcode=%08X", cod[i]);
    }
    if (i == break_idx)
      debug("   ---");
    debug("%s", str.str());
  }

  if (embrace_dump)
    debug("dump shcode ok--");
}
} // namespace ShUtils
