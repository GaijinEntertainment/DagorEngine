#pragma once

#include "wndBase.h"


class ClientWindowArea : public BaseWindow
{
public:
  ClientWindowArea(void *hparent, int x, int y, int w, int h);
  ~ClientWindowArea();

  virtual intptr_t winProc(void *h_wnd, unsigned msg, TSgWParam w_param, TSgLParam l_param);

  void resize(int x, int y, int width, int height);

protected:
  void *mPHandle;
};
