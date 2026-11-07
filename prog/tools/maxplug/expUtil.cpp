// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <plugapi.h>
#include <ilayer.h>
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
#include <notify.h>
#include <cstdint>
#include <format>
#include <functional>
#include <new>
#include <optional>
#include <string_view>
#include <type_traits>
#include "comboBoxHelper.h"
#include "dagor.h"
#include "dagorLogWindow.h"
#include "enumnode.h"
#include "dagfmt.h"
#include "mater.h"
#include "expanim.h"
#include "expanim2.h"
#include "Bones.h"
#include "iparamb2.h"
#include "ISkin.h"
#include "resource.h"
#include "debug.h"
#include "rolluppanel.h"
#include "ci.h"
#include "common.h"
#include "datablk.h"

#include <unordered_set>
#include <unordered_map>
#include <map>
#include <vector>
#include <algorithm>
#include <ranges>
#include <string>
#include <utility>
#include <Shobjidl.h>
#include <Shlobj.h>
#include <sstream>
#include <fstream>
#include <filesystem>

bool is_default_layer(const ILayer &layer);

std::wstring fix_empty_param_values(std::wstring_view _script, std::wstring_view classname);
std::wstring normalize_param_values(std::wstring_view script, std::wstring_view classname);

#include "layout.h"

namespace fs = std::filesystem;

#define ERRMSG_DELAY 3000

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef int FaceNGr[3];

class ExportENCB;

void export_physics(FILE *file, Interface *ip);

void calc_momjs(Interface *ip);

void scale_matrix(Matrix3 &tm)
{
  float masterScale = static_cast<float>(GetSystemUnitScale(UNITS_METERS));
  tm.SetTrans(tm.GetTrans() * masterScale);
}

class DagFileNames;

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
  HWND hExpDag, hExpAnim, hExpOther, hLog;
  ControlPtr<ICustEdit> eastart, eaend;
  ControlPtr<ICustEdit> epose, erote, escle, eorte;
  ControlPtr<ICustEdit> epose2, erote2;

  fs::path exp_fname;
  fs::path exp_anim2_fname;
  fs::path exp_phys_fname;
  fs::path exp_instances_fname;

  int expflg;
  ExportMode exportMode;
  float poseps, roteps, scleps, orteps;
  float poseps2, roteps2;
  TimeValue astart, aend;
  bool suppressPrompts;
  bool separateLayers;

  ExpUtil();
  void BeginEditParams(Interface *ip, IUtil *iu) override;
  void EndEditParams(Interface *ip, IUtil *iu) override;
  void DeleteThis() override {}

  void update_ui_dag(HWND hw);
  void update_ui_anim(HWND hw);
  void update_ui_other(HWND hw);
  void update_ui();
  void update_tooltips();

  void set_expflg(int flg, int);
  void reset_anim_settings();

  void Init(HWND hw);

  void InitAnim(HWND hw);
  void DestroyAnim(HWND hw);

  int input_exp_fname();
  int input_exp_anim2_fname();
  int input_exp_phys_fname();
  int input_exp_instances_fname();
  BOOL export_one_dag(const fs::path &exp_fn, std::unordered_set<INode *> skip_nodes = {});
  BOOL export_one_dag_cb(ExportENCB &cb, const fs::path &exp_fn);
  bool write_via_temp_file(const fs::path &target, const std::function<bool(FILE *)> &write);
  BOOL export_dag();
  void export_anim_v2();
  void exportPhysics();
  void export_instances();
  void calcMomj();
  BOOL export_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam);
  BOOL export_anim_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam);
  BOOL export_other_dlg_proc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam);
  void checkDupesAndSpaces(Tab<INode *> &node_list);
  void errorMessage(const TCHAR *msg);
  void warningMessage(const TCHAR *msg, const TCHAR *title = NULL);

private:
  void exportObjectsAsDagsInternal(DagFileNames &files, INode &node);
  void exportObjectsAsDags();
  void exportLayerAsDag(DagFileNames &files, ILayer &layer);
  void exportLayersAsDagsInternal(DagFileNames &files, ILayer &layer);
  void exportLayersAsDags();

  ToolTipExtender tooltipExtender;
};
static ExpUtil util;

class DagExport : public SceneExport
{
  int ExtCount() override { return 1; }
  const MCHAR *Ext(int) override { return _T("dag"); }
  const MCHAR *LongDesc() override { return GetString(IDS_DAGIMP_LONG); }
  const MCHAR *ShortDesc() override { return GetString(IDS_DAGIMP_SHORT); }
  const MCHAR *AuthorName() override { return GetString(IDS_AUTHOR); }
  const MCHAR *CopyrightMessage() override { return GetString(IDS_COPYRIGHT); }
  const MCHAR *OtherMessage1() override { return _T(""); }
  const MCHAR *OtherMessage2() override { return _T(""); }
  unsigned int Version() override { return 1; }

  void ShowAbout(HWND hWnd) override {}

  int DoExport(const MCHAR *name, ExpInterface *ei, Interface *i, BOOL suppressPrompts = FALSE, DWORD options = 0) override
  {
    util.exp_fname = name;
    util.ip = i;

    Autotoggle suppressPromptsGuard(util.suppressPrompts, suppressPrompts);

    // Not sure what is the correct solution here.
    // In case of errors we already show a message box but if we report back IMPEXP_FAIL then there
    // will be an additional message box shown to the user with no additional information.
    // So we only report back errors if message prompts are suppressed.
    BOOL success = util.export_dag();
    return (success || !are_prompts_suppressed()) ? IMPEXP_SUCCESS : IMPEXP_FAIL;
  }
};

class DagExpCD : public ClassDesc
{
public:
  int IsPublic() override { return TRUE; }
  void *Create(BOOL loading) override { return new DagExport; }
  const TCHAR *ClassName() override { return GetString(IDS_DAGEXP); }
  const MCHAR *NonLocalizedClassName() override { return ClassName(); }
  SClass_ID SuperClassID() override { return SCENE_EXPORT_CLASS_ID; }
  Class_ID ClassID() override { return DAGEXP_CID; }
  const TCHAR *Category() override { return _T(""); }

  const TCHAR *InternalName() override { return _T("DagExporter"); }
  HINSTANCE HInstance() override { return hInstance; }
};
static DagExpCD dagexpcd;

ClassDesc *GetDAGEXPCD() { return &dagexpcd; }

bool are_prompts_suppressed() { return util.suppressPrompts || GetCOREInterface()->GetQuietMode(); }

void explog(const TCHAR *s, ...)
{
  va_list ap;
  va_start(ap, s);
  DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Note, s, ap);
  va_end(ap);
}

void explogWarning(const TCHAR *s, ...)
{
  va_list ap;
  va_start(ap, s);
  DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Warning, s, ap);
  va_end(ap);
}

void explogError(const TCHAR *s, ...)
{
  va_list ap;
  va_start(ap, s);
  DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Error, s, ap);
  va_end(ap);
}

class ExpUtilDesc : public ClassDesc
{
public:
  int IsPublic() override { return 1; }
  void *Create(BOOL loading = FALSE) override { return &util; }
  const TCHAR *ClassName() override { return GetString(IDS_EXPUTIL_NAME); }
  const MCHAR *NonLocalizedClassName() override { return ClassName(); }
  SClass_ID SuperClassID() override { return UTILITY_CLASS_ID; }
  Class_ID ClassID() override { return ExpUtil_CID; }
  const TCHAR *Category() override { return GetString(IDS_UTIL_CAT); }
  BOOL NeedsToSave() override { return TRUE; }
  IOResult Save(ISave *) override;
  IOResult Load(ILoad *) override;
};

// Warning! These ids are stored in the Max file. The gaps are ids that are no longer written.
enum
{
  CH_EXP_FNAME = 1,
  CH_EXPHIDDEN = 2,
  CH_EXPFLG = 3,
  CH_ANIMEPS = 4,
  CH_ANIMRANGE = 5,
  CH_EXP_ANIM2_FNAME = 6,
  CH_ANIMEPS2 = 8,
  CH_EXP_PHYS_FNAME = 9,
  CH_EXP_MODE = 10,
};

