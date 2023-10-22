#include <max.h>
#include <max.h>
#include <plugapi.h>
#include <ilayermanager.h>
#include <ilayerproperties.h>
#include <utilapi.h>
#include <bmmlib.h>
#include <stdmat.h>
#include <splshape.h>
#include <notetrck.h>
#include <modstack.h>
#include <cs/phyexp.h>
#include <meshnormalspec.h>
#include <impexp.h>
#include "comboBoxHelper.h"
#include "dagor.h"
#include "dagorLogWindow.h"
#include "enumnode.h"
#include "dagfmt.h"
#include "mater.h"
#include "expanim.h"
#include "expanim2.h"
#include "Bones.h"
#if MAX_RELEASE >= 4000
#include "iparamb2.h"
#include "ISkin.h"
#endif
#include "resource.h"
#include "debug.h"
#include "rolluppanel.h"
#include "datablk.h"
#if defined(MAX_RELEASE_R13) && MAX_RELEASE >= MAX_RELEASE_R13
#include <INamedSelectionSetManager.h>
#endif

#include <string>
#include <set>
#include <Shobjidl.h>
#include <Shlobj.h>
#include <sstream>
#include <fstream>

std::string wideToStr(const TCHAR *sw);
M_STD_STRING strToWide(const char *sz);
bool is_default_layer(const ILayer &layer);

#include "cfg.h"

#define ERRMSG_DELAY 3000

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef int FaceNGr[3];

class ExportENCB;

void export_physics(FILE *file, Interface *ip);

void calc_momjs(Interface *ip);

/*
static void debug_tm(Matrix3 &tm) {
  debug("");
  for(int i=0;i<4;++i) {
    Point3 p=tm.GetRow(i);
    debug("%f %f %f",p.x,p.y,p.z);
  }
  debug("");
}
*/


static void spaces_to_underscores(char *string)
{
#if 0
  for (char *cur = string; *cur; cur++)
    if (*cur == ' ')
      *cur = '_';
#endif
}


void scale_matrix(Matrix3 &tm)
{
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  float masterScale = (float)GetSystemUnitScale(UNITS_METERS);
#else
  float masterScale = (float)GetMasterScale(UNITS_METERS);
#endif
  tm.SetTrans(tm.GetTrans() * masterScale);
}


class ExpUtil : public UtilityObj
{
public:
  // Warning! This is enum is stored in the Max file.
  enum class ExportMode
  {
    Standard = 0,
    ObjectsAsDags = 1,
    LayersAsDags = 2,
  };

  IUtil *iu;
  Interface *ip;
  HWND hPanel, hLog;
  ICustEdit *eastart, *eaend;
  ICustEdit *epose, *erote, *escle, *eorte;
  ICustEdit *epose2, *erote2;
  TCHAR exp_fname[256];
  TCHAR exp_anim2_fname[256];
  TCHAR exp_camera_fname[256];
  TCHAR exp_phys_fname[256];
  TCHAR exp_instances_fname[256];
  int expflg;
  ExportMode exportMode;
  float poseps, roteps, scleps, orteps;
  float poseps2, roteps2;
  TimeValue astart, aend;
  bool suppressPrompts;

  ExpUtil();
  void BeginEditParams(Interface *ip, IUtil *iu);
  void EndEditParams(Interface *ip, IUtil *iu);
  void DeleteThis() {}
  void update_ui(HWND);
  void set_expflg(int flg, int);

  void Init(HWND hWnd);
  void Destroy(HWND hWnd);
  int input_exp_fname();
  int input_exp_anim2_fname();
  int input_exp_camera_fname();
  int input_exp_phys_fname();
  int input_exp_instances_fname();
  BOOL export_one_dag(const TCHAR *exp_fn, TCHAR **textures = NULL, int max_textures = 0, TCHAR *default_material = NULL);
  BOOL export_one_dag_cb(ExportENCB &cb, const TCHAR *exp_fn, TCHAR **textures = NULL, int max_textures = 0,
    TCHAR *default_material = NULL);
  BOOL export_dag(TCHAR **textures = NULL, int max_textures = 0, TCHAR *default_material = NULL);
  void export_anim_v2();
  void export_camera_v1();
  void exportPhysics();
  void export_instances();
  void calcMomj();
  BOOL dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam);
  void checkDupesAndSpaces(Tab<INode *> &node_list);
  void errorMessage(TCHAR *msg);
  void warningMessage(TCHAR *msg, TCHAR *title = NULL);

private:
  void exportObjectsAsDagsInternal(const TCHAR *folder, INode &node);
  void exportObjectsAsDags();
  void exportLayerAsDag(const TCHAR *folder, ILayer &layer);
  void exportLayersAsDagsInternal(const TCHAR *folder, ILayer &layer);
  void exportLayersAsDags();

  ToolTipExtender tooltipExtender;
};
static ExpUtil util;

class DagExport : public SceneExport
{
  virtual int ExtCount() { return 1; }
  virtual const MCHAR *Ext(int) { return _T("dag"); }
  virtual const MCHAR *LongDesc() { return GetString(IDS_DAGIMP_LONG); }
  virtual const MCHAR *ShortDesc() { return GetString(IDS_DAGIMP_SHORT); }
  virtual const MCHAR *AuthorName() { return GetString(IDS_AUTHOR); }
  virtual const MCHAR *CopyrightMessage() { return GetString(IDS_COPYRIGHT); }
  virtual const MCHAR *OtherMessage1() { return _T(""); }
  virtual const MCHAR *OtherMessage2() { return _T(""); }
  virtual unsigned int Version() { return 1; }

  virtual void ShowAbout(HWND hWnd) {}

  virtual int DoExport(const MCHAR *name, ExpInterface *ei, Interface *i, BOOL suppressPrompts = FALSE, DWORD options = 0)
  {
    _tcscpy(util.exp_fname, name);
    util.ip = i;

    // Not sure what is the correct solution here.
    // In case of errors we already show a message box but if we report back IMPEXP_FAIL then there
    // will be an additional message box shown to the user with no additional information.
    // So we only report back errors if message prompts are suppressed.
    BOOL success = util.export_dag();
    return (success || !suppressPrompts) ? IMPEXP_SUCCESS : IMPEXP_FAIL;
  }
};

class DagExpCD : public ClassDesc
{
public:
  int IsPublic() { return TRUE; }
  void *Create(BOOL loading) { return new DagExport; }
  const TCHAR *ClassName() { return GetString(IDS_DAGEXP); }
  const MCHAR *NonLocalizedClassName() { return ClassName(); }

  SClass_ID SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
  Class_ID ClassID() { return DAGEXP_CID; }
  const TCHAR *Category() { return _T(""); }

  const TCHAR *InternalName() { return _T("DagExporter"); }
  HINSTANCE HInstance() { return hInstance; }
};
static DagExpCD dagexpcd;

ClassDesc *GetDAGEXPCD() { return &dagexpcd; }

bool force_export_to_dag(const TCHAR *fname, Interface *ip, TCHAR **textures, int max_textures, TCHAR *default_material)
{
  util.ip = ip;
  util.expflg = EXP_MESH | EXP_MAT;
  _tcscpy(util.exp_fname, fname);
#if MAX_RELEASE >= 3000
  SetSaveRequiredFlag();
#else
  SetSaveRequired();
#endif

  util.export_dag(textures, max_textures, default_material);

  util.ip = NULL;
  util.expflg = 0;

  return true;
}

void explog(TCHAR *s, ...)
{
  va_list ap;
  va_start(ap, s);
  DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Note, s, ap);
  va_end(ap);
}

void explogWarning(TCHAR *s, ...)
{
  va_list ap;
  va_start(ap, s);
  DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Warning, s, ap);
  va_end(ap);
}

void explogError(TCHAR *s, ...)
{
  va_list ap;
  va_start(ap, s);
  DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Error, s, ap);
  va_end(ap);
}

class ExpUtilDesc : public ClassDesc
{
public:
  int IsPublic() { return 1; }
  void *Create(BOOL loading = FALSE) { return &util; }
  const TCHAR *ClassName() { return GetString(IDS_EXPUTIL_NAME); }
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
  SClass_ID SuperClassID() { return UTILITY_CLASS_ID; }
  Class_ID ClassID() { return ExpUtil_CID; }
  const TCHAR *Category() { return GetString(IDS_UTIL_CAT); }
  BOOL NeedsToSave() { return TRUE; }
  IOResult Save(ISave *);
  IOResult Load(ILoad *);
};

enum
{
  CH_EXP_FNAME = 1,
  CH_EXPHIDDEN,
  CH_EXPFLG,
  CH_ANIMEPS,
  CH_ANIMRANGE,
  CH_EXP_ANIM2_FNAME,
  CH_EXP_CAMERA_FNAME,
  CH_ANIMEPS2,
  CH_EXP_PHYS_FNAME,
  CH_EXP_MODE,
};

IOResult ExpUtilDesc::Save(ISave *io)
{
  ULONG nw;
  if (util.exp_fname[0])
  {
    io->BeginChunk(CH_EXP_FNAME);

    if (io->WriteCString(util.exp_fname) != IO_OK)
      return IO_ERROR;
    io->EndChunk();
  }
  io->BeginChunk(CH_EXPFLG);
  if (io->Write(&util.expflg, 4, &nw) != IO_OK)
    return IO_ERROR;
  io->EndChunk();
  io->BeginChunk(CH_ANIMRANGE);
  if (io->Write(&util.astart, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.aend, 4, &nw) != IO_OK)
    return IO_ERROR;
  io->EndChunk();
  io->BeginChunk(CH_ANIMEPS);
  if (io->Write(&util.poseps, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.roteps, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.scleps, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.orteps, 4, &nw) != IO_OK)
    return IO_ERROR;
  io->EndChunk();

  io->BeginChunk(CH_ANIMEPS2);
  if (io->Write(&util.poseps2, 4, &nw) != IO_OK)
    return IO_ERROR;
  if (io->Write(&util.roteps2, 4, &nw) != IO_OK)
    return IO_ERROR;
  io->EndChunk();

  if (util.exp_anim2_fname[0])
  {
    io->BeginChunk(CH_EXP_ANIM2_FNAME);
    if (io->WriteCString(util.exp_anim2_fname) != IO_OK)
      return IO_ERROR;
    io->EndChunk();
  }
  if (util.exp_camera_fname[0])
  {
    io->BeginChunk(CH_EXP_CAMERA_FNAME);
    if (io->WriteCString(util.exp_camera_fname) != IO_OK)
      return IO_ERROR;
    io->EndChunk();
  }
  if (util.exp_phys_fname[0])
  {
    io->BeginChunk(CH_EXP_PHYS_FNAME);
    if (io->WriteCString(util.exp_phys_fname) != IO_OK)
      return IO_ERROR;
    io->EndChunk();
  }

  io->BeginChunk(CH_EXP_MODE);
  const BYTE exportMode = (BYTE)util.exportMode;
  if (io->Write(&exportMode, 1, &nw) != IO_OK)
    return IO_ERROR;
  io->EndChunk();

  return IO_OK;
}

IOResult ExpUtilDesc::Load(ILoad *io)
{
  ULONG nr;
  TCHAR *str;
  util.expflg = EXP_DEFAULT;
  util.exportMode = ExpUtil::ExportMode::Standard;
  while (io->OpenChunk() == IO_OK)
  {
    switch (io->CurChunkID())
    {
      case CH_EXP_FNAME:
        if (io->ReadCStringChunk(&str) != IO_OK)
          return IO_ERROR;
        _tcscpy(util.exp_fname, str);
      case CH_EXP_ANIM2_FNAME:
        if (io->ReadCStringChunk(&str) != IO_OK)
          return IO_ERROR;
        _tcscpy(util.exp_anim2_fname, str);
        break;
      case CH_EXP_CAMERA_FNAME:
        if (io->ReadCStringChunk(&str) != IO_OK)
          return IO_ERROR;
        _tcscpy(util.exp_camera_fname, str);
        break;
      case CH_EXP_PHYS_FNAME:
        if (io->ReadCStringChunk(&str) != IO_OK)
          return IO_ERROR;
        _tcscpy(util.exp_phys_fname, str);
        break;
      case CH_EXPHIDDEN: util.expflg |= EXP_HID; break;
      case CH_EXPFLG:
        if (io->Read(&util.expflg, 4, &nr) != IO_OK)
          return IO_ERROR;
        break;
      case CH_ANIMRANGE:
        if (io->Read(&util.astart, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.aend, 4, &nr) != IO_OK)
          return IO_ERROR;
        break;
      case CH_ANIMEPS:
        if (io->Read(&util.poseps, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.roteps, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.scleps, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.orteps, 4, &nr) != IO_OK)
          return IO_ERROR;
        break;
      case CH_ANIMEPS2:
        if (io->Read(&util.poseps2, 4, &nr) != IO_OK)
          return IO_ERROR;
        if (io->Read(&util.roteps2, 4, &nr) != IO_OK)
          return IO_ERROR;
        break;
      case CH_EXP_MODE:
        BYTE exportMode;
        if (io->Read(&exportMode, 1, &nr) != IO_OK)
          return IO_ERROR;
        util.exportMode = (ExpUtil::ExportMode)exportMode;
        break;
    }
    io->CloseChunk();
  }
  util.update_ui(util.hPanel);
  return IO_OK;
}

static ExpUtilDesc utilDesc;
ClassDesc *GetExpUtilCD() { return &utilDesc; }

void ExpUtil::set_expflg(int f, int v)
{
  expflg &= ~f;
  if (v)
    expflg |= f;
}

