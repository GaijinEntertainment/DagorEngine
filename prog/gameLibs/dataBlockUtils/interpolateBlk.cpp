#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h>
#include <math/dag_e3dColor.h>

void interpolate_datablock(const DataBlock &from, DataBlock &to, float t)
{
  to.clearData();

  for (unsigned int blockNo = 0; blockNo < from.blockCount(); blockNo++)
  {
    const DataBlock *blk = from.getBlock(blockNo);
    const char *type = blk->getStr("type", NULL);
    if (!type)
    {
      DataBlock *toSubBlk = to.addNewBlock(blk->getBlockName());
      interpolate_datablock(*blk, *toSubBlk, t);
    }
    else if (!stricmp(type, "curve"))
    {
      Point2 prevParam(ZERO<Point2>());
      bool prevParamOk = false;
      bool valOk = false;
      for (int paramNo = 0; paramNo < blk->paramCount(); paramNo++)
      {
        if (stricmp(blk->getParamName(paramNo), "point") != 0)
          continue;

        Point2 param = blk->getPoint2(paramNo);
        if (param.x >= t && prevParamOk)
        {
          float val = cvt(t, prevParam.x, param.x, prevParam.y, param.y);
          to.setReal(blk->getBlockName(), val);
          valOk = true;
          break;
        }
        else
        {
          prevParam = param;
          prevParamOk = true;
        }
      }

      if (!valOk && prevParamOk)
        to.setReal(blk->getBlockName(), prevParam.y);
    }
    else if (!stricmp(type, "curve_p2"))
    {
      Point3 prevParam(ZERO<Point3>());
      bool prevParamOk = false;
      bool valOk = false;
      for (int paramNo = 0; paramNo < blk->paramCount(); paramNo++)
      {
        if (stricmp(blk->getParamName(paramNo), "point") != 0)
          continue;

        Point3 param = blk->getPoint3(paramNo);
        if (param.x >= t && prevParamOk)
        {
          Point2 val(cvt(t, prevParam.x, param.x, prevParam.y, param.y), cvt(t, prevParam.x, param.x, prevParam.z, param.z));
          to.setPoint2(blk->getBlockName(), Point2(val.x, val.y));
          valOk = true;
          break;
        }
        else
        {
          prevParam = param;
          prevParamOk = true;
        }
      }

      if (!valOk && prevParamOk)
        to.setPoint2(blk->getBlockName(), Point2::yz(prevParam));
    }
    else if (!stricmp(type, "gradient"))
    {
      float prevX = 0.f;
      E3DCOLOR prevColor{};
      bool prevParamOk = false;
      bool valOk = false;
      for (unsigned int valNo = 0; valNo < blk->blockCount(); valNo++)
      {
        const DataBlock *valBlk = blk->getBlock(valNo);

        float x = valBlk->getReal("x", 0.f);
        E3DCOLOR color = valBlk->getE3dcolor("color", 0xFFFFFFFF);
        if (x >= t && prevParamOk)
        {
          E3DCOLOR val;
          val.r = (unsigned char)cvt(t, prevX, x, prevColor.r, color.r);
          val.g = (unsigned char)cvt(t, prevX, x, prevColor.g, color.g);
          val.b = (unsigned char)cvt(t, prevX, x, prevColor.b, color.b);
          val.a = (unsigned char)cvt(t, prevX, x, prevColor.a, color.a);
          to.setE3dcolor(blk->getBlockName(), val);
          valOk = true;
          break;
        }
        else
        {
          prevX = x;
          prevColor = color;
          prevParamOk = true;
        }
      }

      if (!valOk && prevParamOk)
        to.setE3dcolor(blk->getBlockName(), prevColor);
    }
    else if (!stricmp(type, "gradient_p4") || !stricmp(type, "gradient_srgb") || !stricmp(type, "gradient_srgb_2x"))
    {
      float prevX = 0.f;
      Point4 prevVal{};
      bool prevParamOk = false;
      bool valOk = false;
      bool srgb = !stricmp(type, "gradient_srgb") || !stricmp(type, "gradient_srgb_2x");
      bool mul2 = !stricmp(type, "gradient_srgb_2x");
      for (unsigned int valNo = 0; valNo < blk->blockCount(); valNo++)
      {
        const DataBlock *valBlk = blk->getBlock(valNo);

        float x = valBlk->getReal("x", 0.f);
        Point4 val = valBlk->getPoint4("val", Point4(0, 0, 0, 0));
        if (srgb)
          val = Point4(powf(val.x, 2.2f), powf(val.y, 2.2f), powf(val.z, 2.2f), val.w);
        if (mul2)
          val = Point4(val.x * 2, val.y * 2, val.z * 2, val.w);
        if (x >= t && prevParamOk)
        {
          val.x = cvt(t, prevX, x, prevVal.x, val.x);
          val.y = cvt(t, prevX, x, prevVal.y, val.y);
          val.z = cvt(t, prevX, x, prevVal.z, val.z);
          val.w = cvt(t, prevX, x, prevVal.w, val.w);
          to.setPoint4(blk->getBlockName(), val);
          valOk = true;
          break;
        }
        else
        {
          prevX = x;
          prevVal = val;
          prevParamOk = true;
        }
      }

      if (!valOk && prevParamOk)
        to.setPoint4(blk->getBlockName(), prevVal);
    }
    else if (!stricmp(type, "tracked_text"))
    {
      float prevX = 0.f;
      const char *prevText = NULL;
      bool prevParamOk = false;
      bool valOk = false;
      for (unsigned int valNo = 0; valNo < blk->blockCount(); valNo++)
      {
        const DataBlock *valBlk = blk->getBlock(valNo);

        float x = valBlk->getReal("x", 0.f);
        const char *text = valBlk->getStr("text", NULL);
        if (x >= t && prevParamOk)
        {
          if (cvt(t, prevX, x, 0.f, 1.f) < 0.5f)
            to.setStr(blk->getBlockName(), prevText);
          else
            to.setStr(blk->getBlockName(), text);

          valOk = true;
          break;
        }
        else
        {
          prevX = x;
          prevText = text;
          prevParamOk = true;
        }
      }

      if (!valOk && prevParamOk)
        to.setStr(blk->getBlockName(), prevText);
    }
    else
    {
      G_ASSERTF(0, "Invalid interpolation type '%s'", type);
    }
  }
}