IOResult ExpUtilDesc::Save(ISave *io)
{
  ULONG nw;
  if (!util.exp_fname.empty())
  {
    io->BeginChunk(CH_EXP_FNAME);

    if (io->WriteCString(util.exp_fname.c_str()) != IO_OK)
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

  if (!util.exp_anim2_fname.empty())
  {
    io->BeginChunk(CH_EXP_ANIM2_FNAME);
    if (io->WriteCString(util.exp_anim2_fname.c_str()) != IO_OK)
      return IO_ERROR;
    io->EndChunk();
  }
  if (!util.exp_phys_fname.empty())
  {
    io->BeginChunk(CH_EXP_PHYS_FNAME);
    if (io->WriteCString(util.exp_phys_fname.c_str()) != IO_OK)
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

static bool exportPathResetRegistered = false;

static void reset_export_paths(void *, NotifyInfo *)
{
  util.exp_fname.clear();
  util.exp_anim2_fname.clear();
  util.exp_phys_fname.clear();
  util.exp_instances_fname.clear();
}

void register_export_path_reset()
{
  if (exportPathResetRegistered)
    return;
  exportPathResetRegistered = true;
  RegisterNotification(reset_export_paths, nullptr, NOTIFY_FILE_PRE_OPEN);
  RegisterNotification(reset_export_paths, nullptr, NOTIFY_SYSTEM_PRE_NEW);
  RegisterNotification(reset_export_paths, nullptr, NOTIFY_SYSTEM_PRE_RESET);
}

void unregister_export_path_reset()
{
  if (!exportPathResetRegistered)
    return;
  exportPathResetRegistered = false;
  UnRegisterNotification(reset_export_paths, nullptr);
}

IOResult ExpUtilDesc::Load(ILoad *io)
{
  ULONG nr;
  TCHAR *str;
  util.expflg = EXP_DEFAULT;
  util.exportMode = ExpUtil::ExportMode::Standard;
  util.reset_anim_settings();
  while (io->OpenChunk() == IO_OK)
  {
    switch (io->CurChunkID())
    {
      case CH_EXP_FNAME:
        if (io->ReadCStringChunk(&str) != IO_OK)
          return IO_ERROR;
        util.exp_fname = str;
        break;
      case CH_EXP_ANIM2_FNAME:
        if (io->ReadCStringChunk(&str) != IO_OK)
          return IO_ERROR;
        util.exp_anim2_fname = str;
        break;
      case CH_EXP_PHYS_FNAME:
        if (io->ReadCStringChunk(&str) != IO_OK)
          return IO_ERROR;
        util.exp_phys_fname = str;
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
  util.update_ui();
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

BOOL ExpUtil::export_dlg_proc(HWND hw, UINT msg, WPARAM wpar, LPARAM lpar)
{
  switch (msg)
  {
    case WM_INITDIALOG: Init(hw); break;

    case WM_COMMAND:
    {
      WORD id = LOWORD(wpar);
      switch (id)
      {
        case IDC_EXPORT:
          if (exportMode == ExportMode::Standard)
          {
            if (input_exp_fname())
              export_dag();
          }
          else if (exportMode == ExportMode::LayersAsDags)
            exportLayersAsDags();
          else if (exportMode == ExportMode::ObjectsAsDags)
            exportObjectsAsDags();
          break;

        case IDC_EXPHIDDEN: set_expflg(EXP_HID, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPSEL: set_expflg(EXP_SEL, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPMESH: set_expflg(EXP_MESH, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXP_VNORM: set_expflg(EXP_NO_VNORM, !IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPLIGHT: set_expflg(EXP_LT, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPCAM: set_expflg(EXP_CAM, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPHELPER: set_expflg(EXP_HLP, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPSPLINE: set_expflg(EXP_SPLINE, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPMATER: set_expflg(EXP_MAT, IsDlgButtonChecked(hw, id)); break;
        case IDC_EXPMATEROPT: set_expflg(EXP_MATOPT, IsDlgButtonChecked(hw, id)); break;

        case IDC_EXPORT_MODE:
          if (HIWORD(wpar) == CBN_SELENDOK)
          {
            exportMode = (ExportMode)ComboBoxHelper::GetSelectedItemData((HWND)lpar, (LPARAM)ExportMode::Standard);
            update_tooltips();
          }
          break;
      }
    }
    break;
    default: return FALSE;
  }
  return TRUE;
}

BOOL ExpUtil::export_anim_dlg_proc(HWND hw, UINT msg, WPARAM wpar, LPARAM lpar)
{
  switch (msg)
  {
    case WM_INITDIALOG: InitAnim(hw); break;

    case WM_DESTROY: DestroyAnim(hw); break;

    case WM_COMMAND:
    {
      WORD id = LOWORD(wpar);
      switch (id)
      {
        case IDC_ARANGE: set_expflg(EXP_ARNG, IsDlgButtonChecked(hw, id)); break;
        case IDC_REDUCE_POS: set_expflg(EXP_DONT_REDUCE_POS, !IsDlgButtonChecked(hw, id)); break;
        case IDC_LOOP_ANIM: set_expflg(EXP_LOOPED_ANIM, IsDlgButtonChecked(hw, id)); break;
        case IDC_REDUCE_ROT: set_expflg(EXP_DONT_REDUCE_ROT, !IsDlgButtonChecked(hw, id)); break;
        case IDC_REDUCE_SCL: set_expflg(EXP_DONT_REDUCE_SCL, !IsDlgButtonChecked(hw, id)); break;
        case IDC_ASTART:
        {
          TSTR s;
          s.Resize(64);
          eastart->GetText(s.dataForWrite(), 64);
          StringToTime(s, astart);
        }
        break;
        case IDC_AEND:
        {
          TSTR s;
          s.Resize(64);
          eaend->GetText(s.dataForWrite(), 64);
          StringToTime(s, aend);
        }
        break;
        case IDC_POSEPS: poseps = epose->GetFloat(); break;
        case IDC_SCLEPS: scleps = escle->GetFloat() / 100; break;
        case IDC_ROTEPS: roteps = DegToRad(erote->GetFloat()); break;
        case IDC_ORTEPS: orteps = DegToRad(eorte->GetFloat()); break;
        case IDC_ORIGIN_LINVEL_EPS: poseps2 = epose2->GetFloat(); break;
        case IDC_ORIGIN_ANGVEL_EPS: roteps2 = DegToRad(erote2->GetFloat()); break;

        case IDC_EXPORT_ANIM2:
          if (input_exp_anim2_fname())
            export_anim_v2();
          break;

        default: break;
      }
    }
    break;
    default: return FALSE;
  }
  return TRUE;
}


BOOL ExpUtil::export_other_dlg_proc(HWND hw, UINT msg, WPARAM wpar, LPARAM lpar)
{
  static bool prevent_enchange = false;

  switch (msg)
  {
    case WM_INITDIALOG: update_ui_other(hw); break;

    case WM_COMMAND:
    {
      WORD id = LOWORD(wpar);
      switch (id)
      {
        case IDC_CALC_MOMJ: calcMomj(); break;
        case IDC_CALC_MOMJ_ON_EXPORT: set_expflg(EXP_DONT_CALC_MOMJ, !IsDlgButtonChecked(hw, id)); break;

        case IDC_SET_DAGORPATH:
        {
          TCHAR dir[MAX_PATH] = {};

          _tcsncpy_s(dir, _countof(dir), dagor_path.c_str(), _TRUNCATE);

          ip->ChooseDirectory(hw, GetString(IDS_CHOOSE_DAGOR_PATH), dir);
          if (dir[0])
          {
            set_dagor_path(dir);
            update_path_edit_control(hw, IDC_DAGORPATH, dagor_path);
          }
        }
        break;

        case IDC_DAGORPATH:
          switch (HIWORD(wpar))
          {
            case EN_SETFOCUS: DisableAccelerators(); break;
            case EN_KILLFOCUS:
              EnableAccelerators();
              set_dagor_path(get_window_text(GetDlgItem(hw, IDC_DAGORPATH)));
              break;

            case EN_CHANGE:
              if (!prevent_enchange)
              {
                const std::wstring path = get_window_text(GetDlgItem(hw, IDC_DAGORPATH));

                std::wstring new_path = drop_quotation_marks(path);
                if (path != new_path)
                {
                  Autotoggle eguard(prevent_enchange);
                  update_path_edit_control(hw, IDC_DAGORPATH, new_path);
                }
              }
              break;
          }
          break;

        case IDC_EXPORT_PHYS:
          if (input_exp_phys_fname())
            exportPhysics();
          break;

        case IDC_EXPORT_INSTANCES:
          if (input_exp_instances_fname())
            export_instances();
          break;

        default: break;
      }
    }
    break;

    default: return FALSE;
  }
  return TRUE;
}

static INT_PTR CALLBACK ExpDagDlgProc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return util.export_dlg_proc(hw, msg, wParam, lParam);
}

static INT_PTR CALLBACK ExportAnimDlgProc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return util.export_anim_dlg_proc(hw, msg, wParam, lParam);
}

static INT_PTR CALLBACK ExportOtherDlgProc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return util.export_other_dlg_proc(hw, msg, wParam, lParam);
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
  hExpDag = hExpAnim = hExpOther = hLog = NULL;

  expflg = EXP_DEFAULT;
  exportMode = ExportMode::Standard;
  reset_anim_settings();

  suppressPrompts = false;
  separateLayers = false;
}

void ExpUtil::reset_anim_settings()
{
  astart = GetAnimStart();
  aend = GetAnimEnd();
  poseps = 0.005f;
  scleps = 0.01f;
  roteps = DegToRad(1);
  orteps = DegToRad(1);

  poseps2 = 0.005f;
  roteps2 = DegToRad(1);
}

void ExpUtil::BeginEditParams(Interface *ip, IUtil *iu)
{
  this->iu = iu;
  this->ip = ip;

  hExpDag = add_rollup_page(ip, IDD_EXPUTIL, ExpDagDlgProc, _T("DAG"));
  hExpAnim = add_rollup_page(ip, IDD_EXPANIM, ExportAnimDlgProc, _T("Animation"));
  hExpOther = add_rollup_page(ip, IDD_EXPOTHER, ExportOtherDlgProc, _T("Other"), APPENDROLL_CLOSED);
  hLog = add_rollup_page(ip, IDD_LOGROLL, LogDlgProc, GetString(IDS_EXPLOG_ROLL), APPENDROLL_CLOSED);
}

void ExpUtil::EndEditParams(Interface *ip, IUtil *iu)
{
  this->iu = NULL;
  this->ip = NULL;

  delete_rollup_page(ip, &hExpDag);
  delete_rollup_page(ip, &hExpAnim);
  delete_rollup_page(ip, &hExpOther);
  delete_rollup_page(ip, &hLog);
}

void ExpUtil::update_tooltips()
{
  HWND expmode = GetDlgItem(hExpDag, IDC_EXPORT_MODE);
  HWND exphidden = GetDlgItem(hExpDag, IDC_EXPHIDDEN);
  HWND expsel = GetDlgItem(hExpDag, IDC_EXPSEL);

  if (exportMode == ExportMode::LayersAsDags)
  {
    tooltipExtender.SetToolTip(expmode, _T("Layers are exported as separate dag files."));
    tooltipExtender.SetToolTip(exphidden,
      _T("Export hidden layers inside layers\n(if those layers should be exported based on other parameters)."));
    tooltipExtender.SetToolTip(expsel, _T("Process only current layer and children of it\n(The default layer is only exported if ")
                                       _T("it's the current layer and \"sel\" is active.)"));
    return;
  }

  if (exportMode == ExportMode::ObjectsAsDags)
  {
    tooltipExtender.SetToolTip(expmode, _T("Objects are exported as separate dag files."));
    tooltipExtender.SetToolTip(exphidden, _T("Export hidden objects"));
    tooltipExtender.SetToolTip(expsel, _T("Export only selected objects"));
    return;
  }

  tooltipExtender.SetToolTip(expmode, _T("Export one file."));
  tooltipExtender.SetToolTip(exphidden, L"Export hidden objects\n(a layered export also writes the files of hidden layers)");
  tooltipExtender.SetToolTip(expsel, _T("Export only selected objects"));
}

void ExpUtil::update_ui_dag(HWND hw)
{
  if (!hw)
    return;

  HWND exportModeComboBox = GetDlgItem(hw, IDC_EXPORT_MODE);
  ComboBox_SetCurSel(exportModeComboBox, ComboBoxHelper::GetItemIndexByData(exportModeComboBox, (LPARAM)exportMode));

  CheckDlgButton(hw, IDC_EXPHIDDEN, expflg & EXP_HID);
  CheckDlgButton(hw, IDC_EXPSEL, expflg & EXP_SEL);
  CheckDlgButton(hw, IDC_EXPMESH, expflg & EXP_MESH);
  CheckDlgButton(hw, IDC_EXP_VNORM, !(expflg & EXP_NO_VNORM));
  CheckDlgButton(hw, IDC_EXPLIGHT, expflg & EXP_LT);
  CheckDlgButton(hw, IDC_EXPCAM, expflg & EXP_CAM);
  CheckDlgButton(hw, IDC_EXPHELPER, expflg & EXP_HLP);
  CheckDlgButton(hw, IDC_EXPSPLINE, expflg & EXP_SPLINE);
  CheckDlgButton(hw, IDC_EXPMATER, expflg & EXP_MAT);
  CheckDlgButton(hw, IDC_EXPMATEROPT, expflg & EXP_MATOPT);
}

void ExpUtil::update_ui_anim(HWND hw)
{
  if (!hw)
    return;

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

  CheckDlgButton(hw, IDC_ARANGE, expflg & EXP_ARNG);
  CheckDlgButton(hw, IDC_REDUCE_POS, !(expflg & EXP_DONT_REDUCE_POS));
  CheckDlgButton(hw, IDC_REDUCE_ROT, !(expflg & EXP_DONT_REDUCE_ROT));
  CheckDlgButton(hw, IDC_REDUCE_SCL, !(expflg & EXP_DONT_REDUCE_SCL));
  CheckDlgButton(hw, IDC_LOOP_ANIM, expflg & EXP_LOOPED_ANIM);
}

void ExpUtil::update_ui_other(HWND hw)
{
  if (!hw)
    return;

  CheckDlgButton(hw, IDC_CALC_MOMJ_ON_EXPORT, !(expflg & EXP_DONT_CALC_MOMJ));
  update_path_edit_control(hw, IDC_DAGORPATH, dagor_path);
}

void ExpUtil::update_ui()
{
  update_ui_dag(hExpDag);
  update_ui_anim(hExpAnim);
  update_ui_other(hExpOther);
}

void ExpUtil::Init(HWND hw)
{
  hExpDag = hw;

  HWND exportModeComboBox = GetDlgItem(hw, IDC_EXPORT_MODE);
  ComboBoxHelper::AddItemWithData(exportModeComboBox, _T("Standard"), (LPARAM)ExportMode::Standard);
  ComboBoxHelper::AddItemWithData(exportModeComboBox, _T("Objects as dags"), (LPARAM)ExportMode::ObjectsAsDags);
  ComboBoxHelper::AddItemWithData(exportModeComboBox, _T("Layers as dags"), (LPARAM)ExportMode::LayersAsDags);
  update_ui_dag(hw);

  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPMESH), TSTR(_T("Export mesh")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPLIGHT), TSTR(_T("Export light")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPCAM), TSTR(_T("Export camera")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPHELPER), TSTR(_T("Export helper")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXP_VNORM), TSTR(_T("Export vertex normals")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPMATER), TSTR(_T("Export materials")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPMATEROPT), TSTR(_T("Enable material optimization")));
  tooltipExtender.SetToolTip(GetDlgItem(hw, IDC_EXPSPLINE), TSTR(_T("Export splines")));
  update_tooltips();

  SendMessage(tooltipExtender.GetToolTipHWND(), TTM_SETDELAYTIME, TTDT_AUTOPOP, 32000);
}

void ExpUtil::InitAnim(HWND hw)
{
  eastart.reset(GetICustEdit(GetDlgItem(hw, IDC_ASTART)));
  eaend.reset(GetICustEdit(GetDlgItem(hw, IDC_AEND)));
  epose.reset(GetICustEdit(GetDlgItem(hw, IDC_POSEPS)));
  erote.reset(GetICustEdit(GetDlgItem(hw, IDC_ROTEPS)));
  escle.reset(GetICustEdit(GetDlgItem(hw, IDC_SCLEPS)));
  eorte.reset(GetICustEdit(GetDlgItem(hw, IDC_ORTEPS)));
  epose2.reset(GetICustEdit(GetDlgItem(hw, IDC_ORIGIN_LINVEL_EPS)));
  erote2.reset(GetICustEdit(GetDlgItem(hw, IDC_ORIGIN_ANGVEL_EPS)));
  update_ui_anim(hw);
}

void ExpUtil::DestroyAnim(HWND hw)
{
  hExpAnim = NULL;
  eastart.reset();
  eaend.reset();
  epose.reset();
  erote.reset();
  escle.reset();
  eorte.reset();
  epose2.reset();
  erote2.reset();
}

int ExpUtil::input_exp_fname()
{
  FilterList fl;
  fl.Append(GetString(IDS_SCENE_FILES));
  fl.Append(_T("*.dag"));
  return get_save_filename(hExpDag, GetString(IDS_SAVE_SCENE_TITLE), fl, _T("dag"), exp_fname);
}

int ExpUtil::input_exp_anim2_fname()
{
  FilterList fl;
  fl.Append(GetString(IDS_ANIM2_FILES));
  fl.Append(_T ("*.a2d"));
  return get_save_filename(hExpAnim, GetString(IDS_SAVE_ANIM2_TITLE2), fl, _T("a2d"), exp_anim2_fname);
}

int ExpUtil::input_exp_phys_fname()
{
  FilterList fl;
  fl.Append(GetString(IDS_PHYS_FILES));
  fl.Append(_T ("*.dphys"));
  return get_save_filename(hExpOther, GetString(IDS_SAVE_PHYS_TITLE), fl, _T("dphys"), exp_phys_fname);
}

int ExpUtil::input_exp_instances_fname()
{
  FilterList fl;
  fl.Append(_T("Instances placement (*.blk)"));
  fl.Append(_T("*.blk"));
  return get_save_filename(hExpOther, _T("Save instances placement..."), fl, _T("blk"), exp_instances_fname);
}

namespace
{
class DagWriter
{
public:
  DagWriter() = default;
  DagWriter(const DagWriter &) = delete;
  DagWriter &operator=(const DagWriter &) = delete;

  void write(const void *data, size_t size)
  {
    const uint8_t *bytes = static_cast<const uint8_t *>(data);
    buf.insert(buf.end(), bytes, bytes + size);
  }

  template <class T>
  void writeValue(const T &value)
  {
    writeArray(&value, 1);
  }

  template <class T>
  void writeArray(const T *data, size_t count)
  {
    static_assert(std::is_trivially_copyable_v<T>);
    write(data, count * sizeof(T));
  }

  template <class T, class V>
  void writeAs(const V &value)
  {
    static_assert(std::is_unsigned_v<T>);
    static_assert(!std::is_floating_point_v<V>);
    writeValue(static_cast<T>(value));
  }

  void writeString(std::string_view str)
  {
    const size_t len = std::min<size_t>(str.size(), 255);
    writeAs<uint8_t>(len);
    write(str.data(), len);
  }

  bool writeTo(FILE *file) const { return buf.empty() || fwrite(buf.data(), buf.size(), 1, file) == 1; }

  class [[nodiscard]] Block
  {
  public:
    Block(DagWriter &writer, uint32_t id) : writer(writer)
    {
      writer.writeAs<uint32_t>(0u);
      writer.writeAs<uint32_t>(id);
      bodyOfs = writer.buf.size();
    }

    ~Block()
    {
      const uint32_t size = static_cast<uint32_t>(writer.buf.size() - bodyOfs + 4);
      memcpy(writer.buf.data() + bodyOfs - 8, &size, sizeof(size));
    }

    Block(const Block &) = delete;
    Block &operator=(const Block &) = delete;

  private:
    DagWriter &writer;
    size_t bodyOfs;
  };

private:
  std::vector<uint8_t> buf;
};

template <class F>
class [[nodiscard]] OnExit
{
public:
  explicit OnExit(F f) : f(std::move(f)) {}
  ~OnExit() { f(); }
  OnExit(const OnExit &) = delete;
  OnExit &operator=(const OnExit &) = delete;

private:
  F f;
};
} // namespace

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

struct EMat
{
  std::wstring name;
  Mtl *mtl;
  INode *node;
  DWORD wirecolor;

  EMat(std::wstring_view nm, Mtl *m, INode *n, DWORD wc) : name(nm), mtl(m), node(n), wirecolor(wc) {}
};

struct ExpMat
{
  DagMater m;
  std::wstring name, classname, script;
  Mtl *mtl;
  DWORD wirecolor;

  ExpMat() : mtl(0), wirecolor(0) {}
};

class MyExpTMAnimCB : public ExpTMAnimCB
{
public:
  INode *node, *pnode, *origin;

  MyExpTMAnimCB(INode *n, INode *pn, INode *orig)
  {
    node = n;
    pnode = pn;
    origin = orig;
  }
  void interp_tm(TimeValue t, Matrix3 &m) override
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
};

struct ExpNode
{
  std::vector<ExpNode *> child;
  ExpNode *parent = nullptr;
  ushort id;

  explicit ExpNode(int i) : id(static_cast<ushort>(i)) {}
  void add_child(ExpNode *n)
  {
    child.push_back(n);
    n->parent = this;
  }
};

const char origin_lin_vel_node_name[] = "origin_lin_vel_node_name";
const char origin_ang_vel_node_name[] = "origin_ang_vel_node_name";

static int face_mtl_slot(MtlID face_mtl_id, int sub_mtl_num) { return face_mtl_id % sub_mtl_num; }

static bool any_explicit_normals_set(const MeshNormalSpec *normalSpec)
{
#if defined(MAX_RELEASE_R26) && MAX_RELEASE >= MAX_RELEASE_R26
  return normalSpec->AnyExplicitNormalsSet();
#else
  for (int i = 0, num = normalSpec->GetNumNormals(); i < num; ++i)
    if (normalSpec->GetNormalExplicit(i))
      return true;
  return false;
#endif
}

static bool exported_map_channel(Mesh &m, int ch) { return m.mapSupport(ch) && m.mapFaces(ch) && m.getNumMapVerts(ch) > 0; }

static bool is_blank(wchar_t c) { return c == L' ' || c == L'\t'; }

static size_t line_start(std::wstring_view script, size_t at)
{
  const size_t nl = script.rfind(L'\n', at);
  return nl == std::wstring_view::npos ? 0 : nl + 1;
}

static size_t skip_blanks(std::wstring_view script, size_t from)
{
  while (from < script.size() && is_blank(script[from]))
    ++from;
  return from;
}

static size_t skip_blanks_back(std::wstring_view script, size_t at)
{
  while (at > 0 && is_blank(script[at - 1]))
    --at;
  return at;
}

static bool is_param_start(std::wstring_view script, size_t at)
{
  const size_t lineStart = line_start(script, at);

  const size_t firstOnLine = skip_blanks(script, lineStart);
  if (script.compare(firstOnLine, 2, L"//") == 0)
    return false;

  const size_t paramStart = skip_blanks_back(script, at);
  return paramStart == lineStart || script[paramStart - 1] == L';' || script[paramStart - 1] == L'"' ||
         script[paramStart - 1] == L'\'';
}

static size_t find_param(std::wstring_view script, std::wstring_view key)
{
  size_t at = script.find(key);
  while (at != std::wstring_view::npos && !is_param_start(script, at))
    at = script.find(key, at + 1);

  return at;
}

static std::wstring_view find_quoted_value(std::wstring_view script, std::wstring_view key)
{
  const size_t at = find_param(script, key);
  if (at == std::wstring_view::npos)
    return {};

  const size_t from = at + key.size();
  const size_t to = script.find(L'"', from);
  if (to == std::wstring_view::npos)
    return {};

  return script.substr(from, to - from);
}

static std::wstring blk_escape(std::wstring_view name)
{
  std::wstring res = replace_all(std::wstring(name), L"~", L"~~");
  res = replace_all(res, L"\"", L"~\"");
  res = replace_all(res, L"\r", L"~r");
  return replace_all(res, L"\n", L"~n");
}

class ExportENCB : public ENodeCB
{
public:
  TimeValue time;
  Tab<INode *> node;
  Tab<bool> nodeExp; // used in a2d export to denote nodes that should be exported; 'node' contains all nodes
  std::vector<ExpMat> mat;
  std::vector<std::unordered_set<Mtl *>> mtls;
  Tab<int> matIDtoMatIdx;
  std::vector<std::wstring> tex;
  std::unordered_map<std::wstring, int, CaseInsensitiveHashW, CaseInsensitiveEqualW> texIndexMap;
  std::vector<EMat> matList;
  std::unordered_set<std::wstring> matNames;
  std::unordered_set<DWORD> wcset;
  int autoMatNameCounter;
  std::unordered_map<INode *, int> nodeIdMap;
  INode *nodeOrigin;
  INode *useIdentityTransformForNode;
  std::unordered_set<INode *> skipNodes;
  char nofaces;
  bool hasDegenerateTriangles;
  bool hasNoSmoothing;
  bool hasBigMeshes;
  bool hasNonDagorMaterials;
  bool hasNonDagorLights;
  bool hasSubSubMaterials;
  bool a2dExported;

  explicit ExportENCB(TimeValue t, bool a2d = false)
  {
    time = t;
    nofaces = 0;
    hasDegenerateTriangles = false;
    hasNoSmoothing = false;
    hasBigMeshes = false;
    hasNonDagorMaterials = false;
    hasNonDagorLights = false;
    hasSubSubMaterials = false;
    nodeOrigin = NULL;
    useIdentityTransformForNode = nullptr;
    a2dExported = a2d;
    autoMatNameCounter = 0;
  }

  int add_tex(const TCHAR *fn)
  {
    if (!fn)
      return -1;
    if (!fn[0])
      return -1;
    auto it = texIndexMap.find(std::wstring_view(fn));
    if (it != texIndexMap.end())
      return it->second;
    assert(tex.size() != 0xFFFF);
    if (tex.size() >= 0xFFFF)
      return -1;
    int idx = static_cast<int>(tex.size());
    tex.emplace_back(fn);
    texIndexMap.emplace(fn, idx);
    return idx;
  }

  bool doesScriptMatch(std::wstring_view a, std::wstring_view b)
  {
    auto split_lines = [](std::wstring_view s) {
      std::vector<std::wstring> lines;
      for (auto sub :
        s | std::views::split(std::wstring_view(L"\r\n")) | std::views::filter([](auto &&sub) { return !std::ranges::empty(sub); }))
        lines.emplace_back(sub.begin(), sub.end());
      std::ranges::sort(lines);
      return lines;
    };

    return split_lines(a) == split_lines(b);
  }

  void prepare_expmat()
  {
    for (const auto &em : matList)
    {
      if (!em.mtl)
      {
        assert(mat.size() != 0xFFFF);
        if (mat.size() >= 0xFFFF)
          return;

        ExpMat e;
        e.name = em.name;
        e.mtl = 0;
        e.wirecolor = em.wirecolor;

        e.m.flags = 0;
        e.m.amb = e.m.diff = Color(em.wirecolor);
        e.m.spec = e.m.emis = Color(0, 0, 0);
        e.m.power = 0;

        for (int i = 0; i < DAGTEXNUM; ++i)
          e.m.texid[i] = DAGBADMATID;

        mtls.push_back(std::unordered_set<Mtl *>());
        mat.push_back(std::move(e));
        continue;
      }

      if (em.mtl->NumSubMtls())
        continue;

      Class_ID cid = em.mtl->ClassID();
      if (cid == Class_ID(DMTL_CLASS_ID, 0))
      {
        explogWarning(_T("'%s' has standard material '%s'\r\n"), em.node->GetName(), em.name.data());
        hasNonDagorMaterials = true;

        assert(mat.size() != 0xFFFF);
        if (mat.size() >= 0xFFFF)
          return;

        StdMat *m = (StdMat *)em.mtl;

        ExpMat e;
        e.name = em.name;
        e.mtl = em.mtl;
        e.wirecolor = 0;

        e.m.flags = DAG_MF_16TEX;
        if (m->GetTwoSided())
          e.m.flags |= DAG_MF_2SIDED;

        float si = m->GetSelfIllum(time);
        e.m.amb = m->GetAmbient(time) * (1 - si);
        e.m.diff = m->GetDiffuse(time) * (1 - si);
        e.m.spec = m->GetSpecular(time) * m->GetShinStr(time);
        e.m.emis = m->GetDiffuse(time) * si;
        e.m.power = powf(2.0f, m->GetShininess(time) * 10.0f) * 4.0f;

        for (int i = 0; i < DAGTEXNUM; ++i)
          e.m.texid[i] = DAGBADMATID;

        Texmap *tex = m->GetSubTexmap(ID_DI);
        if (tex)
          if (tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0))
          {
            BitmapTex *b = (BitmapTex *)tex;
            e.m.texid[0] = add_tex(make_path_rel(b->GetMapName()));
            if (e.m.texid[0] != DAGBADMATID)
            {
              e.m.amb = e.m.diff = Color(1, 1, 1) * (1 - si);
              e.m.emis = Color(1, 1, 1) * si;
            }
          }
        e.classname = L"simple";
        e.script = L"lighting=vltmap";

        mtls.push_back(std::unordered_set<Mtl *>());
        mtls.back().insert(em.mtl);
        mat.push_back(std::move(e));
        continue;
      }

      if (cid == DagorMat_CID || cid == DagorMat2_CID)
      {
        assert(mat.size() != 0xFFFF);
        if (mat.size() >= 0xFFFF)
          return;

        IDagorMat *m = (IDagorMat *)em.mtl->GetInterface(I_DAGORMAT);
        assert(m);

        ExpMat e;
        e.name = em.name;
        e.mtl = em.mtl;
        e.wirecolor = 0;

        e.m.flags = DAG_MF_16TEX;
        if (m->get_2sided() == IDagorMat::Sides::DoubleSided)
          e.m.flags |= DAG_MF_2SIDED;

        e.m.amb = m->get_amb();
        e.m.diff = m->get_diff();
        e.m.spec = m->get_spec();
        e.m.emis = m->get_emis();
        e.m.power = m->get_power();

        for (int i = 0; i < DAGTEXNUM; ++i)
          e.m.texid[i] = DAGBADMATID;

        e.classname = m->get_classname();
        e.script = m->get_script();

        if (!isProxymatName(e.classname))
          for (int i = 0; i < DAGTEXNUM; ++i)
            e.m.texid[i] = add_tex(make_path_rel(m->get_texname(i)));

        em.mtl->ReleaseInterface(I_DAGORMAT, m);

        mtls.push_back(std::unordered_set<Mtl *>());
        mtls.back().insert(em.mtl);
        mat.push_back(std::move(e));
        continue;
      }

      explogWarning(_T("'%s' has material '%s' of unknown type\r\n"), em.node->GetName(), em.name.data());
      hasNonDagorMaterials = true;
    }
  }

  static bool isPlaceholderMat(const ExpMat &m) { return !m.mtl; }

  bool equal_dagormats(const ExpMat &mat_a, const ExpMat &mat_b)
  {
    if (isPlaceholderMat(mat_a) || isPlaceholderMat(mat_b))
      return false;

    if (mat_a.classname.empty())
      return false;

    if (mat_b.classname.empty())
      return false;

    bool is_proxy_a = isProxymatName(mat_a.classname);
    bool is_proxy_b = isProxymatName(mat_b.classname);
    if (is_proxy_a != is_proxy_b)
      return false;

    if (is_proxy_a)
      return iequal(mat_a.classname, mat_b.classname);
    else
    {
      if (!iequal(mat_a.classname, mat_b.classname))
        return false;
    }

    if ((mat_a.m.flags & DAG_MF_2SIDED) != (mat_b.m.flags & DAG_MF_2SIDED))
      return false;

    if (memcmp(&mat_a.m.diff, &mat_b.m.diff, sizeof(mat_a.m.diff)) != 0)
      return false;

    for (int i = 0; i < DAGTEXNUM; ++i)
    {
      unsigned short texid_a = mat_a.m.texid[i];
      unsigned short texid_b = mat_b.m.texid[i];

      // skip empty slot
      if (texid_a == DAGBADMATID && texid_b == DAGBADMATID)
        continue;

      // empty != non-empty
      if (texid_a == DAGBADMATID || texid_b == DAGBADMATID)
        return false;

      // FIXME special symbols
      const std::wstring tex_a = fs::path(tex[texid_a]).stem().wstring();
      const std::wstring tex_b = fs::path(tex[texid_b]).stem().wstring();
      if (!iequal(tex_a, tex_b))
        return false;
    }

    return doesScriptMatch(normalize_param_values(mat_a.script, mat_a.classname),
      normalize_param_values(mat_b.script, mat_b.classname));
  }

  void optimize_materials(Tab<bool> &matUsed)
  {
    for (size_t i = 0; i < mat.size(); ++i)
    {
      const auto &m = mat[i];
      if (!matUsed[i])
        continue;

      for (size_t j = i + 1; j < mat.size(); ++j)
      {
        const auto &em = mat[j];
        if (!matUsed[j])
          continue;

        if (equal_dagormats(em, m))
        {
          mtls[i].insert(em.mtl);
          matUsed[j] = false;
        }
      }
    }
  }

  std::wstring makeUniqueMatName()
  {
    for (;;)
    {
      std::wstring name = std::format(_T("autoNamedMat_{}"), autoMatNameCounter++);
      if (matNames.find(name) == matNames.end())
        return name;
    }
  }

  void add_mtl(INode *node, Mtl *mtl, int subm = 0, DWORD wirecolor = 0xFFFFFF)
  {
    std::wstring mtl_name;

    if (mtl)
    {
      mtl_name = mtl->GetName().data();
      if (mtl_name.empty())
        mtl_name = makeUniqueMatName();

      int num_sub_mtls = mtl->NumSubMtls();
      if (num_sub_mtls && subm)
      {
        explogWarning(_T("'%s' has Multi/Sub-Object material '%s' in Multi/Sub-Object material\r\n"), node->GetName(),
          mtl_name.data());
        hasSubSubMaterials = true;
        return;
      }

      for (int i = 0; i < num_sub_mtls; ++i)
        add_mtl(node, mtl->GetSubMtl(i), true);

      auto it = std::ranges::find_if(matList, [mtl](EMat &em) { return em.mtl == mtl; });
      if (it != matList.end())
        return;
    }
    else // mtl == 0
    {
      if (wcset.find(wirecolor) != wcset.end())
        return;

      mtl_name = makeUniqueMatName();
      wcset.emplace(wirecolor);
    }

    matList.emplace_back(EMat(mtl_name, mtl, node, wirecolor));
    matNames.insert(mtl_name);
  }

  int proc(INode *n) override
  {
    if (!n)
      return ECB_CONT;
    if (skipNodes.count(n))
      return ECB_CONT;
    if (iequal(n->GetName(), L"ORIGIN"))
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
      return add_node(n);
    }

    if (!(util.expflg & EXP_HID))
    {
      if (n->IsNodeHidden())
        return ECB_CONT;
    }
    if (util.expflg & EXP_SEL)
      if (!n->Selected())
        return ECB_CONT;
    if ((util.expflg & EXP_MAT) && !(util.expflg & EXP_OBJECTS))
      add_mtl(n, n->GetMtl(), 0, n->GetWireColor());

    if (!(util.expflg & EXP_OBJECTS))
      return ECB_CONT;

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
    }
    if (!obj)
    {
      if (!(util.expflg & EXP_HLP))
        if (!n->NumberOfChildren())
          return ECB_CONT;
    }

    if (n->Renderable() && !n->GetMtl() && !n->IsGroupHead())
    {
      explogWarning(_T( "'%s' has no material\r\n"), n->GetName());
      hasNonDagorMaterials = true;
    }
    if (util.expflg & EXP_MAT)
      add_mtl(n, n->GetMtl(), 0, n->GetWireColor());

    return add_node(n);
  }

  int add_node(INode *n)
  {
    assert(node.Count() != 0xFFFF);
    if (node.Count() >= 0xFFFF)
      return ECB_CONT;
    nodeIdMap[n] = node.Count();
    node.Append(1, &n);
    return ECB_CONT;
  }

  int getnodeid(INode *n)
  {
    if (!n)
      return -1;
    auto it = nodeIdMap.find(n);
    return it != nodeIdMap.end() ? it->second : -1;
  }
  int getPlaceholderMatId(DWORD wc)
  {
    for (int i = 0; i < (int)mat.size(); ++i)
      if (isPlaceholderMat(mat[i]) && mat[i].wirecolor == wc)
        return i;
    return -1;
  }

  int getmatid(Mtl *m, DWORD wc = 0xFFFFFF)
  {
    if (!m)
      return getPlaceholderMatId(wc);
    for (int i = 0; i < mtls.size(); ++i)
      if (mtls[i].find(m) != mtls[i].end())
        return i;
    return -1;
  }

  int getusedmatid(Mtl *m, DWORD wc = 0xFFFFFF)
  {
    if (!m)
      return getPlaceholderMatId(wc);
    for (int i = 0; i < (int)mat.size(); ++i)
      if (mat[i].mtl == m)
        return i;
    return -1;
  }

  static MtlID resolve_face_mtl(MtlID face_mtl_id, const Tab<int> &sub_mtl_id_lut, int single_mtl_idx)
  {
    if (sub_mtl_id_lut.Count())
      return (MtlID)sub_mtl_id_lut[face_mtl_slot(face_mtl_id, sub_mtl_id_lut.Count())];

    return single_mtl_idx >= 0 ? (MtlID)single_mtl_idx : face_mtl_id;
  }

  bool checkDegenerateTriangle(INode *node, Matrix3 &applied_transform, unsigned int index1, unsigned int index2, unsigned int index3,
    Point3 &vertex1, Point3 &vertex2, Point3 &vertex3);

  bool useMOpt() const { return (util.expflg & EXP_MATOPT) && (util.expflg & EXP_MESH); }

  std::wstring_view findMergeSurvivorName(std::wstring_view named) const
  {
    for (int i = 0; i < (int)mtls.size(); ++i)
    {
      if (matIDtoMatIdx[i] < 0)
        continue;

      if (named == mat[i].name)
        return {};

      if (std::ranges::any_of(mtls[i], [named](Mtl *m) { return m && named == m->GetName().data(); }))
        return mat[i].name;
    }

    return {};
  }

  void rewriteApexInteriorMaterial(std::wstring &script) const
  {
    static constexpr std::wstring_view key = L"apex_interior_material:t=\"";

    const std::wstring_view named = find_quoted_value(script, key);
    if (named.empty())
      return;

    const std::wstring_view survivor = findMergeSurvivorName(named);
    if (!survivor.empty())
      script.replace(named.data() - script.data(), named.size(), blk_escape(survivor));
  }

  bool save_node(ExpNode *enod, DagWriter &w)
  {

    float masterScale = static_cast<float>(GetSystemUnitScale(UNITS_METERS));

    Matrix3 originTm;
    if (nodeOrigin)
    {
      originTm = get_scaled_stretch_node_tm(nodeOrigin, time);
      adjwtm(originTm);
    }
    else
      originTm.IdentityMatrix();

    DagWriter::Block nodeBlk(w, DAG_NODE);
    INode *n = node[enod->id];
    INode *pnode = NULL;
    {
      DagWriter::Block nodeDataBlk(w, DAG_NODE_DATA);
      DagNodeData d;
      uint pid = enod->parent->id;
      if (pid < node.Count())
        pnode = node[pid];
      d.id = enod->id;
      d.cnum = static_cast<unsigned short>(enod->child.size());
      d.flg = 0;
      if (n->Renderable())
        d.flg |= DAG_NF_RENDERABLE;
      if (n->CastShadows())
        d.flg |= DAG_NF_CASTSHADOW;
      if (n->RcvShadows())
        d.flg |= DAG_NF_RCVSHADOW;
      const ObjectState &os = n->EvalWorldState(time);
      if (os.obj && os.obj->SuperClassID() == LIGHT_CLASS_ID)
      {
        LightObject *o = static_cast<LightObject *>(os.obj);
        if (o->GetShadow())
          d.flg |= DAG_NF_CASTSHADOW;
        else
          d.flg &= ~DAG_NF_CASTSHADOW;
      }
      w.writeValue(d);
      const TCHAR *nm = n->GetName();
      if (nm)
      {
        std::string nameCopy = wideToStr(nm);
        w.write(nameCopy.data(), nameCopy.size());
      }
    }

    {
      if (!RollupPanel::correctUserProp(n))
        explogWarning(L"'%s' has object properties that cannot be read as .blk, so they are exported unchanged\r\n", n->GetName());

      TSTR s;
      n->GetUserPropBuffer(s);
      std::wstring trimmed = trim_params(s.data());

      // TODO: create a check for presence of billboards

      if (useMOpt())
        rewriteApexInteriorMaterial(trimmed);

      std::string scr = wideToStr(trimmed);
      DagWriter::Block nodeScriptBlk(w, DAG_NODE_SCRIPT);
      w.write(scr.c_str(), scr.length());
    }
    Tab<int> subMatIdLUT;
    if (util.expflg & EXP_MAT)
    {
      Mtl *mtl = n->GetMtl();
      int num = 0;
      if (mtl)
        num = mtl->NumSubMtls();
      if (num)
      {
        Tab<int> uniqueMatIdx;
        subMatIdLUT.SetCount(num);
        // a slot we cannot export points at the first one we can
        for (int j = 0; j < num; ++j)
          subMatIdLUT[j] = 0;

        for (int j = 0; j < num; ++j)
        {
          int id = getmatid(mtl->GetSubMtl(j));
          if (id < 0 || matIDtoMatIdx[id] < 0)
            continue;

          bool found = false;
          for (int x = 0; x < uniqueMatIdx.Count(); ++x)
            if (uniqueMatIdx[x] == matIDtoMatIdx[id])
            {
              found = true;
              subMatIdLUT[j] = x;
              break;
            }
          if (!found)
          {
            uniqueMatIdx.Append(1, &matIDtoMatIdx[id]);
            subMatIdLUT[j] = uniqueMatIdx.Count() - 1;
          }
        }

        if (uniqueMatIdx.Count())
        {
          DagWriter::Block nodeMaterBlk(w, DAG_NODE_MATER);
          for (int x = 0; x < uniqueMatIdx.Count(); ++x)
            w.writeAs<uint16_t>(uniqueMatIdx[x]);
        }
        else
          subMatIdLUT.SetCount(0);
      }
      else
      {
        num = getmatid(mtl, n->GetWireColor());
        if (num >= 0)
          num = matIDtoMatIdx[num];
        if (num >= 0)
        {
          DagWriter::Block nodeMaterBlk(w, DAG_NODE_MATER);
          w.writeAs<uint16_t>(num);
        }
      }
    }
    {
      Matrix3 ntm = get_scaled_stretch_node_tm(n, time);
      adjwtm(ntm);
      DagWriter::Block nodeTmBlk(w, DAG_NODE_TM);
      Matrix3 ptm;
      if (n == useIdentityTransformForNode)
      {
        ntm.IdentityMatrix();
        adjwtm(ntm);

        ptm.IdentityMatrix();
      }
      else if (pnode)
      {
        ptm = get_scaled_stretch_node_tm(pnode, time);
        if (!pnode->IsRootNode())
          adjwtm(ptm);
      }
      else if (nodeOrigin)
      {
        ptm = originTm;
      }
      else
        ptm.IdentityMatrix();
      ptm = ntm * Inverse(ptm);
      w.writeArray(ptm.GetAddr(), 4);
    }
    const ObjectState &os = n->EvalWorldState(time);
    if (os.obj)
    {
      if (os.obj->ClassID() == Dummy_CID)
        ; // no-op
      else if ((util.expflg & EXP_LT) && os.obj->SuperClassID() == LIGHT_CLASS_ID)
        save_light(w, n, os);
      else if ((util.expflg & EXP_SPLINE) && os.obj->SuperClassID() == SHAPE_CLASS_ID)
        save_splines(w, n, os, masterScale);
      else if ((util.expflg & EXP_MESH) && os.obj->SuperClassID() == GEOMOBJECT_CLASS_ID &&
               os.obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
      {
        if (!save_geom(w, n, os, masterScale, originTm, subMatIdLUT))
          return false;
      }
    }

    if (!enod->child.empty())
    {
      DagWriter::Block nodeChildrenBlk(w, DAG_NODE_CHILDREN);
      for (ExpNode *child : enod->child)
        if (!save_node(child, w))
          return false;
    }
    return true;
  }

  void save_light(DagWriter &w, INode *n, const ObjectState &os)
  {
    GenLight *o = dynamic_cast<GenLight *>(os.obj);
    if (!o)
    {
      hasNonDagorLights = true;
      DagorLogWindow::show();
      DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Warning, _T("The light source '%s' is ignored.\r\n"), n->GetName());
      return;
    }

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
    DagWriter::Block nodeObjBlk(w, DAG_NODE_OBJ);
    DagWriter::Block objLightBlk(w, DAG_OBJ_LIGHT);
    w.writeValue(d);
    w.writeValue(d2);
  }

  void save_splines(DagWriter &w, INode *n, const ObjectState &os, float masterScale)
  {
    ShapeObject *shobj = (ShapeObject *)os.obj;
    if (!shobj->CanMakeBezier())
      return;

    Matrix3 otm;
    otm = get_scaled_object_tm(n, time);
    otm = otm * Inverse(get_scaled_stretch_node_tm(n, time));
    BezierShape shp;
    shobj->MakeBezier(time, shp);
    DagWriter::Block nodeObjBlk(w, DAG_NODE_OBJ);
    DagWriter::Block objSplinesBlk(w, DAG_OBJ_SPLINES);
    int ns = shp.SplineCount();
    w.writeAs<uint32_t>(ns);
    for (int si = 0; si < ns; ++si)
    {
      Spline3D *s = shp.GetSpline(si);
      assert(s);
      w.writeAs<uint8_t>(s->Closed() ? DAG_SPLINE_CLOSED : 0);
      int nk = s->KnotCount();
      w.writeAs<uint32_t>(nk);
      for (int ki = 0; ki < nk; ++ki)
      {
        uint8_t kt;
        switch (s->GetKnotType(ki))
        {
          case KTYPE_AUTO: kt = 0; break;
          case KTYPE_BEZIER: kt = DAG_SKNOT_BEZIER; break;
          case KTYPE_CORNER: kt = DAG_SKNOT_CORNER; break;
          case KTYPE_BEZIER_CORNER:
          default: kt = DAG_SKNOT_BEZIER | DAG_SKNOT_CORNER; break;
        }
        w.writeValue(kt);
        for (int k = 0; k < 3; ++k)
          w.writeValue((s->GetVert(ki * 3 + k) * masterScale) * otm);
      }
    }
  }

  static Modifier *last_modifier(IDerivedObject &der) { return der.GetModifier(der.NumModifiers() - 1); }

  bool save_geom(DagWriter &w, INode *n, const ObjectState &os, float masterScale, const Matrix3 &originTm,
    const Tab<int> &subMatIdLUT)
  {
    int numvert = 0;
    enum class ModKind
    {
      BonesWsm,
      Morpher,
      Skin,
      Physique,
    } modKind = ModKind::BonesWsm;
    struct RestoreModOnExit
    {
      Modifier *disabled = nullptr;

      ~RestoreModOnExit()
      {
        if (disabled)
          disabled->EnableMod();
      }
    } restore_mod;
    const auto disable = [&](Modifier *mod, ModKind kind) {
      if (mod->IsEnabled())
      {
        restore_mod.disabled = mod;
        modKind = kind;
      }
      mod->DisableMod();
    };

    if (n->GetProperty(PROPID_HAS_WSM))
    {
      IDerivedObject &der = *(IDerivedObject *)n->GetProperty(PROPID_HAS_WSM);
      if (der.NumModifiers() >= 1)
      {
        Modifier *mod = last_modifier(der);
        if (mod->ClassID() == BONESMOD_CID)
          disable(mod, ModKind::BonesWsm);
      }
    }

    if (!restore_mod.disabled)
    {
      Object *obj = n->GetObjectRef();
      if (obj)
        if (obj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
        {
          IDerivedObject &der = *(IDerivedObject *)obj;
          if (der.NumModifiers() >= 1)
          {
            Modifier *mod = last_modifier(der);
            assert(mod);

            if (mod->ClassID() == Class_ID(0x17bb6854, 0xa5cba2a3))
            {
              explog(_T( "%s: Morpher mod!\r\n"), n->GetName());
              disable(mod, ModKind::Morpher);
            }
            else if (mod->GetInterface(I_SKIN))
            {
              explog(_T( "%s: skin mod!\r\n"), n->GetName());
              disable(mod, ModKind::Skin);
            }
            else if (mod->ClassID() == Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B))
            {
              explog(_T( "%s: Physique mod!\r\n"), n->GetName());
              disable(mod, ModKind::Physique);
            }
          }
        }
    }

    std::optional<DagWriter::Block> nodeObjBlk;
    TriObject *tri = (TriObject *)n->EvalWorldState(time).obj->ConvertToType(time, Class_ID(TRIOBJ_CLASS_ID, 0));
    if (tri)
    {
      nodeObjBlk.emplace(w, DAG_NODE_OBJ);
      Mesh m = tri->mesh;

      if (tri != os.obj)
        tri->DeleteMe();
      Matrix3 otm;
      int notSmoothed = 0;
      mesh_face_sel(m).ClearAll();

      for (int i = 0; i < m.numFaces; ++i)
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
        hasNoSmoothing = true;
        mesh_face_sel(m).ClearAll();
      }
      int v1 = 2, v2 = 1;
      otm = get_scaled_object_tm(n, time);
      if (otm.Parity())
        std::swap(v1, v2);
      otm = otm * Inverse(get_scaled_stretch_node_tm(n, time));
      numvert = m.numVerts;
      for (int i = 0; i < m.numVerts; ++i)
        m.verts[i] = (m.verts[i] * masterScale) * otm;

      if (m.numFaces == 0 && n->Renderable())
      {
        explogWarning(_T( "Renderable object '%s' has 0 faces!\r\n"), n->GetName());
        nofaces = 1;
      }

      int maxntv = 0;
      for (int i = 0; i < MAX_MESHMAPS; ++i)
        if (exported_map_channel(m, i))
          maxntv = std::max(maxntv, m.getNumMapVerts(i));
      Mtl *mtl = n->GetMtl();
      const int subMtlNum = mtl ? mtl->NumSubMtls() : 0;
      int singleMatIdx = -1;
      if (mtl && !subMtlNum)
        if (int id = getmatid(mtl); id >= 0)
          singleMatIdx = (MtlID)matIDtoMatIdx[id];

      if (m.numVerts >= 0x10000 || m.numFaces >= 0x10000 || maxntv >= 0x10000)
      {
        hasBigMeshes = true;
        save_mesh<DagBigFace, DagBigTFace>(w, n, m, otm, v1, v2, subMatIdLUT, singleMatIdx);
      }
      else
        save_mesh<DagFace, DagTFace>(w, n, m, otm, v1, v2, subMatIdLUT, singleMatIdx);

      {
        DagWriter::Block objFaceflgBlk(w, DAG_OBJ_FACEFLG);
        for (int i = 0; i < m.numFaces; ++i)
        {
          Face &f = m.faces[i];
          uint8_t ef = 0;
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
          w.writeValue(ef);
        }
      }

      if (m.numFaces > 0)
      {
        MeshNormalSpec *normalSpec = m.GetSpecifiedNormals();
        if (normalSpec && any_explicit_normals_set(normalSpec) && !(util.expflg & EXP_NO_VNORM) &&
            normalSpec->GetNumFaces() == m.numFaces && normalSpec->GetNumNormals())
        {
          Tab<FaceNGr> fngr;
          normalSpec->CheckNormals();
          int numNormals = normalSpec->GetNumNormals();
          fngr.SetCount(m.numFaces);
          int vid[3] = {0, v1, v2};
          for (int face = 0; face < m.numFaces; face++)
            for (int vertex = 0; vertex < 3; vertex++)
              fngr[face][vid[vertex]] = normalSpec->Face(face).GetNormalID(vertex);

          const Matrix3 normal_tm = AffineTranspose(Inverse(otm));
          Point3 *normals = normalSpec->GetNormalArray();
          for (int i = 0; i < numNormals; ++i)
          {
            const Point3 src = normals[i];
            const Point3 n = VectorTransform(normal_tm, src);
            const float len = Length(n);
            normals[i] = (len > 0 && std::isfinite(len)) ? n / len : src;
          }

          DagWriter::Block objNormalsBlk(w, DAG_OBJ_NORMALS);
          w.writeAs<uint32_t>(numNormals);
          w.writeArray(normals, numNormals);
          w.writeArray(fngr.data(), fngr.Count());
        }
      }
    }

    if (!restore_mod.disabled)
      return true;
    if (modKind == ModKind::Morpher)
    {
      save_morph(w, restore_mod.disabled);
      return true;
    }
    if (modKind == ModKind::Physique)
      return save_physique(w, n, restore_mod.disabled, numvert, originTm);
    if (modKind == ModKind::Skin)
      return save_skin(w, n, restore_mod.disabled, numvert, originTm);
    return save_bones_wsm(w, n, *(IDerivedObject *)n->GetProperty(PROPID_HAS_WSM), numvert, originTm);
  }

  template <class FaceT, class TFaceT>
  void save_mesh(DagWriter &w, INode *n, Mesh &m, Matrix3 &otm, int v1, int v2, const Tab<int> &subMatIdLUT, int singleMatIdx)
  {
    using Index = std::remove_all_extents_t<decltype(FaceT::v)>;
    using TIndex = std::remove_all_extents_t<decltype(TFaceT::t)>;

    DagWriter::Block meshBlk(w, std::is_same_v<FaceT, DagBigFace> ? DAG_OBJ_BIGMESH : DAG_OBJ_MESH);
    w.writeAs<Index>(m.numVerts);
    w.writeArray<Point3>(m.verts, m.numVerts);
    w.writeAs<Index>(m.numFaces);
    for (int i = 0; i < m.numFaces; ++i)
    {
      Face &mf = m.faces[i];
      FaceT f;
      f.v[0] = static_cast<Index>(mf.v[0]);
      f.v[v1] = static_cast<Index>(mf.v[1]);
      f.v[v2] = static_cast<Index>(mf.v[2]);
      f.smgr = mf.smGroup;
      f.mat = resolve_face_mtl(mf.getMatID(), subMatIdLUT, singleMatIdx);
      w.writeValue(f);

      hasDegenerateTriangles |=
        checkDegenerateTriangle(n, otm, mf.v[0], mf.v[1], mf.v[2], m.verts[mf.v[0]], m.verts[mf.v[1]], m.verts[mf.v[2]]);
    }

    uint8_t numch = 0;
    for (int ch = 0; ch < MAX_MESHMAPS; ++ch)
      if (exported_map_channel(m, ch))
        ++numch;
    w.writeValue(numch);
    for (int ch = 0; ch < MAX_MESHMAPS; ++ch)
    {
      if (!exported_map_channel(m, ch))
        continue;
      const TVFace *tf = m.mapFaces(ch);
      const int ntv = m.getNumMapVerts(ch);
      w.writeAs<Index>(ntv);
      w.writeAs<uint8_t>(ch == 0 ? 3 : 2);
      w.writeAs<uint8_t>(ch);
      const Point3 *tv = m.mapVerts(ch);
      for (int i = 0; i < ntv; ++i)
        w.write(&tv[i], ch == 0 ? 12 : 8);
      for (int i = 0; i < m.numFaces; ++i)
      {
        TFaceT f;
        f.t[0] = static_cast<TIndex>(tf[i].t[0]);
        f.t[v1] = static_cast<TIndex>(tf[i].t[1]);
        f.t[v2] = static_cast<TIndex>(tf[i].t[2]);
        w.writeValue(f);
      }
    }
  }

  void save_morph(DagWriter &w, Modifier *mod)
  {
    std::vector<std::pair<int, INode *>> targets;
    for (int i = 0; i < 100; ++i)
      if (RefTargetHandle ref = mod->GetReference(101 + i))
        targets.emplace_back(i, (INode *)ref);

    DagWriter::Block objMorphBlk(w, DAG_OBJ_MORPH);
    w.writeAs<uint8_t>(targets.size());
    for (const auto &[i, target] : targets)
    {
#if defined(MAX_RELEASE_R27) && MAX_RELEASE >= MAX_RELEASE_R27
      TSTR name_wide = mod->SubAnimName(1 + i, false).data();
#else
      TSTR name_wide = mod->SubAnimName(1 + i);
#endif
      w.writeString(wideToStr(name_wide));
      w.writeAs<uint16_t>(getnodeid(target));
    }
  }

  // Writes a DAG_OBJ_BONES block; wt holds numb rows of numvert weights, one row per bone.
  template <class BoneNode, class BoneTm>
  bool save_bones(DagWriter &w, INode *n, int numb, int numvert, const float *wt, const Matrix3 &originTm, BoneNode bone_node,
    BoneTm bone_tm)
  {
    DagWriter::Block objBonesBlk(w, DAG_OBJ_BONES);
    w.writeAs<uint16_t>(numb);
    for (int i = 0; i < numb; ++i)
    {
      INode *bn = bone_node(i);
      DagBone b;
      b.id = getnodeid(bn);
      if (b.id == 0xFFFF)
      {
        explogError(_T( "%s: skin refers to missing bone <%s> b.id=%04X\r\n"), n->GetName(), bn ? bn->GetName() : _T(""), b.id);
        return false;
      }
      Matrix3 tm = bone_tm(i, bn);
      adjwtm(tm);
      tm = tm * Inverse(originTm);
      memcpy(&b.tm, tm.GetAddr(), sizeof(b.tm));
      w.writeValue(b);
    }
    w.writeAs<uint32_t>(numvert);
    w.writeArray(wt, size_t(numvert) * numb);
    return true;
  }

  Matrix3 stretched_bone_tm(INode *bn, const Matrix3 &init_tm) const
  {
    Matrix3 tm = bn->GetStretchTM(time) * init_tm;
    scale_matrix(tm);
    return tm;
  }

  bool save_physique(DagWriter &w, INode *n, Modifier *mod, int numvert, const Matrix3 &originTm)
  {
    IPhysiqueExport *phyExport = (IPhysiqueExport *)mod->GetInterface(I_PHYINTERFACE);
    if (!phyExport)
    {
      explogWarning(L"%s: no Physique export interface\r\n", n->GetName());
      return true;
    }
    OnExit releasePhyExport([&] { mod->ReleaseInterface(I_PHYINTERFACE, phyExport); });

    IPhyContextExport *contextExport = (IPhyContextExport *)phyExport->GetContextInterface(n);
    if (!contextExport)
    {
      explogWarning(L"%s: no Physique context interface\r\n", n->GetName());
      return true;
    }
    OnExit releaseContextExport([&] { phyExport->ReleaseContextInterface(contextExport); });

    contextExport->ConvertToRigid();
    contextExport->AllowBlending();

    int numv = contextExport->GetNumberVertices();

    // get bone nodes and weights
    std::vector<INode *> bones;
    std::vector<float> wt;
    auto boneIndex = [&](INode *bnode) {
      auto it = std::find(bones.begin(), bones.end(), bnode);
      if (it != bones.end())
        return int(it - bones.begin());
      wt.resize(wt.size() + numv);
      bones.push_back(bnode);
      return int(bones.size()) - 1;
    };

    for (int i = 0; i < numv; ++i)
    {
      IPhyVertexExport *vtx = (IPhyVertexExport *)contextExport->GetVertexInterface(i);
      if (!vtx)
        continue;

      if (vtx->GetVertexType() == RIGID_TYPE)
      {
        IPhyRigidVertex *v = (IPhyRigidVertex *)vtx;
        wt[boneIndex(v->GetNode()) * numv + i] = 1;
      }
      else if (vtx->GetVertexType() == RIGID_BLENDED_TYPE)
      {
        IPhyBlendedRigidVertex *v = (IPhyBlendedRigidVertex *)vtx;
        const int num = v->GetNumberNodes();
        for (int wi = 0; wi < num; ++wi)
          wt[boneIndex(v->GetNode(wi)) * numv + i] = v->GetWeight(wi);
      }

      contextExport->ReleaseVertexInterface(vtx);
    }

    if (bones.empty() || numv != numvert)
    {
      explogWarning(_T( "%s: invalid bones data\r\n"), n->GetName());
      return true;
    }

    unsigned short numb = static_cast<unsigned short>(bones.size());
    if (!save_bones(
          w, n, numb, numvert, wt.data(), originTm, [&](int i) { return bones[i]; },
          [&](int, INode *bn) {
            Matrix3 tm;
            if (phyExport->GetInitNodeTM(bn, tm) != MATRIX_RETURNED)
            {
              explogWarning(_T( "%s: no Physique init tm for %s\r\n"), n->GetName(), bn->GetName());
              tm.IdentityMatrix();
            }
            return stretched_bone_tm(bn, tm);
          }))
      return false;

    explog(_T( "%s: exported Physique skinning\r\n"), n->GetName());
    return true;
  }

  bool save_bones_wsm(DagWriter &w, INode *n, IDerivedObject &der, int numvert, const Matrix3 &originTm)
  {
    Modifier *mod = last_modifier(der);
    WSMObject *wsm = (WSMObject *)mod->GetInterface(I_BONES_WARP);
    if (!wsm)
      return true;
    OnExit releaseWsm([&] { mod->ReleaseInterface(I_BONES_WARP, wsm); });

    Tab<Bone> *bon = (Tab<Bone> *)wsm->GetInterface(I_BONES);
    if (!bon)
      return true;
    OnExit releaseBon([&] { wsm->ReleaseInterface(I_BONES, bon); });

    ModContext *mc = der.GetModContext(0);
    if (!mc || !mc->localData)
      return true;

    BonesModData &md = *(BonesModData *)mc->localData;
    int numbones = 0;
    for (int i = 0; i < bon->Count(); ++i)
      if ((*bon)[i].node)
        ++numbones;
    if (md.VN != numvert || numbones != md.BN)
    {
      explogWarning(_T( "%s: invalid bones data\r\n"), n->GetName());
      return true;
    }

    return save_bones(
      w, n, md.BN, md.VN, md.pt.data(), originTm, [&](int i) { return (*bon)[md.bone[i].node].node; },
      [&](int i, INode *) { return md.bone[i].nodetm; });
  }

  bool save_skin(DagWriter &w, INode *n, Modifier *mod, int numvert, const Matrix3 &originTm)
  {
    ISkin *skin = (ISkin *)mod->GetInterface(I_SKIN);
    if (!skin)
      return true;

    ISkinContextData *ctx = skin->GetContextInterface(n);
    int numb = skin->GetNumBones();
    if (numb <= 0 || !ctx || ctx->GetNumPoints() != numvert)
    {
      explogWarning(_T( "%s: invalid skin bones data\r\n"), n->GetName());
      return true;
    }

    std::vector<float> wt(numvert * numb);
    for (int i = 0; i < numvert; ++i)
    {
      int nb = ctx->GetNumAssignedBones(i);
      float sum = 0;
      for (int j = 0; j < nb; ++j)
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

    return save_bones(
      w, n, numb, numvert, wt.data(), originTm, [&](int i) { return skin->GetBone(i); },
      [&](int, INode *bn) {
        Matrix3 tm;
        if (skin->GetBoneInitTM(bn, tm, FALSE) != SKIN_OK)
        {
          explogWarning(_T( "%s: no skin init tm for %s\r\n"), n->GetName(), bn->GetName());
          tm.IdentityMatrix();
        }
        return stretched_bone_tm(bn, tm);
      });
  }

  void save_textures(DagWriter &w)
  {
    if (tex.empty())
      return;
    DagWriter::Block texturesBlk(w, DAG_TEXTURES);

    int num = static_cast<int>(tex.size());
    if (num > 0xFFFF)
      num = 0xFFFF;

    w.writeAs<uint16_t>(num);
    for (int i = 0; i < num; ++i)
    {
      std::string name = wideToStr(tex[i]);

      std::replace(name.begin(), name.end(), '\\', '/');
      if (name.find('/') == std::string::npos)
        name = "./" + name;

      // if the name is too long
      if (name.size() > 255) // counter must fit in 1 byte
      {
        // sacrifice the directory part
        size_t pos = name.find_last_of('/');
        if (pos != std::string::npos)
          name = name.substr(pos + 1);
      }

      w.writeString(name);
    }
  }

  void save_materials(DagWriter &w)
  {
    for (int i = 0; i < (int)mat.size(); ++i)
    {
      if (matIDtoMatIdx[i] < 0)
        continue;
      DagWriter::Block materBlk(w, DAG_MATER);
      w.writeString(wideToStr(mat[i].name));
      w.writeValue(mat[i].m);
      w.writeString(wideToStr(mat[i].classname));

      // don't export parameters of proxymats
      if (!mat[i].script.empty() && !isProxymatName(mat[i].classname))
      {
        std::string s = wideToStr(fix_empty_param_values(mat[i].script, mat[i].classname));
        w.write(s.data(), s.size());
      }
    }
  }

  bool save_nodes(DagWriter &w)
  {
    // children point into en, so it must not reallocate
    std::vector<ExpNode> en;
    en.reserve(node.Count());
    for (int i = 0; i < node.Count(); ++i)
      en.emplace_back(i);
    ExpNode root(-1);
    for (int i = 0; i < node.Count(); ++i)
    {
      uint pid = getnodeid(node[i]->GetParentNode());
      if (pid < node.Count())
        en[pid].add_child(&en[i]);
      else
        root.add_child(&en[i]);
    }
    DagWriter::Block nodeBlk(w, DAG_NODE);
    {
      DagWriter::Block nodeDataBlk(w, DAG_NODE_DATA);
      DagNodeData d;
      d.id = 0xFFFF;
      d.cnum = static_cast<unsigned short>(root.child.size());
      w.writeValue(d);
    }
    if (!root.child.empty())
    {
      DagWriter::Block nodeChildrenBlk(w, DAG_NODE_CHILDREN);
      for (ExpNode *child : root.child)
        if (!save_node(child, w))
          return false;
    }
    return true;
  }

  bool save(FILE *h, Interface *ip)
  {
    prepare_expmat();

    matIDtoMatIdx.SetCount((int)mat.size());
    for (int i = 0; i < matIDtoMatIdx.Count(); ++i)
      matIDtoMatIdx[i] = (util.expflg & EXP_MESH) ? -1 : i;

    if (util.expflg & EXP_MESH)
    {
      Tab<bool> matUsed;
      matUsed.SetCount((int)mat.size());
      for (size_t i = 0; i < mat.size(); ++i)
        matUsed[i] = false;
      for (int i = 0; i < node.Count(); ++i)
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
                tri->DeleteMe();

              // mirrors the subMatIdLUT default in save_node: a slot it cannot export points at the first one it can
              Tab<int> slotMat;
              slotMat.SetCount(mat_num);
              int firstMat = -1;
              for (int j = 0; j < mat_num; ++j)
              {
                slotMat[j] = getusedmatid(mtl->GetSubMtl(j));
                if (firstMat < 0)
                  firstMat = slotMat[j];
              }
              for (int j = 0; j < mat_num; ++j)
                if (slotMat[j] < 0)
                  slotMat[j] = firstMat;

              if (firstMat >= 0)
                for (int j = 0; j < m.numFaces; ++j)
                  matUsed[slotMat[face_mtl_slot(m.faces[j].getMatID(), mat_num)]] = true;
            }
          }
        }
        else
        {
          int mat = getusedmatid(mtl, node[i]->GetWireColor());
          if (mat >= 0)
            matUsed[mat] = true;
        }
      }

      if (useMOpt())
        optimize_materials(matUsed);

      std::wstring unused_mtls, unused_tex;

      Tab<int> tex_remap;
      std::vector<std::wstring> new_tex;
      tex_remap.SetCount(static_cast<int>(tex.size()));
      for (int i = 0; i < tex_remap.Count(); ++i)
        tex_remap[i] = -1;

      int idx = 0;
      for (size_t i = 0; i < mat.size(); ++i)
        if (matUsed[i])
        {
          // remap textures
          for (int j = 0; j < DAGTEXNUM; j++)
            if (mat[i].m.texid[j] != DAGBADMATID)
            {
              unsigned short &texid = mat[i].m.texid[j];
              if (texid >= static_cast<unsigned short>(tex_remap.Count()))
              {
                texid = DAGBADMATID;
                continue;
              }
              int new_texid = tex_remap[texid];
              if (new_texid < 0)
              {
                new_tex.push_back(tex[texid]);
                tex_remap[texid] = new_texid = static_cast<int>(new_tex.size()) - 1;
              }
              texid = new_texid;
            }
          matIDtoMatIdx[i] = idx;
          ++idx;
        }
        else
        {
          unused_mtls += L"\r\n        '";
          unused_mtls += mat[i].name;
          unused_mtls += L"'";

          // reset unused material props
          mat[i].classname.clear();
          mat[i].script.clear();
          memset(&mat[i].m, 0, sizeof(mat[i].m));
          for (int j = 0; j < DAGTEXNUM; j++)
            mat[i].m.texid[j] = DAGBADMATID;
        }

      for (int i = 0; i < static_cast<int>(tex.size()); ++i)
        if (tex_remap[i] < 0)
        {
          unused_tex += L"\r\n        '";
          unused_tex += tex[i];
          unused_tex += L"'";
        }
      tex = std::move(new_tex);
      texIndexMap.clear();
      for (int j = 0; j < static_cast<int>(tex.size()); ++j)
        texIndexMap.emplace(tex[j], j);

      if (!unused_mtls.empty())
        explog(_T("these materials are UNUSED:%s\r\n"), unused_mtls.data());

      if (!unused_tex.empty())
        explog(_T("these textures are UNUSED:%s\r\n"), unused_tex.data());
    }

    DagWriter w;
    w.writeAs<uint32_t>(DAG_ID);
    {
      DagWriter::Block idBlk(w, DAG_ID);
      save_textures(w);
      save_materials(w);
      if (!save_nodes(w))
        return false;
      DagWriter::Block endBlk(w, DAG_END);
    }
    return w.writeTo(h);
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
    std::vector<char> h(num * 2 - 1);
    char *x = h.data() + num - 1;

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
  }

  bool save_anim2(FILE *fp)
  {
    // pools data
    std::vector<char> namesPool;
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
    std::vector<std::string> additionalNames;
    int linVelId = -1, angVelId = -1;
    if (originLinVel.Count())
    {
      linVelId = (int)additionalNames.size();
      additionalNames.push_back(origin_lin_vel_node_name);
    }
    if (originAngVel.Count())
    {
      angVelId = (int)additionalNames.size();
      additionalNames.push_back(origin_ang_vel_node_name);
    }

    unsigned namePoolSize = 0;
    std::vector<int> nameIdx;
    getNamePool(exportNodes, noteTrackKeys, namesPool, nameIdx, namePoolSize, additionalNames, additionalIds);

    getAnimations(exportNodes, pos, rot, scl);

    unsigned keyPoolSize = 0;
    unsigned timeNum = 0;
    unsigned keyNum;
    float nodeWeight = 1.0f;

    // Point3
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
        keyPoolSize += sizeof(AnimKeyPoint3) * keyNum;
        timeNum += keyNum;
      }
      if ((keyNum = scl[i]->key.Count()) > 0)
      {
        sclCannelParams.keyNums.Append(1, &keyNum);
        sclCannelParams.nodeNameIds.Append(1, &nameIdx[i]);
        sclCannelParams.nodeWeights.Append(1, &nodeWeight);
        keyPoolSize += sizeof(AnimKeyPoint3) * keyNum;
        timeNum += keyNum;
      }
    }
    if (posCannelParams.keyNums.Count())
      ++point3DataTypeHeader.channelNum;
    if (sclCannelParams.keyNums.Count())
      ++point3DataTypeHeader.channelNum;

    // Quat
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
    hdr.dataTypeNum = 0; // Point3, Quat
    if (point3DataTypeHeader.channelNum)
      ++hdr.dataTypeNum;
    if (quatDataTypeHeader.channelNum)
      ++hdr.dataTypeNum;

    //
    // write file
    //
    DagWriter w;
    w.writeValue(hdr);

    // write names pool
    w.write(namesPool.data(), hdr.namePoolSize);

    // write times pool
    w.writeArray(originLinVelT.data(), originLinVelT.Count());
    w.writeArray(originAngVelT.data(), originAngVelT.Count());

    for (int i = 0; i < exportNodes.Count(); ++i)
      for (int j = 0; j < pos[i]->key.Count(); ++j)
        w.writeValue(pos[i]->key[j].t);

    for (int i = 0; i < exportNodes.Count(); ++i)
      for (int j = 0; j < scl[i]->key.Count(); ++j)
        w.writeValue(scl[i]->key[j].t);

    for (int i = 0; i < exportNodes.Count(); ++i)
      for (int j = 0; j < rot[i]->key.Count(); ++j)
        w.writeValue(rot[i]->key[j].t);

    // write keys pool
    //  write pos keys
    std::vector<AnimKeyPoint3> point3AnimKey;
    //   write origin velocitiy keys
    w.writeArray(originLinVel.data(), originLinVel.Count());
    w.writeArray(originAngVel.data(), originAngVel.Count());

    //   write per-node pos keys
    for (int i = 0; i < exportNodes.Count(); ++i)
    {
      if (!pos[i]->key.Count())
        continue;
      point3AnimKey.resize(pos[i]->key.Count());
      memset(point3AnimKey.data(), 0, sizeof(AnimKeyPoint3) * point3AnimKey.size());
      make_postrack(point3AnimKey.data(), pos[i]->key.data(), pos[i]->key.Count());
      w.writeArray(point3AnimKey.data(), point3AnimKey.size());
    }

    //  write scl keys
    for (int i = 0; i < exportNodes.Count(); ++i)
    {
      if (!scl[i]->key.Count())
        continue;
      point3AnimKey.resize(scl[i]->key.Count());
      memset(point3AnimKey.data(), 0, sizeof(AnimKeyPoint3) * point3AnimKey.size());
      make_postrack(point3AnimKey.data(), scl[i]->key.data(), scl[i]->key.Count());
      w.writeArray(point3AnimKey.data(), point3AnimKey.size());
    }

    //  write rot keys
    std::vector<AnimKeyQuat> quatAnimKey;
    for (int i = 0; i < exportNodes.Count(); ++i)
    {
      if (!rot[i]->key.Count())
        continue;
      quatAnimKey.resize(rot[i]->key.Count());
      memset(quatAnimKey.data(), 0, sizeof(AnimKeyQuat) * quatAnimKey.size());
      make_rottrack(quatAnimKey.data(), rot[i]->key.data(), rot[i]->key.Count());
      w.writeArray(quatAnimKey.data(), quatAnimKey.size());
    }

    // write note track pool
    if (noteTrackKeys.Count())
    {
      for (int i = 0; i < noteTrackKeys.Count(); i++)
      {
        w.writeValue(noteTrackKeys[i].idx);
        w.writeValue(noteTrackKeys[i].time);
      }
    }
    else
      explogWarning(_T("Warning: NoteTrack not used\n" ));

    // write data of animation
    //  write point3 data type
    w.writeValue(point3DataTypeHeader);
    writeChannel(originLinVelCannelParams, w);
    writeChannel(originAngVelCannelParams, w);
    writeChannel(posCannelParams, w);
    writeChannel(sclCannelParams, w);

    //  write quat data type
    w.writeValue(quatDataTypeHeader);
    writeChannel(rotCannelParams, w);

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

    for (int i = pos.Count(); --i >= 0;)
      delete pos[i];
    for (int i = scl.Count(); --i >= 0;)
      delete scl[i];
    for (int i = rot.Count(); --i >= 0;)
      delete rot[i];

    return w.writeTo(fp);
  }

private:
  void getNoteTrackKeys(Tab<INode *> &exportNodes, Tab<AnimKeyLabel> &noteTrackKeys);

  void getNamePool(Tab<INode *> &exportNodes, Tab<AnimKeyLabel> &noteTrackKeys, std::vector<char> &namesPool,
    std::vector<int> &nameIdx, unsigned &namePoolSize, std::vector<std::string> &additionalNames, Tab<int> &additionalIds);

  void getAnimations(Tab<INode *> &exportNodes, Tab<AnimChanPoint3 *> &pos, Tab<AnimChanQuat *> &rot, Tab<AnimChanPoint3 *> &scl);

  void getOriginVelocities(Tab<AnimKeyPoint3> &originLinVel, Tab<AnimKeyPoint3> &originAngVel, Tab<int> &originLinVelT,
    Tab<int> &originAngVelT);

  void writeChannel(const ChannelParams &params, DagWriter &w);

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
  const Interval range(util.astart, util.aend);
  for (int i = 0; i < exportNodes.Count(); ++i)
  {
    if (!exportNodes[i]->HasNoteTracks())
      continue;
    for (int j = 0; j < exportNodes[i]->NumNoteTracks(); ++j)
    {
      NoteTrack *track = exportNodes[i]->GetNoteTrack(j);
      if (!track || track->ClassID() != Class_ID(NOTETRACK_CLASS_ID, 0))
        continue;
      DefNoteTrack *noteTrack = static_cast<DefNoteTrack *>(track);
      for (int k = 0; k < noteTrack->keys.Count(); ++k)
      {
        NoteKey *key = noteTrack->keys[k];
        if ((util.expflg & EXP_ARNG) && !range.InInterval(key->time))
          continue;
        AnimKeyLabel label;
        label.name = _strdup(wideToStr(key->note).c_str());
        label.time = key->time;
        noteTrackKeys.Append(1, &label, 32);
      }
    }
  }
}

