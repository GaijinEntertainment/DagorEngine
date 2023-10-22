//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <propPanel2/c_window_event_handler.h>
#include <propPanel2/c_control_event_handler.h>
#include <propPanel2/c_data_flags.h>
#include <propPanel2/c_common.h>

#include <generic/dag_tab.h>
#include <math/dag_math3d.h>
#include <util/dag_string.h>
#include <libTools/util/hdpiUtil.h>


class PropertyContainerControlBase;
class DataBlock;


class PropertyControlBase : public WindowControlEventHandler
{
public:
  PropertyControlBase(int id, ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int x, int y, hdpi::Px w,
    hdpi::Px h);
  virtual ~PropertyControlBase();

  // Sets
  virtual void setEnabled(bool enabled) { G_UNUSED(enabled); };
  virtual void setFocus(){};

  virtual void setUserDataValue(const void *value) { G_UNUSED(value); };
  virtual void setTextValue(const char value[]) { G_UNUSED(value); };
  virtual void setFloatValue(float value) { G_UNUSED(value); };
  virtual void setIntValue(int value) { G_UNUSED(value); };
  virtual void setBoolValue(bool value) { G_UNUSED(value); };
  virtual void setColorValue(E3DCOLOR value) { G_UNUSED(value); };
  virtual void setPoint2Value(Point2 value) { G_UNUSED(value); };
  virtual void setPoint3Value(Point3 value) { G_UNUSED(value); };
  virtual void setPoint4Value(Point4 value) { G_UNUSED(value); };
  virtual void setMatrixValue(const TMatrix &value) { G_UNUSED(value); }
  virtual void setGradientValue(PGradient value) { G_UNUSED(value); };
  virtual void setTextGradientValue(const TextGradient &source) { G_UNUSED(source); }
  virtual void setControlPointsValue(Tab<Point2> &points) { G_UNUSED(points); };

  virtual void setTooltip(const char tooltip[]) { G_UNUSED(tooltip); }

  // for indirect fill

  virtual void setMinMaxStepValue(float min, float max, float step)
  {
    G_UNUSED(min);
    G_UNUSED(max);
    G_UNUSED(step);
  };
  virtual void setPrecValue(int prec) { G_UNUSED(prec); }
  virtual void setStringsValue(const Tab<String> &vals) { G_UNUSED(vals); };
  virtual void setSelectionValue(const Tab<int> &sels) { G_UNUSED(sels); };
  virtual void setCaptionValue(const char value[]) { G_UNUSED(value); };
  virtual void setButtonPictureValues(const char *fname = NULL) { G_UNUSED(fname); };

  virtual int addStringValue(const char *value);
  virtual void removeStringValue(int idx) { G_UNUSED(idx); }

  // Gets
  virtual void *getUserDataValue() const;
  virtual int getTextValue(char *buffer, int buflen) const;
  virtual int getCaptionValue(char *buffer, int buflen) const;
  virtual float getFloatValue() const;
  virtual int getIntValue() const;
  virtual bool getBoolValue() const;
  virtual E3DCOLOR getColorValue() const;
  virtual Point2 getPoint2Value() const;
  virtual Point3 getPoint3Value() const;
  virtual Point4 getPoint4Value() const;
  virtual TMatrix getMatrixValue() const;
  virtual void getGradientValue(PGradient destGradient) const;
  virtual void getTextGradientValue(TextGradient &destGradient) const;
  virtual void getCurveCoefsValue(Tab<Point2> &points) const;
  virtual bool getCurveCubicCoefsValue(Tab<Point2> &xy_4c_per_seg) const;

  virtual int getStringsValue(Tab<String> &vals);
  virtual int getSelectionValue(Tab<int> &sels);

  virtual unsigned getTypeMaskForSet() const = 0;
  virtual unsigned getTypeMaskForGet() const = 0;

  virtual unsigned getWidth() const { return mW; }
  virtual unsigned getHeight() const { return mH; }
  virtual int getX() const { return mX; }
  virtual int getY() const { return mY; }
  virtual int getID() const { return mId; }

  virtual void setWidth(hdpi::Px w) { mW = _px(w); }
  virtual void setHeight(hdpi::Px h) { mH = _px(h); }
  virtual void resize(hdpi::Px w, hdpi::Px h)
  {
    setWidth(w);
    setHeight(h);
  }
  virtual void moveTo(int x, int y)
  {
    mX = x;
    mY = y;
  }

  virtual void reset(){};

  // WindowControlEventHandler
  virtual long onWcChanging(WindowBase *source);
  virtual void onWcChange(WindowBase *source);
  virtual void onWcClick(WindowBase *source);
  virtual void onWcDoubleClick(WindowBase *source);
  virtual long onWcKeyDown(WindowBase *source, unsigned v_key);
  virtual void onWcResize(WindowBase *source);
  virtual void onWcPostEvent(unsigned id);

  virtual const PropertyContainerControlBase *getContainer() const { return NULL; }
  virtual PropertyContainerControlBase *getContainer() { return NULL; }

  virtual ControlEventHandler *getEventHandler() const { return mEventHandler; }
  virtual void setEventHandler(ControlEventHandler *value) { mEventHandler = value; }

  virtual PropertyContainerControlBase *getRootParent();
  virtual PropertyContainerControlBase *getParent();

  void enableChangeHandlers() { mEnabledChanges = true; }

  // saving and loading state with datablock

  virtual int saveState(DataBlock &datablk)
  {
    G_UNUSED(datablk);
    return 0;
  }
  virtual int loadState(DataBlock &datablk)
  {
    G_UNUSED(datablk);
    return 0;
  }


protected:
  int mId;
  ControlEventHandler *mEventHandler;
  PropertyContainerControlBase *mParent;

  int mX, mY;
  unsigned mW, mH;
  bool hasCaption;
  bool mEnabledChanges;
};
