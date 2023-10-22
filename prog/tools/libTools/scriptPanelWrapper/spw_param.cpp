// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <sqplus.h>
#include <propPanel2/c_panel_base.h>

#include <scriptPanelWrapper/spw_main.h>
#include <scriptPanelWrapper/spw_param.h>

#include <debug/dag_debug.h>
#include <util/dag_localization.h>

#include <float.h>
#include <limits.h>


//=============================================================================


class SPText : public ScriptPanelParam
{
public:
  SPText(ScriptPanelContainer *parent, const char *name, const char *caption) : ScriptPanelParam(parent, name, caption), mValue("") {}

  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createEditBox(mPid, mCaption, mValue);
  }


  void getValueFromScript(SquirrelObject param)
  {
    if (param.Exists("value"))
    {
      SimpleString value(param.GetString("value"));
      if (stricmp(value, mValue) != 0)
      {
        mValue = value;
        if (mPanel)
          mPanel->setText(mPid, mValue);
      }
    }
    else
    {
      SquirrelObject def = param.GetValue("def");
      mValue = (!def.IsNull() && def.ToString()) ? def.ToString() : "";
      setToScript(param);
    }
  }


  void setToScript(SquirrelObject &param) { param.SetValue("value", mValue); }


  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid)
    {
      mValue = panel.getText(pid);
      setToScript(mParam);
      callChangeScript(true);
    }
  }

  virtual void save(DataBlock &blk)
  {
    if (!paramName.empty())
      blk.setStr(paramName, mValue);
  }

  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {
      mValue = blk.getStr(paramName, mValue);
      if (mPanel)
        mPanel->setText(mPid, mValue);
      setToScript(mParam);
    }
  }

protected:
  SimpleString mValue;
};


//-----------------------------------------------------------------------------

class SPInt : public ScriptPanelParam
{
public:
  SPInt(ScriptPanelContainer *parent, const char *name, const char *caption) :
    ScriptPanelParam(parent, name, caption), mValue(0), mMin(INT_MIN), mMax(INT_MAX), mStep(1)
  {}

  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createEditInt(mPid, mCaption, mValue);
    mPanel->setMinMaxStep(mPid, mMin, mMax, mStep);
  }

  void getValueFromScript(SquirrelObject param)
  {
    mMin = (param.Exists("min")) ? param.GetInt("min") : mMin;
    mMax = (param.Exists("max")) ? param.GetInt("max") : mMax;
    mStep = (param.Exists("step")) ? param.GetInt("step") : mStep;
    if (mPanel)
      mPanel->setMinMaxStep(mPid, mMin, mMax, mStep);

    if (param.Exists("value"))
    {
      int value = param.GetInt("value");
      if (value != mValue)
      {
        mValue = value;
        if (mPanel)
          mPanel->setInt(mPid, mValue);
      }
    }
    else
    {
      SquirrelObject def = param.GetValue("def");
      mValue = (!def.IsNull() && def.ToInteger()) ? def.ToInteger() : 0;
      setToScript(param);
    }
  }

  void setToScript(SquirrelObject &param)
  {
    param.SetValue("value", mValue);
    param.SetValue("min", mMin);
    param.SetValue("max", mMax);
    param.SetValue("step", mStep);
  }


  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid)
    {
      mValue = panel.getInt(pid);
      setToScript(mParam);
      callChangeScript(true);
    }
  }

  virtual void save(DataBlock &blk)
  {
    if (!paramName.empty())
      blk.setInt(paramName, mValue);
  }

  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {
      mValue = blk.getInt(paramName, mValue);
      if (mPanel)
        mPanel->setInt(mPid, mValue);
      setToScript(mParam);
    }
  }


protected:
  int mValue;
  int mMin, mMax, mStep;
};


//-----------------------------------------------------------------------------


class SPReal : public ScriptPanelParam
{
public:
  SPReal(ScriptPanelContainer *parent, const char *name, const char *caption) :
    ScriptPanelParam(parent, name, caption), mValue(0), mMin(-FLT_MAX), mMax(FLT_MAX), mStep(0.01)
  {}


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createEditFloat(mPid, mCaption, mValue);
    mPanel->setMinMaxStep(mPid, mMin, mMax, mStep);
  }

  void getValueFromScript(SquirrelObject param)
  {
    mMin = (param.Exists("min")) ? param.GetFloat("min") : mMin;
    mMax = (param.Exists("max")) ? param.GetFloat("max") : mMax;
    mStep = (param.Exists("step")) ? param.GetFloat("step") : mStep;
    if (mPanel)
      mPanel->setMinMaxStep(mPid, mMin, mMax, mStep);

    if (param.Exists("value"))
    {
      float value = param.GetFloat("value");
      if (value != mValue)
      {
        mValue = value;
        if (mPanel)
          mPanel->setFloat(mPid, mValue);
      }
    }
    else
    {
      SquirrelObject def = param.GetValue("def");
      mValue = (!def.IsNull() && def.ToFloat()) ? def.ToFloat() : 0;
      setToScript(param);
    }
  }

  void setToScript(SquirrelObject &param)
  {
    param.SetValue("value", mValue);
    param.SetValue("min", mMin);
    param.SetValue("max", mMax);
    param.SetValue("step", mStep);
  }


  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid)
    {
      mValue = panel.getFloat(pid);
      setToScript(mParam);
      callChangeScript(true);
    }
  }

  virtual void save(DataBlock &blk)
  {
    if (!paramName.empty())
      blk.setReal(paramName, mValue);
  }

  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {
      mValue = blk.getReal(paramName, mValue);
      if (mPanel)
        mPanel->setFloat(mPid, mValue);
      setToScript(mParam);
    }
  }

protected:
  float mValue;
  float mMin, mMax, mStep;
};


//-----------------------------------------------------------------------------


class SPBool : public ScriptPanelParam
{
public:
  SPBool(ScriptPanelContainer *parent, const char *name, const char *caption) : ScriptPanelParam(parent, name, caption), mValue(false)
  {}


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createCheckBox(mPid, mCaption, mValue);
  }

  void getValueFromScript(SquirrelObject param)
  {
    if (param.Exists("value"))
    {
      bool value = param.GetBool("value");
      if (value != mValue)
      {
        mValue = value;
        if (mPanel)
          mPanel->setBool(mPid, mValue);
      }
    }
    else
    {
      SquirrelObject def = param.GetValue("def");
      mValue = (!def.IsNull() && def.ToBool()) ? def.ToBool() : false;
      setToScript(param);
    }
  }

  void setToScript(SquirrelObject &param) { param.SetValue("value", mValue); }


  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid)
    {
      mValue = panel.getBool(pid);
      setToScript(mParam);
      callChangeScript(true);
    }
  }

  virtual void save(DataBlock &blk)
  {
    if (!paramName.empty())
      blk.setBool(paramName, mValue);
  }

  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {
      mValue = blk.getBool(paramName, mValue);
      if (mPanel)
        mPanel->setBool(mPid, mValue);
      setToScript(mParam);
    }
  }

private:
  bool mValue;
};


//-----------------------------------------------------------------------------