void ExportENCB::getNamePool(Tab<INode *> &exportNodes, Tab<AnimKeyLabel> &noteTrackKeys, std::vector<char> &namesPool,
  std::vector<int> &nameIdx, unsigned &namePoolSize, std::vector<std::string> &additionalNames, Tab<int> &additionalIds)
{
  additionalIds.SetCount((int)additionalNames.size());
  // prepare nodes name pool
  namePoolSize = 1;
  int len;
  for (int i = 0; i < exportNodes.Count(); ++i)
    if ((len = (int)wideToStr(exportNodes[i]->GetName()).size()) != 0)
      namePoolSize += len + 1;
  for (int i = 0; i < noteTrackKeys.Count(); ++i)
    if ((len = (int)strlen(noteTrackKeys[i].name)) != 0)
      namePoolSize += len + 1;
  for (size_t i = 0; i < additionalNames.size(); ++i)
    if ((len = (int)additionalNames[i].size()) != 0)
      namePoolSize += len + 1;
  namePoolSize = (namePoolSize + 7) & ~7;

  namesPool.assign(namePoolSize, '\0');
  nameIdx.assign(exportNodes.Count(), 0);

  namePoolSize = 1;
  for (int i = 0; i < exportNodes.Count(); ++i)
  {
    std::string name = wideToStr(exportNodes[i]->GetName());
    if ((len = (int)name.size()) == 0)
      nameIdx[i] = 0;
    else
    {
      nameIdx[i] = namePoolSize;
      char *poolName = namesPool.data() + namePoolSize;
      strcpy(poolName, name.c_str());
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
      char *poolName = namesPool.data() + namePoolSize;
      strcpy(poolName, name);
      namePoolSize += len + 1;
    }
    free(name);
  }
  for (int i = 0; i < (int)additionalNames.size(); ++i)
  {
    const std::string &name = additionalNames[i];
    if ((len = (int)name.size()) == 0)
      additionalIds[i] = 0;
    else
    {
      additionalIds[i] = namePoolSize;
      char *poolName = namesPool.data() + namePoolSize;
      strcpy(poolName, name.c_str());
      namePoolSize += len + 1;
    }
  }

  namePoolSize = (namePoolSize + 7) & ~7;
}

