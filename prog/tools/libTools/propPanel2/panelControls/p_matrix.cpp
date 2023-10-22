// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_matrix.h"
#include "../c_constants.h"

CMatrix::CMatrix(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  const char caption[], int prec) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, _pxScaled(DEFAULT_CONTROL_HEIGHT) * 5),
  mCaption(this, parent->getWindow(), x, y, _px(w), _pxS(DEFAULT_CONTROL_HEIGHT)),
  mSpinWidth(_px(w) / 3.5),

  mText0(this, parent->getWindow(), x, y, mSpinWidth / 2, _pxS(DEFAULT_CONTROL_HEIGHT)),
  mSpin00(this, parent->getWindow(), x, y, mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0, 0, prec),
  mSpin01(this, parent->getWindow(), x, y, mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0, 0, prec),
  mSpin02(this, parent->getWindow(), x, y, mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0, 0, prec),

  mText1(this, parent->getWindow(), x, y, mSpinWidth / 2, _pxS(DEFAULT_CONTROL_HEIGHT)),
  mSpin10(this, parent->getWindow(), x, y, mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0, 0, prec),
  mSpin11(this, parent->getWindow(), x, y, mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0, 0, prec),
  mSpin12(this, parent->getWindow(), x, y, mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0, 0, prec),

  mText2(this, parent->getWindow(), x, y, mSpinWidth / 2, _pxS(DEFAULT_CONTROL_HEIGHT)),
  mSpin20(this, parent->getWindow(), x, y, mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0, 0, prec),
  mSpin21(this, parent->getWindow(), x, y, mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0, 0, prec),
  mSpin22(this, parent->getWindow(), x, y, mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0, 0, prec),

  mText3(this, parent->getWindow(), x, y, mSpinWidth / 2, _pxS(DEFAULT_CONTROL_HEIGHT)),
  mSpin30(this, parent->getWindow(), x, y, mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0, 0, prec),
  mSpin31(this, parent->getWindow(), x, y, mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0, 0, prec),
  mSpin32(this, parent->getWindow(), x, y, mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0, 0, prec)
{
  mSpins[0][0] = &mSpin00;
  mSpins[0][1] = &mSpin01;
  mSpins[0][2] = &mSpin02;
  mSpins[1][0] = &mSpin10;
  mSpins[1][1] = &mSpin11;
  mSpins[1][2] = &mSpin12;
  mSpins[2][0] = &mSpin20;
  mSpins[2][1] = &mSpin21;
  mSpins[2][2] = &mSpin22;
  mSpins[3][0] = &mSpin30;
  mSpins[3][1] = &mSpin31;
  mSpins[3][2] = &mSpin32;

  mTexts[0] = &mText0;
  mTexts[1] = &mText1;
  mTexts[2] = &mText2;
  mTexts[3] = &mText3;

  moveTo(x, y);

  mCaption.setTextValue(caption);
  mText0.setTextValue("col0");
  mText1.setTextValue("col1");
  mText2.setTextValue("col2");
  mText3.setTextValue("pos");
  this->setMatrixValue(TMatrix::IDENT);
  for (int x = 0; x < 4; ++x)
  {
    for (int y = 0; y < 4; ++y)
    {
      initTooltip(mSpins[y][x]);
    }
  }
}


PropertyContainerControlBase *CMatrix::createDefault(int id, PropertyContainerControlBase *parent, const char caption[], bool new_line)
{
  parent->createMatrix(id, caption, TMatrix::IDENT, 2, true, new_line);
  return NULL;
}


TMatrix CMatrix::getMatrixValue() const { return mValue; }


void CMatrix::setMatrixValue(const TMatrix &value)
{
  mValue = value;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 3; ++j)
      mSpins[i][j]->setValue(mValue.m[i][j]);
}

void CMatrix::setPrecValue(int prec)
{
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 3; ++j)
      mSpins[i][j]->setPrecValue(prec);
}


void CMatrix::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }


void CMatrix::reset()
{
  this->setMatrixValue(TMatrix::IDENT);
  PropertyControlBase::reset();
}


void CMatrix::onWcChange(WindowBase *source)
{
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 3; ++j)
      if (source == mSpins[i][j])
        mValue.m[i][j] = mSpins[i][j]->getValue();

  PropertyControlBase::onWcChange(source);
}


void CMatrix::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mCaption.moveWindow(x, y);
  int _y = y + _pxS(DEFAULT_CONTROL_HEIGHT);
  for (int i = 0; i < 4; ++i)
  {
    mTexts[i]->moveWindow(x, _y);
    int _x = x + mSpinWidth / 2;
    for (int j = 0; j < 3; ++j)
    {
      mSpins[i][j]->moveWindow(_x, _y);
      _x += mSpinWidth;
    }
    _y += _pxS(DEFAULT_CONTROL_HEIGHT);
  }
}


void CMatrix::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);

  for (int i = 0; i < 4; ++i)
  {
    mTexts[i]->setEnabled(enabled);
    for (int j = 0; j < 3; ++j)
      mSpins[i][j]->setEnabled(enabled);
  }
}


void CMatrix::setWidth(hdpi::Px w)
{
  PropertyControlBase::setWidth(w);

  mSpinWidth = _px(w) / 3.5;
  int ht = mSpin00.getHeight();

  mCaption.resizeWindow(_px(w), mCaption.getHeight());
  for (int i = 0; i < 4; ++i)
  {
    mTexts[i]->resizeWindow(mSpinWidth / 2, ht);
    for (int j = 0; j < 3; ++j)
      mSpins[i][j]->resizeWindow(mSpinWidth, ht);
  }

  this->moveTo(this->getX(), this->getY());
}