class SPColor : public ScriptPanelParam
{
public:
  SPColor(ScriptPanelContainer *parent, const char *name, const char *caption) :
    ScriptPanelParam(parent, name, caption), mValue(E3DCOLOR(0, 0, 0, 0))
  {}


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createSimpleColor(mPid, mCaption, mValue);
  }


  E3DCOLOR getColorFromSQ(SquirrelObject &param, const char param_name[])
  {
    SquirrelObject val = param.GetValue(param_name);
    if (!val.IsNull() && val.GetType() == OT_ARRAY && val.Len() == 4)
      return E3DCOLOR(val.GetInt(SQInteger(0)), val.GetInt(1), val.GetInt(2), val.GetInt(3));
    return E3DCOLOR(0, 0, 0, 0);
  }


  void getValueFromScript(SquirrelObject param)
  {
    if (param.Exists("value"))
    {
      E3DCOLOR value = getColorFromSQ(param, "value");

      if (value != mValue)
      {
        mValue = value;
        if (mPanel)
          mPanel->setColor(mPid, mValue);
      }
    }
    else
    {
      mValue = getColorFromSQ(param, "def");
      setToScript(param);
    }
  }

  void setToScript(SquirrelObject &param)
  {
    if (param.GetValue("value").IsNull())
    {
      param.SetValue("value", SquirrelVM::CreateArray(4));
    }

    SquirrelObject val = param.GetValue("value");
    val.SetValue(SQInteger(0), mValue.r);
    val.SetValue(1, mValue.g);
    val.SetValue(2, mValue.b);
    val.SetValue(3, mValue.a);
  }


  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid)
    {
      mValue = panel.getColor(pid);
      setToScript(mParam);
      callChangeScript(true);
    }
  }

  virtual void save(DataBlock &blk)
  {
    if (!paramName.empty())
      blk.setE3dcolor(paramName, mValue);
  }

  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {
      mValue = blk.getE3dcolor(paramName, mValue);
      if (mPanel)
        mPanel->setColor(mPid, mValue);
      setToScript(mParam);
    }
  }

private:
  E3DCOLOR mValue;
};


//-----------------------------------------------------------------------------


class SPRangedInt : public SPInt
{
public:
  SPRangedInt(ScriptPanelContainer *parent, const char *name, const char *caption) : SPInt(parent, name, caption) {}


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createTrackInt(mPid, mCaption, mValue, mMin, mMax, mStep);
  }
};


//-----------------------------------------------------------------------------


class SPRangedReal : public SPReal
{
public:
  SPRangedReal(ScriptPanelContainer *parent, const char *name, const char *caption) : SPReal(parent, name, caption) {}

  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createTrackFloat(mPid, mCaption, mValue, mMin, mMax, mStep);
  }
};


//-----------------------------------------------------------------------------


class SPPoint2 : public ScriptPanelParam
{
public:
  SPPoint2(ScriptPanelContainer *parent, const char *name, const char *caption) :
    ScriptPanelParam(parent, name, caption), mValue(Point2(0, 0))
  {}


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createPoint2(mPid, mCaption, mValue);
  }


  Point2 getPoint2FromSQ(SquirrelObject &param, const char param_name[])
  {
    SquirrelObject val = param.GetValue(param_name);
    if (!val.IsNull() && val.GetType() == OT_ARRAY && val.Len() == 2)
      return Point2(val.GetFloat(SQInteger(0)), val.GetFloat(1));
    return Point2(0, 0);
  }

  void getValueFromScript(SquirrelObject param)
  {
    if (param.Exists("value"))
    {
      Point2 value = getPoint2FromSQ(param, "value");

      if (value != mValue)
      {
        mValue = value;
        if (mPanel)
          mPanel->setPoint2(mPid, mValue);
      }
    }
    else
    {
      mValue = getPoint2FromSQ(param, "def");
      setToScript(param);
    }
  }

  void setToScript(SquirrelObject &param)
  {
    if (param.GetValue("value").IsNull())
    {
      param.SetValue("value", SquirrelVM::CreateArray(2));
    }

    SquirrelObject val = param.GetValue("value");
    val.SetValue(SQInteger(0), mValue.x);
    val.SetValue(1, mValue.y);
  }


  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid)
    {
      mValue = panel.getPoint2(pid);
      setToScript(mParam);
      callChangeScript(true);
    }
  }

  virtual void save(DataBlock &blk)
  {
    if (!paramName.empty())
      blk.setPoint2(paramName, mValue);
  }

  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {
      mValue = blk.getPoint2(paramName, mValue);
      if (mPanel)
        mPanel->setPoint2(mPid, mValue);
      setToScript(mParam);
    }
  }

private:
  Point2 mValue;
};


//-----------------------------------------------------------------------------


class SPPoint3 : public ScriptPanelParam
{
public:
  SPPoint3(ScriptPanelContainer *parent, const char *name, const char *caption) :
    ScriptPanelParam(parent, name, caption), mValue(Point3(0, 0, 0))
  {}


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createPoint3(mPid, mCaption, mValue);
  }

  Point3 getPoint3FromSQ(SquirrelObject &param, const char param_name[])
  {
    SquirrelObject val = param.GetValue(param_name);
    if (!val.IsNull() && val.GetType() == OT_ARRAY && val.Len() == 3)
      return Point3(val.GetFloat(SQInteger(0)), val.GetFloat(1), val.GetFloat(2));
    return Point3(0, 0, 0);
  }

  void getValueFromScript(SquirrelObject param)
  {
    if (param.Exists("value"))
    {
      Point3 value = getPoint3FromSQ(param, "value");
      if (value != mValue)
      {
        mValue = value;
        if (mPanel)
          mPanel->setPoint3(mPid, mValue);
      }
    }
    else
    {
      mValue = getPoint3FromSQ(param, "def");
      setToScript(param);
    }
  }

  void setToScript(SquirrelObject &param)
  {
    if (param.GetValue("value").IsNull())
    {
      param.SetValue("value", SquirrelVM::CreateArray(3));
    }

    SquirrelObject val = param.GetValue("value");
    val.SetValue(SQInteger(0), mValue.x);
    val.SetValue(1, mValue.y);
    val.SetValue(2, mValue.z);
  }


  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid)
    {
      mValue = panel.getPoint3(pid);
      setToScript(mParam);
      callChangeScript(true);
    }
  }

  virtual void save(DataBlock &blk)
  {
    if (!paramName.empty())
      blk.setPoint3(paramName, mValue);
  }

  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {
      mValue = blk.getPoint3(paramName, mValue);
      if (mPanel)
        mPanel->setPoint3(mPid, mValue);
      setToScript(mParam);
    }
  }

private:
  Point3 mValue;
};


//-----------------------------------------------------------------------------


