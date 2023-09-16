#include <ioSys/dag_dataBlock.h>
#include <math/dag_math3d.h>
#include <math/dag_e3dColor.h>
#include <util/dag_simpleString.h>

bool cpyBlkParam(const DataBlock &source, const int s_ind, DataBlock &dest, const char *new_name)
{
  SimpleString name(new_name ? new_name : source.getParamName(s_ind));
  switch (source.getParamType(s_ind))
  {
    case DataBlock::TYPE_STRING: dest.addStr(name, source.getStr(s_ind)); return true;

    case DataBlock::TYPE_BOOL: dest.addBool(name, source.getBool(s_ind)); return true;

    case DataBlock::TYPE_INT: dest.addInt(name, source.getInt(s_ind)); return true;

    case DataBlock::TYPE_REAL: dest.addReal(name, source.getReal(s_ind)); return true;

    case DataBlock::TYPE_POINT2: dest.addPoint2(name, source.getPoint2(s_ind)); return true;

    case DataBlock::TYPE_POINT3: dest.addPoint3(name, source.getPoint3(s_ind)); return true;

    case DataBlock::TYPE_POINT4: dest.addPoint4(name, source.getPoint4(s_ind)); return true;

    case DataBlock::TYPE_IPOINT2: dest.addIPoint2(name, source.getIPoint2(s_ind)); return true;

    case DataBlock::TYPE_IPOINT3: dest.addIPoint3(name, source.getIPoint3(s_ind)); return true;

    case DataBlock::TYPE_E3DCOLOR: dest.addE3dcolor(name, source.getE3dcolor(s_ind)); return true;

    case DataBlock::TYPE_MATRIX: dest.addTm(name, source.getTm(s_ind)); return true;
  }

  return false;
}


bool cmpBlkParam(const DataBlock &source, const int s_ind, DataBlock &dest, const int d_ind)
{
  if (source.getParamType(s_ind) != dest.getParamType(d_ind))
    return false;

  switch (source.getParamType(s_ind))
  {
    case DataBlock::TYPE_STRING: return (strcmp(source.getStr(s_ind), dest.getStr(d_ind)) == 0);

    case DataBlock::TYPE_BOOL: return (source.getBool(s_ind) == dest.getBool(d_ind));

    case DataBlock::TYPE_INT: return (source.getInt(s_ind) == dest.getInt(d_ind));

    case DataBlock::TYPE_REAL: return (source.getReal(s_ind) == dest.getReal(d_ind));

    case DataBlock::TYPE_POINT2: return (source.getPoint2(s_ind) == dest.getPoint2(d_ind));

    case DataBlock::TYPE_POINT3: return (source.getPoint3(s_ind) == dest.getPoint3(d_ind));

    case DataBlock::TYPE_POINT4: return (source.getPoint4(s_ind) == dest.getPoint4(d_ind));

    case DataBlock::TYPE_IPOINT2: return (source.getIPoint2(s_ind) == dest.getIPoint2(d_ind));

    case DataBlock::TYPE_IPOINT3: return (source.getIPoint3(s_ind) == dest.getIPoint3(d_ind));

    case DataBlock::TYPE_E3DCOLOR: return (source.getE3dcolor(s_ind) == dest.getE3dcolor(d_ind));

    case DataBlock::TYPE_MATRIX: return (source.getTm(s_ind) == dest.getTm(d_ind));
  }

  return false;
}