void ExportENCB::getAnimations(Tab<INode *> &exportNodes, Tab<AnimChanPoint3 *> &pos, Tab<AnimChanQuat *> &rot,
  Tab<AnimChanPoint3 *> &scl)
{
  const int exportNodeCount = exportNodes.Count();
  pos.SetCount(exportNodeCount);
  scl.SetCount(exportNodeCount);
  rot.SetCount(exportNodeCount);

  // get animations
  Tab<ExpTMAnimCB *> acb;
  acb.SetCount(exportNodeCount);
  for (int i = 0; i < exportNodeCount; ++i)
  {
    pos[i] = new AnimChanPoint3;
    scl[i] = new AnimChanPoint3;
    rot[i] = new AnimChanQuat;

    const int pid = getnodeid(exportNodes[i]->GetParentNode());
    INode *parentNode = pid >= 0 ? node[pid] : nullptr;

    acb[i] = new MyExpTMAnimCB(exportNodes[i], parentNode, nodeOrigin);
  }

  get_tm_anim_2(pos, rot, scl, (util.expflg & EXP_ARNG) ? Interval(util.astart, util.aend) : FOREVER, exportNodes, acb, util.poseps,
    util.roteps, util.scleps, util.orteps, util.expflg);

  for (int i = acb.Count(); --i >= 0;)
    delete acb[i];
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
    get_node_vel(originLinVel, originAngVel, originLinVelT, originAngVelT,
      (util.expflg & EXP_ARNG) ? Interval(util.astart, util.aend) : FOREVER, *cb, util.poseps2, util.roteps2, util.orteps,
      util.expflg);
    delete cb;
  }
}

