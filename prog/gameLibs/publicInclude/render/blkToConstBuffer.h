//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <math/dag_adjpow2.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <generic/dag_tab.h>

inline int getDataBlockTypeSize(int type, int string_size = 0)
{
  switch (type)
  {
    case DataBlock::TYPE_STRING: return string_size;
    case DataBlock::TYPE_BOOL:
    case DataBlock::TYPE_INT:
    case DataBlock::TYPE_REAL: return 1;
    case DataBlock::TYPE_IPOINT2:
    case DataBlock::TYPE_POINT2: return 2;
    case DataBlock::TYPE_IPOINT3:
    case DataBlock::TYPE_POINT3: return 3;
    case DataBlock::TYPE_POINT4:
    case DataBlock::TYPE_E3DCOLOR: return 4;
  }
  return 0;
}

struct NameOffset
{
  const char *name = 0;
  int ofs = -1;
  NameOffset() = default;
  NameOffset(const char *n, int o) : name(n), ofs(o) {}
};

inline int alignDataOffset(int paramsSize, int typeSize)
{
  int typeSizeAligned = min(get_closest_pow2(typeSize), 4);
  return paramsSize % typeSizeAligned ? (paramsSize & (~(typeSizeAligned - 1))) + typeSizeAligned : paramsSize;
}

inline int create_constant_buffer_offsets(Tab<NameOffset> &offsets, const DataBlock &params, int string_size)
{
  int paramsSize = 0;
  for (int i = 0; i < params.paramCount(); ++i)
  {
    const char *name = params.getParamName(i);
    int typeSize = getDataBlockTypeSize(params.getParamType(i), string_size);
    paramsSize = alignDataOffset(paramsSize, typeSize);
    offsets.push_back(NameOffset(name, paramsSize));
    paramsSize += typeSize;
  }
  return (paramsSize + 3) & ~3;
}

inline void create_constant_buffer_from_offsets(dag::ConstSpan<NameOffset> offsets, const DataBlock &params, const DataBlock &override,
  float *paramsConst)
{
  for (int i = 0; i < offsets.size(); ++i)
  {
    const char *name = offsets[i].name;
    const int ofs = offsets[i].ofs;
    // debug("param <%s> starts at %d size %d", name, paramsSize, typeSize);
    switch (params.getParamType(i))
    {
      case DataBlock::TYPE_INT: *(int *)(&paramsConst[ofs]) = override.getInt(name, params.getInt(i)); break;
      case DataBlock::TYPE_REAL: *(float *)(&paramsConst[ofs]) = override.getReal(name, params.getReal(i)); break;
      case DataBlock::TYPE_POINT2: *(Point2 *)(&paramsConst[ofs]) = override.getPoint2(name, params.getPoint2(i)); break;
      case DataBlock::TYPE_IPOINT2: *(IPoint2 *)(&paramsConst[ofs]) = override.getIPoint2(name, params.getIPoint2(i)); break;
      case DataBlock::TYPE_POINT3: *(Point3 *)(&paramsConst[ofs]) = override.getPoint3(name, params.getPoint3(i)); break;
      case DataBlock::TYPE_IPOINT3: *(IPoint3 *)(&paramsConst[ofs]) = override.getIPoint3(name, params.getIPoint3(i)); break;
      case DataBlock::TYPE_POINT4: *(Point4 *)(&paramsConst[ofs]) = override.getPoint4(name, params.getPoint4(i)); break;
      case DataBlock::TYPE_BOOL: *(int *)(&paramsConst[ofs]) = override.getBool(name, params.getBool(i)) ? 1 : 0; break;
      case DataBlock::TYPE_E3DCOLOR: *(Color4 *)(&paramsConst[ofs]) = Color4(override.getE3dcolor(name, params.getE3dcolor(i))); break;
    }
  }
}

