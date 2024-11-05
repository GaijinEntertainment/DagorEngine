// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shMaterialUtils.h>
#include <shaders/dag_shaderCommon.h>
#include "shadersBinaryData.h"
#include <shaders/shOpcode.h>
#include <shaders/shOpcodeFormat.h>

dag::Vector<ShaderVarDesc> get_shclass_script_vars_list(const char *shclass_name)
{
  const ScriptedShadersBinDump &binDump = shBinDump();
  const shaderbindump::ShaderClass *shclass = binDump.findShaderClass(shclass_name);
  dag::Vector<ShaderVarDesc> result;
  if (shclass)
  {
    const shaderbindump::VarList &localVars = shclass->localVars;
    for (int i = 0; i < localVars.size(); ++i)
    {
      ShaderVarDesc varDesc;
      varDesc.name = (const char *)binDump.varMap[localVars.getNameId(i)];
      varDesc.type = ShaderVarType(localVars.getType(i));
      bool accepted = false;
      switch (varDesc.type)
      {
        case SHVT_INT:
        case SHVT_REAL:
          // Don't make assumptions that we can copy whole color4 if the type is int or real.
          varDesc.value.c4().zero();
          varDesc.value.i = localVars.get<int>(i);
          accepted = true;
          break;
        case SHVT_COLOR4:
          varDesc.value.c4() = localVars.get<Color4>(i);
          accepted = true;
          break;
        default: break;
      }
      if (accepted)
        result.emplace_back(eastl::move(varDesc));
    }
  }
  return result;
}

unsigned get_shclass_used_tex_mask(const shaderbindump::ShaderClass *sclass)
{
  unsigned usedTexMask = 0;
  for (int j = 0; j < sclass->initCode.size(); j += 2)
    if (shaderopcode::getOp(sclass->initCode[j + 1]) == SHCOD_TEXTURE)
      usedTexMask |= 1 << shaderopcode::getOp2p1(sclass->initCode[j + 1]);
  for (int ci = 0; ci < sclass->code.size(); ci++)
  {
    dag::ConstSpan<int> cod = sclass->code[ci].initCode;
    for (int j = 0; j < cod.size(); j += 2)
      if (shaderopcode::getOp(cod[j + 1]) == SHCOD_TEXTURE)
        usedTexMask |= 1 << shaderopcode::getOp2p1(cod[j + 1]);
  }
  return usedTexMask;
}

unsigned get_shclass_used_tex_mask(const char *shclass_name)
{
  const ScriptedShadersBinDump &binDump = shBinDump();
  const shaderbindump::ShaderClass *sclass = binDump.findShaderClass(shclass_name);
  if (sclass)
    return get_shclass_used_tex_mask(sclass);
  return 0;
}