void ExportENCB::writeChannel(const ChannelParams &params, DagWriter &w)
{
  unsigned nodeNum = params.keyNums.Count();
  if (nodeNum)
  {
    w.writeValue(params.channelType);
    w.writeValue(nodeNum);
    w.writeArray(params.keyNums.data(), params.keyNums.Count());
    w.writeArray(params.nodeNameIds.data(), params.nodeNameIds.Count());
    w.writeArray(params.nodeWeights.data(), params.nodeWeights.Count());
  }
}


void ExpUtil::errorMessage(const TCHAR *msg)
{
  ip->DisplayTempPrompt(msg, ERRMSG_DELAY);

  if (!are_prompts_suppressed())
    MessageBox(ip->GetMAXHWnd(), msg, _T ("Error"), MB_OK | MB_ICONSTOP);

  explogError(_T("ERROR! %s\r\n"), msg);
}

void ExpUtil::warningMessage(const TCHAR *msg, const TCHAR *title)
{
  if (title == NULL)
    title = _T("Warning");

  if (!are_prompts_suppressed())
    MessageBox(ip->GetMAXHWnd(), msg, title, MB_OK | MB_ICONSTOP);

  explogWarning(_T("WARNING! %s\r\n"), msg);
}

struct LayerFile
{
  std::wstring layerName;
  std::wstring path;
};