BOOL ExpUtil::dlg_proc(HWND hw, UINT msg, WPARAM wpar, LPARAM lpar)
{
  switch (msg)
  {
    case WM_INITDIALOG: Init(hw); break;

    case WM_DESTROY: Destroy(hw); break;

    case WM_COMMAND:
    {
      WORD id = LOWORD(wpar);
      switch (id)
      {
        case IDC_EXPORT:
          if (exportMode == ExportMode::Standard)
          {
            if (!input_exp_fname())
              break;
            export_dag();
          }
          else if (exportMode == ExportMode::LayersAsDags)
            exportLayersAsDags();
          else if (exportMode == ExportMode::ObjectsAsDags)
            exportObjectsAsDags();
          break;
        case IDC_EXPORT_ANIM2:
          if (!input_exp_anim2_fname())
            break;
          export_anim_v2();
          break;
        case IDC_EXPORT_CAMERA2:
          if (!input_exp_camera_fname())
            break;
          export_camera_v1();
          break;
        case IDC_EXPORT_PHYS:
          if (!input_exp_phys_fname())
            break;
          exportPhysics();
          break;
        case IDC_CALC_MOMJ: calcMomj(); break;
        case IDC_CALC_MOMJ_ON_EXPORT: set_expflg(EXP_DONT_CALC_MOMJ, !IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPHIDDEN: set_expflg(EXP_HID, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPSEL: set_expflg(EXP_SEL, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPMESH: set_expflg(EXP_MESH, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXP_NO_VNORM: set_expflg(EXP_NO_VNORM, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPLIGHT: set_expflg(EXP_LT, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPCAM: set_expflg(EXP_CAM, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPHELPER: set_expflg(EXP_HLP, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPSPLINE: set_expflg(EXP_SPLINE, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPMATER: set_expflg(EXP_MAT, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPMATEROPT: set_expflg(EXP_MATOPT, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPANIM: set_expflg(EXP_ANI, IsDlgButtonChecked(hw, id)); break;
        case IDC_ARANGE: set_expflg(EXP_ARNG, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPLTARG: set_expflg(EXP_LTARG, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPCTARG: set_expflg(EXP_CTARG, IsDlgButtonChecked(hw, id)); break;
        case IDC_USEKEYS: set_expflg(EXP_UKEYS, IsDlgButtonChecked(hw, id)); break;
        case IDC_USENTKEYS: set_expflg(EXP_UNTKEYS, IsDlgButtonChecked(hw, id)); break;
        case IDC_DONTCHKKEYS: set_expflg(EXP_DONTCHKKEYS, IsDlgButtonChecked(hw, id)); break;
        case IDC_DONT_REDUCE_POS: set_expflg(EXP_DONT_REDUCE_POS, IsDlgButtonChecked(hw, id)); break;
        case IDC_LOOP_ANIM: set_expflg(EXP_LOOPED_ANIM, IsDlgButtonChecked(hw, id)); break;
        case IDC_DONT_REDUCE_ROT: set_expflg(EXP_DONT_REDUCE_ROT, IsDlgButtonChecked(hw, id)); break;
        case IDC_DONT_REDUCE_SCL: set_expflg(EXP_DONT_REDUCE_SCL, IsDlgButtonChecked(hw, id)); break;
        case IDC_ASTART:
        {
          TSTR s;
          s.Resize(64);
          eastart->GetText(STR_DEST(s), 64);
          StringToTime(s, astart);
        }
        break;
        case IDC_AEND:
        {
          TSTR s;
          s.Resize(64);
          eaend->GetText(STR_DEST(s), 64);
          StringToTime(s, aend);
        }
        break;
        case IDC_POSEPS: poseps = epose->GetFloat(); break;
        case IDC_SCLEPS: scleps = escle->GetFloat() / 100; break;
        case IDC_ROTEPS: roteps = DegToRad(erote->GetFloat()); break;
        case IDC_ORTEPS: orteps = DegToRad(eorte->GetFloat()); break;
        case IDC_ORIGIN_LINVEL_EPS: poseps2 = epose2->GetFloat(); break;
        case IDC_ORIGIN_ANGVEL_EPS: roteps2 = DegToRad(erote2->GetFloat()); break;
        case IDC_SET_DAGORPATH:
        {
          TCHAR dir[256];

          _tcscpy(dir, strToWide(dagor_path).c_str());

          ip->ChooseDirectory(hw, GetString(IDS_CHOOSE_DAGOR_PATH), dir);
          if (dir[0])
          {
            set_dagor_path(wideToStr(dir).c_str());
            SetDlgItemText(hw, IDC_DAGORPATH, strToWide(dagor_path).c_str());
          }
        }
        break;

        case IDC_EXPORT_INSTANCES:
          if (!input_exp_instances_fname())
            break;
          export_instances();
          break;

        case IDC_EXPORT_MODE:
          if (HIWORD(wpar) == CBN_SELENDOK)
            exportMode = (ExportMode)ComboBoxHelper::GetSelectedItemData((HWND)lpar, (LPARAM)ExportMode::Standard);
          break;
      }
    }
    break;
    default: return FALSE;
  }
  return TRUE;
}

static INT_PTR CALLBACK ExpUtilDlgProc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return util.dlg_proc(hw, msg, wParam, lParam);
}

static INT_PTR CALLBACK LogDlgProc(HWND hw, UINT msg, WPARAM wpar, LPARAM lpar)
{
  switch (msg)
  {
    case WM_COMMAND:
      switch (LOWORD(wpar))
      {
        case IDC_OPEN_LOG: DagorLogWindow::show(/*reset_position_and_size = */ true); break;

        case IDC_CLEAR_LOG: DagorLogWindow::clearLog(); break;
      }
      break;
    default: return FALSE;
  }
  return TRUE;
}

ExpUtil::ExpUtil()
{
  iu = NULL;
  ip = NULL;
  hPanel = hLog = NULL;
  exp_fname[0] = 0;
  exp_anim2_fname[0] = 0;
  exp_camera_fname[0] = 0;
  exp_phys_fname[0] = 0;
  expflg = EXP_DEFAULT;
  exportMode = ExportMode::Standard;
  astart = GetAnimStart();
  aend = GetAnimEnd();
  poseps = 0.005f;
  scleps = 0.01f;
  roteps = DegToRad(1);
  orteps = DegToRad(1);

  poseps2 = 0.005f;
  roteps2 = DegToRad(1);
  eastart = eaend = NULL;
  epose = erote = escle = eorte = NULL;

  suppressPrompts = false;
}

void ExpUtil::BeginEditParams(Interface *ip, IUtil *iu)
{
  this->iu = iu;
  this->ip = ip;
  hPanel = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_EXPUTIL), ExpUtilDlgProc, GetString(IDS_EXPUTIL_ROLL), 0);
  hLog = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_LOGROLL), LogDlgProc, GetString(IDS_EXPLOG_ROLL), 0); //,APPENDROLL_CLOSED);
}

void ExpUtil::EndEditParams(Interface *ip, IUtil *iu)
{
  this->iu = NULL;
  this->ip = NULL;
  if (hPanel)
  {
    ip->DeleteRollupPage(hPanel);
    hPanel = NULL;
  }
  if (hLog)
  {
    ip->DeleteRollupPage(hLog);
    hLog = NULL;
  }
}

void ExpUtil::update_ui(HWND hw)
{
  if (!hw)
    return;
  CheckDlgButton(hw, IDC_CALC_MOMJ_ON_EXPORT, !(expflg & EXP_DONT_CALC_MOMJ));
  CheckDlgButton(hw, IDC_EXPHIDDEN, expflg & EXP_HID);
  CheckDlgButton(hw, IDC_EXPSEL, expflg & EXP_SEL);
  CheckDlgButton(hw, IDC_EXPMESH, expflg & EXP_MESH);
  CheckDlgButton(hw, IDC_EXP_NO_VNORM, expflg & EXP_NO_VNORM);
  CheckDlgButton(hw, IDC_EXPLIGHT, expflg & EXP_LT);
  CheckDlgButton(hw, IDC_EXPCAM, expflg & EXP_CAM);
  CheckDlgButton(hw, IDC_EXPHELPER, expflg & EXP_HLP);
  CheckDlgButton(hw, IDC_EXPSPLINE, expflg & EXP_SPLINE);
  CheckDlgButton(hw, IDC_EXPMATER, expflg & EXP_MAT);
  CheckDlgButton(hw, IDC_EXPMATEROPT, expflg & EXP_MATOPT);
  CheckDlgButton(hw, IDC_EXPANIM, expflg & EXP_ANI);
  CheckDlgButton(hw, IDC_ARANGE, expflg & EXP_ARNG);
  CheckDlgButton(hw, IDC_EXPLTARG, expflg & EXP_LTARG);
  CheckDlgButton(hw, IDC_EXPCTARG, expflg & EXP_CTARG);
  CheckDlgButton(hw, IDC_USEKEYS, expflg & EXP_UKEYS);
  CheckDlgButton(hw, IDC_USENTKEYS, expflg & EXP_UNTKEYS);
  CheckDlgButton(hw, IDC_DONTCHKKEYS, expflg & EXP_DONTCHKKEYS);
  CheckDlgButton(hw, IDC_DONT_REDUCE_POS, expflg & EXP_DONT_REDUCE_POS);
  CheckDlgButton(hw, IDC_DONT_REDUCE_ROT, expflg & EXP_DONT_REDUCE_ROT);
  CheckDlgButton(hw, IDC_DONT_REDUCE_SCL, expflg & EXP_DONT_REDUCE_SCL);
  CheckDlgButton(hw, IDC_LOOP_ANIM, expflg & EXP_LOOPED_ANIM);

  HWND exportModeComboBox = GetDlgItem(hw, IDC_EXPORT_MODE);
  ComboBox_SetCurSel(exportModeComboBox, ComboBoxHelper::GetItemIndexByData(exportModeComboBox, (LPARAM)exportMode));

  TSTR s;
  TimeToString(astart, s);
  eastart->SetText(s);
  TimeToString(aend, s);
  eaend->SetText(s);
  epose->SetText(poseps, 3);
  escle->SetText(scleps * 100, 2);
  erote->SetText(RadToDeg(roteps), 2);
  eorte->SetText(RadToDeg(orteps), 2);
  epose2->SetText(poseps2, 3);
  erote2->SetText(RadToDeg(roteps2), 2);
  SetDlgItemText(hw, IDC_DAGORPATH, strToWide(dagor_path).c_str());
}

/*
void ExpUtil::update_vars() {
  if(!ip) return;
  util.exphidden=IsDlgButtonChecked(hPanel,IDC_EXPHIDDEN);
}
*/

void ExpUtil::Init(HWND hw)
{
#define geted(e, idc)                    \
  e = GetICustEdit(GetDlgItem(hw, idc)); \
  assert(e)
  geted(eastart, IDC_ASTART);
  geted(eaend, IDC_AEND);
  geted(epose, IDC_POSEPS);
  geted(erote, IDC_ROTEPS);
  geted(escle, IDC_SCLEPS);
  geted(eorte, IDC_ORTEPS);
  geted(epose2, IDC_ORIGIN_LINVEL_EPS);
  geted(erote2, IDC_ORIGIN_ANGVEL_EPS);
#undef geted

  HWND exportModeComboBox = GetDlgItem(hw, IDC_EXPORT_MODE);
  ComboBoxHelper::AddItemWithData(exportModeComboBox, _T("Standard"), (LPARAM)ExportMode::Standard);
  ComboBoxHelper::AddItemWithData(exportModeComboBox, _T("Objects as dags"), (LPARAM)ExportMode::ObjectsAsDags);
  ComboBoxHelper::AddItemWithData(exportModeComboBox, _T("Layers as dags"), (LPARAM)ExportMode::LayersAsDags);

  update_ui(hw);

  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPORT_MODE),
    TSTR(_T("In \"Objects as dags\" export mode objects are exported as separate dag files, in \"Layers as dags\" mode layers are ")
         _T("exported separately.")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPHIDDEN),
    TSTR(_T("When active, export hidden objects as well, otherwise export only visible ones.\n\nIN LAYERS AS DAGS EXPORT MODE:\nWhen ")
         _T("active, export hidden layers and hidden objects inside layers (if those layers/objects should be exported based on ")
         _T("other parameters). Otherwise, exclude hidden layers and objects from the export.")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPSEL),
    TSTR(_T("When active, export only selected objects. Otherwise export all.\n\nIN LAYERS AS DAGS EXPORT MODE:\nWhen active, ")
         _T("process only current layer and children of it. Otherwise process each layer. (The default layer is only exported if ")
         _T("it's the current layer and \"sel\" is active.)")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPMESH), TSTR(_T("Export mesh")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPLIGHT), TSTR(_T("Export light")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPCAM), TSTR(_T("Export camera")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPHELPER), TSTR(_T("Export helper")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXP_NO_VNORM), TSTR(_T("Do not export vertex normals")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPMATER), TSTR(_T("Export materials")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPMATEROPT), TSTR(_T("Enable material optimization")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPSPLINE), TSTR(_T("Export splines")));
  SendMessage(tooltipExtender.GetToolTipHWND(), TTM_SETDELAYTIME, TTDT_AUTOPOP, 32000);
}

void ExpUtil::Destroy(HWND hw)
{
#define reled(e) ReleaseICustEdit(e)
  reled(eastart);
  reled(eaend);
  reled(epose);
  reled(erote);
  reled(escle);
  reled(eorte);
  reled(epose2);
  reled(erote2);
#undef reled
}


int ExpUtil::input_exp_fname()
{
  static TCHAR fname[256];
  _tcscpy(fname, exp_fname);
  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(ofn));
  FilterList fl;
  fl.Append(GetString(IDS_SCENE_FILES));
  fl.Append(_T ("*.dag"));
  TSTR title = GetString(IDS_SAVE_SCENE_TITLE);

  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hPanel;
  ofn.lpstrFilter = fl;
  ofn.lpstrFile = fname;
  ofn.nMaxFile = 256;
  ofn.lpstrInitialDir = _T ("");
  ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
  ofn.lpstrDefExt = _T ("dag");
  ofn.lpstrTitle = title;

tryAgain:
  if (GetSaveFileName(&ofn))
  {
    if (DoesFileExist(fname))
    {
      TSTR buf1;
      buf1.printf(GetString(IDS_FILE_EXISTS), fname);
      if (IDYES != MessageBox(hPanel, buf1, title, MB_YESNO | MB_ICONQUESTION))
        goto tryAgain;
    }
    if (_tcscmp(exp_fname, fname) != 0)
    {
      _tcscpy(exp_fname, fname);
#if MAX_RELEASE >= 3000
      SetSaveRequiredFlag();
#else
      SetSaveRequired();
#endif
    }
    return 1;
  }
  return 0;
}
int ExpUtil::input_exp_anim2_fname()
{
  static TCHAR fname[256];
  _tcscpy(fname, exp_anim2_fname);
  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(ofn));
  FilterList fl;
  fl.Append(GetString(IDS_ANIM2_FILES));
  fl.Append(_T ("*.a2d"));
  TSTR title = GetString(IDS_SAVE_ANIM2_TITLE2);

  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hPanel;
  ofn.lpstrFilter = fl;
  ofn.lpstrFile = fname;
  ofn.nMaxFile = 256;
  ofn.lpstrInitialDir = _T ("");
  ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
  ofn.lpstrDefExt = _T ("a2d");
  ofn.lpstrTitle = title;

tryAgain:
  if (GetSaveFileName(&ofn))
  {
    if (DoesFileExist(fname))
    {
      TSTR buf1;
      buf1.printf(GetString(IDS_FILE_EXISTS), fname);
      if (IDYES != MessageBox(hPanel, buf1, title, MB_YESNO | MB_ICONQUESTION))
        goto tryAgain;
    }
    if (_tcscmp(exp_anim2_fname, fname) != 0)
    {
      _tcscpy(exp_anim2_fname, fname);
      SetSaveRequiredFlag();
    }
    return 1;
  }
  return 0;
}


int ExpUtil::input_exp_camera_fname()
{
  static TCHAR fname[260];

  _tcsncpy(fname, exp_camera_fname, 259);
  fname[259] = 0;

  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(ofn));
  FilterList fl;
  fl.Append(GetString(IDS_CAMERA_FILES));
  fl.Append(_T ("*.cam"));
  TSTR title = GetString(IDS_SAVE_CAMERA_TITLE);

  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hPanel;
  ofn.lpstrFilter = fl;
  ofn.lpstrFile = fname;
  ofn.nMaxFile = 256;
  ofn.lpstrInitialDir = _T ("");
  ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
  ofn.lpstrDefExt = _T ("cam");
  ofn.lpstrTitle = title;

tryAgain:
  if (GetSaveFileName(&ofn))
  {
    if (DoesFileExist(fname))
    {
      TSTR buf1;
      buf1.printf(GetString(IDS_FILE_EXISTS), fname);
      if (IDYES != MessageBox(hPanel, buf1, title, MB_YESNO | MB_ICONQUESTION))
        goto tryAgain;
    }
    if (_tcscmp(exp_camera_fname, fname) != 0)
    {
      _tcscpy(exp_camera_fname, fname);
      SetSaveRequiredFlag();
    }
    return 1;
  }
  return 0;
}


int ExpUtil::input_exp_phys_fname()
{
  static TCHAR fname[260];

  _tcsncpy(fname, exp_phys_fname, 259);
  fname[259] = 0;

  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(ofn));
  FilterList fl;
  fl.Append(GetString(IDS_PHYS_FILES));
  fl.Append(_T ("*.dphys"));
  TSTR title = GetString(IDS_SAVE_PHYS_TITLE);

  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hPanel;
  ofn.lpstrFilter = fl;
  ofn.lpstrFile = fname;
  ofn.nMaxFile = 256;
  ofn.lpstrInitialDir = _T ("");
  ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
  ofn.lpstrDefExt = _T ("cam");
  ofn.lpstrTitle = title;

tryAgain:
  if (GetSaveFileName(&ofn))
  {
    if (DoesFileExist(fname))
    {
      TSTR buf1;
      buf1.printf(GetString(IDS_FILE_EXISTS), fname);
      if (IDYES != MessageBox(hPanel, buf1, title, MB_YESNO | MB_ICONQUESTION))
        goto tryAgain;
    }
    if (_tcscmp(exp_phys_fname, fname) != 0)
    {
      _tcscpy(exp_phys_fname, fname);
      SetSaveRequiredFlag();
    }
    return 1;
  }
  return 0;
}


int ExpUtil::input_exp_instances_fname()
{
  static TCHAR fname[260];

  _tcsncpy(fname, exp_instances_fname, 259);
  fname[259] = 0;

  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(ofn));
  FilterList fl;
  fl.Append(_T("Instances placement (*.blk)"));
  fl.Append(_T("*.blk"));
  TSTR title = _T("Save instances placement...");

  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hPanel;
  ofn.lpstrFilter = fl;
  ofn.lpstrFile = fname;
  ofn.nMaxFile = 256;
  ofn.lpstrInitialDir = _T ("");
  ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
  ofn.lpstrDefExt = _T ("cam");
  ofn.lpstrTitle = title;

tryAgain:
  if (GetSaveFileName(&ofn))
  {
    if (DoesFileExist(fname))
    {
      TSTR buf1;
      buf1.printf(GetString(IDS_FILE_EXISTS), fname);
      if (IDYES != MessageBox(hPanel, buf1, title, MB_YESNO | MB_ICONQUESTION))
        goto tryAgain;
    }
    if (_tcscmp(exp_instances_fname, fname) != 0)
    {
      _tcscpy(exp_instances_fname, fname);
      SetSaveRequiredFlag();
    }
    return 1;
  }
  return 0;
}


struct Block
{
  int ofs;
};

static Tab<Block> blk;
static FILE *fileh = NULL;

static void init_blk(FILE *h)
{
  blk.SetCount(0);
  fileh = h;
}

static int begin_blk(int t)
{
  int n = 4;
  if (fwrite(&n, 4, 1, fileh) != 1)
    return 0;
  if (fwrite(&t, 4, 1, fileh) != 1)
    return 0;
  Block b;
  b.ofs = ftell(fileh);
  n = blk.Count();
  blk.Append(1, &b);
  if (blk.Count() != n + 1)
    return 0;
  return 1;
}

static int end_blk()
{
  int i = blk.Count() - 1;
  if (i < 0)
    return 0;
  int o = ftell(fileh);
  fseek(fileh, blk[i].ofs - 8, SEEK_SET);
  int n = o - blk[i].ofs + 4;
  if (fwrite(&n, 4, 1, fileh) != 1)
    return 0;
  blk.Delete(i, 1);
  fseek(fileh, o, SEEK_SET);
  return 1;
}


static inline void adjwtm(Matrix3 &tm)
{
  MRow *m = tm.GetAddr();
  for (int i = 0; i < 4; ++i)
  {
    float a = m[i][1];
    m[i][1] = m[i][2];
    m[i][2] = a;
  }
  tm.ClearIdentFlag(ROT_IDENT | SCL_IDENT);
}

struct ExpMat
{
  DagMater m;
  TCHAR *name, *classname, *script;
  Mtl *mtl;
  DWORD wirecolor;

  ExpMat() { name = classname = script = NULL; }
};

class MyExpTMAnimCB : public ExpTMAnimCB
{
public:
  INode *node, *pnode, *origin;
  char nonort;

  MyExpTMAnimCB(INode *n, INode *pn, INode *orig)
  {
    // debug ( "n=%p  pn=%p  origin=%p", n, pn, origin );
    node = n;
    pnode = pn;
    origin = orig;
    nonort = 0;
  }
  const TCHAR *get_name()
  {
    if (!node)
      return _T("<NULL>");
    return node->GetName();
  }
  void interp_tm(TimeValue t, Matrix3 &m)
  {
    Matrix3 ntm = get_scaled_stretch_node_tm(node, t);
    adjwtm(ntm);
    Matrix3 ptm;
    if (pnode)
    {
      ptm = get_scaled_stretch_node_tm(pnode, t);
      if (!pnode->IsRootNode())
        adjwtm(ptm);
      m = ntm * Inverse(ptm);
    }
    else if (origin)
    {
      Matrix3 otm;
      otm = get_scaled_stretch_node_tm(origin, t);
      adjwtm(otm);
      m = ntm * Inverse(otm);
    }
    else
      m = ntm;
  }
  void non_orthog_tm(TimeValue t) { nonort = 1; }
};

static int save_pos_anim(FILE *h, Tab<PosKey> &k)
{
#define wr(p, l)                 \
  {                              \
    if (fwrite(p, l, 1, h) != 1) \
      return 0;                  \
  }
  int num = k.Count();
  if (num > 0xFFFF)
    num = 0xFFFF;
  wr(&num, 2);
  if (num == 1)
  {
    wr(&k[0].p, 12);
  }
  else
  {
    for (int i = 0; i < num; ++i)
    {
      DagPosKey d;
      d.t = k[i].t;
      d.p = k[i].p;
      d.i = k[i].i;
      d.o = k[i].o;
      wr(&d, sizeof(d));
    }
  }
  return 1;
#undef wr
}

static int save_rot_anim(FILE *h, Tab<RotKey> &k)
{
#define wr(p, l)                 \
  {                              \
    if (fwrite(p, l, 1, h) != 1) \
      return 0;                  \
  }
  int num = k.Count();
  if (num > 0xFFFF)
    num = 0xFFFF;
  wr(&num, 2);
  if (num == 1)
  {
    Quat q = Conjugate(k[0].p);
    wr(&q, 16);
  }
  else
  {
    for (int i = 0; i < num; ++i)
    {
      DagRotKey d;
      d.t = k[i].t;
      d.p = Conjugate(k[i].p);
      d.i = Conjugate(k[i].i);
      d.o = Conjugate(k[i].o);
      wr(&d, sizeof(d));
    }
  }
  return 1;
#undef wr
}

struct ExpNode
{
  Tab<ExpNode *> child;
  ExpNode *parent;
  ushort id;

  ExpNode(int i)
  {
    id = i;
    parent = NULL;
  }
  ~ExpNode()
  {
    for (int i = 0; i < child.Count(); ++i)
      if (child[i])
        delete (child[i]);
  }
  void add_child(ExpNode *n)
  {
    if (!n)
      return;
    child.Append(1, &n);
    n->parent = this;
  }
};

struct ExpKeyLabel
{
  ushort id;
  TimeValue t;
};

struct ExpNoteTrack
{
  Tab<ExpKeyLabel> kl;
  INode *node;

  ExpNoteTrack(INode *n) { node = n; }
  /*  int operator ==(ExpNoteTrack &a) {
      if(kl.Count()!=a.kl.Count()) return 0;
      int num=kl.Count();
      for(int i=0;i<num;++i)
        if(kl[i].id!=a.kl[i].id || kl[i].t!=a.kl[i].t) return 0;
      return 1;
    }
  */
};

const char origin_lin_vel_node_name[] = "origin_lin_vel_node_name";
const char origin_ang_vel_node_name[] = "origin_ang_vel_node_name";

class ExportENCB : public ENodeCB
{
public:
  TimeValue time;
  Tab<INode *> node;
  Tab<bool> nodeExp; // used in a2d export to denote nodes that should be exported; 'node' contains all nodes
  Tab<ExpMat> mat;
  std::vector<std::set<Mtl *>> mtls;
  Tab<int> matIDtoMatIdx;
  Tab<TCHAR *> tex;
  Tab<TCHAR *> klabel;
  Tab<ExpNoteTrack *> ntrack;
  int max_pkeys, max_rkeys, max_skeys;
  INode *max_pkeys_n, *max_rkeys_n, *max_skeys_n;
  INodeTab nonort_nodes;
  INode *nodeOrigin;
  INode *useIdentityTransformForNode;
  char nonort, nofaces;
  bool hasDegenerateTriangles;
  bool hasNoSmoothing;
  bool hasBigMeshes;
  bool hasNonDagorMaterials;
  bool hasSubSubMaterials;
  bool hasAbsolutePaths;
  bool a2dExported;
  TCHAR *default_material;

  ExportENCB(TimeValue t, bool a2d)
  {
    time = t;
    max_pkeys = max_rkeys = max_skeys = 0;
    max_pkeys_n = max_rkeys_n = max_skeys_n = NULL;
    nonort = 0;
    nofaces = 0;
    hasDegenerateTriangles = false;
    hasNoSmoothing = false;
    hasBigMeshes = false;
    hasNonDagorMaterials = false;
    hasSubSubMaterials = false;
    hasAbsolutePaths = false;
    nodeOrigin = NULL;
    useIdentityTransformForNode = nullptr;
    a2dExported = a2d;
    default_material = NULL;
  }

  ~ExportENCB()
  {
    int i;
    for (i = 0; i < mat.Count(); ++i)
    {
      if (mat[i].name)
        free(mat[i].name);
      if (mat[i].classname)
        free(mat[i].classname);
      if (mat[i].script)
        free(mat[i].script);
    }
    for (i = 0; i < tex.Count(); ++i)
      if (tex[i])
        free(tex[i]);
    for (i = 0; i < klabel.Count(); ++i)
      if (klabel[i])
        free(klabel[i]);
    for (i = 0; i < ntrack.Count(); ++i)
      if (ntrack[i])
        delete (ntrack[i]);
  }

  int add_klabel(const TCHAR *n)
  {
    // DebugPrint("add_klabel <%s>\n",n);
    if (!n)
      return -1;
    if (!n[0])
      return -1;
    for (int i = 0; i < klabel.Count(); ++i)
      if (_tcsicmp(klabel[i], n) == 0)
        return i;
    assert(klabel.Count() != 0xFFFF);
    if (klabel.Count() >= 0xFFFF)
      return -1;
    TCHAR *s = _tcsdup(n);
    assert(s);
    klabel.Append(1, &s);
    // DebugPrint("  added %d\n",klabel.Count());
    return klabel.Count() - 1;
  }

  int get_notetrack(INode *n, Tab<TimeValue> &gk)
  {
    // DebugPrint("get_notetrk for <%s>\n",n->GetName());
    for (; n; n = n->GetParentNode())
    {
      int id = -1;
      for (int i = 0; i < ntrack.Count(); ++i)
        if (ntrack[i]->node == n)
          return i;
      if (n->HasNoteTracks())
      {
        // DebugPrint("notetrk of <%s>\n",n->GetName());
        int num = n->NumNoteTracks();
        if (num > 0)
        {
          for (int ti = 0; ti < num; ++ti)
          {
            DefNoteTrack *nt = (DefNoteTrack *)n->GetNoteTrack(ti);
            if (!nt)
              continue;
            // DebugPrint("notetrk %d/%d\n",ti+1,num);
            for (int i = 0; i < nt->keys.Count(); ++i)
            {
              gk.Append(1, &nt->keys[i]->time);
              if (nt->keys[i]->note.length())
              {
                if (id < 0)
                {
                  assert(ntrack.Count() != 0xFFFF);
                  id = ntrack.Count();
                  ExpNoteTrack *tr = new ExpNoteTrack(n);
                  assert(tr);
                  ntrack.Append(1, &tr);
                }
                ExpKeyLabel k;
                k.t = nt->keys[i]->time;
                k.id = add_klabel(nt->keys[i]->note);
                // DebugPrint("notekey[%d] at %d <%s> (%d)\n",i,k.t,(char*)nt->keys[i]->note,k.id);
                ntrack[id]->kl.Append(1, &k);
              }
            }
          }
        }
      }
      if (id >= 0)
      {
        /*        if(id==ntrack.Count()-1) {
                  for(int i=0;i<id;++i) if(*ntrack[i]==*ntrack[id]) {
                    delete(ntrack[id]);
                    ntrack.Delete(id,1);
                    return i;
                  }
                }
        */
        return id;
      }
    }
    return -1;
  }

  int add_tex(const TCHAR *fn)
  {
    if (!fn)
      return -1;
    if (!fn[0])
      return -1;
    for (int i = 0; i < tex.Count(); ++i)
      if (_tcsicmp(tex[i], fn) == 0)
        return i;
    assert(tex.Count() != 0xFFFF);
    if (tex.Count() >= 0xFFFF)
      return -1;
    TCHAR *s = _tcsdup(fn);
    assert(s);
    tex.Append(1, &s);
    return tex.Count() - 1;
  }

  bool doesScriptMatch(const TCHAR *a, const TCHAR *b)
  {
    std::wstring propsa(a), propsb(b);
    if (propsa.length() != propsb.length())
      return false; // No need to check b entries against a
    size_t pos = 0, prevPos = 0;
    pos = propsa.find(_T("\r\n"), pos);
    wchar_t prop[512];
    while (pos != std::wstring::npos)
    {
      propsa.copy(prop, pos - prevPos, prevPos);
      prop[pos - prevPos] = L'\0';
      size_t posB = propsb.find(prop);
      if (posB == std::wstring::npos)
        return false;
      if (pos < propsa.length() - 2)
        pos += 2;
      if (pos - prevPos == 0)
        break;
      prevPos = pos;
      pos = propsa.find(_T("\r\n"), pos);
    }
    if (pos < propsa.length())
    {
      propsa.copy(prop, propsa.length() - pos, pos);
      prop[propsa.length() - pos] = L'\0';
      size_t posB = propsb.find(prop);
      if (posB == std::wstring::npos)
        return false;
    }
    return true;
  }

  void add_mtl(INode *node, Mtl *mtl, int subm = 0, DWORD wirecolor = 0xFFFFFF)
  {
    if (!mtl)
    {
      int i;
      for (i = 0; i < mat.Count(); ++i)
        if ((mat[i].mtl == NULL || mtls[i].empty()) && mat[i].wirecolor == wirecolor)
          return;
      assert(mat.Count() != 0xFFFF);
      if (mat.Count() >= 0xFFFF)
        return;
      ExpMat e;
      e.m.flags = 0;
      e.mtl = NULL;
      e.wirecolor = wirecolor;
      e.m.amb = e.m.diff = Color(wirecolor);
      e.m.spec = e.m.emis = Color(0, 0, 0);
      e.m.power = 0;
      for (i = 0; i < DAGTEXNUM; ++i)
        e.m.texid[i] = DAGBADMATID;
      e.name = validateMatName(e.name);
      mat.Append(1, &e);
      mtls.push_back(std::set<Mtl *>());
      return;
    }
    int num = mtl->NumSubMtls();
    if (num)
    {
      if (subm)
      {
        explogWarning(_T("'%s' has Multi/Sub-Object material '%s' in Multi/Sub-Object material\r\n"), node->GetName(), mtl->GetName());
        hasSubSubMaterials = true;
        return;
      }
      for (int i = 0; i < num; ++i)
        add_mtl(node, mtl->GetSubMtl(i), 1);
      return;
    }
    Class_ID cid = mtl->ClassID();
    if (cid == Class_ID(DMTL_CLASS_ID, 0))
    {
      explogWarning(_T("'%s' has standard material '%s'\r\n"), node->GetName(), mtl->GetName());
      hasNonDagorMaterials = true;

      assert(mat.Count() != 0xFFFF);
      if (mat.Count() >= 0xFFFF)
        return;
      StdMat *m = (StdMat *)mtl;
      ExpMat e;
      e.m.flags = DAG_MF_16TEX;
      e.mtl = mtl;
      if (m->GetTwoSided())
        e.m.flags |= DAG_MF_2SIDED;
      float si = m->GetSelfIllum(time);
      e.m.amb = m->GetAmbient(time) * (1 - si);
      e.m.diff = m->GetDiffuse(time) * (1 - si);
      e.m.spec = m->GetSpecular(time) * m->GetShinStr(time);
      e.m.emis = m->GetDiffuse(time) * si;
      e.m.power = powf(2.0f, m->GetShininess(time) * 10.0f) * 4.0f;
      e.name = _tcsdup(mtl->GetName());
      e.wirecolor = 0;
      for (int i = 0; i < DAGTEXNUM; ++i)
        e.m.texid[i] = DAGBADMATID;
      Texmap *tex = m->GetSubTexmap(ID_DI);
      if (tex)
        if (tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0))
        {
          BitmapTex *b = (BitmapTex *)tex;

          e.m.texid[0] = add_tex(makeCheckedRelPath(node, mtl, b->GetMapName()));
          if (e.m.texid[0] != DAGBADMATID)
          {
            e.m.amb = e.m.diff = Color(1, 1, 1) * (1 - si);
            e.m.emis = Color(1, 1, 1) * si;
          }
        }
      e.classname = _tcsdup(_T("simple"));
      e.script = _tcsdup(_T("lighting=vltmap"));

      e.name = validateMatName(e.name);

      for (int i = 0; i < mat.Count(); ++i)
      {
        if (mat[i].mtl == mtl || (mat[i].m == e.m && (useMOpt() || !_tcscmp(mat[i].name, e.name)) &&
                                   !_tcscmp(mat[i].classname, e.classname) && doesScriptMatch(mat[i].script, e.script)))
        {
          mtls[i].insert(mtl);
          return;
        }
      }
      mtls.push_back(std::set<Mtl *>());
      mtls.back().insert(mtl);
      mat.Append(1, &e);
    }
    else if (cid == DagorMat_CID || cid == DagorMat2_CID)
    {
      assert(mat.Count() != 0xFFFF);
      if (mat.Count() >= 0xFFFF)
        return;
      IDagorMat *m = (IDagorMat *)mtl->GetInterface(I_DAGORMAT);
      assert(m);
      ExpMat e;
      e.m.flags = DAG_MF_16TEX;
      e.mtl = mtl;
      if (m->get_2sided())
        e.m.flags |= DAG_MF_2SIDED;
      e.m.amb = m->get_amb();
      e.m.diff = m->get_diff();
      e.m.spec = m->get_spec();
      e.m.emis = m->get_emis();
      e.m.power = m->get_power();
      e.classname = _tcsdup(m->get_classname());
      assert(e.classname);
      e.script = _tcsdup(m->get_script());
      assert(e.script);
      e.name = _tcsdup(mtl->GetName());
      e.wirecolor = 0;

      TCHAR filename[MAX_PATH];
      CfgShader::GetCfgFilename(_T("DagorShaders.cfg"), filename);
      CfgShader cfg(filename);
      cfg.GetSettingsParams();

      for (int i = 0; i < DAGTEXNUM; ++i)
      {
        if (i >= cfg.settings_params.size())
        {
          e.m.texid[i] = DAGBADMATID;
          continue;
        }
        M_STD_STRING value = cfg.GetParamValue(CFG_SETTINGS_NAME, cfg.settings_params.at(i));
        M_STD_STRING tex_slot = cfg.settings_params.at(i);

        if (_tcslen(e.classname))
        {
          cfg.GetShaderParams(e.classname);
          for (int j = 0; j < cfg.shader_params.size(); ++j)
            if (cfg.shader_params.at(j) == tex_slot)
            {
              value = cfg.GetParamValue(e.classname, tex_slot.c_str());
              break;
            }
        }

        e.m.texid[i] = (value == _T("")) ? DAGBADMATID : add_tex(makeCheckedRelPath(node, mtl, m->get_texname(i)));
      }

      mtl->ReleaseInterface(I_DAGORMAT, m);

      e.name = validateMatName(e.name);
      for (int i = 0; i < mat.Count(); ++i)
      {
        if (mat[i].mtl == mtl || (mat[i].m == e.m && (useMOpt() || !_tcscmp(mat[i].name, e.name)) &&
                                   !_tcscmp(mat[i].classname, e.classname) && doesScriptMatch(mat[i].script, e.script)))
        {
          mtls[i].insert(mtl);
          return;
        }
      }
      mtls.push_back(std::set<Mtl *>());
      mtls.back().insert(mtl);
      mat.Append(1, &e);
    }
    else
    {
      explogWarning(_T("'%s' has material '%s' of unknown type\r\n"), node->GetName(), mtl->GetName());
      hasNonDagorMaterials = true;
    }
  }

  TCHAR *validateMatName(TCHAR *mat_name)
  {
    DebugPrint(_T("validateMatName <%s>"), mat_name);
    if (mat_name && *mat_name)
      return mat_name;

    TCHAR buf[64];
    int name_idx = 0;
    if (mat_name)
      free(mat_name);
    for (;;)
    {
      bool found = false;
      _stprintf(buf, _T("autoNamedMat_%d"), name_idx);
      for (int i = 0; i < mat.Count(); i++)
        if (_tcscmp(buf, mat[i].name) == 0)
        {
          found = true;
          break;
        }
      if (!found)
        return _tcsdup(buf);
      name_idx++;
    }
    return NULL;
  }

  int proc(INode *n)
  {
    if (!n)
      return ECB_CONT;
    if (_tcsicmp(n->GetName(), _T("ORIGIN")) == 0)
    {
      if ((util.expflg & EXP_SEL) && !n->Selected())
      {
        explog(_T( "skip non-selected origin\r\n" ));
        return ECB_CONT;
      }
      if (!(util.expflg & EXP_HID) && n->IsNodeHidden())
      {
        explog(_T( "skip hidden origin\r\n"));
        return ECB_CONT;
      }

      if (!nodeOrigin)
        explog(_T( "found origin node\r\n"));
      else
        explog(_T( "duplicate origin node found, use last\r\n"));
      nodeOrigin = n;
      return ECB_CONT; // don't export origin itself
    }

    if (a2dExported)
    {
      bool vis = true;

      if (!(util.expflg & EXP_HID))
        if (n->IsNodeHidden())
          vis = false;
      if (util.expflg & EXP_SEL)
        if (!n->Selected())
          vis = false;
      assert(nodeExp.Count() != 0xFFFF);
      nodeExp.Append(1, &vis);
    }
    else
    {

      if (!(util.expflg & EXP_HID))
      {
        if (n->IsNodeHidden())
          return ECB_CONT;
        // if(n->GetVisibility(time)<0) return ECB_CONT;
      }
      if (util.expflg & EXP_SEL)
        if (!n->Selected())
          return ECB_CONT;

      if ((util.expflg & EXP_MAT) && !(util.expflg & EXP_OBJECTS))
        add_mtl(n, n->GetMtl(), 0, n->GetWireColor());

      if (!(util.expflg & EXP_OBJECTS))
        return ECB_CONT;

      // OutputDebugString(n->GetName());
      // OutputDebugString("\n");

      Object *obj = n->EvalWorldState(time).obj;
      if (obj)
      {
        SClass_ID scid = obj->SuperClassID();
        Class_ID cid = obj->ClassID();

        if (scid == GEOMOBJECT_CLASS_ID && cid == Class_ID(TARGET_CLASS_ID, 0) && !(util.expflg & EXP_CAM))
          return ECB_CONT;

        if (cid == Dummy_CID)
          obj = NULL;
        else if (scid == LIGHT_CLASS_ID)
        {
          if (!(util.expflg & EXP_LT))
            if (!n->NumberOfChildren())
              return ECB_CONT;
        }
        else if (scid == SHAPE_CLASS_ID)
        {
          /*
          if(!(util.expflg&EXP_SPLINE))
          if(!n->NumberOfChildren()) return ECB_CONT;
          */
        }
        else if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
        {
          /*
          if(!(util.expflg&EXP_MESH))
          if(!n->NumberOfChildren()) return ECB_CONT;
          */
        }
      }
      if (!obj)
      {
        if (!(util.expflg & EXP_HLP))
          if (!n->NumberOfChildren())
            return ECB_CONT;
      }

      /*      if ( util.expflg & EXP_ANI ) {
              Tab<TimeValue> gk;
              get_notetrack (n, gk);
            }*/
      if (n->Renderable() && !n->GetMtl() && !n->IsGroupHead())
      {
        explogWarning(_T( "'%s' has no material\r\n"), n->GetName());
        hasNonDagorMaterials = true;
      }

      if (util.expflg & EXP_MAT)
        add_mtl(n, n->GetMtl(), 0, n->GetWireColor());
    }

    assert(node.Count() != 0xFFFF);
    if (node.Count() >= 0xFFFF)
      return ECB_CONT;
    node.Append(1, &n);
    return ECB_CONT;
  }

  int getnodeid(INode *n)
  {
    if (!n)
      return -1;
    for (int i = 0; i < node.Count(); ++i)
      if (node[i] == n)
        return i;
    return -1;
  }

  int getmatid(Mtl *m, DWORD wc = 0xFFFFFF)
  {
    if (!m)
    {
      for (int i = 0; i < mat.Count(); ++i)
        if (mtls[i].empty() && mat[i].wirecolor == wc)
          return i;
      return -1;
    }
    for (int i = 0; i < mat.Count(); ++i)
      if (mtls[i].find(m) != mtls[i].end())
        return i;
    return -1;
  }

#define wr(p, l)                   \
  {                                \
    if (l > 0)                     \
      if (fwrite(p, l, 1, h) != 1) \
        return 0;                  \
  }

  /*
bool wr_hlp( const void * p, int l, FILE * h )
{
  int pos = ftell( h );
  if ( pos + l >0xc81 )
  {
    int r = 0xc81 - pos - 1;
    if ( r >= 0 )
    {
      BYTE b = ((BYTE*)p)[ r ];
      debug( "test");
    }
  }

    if(fwrite(p,l,1,h)!=1)
      return false;



  return true;
}

#define wr(p,l) {if(l>0) if(!wr_hlp(p,l,h)) return 0;}*/

#define bblk(id)        \
  {                     \
    if (!begin_blk(id)) \
      return 0;         \
  }
#define eblk        \
  {                 \
    if (!end_blk()) \
      return 0;     \
  }

  bool checkDegenerateTriangle(INode *node, Matrix3 &applied_transform, unsigned int index1, unsigned int index2, unsigned int index3,
    Point3 &vertex1, Point3 &vertex2, Point3 &vertex3);

  const TCHAR *makeCheckedRelPath(INode *node, Mtl *mtl, const TCHAR *absolute_path);

  bool useMOpt() const { return (util.expflg & EXP_MATOPT) && (util.expflg & EXP_MESH); }

  int save_node(ExpNode *enod, FILE *h)
  {

#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
    float masterScale = (float)GetSystemUnitScale(UNITS_METERS);
#else
    float masterScale = (float)GetMasterScale(UNITS_METERS);
#endif

    Matrix3 originTm;
    if (nodeOrigin)
    {
      originTm = get_scaled_stretch_node_tm(nodeOrigin, time);
      adjwtm(originTm);
    }
    else
      originTm.IdentityMatrix();

    bblk(DAG_NODE);
    INode *n = node[enod->id];
    INode *pnode = NULL;
    {
      bblk(DAG_NODE_DATA);
      DagNodeData d;
      uint pid = enod->parent->id;
      if (pid < node.Count())
        pnode = node[pid];
      d.id = enod->id;
      d.cnum = enod->child.Count();
      d.flg = 0;
      if (n->Renderable())
        d.flg |= DAG_NF_RENDERABLE;
      if (n->CastShadows())
        d.flg |= DAG_NF_CASTSHADOW;
      if (n->RcvShadows())
        d.flg |= DAG_NF_RCVSHADOW;
      const ObjectState &os = n->EvalWorldState(time);
      if (os.obj->SuperClassID() == LIGHT_CLASS_ID)
      {
        GenLight *o = (GenLight *)os.obj;
        if (o->GetShadow())
          d.flg |= DAG_NF_CASTSHADOW;
        else
          d.flg &= ~DAG_NF_CASTSHADOW;
      }
      wr(&d, sizeof(d));
      const TCHAR *nm = n->GetName();
      if (nm)
      {
        char *nameCopy = strdup(wideToStr(nm).c_str());

        if (_tcschr(nm, ' '))
        {
          spaces_to_underscores(nameCopy);
          wr(nameCopy, strlen(nameCopy));
        }
        else
        {
          wr(nameCopy, strlen(nameCopy));
        }

        free(nameCopy);
      }
      eblk;
    }

    {
      RollupPanel::correctUserProp(n);

      TSTR s;
      n->GetUserPropBuffer(s);
      //      DataBlock blk;
      //      RollupPanel::saveUserPropBufferToBlk(blk, n);

      // TODO: create a check for presence of billboards

      //      if(blk.paramCount())
      //      {
      //        if(n->GetMtl())
      //        {
      //          IDagorMat *m = (IDagorMat *) n->GetMtl()->GetInterface (I_DAGORMAT);
      //          if(m && (strcmp(m->get_classname(),"billboard_atest")==NULL ||
      //                   strcmp(m->get_classname(),"facing_leaves")==NULL) )
      //          {
      //            blk.setBool("billboard", true );
      //            debug("find billboard material '%s'", m->get_classname());
      //          }
      //        }
      //      }

      std::string scr = wideToStr(s);
      if (useMOpt())
      {
        for (int i = 0; i < mtls.size(); ++i)
        {
          size_t pos = scr.find("apex_interior_material:t=\"");
          if (pos != std::string::npos)
          {
            for (std::set<Mtl *>::iterator mtlIt = mtls[i].begin(); mtlIt != mtls[i].end(); ++mtlIt)
            {
              std::string matName = wideToStr((*mtlIt)->GetName());
              if (matName == wideToStr(mat[i].name))
                continue;
              size_t posVal = scr.find(matName.c_str(), pos, matName.length());
              if (posVal != std::string::npos)
                scr.replace(posVal, matName.length(), wideToStr(mat[i].name));
            }
          }
        }
      }
      bblk(DAG_NODE_SCRIPT);
      wr(scr.c_str(), scr.length());
      eblk;
    }
    Tab<int> subMatIdLUT;
    int unusedSlotIdx = -1;
    if (util.expflg & EXP_MAT)
    {
      Mtl *mtl = n->GetMtl();
      int num = 0;
      if (mtl)
        num = mtl->NumSubMtls();
      if (num)
      {
        bblk(DAG_NODE_MATER);
        if (useMOpt())
        {
          Tab<int> uniqueMatIdx;
          subMatIdLUT.SetCount(num);
          for (int j = 0; j < subMatIdLUT.Count(); ++j)
            subMatIdLUT[j] = -1;
          for (int j = 0; j < num; ++j)
          {
            int id = getmatid(mtl->GetSubMtl(j));
            if (matIDtoMatIdx[id] < 0)
              continue;

            bool found = false;
            for (unsigned short x = 0; x < uniqueMatIdx.Count(); ++x)
              if (uniqueMatIdx[x] == matIDtoMatIdx[id])
              {
                found = true;
                subMatIdLUT[j] = x;
                break;
              }
            if (!found)
            {
              uniqueMatIdx.SetCount(uniqueMatIdx.Count() + 1);
              uniqueMatIdx[uniqueMatIdx.Count() - 1] = matIDtoMatIdx[id];
              if (mtls[matIDtoMatIdx[id]].empty())
                unusedSlotIdx = matIDtoMatIdx[id];
              subMatIdLUT[j] = uniqueMatIdx.Count() - 1;
              wr(&matIDtoMatIdx[id], 2);
            }
          }
        }
        else
          for (int j = 0; j < num; ++j)
          {
            int id = getmatid(mtl->GetSubMtl(j));
            wr(&id, 2);
          }
        eblk;
      }
      else
      {
        num = getmatid(mtl, n->GetWireColor());
        if (num >= 0 && useMOpt())
          num = matIDtoMatIdx[num];
        if (num >= 0)
        {
          bblk(DAG_NODE_MATER);
          wr(&num, 2);
          eblk;
        }
      }
    }
    {
      MyExpTMAnimCB cb(n, pnode, nodeOrigin);
      Tab<PosKey> pos, scl;
      Tab<RotKey> rot;
      ushort ntid = 0xFFFF;
      if (/*util.expflg & EXP_ANI*/ false)
      {
        Tab<TimeValue> gkeys;
        ntid = get_notetrack(n, gkeys);
        Control *ctrl = n->GetTMController();
        // DebugPrint("getting anim for %s\n",(char*)n->GetName());
        if (!get_tm_anim(pos, rot, scl, gkeys, util.expflg & EXP_ARNG ? Interval(util.astart, util.aend) : FOREVER, ctrl, cb,
              GetTicksPerFrame() / 2, util.poseps, cosf(util.roteps), util.scleps, cosf(HALFPI - util.orteps),
              util.expflg & EXP_UKEYS ? 1 : 0, util.expflg & EXP_UNTKEYS ? 1 : 0, util.expflg & EXP_DONTCHKKEYS ? 1 : 0))
          return 0;
      }
      else
      {
        pos.SetCount(1);
        rot.SetCount(1);
        scl.SetCount(1);
        pos[0].t = rot[0].t = scl[0].t = time;
      }
      if (pos.Count() > max_pkeys)
      {
        max_pkeys = pos.Count();
        max_pkeys_n = n;
      }
      if (rot.Count() > max_rkeys)
      {
        max_rkeys = rot.Count();
        max_rkeys_n = n;
      }
      if (scl.Count() > max_skeys)
      {
        max_skeys = scl.Count();
        max_skeys_n = n;
      }

      {
        Matrix3 ntm = get_scaled_stretch_node_tm(n, pos[0].t);
        adjwtm(ntm);
        bblk(DAG_NODE_TM);
        Matrix3 ptm;
        if (n == useIdentityTransformForNode)
        {
          ntm.IdentityMatrix();
          adjwtm(ntm);

          ptm.IdentityMatrix();
        }
        else if (pnode)
        {
          ptm = get_scaled_stretch_node_tm(pnode, pos[0].t);
          if (!pnode->IsRootNode())
            adjwtm(ptm);
        }
        else if (nodeOrigin)
        {
          ptm = originTm;
        }
        else
          ptm.IdentityMatrix();
        /*
                debug ( "node <%s> wtm (rows):", wideToStr(n->GetName()).c_str());
                debug ( "  (%.3f,%.3f,%.3f)\n  (%.3f,%.3f,%.3f)\n  (%.3f,%.3f,%.3f)\n  (%.3f,%.3f,%.3f)",
                        ntm.GetRow(0).x, ntm.GetRow(0).y, ntm.GetRow(0).z, ntm.GetRow(1).x, ntm.GetRow(1).y, ntm.GetRow(1).z,
                        ntm.GetRow(2).x, ntm.GetRow(2).y, ntm.GetRow(2).z, ntm.GetRow(3).x, ntm.GetRow(3).y, ntm.GetRow(3).z );
        */
        ptm = ntm * Inverse(ptm);
        /*
                debug ( "node <%s> tm (rows) local:", wideToStr(n->GetName()).c_str());
                debug ( "  (%.3f,%.3f,%.3f)\n  (%.3f,%.3f,%.3f)\n  (%.3f,%.3f,%.3f)\n  (%.3f,%.3f,%.3f)",
                        ptm.GetRow(0).x, ptm.GetRow(0).y, ptm.GetRow(0).z, ptm.GetRow(1).x, ptm.GetRow(1).y, ptm.GetRow(1).z,
                        ptm.GetRow(2).x, ptm.GetRow(2).y, ptm.GetRow(2).z, ptm.GetRow(3).x, ptm.GetRow(3).y, ptm.GetRow(3).z );
        */
        wr(ptm.GetAddr(), 4 * 3 * 4);
        eblk;
      }
      if (pos.Count() == 1 && rot.Count() == 1 && scl.Count() == 1)
      {
        if (/*ntid>=0 &&*/ ntid < ntrack.Count())
        {
          bblk(DAG_NODE_NOTETRACK);
          wr(&ntid, 2);
          eblk;
        }
      }
      else
      {
        if (cb.nonort)
        {
          nonort_nodes.Append(1, &n);
          nonort = 1;
        }
        bblk(DAG_NODE_ANIM);
        wr(&ntid, 2);
        if (!save_pos_anim(h, pos))
          return 0;
        if (!save_rot_anim(h, rot))
          return 0;
        if (!save_pos_anim(h, scl))
          return 0;
        eblk;
      }
    }
    const ObjectState &os = n->EvalWorldState(time);
    if (os.obj)
    {
      if (os.obj->ClassID() == Dummy_CID)
        ; // no-op
      else if ((util.expflg & EXP_LT) && os.obj->SuperClassID() == LIGHT_CLASS_ID)
      {
        GenLight *o = (GenLight *)os.obj;
        DagLight d;
        DagLight2 d2;
        Color col = o->GetRGBColor(time) * o->GetIntensity(time);
        d.r = col.r;
        d.g = col.g;
        d.b = col.b;
        d.drad = o->GetDecayRadius(time);
        d.range = o->GetAtten(time, ATTEN_END);
        d.decay = o->GetDecayType();
        d2.mult = o->GetIntensity(time);
        d2.falloff = o->GetFallsize(time);
        d2.hotspot = o->GetHotspot(time);
        switch (o->Type())
        {
          case TSPOT_LIGHT:
          case FSPOT_LIGHT: d2.type = DAG_LIGHT_SPOT; break;
          case DIR_LIGHT:
          case TDIR_LIGHT: d2.type = DAG_LIGHT_DIR; break;
          default: d2.type = DAG_LIGHT_OMNI; break;
        }
        bblk(DAG_NODE_OBJ);
        bblk(DAG_OBJ_LIGHT);
        wr(&d, sizeof(d));
        wr(&d2, sizeof(d2));
        eblk;
        eblk;
      }
      else if ((util.expflg & EXP_SPLINE) && os.obj->SuperClassID() == SHAPE_CLASS_ID)
      {
        ShapeObject *shobj = (ShapeObject *)os.obj;
        if (shobj->CanMakeBezier())
        {
          Matrix3 otm;
          otm = get_scaled_object_tm(n, time);
          otm = otm * Inverse(get_scaled_stretch_node_tm(n, time));
          BezierShape shp;
          shobj->MakeBezier(time, shp);
          bblk(DAG_NODE_OBJ);
          bblk(DAG_OBJ_SPLINES);
          int ns = shp.SplineCount();
          wr(&ns, 4);
          for (int si = 0; si < ns; ++si)
          {
            Spline3D &s = *shp.GetSpline(si);
            assert(&s);
            char flg = s.Closed() ? DAG_SPLINE_CLOSED : 0;
            wr(&flg, 1);
            int nk = s.KnotCount();
            wr(&nk, 4);
            for (int ki = 0; ki < nk; ++ki)
            {
              char kt;
              switch (s.GetKnotType(ki))
              {
                case KTYPE_AUTO: kt = 0; break;
                case KTYPE_BEZIER: kt = DAG_SKNOT_BEZIER; break;
                case KTYPE_CORNER: kt = DAG_SKNOT_CORNER; break;
                case KTYPE_BEZIER_CORNER: kt = DAG_SKNOT_BEZIER | DAG_SKNOT_CORNER; break;
                default: kt = DAG_SKNOT_BEZIER | DAG_SKNOT_CORNER; break;
              }
              wr(&kt, 1);
              Point3 p;
              p = (s.GetVert(ki * 3) * masterScale) * otm;
              wr(&p, 3 * 4);
              p = (s.GetVert(ki * 3 + 1) * masterScale) * otm;
              wr(&p, 3 * 4);
              p = (s.GetVert(ki * 3 + 2) * masterScale) * otm;
              wr(&p, 3 * 4);
            }
          }
          eblk;
          eblk;
        }
      }
      else if ((util.expflg & EXP_MESH) && os.obj->SuperClassID() == GEOMOBJECT_CLASS_ID &&
               os.obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
      {
        // DebugPrint("mesh <%s>\n",(char*)n->GetName());

        int modon = 0, numvert = 0;
        bool skinmod = false;
        bool physiqueMod = false;
        bool morpherMod = false;
        struct RestoreModOnExit
        {
          INode *n;
          bool restore_prop_WSM, restore_obj_ref;

          RestoreModOnExit(INode *n_) : n(n_), restore_prop_WSM(false), restore_obj_ref(false) {}
          ~RestoreModOnExit()
          {
            if (restore_prop_WSM)
            {
              IDerivedObject &der = *(IDerivedObject *)n->GetProperty(PROPID_HAS_WSM);
              der.GetModifier(der.NumModifiers() - 1)->EnableMod();
            }
#if MAX_RELEASE >= 4000
            else if (restore_obj_ref)
            {
              IDerivedObject &der = *(IDerivedObject *)n->GetObjectRef();
              der.GetModifier(der.NumModifiers() - 1)->EnableMod();
            }
#endif
          }
        } restore_mod(n);

        if (n->GetProperty(PROPID_HAS_WSM))
        {
          IDerivedObject &der = *(IDerivedObject *)n->GetProperty(PROPID_HAS_WSM);
          if (der.NumModifiers() >= 1)
          {
            Modifier *mod = der.GetModifier(der.NumModifiers() - 1);
            if (mod->ClassID() == BONESMOD_CID)
            {
              modon = mod->IsEnabled();
              mod->DisableMod();
              restore_mod.restore_prop_WSM = modon;
            }
          }
        }

#if MAX_RELEASE >= 4000
        if (!modon)
        {
          Object *obj = n->GetObjectRef();
          if (obj)
            if (obj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
            {
              IDerivedObject &der = *(IDerivedObject *)obj;
              if (der.NumModifiers() >= 1)
              {
                Modifier *mod = der.GetModifier(der.NumModifiers() - 1);
                assert(mod);

                if (mod->ClassID() == Class_ID(0x17bb6854, 0xa5cba2a3))
                {
                  explog(_T( "%s: Morpher mod!\r\n"), n->GetName());
                  morpherMod = true;
                  modon = mod->IsEnabled();
                  mod->DisableMod();
                  restore_mod.restore_obj_ref = modon;
                }
                else if (mod->GetInterface(I_SKIN))
                {
                  explog(_T( "%s: skin mod!\r\n"), n->GetName());
                  skinmod = true;
                  modon = mod->IsEnabled();
                  mod->DisableMod();
                  restore_mod.restore_obj_ref = modon;
                }
                else if (mod->ClassID() == Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B))
                {
                  explog(_T( "%s: Physique mod!\r\n"), n->GetName());
                  physiqueMod = true;
                  modon = mod->IsEnabled();
                  mod->DisableMod();
                  restore_mod.restore_obj_ref = modon;
                }
              }
            }
        }
#endif

        TriObject *tri = (TriObject *)n->EvalWorldState(time).obj->ConvertToType(time, Class_ID(TRIOBJ_CLASS_ID, 0));
        if (tri)
        {
          bblk(DAG_NODE_OBJ);
          Mesh m = tri->mesh;

          if (tri != os.obj)
            delete tri;
          Matrix3 otm;
          int i, notSmoothed = 0;
          ;
          mesh_face_sel(m).ClearAll();

          for (i = 0; i < m.numFaces; ++i)
          {
            if (!m.faces[i].smGroup)
            {
              mesh_face_sel(m).Set(i);
              notSmoothed++;
            }
          }

          if (notSmoothed > 0)
          {
            explogWarning(_T( "Object '%s' has no smoothing on %d faces!\r\n" ), n->GetName(), notSmoothed);
            m.AutoSmooth(25.0f * 3.1415926f / 180.0f, TRUE);
            hasNoSmoothing |= true;
            mesh_face_sel(m).ClearAll();
          }
          // if(os.GetTM()) otm=*os.GetTM();
          // else otm.IdentityMatrix();
          int v1 = 2, v2 = 1;
          otm = get_scaled_object_tm(n, time);
          if (otm.Parity())
          {
            int a = v1;
            v1 = v2;
            v2 = a;
          }
          otm = otm * Inverse(get_scaled_stretch_node_tm(n, time));
          //          if(!otm.Parity()) {int a=v1;v1=v2;v2=a;}
          numvert = m.numVerts;
          for (i = 0; i < m.numVerts; ++i)
            m.verts[i] = (m.verts[i] * masterScale) * otm;

          if (m.numFaces == 0 && n->Renderable())
          {
            explogWarning(_T( "Renderable object '%s' has 0 faces!\r\n"), n->GetName());
            nofaces = 1;
          }

#if MAX_RELEASE >= 3000
          int maxntv = 0;
          for (i = 0; i < MAX_MESHMAPS; ++i)
            if (m.mapSupport(i))
              if (m.mapFaces(i))
              {
                int ntv = m.getNumMapVerts(i);
                if (ntv > maxntv)
                  maxntv = ntv;
              }
#else
          int maxntv = m.numTVerts;
          if (m.numCVerts > maxntv)
            maxntv = m.numCVerts;
#endif
          if (m.numVerts >= 0x10000 || m.numFaces >= 0x10000 || maxntv >= 0x10000)
          {

            /*
            if (m.numVerts >= 0x10000)
              explog(_T( "'%s' has more than 64K vertices\r\n"), n->GetName());

            if (m.numFaces >= 0x10000)
              explog(_T( "'%s' has more than 64K faces\r\n"), n->GetName());

            if (maxntv >= 0x10000)
              explog(_T( "'%s' has more than 64K texture vertices\r\n"), n->GetName());
            */

            hasBigMeshes = true;

            bblk(DAG_OBJ_BIGMESH);
            wr(&m.numVerts, 4);
            int i;
            for (i = 0; i < m.numVerts; ++i)
              wr(&m.verts[i], 12);
            wr(&m.numFaces, 4);
            for (i = 0; i < m.numFaces; ++i)
            {
              DagBigFace f;
              f.v[0] = m.faces[i].v[0];
              f.v[v1] = m.faces[i].v[1];
              f.v[v2] = m.faces[i].v[2];
              f.smgr = m.faces[i].smGroup;
              MtlID mid = m.faces[i].getMatID();
              if (useMOpt() && n->GetMtl())
              {
                Mtl *mtl = n->GetMtl();
                int num = mtl->NumSubMtls();
                if (num)
                {
                  mid = subMatIdLUT[mid % num];
                  if (mid < 0 && unusedSlotIdx >= 0)
                    mid = (unsigned short)unusedSlotIdx;
                }
                else
                {
                  int id = getmatid(mtl);
                  if (id >= 0)
                    mid = matIDtoMatIdx[id];
                }
              }

              f.mat = mid;
              wr(&f, sizeof(f));

              hasDegenerateTriangles |= checkDegenerateTriangle(n, otm, m.faces[i].v[0], m.faces[i].v[1], m.faces[i].v[2],
                m.verts[m.faces[i].v[0]], m.verts[m.faces[i].v[1]], m.verts[m.faces[i].v[2]]);
            }
            unsigned char numch = 0;
#if MAX_RELEASE >= 3000
            int ch;
            for (ch = 0; ch < MAX_MESHMAPS; ++ch)
              if (m.mapSupport(ch))
                if (m.mapFaces(ch))
                  if (m.getNumMapVerts(ch) > 0)
                    ++numch;
            wr(&numch, 1);
            for (ch = 0; ch < MAX_MESHMAPS; ++ch)
            {
              if (!m.mapSupport(ch))
                continue;
              TVFace *tf = m.mapFaces(ch);
              if (!tf)
                continue;
              int ntv = m.getNumMapVerts(ch);
              if (ntv <= 0)
                continue;
              wr(&ntv, 4);
              wr(ch == 0 ? "\3" : "\2", 1);
              wr(&ch, 1);
              Point3 *tv = m.mapVerts(ch);
              for (i = 0; i < ntv; ++i)
                wr(&tv[i], ch == 0 ? 12 : 8);
              for (i = 0; i < m.numFaces; ++i)
              {
                DagBigTFace f;
                f.t[0] = tf[i].t[0];
                f.t[v1] = tf[i].t[1];
                f.t[v2] = tf[i].t[2];
                wr(&f, sizeof(f));
              }
            }
#else
            if (m.numTVerts && m.tvFace)
              ++numch;
            if (m.numCVerts && m.vcFace)
              ++numch;
            wr(&numch, 1);
            if (m.numTVerts && m.tvFace)
            {
              wr(&m.numTVerts, 4);
              numch = 2;
              wr(&numch, 1);
              numch = 1;
              wr(&numch, 1);
              for (i = 0; i < m.numTVerts; ++i)
                wr(&m.tVerts[i], 8);
              for (i = 0; i < m.numFaces; ++i)
              {
                DagBigTFace f;
                f.t[0] = m.tvFace[i].t[0];
                f.t[v1] = m.tvFace[i].t[1];
                f.t[v2] = m.tvFace[i].t[2];
                wr(&f, sizeof(f));
              }
            }
            if (m.numCVerts && m.vcFace)
            {
              wr(&m.numCVerts, 4);
              numch = 2;
              wr(&numch, 1);
              numch = 2;
              wr(&numch, 1);
              for (i = 0; i < m.numCVerts; ++i)
                wr(&m.vertCol[i], 8);
              for (i = 0; i < m.numFaces; ++i)
              {
                DagBigTFace f;
                f.t[0] = m.vcFace[i].t[0];
                f.t[v1] = m.vcFace[i].t[1];
                f.t[v2] = m.vcFace[i].t[2];
                wr(&f, sizeof(f));
              }
            }
#endif
            eblk;
          }
          else
          {
            bblk(DAG_OBJ_MESH);
            wr(&m.numVerts, 2);
            int i;
            for (i = 0; i < m.numVerts; ++i)
              wr(&m.verts[i], 12);
            wr(&m.numFaces, 2);
            for (i = 0; i < m.numFaces; ++i)
            {
              DagFace f;
              f.v[0] = (unsigned short)m.faces[i].v[0];
              f.v[v1] = (unsigned short)m.faces[i].v[1];
              f.v[v2] = (unsigned short)m.faces[i].v[2];
              f.smgr = m.faces[i].smGroup;
              MtlID mid = m.faces[i].getMatID();
              if (useMOpt() && n->GetMtl())
              {
                Mtl *mtl = n->GetMtl();
                int num = mtl->NumSubMtls();
                if (num)
                {
                  mid = subMatIdLUT[mid % num];
                  if (mid < 0 && unusedSlotIdx >= 0)
                    mid = (unsigned short)unusedSlotIdx;
                }
                else
                {
                  int id = getmatid(mtl);
                  if (id >= 0)
                    mid = matIDtoMatIdx[id];
                }
              }
              f.mat = mid;
              wr(&f, sizeof(f));

              hasDegenerateTriangles |= checkDegenerateTriangle(n, otm, m.faces[i].v[0], m.faces[i].v[1], m.faces[i].v[2],
                m.verts[m.faces[i].v[0]], m.verts[m.faces[i].v[1]], m.verts[m.faces[i].v[2]]);
            }
            unsigned char numch = 0;
#if MAX_RELEASE >= 3000
            int ch;
            for (ch = 0; ch < MAX_MESHMAPS; ++ch)
              if (m.mapSupport(ch))
                if (m.mapFaces(ch))
                  if (m.getNumMapVerts(ch) > 0)
                    ++numch;
            wr(&numch, 1);
            for (ch = 0; ch < MAX_MESHMAPS; ++ch)
            {
              if (!m.mapSupport(ch))
                continue;
              TVFace *tf = m.mapFaces(ch);
              if (!tf)
                continue;
              int ntv = m.getNumMapVerts(ch);
              if (ntv <= 0)
                continue;
              wr(&ntv, 2);
              wr(ch == 0 ? "\3" : "\2", 1);
              wr(&ch, 1);
              Point3 *tv = m.mapVerts(ch);
              for (i = 0; i < ntv; ++i)
                wr(&tv[i], ch == 0 ? 12 : 8);
              for (i = 0; i < m.numFaces; ++i)
              {
                DagTFace f;
                f.t[0] = (unsigned short)tf[i].t[0];
                f.t[v1] = (unsigned short)tf[i].t[1];
                f.t[v2] = (unsigned short)tf[i].t[2];
                wr(&f, sizeof(f));
              }
            }
#else
            if (m.numTVerts && m.tvFace)
              ++numch;
            if (m.numCVerts && m.vcFace)
              ++numch;
            wr(&numch, 1);
            if (m.numTVerts && m.tvFace)
            {
              wr(&m.numTVerts, 2);
              numch = 2;
              wr(&numch, 1);
              numch = 1;
              wr(&numch, 1);
              for (i = 0; i < m.numTVerts; ++i)
                wr(&m.tVerts[i], 8);
              for (i = 0; i < m.numFaces; ++i)
              {
                DagTFace f;
                f.t[0] = (unsigned short)m.tvFace[i].t[0];
                f.t[v1] = (unsigned short)m.tvFace[i].t[1];
                f.t[v2] = (unsigned short)m.tvFace[i].t[2];
                wr(&f, sizeof(f));
              }
            }
            if (m.numCVerts && m.vcFace)
            {
              wr(&m.numCVerts, 2);
              numch = 2;
              wr(&numch, 1);
              numch = 2;
              wr(&numch, 1);
              for (i = 0; i < m.numCVerts; ++i)
                wr(&m.vertCol[i], 8);
              for (i = 0; i < m.numFaces; ++i)
              {
                DagTFace f;
                f.t[0] = (unsigned short)m.vcFace[i].t[0];
                f.t[v1] = (unsigned short)m.vcFace[i].t[1];
                f.t[v2] = (unsigned short)m.vcFace[i].t[2];
                wr(&f, sizeof(f));
              }
            }
#endif
            eblk;
          }

          bblk(DAG_OBJ_FACEFLG);
          for (i = 0; i < m.numFaces; ++i)
          {
            Face &f = m.faces[i];
            char ef = 0;
            if (f.flags & EDGE_B)
              ef |= DAG_FACEFLG_EDGE1;
            if (v1 == 1)
            {
              if (f.flags & EDGE_A)
                ef |= DAG_FACEFLG_EDGE0;
              if (f.flags & EDGE_C)
                ef |= DAG_FACEFLG_EDGE2;
            }
            else
            {
              if (f.flags & EDGE_A)
                ef |= DAG_FACEFLG_EDGE2;
              if (f.flags & EDGE_C)
                ef |= DAG_FACEFLG_EDGE0;
            }
            if (f.flags & FACE_HIDDEN)
              ef |= DAG_FACEFLG_HIDDEN;
            wr(&ef, 1);
          }
          eblk;


          if (m.numFaces > 0)
          {

            MeshNormalSpec *normalSpec = m.GetSpecifiedNormals();
            if (normalSpec && !(util.expflg & EXP_NO_VNORM) && normalSpec->GetNumFaces() == m.numFaces && normalSpec->GetNumNormals())
            {
              Tab<FaceNGr> fngr;
              normalSpec->BuildNormals();
              normalSpec->ComputeNormals();
              int numNormals = normalSpec->GetNumNormals();
              fngr.Resize(m.numFaces);
              fngr.SetCount(m.numFaces);
              int vid[3] = {0, v1, v2};
              for (int face = 0; face < m.numFaces; face++)
                for (int vertex = 0; vertex < 3; vertex++)
                  fngr[face][vid[vertex]] = normalSpec->Face(face).GetNormalID(vertex);

              bblk(DAG_OBJ_NORMALS);
              wr(&numNormals, 4);
              wr(normalSpec->GetNormalArray(), numNormals * 12);
              wr(fngr.Addr(0), m.numFaces * 3 * 4);
              eblk;
            }
          }
        }

        if (modon)
        {
          if (morpherMod)
          {
            IDerivedObject &der = *(IDerivedObject *)n->GetObjectRef();
            Modifier *mod = der.GetModifier(der.NumModifiers() - 1);

            bblk(DAG_OBJ_MORPH);

            unsigned char num = 0;

            for (int i = 0; i < 100; ++i)
            {
              RefTargetHandle ref = mod->GetReference(101 + i);
              if (!ref)
                continue;
              ++num;
            }

            wr(&num, 1);

            for (int i = 0; i < 100; ++i)
            {
              RefTargetHandle ref = mod->GetReference(101 + i);
              if (!ref)
                continue;

              TSTR name = mod->SubAnimName(1 + i);

              unsigned char len;
              if (name.Length() >= 255)
                len = 255;
              else
                len = name.Length();

              wr(&len, 1);
              wr(name.data(), len);

              unsigned short nodeId = getnodeid((INode *)ref);
              wr(&nodeId, sizeof(nodeId));
            }

            eblk;
          }
          else if (physiqueMod)
          {
            IDerivedObject &der = *(IDerivedObject *)n->GetObjectRef();
            Modifier *mod = der.GetModifier(der.NumModifiers() - 1);

            IPhysiqueExport *phyExport = (IPhysiqueExport *)mod->GetInterface(I_PHYINTERFACE);
            IPhyContextExport *contextExport = (IPhyContextExport *)phyExport->GetContextInterface(n);

            contextExport->ConvertToRigid();
            contextExport->AllowBlending();

            int numv = contextExport->GetNumberVertices();

            // get bone nodes and weights
            Tab<INode *> bones;
            Tab<float> wt;

            for (int i = 0; i < numv; ++i)
            {
              IPhyVertexExport *vtx = (IPhyVertexExport *)contextExport->GetVertexInterface(i);
              if (!vtx)
                continue;

              if (vtx->GetVertexType() == RIGID_TYPE)
              {
                // rigid case
                IPhyRigidVertex *v = (IPhyRigidVertex *)vtx;

                INode *bnode = v->GetNode();

                int j;
                for (j = 0; j < bones.Count(); ++j)
                  if (bones[j] == bnode)
                    break;

                if (j >= bones.Count())
                {
                  wt.SetCount((bones.Count() + 1) * numv);
                  memset(&wt[bones.Count() * numv], 0, sizeof(float) * numv);
                  bones.Append(1, &bnode);
                }

                wt[j * numv + i] = 1;
              }
              else if (vtx->GetVertexType() == RIGID_BLENDED_TYPE)
              {
                // blended case
                IPhyBlendedRigidVertex *v = (IPhyBlendedRigidVertex *)vtx;

                int num = v->GetNumberNodes();

                for (int wi = 0; wi < num; ++wi)
                {
                  INode *bnode = v->GetNode(wi);

                  int j;
                  for (j = 0; j < bones.Count(); ++j)
                    if (bones[j] == bnode)
                      break;

                  if (j >= bones.Count())
                  {
                    wt.SetCount((bones.Count() + 1) * numv);
                    memset(&wt[bones.Count() * numv], 0, sizeof(float) * numv);
                    bones.Append(1, &bnode);
                  }

                  wt[j * numv + i] = v->GetWeight(wi);
                }
              }

              contextExport->ReleaseVertexInterface(vtx);
            }

            // write skinning data
            if (bones.Count() > 0 && numv == numvert)
            {
              bblk(DAG_OBJ_BONES);

              unsigned short numb = bones.Count();
              wr(&numb, 2);

              for (int i = 0; i < numb; ++i)
              {
                DagBone b;
                INode *bn = bones[i];
                b.id = getnodeid(bn);
                if (b.id == 0xFFFF)
                {
                  explogError(_T( "%s: skin refers to missing bone <%s> b.id=%04X\r\n"), n->GetName(), bn ? bn->GetName() : _T(""),
                    b.id);
                  return 0;
                }
                Matrix3 tm;
                if (phyExport->GetInitNodeTM(bn, tm) != MATRIX_RETURNED)
                {
                  explogWarning(_T( "%s: no Physique init tm for %s\r\n"), n->GetName(), bn->GetName());
                  tm.IdentityMatrix();
                }

                tm = bn->GetStretchTM(time) * tm;
                scale_matrix(tm);

                adjwtm(tm);
                tm = tm * Inverse(originTm);

                memcpy(&b.tm, tm.GetAddr(), 4 * 3 * 4);
                wr(&b, sizeof(b));
              }

              wr(&numvert, 4);
              wr(&wt[0], numvert * numb * sizeof(float));

              eblk;

              explog(_T( "%s: exported Physique skinning\r\n"), n->GetName());
            }
            else
            {
              explogWarning(_T( "%s: invalid bones data\r\n"), n->GetName());
            }

            // clean-up
            phyExport->ReleaseContextInterface(contextExport);
            mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
          }
          else if (!skinmod)
          {
            IDerivedObject &der = *(IDerivedObject *)n->GetProperty(PROPID_HAS_WSM);
            Modifier *mod = der.GetModifier(der.NumModifiers() - 1);
            WSMObject *wsm = (WSMObject *)mod->GetInterface(I_BONES_WARP);
            if (wsm)
            {
              Tab<Bone> *bon = (Tab<Bone> *)wsm->GetInterface(I_BONES);
              if (bon)
              {
                ModContext *mc = der.GetModContext(0);
                if (mc)
                  if (mc->localData)
                  {
                    BonesModData &md = *(BonesModData *)mc->localData;
                    int numbones = 0;
                    for (int i = 0; i < bon->Count(); ++i)
                      if ((*bon)[i].node)
                        ++numbones;
                    if (md.VN == numvert && numbones == md.BN)
                    {
                      bblk(DAG_OBJ_BONES);
                      wr(&md.BN, 2);
                      for (int i = 0; i < md.BN; ++i)
                      {
                        DagBone b;
                        INode *bn = (*bon)[md.bone[i].node].node;
                        b.id = getnodeid(bn);
                        if (b.id == 0xFFFF)
                        {
                          explogError(_T( "%s: skin refers to missing bone <%s> b.id=%04X\r\n"), n->GetName(),
                            bn ? bn->GetName() : _T(""), b.id);
                          return 0;
                        }
                        Matrix3 tm = md.bone[i].nodetm;
                        // debug("bone %d:",i);
                        // debug_tm(tm);
                        adjwtm(tm);
                        tm = tm * Inverse(originTm);
                        // debug_tm(tm);
                        memcpy(&b.tm, tm.GetAddr(), 4 * 3 * 4);
                        wr(&b, sizeof(b));
                      }
                      wr(&md.VN, 4);
                      wr(&md.pt[0], md.VN * md.BN * sizeof(float));
                      eblk;
                    }
                    else
                      explogWarning(_T( "%s: invalid bones data\r\n"), n->GetName());
                  }
                wsm->ReleaseInterface(I_BONES, bon);
              }
              mod->ReleaseInterface(I_BONES_WARP, wsm);
            }

#if MAX_RELEASE >= 4000
          }
          else
          {
            IDerivedObject &der = *(IDerivedObject *)n->GetObjectRef();
            Modifier *mod = der.GetModifier(der.NumModifiers() - 1);
            ISkin *skin = (ISkin *)mod->GetInterface(I_SKIN);
            if (skin)
            {
              ISkinContextData *ctx = skin->GetContextInterface(n);
              int numb = skin->GetNumBones();
              if (numb > 0 && ctx && ctx->GetNumPoints() == numvert)
              {
                bblk(DAG_OBJ_BONES);
                wr(&numb, 2);
                int i;
                for (i = 0; i < numb; ++i)
                {
                  DagBone b;
                  INode *bn = skin->GetBone(i);
                  b.id = getnodeid(bn);
                  if (b.id == 0xFFFF)
                  {
                    explogError(_T( "%s: skin refers to missing bone <%s> b.id=%04X\r\n"), n->GetName(), bn ? bn->GetName() : _T(""),
                      b.id);
                    return 0;
                  }
                  Matrix3 tm;
                  if (skin->GetBoneInitTM(bn, tm, FALSE) != SKIN_OK)
                  {
                    explogWarning(_T( "%s: no skin init tm for %s\r\n"), n->GetName(), bn->GetName());
                    tm.IdentityMatrix();
                  }

                  tm = bn->GetStretchTM(time) * tm;
                  scale_matrix(tm);

                  /*
                  debug(_T("%s: (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f)"), (TCHAR*) bn->GetName (),
                          tm.GetRow(0).x, tm.GetRow(0).y, tm.GetRow(0).z, tm.GetRow(1).x, tm.GetRow(1).y, tm.GetRow(1).z,
                          tm.GetRow(2).x, tm.GetRow(2).y, tm.GetRow(2).z, tm.GetRow(3).x, tm.GetRow(3).y, tm.GetRow(3).z );
                  //*/
                  // debug("bone %d:",i);
                  // debug_tm(tm);
                  adjwtm(tm);
                  tm = tm * Inverse(originTm);
                  // debug_tm(tm);
                  memcpy(&b.tm, tm.GetAddr(), 4 * 3 * 4);
                  wr(&b, sizeof(b));
                }
                wr(&numvert, 4);
                Tab<float> wt;
                wt.SetCount(numvert * numb);
                memset(&wt[0], 0, wt.Count() * sizeof(float));
                for (i = 0; i < numvert; ++i)
                {
                  int nb = ctx->GetNumAssignedBones(i), j;
                  float sum = 0;
                  for (j = 0; j < nb; ++j)
                  {
                    int b = ctx->GetAssignedBone(i, j);
                    assert(b >= 0 && b < numb);
                    wt[i + b * numvert] = ctx->GetBoneWeight(i, j);
                    sum += wt[i + b * numvert];
                  }

                  if (sum != 0 && fabsf(sum - 1) > 1e-5)
                    for (int j = 0; j < nb; ++j)
                    {
                      int b = ctx->GetAssignedBone(i, j);
                      wt[i + b * numvert] /= sum;
                    }
                }
                wr(&wt[0], numvert * numb * sizeof(float));
                eblk;
              }
              else
                explogWarning(_T( "%s: invalid skin bones data\r\n"), n->GetName());
            }
#endif
          }
        }

        if (tri)
          eblk;
      }
    }

    if (enod->child.Count())
    {
      bblk(DAG_NODE_CHILDREN);
      for (int i = 0; i < enod->child.Count(); ++i)
        if (!save_node(enod->child[i], h))
          return 0;
      eblk;
    }
    eblk;
    return 1;
  }

  //===============================================================================//

  int save(FILE *h, Interface *ip, TCHAR **textures, int max_textures)
  {
    if ((util.expflg & EXP_MATOPT) && (util.expflg & EXP_MESH))
    {
      Tab<bool> matUsed;
      int i;

      matUsed.SetCount(mat.Count());
      matIDtoMatIdx.SetCount(mat.Count());
      for (i = 0; i < matUsed.Count(); ++i)
      {
        matUsed[i] = false;
        matIDtoMatIdx[i] = -1;
      }

      for (i = 0; i < node.Count(); ++i)
      {
        if (!node[i])
          continue;
        const ObjectState &os = node[i]->EvalWorldState(time);
        Mtl *mtl = node[i]->GetMtl();
        if (int mat_num = mtl ? mtl->NumSubMtls() : 0)
        {
          if (os.obj && os.obj->SuperClassID() == GEOMOBJECT_CLASS_ID && os.obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
          {
            if (TriObject *tri = (TriObject *)os.obj->ConvertToType(time, Class_ID(TRIOBJ_CLASS_ID, 0)))
            {
              Mesh m = tri->mesh;
              if (tri != os.obj)
                delete tri;

              for (int j = 0; j < m.numFaces; ++j)
              {
                int mat = m.faces[j].getMatID();
                if (mat >= 0 && mat < mat_num)
                {
                  mat = getmatid(mtl->GetSubMtl(mat));
                  if (mat >= 0)
                    matUsed[mat] = true;
                }
              }
            }
          }
        }
        else
        {
          int mat = getmatid(mtl, node[i]->GetWireColor());
          if (mat >= 0)
            matUsed[mat] = true;
        }
      }

      Tab<int> tex_remap;
      Tab<TCHAR *> new_tex;
      tex_remap.SetCount(tex.Count());
      for (i = 0; i < tex_remap.Count(); ++i)
        tex_remap[i] = -1;

      int idx = 0;
      for (i = 0; i < mat.Count(); ++i)
        if (matUsed[i])
        {
          // remap textures
          for (int j = 0; j < DAGTEXNUM; j++)
            if (mat[i].m.texid[j] != DAGBADMATID)
            {
              unsigned short &texid = mat[i].m.texid[j];
              int new_texid = tex_remap[texid];
              if (new_texid < 0)
                tex_remap[texid] = new_texid = new_tex.Append(1, &tex[texid]);
              texid = new_texid;
            }
          matIDtoMatIdx[i] = idx;
          ++idx;
        }
        else
        {
          explog(_T("mat %s UNUSED\r\n"), mat[i].name);
          // reset unused material props
          if (mat[i].classname)
            free(mat[i].classname);
          if (mat[i].script)
            free(mat[i].script);

          mat[i].classname = mat[i].script = NULL;
          memset(&mat[i].m, 0, sizeof(mat[i].m));
          for (int j = 0; j < DAGTEXNUM; j++)
            mat[i].m.texid[j] = DAGBADMATID;
        }


      for (i = 0; i < tex.Count(); ++i)
        if (tex_remap[i] < 0)
        {
          explog(_T("tex %s UNUSED\r\n"), tex[i]);
          free(tex[i]);
        }
      tex = new_tex;
    }

    {
      int n = DAG_ID;
      wr(&n, 4);
    }
    init_blk(h);
    bblk(DAG_ID);

    // save textures
    if (tex.Count())
    {
      bblk(DAG_TEXTURES);

      int num = tex.Count();
      if (num > 0xFFFF)
        num = 0xFFFF;

      wr(&num, 2);
      for (int i = 0; i < num; ++i)
      {
        char name[256];
        _snprintf(name, 255, wideToStr(tex[i]).c_str());
        size_t l = strlen(name);

        for (char *s = name; *s; ++s)
          if (*s == '\\')
            *s = '/';

        if (!strchr(name, '/'))
        {
          _snprintf(name, 255, "./%s", wideToStr(tex[i]).c_str());
          l = strlen(name);
        }

        if (i < max_textures)
        {
          textures[i] = new TCHAR[256];
          // strcpy(textures[i], name);
          _tcscpy(textures[i], strToWide(name).c_str());

          sprintf(name, "#%d", i);
          l = strlen(name);
        }

        wr(&l, 1);
        if (l)
          wr(name, l);

        //        TSTR s = tex[i];
        //
        //        int l = s.length ();
        //        if ( l > 255 )
        //          l = 255;
        //
        //        for ( int j = 0; j < l; ++j )
        //        {
        //          if ( s[j] == '\\' )
        //            s[j] = '/';
        //        }
        //
        //        wr (&l, 1);
        //        if ( l )
        //          wr ( s, l);
      }
      eblk;
    }

    // save materials
    int i;
    for (i = 0; i < mat.Count(); ++i)
    {
      if (useMOpt() && matIDtoMatIdx[i] < 0)
        continue;
      bblk(DAG_MATER);
      if (mat[i].name)
      {
        std::string s = wideToStr(mat[i].name);
        size_t l = s.length();
        if (l > 255)
          l = 255;
        wr(&l, 1);
        if (l)
          wr(s.c_str(), l);
      }
      else
        wr("\0", 1);

      wr(&mat[i].m, sizeof(DagMater));
      if (mat[i].classname)
      {
        std::string s = wideToStr(mat[i].classname);
        size_t l = s.length();
        if (l > 255)
          l = 255;
        wr(&l, 1);
        if (l)
          wr(s.c_str(), l);
      }
      else
        wr("\0", 1);

      if (mat[i].script)
      {
        std::string s = wideToStr(mat[i].script);
        size_t l = s.length();
        if (l)
          wr(s.c_str(), l);
      }
      eblk;
    }

    // save key labels
    if (klabel.Count())
    {
      bblk(DAG_KEYLABELS);
      int num = klabel.Count();
      if (num > 0xFFFF)
        num = 0xFFFF;
      wr(&num, 2);
      for (int i = 0; i < num; ++i)
      {
        int l = (int)_tcslen(klabel[i]);
        if (l > 255)
          l = 255;
        wr(&l, 1);
        if (l)
          wr(klabel[i], l);
      }
      eblk;
    }

    // save note tracks
    for (i = 0; i < ntrack.Count(); ++i)
    {
      bblk(DAG_NOTETRACK);
      int num = ntrack[i]->kl.Count();
      for (int j = 0; j < num; ++j)
      {
        DagNoteKey k;
        k.id = ntrack[i]->kl[j].id;
        k.t = ntrack[i]->kl[j].t;
        wr(&k, sizeof(k));
      }
      eblk;
    }

    // save nodes
    {
      Tab<ExpNode *> en;
      en.SetCount(node.Count());
      for (i = 0; i < en.Count(); ++i)
      {
        en[i] = new ExpNode(i);
        assert(en[i]);
      }
      ExpNode *root = new ExpNode(-1);
      assert(root);
      for (i = 0; i < node.Count(); ++i)
      {
        uint pid = getnodeid(node[i]->GetParentNode());
        if (pid < node.Count())
        {
          en[pid]->add_child(en[i]);
          // debug ( "  <%s> is linked to <%s>", node[i]->GetName(), node[pid]->GetName());
        }
        else
        {
          // debug ( "  <%s> has no parent! linked to root", node[i]->GetName());
          root->add_child(en[i]);
        }
      }
      bblk(DAG_NODE);
      bblk(DAG_NODE_DATA);
      DagNodeData d;
      d.id = 0xFFFF;
      d.cnum = root->child.Count();
      wr(&d, sizeof(d));
      eblk;
      if (root->child.Count())
      {
        bblk(DAG_NODE_CHILDREN);
        for (i = 0; i < root->child.Count(); ++i)
          if (!save_node(root->child[i], h))
            return 0;
        eblk;
      }
      eblk;
    }

    bblk(DAG_END);
    eblk;

    eblk;
    return 1;
  }

  //
  // Export animation in Anim-v2 format
  //
  struct AnimDataHeader
  {
    unsigned label;    // MAKE4C('A','N','I','M')
    unsigned ver;      // 0x200
    unsigned hdrSize;  // =sizeof(AnimDataHeader)
    unsigned reserved; // 0x00000000

    unsigned namePoolSize;      // size of name pool, in bytes
    unsigned timeNum;           // number of entries in time pool
    unsigned keyPoolSize;       // size of key pool, in bytes
    unsigned totalAnimLabelNum; // number of keys in note track (time labels)
    unsigned dataTypeNum;       // number of data type records
  };
  struct DataTypeHeader
  {
    unsigned dataType;         // id of data type, analogue for channelType: POINT3, QUAT, REAL
    unsigned offsetInKeyPool;  // offset in bytes from the beginning of key pool
    unsigned offsetInTimePool; // offset in elements from the beginning of time pool
    unsigned channelNum;       // number of channels of this data type (see below)
  };

  struct ChannelParams
  {
    unsigned channelType;
    Tab<unsigned> keyNums;
    Tab<int> nodeNameIds;
    Tab<float> nodeWeights;
  };

  enum
  {
    DATATYPE_POINT3 = 0x577084E5u,
    DATATYPE_QUAT = 0x39056464u,
    DATATYPE_REAL = 0xEE0BD66Au,
  };

  enum
  {
    CHTYPE_POSITION = 0xE66CF0CCu,
    CHTYPE_SCALE = 0x48D43D77u,
    CHTYPE_ORIGIN_LIN_VEL = 0x31DE5F48u,
    CHTYPE_ORIGIN_ANG_VEL = 0x09FD7093u,
    CHTYPE_ROTATION = 0x8D490AE4u,
  };

  struct AnimKeyQuat
  {
    float pw, bw;
    float sinpw, sinbw;
    Quat p, b0, b1;
  };
  struct AnimKeyLabel
  {
    union
    {
      char *name;
      int idx;
    };
    int time;
  };

  static void make_postrack(AnimKeyPoint3 *k, PosKey *s, int num)
  {
    for (; num > 1; ++k, ++s, --num)
    {
      k->p = s->p;
      k->k1 = (s->o - s->p) * 3;
      k->k2 = (s->p + s[1].i - s->o * 2) * 3;
      k->k3 = (s->o - s[1].i) * 3 + s[1].p - s->p;
    }
    k->p = s->p;
  }
  static void make_rottrack(AnimKeyQuat *k, RotKey *s, int num)
  {
    char *h = (char *)malloc(num * 2 - 1);
    char *x = h + num - 1;

    int i;
    for (i = 0; i < num - 1; ++i)
    {
      k[i].p = Conjugate(s[i].p);
      Quat b0 = Conjugate(s[i].o), b1 = Conjugate(s[i + 1].i);
      Quat sip = Conjugate(s[i].p), sipn = Conjugate(s[i + 1].p);

      float f = b0.x * b1.x + b0.y * b1.y + b0.z * b1.z + b0.w * b1.w;
      if (f < 0)
      {
        b1 = -b1;
        f = -f;
      }
      k[i].b0 = b0;
      k[i].b1 = b1;
      if (f >= 1)
        k[i].sinbw = k[i].bw = 0;
      else
        k[i].sinbw = sinf(k[i].bw = acosf(f));

      f = sip.x * sipn.x + sip.y * sipn.y + sip.z * sipn.z + sip.w * sipn.w;
      h[i] = f < 0 ? 1 : 0;
      k[i].pw = f;
    }
    k[i].p = Conjugate(s[i].p);
    k[i].pw = 0;

    memset(x, 0, num);

    for (;;)
    {
      for (i = 0; i < num - 1; ++i)
        if (h[i])
          break;
      if (i >= num - 1)
        break;
      for (i = 1; i < num - 1; ++i)
        if (h[i - 1] && h[i])
        {
          h[i - 1] = h[i] = 0;
          x[i] ^= 1;
        }
      if (h[0])
      {
        h[0] = 0;
        x[0] ^= 1;
      }
      for (i = 1; i < num - 1; ++i)
        if (h[i])
        {
          h[i - 1] ^= 1;
          h[i] = 0;
          x[i] ^= 1;
        }
    }

    for (i = 0; i < num; ++i)
      if (x[i])
      {
        k[i].p = -k[i].p;
        k[i].pw = -k[i].pw;
        if (i > 0)
          k[i - 1].pw = -k[i - 1].pw;
      }
    for (i = 0; i < num; ++i)
    {
      if (k[i].pw >= 1)
        k[i].sinpw = k[i].pw = 0;
      else
        k[i].sinpw = sinf(k[i].pw = acosf(k[i].pw));
    }

    free(h);
  }

  int save_anim2(FILE *fp, Interface *ip)
  {
    // pools data
    char *namesPool;
    Tab<int> originLinVelT;
    Tab<int> originAngVelT;
    Tab<AnimKeyPoint3> originLinVel;
    Tab<AnimKeyPoint3> originAngVel;
    Tab<AnimChanPoint3 *> pos;
    Tab<AnimChanPoint3 *> scl;
    Tab<AnimChanQuat *> rot;
    Tab<AnimKeyLabel> noteTrackKeys;

    // get nodes for export
    Tab<INode *> exportNodes;
    for (int i = 0; i < node.Count(); ++i)
      if (nodeExp[i])
        exportNodes.Append(1, &node[i]);

    // process data to prepare for writing
    getNoteTrackKeys(exportNodes, noteTrackKeys);

    getOriginVelocities(originLinVel, originAngVel, originLinVelT, originAngVelT);

    Tab<int> additionalIds;
    Tab<char *> additionalNames;
    int linVelId = -1, angVelId = -1;
    char *str;
    if (originLinVel.Count())
      linVelId = additionalNames.Append(1, (char **)&(str = strdup(origin_lin_vel_node_name)));
    if (originAngVel.Count())
      angVelId = additionalNames.Append(1, (char **)&(str = strdup(origin_ang_vel_node_name)));

    unsigned namePoolSize = 0;
    int *nameIdx = NULL;
    getNamePool(exportNodes, noteTrackKeys, namesPool, nameIdx, namePoolSize, additionalNames, additionalIds);

    if (!getAnimations(node, nodeExp, exportNodes, pos, rot, scl))
      return 0;

    unsigned keyPoolSize = 0;
    unsigned timeNum = 0;
    unsigned keyNum;
    float nodeWeight = 1.0f;

    // Point3
    unsigned maxSizePoint3AnimKey = 0;
    DataTypeHeader point3DataTypeHeader;
    point3DataTypeHeader.dataType = DATATYPE_POINT3;
    point3DataTypeHeader.offsetInKeyPool = 0;
    point3DataTypeHeader.offsetInTimePool = 0;
    point3DataTypeHeader.channelNum = 0;

    ChannelParams originLinVelCannelParams;
    if (originLinVel.Count())
    {
      originLinVelCannelParams.channelType = CHTYPE_ORIGIN_LIN_VEL;
      originLinVelCannelParams.keyNums.Append(1, &(keyNum = originLinVel.Count()));
      originLinVelCannelParams.nodeNameIds.Append(1, &additionalIds[linVelId]);
      originLinVelCannelParams.nodeWeights.Append(1, &nodeWeight);
      if (maxSizePoint3AnimKey < originLinVel.Count())
        maxSizePoint3AnimKey = originLinVel.Count();
      keyPoolSize += sizeof(AnimKeyPoint3) * originLinVel.Count();
      timeNum += originLinVel.Count();
      ++point3DataTypeHeader.channelNum;
    }

    ChannelParams originAngVelCannelParams;
    if (originAngVel.Count())
    {
      originAngVelCannelParams.channelType = CHTYPE_ORIGIN_ANG_VEL;
      originAngVelCannelParams.keyNums.Append(1, &(keyNum = originAngVel.Count()));
      originAngVelCannelParams.nodeNameIds.Append(1, &additionalIds[angVelId]);
      originAngVelCannelParams.nodeWeights.Append(1, &nodeWeight);
      if (maxSizePoint3AnimKey < originAngVel.Count())
        maxSizePoint3AnimKey = originAngVel.Count();
      keyPoolSize += sizeof(AnimKeyPoint3) * originAngVel.Count();
      timeNum += originAngVel.Count();
      ++point3DataTypeHeader.channelNum;
    }

    ChannelParams posCannelParams;
    ChannelParams sclCannelParams;
    posCannelParams.channelType = CHTYPE_POSITION;
    sclCannelParams.channelType = CHTYPE_SCALE;
    for (int i = 0; i < exportNodes.Count(); ++i)
    {
      if ((keyNum = pos[i]->key.Count()) > 0)
      {
        posCannelParams.keyNums.Append(1, &keyNum);
        posCannelParams.nodeNameIds.Append(1, &nameIdx[i]);
        posCannelParams.nodeWeights.Append(1, &nodeWeight);
        if (maxSizePoint3AnimKey < keyNum)
          maxSizePoint3AnimKey = keyNum;
        keyPoolSize += sizeof(AnimKeyPoint3) * keyNum;
        timeNum += keyNum;
      }
      if ((keyNum = scl[i]->key.Count()) > 0)
      {
        sclCannelParams.keyNums.Append(1, &keyNum);
        sclCannelParams.nodeNameIds.Append(1, &nameIdx[i]);
        sclCannelParams.nodeWeights.Append(1, &nodeWeight);
        if (maxSizePoint3AnimKey < keyNum)
          maxSizePoint3AnimKey = keyNum;
        keyPoolSize += sizeof(AnimKeyPoint3) * keyNum;
        timeNum += keyNum;
      }
    }
    if (posCannelParams.keyNums.Count())
      ++point3DataTypeHeader.channelNum;
    if (sclCannelParams.keyNums.Count())
      ++point3DataTypeHeader.channelNum;

    // Quat
    unsigned maxSizeQuatAnimKey = 0;
    DataTypeHeader quatDataTypeHeader;
    quatDataTypeHeader.dataType = DATATYPE_QUAT;
    quatDataTypeHeader.offsetInKeyPool = keyPoolSize;
    quatDataTypeHeader.offsetInTimePool = timeNum;
    quatDataTypeHeader.channelNum = 0;

    ChannelParams rotCannelParams;
    rotCannelParams.channelType = CHTYPE_ROTATION;
    for (int i = 0; i < exportNodes.Count(); ++i)
    {
      if ((keyNum = rot[i]->key.Count()) > 0)
      {
        rotCannelParams.keyNums.Append(1, &keyNum);
        rotCannelParams.nodeNameIds.Append(1, &nameIdx[i]);
        rotCannelParams.nodeWeights.Append(1, &nodeWeight);
        if (maxSizeQuatAnimKey < keyNum)
          maxSizeQuatAnimKey = keyNum;
        keyPoolSize += sizeof(AnimKeyQuat) * keyNum;
        timeNum += keyNum;
      }
    }
    if (rotCannelParams.keyNums.Count())
      ++quatDataTypeHeader.channelNum;

    AnimDataHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.label = 'MINA';
    hdr.ver = 0x200;
    hdr.hdrSize = sizeof(hdr);

    hdr.namePoolSize = namePoolSize;
    hdr.timeNum = timeNum;
    hdr.keyPoolSize = keyPoolSize;
    hdr.totalAnimLabelNum = noteTrackKeys.Count();
    hdr.dataTypeNum = 0; // Point3, Quot
    if (point3DataTypeHeader.channelNum)
      ++hdr.dataTypeNum;
    if (quatDataTypeHeader.channelNum)
      ++hdr.dataTypeNum;

    //
    // write file
    //
    fwrite(&hdr, sizeof(hdr), 1, fp);

    // write names pool
    fwrite(namesPool, hdr.namePoolSize, 1, fp);

    // write times pool
    if (originLinVelT.Count())
      fwrite(originLinVelT.Addr(0), sizeof(int), originLinVelT.Count(), fp);

    if (originAngVelT.Count())
      fwrite(originAngVelT.Addr(0), sizeof(int), originAngVelT.Count(), fp);

    for (int i = 0; i < exportNodes.Count(); ++i)
      for (int j = 0; j < pos[i]->key.Count(); ++j)
        fwrite(&pos[i]->key[j].t, sizeof(int), 1, fp);

    for (int i = 0; i < exportNodes.Count(); ++i)
      for (int j = 0; j < scl[i]->key.Count(); ++j)
        fwrite(&scl[i]->key[j].t, sizeof(int), 1, fp);

    for (int i = 0; i < exportNodes.Count(); ++i)
      for (int j = 0; j < rot[i]->key.Count(); ++j)
        fwrite(&rot[i]->key[j].t, sizeof(int), 1, fp);

    // write keys pool
    //  write pos keys
    AnimKeyPoint3 *point3AnimKey = new AnimKeyPoint3[maxSizePoint3AnimKey];
    //   write origin velocitiy keys
    if (originLinVel.Count())
      fwrite(originLinVel.Addr(0), sizeof(AnimKeyPoint3), originLinVel.Count(), fp);

    if (originAngVel.Count())
      fwrite(originAngVel.Addr(0), sizeof(AnimKeyPoint3), originAngVel.Count(), fp);

    //   write per-node pos keys
    for (int i = 0; i < exportNodes.Count(); ++i)
    {
      if (!pos[i]->key.Count())
        continue;
      memset(point3AnimKey, 0, sizeof(AnimKeyPoint3) * maxSizePoint3AnimKey);
      make_postrack(point3AnimKey, pos[i]->key.Addr(0), pos[i]->key.Count());
      fwrite(point3AnimKey, sizeof(AnimKeyPoint3), pos[i]->key.Count(), fp);
    }

    //  write scl keys
    for (int i = 0; i < exportNodes.Count(); ++i)
    {
      if (!scl[i]->key.Count())
        continue;
      memset(point3AnimKey, 0, sizeof(AnimKeyPoint3) * maxSizePoint3AnimKey);
      make_postrack(point3AnimKey, scl[i]->key.Addr(0), scl[i]->key.Count());
      fwrite(point3AnimKey, sizeof(AnimKeyPoint3), scl[i]->key.Count(), fp);
    }
    delete point3AnimKey;

    //  write rot keys
    AnimKeyQuat *quatAnimKey = new AnimKeyQuat[maxSizeQuatAnimKey];
    for (int i = 0; i < exportNodes.Count(); ++i)
    {
      if (!rot[i]->key.Count())
        continue;
      memset(quatAnimKey, 0, sizeof(AnimKeyQuat) * maxSizeQuatAnimKey);
      make_rottrack(quatAnimKey, rot[i]->key.Addr(0), rot[i]->key.Count());
      fwrite(quatAnimKey, sizeof(AnimKeyQuat), rot[i]->key.Count(), fp);
    }
    delete quatAnimKey;

    // write note track pool
    if (noteTrackKeys.Count())
    {
      for (int i = 0; i < noteTrackKeys.Count(); i++)
      {
        fwrite(&noteTrackKeys[i].idx, sizeof(int), 1, fp);
        fwrite(&noteTrackKeys[i].time, sizeof(int), 1, fp);
      }
    }
    else
      explogWarning(_T("Warning: NoteTrack not used\n" ));

    // write data of animation
    //  write point3 data type
    fwrite(&point3DataTypeHeader, sizeof(point3DataTypeHeader), 1, fp);
    writeChannel(originLinVelCannelParams, fp);
    writeChannel(originAngVelCannelParams, fp);
    writeChannel(posCannelParams, fp);
    writeChannel(sclCannelParams, fp);

    //  write quat data type
    fwrite(&quatDataTypeHeader, sizeof(quatDataTypeHeader), 1, fp);
    writeChannel(rotCannelParams, fp);

    explog(_T(" originLinVelKeyNum=%d\r\n originAngVelKeyNum=%d\r\n"),
      originLinVelCannelParams.keyNums.Count() ? originLinVelCannelParams.keyNums[0] : 0,
      originAngVelCannelParams.keyNums.Count() ? originAngVelCannelParams.keyNums[0] : 0);

    explog(_T(" keyPosNum=%d\r\n keySclNum=%d\r\n keyRotNum=%d\r\n"), getKeyNumInChannel(posCannelParams),
      getKeyNumInChannel(sclCannelParams), getKeyNumInChannel(rotCannelParams));

    explog(_T(" posNodeNum=%d\r\n sclNodeNum=%d\r\n rotNodeNum=%d\r\n"), posCannelParams.keyNums.Count(),
      sclCannelParams.keyNums.Count(), rotCannelParams.keyNums.Count());

    explog(_T(" namePoolSz=%d\r\n"), namePoolSize);
    explog(_T(" timePoolSz=%d\r\n"), timeNum * sizeof(int));
    explog(_T(" keyPoolSz=%d\r\n"), keyPoolSize);

    for (int i = additionalNames.Count(); --i >= 0;)
      delete additionalNames[i];
    for (int i = pos.Count(); --i >= 0;)
      delete pos[i];
    for (int i = scl.Count(); --i >= 0;)
      delete scl[i];
    for (int i = rot.Count(); --i >= 0;)
      delete rot[i];
    delete nameIdx;
    free(namesPool);

    return 1;
  }

#undef wr
#undef bblk
#undef eblk

private:
  void getNoteTrackKeys(Tab<INode *> &exportNodes, Tab<AnimKeyLabel> &noteTrackKeys);

  void getNamePool(Tab<INode *> &exportNodes, Tab<AnimKeyLabel> &noteTrackKeys, char *&namesPool, int *&nameIdx,
    unsigned &namePoolSize, Tab<char *> &additionalNames, Tab<int> &additionalIds);

  bool getAnimations(Tab<INode *> &node, Tab<bool> &nodeExp, Tab<INode *> &exportNodes, Tab<AnimChanPoint3 *> &pos,
    Tab<AnimChanQuat *> &rot, Tab<AnimChanPoint3 *> &scl);

  void getOriginVelocities(Tab<AnimKeyPoint3> &originLinVel, Tab<AnimKeyPoint3> &originAngVel, Tab<int> &originLinVelT,
    Tab<int> &originAngVelT);

  void writeChannel(const ChannelParams &params, FILE *fp);

  int getKeyNumInChannel(const ChannelParams &params);
};

int ExportENCB::getKeyNumInChannel(const ChannelParams &params)
{
  int count = 0;
  for (int i = params.keyNums.Count(); --i >= 0;)
    count += params.keyNums[i];
  return count;
}

void ExportENCB::getNoteTrackKeys(Tab<INode *> &exportNodes, Tab<AnimKeyLabel> &noteTrackKeys)
{
  DefNoteTrack *noteTrack = NULL;
  for (int i = 0; i < exportNodes.Count(); ++i)
  {
    if (!exportNodes[i]->HasNoteTracks())
      continue;
    for (int j = 0; j < exportNodes[i]->NumNoteTracks(); ++j)
    {
      if ((noteTrack = (DefNoteTrack *)exportNodes[i]->GetNoteTrack(j)) == NULL)
        continue;
      for (int k = 0; k < noteTrack->keys.Count(); ++k)
      {
        NoteKey *key = noteTrack->keys[k];
        AnimKeyLabel label;
        label.name = strdup(wideToStr(key->note).c_str());
        label.time = key->time;
        noteTrackKeys.Append(1, &label, 32);
      }
    }
  }
}

void ExportENCB::getNamePool(Tab<INode *> &exportNodes, Tab<AnimKeyLabel> &noteTrackKeys, char *&namesPool, int *&nameIdx,
  unsigned &namePoolSize, Tab<char *> &additionalNames, Tab<int> &additionalIds)
{
  additionalIds.SetCount(additionalNames.Count());
  // prepare nodes name pool
  namePoolSize = 1;
  int len;
  for (int i = 0; i < exportNodes.Count(); ++i)
    if ((len = exportNodes[i]->GetName() ? (int)_tcslen(exportNodes[i]->GetName()) : 0) != 0)
      namePoolSize += len + 1;
  for (int i = 0; i < noteTrackKeys.Count(); ++i)
    if ((len = (int)strlen(noteTrackKeys[i].name)) != 0)
      namePoolSize += len + 1;
  for (int i = 0; i < additionalNames.Count(); ++i)
    if ((len = (int)strlen(additionalNames[i])) != 0)
      namePoolSize += len + 1;
  namePoolSize = (namePoolSize + 7) & ~7;

  namesPool = (char *)malloc(namePoolSize);
  nameIdx = new int[exportNodes.Count()];
  memset(nameIdx, 0, sizeof(int) * exportNodes.Count());

  namesPool[0] = '\0';
  namePoolSize = 1;
  for (int i = 0; i < exportNodes.Count(); ++i)
  {
    std::string name = wideToStr(exportNodes[i]->GetName());
    if ((len = (int)name.size()) == 0)
      nameIdx[i] = 0;
    else
    {
      nameIdx[i] = namePoolSize;
      char *poolName = namesPool + namePoolSize;
      strcpy(poolName, name.c_str());
      spaces_to_underscores(poolName);
      namePoolSize += len + 1;
    }
  }
  for (int i = 0; i < noteTrackKeys.Count(); ++i)
  {
    char *name = noteTrackKeys[i].name;
    if ((len = (int)strlen(name)) == 0)
      noteTrackKeys[i].idx = 0;
    else
    {
      noteTrackKeys[i].idx = namePoolSize;
      char *poolName = namesPool + namePoolSize;
      strcpy(poolName, name);
      spaces_to_underscores(poolName);
      namePoolSize += len + 1;
    }
    free(name);
  }
  for (int i = 0; i < additionalNames.Count(); ++i)
  {
    char *name = additionalNames[i];
    if ((len = (int)strlen(name)) == 0)
      additionalIds[i] = 0;
    else
    {
      additionalIds[i] = namePoolSize;
      char *poolName = namesPool + namePoolSize;
      strcpy(poolName, name);
      spaces_to_underscores(poolName);
      namePoolSize += len + 1;
    }
  }

  namePoolSize = (namePoolSize + 7) & ~7;
}

bool ExportENCB::getAnimations(Tab<INode *> &node, Tab<bool> &nodeExp, Tab<INode *> &exportNodes, Tab<AnimChanPoint3 *> &pos,
  Tab<AnimChanQuat *> &rot, Tab<AnimChanPoint3 *> &scl)
{
  // build exp nodes
  Tab<ExpNode *> en;
  en.SetCount(node.Count());
  for (int i = 0; i < en.Count(); ++i)
  {
    en[i] = new ExpNode(i);
    assert(en[i]);
  }

  ExpNode *root = new ExpNode(-1);
  assert(root);
  int i;
  for (i = 0; i < node.Count(); ++i)
  {
    uint pid = getnodeid(node[i]->GetParentNode());
    if (pid < node.Count())
      en[pid]->add_child(en[i]);
    else
      root->add_child(en[i]);
  }

  AnimChanPoint3 *animChanPoint3;
  AnimChanQuat *animChanQuat;
  const int exportNodeCount = exportNodes.Count();
  pos.SetCount(exportNodeCount);
  pos.ZeroCount();
  scl.SetCount(exportNodeCount);
  scl.ZeroCount();
  rot.SetCount(exportNodeCount);
  rot.ZeroCount();

  // get animations
  Tab<ExpTMAnimCB *> acb;
  for (i = 0; i < node.Count(); ++i)
  {
    if (!nodeExp[i])
      continue;

    pos.Append(1, &(animChanPoint3 = new AnimChanPoint3));
    scl.Append(1, &(animChanPoint3 = new AnimChanPoint3));
    rot.Append(1, &(animChanQuat = new AnimChanQuat));

    INode *parentNode = NULL;
    uint pid = en[i]->parent->id;
    if (pid < node.Count())
      parentNode = node[pid];

    ExpTMAnimCB *expAnim = new MyExpTMAnimCB(node[i], parentNode, nodeOrigin);
    acb.Append(1, &expAnim);
    // debug ( "acb[%d]: n=%p (en[i]->id=%d) pnode=%p (en[i]->parent->id=%u)", i, n, en[i]->id, pnode, en[i]->parent->id );
  }

  bool ret = get_tm_anim_2(pos, rot, scl, (util.expflg & EXP_ARNG) ? Interval(util.astart, util.aend) : FOREVER, exportNodes, acb,
    util.poseps, cosf(util.roteps), util.scleps, cosf(HALFPI - util.orteps), util.expflg);

  for (int i = acb.Count(); --i >= 0;)
    delete acb[i];
  delete root;

  return ret;
}

void ExportENCB::getOriginVelocities(Tab<AnimKeyPoint3> &originLinVel, Tab<AnimKeyPoint3> &originAngVel, Tab<int> &originLinVelT,
  Tab<int> &originAngVelT)
{
  // get origin velocities
  originLinVel.ZeroCount();
  originAngVel.ZeroCount();
  originLinVelT.ZeroCount();
  originAngVelT.ZeroCount();
  if (nodeOrigin)
  {
    MyExpTMAnimCB *cb = new MyExpTMAnimCB(nodeOrigin, NULL, NULL);
    if (!get_node_vel(originLinVel, originAngVel, originLinVelT, originAngVelT,
          (util.expflg & EXP_ARNG) ? Interval(util.astart, util.aend) : FOREVER, *cb, util.poseps2, cosf(util.roteps2), util.expflg))
    {
      explogWarning(_T( "cannot get velocity of origin\r\n"));
      debug("cannot get velocity of origin");
      originLinVel.ZeroCount();
      originAngVel.ZeroCount();
      originLinVelT.ZeroCount();
      originAngVelT.ZeroCount();
    }
    delete cb;
  }
}

void ExportENCB::writeChannel(const ChannelParams &params, FILE *fp)
{
  unsigned nodeNum = params.keyNums.Count();
  if (nodeNum)
  {
    fwrite(&params.channelType, sizeof(unsigned), 1, fp);
    fwrite(&nodeNum, sizeof(unsigned), 1, fp);
    fwrite(params.keyNums.Addr(0), sizeof(unsigned), params.keyNums.Count(), fp);
    fwrite(params.nodeNameIds.Addr(0), sizeof(int), params.nodeNameIds.Count(), fp);
    fwrite(params.nodeWeights.Addr(0), sizeof(float), params.nodeWeights.Count(), fp);
  }
}


void ExpUtil::errorMessage(TCHAR *msg)
{
  ip->DisplayTempPrompt(msg, ERRMSG_DELAY);

  if (!suppressPrompts)
    MessageBox(ip->GetMAXHWnd(), msg, _T ("Error"), MB_OK | MB_ICONSTOP);

  explogError(_T("ERROR! %s\r\n"), msg);
}

void ExpUtil::warningMessage(TCHAR *msg, TCHAR *title)
{
  if (title == NULL)
    title = _T("Warning");

  if (!suppressPrompts)
    MessageBox(ip->GetMAXHWnd(), msg, title, MB_OK | MB_ICONSTOP);

  explogWarning(_T("WARNING! %s\r\n"), msg);
}

#if defined(MAX_RELEASE_R13) && MAX_RELEASE >= MAX_RELEASE_R13
static bool trail_stricmp(const TCHAR *str, const TCHAR *str2)
{
  size_t l = _tcslen(str), l2 = _tcslen(str2);
  return (l >= l2) ? _tcsncicmp(str + l - l2, str2, l2) == 0 : false;
}

static bool find_co_layers(const TCHAR *fname, Tab<TCHAR *> &fnames)
{
  if (!trail_stricmp(fname, _T(".lod00.dag")))
    return false;
  int base_len = int(_tcslen(fname) - _tcslen(_T(".lod00.dag")));

  ILayerManager *manager = GetCOREInterface13()->GetLayerManager();
  ILayer *l = NULL;

  TCHAR *p[2];
  TCHAR str_buf[512];
  for (int i = 0; i < 16; i++)
  {
    _stprintf(str_buf, _T("LOD%02d"), i);
    l = manager->GetLayer(TSTR(str_buf));
    if (!l)
    {
      _stprintf(str_buf, _T("lod%02d"), i);
      l = manager->GetLayer(TSTR(str_buf));
      if (!l)
      {
        _stprintf(str_buf, _T("Lod%02d"), i);
        l = manager->GetLayer(TSTR(str_buf));
      }
    }
    if (l && l->HasObjects())
    {
      p[0] = _tcsdup(str_buf);
      _stprintf(str_buf, _T("%.*s.lod%02d.dag"), base_len, fname, i);
      p[1] = _tcsdup(str_buf);
      fnames.Append(2, p);
    }
  }

  l = manager->GetLayer(TSTR(p[0] = _T("DESTR")));
  if (!l)
    l = manager->GetLayer(TSTR(p[0] = _T("destr")));

  if (l && l->HasObjects())
  {
    p[0] = _tcsdup(p[0]);
    _stprintf(str_buf, _T("%.*s_destr.lod00.dag"), base_len, fname);
    p[1] = _tcsdup(str_buf);
    fnames.Append(2, p);
  }

  // if (base_len > 2 && _tcsncmp(&fname[base_len-2], _T("_a"), 2)==0)
  //   base_len -= 2;

  l = manager->GetLayer(TSTR(p[0] = _T("DM")));
  if (!l)
  {
    l = manager->GetLayer(TSTR(p[0] = _T("dm")));
    if (!l)
      l = manager->GetLayer(TSTR(p[0] = _T("Dm")));
  }
  if (l && l->HasObjects())
  {
    p[0] = _tcsdup(p[0]);
    _stprintf(str_buf, _T("%.*s_dm.dag"), base_len, fname);
    p[1] = _tcsdup(str_buf);
    fnames.Append(2, p);
  }

  for (int i = 0; i < 16; i++)
  {
    _stprintf(str_buf, _T("DMG_LOD%02d"), i);
    l = manager->GetLayer(TSTR(str_buf));
    if (!l)
    {
      _stprintf(str_buf, _T("dmg_lod%02d"), i);
      l = manager->GetLayer(TSTR(str_buf));
      if (!l)
      {
        _stprintf(str_buf, _T("Dmg_lod%02d"), i);
        l = manager->GetLayer(TSTR(str_buf));
      }
    }
    if (l && l->HasObjects())
    {
      p[0] = _tcsdup(str_buf);
      _stprintf(str_buf, _T("%.*s_dmg.lod%02d.dag"), base_len, fname, i);
      p[1] = _tcsdup(str_buf);
      fnames.Append(2, p);
    }
  }

  for (int i = 0; i < 16; i++)
  {
    _stprintf(str_buf, _T("DMG2_LOD%02d"), i);
    l = manager->GetLayer(TSTR(str_buf));
    if (!l)
    {
      _stprintf(str_buf, _T("dmg2_lod%02d"), i);
      l = manager->GetLayer(TSTR(str_buf));
      if (!l)
      {
        _stprintf(str_buf, _T("Dmg2_lod%02d"), i);
        l = manager->GetLayer(TSTR(str_buf));
      }
    }
    if (l && l->HasObjects())
    {
      p[0] = _tcsdup(str_buf);
      _stprintf(str_buf, _T("%.*s_dmg2.lod%02d.dag"), base_len, fname, i);
      p[1] = _tcsdup(str_buf);
      fnames.Append(2, p);
    }
  }

  for (int i = 0; i < 16; i++)
  {
    _stprintf(str_buf, _T("EXPL_LOD%02d"), i);
    l = manager->GetLayer(TSTR(str_buf));
    if (!l)
    {
      _stprintf(str_buf, _T("expl_lod%02d"), i);
      l = manager->GetLayer(TSTR(str_buf));
      if (!l)
      {
        _stprintf(str_buf, _T("Expl_lod%02d"), i);
        l = manager->GetLayer(TSTR(str_buf));
      }
    }
    if (l && l->HasObjects())
    {
      p[0] = _tcsdup(str_buf);
      _stprintf(str_buf, _T("%.*s_expl.lod%02d.dag"), base_len, fname, i);
      p[1] = _tcsdup(str_buf);
      fnames.Append(2, p);
    }
  }

  l = manager->GetLayer(TSTR(p[0] = _T("XRAY")));
  if (!l)
    l = manager->GetLayer(TSTR(p[0] = _T("xray")));

  if (l && l->HasObjects())
  {
    p[0] = _tcsdup(p[0]);
    _stprintf(str_buf, _T("%.*s_xray.dag"), base_len, fname);
    p[1] = _tcsdup(str_buf);
    fnames.Append(2, p);
  }

  if (fnames.Count() / 2 > 1)
    return true;

  for (int i = 0; i < fnames.Count(); i++)
    free(fnames[i]);
  fnames.ZeroCount();
  return false;
}
#endif

BOOL ExpUtil::export_dag(TCHAR **textures, int max_textures, TCHAR *default_material)
{
#if defined(MAX_RELEASE_R13) && MAX_RELEASE >= MAX_RELEASE_R13
  DagorLogWindowAutoShower logWindowAutoShower(/*clear_log = */ true);

  Tab<TCHAR *> fnames;
  Tab<bool> isHidden;

  if (!find_co_layers(exp_fname, fnames))
    return export_one_dag(exp_fname, textures, max_textures, default_material);

  TCHAR buf[8192];
  _stprintf(buf, _T("Export layers to %d separate files?\n"), fnames.Count() / 2);
  for (int i = 0; i < fnames.Count(); i += 2)
    if (_tcslen(buf) < (7 << 10))
    {
      int fn_len = (int)_tcslen(fnames[i + 1]);
      _stprintf(buf + _tcslen(buf), _T("%s -> %s%s\n"), fnames[i], fn_len > 64 ? _T("...") : _T(""),
        fn_len > 48 ? fnames[i + 1] + fn_len - 48 : fnames[i + 1]);
    }

  if (MessageBox(GetFocus(), buf, _T("Export layered DAGs"), MB_YESNO | MB_ICONQUESTION) != IDYES)
  {
    for (int i = 0; i < fnames.Count(); i++)
      free(fnames[i]);
    return export_one_dag(exp_fname, textures, max_textures, default_material);
  }

  ILayerManager *manager = GetCOREInterface13()->GetLayerManager();
  isHidden.SetCount(fnames.Count() / 2);
  for (int i = 0; i < fnames.Count(); i += 2)
  {
    ILayer *l = manager->GetLayer(TSTR(fnames[i]));
    isHidden[i / 2] = l->IsHidden();
    l->Hide(true);
  }
  for (int i = 0; i < fnames.Count(); i += 2)
  {
    TSTR lnm(fnames[i]);
    ILayer *l = manager->GetLayer(lnm);
    manager->SetCurrentLayer(lnm);
    l->Hide(false);

    export_one_dag(fnames[i + 1], textures, max_textures, default_material);
    l->Hide(true);
  }
  for (int i = 0; i < fnames.Count(); i += 2)
  {
    if (!isHidden[i / 2])
      manager->GetLayer(TSTR(fnames[i]))->Hide(false);
    free(fnames[i]);
    free(fnames[i + 1]);
  }
  manager->SetCurrentLayer();

  return true;
#else
  return export_one_dag(exp_fname, textures, max_textures, default_material);
#endif
}

BOOL ExpUtil::export_one_dag(const TCHAR *exp_fn, TCHAR **textures, int max_textures, TCHAR *default_material)
{
  ExportENCB cb(ip->GetTime(), false);
  cb.default_material = default_material;
  enum_nodes(ip->GetRootNode(), &cb);

  if (!cb.node.Count())
  {
    errorMessage(_T("There are no nodes."));
    return false;
  }

  return export_one_dag_cb(cb, exp_fn, textures, max_textures, default_material);
}

BOOL ExpUtil::export_one_dag_cb(ExportENCB &cb, const TCHAR *exp_fn, TCHAR **textures, int max_textures, TCHAR *default_material)
{
  INamedSelectionSetManager *IPNSS = INamedSelectionSetManager::GetInstance();

  checkDupesAndSpaces(cb.node);

  FILE *h = _tfopen(exp_fn, _T ("wb"));
  if (!h)
  {
    errorMessage(GetString(IDS_FILE_CREATE_ERR));
    return false;
  }
  if (!cb.save(h, ip, textures, max_textures))
  {
    errorMessage(GetString(IDS_FILE_WRITE_ERR));
    fclose(h);
    return false;
  }
  fclose(h);
  explog(_T ("%d pos keys max\r\n"), cb.max_pkeys);
  if (cb.max_pkeys_n)
    explog(_T ("  for \"%s\"\r\n"), cb.max_pkeys_n->GetName());
  explog(_T ("%d rot keys max\r\n"), cb.max_rkeys);
  if (cb.max_rkeys_n)
    explog(_T ("  for \"%s\"\r\n"), cb.max_rkeys_n->GetName());
  explog(_T ("%d scl keys max\r\n"), cb.max_skeys);
  if (cb.max_skeys_n)
    explog(_T ("  for \"%s\"\r\n"), cb.max_skeys_n->GetName());
  explog(_T ("%d nodes\r\n"), cb.node.Count());
  explog(_T ("%d materials\r\n"), cb.mat.Count());
  explog(_T ("%d textures\r\n"), cb.tex.Count());
  explog(_T ("%d key labels\r\n"), cb.klabel.Count());
  explog(_T ("%d note tracks\r\n"), cb.ntrack.Count());
  if (cb.nonort)
  {
    warningMessage(GetString(IDS_NONORT_ANIM));
    IPNSS->AddNewNamedSelSet(cb.nonort_nodes, TSTR(GetString(IDS_NONORT_SELSET)));
    explogWarning(_T ("%d nodes with bad animation\r\n"), cb.nonort_nodes.Count());
  }
  else
    IPNSS->RemoveNamedSelSet(TSTR(GetString(IDS_NONORT_SELSET)));
  if (cb.nofaces)
  {
    warningMessage(GetString(IDS_NOFACES_NODES));
  }
  if (cb.hasDegenerateTriangles)
  {
    warningMessage(GetString(IDS_DEGENERATE_TRIANGLES));
  }

  if (cb.hasNoSmoothing)
  {
    warningMessage(GetString(IDS_NO_SMOOTHING));
  }
  if (cb.hasBigMeshes)
  {
    warningMessage(GetString(IDS_BIG_MESHES));
  }
  if (cb.hasNonDagorMaterials)
  {
    warningMessage(GetString(IDS_NON_DAGOR_MATERIALS));
  }
  if (cb.hasSubSubMaterials)
  {
    warningMessage(GetString(IDS_SUB_SUB_MATERIALS));
  }
  if (cb.hasAbsolutePaths)
  {
    // TSTR title = GetString (IDS_WARNING);
    // MessageBox (ip->GetMAXHWnd (), GetString (IDS_ABSOLUTE_PATHS), title, MB_OK | MB_ICONSTOP);
  }

  return true;
}
void ExpUtil::export_anim_v2()
{
  DagorLogWindowAutoShower logWindowAutoShower(/*clear_log = */ true);
  explog(_T ("Exporting Anim v2 to file <%s>...\r\n"), exp_anim2_fname);
  int t0 = timeGetTime();

  ExportENCB cb(ip->GetTime(), true);
  enum_nodes(ip->GetRootNode(), &cb);
  t0 = timeGetTime() - t0;
  explog(_T (" enum node: %d ms\r\n"), t0);

  checkDupesAndSpaces(cb.node);

  FILE *h = _tfopen(exp_anim2_fname, _T ("wb"));
  if (!h)
  {
    errorMessage(GetString(IDS_FILE_CREATE_ERR));
    return;
  }
  t0 = timeGetTime();
  if (!cb.save_anim2(h, ip))
  {
    errorMessage(GetString(IDS_FILE_WRITE_ERR));
    fclose(h);
    return;
  }
  t0 = timeGetTime() - t0;
  explog(_T (" save anim2: %d ms\r\n"), t0);

  explog(_T (" %d nodes\r\n"), cb.node.Count());
  explog(_T ("Success!\r\n"));
  fclose(h);
}
#if defined(MAX_RELEASE_R19_PREVIEW) && MAX_RELEASE >= MAX_RELEASE_R19_PREVIEW
static bool FolderOpenDialog(TCHAR *path, HWND)
{
  IFileDialog *pfd;
  HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
  if (!SUCCEEDED(hr))
    return false;

  DWORD dwFlags;
  hr = pfd->GetOptions(&dwFlags);
  if (!SUCCEEDED(hr))
  {
    pfd->Release();
    return false;
  }
  hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS);
  if (!SUCCEEDED(hr))
  {
    pfd->Release();
    return false;
  }

  hr = pfd->Show(NULL);
  if (!SUCCEEDED(hr))
  {
    pfd->Release();
    return false;
  }

  IShellItem *psiResult;
  hr = pfd->GetResult(&psiResult);
  if (!SUCCEEDED(hr))
  {
    pfd->Release();
    return false;
  }

  PWSTR pszFilePath = NULL;
  hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
  _tcscpy(path, pszFilePath);
  CoTaskMemFree(pszFilePath);
  psiResult->Release();
  pfd->Release();
  return SUCCEEDED(hr);
}
#else
static bool FolderOpenDialog(TCHAR *path, HWND hPanel)
{
  BROWSEINFO bi;
  bi.hwndOwner = hPanel;
  bi.iImage = 0;
  bi.lParam = 0;
  bi.lpfn = NULL;
  bi.lpszTitle = _T("Browse for folder");
  bi.pidlRoot = NULL;
  bi.pszDisplayName = path;
  bi.ulFlags = BIF_NEWDIALOGSTYLE;
  PCIDLIST_ABSOLUTE pidl = ::SHBrowseForFolder(&bi);
  if (!pidl)
    return false;
  return ::SHGetPathFromIDList(pidl, path);
}
#endif

void ExpUtil::exportObjectsAsDagsInternal(const TCHAR *folder, INode &node)
{
  const int childrenCount = node.NumberOfChildren();
  for (int i = 0; i < childrenCount; ++i)
  {
    INode *childNode = node.GetChildNode(i);

    if (((util.expflg & EXP_SEL) != 0 && childNode->Selected() == 0) || ((util.expflg & EXP_HID) == 0 && childNode->IsHidden()))
    {
      exportObjectsAsDagsInternal(folder, *childNode);
      continue;
    }

    ExportENCB cb(ip->GetTime(), false);

    if (childNode->IsGroupHead())
      enum_nodes_by_node(childNode, &cb);
    else
      cb.proc(childNode);

    if (cb.node.Count() > 0)
    {
      // Use the node as origin instead of the scene.
      cb.useIdentityTransformForNode = childNode;

      TSTR path;
      path.printf(_T("%s\\%s.dag"), folder, childNode->GetName());

      explog(_T("Exporting node \"%s\" to file \"%s\"\r\n"), childNode->GetName(), path.data());
      export_one_dag_cb(cb, path);
      explog(_T("\r\n"));
    }
    else
    {
      explogWarning(_T("Skipping node '%s'. It does no match the export settings (beside \"sel\" and \"hid\").\r\n"),
        childNode->GetName());
    }

    // If this is a group then there is no need to descend further because groups are exported as a single dag.
    if (!childNode->IsGroupHead())
      exportObjectsAsDagsInternal(folder, *childNode);
  }
}

void ExpUtil::exportObjectsAsDags()
{
  DagorLogWindowAutoShower logWindowAutoShower(/*clear_log = */ true);

  const int PATH_MAX = 512;
  TCHAR folder[PATH_MAX];
  if (!FolderOpenDialog(folder, hPanel))
    return;

  INode *rootNode = GetCOREInterface13()->GetRootNode();
  if (!rootNode)
  {
    explogError(_T("Could not get the root node.\r\n"));
    explogError(_T("Aborting export.\r\n"));
    return;
  }

  exportObjectsAsDagsInternal(folder, *rootNode);
}

void ExpUtil::exportLayerAsDag(const TCHAR *folder, ILayer &layer)
{
  // "sel" only affects the layers we work with, so do not use it when processing the nodes.
  const int oldExpSel = util.expflg & EXP_SEL;
  util.expflg &= ~EXP_SEL;

  ILayerProperties *layerProperties = (ILayerProperties *)layer.GetInterface(LAYERPROPERTIES_INTERFACE);
  if (layerProperties)
  {
    ExportENCB cb(ip->GetTime(), false);

    // ILayerProperties::Nodes returns with all nodes belonging to a layer, regardless of the hierarchy,
    // so there is no need to call enum_nodes.
    Tab<INode *> nodes;
    layerProperties->Nodes(nodes);
    for (int i = 0; i < nodes.Count(); ++i)
      cb.proc(nodes[i]);

    if (cb.node.Count() > 0)
    {
      TSTR path;
      path.printf(_T("%s\\%s.dag"), folder, layer.GetName().data());

      explog(_T("Exporting layer \"%s\" to file \"%s\"\r\n"), layer.GetName().data(), path.data());
      export_one_dag_cb(cb, path);
      explog(_T("\r\n"));
    }
    else
    {
      explogWarning(_T("Skipping layer '%s'. It has no nodes to export that matches the export settings.\r\n"),
        layer.GetName().data());
    }
  }
  else
  {
    explogError(_T("Cannot get the layer properties for layer '%s'.\r\n"), layer.GetName().data());
  }

  util.expflg |= oldExpSel;
}

void ExpUtil::exportLayersAsDagsInternal(const TCHAR *folder, ILayer &layer)
{
#if defined(MAX_RELEASE_R19) && MAX_RELEASE >= MAX_RELEASE_R19
  const bool matchesVisibilityFilter = (util.expflg & EXP_HID) != 0 || !layer.IsHidden(false);
#else
  const bool matchesVisibilityFilter = (util.expflg & EXP_HID) != 0 || !layer.IsHidden();
#endif

  const int childLayerCount = layer.GetNumOfChildLayers();

  if (childLayerCount > 0)
  {
    if (matchesVisibilityFilter)
      explog(_T("Layer '%s' has child layer(s), it will not be exported.\r\n"), layer.GetName().data());

    for (int i = 0; i < childLayerCount; ++i)
      exportLayersAsDagsInternal(folder, *layer.GetChildLayer(i));
  }
  else if (matchesVisibilityFilter)
    exportLayerAsDag(folder, layer);
}

void ExpUtil::exportLayersAsDags()
{
  DagorLogWindowAutoShower logWindowAutoShower(/*clear_log = */ true);

  const int PATH_MAX = 512;
  TCHAR folder[PATH_MAX];
  if (!FolderOpenDialog(folder, hPanel))
    return;

  ILayerManager *manager = GetCOREInterface13()->GetLayerManager();
  if (!manager)
  {
    explogError(_T("Could not get the layer manager.\r\n"));
    explogError(_T("Aborting export.\r\n"));
    return;
  }

  if ((util.expflg & EXP_SEL) == 0)
  {
    const int layerCount = manager->GetLayerCount();
    for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
      ILayer *layer = manager->GetLayer(layerIndex);

      // Ignore non-top level layers, we will enumerate them hierarchially.
      if (layer->GetParentLayer())
        continue;

      // The default layer itself is not exported (except when "sel" is enabled and it is the active layer), only its children.
      if (is_default_layer(*layer))
      {
        const int childLayerCount = layer->GetNumOfChildLayers();
        for (int childLayerIndex = 0; childLayerIndex < childLayerCount; ++childLayerIndex)
          exportLayersAsDagsInternal(folder, *layer->GetChildLayer(childLayerIndex));
      }
      else
        exportLayersAsDagsInternal(folder, *layer);
    }
  }
  else
  {
    ILayer *currentLayer = manager->GetCurrentLayer();
    if (!currentLayer)
    {
      explogError(_T("Could not get the current layer.\r\n"));
      explogError(_T("Aborting export.\r\n"));
      return;
    }

    exportLayersAsDagsInternal(folder, *currentLayer);
  }
}

