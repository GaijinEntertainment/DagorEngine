#ifndef __GAIJIN_DE_ABOUT_DLG__
#define __GAIJIN_DE_ABOUT_DLG__
#pragma once


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
};


#endif //__GAIJIN_DE_ABOUT_DLG__