static bool find_co_layers(const fs::path &fname, std::vector<LayerFile> &layers)
{
  if (!is_dag_file(fname) || !iequal(fname.stem().extension().c_str(), _T(".lod00")))
    return false;
  const fs::path base = fname.parent_path() / fname.stem().stem();

  ILayerManager *manager = GetCOREInterface13()->GetLayerManager();
  ILayer *l = NULL;

  std::wstring matched_layer_name;
  std::wstring formatted_name;
  for (int i = 0; i < 16; i++)
  {
    formatted_name = std::format(_T("LOD{:02}"), i);
    l = manager->GetLayer(formatted_name.c_str());
    if (!l)
    {
      formatted_name = std::format(_T("lod{:02}"), i);
      l = manager->GetLayer(formatted_name.c_str());
      if (!l)
      {
        formatted_name = std::format(_T("Lod{:02}"), i);
        l = manager->GetLayer(formatted_name.c_str());
      }
    }
    if (l && l->HasObjects())
    {
      layers.push_back({formatted_name, std::format(_T("{}.lod{:02}.dag"), base.c_str(), i)});
    }
  }

  for (int i = 0; i < 16; i++)
  {
    matched_layer_name = std::format(_T("DESTR_LOD{:02}"), i);
    l = manager->GetLayer(matched_layer_name.c_str());
    if (!l)
    {
      matched_layer_name = std::format(_T("destr_lod{:02}"), i);
      l = manager->GetLayer(matched_layer_name.c_str());
      if (!l)
      {
        matched_layer_name = std::format(_T("Destr_lod{:02}"), i);
        l = manager->GetLayer(matched_layer_name.c_str());
      }
    }

    if (l && l->HasObjects())
    {
      layers.push_back({matched_layer_name, std::format(_T("{}_destr.lod{:02}.dag"), base.c_str(), i)});
    }
  }

  matched_layer_name = _T("DM");
  l = manager->GetLayer(matched_layer_name.c_str());
  if (!l)
  {
    matched_layer_name = _T("dm");
    l = manager->GetLayer(matched_layer_name.c_str());
    if (!l)
    {
      matched_layer_name = _T("Dm");
      l = manager->GetLayer(matched_layer_name.c_str());
    }
  }
  if (l && l->HasObjects())
  {
    layers.push_back({matched_layer_name, std::format(_T("{}_dm.dag"), base.c_str())});
  }

  for (int i = 0; i < 16; i++)
  {
    formatted_name = std::format(_T("DMG_LOD{:02}"), i);
    l = manager->GetLayer(formatted_name.c_str());
    if (!l)
    {
      formatted_name = std::format(_T("dmg_lod{:02}"), i);
      l = manager->GetLayer(formatted_name.c_str());
      if (!l)
      {
        formatted_name = std::format(_T("Dmg_lod{:02}"), i);
        l = manager->GetLayer(formatted_name.c_str());
      }
    }
    if (l && l->HasObjects())
    {
      layers.push_back({formatted_name, std::format(_T("{}_dmg.lod{:02}.dag"), base.c_str(), i)});
    }
  }

  for (int i = 0; i < 16; i++)
  {
    formatted_name = std::format(_T("DMG2_LOD{:02}"), i);
    l = manager->GetLayer(formatted_name.c_str());
    if (!l)
    {
      formatted_name = std::format(_T("dmg2_lod{:02}"), i);
      l = manager->GetLayer(formatted_name.c_str());
      if (!l)
      {
        formatted_name = std::format(_T("Dmg2_lod{:02}"), i);
        l = manager->GetLayer(formatted_name.c_str());
      }
    }
    if (l && l->HasObjects())
    {
      layers.push_back({formatted_name, std::format(_T("{}_dmg2.lod{:02}.dag"), base.c_str(), i)});
    }
  }

  for (int i = 0; i < 16; i++)
  {
    formatted_name = std::format(_T("EXPL_LOD{:02}"), i);
    l = manager->GetLayer(formatted_name.c_str());
    if (!l)
    {
      formatted_name = std::format(_T("expl_lod{:02}"), i);
      l = manager->GetLayer(formatted_name.c_str());
      if (!l)
      {
        formatted_name = std::format(_T("Expl_lod{:02}"), i);
        l = manager->GetLayer(formatted_name.c_str());
      }
    }
    if (l && l->HasObjects())
    {
      layers.push_back({formatted_name, std::format(_T("{}_expl.lod{:02}.dag"), base.c_str(), i)});
    }
  }

  matched_layer_name = _T("XRAY");
  l = manager->GetLayer(matched_layer_name.c_str());
  if (!l)
  {
    matched_layer_name = _T("xray");
    l = manager->GetLayer(matched_layer_name.c_str());
  }

  if (l && l->HasObjects())
  {
    layers.push_back({matched_layer_name, std::format(_T("{}_xray.dag"), base.c_str())});
  }

  if (layers.size() > 1)
    return true;

  layers.clear();
  return false;
}