class SPPoint4 : public ScriptPanelParam
{
public:
  SPPoint4(ScriptPanelContainer *parent, const char *name, const char *caption) :
    ScriptPanelParam(parent, name, caption), mValue(Point4(0, 0, 0, 0))
  {}


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createPoint4(mPid, mCaption, mValue);
  }


  Point4 getPoint4FromSQ(SquirrelObject &param, const char param_name[])
  {
    SquirrelObject val = param.GetValue(param_name);
    if (!val.IsNull() && val.GetType() == OT_ARRAY && val.Len() == 4)
      return Point4(val.GetFloat(SQInteger(0)), val.GetFloat(1), val.GetFloat(2), val.GetFloat(3));
    return Point4(0, 0, 0, 0);
  }

  void getValueFromScript(SquirrelObject param)
  {
    if (param.Exists("value"))
    {
      Point4 value = getPoint4FromSQ(param, "value");
      if (value != mValue)
      {
        mValue = value;
        if (mPanel)
          mPanel->setPoint4(mPid, mValue);
      }
    }
    else
    {
      mValue = getPoint4FromSQ(param, "def");
      setToScript(param);
    }
  }

  void setToScript(SquirrelObject &param)
  {
    if (param.GetValue("value").IsNull())
    {
      param.SetValue("value", SquirrelVM::CreateArray(4));
    }

    SquirrelObject val = param.GetValue("value");
    val.SetValue(SQInteger(0), mValue.x);
    val.SetValue(1, mValue.y);
    val.SetValue(2, mValue.z);
    val.SetValue(3, mValue.w);
  }


  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid)
    {
      mValue = panel.getPoint4(pid);
      setToScript(mParam);
      callChangeScript(true);
    }
  }

  virtual void save(DataBlock &blk)
  {
    if (!paramName.empty())
      blk.setPoint4(paramName, mValue);
  }

  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {
      mValue = blk.getPoint4(paramName, mValue);
      if (mPanel)
        mPanel->setPoint4(mPid, mValue);
      setToScript(mParam);
    }
  }

private:
  Point4 mValue;
};


//-----------------------------------------------------------------------------


class SPIPoint2 : public ScriptPanelParam
{
public:
  SPIPoint2(ScriptPanelContainer *parent, const char *name, const char *caption) :
    ScriptPanelParam(parent, name, caption), mValue(IPoint2(0, 0))
  {}


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createPoint2(mPid, mCaption, mValue);
    mPanel->setPrec(mPid, 1);
  }


  IPoint2 getIPoint2FromSQ(SquirrelObject &param, const char param_name[])
  {
    SquirrelObject val = param.GetValue(param_name);
    if (!val.IsNull() && val.GetType() == OT_ARRAY && val.Len() == 2)
      return IPoint2(val.GetInt(SQInteger(0)), val.GetInt(1));
    return IPoint2(0, 0);
  }


  void getValueFromScript(SquirrelObject param)
  {
    if (param.Exists("value"))
    {
      IPoint2 value = getIPoint2FromSQ(param, "value");
      if (value != mValue)
      {
        mValue = value;
        if (mPanel)
          mPanel->setPoint2(mPid, mValue);
      }
    }
    else
    {
      mValue = getIPoint2FromSQ(param, "def");
      setToScript(param);
    }
  }

  void setToScript(SquirrelObject &param)
  {
    if (param.GetValue("value").IsNull())
    {
      param.SetValue("value", SquirrelVM::CreateArray(2));
    }

    SquirrelObject val = param.GetValue("value");
    val.SetValue(SQInteger(0), mValue.x);
    val.SetValue(1, mValue.y);
  }


  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid)
    {
      Point2 val = panel.getPoint2(pid);
      mValue = IPoint2(val.x, val.y);
      setToScript(mParam);
      callChangeScript(true);
    }
  }

  virtual void save(DataBlock &blk)
  {
    if (!paramName.empty())
      blk.setIPoint2(paramName, mValue);
  }

  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {
      mValue = blk.getIPoint2(paramName, mValue);
      if (mPanel)
        mPanel->setPoint2(mPid, mValue);
      setToScript(mParam);
    }
  }


private:
  IPoint2 mValue;
};


//-----------------------------------------------------------------------------


class SPIPoint3 : public ScriptPanelParam
{
public:
  SPIPoint3(ScriptPanelContainer *parent, const char *name, const char *caption) :
    ScriptPanelParam(parent, name, caption), mValue(IPoint3(0, 0, 0))
  {}


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createPoint3(mPid, mCaption, mValue);
    mPanel->setPrec(mPid, 1);
  }


  IPoint3 getIPoint3FromSQ(SquirrelObject &param, const char param_name[])
  {
    SquirrelObject val = param.GetValue(param_name);
    if (!val.IsNull() && val.GetType() == OT_ARRAY && val.Len() == 3)
      return IPoint3(val.GetInt(SQInteger(0)), val.GetInt(1), val.GetInt(2));
    return IPoint3(0, 0, 0);
  }


  void getValueFromScript(SquirrelObject param)
  {
    if (param.Exists("value"))
    {
      IPoint3 value = getIPoint3FromSQ(param, "value");
      if (value != mValue)
      {
        mValue = value;
        if (mPanel)
          mPanel->setPoint3(mPid, mValue);
      }
    }
    else
    {
      mValue = getIPoint3FromSQ(param, "def");
      setToScript(param);
    }
  }

  void setToScript(SquirrelObject &param)
  {
    if (param.GetValue("value").IsNull())
    {
      param.SetValue("value", SquirrelVM::CreateArray(3));
    }

    SquirrelObject val = param.GetValue("value");
    val.SetValue(SQInteger(0), mValue.x);
    val.SetValue(1, mValue.y);
    val.SetValue(2, mValue.z);
  }


  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid)
    {
      Point3 val = panel.getPoint3(pid);
      mValue = IPoint3(val.x, val.y, val.z);
      setToScript(mParam);
      callChangeScript(true);
    }
  }

  virtual void save(DataBlock &blk)
  {
    if (!paramName.empty())
      blk.setIPoint3(paramName, mValue);
  }

  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {
      mValue = blk.getIPoint3(paramName, mValue);
      if (mPanel)
        mPanel->setPoint3(mPid, mValue);
      setToScript(mParam);
    }
  }


private:
  IPoint3 mValue;
};

//-----------------------------------------------------------------------------


class SPMatrix : public ScriptPanelParam
{
public:
  SPMatrix(ScriptPanelContainer *parent, const char *name, const char *caption) :
    ScriptPanelParam(parent, name, caption), mValue(TMatrix::IDENT)
  {}

  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createMatrix(mPid, mCaption, mValue);
    mPanel->setPrec(mPid, 2);
  }

  TMatrix getMatrixFromSQ(SquirrelObject &param, const char param_name[])
  {
    TMatrix val(TMatrix::IDENT);
    SquirrelObject matrix = param.GetValue(param_name);
    if (!matrix.IsNull() && matrix.GetType() == OT_ARRAY && matrix.Len() == 4)
      for (int i = 0; i < matrix.Len(); ++i)
      {
        SquirrelObject col = matrix.GetValue(i);
        if (!col.IsNull() && col.GetType() == OT_ARRAY && col.Len() == 3)
          for (int j = 0; j < col.Len(); ++j)
            val.m[i][j] = col.GetFloat(j);
      }
    return val;
  }

  void getValueFromScript(SquirrelObject param)
  {
    if (param.Exists("value"))
    {
      TMatrix value = getMatrixFromSQ(param, "value");
      if (value != mValue)
      {
        mValue = value;
        if (mPanel)
          mPanel->setMatrix(mPid, mValue);
      }
    }
    else
    {
      mValue = getMatrixFromSQ(param, "def");
      setToScript(param);
    }
  }

  void setToScript(SquirrelObject &param)
  {
    if (param.GetValue("value").IsNull())
    {
      param.SetValue("value", SquirrelVM::CreateArray(4));
      SquirrelObject matrix = param.GetValue("value");
      for (int i = 0; i < 4; ++i)
        matrix.SetValue(i, SquirrelVM::CreateArray(3));
    }

    SquirrelObject matrix = param.GetValue("value");
    if (!matrix.IsNull() && matrix.GetType() == OT_ARRAY && matrix.Len() == 4)
      for (int i = 0; i < matrix.Len(); ++i)
      {
        SquirrelObject col = matrix.GetValue(i);
        if (!col.IsNull() && col.GetType() == OT_ARRAY && col.Len() == 3)
          for (int j = 0; j < col.Len(); ++j)
            col.SetValue(j, mValue.m[i][j]);
      }
  }

  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid)
    {
      mValue = panel.getMatrix(pid);
      setToScript(mParam);
      callChangeScript(true);
    }
  }

  virtual void save(DataBlock &blk)
  {
    if (!paramName.empty())
      blk.setTm(paramName, mValue);
  }

  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {
      mValue = blk.getTm(paramName, mValue);
      if (mPanel)
        mPanel->setMatrix(mPid, mValue);
      setToScript(mParam);
    }
  }

