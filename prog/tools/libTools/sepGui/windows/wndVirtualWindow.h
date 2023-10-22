#pragma once

#include "../wndBase.h"


class ClientWindow;
class SplitterWindow;


class VirtualWindow : public CascadeWindow
{
public:
  VirtualWindow(WinManager *owner);
  ~VirtualWindow();

  virtual void getPos(IPoint2 &left_top, IPoint2 &right_bottom) const;
  virtual void resize(const IPoint2 &left_top, const IPoint2 &right_bottom);

  virtual bool getMinSize(hdpi::Px &min_w, hdpi::Px &min_h) const;
  virtual WindowSizeLock getSizeLock() const;

  virtual ClientWindow *getWindowOnPos(const IPoint2 &cur_point);
  virtual VirtualWindow *getVirtualWindow() const;
  virtual ClientWindow *getClientWindow() const;

  void setWindows(CascadeWindow *one, CascadeWindow *two, SplitterWindow *splitter, float ratio, const IPoint2 &p1, const IPoint2 &p2);

  void refreshPos();

  void setRatio(float new_ratio);
  float getRatio() const;

  void deleteSplitter(bool delete_right = true);

  // Left / Top
  void setFirst(CascadeWindow *one);
  CascadeWindow *getFirst() const;
  // Right / Bottom
  void setSecond(CascadeWindow *two);
  CascadeWindow *getSecond() const;

  bool isVertical() const;

private:
  CascadeWindow *mLeftWnd, *mRightWnd;
  SplitterWindow *mSplitterWnd;
  float mRatio;
};