static bool ask_layered_export(const std::vector<LayerFile> &layers)
{
  std::wstring buf = L"Export layers to " + std::to_wstring(layers.size()) + L" separate files?\n";
  for (size_t i = 0; i < layers.size() && buf.size() < (7 << 10); ++i)
  {
    const std::wstring &dst = layers[i].path;
    buf += layers[i].layerName;
    buf += L" -> ";
    buf += dst.size() > 48 ? L"..." + dst.substr(dst.size() - 48) : dst;
    buf += L'\n';
  }

  return MessageBox(GetFocus(), buf.c_str(), L"Export layered DAGs", MB_YESNO | MB_ICONQUESTION) == IDYES;
}

BOOL ExpUtil::export_dag()
{
  DagorLogWindowAutoShower logWindowAutoShower(/*clear_log = */ true);

  std::vector<LayerFile> layers;

  if (!find_co_layers(exp_fname, layers))
    return export_one_dag(exp_fname);

  const bool layered = are_prompts_suppressed() ? separateLayers : ask_layered_export(layers);
  if (!layered)
  {
    if (are_prompts_suppressed())
      explog(L"prompts are off: exporting one file instead of %d layered dags\r\n", int(layers.size()));
    return export_one_dag(exp_fname);
  }

  ILayerManager *manager = GetCOREInterface13()->GetLayerManager();
  const int layerCount = (int)layers.size();
  std::vector<INodeTab> layerNodes(layerCount);
  for (int i = 0; i < layerCount; ++i)
  {
    ILayer *l = manager->GetLayer(layers[i].layerName.c_str());
    if (ILayerProperties *props = (ILayerProperties *)l->GetInterface(LAYERPROPERTIES_INTERFACE))
      props->Nodes(layerNodes[i]);
    else
      explogError(L"Cannot get the layer properties for layer '%s'.\r\n", l->GetName().data());
  }
  BOOL result = TRUE;
  for (int i = 0; i < layerCount; ++i)
  {
    ILayer *l = manager->GetLayer(layers[i].layerName.c_str());
    if (!(expflg & EXP_HID) && l->IsHidden())
    {
      explogWarning(L"layer '%s' is hidden, '%s' is not exported\r\n", l->GetName().data(), layers[i].path.c_str());
      continue;
    }

    explog(L"exporting layer '%s' to '%s'\r\n", l->GetName().data(), layers[i].path.c_str());
    std::unordered_set<INode *> otherLayerNodes;
    for (int j = 0; j < layerCount; ++j)
      if (j != i)
        for (int k = 0; k < layerNodes[j].Count(); ++k)
          otherLayerNodes.insert(layerNodes[j][k]);

    if (!export_one_dag(layers[i].path, std::move(otherLayerNodes)))
      result = FALSE;
  }

  return result;
}

BOOL ExpUtil::export_one_dag(const fs::path &exp_fn, std::unordered_set<INode *> skip_nodes)
{
  ExportENCB cb(ip->GetTime());
  cb.skipNodes = std::move(skip_nodes);
  enum_nodes(ip->GetRootNode(), &cb);

  if (!cb.node.Count())
  {
    errorMessage(_T("There are no nodes."));
    return false;
  }

  return export_one_dag_cb(cb, exp_fn);
}

bool ExpUtil::write_via_temp_file(const fs::path &target, const std::function<bool(FILE *)> &write)
{
  std::error_code ec;
  wchar_t dir[MAX_PATH] = {}, path[MAX_PATH] = {};
  if (!GetTempPathW(MAX_PATH, dir) || !GetTempFileNameW(dir, L"exp", 0, path))
  {
    errorMessage(GetString(IDS_FILE_CREATE_ERR));
    return false;
  }

  FILE *h = _wfopen(path, L"wb");
  if (!h)
  {
    errorMessage(GetString(IDS_FILE_CREATE_ERR));
    fs::remove(path, ec);
    return false;
  }
  bool written = false;
  try
  {
    written = write(h) && !ferror(h);
  }
  catch (const std::bad_alloc &)
  {
    explogError(L"Out of memory while writing '%s'\r\n", target.c_str());
  }
  const bool closed = fclose(h) == 0;
  if (!written || !closed)
  {
    errorMessage(GetString(IDS_FILE_WRITE_ERR));
    fs::remove(path, ec);
    return false;
  }

  const bool copied = CopyFileW(path, target.c_str(), FALSE);
  fs::remove(path, ec);
  if (!copied)
  {
    errorMessage(GetString(IDS_FILE_CREATE_ERR));
    return false;
  }
  return true;
}

BOOL ExpUtil::export_one_dag_cb(ExportENCB &cb, const fs::path &exp_fn)
{
  checkDupesAndSpaces(cb.node);

  if (!write_via_temp_file(exp_fn, [&](FILE *h) { return cb.save(h, ip); }))
    return false;
  explog(_T ("%d nodes\r\n"), cb.node.Count());
  explog(_T ("%d materials\r\n"), (int)cb.mat.size());
  explog(_T ("%d textures\r\n"), (int)cb.tex.size());
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
  if (cb.hasNonDagorLights)
  {
    warningMessage(GetString(IDS_NON_DAGOR_LIGHTS));
  }
  if (cb.hasSubSubMaterials)
  {
    warningMessage(GetString(IDS_SUB_SUB_MATERIALS));
  }
  return true;
}
void ExpUtil::export_anim_v2()
{
  DagorLogWindowAutoShower logWindowAutoShower(/*clear_log = */ true);
  explog(_T ("Exporting Anim v2 to file <%s>...\r\n"), exp_anim2_fname.c_str());
  int t0 = timeGetTime();

  ExportENCB cb(ip->GetTime(), /*a2d = */ true);
  enum_nodes(ip->GetRootNode(), &cb);
  t0 = timeGetTime() - t0;
  explog(_T (" enum node: %d ms\r\n"), t0);

  int expNodeCount = 0;
  for (int i = 0; i < cb.node.Count(); ++i)
    if (cb.nodeExp[i])
      ++expNodeCount;
  if (!expNodeCount)
  {
    errorMessage(L"There are no nodes.");
    return;
  }

  checkDupesAndSpaces(cb.node);

  // save_anim2 function is not fast, and even its single write reaches the disk in parts.
  // If we are saving an a2d opened in AssetViewer we are highly likely to crash because AV will
  // trigger on first file flush, try to read not yet fully written file and fail. To avoid this we
  // export to a temp file and then copy it to the final destination. The copy still writes the
  // target as it goes, but it is short, and AV waits up to 500 ms for a file open for writing.
  t0 = timeGetTime();
  if (!write_via_temp_file(exp_anim2_fname, [&](FILE *h) { return cb.save_anim2(h); }))
    return;

  t0 = timeGetTime() - t0;
  explog(_T (" save anim2: %d ms\r\n"), t0);

  explog(L" %d nodes\r\n", expNodeCount);
  explog(_T ("Success!\r\n"));
}
static bool FolderOpenDialog(fs::path &path, HWND)
{
  path.clear();

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

  IShellItem *psiResult = 0;
  hr = pfd->GetResult(&psiResult);
  if (!SUCCEEDED(hr))
  {
    pfd->Release();
    return false;
  }

  PWSTR pszFilePath = NULL;
  psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
  if (pszFilePath)
  {
    path = pszFilePath;
    CoTaskMemFree(pszFilePath);
  }
  psiResult->Release();
  pfd->Release();
  return !path.empty();
}

static bool is_windows_device_name(std::wstring_view stem)
{
  while (!stem.empty() && stem.back() == L' ')
    stem.remove_suffix(1);
  for (const wchar_t *device : {L"CON", L"PRN", L"AUX", L"NUL", L"CONIN$", L"CONOUT$", L"CLOCK$"})
    if (iequal(stem, device))
      return true;
  if (stem.size() != 4 || !(istarts_with(stem, L"COM") || istarts_with(stem, L"LPT")))
    return false;
  constexpr std::wstring_view superscriptDigits = L"\xB9\xB2\xB3";
  const wchar_t number = stem[3];
  return (number >= L'1' && number <= L'9') || superscriptDigits.find(number) != std::wstring_view::npos;
}