private:
  TMatrix mValue;
};


//-----------------------------------------------------------------------------


class SPTargetFile : public SPText
{
public:
  SPTargetFile(ScriptPanelContainer *parent, const char *name, const char *caption) : SPText(parent, name, caption), mFilter(midmem) {}


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createFileEditBox(mPid, mCaption, mValue);
    mPanel->setStrings(mPid, mFilter);
    mPanel->setInt(mPid, FS_DIALOG_OPEN_FILE);
    mPanel->setUserData(mPid, mBaseFolder.str());
  }

  void getValueFromScript(SquirrelObject param)
  {
    clear_and_shrink(mFilter);
    SquirrelObject options = param.GetValue("options");
    if (!options.IsNull() && options.GetType() == OT_ARRAY)
      for (int i = 0; i < options.Len(); ++i)
        if (options.GetString(i))
          mFilter.push_back() = options.GetString(i);
    if (mPanel)
      mPanel->setStrings(mPid, mFilter);

    SquirrelObject folder = param.GetValue("folder");
    if (!folder.IsNull())
    {
      mBaseFolder = folder.ToString();
      if (mPanel)
        mPanel->setUserData(mPid, mBaseFolder.str());
    }

    SPText::getValueFromScript(param);
  }

private:
  Tab<String> mFilter;
  SimpleString mBaseFolder;
};


//-----------------------------------------------------------------------------


class SPTargetFolder : public SPText
{
public:
  SPTargetFolder(ScriptPanelContainer *parent, const char *name, const char *caption) : SPText(parent, name, caption) {}


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createFileEditBox(mPid, mCaption, mValue);
    mPanel->setInt(mPid, FS_DIALOG_DIRECTORY);
    mPanel->setUserData(mPid, mBaseFolder.str());
  }


  void getValueFromScript(SquirrelObject param)
  {
    SquirrelObject folder = param.GetValue("folder");
    if (!folder.IsNull())
    {
      mBaseFolder = folder.ToString();
      if (mPanel)
        mPanel->setUserData(mPid, mBaseFolder.str());
    }

    SPText::getValueFromScript(param);
  }

private:
  SimpleString mBaseFolder;
};


//-----------------------------------------------------------------------------


class SPEnumText : public SPText
{
public:
  SPEnumText(ScriptPanelContainer *parent, const char *name, const char *caption) : SPText(parent, name, caption), mVals(midmem) {}


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createCombo(mPid, mCaption, mVals, mValue);
  }

  void getValueFromScript(SquirrelObject param)
  {
    if (mPanel)
    {
      clear_and_shrink(mVals);

      SquirrelObject options = param.GetValue("options");
      if (!options.IsNull() && options.GetType() == OT_ARRAY)
        for (int i = 0; i < options.Len(); ++i)
          if (options.GetString(i))
            mVals.push_back() = options.GetString(i);
      mPanel->setStrings(mPid, mVals);
    }

    SPText::getValueFromScript(param);
    if (mPanel)
      mPanel->setText(mPid, mValue);
  }


protected:
  Tab<String> mVals;
};


//-----------------------------------------------------------------------------


class SPEnumInt : public SPInt
{
public:
  SPEnumInt(ScriptPanelContainer *parent, const char *name, const char *caption) : SPInt(parent, name, caption), mVals(midmem) {}


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createCombo(mPid, mCaption, mVals, String(32, "%d", mValue));
  }

  void getValueFromScript(SquirrelObject param)
  {
    if (mPanel)
    {
      clear_and_shrink(mVals);

      SquirrelObject options = param.GetValue("options");
      if (!options.IsNull() && options.GetType() == OT_ARRAY)
        for (int i = 0; i < options.Len(); ++i)
          mVals.push_back(String(32, "%d", options.GetInt(i)));
      mPanel->setStrings(mPid, mVals);
    }

    SPInt::getValueFromScript(param);
    if (mPanel)
      mPanel->setText(mPid, String(32, "%d", mValue));
  }


  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid)
    {
      mValue = atoi(panel.getText(pid));
      setToScript(mParam);
      callChangeScript(true);
    }
  }


  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {
      mValue = blk.getInt(paramName, mValue);
      if (mPanel)
        mPanel->setText(mPid, String(32, "%d", mValue));
      setToScript(mParam);
    }
  }


protected:
  Tab<String> mVals;
};

//-----------------------------------------------------------------------------


class SPEnumReal : public SPReal
{
public:
  SPEnumReal(ScriptPanelContainer *parent, const char *name, const char *caption) : SPReal(parent, name, caption), mVals(midmem) {}

  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createCombo(mPid, mCaption, mVals, formatFloat(mValue));
  }

  void getValueFromScript(SquirrelObject param)
  {
    if (mPanel)
    {
      clear_and_shrink(mVals);

      SquirrelObject options = param.GetValue("options");
      if (!options.IsNull() && options.GetType() == OT_ARRAY)
        for (int i = 0; i < options.Len(); ++i)
          mVals.push_back(formatFloat(options.GetFloat(i)));
      mPanel->setStrings(mPid, mVals);
    }

    SPReal::getValueFromScript(param);
    if (mPanel)
      mPanel->setText(mPid, formatFloat(mValue));
  }


  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid)
    {
      mValue = atof(panel.getText(pid));
      setToScript(mParam);
      callChangeScript(true);
    }
  }


  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {
      mValue = blk.getReal(paramName, mValue);
      if (mPanel)
        mPanel->setText(mPid, formatFloat(mValue));
      setToScript(mParam);
    }
  }


  String formatFloat(float value)
  {
    String buf(32, "%f", value);
    char *ptr = buf.str() + buf.length() - 1;
    if (!buf.empty() && strchr(buf.str(), '.'))
      while (ptr > buf.str() && (*ptr == '0' || *ptr == '.'))
      {
        *ptr = 0;
        if (*ptr == '.')
          break;
        --ptr;
      }

    return buf;
  }


protected:
  Tab<String> mVals;
};


//-----------------------------------------------------------------------------