class CameraNodeEnumerator : public ENodeCB
{
public:
  Interface *ip;
  struct Cam
  {
    INode *cameraNode;
    CameraObject *camera;
    INode *hero;
    int keyNum;
    bool animated;
  };
  Tab<Cam> cam;
  int keyNum;

  CameraNodeEnumerator(Interface *_ip) : ip(_ip), keyNum(0) {}
  ~CameraNodeEnumerator() {}

  int proc(INode *node)
  {
    if (node->IsHidden())
      return ECB_CONT;

    Object *obj = node->GetObjectRef();
    if (!obj)
      return ECB_CONT;

    if (obj->SuperClassID() != CAMERA_CLASS_ID)
      return ECB_CONT;

    TCHAR s[1024];
    int j;

    _stprintf(s, _T("%s_hero"), node->GetName());

    Cam c;
    c.cameraNode = node;
    c.camera = (CameraObject *)obj;
    c.hero = ip->GetINodeByName(s);
    c.keyNum = 0;
    c.animated = false;
    // debug ( "%s: %p:%p:%p (%s)", (char*)node->GetName(), c.cameraNode, c.camera, c.hero, s );

    if (!node->HasNoteTracks())
      c.keyNum = 1;
    else
    {
      for (j = 0; j < node->NumNoteTracks(); j++)
      {
        DefNoteTrack *_nt = (DefNoteTrack *)node->GetNoteTrack(j);
        if (!_nt)
          continue;
        c.keyNum += _nt->keys.Count();
        if (c.keyNum > 1)
          c.animated = true;
      }
      if (c.keyNum < 1)
        c.keyNum = 1;
    }

    keyNum += c.keyNum;

    cam.Append(1, &c);
    return ECB_CONT;
  }

