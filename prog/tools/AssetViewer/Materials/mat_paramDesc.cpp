// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mat_paramDesc.h"

#include <ioSys/dag_dataBlock.h>


void MaterialParamDescr::loadFromBlk(const DataBlock &blk)
{
  name = blk.getBlockName();
  caption = blk.getStr("caption", NULL);
  type = strToType(blk.getStr("type", typeToStr(PT_UNKNOWN)));

  iConstaraints = blk.getIPoint2("constrains_int", IPoint2(0, 0));
  slot = blk.getInt("slot", 0);
  rConstaraints = blk.getPoint2("constrains_real", Point2(-MAX_REAL, MAX_REAL));

  if (!blk.getBool("use_real_min", false))
    rConstaraints.x = -MAX_REAL;
  if (!blk.getBool("use_real_max", false))
    rConstaraints.y = MAX_REAL;

  const DataBlock *comboBlk = blk.getBlockByName("values");

  if (comboBlk)
  {
    for (int i = 0; i < comboBlk->paramCount(); ++i)
      comboVals.push_back() = comboBlk->getStr(i);

    defStrVal = blk.getStr("def", comboVals.size() ? comboVals[0] : "");
  }

  if (type == PT_E3DCOLOR)
  {
    if (stricmp(name, "mat_diff") == 0)
      slot = 0;
    else if (stricmp(name, "mat_amb") == 0)
      slot = 1;
    else if (stricmp(name, "mat_spec") == 0)
      slot = 2;
    else if (stricmp(name, "mat_emis") == 0)
      slot = 3;
    else
      slot = -1;
  }

  texPath = blk.getStr("texture", texPath);
}


const char *MaterialParamDescr::typeToStr(MaterialParamDescr::Type type)
{
  switch (type)
  {
    case PT_TEXTURE: return "texture";
    case PT_TRIPLE_INT: return "triple_int";
    case PT_TRIPLE_REAL: return "triple_real";
    case PT_E3DCOLOR: return "e3dcolor";
    case PT_COMBO: return "combo";
    case PT_CUSTOM: return "custom";
  }

  return NULL;
}


MaterialParamDescr::Type MaterialParamDescr::strToType(const char *type)
{
  if (!stricmp(type, "texture"))
    return PT_TEXTURE;
  if (!stricmp(type, "triple_int"))
    return PT_TRIPLE_INT;
  if (!stricmp(type, "triple_real"))
    return PT_TRIPLE_REAL;
  if (!stricmp(type, "e3dcolor"))
    return PT_E3DCOLOR;
  if (!stricmp(type, "combo"))
    return PT_COMBO;
  if (!stricmp(type, "custom"))
    return PT_CUSTOM;

  return PT_UNKNOWN;
}
