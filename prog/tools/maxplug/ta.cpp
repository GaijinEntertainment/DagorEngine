#include <max.h>
#include <bmmlib.h>
#include "dagor.h"
#include "ta.h"
#include "loadta.h"
#include <string>

std::string wideToStr(const TCHAR *sw);
M_STD_STRING strToWide(const char *sz);

class TAClassDesc : public ClassDesc
{
public:
  int IsPublic() { return 1; }
  void *Create(BOOL loading = FALSE) { return new BitmapIO_TA; }
  const TCHAR *ClassName() { return _T("Dagor TexAnim"); }
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
  SClass_ID SuperClassID() { return BMM_IO_CLASS_ID; }
  Class_ID ClassID() { return TexAnimIO_CID; }
  const TCHAR *Category() { return _T("Bitmap I/O"); }
};

static TAClassDesc TADesc;
ClassDesc *GetTexAnimIOCD() { return &TADesc; }

BitmapIO_TA::BitmapIO_TA() {}

BitmapIO_TA::~BitmapIO_TA() {}

void BitmapIO_TA::ShowAbout(HWND hWnd) {}

BOOL BitmapIO_TA::ShowControl(HWND hWnd, DWORD flag) { return (FALSE); }

static void check_image_file(TCHAR *fn)
{
  int e;
  for (e = (int)_tcslen(fn) - 1; e >= 0; --e)
    if (fn[e] == '\\' || fn[e] == '/' || fn[e] == ':' || fn[e] == '.')
      break;
  if (e >= 0)
    if (fn[e] == '.')
      return;
  e = (int)_tcslen(fn);
  _tcscpy(fn + e, _T(".tga"));
  if (DoesFileExist(fn))
    return;
  _tcscpy(fn + e, _T(".jpg"));
  if (DoesFileExist(fn))
    return;
  fn[e] = 0;
}

BMMRES BitmapIO_TA::GetImageInfo(BitmapInfo *fbi)
{
  //-- Define Number of Frames
  TexAnimFile file;
  std::string s = wideToStr(fbi->Name());
  if (!file.load(s.c_str()))
    return (ProcessImageIOError(fbi, (TCHAR *)strToWide(file.getlasterr()).c_str()));
  int frameCount = file.frm.Count();

  if (!frameCount)
    return (ProcessImageIOError(fbi, _T("no frames in .TA file")));

  //-- Get First Image from List

  TCHAR filenameOut[MAX_PATH];
  BitmapInfo tbi;


  _tcscpy(filenameOut, strToWide(file.tex[0]).c_str());
  check_image_file(filenameOut);
  tbi.SetName(filenameOut);
  BMMRES res = TheManager->GetImageInfo(&tbi, filenameOut);
  if (res != BMMRES_SUCCESS)
    return (res);

  //-- Set Image Info

  fbi->SetWidth(tbi.Width());
  fbi->SetHeight(tbi.Height());
  // fbi->SetGamma(tbi.Gamma());
  fbi->SetAspect(tbi.Aspect());
  fbi->SetType(tbi.Type());
  fbi->SetFirstFrame(0);
  fbi->SetLastFrame(frameCount - 1);

  return BMMRES_SUCCESS;
}

//    We don't load anything...
BitmapStorage *BitmapIO_TA::Load(BitmapInfo *fbi, Bitmap *map, BMMRES *status)
{
  *status = ProcessImageIOError(fbi, BMMRES_INTERNALERROR);
  return NULL;
}

BMMRES BitmapIO_TA::GetImageName(BitmapInfo *fbi, TCHAR *filename)
{
  //-- Define Number of Frames
  TexAnimFile file;

  std::string s = wideToStr(fbi->Name());
  if (!file.load(s.c_str()))
    return (ProcessImageIOError(fbi, (TCHAR *)strToWide(file.getlasterr()).c_str()));
  int frameCount = file.frm.Count();

  if (!frameCount)
  {

    //-- Check for silent mode
    //-- Display Error Dialog Box
    //-- Log Error

    return (BMMRES_ERRORTAKENCARE);
  }

  //-- Handle Given Frame Number

  int frame = fbi->CurrentFrame();

  if (frame >= frameCount)
  {
    frame %= frameCount;
    if (fbi->SequenceOutBound() == BMM_SEQ_WRAP)
      ; // no-op
    else
    {
      TheManager->Max()->Log()->LogEntry(SYSLOG_ERROR, NO_DIALOG, NULL, _T("frame out of bounds"), fbi->Name());
      return (BMMRES_ERRORTAKENCARE);
    }
  }

  //-- Update Number of Frames

  fbi->SetFirstFrame(0);
  fbi->SetLastFrame(frameCount - 1);

  //-- Get file from IFL file

  if (frame < 0)
    frame = 0;
  _tcscpy(filename, strToWide(file.tex[file.frm[frame]]).c_str());
  check_image_file(filename);

  return BMMRES_SUCCESS;
}

//    Not supported
BMMRES BitmapIO_TA::OpenOutput(BitmapInfo *fbi, Bitmap *map) { return BMMRES_INTERNALERROR; }

//    Not supported
BMMRES BitmapIO_TA::Write(int frame) { return BMMRES_INTERNALERROR; }

int BitmapIO_TA::Close(int flag) { return 1; }