class SPTarget : public SPText
{
public:
  SPTarget(ScriptPanelContainer *parent, const char *name, const char *caption) :
    SPText(parent, name, caption), mCB(NULL), objType(""), objFilter("")
  {}


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createTargetButton(mPid, mCaption, mValue);
  }


  void getValueFromScript(SquirrelObject param)
  {
    if (param.Exists("value"))
    {
      SimpleString value(param.GetString("value"));
      if (stricmp(value, mValue) != 0)
      {
        mValue = value;
        if (mPanel)
          mPanel->setText(mPid, mValue);
      }
    }
    else
    {
      SquirrelObject def = param.GetValue("def");
      mValue = (!def.IsNull() && def.ToString()) ? def.ToString() : "";
      setToScript(param);
    }

    if (objType.empty())
    {
      if (param.Exists("target_type"))
        objType = param.GetString("target_type");
      else
        objType = (stricmp(mParent->getParamType(param), "target_asset") == 0) ? "asset" : "default";
    }

    if (param.Exists("filter"))
      objFilter = param.GetString("filter");
  }


  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid && mCB)
    {
      const char *choise = mCB->getTarget(mValue, objType, objFilter);
      mValue = (choise) ? choise : "";
      mPanel->setText(mPid, mValue);
      setToScript(mParam);
      callChangeScript(true);
    }
    else if (!mCB)
      logerr("Panel script error: SPTarget callback is empty [%s]", mCaption);
  }

  virtual bool onMessage(int pid, int msg, void *arg)
  {
    if (pid == 0 || pid == mPid)
    {
      if (msg == MSG_SET_TEXT)
      {
        const char *choise = (const char *)arg;
        mValue = (choise) ? choise : "";
        mPanel->setText(mPid, mValue);
        setToScript(mParam);
        callChangeScript(true);
        return true;
      }
    }
    return false;
  }

  virtual void onClick(int pid, PropPanel2 &panel) {}

  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {
      mValue = blk.getStr(paramName, mValue);
      if (mPanel)
        mPanel->setText(mPid, mValue);
      setToScript(mParam);
    }
  }

  void setCB(IScriptPanelTargetCB *cb) { mCB = cb; }

  void validate()
  {
    if (mCB)
    {
      if (const char *new_name = mCB->validateTarget(mValue.str(), objType))
      {
        if (new_name == mValue.str())
          return;
        else
          mValue = new_name;
      }
      else if (clearInvalid())
        mValue = "";

      if (mPanel)
        mPanel->setText(mPid, mValue);
      setToScript(mParam);
    }
  }

  virtual void getTargetList(Tab<SimpleString> &list) { list.push_back(mValue); }

protected:
  bool clearInvalid() { return true; }

  IScriptPanelTargetCB *mCB;
  SimpleString objType, objFilter;
};


//-----------------------------------------------------------------------------

class SPCustomTarget : public SPTarget
{
public:
  SPCustomTarget(ScriptPanelContainer *parent, const char *name, const char *caption) : SPTarget(parent, name, caption) {}

  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createFileEditBox(mPid, mCaption, mValue);
    mPanel->setInt(mPid, FS_DIALOG_NONE);
  }

  virtual void onChange(int pid, PropPanel2 &panel) { SPText::onChange(pid, panel); }

  virtual void onClick(int pid, PropPanel2 &panel)
  {
    if (pid == mPid && mCB)
    {
      const char *choise = mCB->getTarget(mValue, objType, objFilter);
      mValue = (choise) ? choise : "";
      mPanel->setText(mPid, mValue);
    }
    else if (!mCB)
      logerr("Panel script error: SPCustomTarget callback is empty [%s]", mCaption);
  }

protected:
  bool clearInvalid() { return false; }
};


//-----------------------------------------------------------------------------


class SPGradient : public ScriptPanelParam
{
public:
  SPGradient(ScriptPanelContainer *parent, const char *name, const char *caption) :
    ScriptPanelParam(parent, name, caption), mValue(midmem), mMin(2), mMax(100)
  {
    mValue.push_back(GradientKey(0, E3DCOLOR(0, 0, 0, 255)));
    mValue.push_back(GradientKey(1, E3DCOLOR(255, 255, 255, 255)));
  }


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createGradientBox(mPid, mCaption);
    mPanel->setGradient(mPid, &mValue);
    mPanel->setMinMaxStep(mPid, mMin, mMax, 1);
  }


  void getGradientFromSQ(PGradient value, SquirrelObject &param, const char param_name[])
  {
    clear_and_shrink(*value);
    SquirrelObject val = param.GetValue(param_name);
    if (!val.IsNull() && val.GetType() == OT_ARRAY && val.BeginIteration())
    {
      SquirrelObject gr_point, key;

      while (val.Next(key, gr_point))
        if (gr_point.GetType() == OT_TABLE)
        {
          float x = (gr_point.Exists("x")) ? gr_point.GetFloat("x") : -1;
          E3DCOLOR color = E3DCOLOR(0, 0, 0, 0);

          SquirrelObject col = gr_point.GetValue("color");
          if (!col.IsNull() && col.GetType() == OT_ARRAY && col.Len() == 4)
            color = E3DCOLOR(col.GetInt(SQInteger(0)), col.GetInt(1), col.GetInt(2), col.GetInt(3));

          value->push_back(GradientKey(x, color));
        }

      val.EndIteration();
    }
  }


  void getValueFromScript(SquirrelObject param)
  {
    int min = (param.Exists("min")) ? param.GetInt("min") : mMin;
    int max = (param.Exists("max")) ? param.GetInt("max") : mMax;
    if (min != mMin || max != mMax)
    {
      mMin = (min < 2) ? 2 : min;
      mMax = (max >= mMin) ? max : mMin;
      if (mPanel)
        mPanel->setMinMaxStep(mPid, mMin, mMax, 1);
    }

    if (param.Exists("mark") && mPanel)
      mPanel->setFloat(mPid, param.GetFloat("mark"));

    if (param.Exists("value"))
    {
      Gradient value(tmpmem);
      getGradientFromSQ(&value, param, "value");

      bool need_update = false;
      if (value.size() != mValue.size())
        need_update = true;
      for (int i = 0; i < value.size(); ++i)
        if (value[i].position != mValue[i].position || value[i].color != mValue[i].color)
        {
          need_update = true;
          break;
        }

      if (need_update)
      {
        mValue = value;
        if (mPanel)
          mPanel->setGradient(mPid, &mValue);
      }
    }
    else
    {
      if (param.Exists("def"))
        getGradientFromSQ(&mValue, param, "def");

      setToScript(param);
    }
  }


  void setToScript(SquirrelObject &param)
  {
    param.SetValue("min", mMin);
    param.SetValue("max", mMax);

    SquirrelObject vals, val, col;

    param.SetValue("value", SquirrelVM::CreateArray(mValue.size()));
    vals = param.GetValue("value");

    for (int i = 0; i < mValue.size(); ++i)
    {
      vals.SetValue(i, SquirrelVM::CreateTable());
      val = vals.GetValue(i);
      val.SetValue("x", mValue[i].position);
      val.SetValue("color", SquirrelVM::CreateArray(4));
      col = val.GetValue("color");
      col.SetValue(SQInteger(0), mValue[i].color.r);
      col.SetValue(1, mValue[i].color.g);
      col.SetValue(2, mValue[i].color.b);
      col.SetValue(3, mValue[i].color.a);
    }
  }


  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid)
    {
      panel.getGradient(pid, &mValue);
      setToScript(mParam);
      callChangeScript(true);
    }
  }

  virtual void save(DataBlock &blk)
  {
    if (!paramName.empty())
    {
      DataBlock *grad_blk = (paramName.empty()) ? &blk : blk.addBlock(paramName);
      if (grad_blk)
      {
        grad_blk->setStr("type", "gradient");
        grad_blk->removeBlock("value");
        for (int i = 0; i < mValue.size(); ++i)
        {
          DataBlock *val_blk = grad_blk->addNewBlock("value");
          val_blk->setReal("x", mValue[i].position);
          val_blk->setE3dcolor("color", mValue[i].color);
        }
      }
    }
  }

  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {
      const DataBlock *grad_blk = blk.getBlockByName(paramName);
      if (grad_blk && (stricmp("gradient", grad_blk->getStr("type", "")) == 0))
      {
        int val_name_id = grad_blk->getNameId("value");
        if (val_name_id == -1)
          return;

        clear_and_shrink(mValue);
        for (int i = 0; i < grad_blk->blockCount(); ++i)
        {
          const DataBlock *val_blk = grad_blk->getBlock(i);
          if (val_blk->getBlockNameId() != val_name_id)
            continue;
          mValue.push_back(GradientKey(val_blk->getReal("x", 0), val_blk->getE3dcolor("color", 0)));
        }
      }

      if (mPanel)
        mPanel->setGradient(mPid, &mValue);
      setToScript(mParam);
    }
  }

