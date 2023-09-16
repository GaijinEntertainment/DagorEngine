
//-----------------------------------------------------------------------------
//-- Class Definition ---------------------------------------------------------
//

class BitmapIO_TA : public BitmapIO
{
public:
  //-- Constructors/Destructors

  BitmapIO_TA();
  // BitmapIO_IFL       ( BitmapStorage *s,BitmapIO *previous,int frame );
  ~BitmapIO_TA();

  //-- Number of extemsions supported

  int ExtCount() { return 1; }

  //-- Extension #n (i.e. "3DS")

  const TCHAR *Ext(int n) { return _T("ta"); }

  //-- Descriptions

  const TCHAR *LongDesc() { return _T("Dagor Texture Animation"); }
  const TCHAR *ShortDesc() { return _T("TexAnim"); }

  //-- Miscelaneous Messages

  const TCHAR *AuthorName() { return _T("Aleksey Volynskov"); }
  const TCHAR *CopyrightMessage() { return _T("(c) 2002 Aleksey Volynskov"); }
  const TCHAR *OtherMessage1() { return _T(""); }
  const TCHAR *OtherMessage2() { return _T(""); }

  unsigned int Version() { return (1); }

  //-- Driver capabilities

  int Capability()
  {
    return BMMIO_READER | BMMIO_EXTENSION |
           // BMMIO_INFODLG    |
           // BMMIO_OWN_VIEWER |
           // BMMIO_CONTROLREAD |
           BMMIO_IFL;
  }

  //-- Driver Configuration

#if defined(MAX_RELEASE_R15) && MAX_RELEASE >= MAX_RELEASE_R15
  BOOL LoadConfigure(void *ptr, DWORD piDataSize) { return 1; }
#else
  BOOL LoadConfigure(void *ptr) { return 1; }
#endif
  BOOL SaveConfigure(void *ptr) { return 1; }
  DWORD EvaluateConfigure() { return 0; }

  //-- Show DLL's "About..." box

  void ShowAbout(HWND hWnd);

  //-- Show DLL's Control Panel

  BOOL ShowControl(HWND hWnd, DWORD flag);

  //-- Return info about image

  // BMMRES         GetImageInfoDlg    ( HWND hWnd, BitmapInfo *fbi, const TCHAR *filename);
  BMMRES GetImageInfo(BitmapInfo *bi);

  //-- I/O Interface

  BMMRES GetImageName(BitmapInfo *fbi, TCHAR *filename);

  BitmapStorage *Load(BitmapInfo *fbi, Bitmap *map, BMMRES *status);
  BMMRES OpenOutput(BitmapInfo *bi, Bitmap *map);
  BMMRES Write(int frame);
  int Close(int flag);


  BOOL Control(HWND, UINT, WPARAM, LPARAM);
};