class DagFileNames
{
public:
  explicit DagFileNames(fs::path folder_) : folder(std::move(folder_)) {}

  fs::path take(const wchar_t *what, const wchar_t *name)
  {
    std::wstring file = name;
    for (wchar_t &c : file)
      if (c < L' ' || std::wstring_view(L"<>:\"/\\|?*").find(c) != std::wstring_view::npos)
        c = L'_';
    const bool renamed = file != name;
    const size_t stemEnd = std::min(file.find(L'.'), file.size());
    const bool device = is_windows_device_name(std::wstring_view(file).substr(0, stemEnd));
    if (device)
      file.insert(stemEnd, L"_");
    file += L".dag";

    std::wstring key(file.size(), L'\0');
    LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_UPPERCASE, file.c_str(), int(file.size()), key.data(), int(key.size()), nullptr,
      nullptr, 0);
    const auto [taken, isNew] = written.try_emplace(std::move(key), name);
    if (!isNew)
    {
      explogError(L"%s '%s' is not exported: the file name '%s' is already taken by '%s' in this export\r\n", what, name, file.c_str(),
        taken->second.c_str());
      return {};
    }
    if (renamed)
      explogWarning(L"%s '%s' has characters a file name cannot hold, its file is named '%s'\r\n", what, name, file.c_str());
    else if (device)
      explogWarning(L"%s '%s' is a Windows device name, its file is named '%s'\r\n", what, name, file.c_str());
    return folder / file;
  }

private:
  fs::path folder;
  std::unordered_map<std::wstring, std::wstring> written;
};

void ExpUtil::exportObjectsAsDagsInternal(DagFileNames &files, INode &node)
{
  const int childrenCount = node.NumberOfChildren();
  for (int i = 0; i < childrenCount; ++i)
  {
    INode *childNode = node.GetChildNode(i);

    if (((util.expflg & EXP_SEL) != 0 && childNode->Selected() == 0) || ((util.expflg & EXP_HID) == 0 && childNode->IsHidden()))
    {
      exportObjectsAsDagsInternal(files, *childNode);
      continue;
    }

    ExportENCB cb(ip->GetTime());

    if (childNode->IsGroupHead())
      enum_nodes_by_node(childNode, &cb);
    else
      cb.proc(childNode);

    if (cb.node.Count() > 0)
    {
      // Use the node as origin instead of the scene.
      cb.useIdentityTransformForNode = childNode;

      const fs::path path = files.take(L"Node", childNode->GetName());
      if (!path.empty())
      {
        explog(L"Exporting node \"%s\" to file \"%s\"\r\n", childNode->GetName(), path.c_str());
        export_one_dag_cb(cb, path);
        explog(L"\r\n");
      }
    }
    else
    {
      explogWarning(_T("Skipping node '%s'. It does no match the export settings (beside \"sel\" and \"hid\").\r\n"),
        childNode->GetName());
    }

    // If this is a group then there is no need to descend further because groups are exported as a single dag.
    if (!childNode->IsGroupHead())
      exportObjectsAsDagsInternal(files, *childNode);
  }
}

void ExpUtil::exportObjectsAsDags()
{
  DagorLogWindowAutoShower logWindowAutoShower(/*clear_log = */ true);

  fs::path folder;
  if (!FolderOpenDialog(folder, hExpDag))
    return;

  INode *rootNode = GetCOREInterface13()->GetRootNode();
  if (!rootNode)
  {
    explogError(_T("Could not get the root node.\r\n"));
    explogError(_T("Aborting export.\r\n"));
    return;
  }

  DagFileNames files(folder);
  exportObjectsAsDagsInternal(files, *rootNode);
}

void ExpUtil::exportLayerAsDag(DagFileNames &files, ILayer &layer)
{
  // "sel" only affects the layers we work with, so do not use it when processing the nodes.
  const int oldExpSel = util.expflg & EXP_SEL;
  util.expflg &= ~EXP_SEL;

  ILayerProperties *layerProperties = (ILayerProperties *)layer.GetInterface(LAYERPROPERTIES_INTERFACE);
  if (layerProperties)
  {
    ExportENCB cb(ip->GetTime());

    // ILayerProperties::Nodes returns with all nodes belonging to a layer, regardless of the hierarchy,
    // so there is no need to call enum_nodes.
    Tab<INode *> nodes;
    layerProperties->Nodes(nodes);
    for (int i = 0; i < nodes.Count(); ++i)
      cb.proc(nodes[i]);

    if (cb.node.Count() > 0)
    {
      const fs::path path = files.take(L"Layer", layer.GetName().data());
      if (!path.empty())
      {
        explog(L"Exporting layer \"%s\" to file \"%s\"\r\n", layer.GetName().data(), path.c_str());
        export_one_dag_cb(cb, path);
        explog(L"\r\n");
      }
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

void ExpUtil::exportLayersAsDagsInternal(DagFileNames &files, ILayer &layer)
{
  const bool matchesVisibilityFilter = (util.expflg & EXP_HID) != 0 || !layer.IsHidden(true);

  const int childLayerCount = layer.GetNumOfChildLayers();

  if (childLayerCount > 0)
  {
    explog(L"Layer '%s' has child layer(s), it will not be exported.\r\n", layer.GetName().data());

    for (int i = 0; i < childLayerCount; ++i)
      exportLayersAsDagsInternal(files, *layer.GetChildLayer(i));
  }
  else if (matchesVisibilityFilter)
    exportLayerAsDag(files, layer);
  else
    explog(L"Layer '%s' is hidden, it will not be exported.\r\n", layer.GetName().data());
}

void ExpUtil::exportLayersAsDags()
{
  DagorLogWindowAutoShower logWindowAutoShower(/*clear_log = */ true);

  fs::path folder;
  if (!FolderOpenDialog(folder, hExpDag))
    return;

  DagFileNames files(folder);
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
          exportLayersAsDagsInternal(files, *layer->GetChildLayer(childLayerIndex));
      }
      else
        exportLayersAsDagsInternal(files, *layer);
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

    exportLayersAsDagsInternal(files, *currentLayer);
  }
}


void ExpUtil::exportPhysics()
{
  DagorLogWindowAutoShower logWindowAutoShower(/*clear_log = */ true);
  explog(_T ("Exporting physics file <%s>...\r\n"), exp_phys_fname.c_str());

  if (!(util.expflg & EXP_DONT_CALC_MOMJ))
  {
    explog(_T("Updating masses and momjs...\r\n"));
    ::calc_momjs(ip);
  }

  FILE *h = _tfopen(exp_phys_fname.c_str(), _T ("wb"));
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
  bool hasEmptyNames = false;

  std::unordered_map<std::wstring_view, bool, CaseInsensitiveHashW, CaseInsensitiveEqualW> seenNames;
  seenNames.reserve(node_list.Count());

  for (int i = 0; i < node_list.Count(); ++i)
  {
    if (!(expflg & EXP_HID) && node_list[i]->IsNodeHidden())
      continue;

    if ((expflg & EXP_SEL) && !node_list[i]->Selected())
      continue;

    const wchar_t *name = node_list[i]->GetName();

    if (name[0] == 0)
    {
      explogWarning(L"node with empty name\r\n");
      hasEmptyNames = true;
    }

    auto [it, inserted] = seenNames.emplace(name, false);
    if (!inserted)
    {
      const wchar_t *duplicateNameFmt = L"duplicate node name \"%s\"\r\n";
      if (!it->second)
      {
        explogWarning(duplicateNameFmt, it->first.data());
        it->second = true;
      }
      explogWarning(duplicateNameFmt, name);
      hasDupes = true;
    }
  }

  if (hasDupes)
  {
    ip->DisplayTempPrompt(L"There are duplicate node names.", ERRMSG_DELAY);
    warningMessage(L"There are nodes with the same names.\n See log for details.", L"Duplicate names");
  }

  if (hasEmptyNames)
  {
    ip->DisplayTempPrompt(L"There is a node with empty name.", ERRMSG_DELAY);
    warningMessage(L"There is a node with empty name.\n See log for details.", L"Empty name");
  }
}


void ExpUtil::export_instances()
{
  DagorLogWindowAutoShower logWindowAutoShower(/*clear_log = */ true);
  explog(_T("Exporting instances placement file <%s>...\r\n"), exp_instances_fname.c_str());

  ExportENCB cb(ip->GetTime());
  enum_nodes(ip->GetRootNode(), &cb);

  std::ofstream os(exp_instances_fname, std::ios::binary);
  if (!os)
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

    if (!istarts_with(cb.node[nodeNo]->GetName(), _T("GroundPlane")))
      continue;

    float newPlaneOffsetX;
    if (!cb.node[nodeNo]->GetUserPropFloat(_T("PlaneOffsetX:r"), newPlaneOffsetX))
    {
      errorMessage(std::format(_T("'{}': invalid PlaneOffsetX:r"), cb.node[nodeNo]->GetName()).c_str());
      return;
    }

    float newPlaneOffsetZ;
    if (!cb.node[nodeNo]->GetUserPropFloat(_T("PlaneOffsetZ:r"), newPlaneOffsetZ))
    {
      errorMessage(std::format(_T("'{}': invalid PlaneOffsetZ:r"), cb.node[nodeNo]->GetName()).c_str());
      return;
    }

    if (newPlaneOffsetX <= planeOffsetX || newPlaneOffsetZ <= planeOffsetZ)
    {
      planeOffsetX = newPlaneOffsetX;
      planeOffsetZ = newPlaneOffsetZ;

      if (!cb.node[nodeNo]->GetUserPropFloat(_T("PlaneSize:r"), planeSize))
      {
        errorMessage(std::format(_T("'{}': invalid PlaneSize:r"), cb.node[nodeNo]->GetName()).c_str());
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

  os << std::format("Version:i=20070810\r\nPlaneOffsetX:r={:f}\r\nPlaneOffsetZ:r={:f}\r\nPlaneSize:r={:f}\r\n"
                    "numTexturesWidth:i={}\r\nnumTexturesHeight:i={}\r\n\r\n",
    planeOffsetX, planeOffsetZ, planeSize, numTexturesWidth, numTexturesHeight);


  // Export nodes.

  unsigned int numExportedNodes = 0;
  for (unsigned int nodeNo = 0; nodeNo < cb.node.Count(); nodeNo++)
  {
    if (!cb.node[nodeNo]->GetParentNode() || !cb.node[nodeNo]->GetParentNode()->IsRootNode())
      continue;

    std::string name = wideToStr(cb.node[nodeNo]->GetName());
    if (istarts_with(name, "GroundPlane"))
      continue;

    while (name.length() > 0 && isdigit((unsigned char)name[name.length() - 1]))
      name.erase(name.length() - 1, 1);

    if (name.length() > 0 && name[name.length() - 1] == '_')
      name.erase(name.length() - 1, 1);

    float masterScale = static_cast<float>(GetSystemUnitScale(UNITS_METERS));
    Matrix3 tm = cb.node[nodeNo]->GetNodeTM(0);
    tm.SetTrans(tm.GetTrans() * masterScale + Point3(offsetX, offsetZ, 0.f));
    const Matrix3 m = tm;

    os << std::format("object{{\r\n"
                      "  name:t=\"{}\"\r\n"
                      "  numericName:t=\"{} {}\"\r\n"
                      "  matrix:m=[[{:f}, {:f}, {:f}] [{:f}, {:f}, {:f}] [{:f}, {:f}, {:f}] [{:f}, {:f}, {:f}]]\r\n"
                      "}}\r\n",
      name, name, planeNo * 100000 + numExportedNodes, m[0][0], m[0][2], m[0][1], m[2][0], m[2][2], m[2][1], m[1][0], m[1][2], m[1][1],
      m[3][0], m[3][2], m[3][1]);

    numExportedNodes++;
  }

  os.close();
  if (!os)
  {
    errorMessage(GetString(IDS_FILE_WRITE_ERR));
    return;
  }

  explog(_T("Success!\r\n"));
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
    Point3 transformedPoint = (*degenerateAt) / static_cast<float>(GetSystemUnitScale(UNITS_METERS));
    transformedPoint = Inverse(applied_transform) * node->GetObjTMAfterWSM(0) * transformedPoint;

    explogWarning(_T( "'%s' has degenerate triangle at (%g, %g, %g) (in units)\r\n"), node->GetName(), transformedPoint.x,
      transformedPoint.y, transformedPoint.z);
  }

  return degenerateAt != NULL;
}

void update_export_mode(bool use_legacy_import)
{
  util.exportMode = use_legacy_import ? ExpUtil::ExportMode::Standard : ExpUtil::ExportMode::LayersAsDags;
  util.update_ui_dag(util.hExpDag);
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
  BEGIN_FUNCTION_MAP FN_5(fun_export, TYPE_BOOL, export_dag, TYPE_STRING, TYPE_INTERVAL, TYPE_BOOL, TYPE_BOOL,
    TYPE_BOOL) END_FUNCTION_MAP

    // `range` is a stub. It used to set the animation range and nothing reads it any more, but it is a
    // positional parameter, so it is kept to avoid breaking the maxscripts that pass it.
    BOOL export_dag(const TCHAR *fn, Interval /*range*/, bool selectedOnly, bool suppressPrompts, bool separateLayers)
  {
    const int bkExpflg = util.expflg;
    Autotoggle suppressPromptsGuard(util.suppressPrompts, suppressPrompts);
    Autotoggle separateLayersGuard(util.separateLayers, separateLayers);

    if (selectedOnly)
      util.expflg |= EXP_SEL;
    else
      util.expflg &= ~EXP_SEL;

    util.exp_fname = fn;
    util.ip = GetCOREInterface();
    util.update_ui();
    BOOL result = util.export_dag();

    util.expflg = bkExpflg;
    util.update_ui();

    return result;
  }
};

static IDagorExportUtil dagorexputiliface(Interface_ID(0x18da32ce, 0x739f1b15), _T ("dagorExport"), IDS_DAGOR_EXPORT_IFACE, NULL,
  FP_CORE, fun_export, _T ("export"), -1, TYPE_BOOL, 0, 5, _T ("filename"), -1, TYPE_STRING, _T ("range"), -1, TYPE_INTERVAL,
  // f_keyArgDefault marks an optional keyArg param. The value
  // after that is its default value.
  _T ("selectedOnly"), -1, TYPE_BOOL, f_keyArgDefault, false, _T ("suppressPrompts"), -1, TYPE_BOOL, f_keyArgDefault, false,
  // separateLayers answers the layered DAGs question when suppressPrompts is true or Max runs in
  // quiet mode (3dsmaxbatch); otherwise the user is asked
  _T ("separateLayers"), -1, TYPE_BOOL, f_keyArgDefault, false, p_end);