private:
  Gradient mValue;
  int mMin, mMax;
};


//-----------------------------------------------------------------------------

class SPTextGradient : public ScriptPanelParam
{
public:
  SPTextGradient(ScriptPanelContainer *parent, const char *name, const char *caption) :
    ScriptPanelParam(parent, name, caption), mValue(midmem), mMin(2), mMax(100), mCB(NULL), objType(""), objFilter(""), isTarget(false)
  {
    mValue.push_back(TextGradientKey(0, "start"));
    mValue.push_back(TextGradientKey(1, "finish"));
  }


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createTextGradient(mPid, mCaption);
    mPanel->setTextGradient(mPid, mValue);
    mPanel->setMinMaxStep(mPid, mMin, mMax, 1);
  }


  void getGradientFromSQ(TextGradient &value, SquirrelObject &param, const char param_name[])
  {
    clear_and_shrink(value);
    SquirrelObject val = param.GetValue(param_name);
    if (!val.IsNull() && val.GetType() == OT_ARRAY && val.BeginIteration())
    {
      SquirrelObject gr_point, key;
      while (val.Next(key, gr_point))
        if (gr_point.GetType() == OT_TABLE)
        {
          float x = (gr_point.Exists("x")) ? gr_point.GetFloat("x") : -1;
          SimpleString text(gr_point.Exists("text") ? gr_point.GetString("text") : "");
          value.push_back(TextGradientKey(x, text.str()));
        }
      val.EndIteration();
    }
  }


  void getValueFromScript(SquirrelObject param)
  {
    int min = (param.Exists("min")) ? param.GetInt("min") : mMin;
    int max = (param.Exists("max")) ? param.GetInt("max") : mMax;
    if (min != mMin || max != mMax)
    {
      mMin = (min < 2) ? 2 : min;
      mMax = (max >= mMin) ? max : mMin;
      if (mPanel)
        mPanel->setMinMaxStep(mPid, mMin, mMax, 1);
    }

    if (param.Exists("mark") && mPanel)
      mPanel->setFloat(mPid, param.GetFloat("mark"));

    if (param.Exists("value"))
    {
      TextGradient value(tmpmem);
      getGradientFromSQ(value, param, "value");

      bool need_update = false;
      if (value.size() != mValue.size())
        need_update = true;
      for (int i = 0; i < value.size(); ++i)
        if (value[i].position != mValue[i].position || strcmp(value[i].text.str(), mValue[i].text.str()) != 0)
        {
          need_update = true;
          break;
        }

      if (need_update)
      {
        mValue = value;
        if (mPanel)
          mPanel->setTextGradient(mPid, mValue);
      }
    }
    else
    {
      if (param.Exists("def"))
        getGradientFromSQ(mValue, param, "def");

      setToScript(param);
    }

    if (objType.empty())
    {
      if (param.Exists("target_type"))
      {
        objType = param.GetString("target_type");
        isTarget = true;
      }
      else
      {
        objType = "default";
        isTarget = false;
      }
    }

    if (param.Exists("filter"))
      objFilter = param.GetString("filter");
  }


  void setToScript(SquirrelObject &param)
  {
    param.SetValue("min", mMin);
    param.SetValue("max", mMax);

    SquirrelObject vals, val, col;

    param.SetValue("value", SquirrelVM::CreateArray(mValue.size()));
    vals = param.GetValue("value");

    for (int i = 0; i < mValue.size(); ++i)
    {
      vals.SetValue(i, SquirrelVM::CreateTable());
      val = vals.GetValue(i);
      val.SetValue("x", mValue[i].position);
      val.SetValue("text", mValue[i].text.str());
    }
  }

  long onChanging(int pid, PropPanel2 &panel)
  {
    if (isTarget)
    {
      if (mCB)
      {
        SimpleString old_choise = mPanel->getText(mPid);
        const char *choise = mCB->getTarget(old_choise.str(), objType, objFilter);
        mPanel->setText(mPid, (choise) ? choise : "");
        return 1;
      }
      else
        logerr("Panel script error: SPTarget callback is empty [%s]", mCaption);
    }
    return 0;
  }


  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid)
    {
      panel.getTextGradient(pid, mValue);
      setToScript(mParam);
      callChangeScript(true);
    }
  }


  virtual void save(DataBlock &blk)
  {
    if (!paramName.empty())
    {
      DataBlock *grad_blk = (paramName.empty()) ? &blk : blk.addBlock(paramName);
      if (grad_blk)
      {
        grad_blk->setStr("type", "text_gradient");
        grad_blk->removeBlock("value");
        for (int i = 0; i < mValue.size(); ++i)
        {
          DataBlock *val_blk = grad_blk->addNewBlock("value");
          val_blk->setReal("x", mValue[i].position);
          val_blk->setStr("text", mValue[i].text.str());
        }
      }
    }
  }


  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {
      const DataBlock *grad_blk = blk.getBlockByName(paramName);
      if (grad_blk && (stricmp("text_gradient", grad_blk->getStr("type", "")) == 0))
      {
        int val_name_id = grad_blk->getNameId("value");
        if (val_name_id == -1)
          return;

        clear_and_shrink(mValue);
        for (int i = 0; i < grad_blk->blockCount(); ++i)
        {
          const DataBlock *val_blk = grad_blk->getBlock(i);
          if (val_blk->getBlockNameId() != val_name_id)
            continue;
          mValue.push_back(TextGradientKey(val_blk->getReal("x", 0), val_blk->getStr("text", "")));
        }
      }

      if (mPanel)
        mPanel->setTextGradient(mPid, mValue);
      setToScript(mParam);
    }
  }

  void validate()
  {
    if (isTarget && mCB)
    {
      for (int i = 0; i < mValue.size(); ++i)
        if (const char *new_name = mCB->validateTarget(mValue[i].text.str(), objType))
        {
          if (new_name == mValue[i].text.str())
            return;
          else
            mValue[i].text = new_name;
        }

      if (mPanel)
        mPanel->setTextGradient(mPid, mValue);
      setToScript(mParam);
    }
  }


  virtual void getTargetList(Tab<SimpleString> &list)
  {
    if (isTarget)
      for (int i = 0; i < mValue.size(); ++i)
        list.push_back(mValue[i].text);
  }


  void setCB(IScriptPanelTargetCB *cb) { mCB = cb; }

private:
  TextGradient mValue;
  int mMin, mMax;
  IScriptPanelTargetCB *mCB;
  SimpleString objType, objFilter;
  bool isTarget;
};


//-----------------------------------------------------------------------------