  void export_cameras(FILE *fp)
  {
    struct AdvCameraFileHdr
    {
      unsigned label;
      unsigned ver;
      unsigned hdrSize;
      unsigned cameraNum;
      unsigned namePoolSz;
      unsigned keyPoolSz;
    };
    struct AdvCameraDataRec
    {
      unsigned nameIdx;
      unsigned keyNum;
      unsigned keyIdx;
      unsigned flags;

      enum
      {
        F_Animated = 0x0001
      };
    };
    struct AdvCameraKey
    {
      Quat rot;
      Point3 pos;
      Point3 heroPos;
      float w, fov, wFollow;
    };
    AdvCameraFileHdr hdr;
    AdvCameraDataRec *data;
    AdvCameraKey *key;
    int i, j;

    hdr.label = 'MACa';
    hdr.ver = 0x100;
    hdr.hdrSize = sizeof(hdr);
    hdr.cameraNum = cam.Count();
    hdr.namePoolSz = 0;
    hdr.keyPoolSz = keyNum;

    for (i = 0; i < hdr.cameraNum; i++)
      hdr.namePoolSz += (int)_tcslen(cam[i].cameraNode->GetName()) + 1;

    int nmpool_used = hdr.namePoolSz;
    hdr.namePoolSz = (hdr.namePoolSz + 7) & ~7;

    fwrite(&hdr, 1, sizeof(hdr), fp);

    data = new AdvCameraDataRec[hdr.cameraNum];
    key = new AdvCameraKey[hdr.keyPoolSz];

    // store names
    for (i = 0; i < hdr.cameraNum; i++)
    {
      std::string s = wideToStr(cam[i].cameraNode->GetName());
      fwrite(s.c_str(), 1, s.length() + 1, fp);
    }
    for (i = nmpool_used; i < hdr.namePoolSz; i++)
      fwrite("\0", 1, 1, fp);

    // build structures
    hdr.namePoolSz = 0;
    keyNum = 0;
    for (i = 0; i < hdr.cameraNum; i++)
    {
      AdvCameraDataRec &drec = data[i];
      drec.nameIdx = hdr.namePoolSz;
      drec.keyNum = cam[i].keyNum;
      drec.keyIdx = keyNum;
      drec.flags = 0;

      hdr.namePoolSz += (int)_tcslen(cam[i].cameraNode->GetName()) + 1;
      keyNum += cam[i].keyNum;

      if (cam[i].animated)
      {
        int idx = drec.keyIdx;
        drec.flags |= AdvCameraDataRec::F_Animated;

        for (j = 0; j < cam[i].cameraNode->NumNoteTracks(); j++)
        {
          DefNoteTrack *_nt = (DefNoteTrack *)cam[i].cameraNode->GetNoteTrack(j);
          if (!_nt)
            continue;
          for (int k = 0; k < _nt->keys.Count(); k++)
          {
            AdvCameraKey &krec = key[idx];
            NoteKey *nk = _nt->keys[k];
            std::string wnote = wideToStr(nk->note);

            const char *wt = strchr(wnote.c_str(), '@');
            if (wt)
              wt++;

            getRotPos(cam[i].cameraNode, krec.pos, krec.rot, nk->time);
            if (cam[i].hero)
              getPos(cam[i].hero, cam[i].cameraNode->GetParentNode(), krec.heroPos, nk->time);
            else
              krec.heroPos = Point3(0, 0, 0);

            if (wt)
              krec.w = atof(wt);
            else
              krec.w = 1.0;
            krec.fov = 1.0 / tan(cam[i].camera->GetFOV(nk->time) / 2);
            debug("krec.fov=%.3f (%.3f deg)", krec.fov, cam[i].camera->GetFOV(nk->time) * 180 / PI);

            if (strnicmp(wnote.c_str(), "follow", 6) == 0)
              krec.wFollow = 1.0;
            else if (strnicmp(wnote.c_str(), "still", 5) == 0)
              krec.wFollow = 0.0;

            // debug ( "note: %s krec.wFollow=%.3f wt=%.3f", (char*)nk->note, krec.wFollow, krec.w );
            idx++;
          }
        }
        if (idx - drec.keyIdx != drec.keyNum)
          debug("node %s: idx=%d drec.keyIdx=%d drec.keyNum=%d", (char *)cam[i].cameraNode->GetName(), idx, drec.keyIdx, drec.keyNum);
      }
      else
      {
        AdvCameraKey &krec = key[drec.keyIdx];
        getRotPos(cam[i].cameraNode, krec.pos, krec.rot, 0);
        if (cam[i].hero)
          getPos(cam[i].hero, cam[i].cameraNode->GetParentNode(), krec.pos, 0);
        else
          krec.heroPos = Point3(0, 0, 0);
        krec.w = 1.0;
        krec.fov = 1.0 / tan(cam[i].camera->GetFOV(0) / 2);
        krec.wFollow = 0.0;
      }
    }

    // store keys
    fwrite(key, hdr.keyPoolSz, sizeof(AdvCameraKey), fp);

    // store cameras
    fwrite(data, hdr.cameraNum, sizeof(AdvCameraDataRec), fp);

    if (data)
      delete data;
    if (key)
      delete key;
  }

