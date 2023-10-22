#ifndef __GAIJIN_DE_ABOUT_DLG__
#define __GAIJIN_DE_ABOUT_DLG__
#pragma once

#include <libTools/util/hdpiUtil.h>

class AboutDlg
{
public:
  AboutDlg(void *hwnd);
  ~AboutDlg();

  void show();

  long processMessages(void *hwnd, unsigned msg, void *w_param, void *l_param);

private:
  void *mHandle, *mpHandle, *mFNH, *mFBH, *mBitmapHandle;
  void *mTextHandle, *mOkButtonHandle;
  int bmpW, bmpH;

  int mScrollPos, scrollH;

  void fill();
  void addLine(const char text[], bool bolt = false);

  const int FONT_HEIGHT = hdpi::_pxS(14);
  const int HOR_INDENT = hdpi::_pxS(3), LABEL_HEIGHT = hdpi::_pxS(20);
  const int ICON_LEFT = hdpi::_pxS(20), ICON_TOP = hdpi::_pxS(20);
  const int TEXT_LEFT = (ICON_LEFT * 2 + hdpi::_pxS(42)), TEXT_TOP = ICON_TOP, TEXT_HEIGHT = (4 * LABEL_HEIGHT);
  const int SCROLL_TOP = (TEXT_TOP + TEXT_HEIGHT + hdpi::_pxS(4));
  const int LINE_SIZE = hdpi::_pxS(18);
  const int BUTTON_WIDTH = hdpi::_pxS(100), BUTTON_HEIGHT = hdpi::_pxS(28);
  const int DIALOG_WIDTH = hdpi::_pxS(340), DIALOG_HEIGHT = hdpi::_pxS(350);
  const int CLIENT_WIDTH = (DIALOG_WIDTH - HOR_INDENT * 2);
  const int TEXT_WIDTH = (CLIENT_WIDTH - TEXT_LEFT - HOR_INDENT);
  const int SCROLL_HEIGHT = (DIALOG_HEIGHT - SCROLL_TOP - BUTTON_HEIGHT * 3);
};


#endif //__GAIJIN_DE_ABOUT_DLG__