class SPCurve : public ScriptPanelParam
{
public:
  SPCurve(ScriptPanelContainer *parent, const char *name, const char *caption) :
    ScriptPanelParam(parent, name, caption),
    mValue(midmem),
    mMin(2),
    mMax(100),
    mPoint0(Point2(0, 0)),
    mPoint1(Point2(0, 0)),
    mHeight(hdpi::_pxScaled(60))
  {
    mValue.push_back(Point2(0, 0));
    mValue.push_back(Point2(1, 1));
  }


  void createControl()
  {
    if (!mPanel)
      return;
    mPanel->createCurveEdit(mPid, mCaption, mHeight);
    mPanel->setMinMaxStep(mPid, mMin, mMax, CURVE_MIN_MAX_POINTS);
    mPanel->setMinMaxStep(mPid, mPoint0.x, mPoint1.x, CURVE_MIN_MAX_X);
    mPanel->setMinMaxStep(mPid, mPoint0.y, mPoint1.y, CURVE_MIN_MAX_Y);
    mPanel->setControlPoints(mPid, mValue);
  }


  void getPointsFromSQ(Tab<Point2> &value, SquirrelObject &param, const char param_name[])
  {
    clear_and_shrink(value);
    SquirrelObject val = param.GetValue(param_name);
    if (!val.IsNull() && val.GetType() == OT_ARRAY && val.BeginIteration())
    {
      SquirrelObject key, point;
      while (val.Next(key, point))
        if (point.GetType() == OT_ARRAY && point.Len() == 2)
          value.push_back(Point2(point.GetFloat(SQInteger(0)), point.GetFloat(1)));
      val.EndIteration();
    }
  }


  void getValueFromScript(SquirrelObject param)
  {
    if (param.Exists("height"))
    {
      mHeight = hdpi::_pxScaled(param.GetInt("height"));
      if (mPanel)
      {
        PropertyControlBase *ctrl = mPanel->getById(mPid);
        if (ctrl)
          ctrl->setHeight(mHeight);
      }
    }

    int min = (param.Exists("min")) ? param.GetInt("min") : mMin;
    int max = (param.Exists("max")) ? param.GetInt("max") : mMax;
    if (min != mMin || max != mMax)
    {
      mMin = (min < 0) ? 0 : min;
      mMax = (max >= mMin) ? max : mMin;
      if (mPanel)
        mPanel->setMinMaxStep(mPid, mMin, mMax, CURVE_MIN_MAX_POINTS);
    }

    if (param.Exists("mark") && mPanel)
      mPanel->setFloat(mPid, param.GetFloat("mark"));

    SquirrelObject options = param.GetValue("options");
    if (mPanel && !options.IsNull() && options.GetType() == OT_ARRAY && options.Len() == 4)
    {
      mPoint0 = Point2(options.GetFloat(SQInteger(0)), options.GetFloat(1));
      mPoint1 = Point2(options.GetFloat(2), options.GetFloat(3));
      mPanel->setMinMaxStep(mPid, mPoint0.x, mPoint1.x, CURVE_MIN_MAX_X);
      mPanel->setMinMaxStep(mPid, mPoint0.y, mPoint1.y, CURVE_MIN_MAX_Y);
    }

    if (param.Exists("value"))
    {
      Tab<Point2> value(tmpmem);
      getPointsFromSQ(value, param, "value");

      bool need_update = false;
      if (value.size() != mValue.size())
        need_update = true;
      for (int i = 0; i < value.size(); ++i)
        if (value[i].x != mValue[i].x || value[i].y != mValue[i].y)
        {
          need_update = true;
          break;
        }

      if (need_update)
      {
        mValue = value;
        if (mPanel)
          mPanel->setControlPoints(mPid, mValue);
      }
    }
    else
    {
      if (param.Exists("def"))
        getPointsFromSQ(mValue, param, "def");

      setToScript(param);
    }
  }


  void setToScript(SquirrelObject &param)
  {
    param.SetValue("min", mMin);
    param.SetValue("max", mMax);

    SquirrelObject vals, val;
    param.SetValue("value", SquirrelVM::CreateArray(mValue.size()));
    vals = param.GetValue("value");

    for (int i = 0; i < mValue.size(); ++i)
    {
      vals.SetValue(i, SquirrelVM::CreateArray(2));
      val = vals.GetValue(i);
      val.SetValue(SQInteger(0), mValue[i].x);
      val.SetValue(1, mValue[i].y);
    }
  }


  virtual void onChange(int pid, PropPanel2 &panel)
  {
    if (pid == mPid)
    {
      clear_and_shrink(mValue);
      panel.getCurveCoefs(pid, mValue);
      setToScript(mParam);
      callChangeScript(true);
    }
  }

  virtual void save(DataBlock &blk)
  {
    if (!paramName.empty())
    {
      DataBlock *curve_blk = (paramName.empty()) ? &blk : blk.addBlock(paramName);
      if (curve_blk)
      {
        curve_blk->setStr("type", "curve");
        curve_blk->removeParam("point");
        for (int i = 0; i < mValue.size(); ++i)
          curve_blk->addPoint2("point", mValue[i]);
      }
    }
  }

  virtual void load(const DataBlock &blk)
  {
    if (!paramName.empty())
    {

      const DataBlock *curve_blk = blk.getBlockByName(paramName);
      if (curve_blk && (stricmp("curve", curve_blk->getStr("type", "")) == 0))
      {
        int val_name_id = curve_blk->getNameId("point");
        if (val_name_id == -1)
          return;

        clear_and_shrink(mValue);
        for (int i = 0; i < curve_blk->paramCount(); ++i)
        {
          if (curve_blk->getParamNameId(i) != val_name_id)
            continue;

          mValue.push_back(curve_blk->getPoint2(i));
        }
      }

      if (mPanel)
        mPanel->setControlPoints(mPid, mValue);
      setToScript(mParam);
    }
  }

private:
  Tab<Point2> mValue;
  int mMin, mMax;
  Point2 mPoint0, mPoint1;
  hdpi::Px mHeight;
};


//=============================================================================


class SPGroup : public ScriptPanelContainer
{
public:
  SPGroup(CSQPanelWrapper *wrapper, ScriptPanelContainer *parent, const char *name, const char *caption, bool is_ext) :
    ScriptPanelContainer(wrapper, parent, name, caption), isExt(is_ext), isRenameble(false), namePid(-1)
  {}

  void createControl()
  {
    PropPanel2 *grp = (!mPanel) ? NULL : ((isExt) ? mPanel->createExtGroup(mPid, mCaption) : mPanel->createGroup(mPid, mCaption));
    if (!mControls.IsNull())
      ScriptPanelContainer::fillParams(grp, mPanelWrapper->getFreePid(), mControls);
    if (isRenameble)
    {
      int &pid = mPanelWrapper->getFreePid();
      namePid = ++pid;
      grp->createTargetButton(namePid, "Name", paramName);
      if (mItems.size() > 0)
        grp->moveById(namePid, mItems[0]->getPid());
    }
    if (grp)
      grp->setBoolValue(isMinimized);
  }

  void getValueFromScript(SquirrelObject param)
  {
    isMinimized = param.Exists("def") ? !param.GetBool("def") : false;
    mControls = param.GetValue("controls");
    if (isExt && param.Exists("is_renameble"))
      isRenameble = param.GetBool("is_renameble");
  }