  void getPos(INode *node, INode *parent, Point3 &pos, TimeValue t)
  {
    Matrix3 m;
    interp_tm(node, parent, t, m);
    pos = m.GetRow(3);
    // debug ( "%s: %d: pos=(%.3f,%.3f,%.3f)", (char*)node->GetName(), t, pos.x, pos.y, pos.z );
  }
  void getRotPos(INode *node, Point3 &pos, Quat &rot, TimeValue t)
  {
    Matrix3 m;
    interp_tm(node, t, m);
    /*
        debug ( "%s: %d: (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f)", (char*)node->GetName(), t,
                m.GetRow(0).x, m.GetRow(0).y, m.GetRow(0).z, m.GetRow(1).x, m.GetRow(1).y, m.GetRow(1).z,
                m.GetRow(2).x, m.GetRow(2).y, m.GetRow(2).z, m.GetRow(3).x, m.GetRow(3).y, m.GetRow(3).z );
    */
    Point3 ax = m.GetRow(0), ay = m.GetRow(1), az = m.GetRow(2);
    pos = m.GetRow(3);
    float lx = Length(ax), ly = Length(ay), lz = Length(az);
    if (m.Parity())
      lz = -lz;
    if (lx != 0)
      m.SetRow(0, ax /= lx);
    if (ly != 0)
      m.SetRow(1, ay /= ly);
    if (lz != 0)
      m.SetRow(2, az /= lz);
    // m.SetRow(3,Point3(0,0,0));
    // m.SetIdentFlags(POS_IDENT|SCL_IDENT);
    rot = Quat(m);
    rot = Conjugate(rot);
  }
  void interp_tm(INode *node, TimeValue t, Matrix3 &m)
  {
    INode *pnode = node->GetParentNode();
    Matrix3 ntm = get_scaled_stretch_node_tm(node, t);
    Matrix3 ptm;

    adjwtm(ntm);
    if (pnode)
    {
      ptm = get_scaled_stretch_node_tm(pnode, t);
      if (!pnode->IsRootNode())
        adjwtm(ptm);
      m = ntm * Inverse(ptm);
    }
    else
      m = ntm;
  }
  void interp_tm(INode *node, INode *pnode, TimeValue t, Matrix3 &m)
  {
    Matrix3 ntm = get_scaled_stretch_node_tm(node, t);
    Matrix3 ptm;

    adjwtm(ntm);
    if (pnode)
    {
      ptm = get_scaled_stretch_node_tm(pnode, t);
      if (!pnode->IsRootNode())
        adjwtm(ptm);
      m = ntm * Inverse(ptm);
    }
    else
      m = ntm;
  }
};


