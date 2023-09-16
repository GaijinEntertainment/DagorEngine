#ifndef __GAIJIN_EDITORCORE_EC_WINDOW_H__
#define __GAIJIN_EDITORCORE_EC_WINDOW_H__
#pragma once

typedef void *TEcHandle;
typedef void *TEcWParam;
typedef void *TEcLParam;

struct EcRect;


class EcWindow
{
public:
  EcWindow(TEcHandle parent, int x, int y, int w, int h);
  virtual ~EcWindow();

  virtual int windowProc(TEcHandle h_wnd, unsigned msg, TEcWParam w_param, TEcLParam l_param);

  TEcHandle getHandle() const { return mHWnd; }
  TEcHandle getParentHandle() const;

  int getW() const;
  int getH() const;
  bool isVisible() const { return true; }

  void getClientRect(EcRect &clientRect) const;

  void moveWindow(int left, int top, bool repaint = true);
  void resizeWindow(int w, int h, bool repaint = true);
  void moveResizeWindow(int left, int top, int w, int h, bool repaint = true);

  virtual void activate(bool repaint);
  bool isActive() const;

  void translateToScreen(int &x, int &y);
  void translateToClient(int &x, int &y);

  void invalidateClientRect();

  void captureMouse();
  void releaseMouse();

private:
  TEcHandle mHWnd;
};


#endif