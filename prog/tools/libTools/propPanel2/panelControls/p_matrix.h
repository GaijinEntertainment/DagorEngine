#pragma once

#include "p_base.h"
#include "../windowControls/w_spin_edit.h"
#include <propPanel2/c_panel_base.h>

// #include <math/dag_TMatrix.h>

class TMatrix;


class CMatrix : public BasicPropertyControl
{
public:
  CMatrix(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, const char caption[],
    int prec);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_MATRIX | CONTROL_CAPTION | CONTROL_DATA_PREC; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_MATRIX; }

  TMatrix getMatrixValue() const;
  void setMatrixValue(const TMatrix &value);
  void setPrecValue(int prec);
  void setCaptionValue(const char value[]);

  void reset();

  void setEnabled(bool enabled);
  void setWidth(unsigned w);
  void moveTo(int x, int y);

protected:
  virtual void onWcChange(WindowBase *source);

private:
  int mSpinWidth;
  TMatrix mValue;

  WStaticText mCaption, mText0, mText1, mText2, mText3;
  WSpinEdit mSpin00, mSpin01, mSpin02;
  WSpinEdit mSpin10, mSpin11, mSpin12;
  WSpinEdit mSpin20, mSpin21, mSpin22;
  WSpinEdit mSpin30, mSpin31, mSpin32;

  WSpinEdit *mSpins[4][3];
  WStaticText *mTexts[4];
};
