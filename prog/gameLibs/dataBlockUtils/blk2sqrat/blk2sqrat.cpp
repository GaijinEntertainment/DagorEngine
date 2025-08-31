// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dataBlockUtils/blk2sqrat/blk2sqrat.h>

#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IPoint4.h>
#include <sqrat.h>

Sqrat::Object blk_to_sqrat(HSQUIRRELVM vm, const DataBlock &blk)
{
  Sqrat::Table sqobj(vm);

  for (int i = 0; i < blk.paramCount(); ++i)
  {
    int type = blk.getParamType(i);
    const char *key = blk.getParamName(i);
    G_ASSERT(key != NULL);

    switch (type)
    {
      case DataBlock::TYPE_NONE: sqobj.SetValue(key, Sqrat::Object(vm)); break;

      case DataBlock::TYPE_STRING: sqobj.SetValue(key, blk.getStr(i)); break;

      case DataBlock::TYPE_INT: sqobj.SetValue(key, blk.getInt(i)); break;

      case DataBlock::TYPE_INT64: sqobj.SetValue(key, (int64_t)blk.getInt64(i)); break;

      case DataBlock::TYPE_BOOL: sqobj.SetValue(key, (bool)blk.getBool(i)); break;

      case DataBlock::TYPE_REAL: sqobj.SetValue(key, blk.getReal(i)); break;

      case DataBlock::TYPE_POINT2:
      {
        Point2 p2 = blk.getPoint2(i);
        Sqrat::Array child(vm);
        child.Append(p2.x);
        child.Append(p2.y);
        sqobj.SetValue(key, child);
        break;
      }

      case DataBlock::TYPE_POINT3:
      {
        Point3 p3 = blk.getPoint3(i);
        Sqrat::Array child(vm);
        child.Append(p3.x);
        child.Append(p3.y);
        child.Append(p3.z);
        sqobj.SetValue(key, child);
        break;
      }

      case DataBlock::TYPE_POINT4:
      {
        Point4 p4 = blk.getPoint4(i);
        Sqrat::Array child(vm);
        child.Append(p4.x);
        child.Append(p4.y);
        child.Append(p4.z);
        child.Append(p4.w);
        sqobj.SetValue(key, child);
        break;
      }

      case DataBlock::TYPE_IPOINT2:
      {
        IPoint2 ip2 = blk.getIPoint2(i);
        Sqrat::Array child(vm);
        child.Append(ip2.x);
        child.Append(ip2.y);
        sqobj.SetValue(key, child);
        break;
      }

      case DataBlock::TYPE_IPOINT3:
      {
        IPoint3 ip3 = blk.getIPoint3(i);
        Sqrat::Array child(vm);
        child.Append(ip3.x);
        child.Append(ip3.y);
        child.Append(ip3.z);
        sqobj.SetValue(key, child);
        break;
      }

      case DataBlock::TYPE_IPOINT4:
      {
        IPoint4 ip4 = blk.getIPoint4(i);
        Sqrat::Array child(vm);
        child.Append(ip4.x);
        child.Append(ip4.y);
        child.Append(ip4.z);
        child.Append(ip4.w);
        sqobj.SetValue(key, child);
        break;
      }

      case DataBlock::TYPE_E3DCOLOR:
      {
        E3DCOLOR c = blk.getE3dcolor(i);
        Sqrat::Array child(vm);
        child.Append(c.r);
        child.Append(c.g);
        child.Append(c.b);
        child.Append(c.a);
        sqobj.SetValue(key, child);
        break;
      }

      case DataBlock::TYPE_MATRIX:
      {
        TMatrix m = blk.getTm(i);
        Sqrat::Array child(vm);
        for (int col = 0; col < 4; ++col)
        {
          Point3 p3 = m.getcol(col);
          Sqrat::Array j(vm);
          j.Append(p3.x);
          j.Append(p3.y);
          j.Append(p3.z);
          child.Append(j);
        }
        sqobj.SetValue(key, child);
        break;
      }

      default: G_ASSERT(!"Unexpected type"); return Sqrat::Object();
    }
  }

  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock *sub = blk.getBlock(i);
    const char *key = sub->getBlockName();
    G_ASSERT(key != NULL);

    sqobj.SetValue(key, blk_to_sqrat(vm, *sub));
  }

  return sqobj;
}

void sqrat_to_blk(DataBlock &result, Sqrat::Object sq_tbl)
{
  Sqrat::Object::iterator it;
  while (sq_tbl.Next(it))
  {
    Sqrat::Object valObj(it.getValue(), sq_tbl.GetVM());
    switch (valObj.GetType())
    {
      case OT_INTEGER:
      {
        int64_t ival = valObj.Cast<int64_t>();
        if (ival <= INT32_MAX && ival >= INT32_MIN)
          result.setInt(it.getName(), (int)ival);
        else
          result.setInt64(it.getName(), ival);
      }
      break;
      case OT_FLOAT: result.setReal(it.getName(), valObj.Cast<float>()); break;
      case OT_BOOL: result.setBool(it.getName(), valObj.Cast<bool>()); break;
      case OT_STRING: result.setStr(it.getName(), valObj.GetVar<const char *>().value); break;
      case OT_TABLE:
      {
        DataBlock valBlk;
        sqrat_to_blk(valBlk, valObj);
        result.setBlock(&valBlk, it.getName());
      }
      break;
      default: break;
    }
  }
}