void ExpUtil::export_camera_v1()
{
  DagorLogWindowAutoShower logWindowAutoShower(/*clear_log = */ true);
  explog(_T ("Exporting Adv. Camera file <%s>...\r\n"), exp_camera_fname);
  int t0;

  CameraNodeEnumerator cne(ip);
  enum_nodes(ip->GetRootNode(), &cne);

  FILE *h = _tfopen(exp_camera_fname, _T ("wb"));
  if (!h)
  {
    errorMessage(GetString(IDS_FILE_CREATE_ERR));
    return;
  }
  t0 = timeGetTime();
  cne.export_cameras(h);
  t0 = timeGetTime() - t0;
  explog(_T (" save adv.camera: %d ms\r\n"), t0);

  explog(_T ("Success!\r\n"));
  fclose(h);
}


void ExpUtil::exportPhysics()
{
  DagorLogWindowAutoShower logWindowAutoShower(/*clear_log = */ true);
  explog(_T ("Exporting physics file <%s>...\r\n"), exp_phys_fname);

  if (!(util.expflg & EXP_DONT_CALC_MOMJ))
  {
    explog(_T("Updating masses and momjs...\r\n"));
    ::calc_momjs(ip);
  }

  FILE *h = _tfopen(exp_phys_fname, _T ("wb"));
  if (!h)
  {
    errorMessage(GetString(IDS_FILE_CREATE_ERR));
    return;
  }

  ::export_physics(h, ip);

  explog(_T ("Success!\r\n"));
  fclose(h);
}


