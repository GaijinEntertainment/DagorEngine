// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

//-----------------------------------------------------------------------------
//-- Class Definition ---------------------------------------------------------
//

class BitmapIO_TA : public BitmapIO
{
public:
  //-- Constructors/Destructors

  BitmapIO_TA();
  // BitmapIO_IFL       ( BitmapStorage *s,BitmapIO *previous,int frame );
  ~BitmapIO_TA() override;

  //-- Number of extemsions supported

  int ExtCount() override { return 1; }

  //-- Extension #n (i.e. "3DS")

  const TCHAR *Ext(int n) override { return _T("ta"); }

  //-- Descriptions

  const TCHAR *LongDesc() override { return _T("Dagor Texture Animation"); }
  const TCHAR *ShortDesc() override { return _T("TexAnim"); }

  //-- Miscelaneous Messages

  const TCHAR *AuthorName() override { return _T("Aleksey Volynskov"); }
  const TCHAR *CopyrightMessage() override { return _T("(c) 2002 Aleksey Volynskov"); }
  const TCHAR *OtherMessage1() { return _T(""); }
  const TCHAR *OtherMessage2() { return _T(""); }

  unsigned int Version() override { return (1); }

  //-- Driver capabilities

  int Capability() override
  {
    return BMMIO_READER | BMMIO_EXTENSION |
           // BMMIO_INFODLG    |
           // BMMIO_OWN_VIEWER |
           // BMMIO_CONTROLREAD |
           BMMIO_IFL;
  }

  //-- Driver Configuration

#if defined(MAX_RELEASE_R15) && MAX_RELEASE >= MAX_RELEASE_R15
  BOOL LoadConfigure(void *ptr, DWORD piDataSize) override { return 1; }
#else
  BOOL LoadConfigure(void *ptr) { return 1; }
#endif
  BOOL SaveConfigure(void *ptr) override { return 1; }
  DWORD EvaluateConfigure() override { return 0; }

  //-- Show DLL's "About..." box

  void ShowAbout(HWND hWnd) override;

  //-- Show DLL's Control Panel

  BOOL ShowControl(HWND hWnd, DWORD flag) override;

  //-- Return info about image

  // BMMRES         GetImageInfoDlg    ( HWND hWnd, BitmapInfo *fbi, const TCHAR *filename);
  BMMRES GetImageInfo(BitmapInfo *bi) override;

  //-- I/O Interface

  BMMRES GetImageName(BitmapInfo *fbi, TCHAR *filename) override;

  BitmapStorage *Load(BitmapInfo *fbi, Bitmap *map, BMMRES *status) override;
  BMMRES OpenOutput(BitmapInfo *bi, Bitmap *map) override;
  BMMRES Write(int frame) override;
  int Close(int flag) override;


  BOOL Control(HWND, UINT, WPARAM, LPARAM);
};