inline void create_constant_buffer(const DataBlock &params, const DataBlock &override, Tab<Point4> &constBuffer)
{
  int paramsSize = 0;
  const int start = constBuffer.size();
  for (int i = 0; i < params.paramCount(); ++i)
  {
    const char *name = params.getParamName(i);
    int typeSize = getDataBlockTypeSize(params.getParamType(i), 0);
    paramsSize = alignDataOffset(paramsSize, typeSize);
    if ((constBuffer.size() - start) * 4 < paramsSize + typeSize)
      constBuffer.push_back(Point4(0, 0, 0, 0));
    float *paramsConst = &constBuffer[start].x;
    // debug("param <%s> starts at %d size %d", name, paramsSize, typeSize);
    switch (params.getParamType(i))
    {
      case DataBlock::TYPE_INT: *(int *)(&paramsConst[paramsSize]) = override.getInt(name, params.getInt(i)); break;
      case DataBlock::TYPE_REAL: *(float *)(&paramsConst[paramsSize]) = override.getReal(name, params.getReal(i)); break;
      case DataBlock::TYPE_POINT2: *(Point2 *)(&paramsConst[paramsSize]) = override.getPoint2(name, params.getPoint2(i)); break;
      case DataBlock::TYPE_IPOINT2: *(IPoint2 *)(&paramsConst[paramsSize]) = override.getIPoint2(name, params.getIPoint2(i)); break;
      case DataBlock::TYPE_POINT3: *(Point3 *)(&paramsConst[paramsSize]) = override.getPoint3(name, params.getPoint3(i)); break;
      case DataBlock::TYPE_IPOINT3: *(IPoint3 *)(&paramsConst[paramsSize]) = override.getIPoint3(name, params.getIPoint3(i)); break;
      case DataBlock::TYPE_POINT4: *(Point4 *)(&paramsConst[paramsSize]) = override.getPoint4(name, params.getPoint4(i)); break;
      case DataBlock::TYPE_BOOL: *(int *)(&paramsConst[paramsSize]) = override.getBool(name, params.getBool(i)) ? 1 : 0; break;
      case DataBlock::TYPE_E3DCOLOR:
        *(Color4 *)(&paramsConst[paramsSize]) = Color4(override.getE3dcolor(name, params.getE3dcolor(i)));
        break;
    }
    paramsSize += typeSize;
  }
}

inline void create_constant_buffer(const DataBlock &params, Tab<Point4> &constBuffer)
{
  int paramsSize = 0;
  const int start = constBuffer.size();
  for (int i = 0; i < params.paramCount(); ++i)
  {
    int typeSize = getDataBlockTypeSize(params.getParamType(i), 0);
    paramsSize = alignDataOffset(paramsSize, typeSize);
    if ((constBuffer.size() - start) * 4 < paramsSize + typeSize)
      constBuffer.push_back(Point4(0, 0, 0, 0));
    float *paramsConst = &constBuffer[start].x;
    switch (params.getParamType(i))
    {
      case DataBlock::TYPE_INT: *(int *)(&paramsConst[paramsSize]) = params.getInt(i); break;
      case DataBlock::TYPE_REAL: *(float *)(&paramsConst[paramsSize]) = params.getReal(i); break;
      case DataBlock::TYPE_POINT2: *(Point2 *)(&paramsConst[paramsSize]) = params.getPoint2(i); break;
      case DataBlock::TYPE_IPOINT2: *(IPoint2 *)(&paramsConst[paramsSize]) = params.getIPoint2(i); break;
      case DataBlock::TYPE_POINT3: *(Point3 *)(&paramsConst[paramsSize]) = params.getPoint3(i); break;
      case DataBlock::TYPE_IPOINT3: *(IPoint3 *)(&paramsConst[paramsSize]) = params.getIPoint3(i); break;
      case DataBlock::TYPE_POINT4: *(Point4 *)(&paramsConst[paramsSize]) = params.getPoint4(i); break;
      case DataBlock::TYPE_BOOL: *(int *)(&paramsConst[paramsSize]) = params.getBool(i) ? 1 : 0; break;
      case DataBlock::TYPE_E3DCOLOR: *(Color4 *)(&paramsConst[paramsSize]) = Color4(params.getE3dcolor(i)); break;
    }
    paramsSize += typeSize;
  }
}