void ExpUtil::calcMomj()
{
  DagorLogWindowAutoShower logWindowAutoShower(/*clear_log = */ true);
  explog(_T("Updating masses and momjs...\r\n"));

  ::calc_momjs(ip);

  explog(_T("Success!\r\n"));
}


void ExpUtil::checkDupesAndSpaces(Tab<INode *> &node_list)
{
  bool hasDupes = false;
  bool hasSpaces = false;
  bool hasEmptyNames = false;
  for (int i = 0; i < node_list.Count(); ++i)
  {
    if (!(expflg & EXP_HID) && node_list[i]->IsNodeHidden())
      continue;

    if ((expflg & EXP_SEL) && !node_list[i]->Selected())
      continue;

    if (node_list[i]->GetName()[0] == 0)
    {
      explogWarning(_T("node with empty name\r\n"));
      hasEmptyNames = true;
    }

    if (_tcschr(node_list[i]->GetName(), ' ') != NULL)
    {
      // explog(_T("node name \"%s\" contains spaces\r\n"), node_list[i]->GetName());
      hasSpaces = true;
    }

    for (int j = i + 1; j < node_list.Count(); ++j)
    {
      if (!(expflg & EXP_HID) && node_list[j]->IsNodeHidden())
        continue;

      if ((expflg & EXP_SEL) && !node_list[j]->Selected())
        continue;

      char *leftName = strdup(wideToStr(node_list[i]->GetName()).c_str());
      spaces_to_underscores(leftName);
      char *rightName = strdup(wideToStr(node_list[j]->GetName()).c_str());
      spaces_to_underscores(rightName);

      if (strcmp(leftName, rightName) == 0)
      {
        explogWarning(_T ("duplicate node name \"%s\"\r\n"), node_list[i]->GetName());
        hasDupes = true;
      }

      free(leftName);
      free(rightName);
    }
  }

  if (hasDupes)
  {
    ip->DisplayTempPrompt(_T("There are duplicate node names."), ERRMSG_DELAY);
    warningMessage(_T("There are nodes with the same names.\n See log for details."), _T("Duplicate names"));
  }

  if (hasSpaces)
  {
    // ip->DisplayTempPrompt (_T("There are node names with spaces."), ERRMSG_DELAY);
    // MessageBox (ip->GetMAXHWnd (), _T("There are node names with spaces.\n Spaces will NOT be replaced with '_' right now.\n You may
    // press 'Convert spaces' button\n on the 'Dagor Utility' rollout to convert names.\n See log for details."), _T ("Names with
    // spaces"), MB_OK | MB_ICONSTOP);
  }

  if (hasEmptyNames)
  {
    ip->DisplayTempPrompt(_T("There is a node with empty name."), ERRMSG_DELAY);
    warningMessage(_T("There is a node with empty name.\n See log for details."), _T("Empty name"));
  }
}