  void load(const DataBlock &blk)
  {
    if (isRenameble)
      mPanel->setText(namePid, paramName);

    bool is_minim = isMinimized;
    if (mPanel)
    {
      is_minim = mPanel->getBool(mPid);
      mPanel->setBool(mPid, false);
    }
    ScriptPanelContainer::load(blk);
    if (mPanel)
      mPanel->setBool(mPid, is_minim);
  }

  void onChange(int pid, PropPanel2 &panel)
  {
    IScriptPanelTargetCB *cb = mPanelWrapper->getObjectCB();
    if (isRenameble && pid == namePid && cb)
    {
      const char *choise = cb->getTarget(paramName, "ext_param_name", "");
      paramName = (choise) ? choise : paramName;
      mPanel->setText(namePid, paramName);
      return;
    }

    ScriptPanelContainer::onChange(pid, panel);
  }


protected:
  bool isExt, isMinimized;
  SquirrelObject mControls;
  bool isRenameble;
  int namePid;
};


class SPGroupBox : public ScriptPanelContainer
{
public:
  SPGroupBox(CSQPanelWrapper *wrapper, ScriptPanelContainer *parent, const char *name, const char *caption) :
    ScriptPanelContainer(wrapper, parent, name, caption)
  {}

  void createControl()
  {
    PropPanel2 *grp = (!mPanel) ? NULL : mPanel->createGroupBox(mPid, mCaption);
    if (!mControls.IsNull())
      ScriptPanelContainer::fillParams(grp, mPanelWrapper->getFreePid(), mControls);
  }

  void getValueFromScript(SquirrelObject param) { mControls = param.GetValue("controls"); }

protected:
  bool isExt;
  SquirrelObject mControls;
};


//=============================================================================

class SPSet : public ScriptPanelContainer
{
public:
  SPSet(CSQPanelWrapper *wrapper, ScriptPanelContainer *parent, const char *name, const char *caption) :
    ScriptPanelContainer(wrapper, parent, name, caption)
  {}

  virtual void createControl()
  {
    PropPanel2 *grp = !mPanel ? NULL : mPanel->createGroup(mPid, mCaption);
    if (!controls.IsNull())
      ScriptPanelContainer::fillParams(grp, mPanelWrapper->getFreePid(), controls);
    if (grp)
      grp->setBoolValue(isMinimized);
  }

  virtual void getValueFromScript(SquirrelObject param)
  {
    controls = param.GetValue("controls");
    isMinimized = param.Exists("def") ? !param.GetBool("def") : false;
  }

  virtual void load(const DataBlock &blk)
  {
    for (int i = 0; i < mItems.size(); ++i)
    {
      mItems[i]->load(blk);
    }
  }

  virtual void save(DataBlock &blk)
  {
    for (int i = 0; i < mItems.size(); ++i)
    {
      mItems[i]->save(blk);
    }
  }

private:
  SquirrelObject controls;
  bool isMinimized;
};

//=============================================================================


void ScriptPanelContainer::scriptControlFactory(PropPanel2 *panel, int &pid, SquirrelObject param)
{
  ScriptPanelParam *panel_param = NULL;

  // extensible containers
  bool is_ext = (param.Exists("extensible")) ? param.GetBool("extensible") : false;
  if (is_ext && scriptExtFactory(panel, pid, param))
    return;

  // controls create
  const char *type = getParamType(param);
  const char *param_name = getParamName(param);
  const char *tooltipLocalizationKey = getTooltipLocalizationKey(param);
  const char *defaultTooltip = getDefaultTooltip(param);
  const char *caption = getParamCaption(param, param_name);

  SquirrelObject def = param.GetValue("def");
  SquirrelObject controls = param.GetValue("controls");

  if (stricmp(type, "group") == 0)
    panel_param = new SPGroup(mPanelWrapper, this, param_name, caption, is_ext);
  else if (stricmp(type, "uncollapsible_group") == 0)
    panel_param = new SPGroupBox(mPanelWrapper, this, param_name, caption);
  else if (stricmp(type, "text") == 0)
    panel_param = new SPText(this, param_name, caption);
  else if (stricmp(type, "int") == 0)
    panel_param = new SPInt(this, param_name, caption);
  else if (stricmp(type, "real") == 0)
    panel_param = new SPReal(this, param_name, caption);
  else if (stricmp(type, "bool") == 0)
    panel_param = new SPBool(this, param_name, caption);
  else if (stricmp(type, "color") == 0)
    panel_param = new SPColor(this, param_name, caption);
  else if (stricmp(type, "ranged_int") == 0)
    panel_param = new SPRangedInt(this, param_name, caption);
  else if (stricmp(type, "ranged_real") == 0)
    panel_param = new SPRangedReal(this, param_name, caption);
  else if (stricmp(type, "point2") == 0)
    panel_param = new SPPoint2(this, param_name, caption);
  else if (stricmp(type, "point3") == 0)
    panel_param = new SPPoint3(this, param_name, caption);
  else if (stricmp(type, "point4") == 0)
    panel_param = new SPPoint4(this, param_name, caption);
  else if (stricmp(type, "ipoint2") == 0)
    panel_param = new SPIPoint2(this, param_name, caption);
  else if (stricmp(type, "ipoint3") == 0)
    panel_param = new SPIPoint3(this, param_name, caption);
  else if (stricmp(type, "matrix") == 0)
    panel_param = new SPMatrix(this, param_name, caption);
  else if (stricmp(type, "enum_text") == 0)
    panel_param = new SPEnumText(this, param_name, caption);
  else if (stricmp(type, "enum_int") == 0)
    panel_param = new SPEnumInt(this, param_name, caption);
  else if (stricmp(type, "enum_real") == 0)
    panel_param = new SPEnumReal(this, param_name, caption);
  else if (stricmp(type, "target_file") == 0)
    panel_param = new SPTargetFile(this, param_name, caption);
  else if (stricmp(type, "target_folder") == 0)
    panel_param = new SPTargetFolder(this, param_name, caption);
  else if (stricmp(type, "target_asset") == 0 || stricmp(type, "target_object") == 0)
  {
    panel_param = new SPTarget(this, param_name, caption);
    if (mPanelWrapper && panel_param)
      ((SPTarget *)panel_param)->setCB(mPanelWrapper->getObjectCB());
  }
  else if (stricmp(type, "object_or_text") == 0)
  {
    panel_param = new SPCustomTarget(this, param_name, caption);
    if (mPanelWrapper && panel_param)
      ((SPCustomTarget *)panel_param)->setCB(mPanelWrapper->getObjectCB());
  }
  else if (stricmp(type, "gradient") == 0)
    panel_param = new SPGradient(this, param_name, caption);
  else if (stricmp(type, "text_gradient") == 0)
  {
    panel_param = new SPTextGradient(this, param_name, caption);
    if (mPanelWrapper && panel_param)
      ((SPTextGradient *)panel_param)->setCB(mPanelWrapper->getObjectCB());
  }
  else if (stricmp(type, "set") == 0)
    panel_param = new SPSet(mPanelWrapper, this, param_name, caption);
  else if (stricmp(type, "curve") == 0)
    panel_param = new SPCurve(this, param_name, caption);

  if (panel_param)
  {
    mItems.push_back(panel_param);
    panel_param->fillParameter(panel, pid, param);
    panel_param->setTooltip(get_localized_text(tooltipLocalizationKey, defaultTooltip));
  }
}
