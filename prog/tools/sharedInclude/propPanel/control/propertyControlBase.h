//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/c_common.h>
#include <propPanel/c_control_event_handler.h>
#include <propPanel/c_window_event_handler.h>
#include <propPanel/c_data_flags.h>
#include <propPanel/focusHelper.h>

#include <generic/dag_tab.h>
#include <libTools/util/hdpiUtil.h>
#include <math/dag_math3d.h>
#include <util/dag_string.h>

class DataBlock;

namespace PropPanel
{

class ContainerPropertyControl;

class PropertyControlBase : public WindowControlEventHandler
{
public:
  PropertyControlBase(int id, ControlEventHandler *event_handler, ContainerPropertyControl *parent, int x = 0, int y = 0,
    hdpi::Px w = hdpi::Px(0), hdpi::Px h = hdpi::Px(0));
  ~PropertyControlBase();

  // Sets
  virtual void setEnabled(bool enabled) { G_UNUSED(enabled); }
  virtual void setFocus() { focus_helper.requestFocus(this); }

  virtual void setUserDataValue(const void *value) { G_UNUSED(value); }
  virtual void setTextValue(const char value[]) { G_UNUSED(value); }
  virtual void setFloatValue(float value) { G_UNUSED(value); }
  virtual void setIntValue(int value) { G_UNUSED(value); }
  virtual void setBoolValue(bool value) { G_UNUSED(value); }
  virtual void setColorValue(E3DCOLOR value) { G_UNUSED(value); }
  virtual void setPoint2Value(Point2 value) { G_UNUSED(value); }
  virtual void setPoint3Value(Point3 value) { G_UNUSED(value); }
  virtual void setPoint4Value(Point4 value) { G_UNUSED(value); }
  virtual void setMatrixValue(const TMatrix &value) { G_UNUSED(value); }
  virtual void setGradientValue(PGradient value) { G_UNUSED(value); }
  virtual void setTextGradientValue(const TextGradient &source) { G_UNUSED(source); }
  virtual void setControlPointsValue(Tab<Point2> &points) { G_UNUSED(points); }

  virtual void setTooltip(const char tooltip[]) { controlTooltip = tooltip; }

  // for indirect fill

  virtual void setMinMaxStepValue(float min, float max, float step)
  {
    G_UNUSED(min);
    G_UNUSED(max);
    G_UNUSED(step);
  }

  virtual void setPrecValue(int prec) { G_UNUSED(prec); }
  virtual void setStringsValue(const Tab<String> &vals) { G_UNUSED(vals); }
  virtual void setSelectionValue(const Tab<int> &sels) { G_UNUSED(sels); }
  virtual void setCaptionValue(const char value[]) { G_UNUSED(value); }
  virtual void setButtonPictureValues(const char *fname = nullptr) { G_UNUSED(fname); }

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

  // You can use Constants::LISTBOX_FULL_HEIGHT for list boxes here.
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

  virtual void reset() {}

  // WindowControlEventHandler
  virtual long onWcChanging(WindowBase *source) override;
  virtual void onWcChange(WindowBase *source) override;
  virtual void onWcClick(WindowBase *source) override;
  virtual void onWcDoubleClick(WindowBase *source) override;
  virtual long onWcKeyDown(WindowBase *source, unsigned v_key) override;
  virtual void onWcResize(WindowBase *source) override;
  virtual void onWcPostEvent(unsigned id) override;

  virtual const ContainerPropertyControl *getContainer() const { return nullptr; }
  virtual ContainerPropertyControl *getContainer() { return nullptr; }

  virtual ControlEventHandler *getEventHandler() const { return mEventHandler; }
  virtual void setEventHandler(ControlEventHandler *value) { mEventHandler = value; }

  virtual ContainerPropertyControl *getRootParent();
  virtual ContainerPropertyControl *getParent();

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

  virtual void updateImgui() {}

  // Returns with PropPanel::ControlType.
  virtual int getImguiControlType() const { return 0; }

protected:
  void setFocusToNextImGuiControlIfRequested(int offset = 0) { focus_helper.setFocusToNextImGuiControlIfRequested(this, offset); }

  void setPreviousImguiControlTooltip();

  int mId;
  ControlEventHandler *mEventHandler;
  ContainerPropertyControl *mParent;

  int mX, mY;
  unsigned mW, mH;
  bool hasCaption;
  bool mEnabledChanges;

  String controlTooltip;
};

} // namespace PropPanel