void ExpUtil::export_instances()
{
  DagorLogWindowAutoShower logWindowAutoShower(/*clear_log = */ true);
  explog(_T("Exporting instances placement file <%s>...\r\n"), exp_instances_fname);

  ExportENCB cb(ip->GetTime(), false);
  enum_nodes(ip->GetRootNode(), &cb);

  FILE *h = _tfopen(exp_instances_fname, _T("wb"));
  if (!h)
  {
    errorMessage(GetString(IDS_FILE_CREATE_ERR));
    return;
  }


  // Find base ground plane.

  float planeOffsetX = 1e20f;
  float planeOffsetZ = 1e20f;
  float planeSize = 0;
  int numTexturesWidth = 0, numTexturesHeight = 0;
  bool wasFound = false;
  for (unsigned int nodeNo = 0; nodeNo < cb.node.Count(); nodeNo++)
  {
    if (!cb.node[nodeNo]->GetParentNode() || !cb.node[nodeNo]->GetParentNode()->IsRootNode())
      continue;

    if (_tcsncicmp(cb.node[nodeNo]->GetName(), _T("GroundPlane"), _tcslen(_T("GroundPlane"))))
      continue;

    float newPlaneOffsetX;
    if (!cb.node[nodeNo]->GetUserPropFloat(_T("PlaneOffsetX:r"), newPlaneOffsetX))
    {
      TCHAR buf[1000];
      _stprintf(buf, _T("'%s': invalid PlaneOffsetX:r"), cb.node[nodeNo]->GetName());
      errorMessage(buf);
      return;
    }

    float newPlaneOffsetZ;
    if (!cb.node[nodeNo]->GetUserPropFloat(_T("PlaneOffsetZ:r"), newPlaneOffsetZ))
    {
      TCHAR buf[1000];
      _stprintf(buf, _T("'%s': invalid PlaneOffsetZ:r"), cb.node[nodeNo]->GetName());
      errorMessage(buf);
      return;
    }

    if (newPlaneOffsetX <= planeOffsetX || newPlaneOffsetZ <= planeOffsetZ)
    {
      planeOffsetX = newPlaneOffsetX;
      planeOffsetZ = newPlaneOffsetZ;

      if (!cb.node[nodeNo]->GetUserPropFloat(_T("PlaneSize:r"), planeSize))
      {
        TCHAR buf[1000];
        _stprintf(buf, _T("'%s': invalid PlaneSize:r"), cb.node[nodeNo]->GetName());
        errorMessage(buf);
        return;
      }

      if (!cb.node[nodeNo]->GetUserPropInt(_T("numTexturesWidth:i"), numTexturesWidth))
        numTexturesWidth = 16;

      if (!cb.node[nodeNo]->GetUserPropInt(_T("numTexturesHeight:i"), numTexturesHeight))
        numTexturesHeight = 16;

      wasFound = true;
    }
  }

  if (!wasFound)
  {
    errorMessage(_T("GroundPlane not found"));
    return;
  }

  float offsetX = planeOffsetX - 0.5f * numTexturesWidth * planeSize;
  float offsetZ = planeOffsetZ - 0.5f * numTexturesHeight * planeSize;
  unsigned int planeNo =
    (unsigned int)floorf(planeOffsetZ / planeSize + 0.5f) * numTexturesWidth + (unsigned int)floorf(planeOffsetX / planeSize + 0.5f);

  fprintf(h,
    "Version:i=20070810\r\nPlaneOffsetX:r=%f\r\nPlaneOffsetZ:r=%f\r\nPlaneSize:r=%f\r\n"
    "numTexturesWidth:i=%d\r\nnumTexturesHeight:i=%d\r\n\r\n",
    planeOffsetX, planeOffsetZ, planeSize, numTexturesWidth, numTexturesHeight);


  // Export nodes.

  unsigned int numExportedNodes = 0;
  for (unsigned int nodeNo = 0; nodeNo < cb.node.Count(); nodeNo++)
  {
    if (!cb.node[nodeNo]->GetParentNode() || !cb.node[nodeNo]->GetParentNode()->IsRootNode())
      continue;

    std::string name = wideToStr(cb.node[nodeNo]->GetName());
    if (!strnicmp(name.c_str(), "GroundPlane", strlen("GroundPlane")))
      continue;

    while (name.length() > 0 && isdigit((unsigned char)name[name.length() - 1]))
      name.erase(name.length() - 1, 1);

    if (name.length() > 0 && name[name.length() - 1] == '_')
      name.erase(name.length() - 1, 1);

#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
    float masterScale = (float)GetSystemUnitScale(UNITS_METERS);
#else
    float masterScale = (float)GetMasterScale(UNITS_METERS);
#endif
    Matrix3 tm = cb.node[nodeNo]->GetNodeTM(0);
    tm.SetTrans(tm.GetTrans() * masterScale + Point3(offsetX, offsetZ, 0.f));
    const Matrix3 m = tm;

    fprintf(h,
      "object{\r\n"
      "  name:t=\"%s\"\r\n"
      "  numericName:t=\"%s %d\"\r\n"
      "  matrix:m=[[%f, %f, %f] [%f, %f, %f] [%f, %f, %f] [%f, %f, %f]]\r\n"
      "}\r\n",
      name.c_str(), name.c_str(), planeNo * 100000 + numExportedNodes, m[0][0], m[0][2], m[0][1], m[2][0], m[2][2], m[2][1], m[1][0],
      m[1][2], m[1][1], m[3][0], m[3][2], m[3][1]);

    numExportedNodes++;
  }

  explog(_T("Success!\r\n"));
  fclose(h);
}


bool ExportENCB::checkDegenerateTriangle(INode *node, Matrix3 &applied_transform, unsigned int index1, unsigned int index2,
  unsigned int index3, Point3 &vertex1, Point3 &vertex2, Point3 &vertex3)
{
  Point3 *degenerateAt = NULL;

  if (index1 == index2)
    degenerateAt = &vertex1;

  if (index1 == index3)
    degenerateAt = &vertex1;

  if (index2 == index3)
    degenerateAt = &vertex2;

  if (is_equal_point(vertex1, vertex2, DEGENERATE_VERTEX_DELTA))
    degenerateAt = &vertex1;

  if (is_equal_point(vertex1, vertex3, DEGENERATE_VERTEX_DELTA))
    degenerateAt = &vertex1;

  if (is_equal_point(vertex2, vertex3, DEGENERATE_VERTEX_DELTA))
    degenerateAt = &vertex2;

  Point3 diff = vertex3 - vertex1;
  Point3 dir = vertex2 - vertex1;
  float sqrlen = dir.LengthSquared();
  float t = DotProd(diff, dir) / sqrlen;
  diff -= t * dir;
  float linedist = diff.Length();

  Point3 center;

  if (linedist < DEGENERATE_VERTEX_DELTA)
  {
    center = (vertex1 + vertex2 + vertex3) / 3;
    degenerateAt = &center;
  }

  if (degenerateAt)
  {
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
    Point3 transformedPoint = (*degenerateAt) / (float)GetSystemUnitScale(UNITS_METERS);
#else
    Point3 transformedPoint = (*degenerateAt) / (float)GetMasterScale(UNITS_METERS);
#endif
    transformedPoint = Inverse(applied_transform) * node->GetObjTMAfterWSM(0) * transformedPoint;

    explogWarning(_T( "'%s' has degenerate triangle at (%g, %g, %g) (in units)\r\n"), node->GetName(), transformedPoint.x,
      transformedPoint.y, transformedPoint.z);

#if 0
      // Highlight degenerates.

      Object *obj = (Object*)util.ip->CreateInstance(
        GEOMOBJECT_CLASS_ID,
        Class_ID(BOXOBJ_CLASS_ID,0));

      IParamArray *pParams = obj->GetParamBlock();
      assert(pParams);

      int l = obj->GetParamBlockIndex(BOXOBJ_LENGTH);
      pParams->SetValue(l,TimeValue(0), 0.01f);
      int w = obj->GetParamBlockIndex(BOXOBJ_WIDTH);
      pParams->SetValue(w,TimeValue(0), 0.01f);
      int h = obj->GetParamBlockIndex(BOXOBJ_HEIGHT);
      pParams->SetValue(h,TimeValue(0), 0.01f);

      INode *node = util.ip->CreateObjectNode(obj);
      TSTR name(_T("Degenerate"));
      util.ip->MakeNameUnique(name);
      node->SetName(name);
      node->SetWireColor(RGB(255, 64, 64));

      node->SetNodeTM(0, TransMatrix(transformedPoint));
#endif
  }

  return degenerateAt != NULL;
}


const TCHAR *ExportENCB::makeCheckedRelPath(INode *node, Mtl *mtl, const TCHAR *absolute_path)
{
  const TCHAR *relative_path = make_path_rel(absolute_path);
  if (absolute_path && absolute_path[0] && relative_path == absolute_path) // Pointer comparison is valid here.
  {
    // explog("'%s' has material '%s' with absolute texture path '%s'\r\n",
    //   node->GetName(),
    //   mtl->GetName(),
    //   absolute_path);

    hasAbsolutePaths = true;
  }

  return relative_path;
}


//==========================================================================//

enum ExpOps
{
  fun_export
};

class IDagorExportUtil : public FPStaticInterface
{
public:
  DECLARE_DESCRIPTOR(IDagorExportUtil)
  BEGIN_FUNCTION_MAP FN_4(fun_export, TYPE_BOOL, export_dag, TYPE_STRING, TYPE_INTERVAL, TYPE_BOOL, TYPE_BOOL) END_FUNCTION_MAP

    BOOL export_dag(const TCHAR *fn, Interval ival, bool selectedOnly, bool suppressPrompts)
  {
    if (ival.Empty())
    {
      util.expflg &= ~EXP_ARNG;
    }
    else
    {
      util.expflg |= EXP_ARNG;
      util.astart = ival.Start();
      util.aend = ival.End();
    }

    if (selectedOnly)
      util.expflg |= EXP_SEL;
    else
      util.expflg &= ~EXP_SEL;

    util.suppressPrompts = suppressPrompts;

    _tcscpy(util.exp_fname, fn);
    util.update_ui(util.hPanel);
    BOOL result = util.export_dag();

    util.suppressPrompts = false;

    return result;
  }
};

static IDagorExportUtil dagorexputiliface(Interface_ID(0x18da32ce, 0x739f1b15), _T ("dagorExport"), IDS_DAGOR_EXPORT_IFACE, NULL,
  FP_CORE, fun_export, _T ("export"), -1, TYPE_BOOL, 0, 4, _T ("filename"), -1, TYPE_STRING, _T ("range"), -1, TYPE_INTERVAL,
  // f_keyArgDefault marks an optional keyArg param. The value
  // after that is its default value.
  _T ("selectedOnly"), -1, TYPE_BOOL, f_keyArgDefault, false, _T ("suppressPrompts"), -1, TYPE_BOOL, f_keyArgDefault, false,
#if defined(MAX_RELEASE_R15) && MAX_RELEASE >= MAX_RELEASE_R15
  p_end
#else
  end
#endif
);
