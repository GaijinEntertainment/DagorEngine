// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <functional>
#include <regex>
#include <unordered_set>
#include <max.h>
#include <CommCtrl.h>
#include <plugapi.h>
#include <utilapi.h>
#include <ilayermanager.h>
#include <ILayerProperties.h>
#include <ilayer.h>
#include <modstack.h>
#include <iparamb2.h>
#include <iskin.h>
#include <bmmlib.h>
#include <stdmat.h>
#include <splshape.h>
#include <notetrck.h>
#include <MeshNormalSpec.h>
#include "dagor.h"
#include "dagfmt.h"
#include "mater.h"
#include "resource.h"
#include "debug.h"
#include "enumnode.h"
#include "layout.h"
#include "common.h"
#include "dagorLogWindow.h"
#include <io.h>

#define ERRMSG_DELAY 5000

#define VALID_REGEX_COLOR_BG   (RGB(255, 255, 255))
#define INVALID_REGEX_COLOR_BG (RGB(255, 192, 192))

extern void update_export_mode(bool use_legacy_import);

struct SkinData
{
  int numb, numvert;
  Tab<DagBone> bones;
  Tab<float> wt;
  INode *skinNode;
  SkinData() : numb(0), numvert(0), skinNode(NULL) {}
};
struct NodeId
{
  int id;
  INode *node;
  NodeId(int _id, INode *n) : id(_id), node(n) {}
};

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

extern void convertnew(Interface *ip, bool on_import);
extern void collapse_materials(Interface *ip);

TCHAR *ReadCharString(int len, FILE *h)
{
  char *s = (char *)malloc(len + 1);
  assert(s);
  if (len)
  {
    if (fread(s, len, 1, h) != 1)
    {
      free(s);
      return NULL;
    }
  }
  s[len] = 0;
#ifdef _UNICODE
  TCHAR *sw = (TCHAR *)malloc(sizeof(TCHAR) * (len + 1));
  mbstowcs(sw, s, len + 1);
  free(s);
  return sw;
#else
  return s;
#endif
}


struct Block
{
  int ofs, len;
  int t;
};

static Tab<Block> blk;
static FILE *fileh = NULL;

static void init_blk(FILE *h)
{
  blk.SetCount(0);
  fileh = h;
}

static int begin_blk()
{
  Block b;
  if (fread(&b.len, 4, 1, fileh) != 1)
    return 0;
  if (fread(&b.t, 4, 1, fileh) != 1)
    return 0;
  b.len -= 4;
  b.ofs = ftell(fileh);
  int n = blk.Count();
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
  fseek(fileh, blk[i].ofs + blk[i].len, SEEK_SET);
  blk.Delete(i, 1);
  return 1;
}

static int blk_type()
{
  int i = blk.Count() - 1;
  if (i < 0)
  {
    assert(0);
    return 0;
  }
  return blk[i].t;
}

static int blk_len()
{
  int i = blk.Count() - 1;
  if (i < 0)
  {
    assert(0);
    return 0;
  }
  return blk[i].len;
}

static int blk_rest()
{
  int i = blk.Count() - 1;
  if (i < 0)
  {
    assert(0);
    return 0;
  }
  return blk[i].ofs + blk[i].len - ftell(fileh);
}

//===============================================================================//

static void do_files_include_re(std::vector<TSTR> &result, const std::vector<TSTR> &files, const TSTR &re)
{
  try
  {
    std::wregex reg(re, std::regex_constants::icase);
    for (const TSTR &f : files)
    {
      TSTR filename, ext;
      SplitFilename(f, 0, &filename, &ext);
      if (std::regex_match(filename.data(), reg) || std::regex_match((filename + ext).data(), reg))
        result.push_back(f);
    }
  }
  catch (std::regex_error &e)
  {
    TSTR msg;
    msg.printf(_T("%s\n%s"), re, strToWide(e.what()));
    MessageBox(NULL, msg.data(), _T("Include error"), MB_ICONERROR | MB_OK);
  }
}

static void do_files_exclude_re(std::vector<TSTR> &result, const std::vector<TSTR> &files, const TSTR &re)
{
  try
  {
    std::wregex reg(re, std::regex_constants::icase);
    for (const TSTR &f : files)
    {
      TSTR filename, ext;
      SplitFilename(f, 0, &filename, &ext);
      if (std::regex_match(filename.data(), reg) || std::regex_match((filename + ext).data(), reg))
        continue;
      result.push_back(f);
    }
  }
  catch (std::regex_error &e)
  {
    TSTR msg;
    msg.printf(_T("%s\n%s"), re, strToWide(e.what()));
    MessageBox(NULL, msg.data(), _T("Exclude error"), MB_ICONERROR | MB_OK);
  }
}

static std::vector<TSTR> files_include_re(const std::vector<TSTR> &files, const TSTR &re)
{
  std::vector<TSTR> result;
  result.reserve(32);
  do_files_include_re(result, files, re);
  return result;
}

static std::vector<TSTR> files_exclude_re(const std::vector<TSTR> &files, const TSTR &re)
{
  std::vector<TSTR> result;
  result.reserve(32);
  do_files_exclude_re(result, files, re);
  return result;
}

static bool probe_match_re(const std::vector<TSTR> &files, const TSTR &re)
{
  std::wregex reg(re, std::regex_constants::icase);
  for (const TSTR &f : files)
  {
    TSTR filename, ext;
    SplitFilename(f, 0, &filename, &ext);
    if (std::regex_match(filename.data(), reg) || std::regex_match((filename + ext).data(), reg))
      return true;
  }
  return false;
}

static std::wstring replace_all(std::wstring str, const std::wstring &from, const std::wstring &to)
{
  if (from.empty())
    return str;
  for (size_t pos = 0; (pos = str.find(from, pos)) != std::wstring::npos; pos += to.size())
    str.replace(pos, from.size(), to);
  return str;
}

static std::wstring fnmatch_to_regex(const std::wstring &wildcard)
{
  size_t i = 0, n = wildcard.size();
  std::wstring result;

  while (i < n)
  {
    TCHAR c = wildcard[i];
    ++i;

    if (c == _T('*'))
    {
      result += _T(".*");
      continue;
    }

    if (c == _T('?'))
    {
      result += _T('.');
      continue;
    }

    if (c == _T('['))
    {
      size_t j = i;

      if (j < n && wildcard[j] == _T('!'))
        ++j;

      if (j < n && wildcard[j] == _T(']'))
        ++j;

      while (j < n && wildcard[j] != _T(']'))
        ++j;

      if (j >= n)
      {
        result += _T("\\[");
        continue;
      }

      std::wstring stuff = replace_all(std::wstring(&wildcard[i], j - i), _T("\\"), _T("\\\\"));

      TCHAR first_char = wildcard[i];
      i = j + 1;
      result += _T("[");

      if (first_char == _T('!'))
        result += _T("^") + stuff.substr(1);

      else if (first_char == _T('^'))
        result += _T("\\") + stuff;

      else
        result += stuff;

      result += _T("]");

      continue;
    }

    if (isalnum(c))
    {
      result += c;
      continue;
    }

    result += _T("\\");
    result += c;
  }

  return result;
}

//===============================================================================//

struct ImportedFile
{
  std::wstring fullpath;
  std::wstring basename;

  explicit ImportedFile(const TCHAR *fname) : fullpath(fname), basename(extract_basename(fname).data()) {}
  bool operator==(const ImportedFile &other) const { return fullpath == other.fullpath; }
  bool equalBasename(const TSTR &bname) const { return basename == std::wstring(bname.data()); }
};

struct ImportedFileHash
{
  size_t operator()(const ImportedFile &imp) const
  {
    size_t h1 = std::hash<std::wstring>()(imp.fullpath);
    size_t h2 = std::hash<std::wstring>()(imp.basename);
    return h1 ^ (h2 << 1);
  }
};

//===============================================================================//

class DagImp : public SceneImport
{
public:
  DagImp() {}
  ~DagImp() override {}

  int ExtCount() override { return 1; }
  const TCHAR *Ext(int n) override { return _T("dag"); }

  const TCHAR *LongDesc() override { return GetString(IDS_DAGIMP_LONG); }
  const TCHAR *ShortDesc() override { return GetString(IDS_DAGIMP_SHORT); }
  const TCHAR *AuthorName() override { return GetString(IDS_AUTHOR); }
  const TCHAR *CopyrightMessage() override { return GetString(IDS_COPYRIGHT); }
  const TCHAR *OtherMessage1() override { return _T(""); }
  const TCHAR *OtherMessage2() override { return _T(""); }

  unsigned int Version() override { return 1; }

  void ShowAbout(HWND hWnd) override {}

  int DoImport(const TCHAR *name, ImpInterface *ii, Interface *i, BOOL suppressPrompts = FALSE) override;
  int doImportOne(const TCHAR *name, ImpInterface *ii, Interface *i, BOOL suppressPrompts);

  static bool separateLayers; // Only used when suppressPrompts is set.

  static bool useLegacyImport;
  static bool searchInSubfolders;
  static bool reimportExisting;

  static bool nonInteractive;
  static bool calledFromBatchImport;

  struct Categories
  {
    bool lod;
    bool destr;
    bool dp;
    bool dmg;

    bool any() const { return lod || destr || dp || dmg; }
    void enableAll() { lod = destr = dp = dmg = true; };
  };

  static struct Categories checked;
  static struct Categories detected;
  static ToolTipExtender tooltipExtender;

  static std::vector<TSTR> batchImportFiles;
  static std::unordered_set<ImportedFile, ImportedFileHash> importedFiles;

private:
  int doHierImport(const TSTR &fname, ImpInterface *ii, Interface *ip, bool suppressPrompts);
  void makeHierLayer(const std::vector<TSTR> &fnames, ImpInterface *ii, Interface *ip, bool nomsg);
  void makeHierLayer(const TSTR &fname, ImpInterface *ii, Interface *ip, bool nomsg);

  int doLegacyImport(const TCHAR *fname, ImpInterface *ii, Interface *ip, bool nomsg);
  int doBatchImport(const TSTR &fname, ImpInterface *ii, Interface *ip, bool nomsg);
  int doMaxscriptImport(const TSTR &fname, ImpInterface *ii, Interface *ip, bool nomsg);

  bool isNamesake(const TSTR &fname) const;
};

class DagImpCD : public ClassDesc
{
public:
  int IsPublic() override { return TRUE; }
  void *Create(BOOL loading) override { return new DagImp; }
  const TCHAR *ClassName() override { return GetString(IDS_DAGIMP); }
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  const MCHAR *NonLocalizedClassName() override { return ClassName(); }
#else
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
#endif
  SClass_ID SuperClassID() override { return SCENE_IMPORT_CLASS_ID; }
  Class_ID ClassID() override { return DAGIMP_CID; }
  const TCHAR *Category() override { return _T(""); }

  const TCHAR *InternalName() override { return _T("DagImporter"); }
  HINSTANCE HInstance() override { return hInstance; }
};
static DagImpCD dagimpcd;

ClassDesc *GetDAGIMPCD() { return &dagimpcd; }

//===============================================================================//

class ImpUtil : public UtilityObj
{
public:
  struct Rules
  {
    bool isWildcard;
    std::vector<TSTR> incl;
    std::vector<TSTR> excl;

    explicit Rules(bool isWildcard_) : isWildcard(isWildcard_) {}

    void clear()
    {
      incl.clear();
      excl.clear();
    }
  };

public:
  IUtil *iu;
  Interface *ip;
  HWND hImpHelp, hImpParam, hImpPanel, hTab, hTabVisible;
  int selectedTab;

  Rules wildcard, regex;

  TSTR dirPath;
  TSTR filePath;

  ToolTipExtender tooltipExtender;

  ImpUtil();
  void BeginEditParams(Interface *ip, IUtil *iu) override;
  void EndEditParams(Interface *ip, IUtil *iu) override;
  void DeleteThis() override {}

  LPCTSTR tabResourceName() const;
  DLGPROC tabDlgProc() const;
  LPARAM tabInitParam() const;
  LPCTSTR tabHelp() const;
  LPCTSTR tabHint(int index) const;

  void updateUi() const;
  void updateTab() const;
  void onTabChanged(HWND hDlg);

  void openImportDirectory() const;
  void openDocumentation() const;

  void doImport();
  void doStandardImport() const;
  void doRegexImport(const Rules &rules);
};

static ImpUtil util;

ImpUtil::ImpUtil() :
  iu(0),
  ip(0),
  hImpHelp(0),
  hImpParam(0),
  hImpPanel(0),
  hTab(0),
  hTabVisible(0),
  selectedTab(1),
  wildcard(true),
  regex(false),
  dirPath(_T("c:\\tmp"))
{}

void ImpUtil::updateUi() const
{
  if (!hImpPanel)
    return;

  switch (selectedTab)
  {
    case 0: CheckDlgButton(hTabVisible, IDC_SEPARATE_LAYERS, DagImp::separateLayers); break;

    case 1:
      CheckDlgButton(hTabVisible, IDC_IMPORT_LOD, DagImp::checked.lod);
      CheckDlgButton(hTabVisible, IDC_IMPORT_DP, DagImp::checked.dp);
      CheckDlgButton(hTabVisible, IDC_IMPORT_DMG, DagImp::checked.dmg);
      CheckDlgButton(hTabVisible, IDC_IMPORT_DESTR, DagImp::checked.destr);
      CheckDlgButton(hTabVisible, (!DagImp::reimportExisting ? IDC_RENAME_NEW_LAYER : IDC_REPLACE_EXISTING_LAYER), true);
      CheckDlgButton(hTabVisible, (DagImp::reimportExisting ? IDC_RENAME_NEW_LAYER : IDC_REPLACE_EXISTING_LAYER), false);
      break;

    default: break;
  }

  SetDlgItemText(hImpPanel, IDC_DAGORPATH, filePath.data());
}

LPCTSTR ImpUtil::tabResourceName() const
{
  assert(selectedTab < 4);
  static LPCTSTR data[] = {MAKEINTRESOURCE(IDD_IMPUTIL_LEGACY), MAKEINTRESOURCE(IDD_IMPUTIL_STANDARD),
    MAKEINTRESOURCE(IDD_IMPUTIL_REGEX), MAKEINTRESOURCE(IDD_IMPUTIL_REGEX)};
  return data[selectedTab];
}

static INT_PTR CALLBACK legacy_dlg_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK standard_dlg_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK regex_dlg_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK regex_dlg_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

DLGPROC ImpUtil::tabDlgProc() const
{
  assert(selectedTab < 4);
  static DLGPROC data[] = {legacy_dlg_proc, standard_dlg_proc, regex_dlg_proc, regex_dlg_proc};
  return data[selectedTab];
}

LPARAM ImpUtil::tabInitParam() const
{
  assert(selectedTab < 4);
  static LPARAM data[] = {0, 0, (LPARAM)&wildcard, (LPARAM)&regex};
  return data[selectedTab];
}

LPCTSTR ImpUtil::tabHelp() const
{
  assert(selectedTab < 4);

  // clang-format off
  static LPCTSTR data[] = {
    _T("Use the previous import method:\n")
    _T("detect linked DAGs\n")
    _T("and load them at once\n")
    _T("into separate layers."),

    _T("Import \"* .dag\"\nand related variations."),

    _T("Uses fnmatch-like syntax\n")
    _T("to check asset names.\n")
    _T(" * anything\n")
    _T(" ? any single character\n")
    _T("[seq]  any character in seq\n")
    _T("[!seq] any character not in seq\n"),

    _T("Uses regular expressions\n")
    _T("to check asset names.\n")
  };
  // clang-format on

  return data[selectedTab];
}

LPCTSTR ImpUtil::tabHint(int index) const
{
  // clang-format off
  static LPCTSTR data[] = {
    _T("The previous import method"),
    _T("Import single asset and its related files"),
    _T("Import multiple assets using fnmatch search"),
    _T("Import multiple assets using regular expressions for search")
  };
  // clang-format on

  return (0 <= index && index <= 3) ? data[index] : _T("");
}

void ImpUtil::updateTab() const
{
  RECT rc;
  GetClientRect(hTab, &rc);
  TabCtrl_AdjustRect(hTab, FALSE, &rc);
  MoveWindow(hTabVisible, rc.left, rc.top * 1.2, rc.right - rc.left, rc.bottom - rc.top, TRUE);

  RECT rct;
  GetWindowRect(hTabVisible, &rct);
  update_layout(hTabVisible, MAKELPARAM(rc.right - rc.left, rct.bottom - rct.top));

  // hack: force repaint ALL the area of the panel
  ShowWindow(hTabVisible, SW_HIDE);
  ShowWindow(hTabVisible, SW_SHOW);
}

void ImpUtil::onTabChanged(HWND hDlg)
{
  if (!hTab)
    return;

  if (hTabVisible)
  {
    DestroyWindow(hTabVisible);
    hTabVisible = 0;
  }

  selectedTab = TabCtrl_GetCurSel(hTab);
  assert(selectedTab >= 0 && selectedTab <= 3);
  DagImp::useLegacyImport = (selectedTab == 0);

  hTabVisible = CreateDialogParam(hInstance, tabResourceName(), hDlg, tabDlgProc(), tabInitParam());
  attach_layout_to_dialog(hTabVisible, tabResourceName());
  updateTab();

  SetWindowText(GetDlgItem(hImpHelp, IDC_STATIC1), tabHelp());
  EnableWindow(GetDlgItem(hImpHelp, IDC_OPEN_DOC), selectedTab >= 2);

  EnableWindow(GetDlgItem(hImpParam, IDC_SEARCH_IN_SUBFOLDERS), selectedTab >= 1);
  EnableWindow(GetDlgItem(hImpParam, IDC_REIMPORT_EXISTING), selectedTab >= 1);
}

static bool do_open_import_directory(TSTR &path)
{
  DWORD attr = GetFileAttributes(path.data());
  if (!(attr & FILE_ATTRIBUTE_DIRECTORY))
    path = extract_directory(path);

  INT_PTR result = (INT_PTR)ShellExecute(NULL, _T("explore"), path.data(), NULL, NULL, SW_SHOW);
  if (result <= 32)
    return false;

  return true;
}

void ImpUtil::openImportDirectory() const
{
  TSTR path = (selectedTab <= 1) ? filePath.data() : dirPath.data();
  if (path.isNull() || !do_open_import_directory(path))
    MessageBox(NULL, (TSTR(_T("Failed to open directory: \"")) + path + TSTR(_T("\""))).data(), _T("Error"), MB_ICONERROR | MB_OK);
}

void ImpUtil::openDocumentation() const
{
  // clang-format off
  static const TSTR url[] = {
    _T(""),
    _T(""),
    _T("https://docs.python.org/3/library/fnmatch.html"),
    _T("https://docs.python.org/3/library/re.html#regular-expression-syntax")
  };
  // clang-format on

  ShellExecute(NULL, _T("open"), url[selectedTab], NULL, NULL, SW_SHOWNORMAL);
}

static bool is_regex_empty(const TSTR &re)
{
  // rule that consists solely of spaces is invalid
  for (int i = 0; i < re.Length(); ++i)
    if (re[i] != _T(' '))
      return false;

  return true;
}

static bool is_regex_valid(bool isWildcard, TSTR &re)
{
  if (is_regex_empty(re))
    return false;

  try
  {
    if (isWildcard)
      re = fnmatch_to_regex(re.data()).data();

    std::wregex reg(re, std::regex_constants::icase);
  }
  catch (std::regex_error)
  {
    return false;
  }

  return true;
}

static std::vector<TSTR> get_valid_rules(bool isWildcard, const std::vector<TSTR> &rules)
{
  std::vector<TSTR> result;
  result.reserve(rules.size());

  for (TSTR r : rules)
    if (is_regex_valid(isWildcard, r))
      result.push_back(r);

  return result;
}

void ImpUtil::doStandardImport() const
{
  Autotoggle guard(DagImp::nonInteractive);
  GetCOREInterface()->ImportFromFile(filePath, true);
}

void ImpUtil::doRegexImport(const Rules &rules)
{
  std::vector<TSTR> files = glob(dirPath.data(), DagImp::searchInSubfolders);

  std::vector<TSTR> valid_rules = get_valid_rules(rules.isWildcard, rules.incl);
  if (!valid_rules.empty())
  {
    std::vector<TSTR> tmp_files;
    tmp_files.reserve(files.size());
    for (const TSTR &re : valid_rules)
      do_files_include_re(tmp_files, files, re);
    files = tmp_files;
  }

  valid_rules = get_valid_rules(rules.isWildcard, rules.excl);
  if (!valid_rules.empty())
  {
    std::vector<TSTR> tmp_files;
    tmp_files.reserve(files.size());
    for (const TSTR &re : valid_rules)
      do_files_exclude_re(tmp_files, files, re);
    files = tmp_files;
  }

  DagImp::batchImportFiles = files;
  Autotoggle guard(DagImp::calledFromBatchImport);
  GetCOREInterface()->ImportFromFile(_T("ignored.dag"), true);
}

void ImpUtil::doImport()
{
  assert(selectedTab < 4);
  // DagorLogWindow::clearLog();
  switch (selectedTab)
  {
    case 0:
    case 1: doStandardImport(); break;
    case 2: doRegexImport(util.wildcard); break;
    case 3: doRegexImport(util.regex); break;
    default: assert(false); break;
  }
}

//==========================================================================//

class WListView
{
  HWND hw;
  bool isWildcard;
  std::vector<TSTR> &rules;

public:
  WListView(HWND hw_, bool isWildcard_, std::vector<TSTR> &rules_);

  void AddRule();
  void DelRule();
  INT_PTR EditRule(LPARAM lParam);
  void UpdateView();

private:
  COLORREF ruleColor(TSTR rule);
};

WListView::WListView(HWND hw_, bool isWildcard_, std::vector<TSTR> &rules_) : hw(hw_), isWildcard(isWildcard), rules(rules_)
{
  ListView_SetExtendedListViewStyle(hw, LVS_EX_FULLROWSELECT);

  LVCOLUMN lvc;
  memset(&lvc, 0, sizeof(lvc));
  lvc.mask = LVCF_WIDTH;
  lvc.cx = 1024;
  ListView_InsertColumn(hw, 0, &lvc);

  UpdateView();
}

void WListView::AddRule()
{
  LVITEM lvi;
  memset(&lvi, 0, sizeof(lvi));
  lvi.mask = LVIF_TEXT;
  lvi.cchTextMax = MAX_PATH;
  lvi.iItem = ListView_GetItemCount(hw);
  lvi.iSubItem = 0;
  lvi.pszText = 0;
  ListView_InsertItem(hw, &lvi);
  rules.push_back(lvi.pszText);
}

void WListView::DelRule()
{
  int i = ListView_GetNextItem(hw, -1, LVNI_SELECTED);
  if (i != -1)
  {
    ListView_DeleteItem(hw, i);
    rules.erase(rules.begin() + i);
  }
}

INT_PTR WListView::EditRule(LPARAM lParam)
{
  UINT code = ((LPNMHDR)lParam)->code;

  if (code == LVN_BEGINLABELEDIT)
  {
    DisableAccelerators();
    return FALSE; // allow edit
  }

  if (code == LVN_ENDLABELEDIT)
  {
    EnableAccelerators();

    LVITEM &item = ((LPNMLVDISPINFOW)lParam)->item;
    if (!item.pszText)
      return FALSE; // cancelled by user

    // simple `return TRUE/FALSE` doesn't work here, I don't know why
    ListView_SetItemText(hw, item.iItem, 0, item.pszText);
    rules[item.iItem] = item.pszText;
    return TRUE;
  }

  if (code == NM_CUSTOMDRAW)
  {
    LPNMCUSTOMDRAW lpnmcd = (LPNMCUSTOMDRAW)lParam;
    if (lpnmcd->dwDrawStage == CDDS_PREPAINT)
    {
      SetWindowLong(GetParent(hw), DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
      return CDRF_NOTIFYITEMDRAW;
    }
    if (lpnmcd->dwDrawStage == CDDS_ITEMPREPAINT)
    {
      LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
      int iItem = (int)lplvcd->nmcd.dwItemSpec;
      lplvcd->clrTextBk = ruleColor(rules[iItem]);
    }
    SetWindowLong(GetParent(hw), DWLP_MSGRESULT, CDRF_DODEFAULT);
    return CDRF_DODEFAULT;
  }

  return TRUE;
}

void WListView::UpdateView()
{
  ListView_DeleteAllItems(hw);
  for (TSTR &s : rules)
  {
    LVITEM lvi;
    memset(&lvi, 0, sizeof(lvi));
    lvi.mask = LVIF_TEXT;
    lvi.cchTextMax = MAX_PATH;
    lvi.iItem = ListView_GetItemCount(hw);
    lvi.iSubItem = 0;
    lvi.pszText = (LPWSTR)s.data();
    ListView_InsertItem(hw, &lvi);
  }
}

COLORREF WListView::ruleColor(TSTR rule) { return is_regex_valid(isWildcard, rule) ? VALID_REGEX_COLOR_BG : INVALID_REGEX_COLOR_BG; }

//==========================================================================//

static INT_PTR CALLBACK legacy_dlg_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  static TCHAR path[MAX_PATH] = _T("");
  static bool prevent_enchange = false;

  switch (message)
  {
    case WM_INITDIALOG:
      CheckDlgButton(hDlg, IDC_SEPARATE_LAYERS, DagImp::separateLayers);
      update_path_edit_control(hDlg, IDC_DAGORPATH, util.filePath);
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_SEPARATE_LAYERS), TSTR(_T("Import layered DAGs")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_DAGORPATH), TSTR(_T("Path to file that should be imported")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_SET_DAGORPATH), TSTR(_T("Open a file browser")));
      return FALSE;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_SEPARATE_LAYERS:
          if (HIWORD(wParam) == BN_CLICKED)
            DagImp::separateLayers = IsDlgButtonChecked(hDlg, IDC_SEPARATE_LAYERS);
          break;

        case IDC_SET_DAGORPATH:
          if (get_open_filename(hDlg, util.filePath))
          {
            Autotoggle eguard(prevent_enchange);
            update_path_edit_control(hDlg, IDC_DAGORPATH, util.filePath);
          }
          break;

        case IDC_DAGORPATH:
          switch (HIWORD(wParam))
          {
            case EN_SETFOCUS: DisableAccelerators(); break;
            case EN_KILLFOCUS: EnableAccelerators(); break;
            case EN_CHANGE:
              if (!prevent_enchange)
              {
                GetDlgItemText(hDlg, IDC_DAGORPATH, path, MAX_PATH);

                TSTR new_path = drop_quotation_marks(path);
                if (path != new_path)
                {
                  Autotoggle eguard(prevent_enchange);
                  update_path_edit_control(hDlg, IDC_DAGORPATH, new_path);
                  util.filePath = new_path;
                }
              }
              break;
          }
          break;

        case IDC_OPENDIR: util.openImportDirectory(); break;

        default: break;
      }
      break;

    default: return FALSE;
  }
  return TRUE;
}

static INT_PTR CALLBACK standard_dlg_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  static TCHAR path[MAX_PATH];
  static bool prevent_enchange = false;

  switch (message)
  {
    case WM_INITDIALOG:
    {
      CheckDlgButton(hDlg, IDC_IMPORT_LOD, DagImp::checked.lod);
      CheckDlgButton(hDlg, IDC_IMPORT_DP, DagImp::checked.dp);
      CheckDlgButton(hDlg, IDC_IMPORT_DMG, DagImp::checked.dmg);
      CheckDlgButton(hDlg, IDC_IMPORT_DESTR, DagImp::checked.destr);
      Autotoggle guard(prevent_enchange, true);
      update_path_edit_control(hDlg, IDC_DAGORPATH, util.filePath);
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_IMPORT_LOD), TSTR(_T("Search for other levels of detail")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_IMPORT_DP), TSTR(_T("Search for related Damage Parts")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_IMPORT_DMG), TSTR(_T("Search for Damaged versions")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_IMPORT_DESTR), TSTR(_T("Search for dynamic destr asset")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_DAGORPATH), TSTR(_T("Path to file that should be imported")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_SET_DAGORPATH), TSTR(_T("Open a file browser")));
    }
      return FALSE;

    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
      {
        DagImp::checked.lod = IsDlgButtonChecked(hDlg, IDC_IMPORT_LOD);
        DagImp::checked.dp = IsDlgButtonChecked(hDlg, IDC_IMPORT_DP);
        DagImp::checked.dmg = IsDlgButtonChecked(hDlg, IDC_IMPORT_DMG);
        DagImp::checked.destr = IsDlgButtonChecked(hDlg, IDC_IMPORT_DESTR);
      }
      switch (LOWORD(wParam))
      {
        case IDC_SET_DAGORPATH:
          if (get_open_filename(hDlg, util.filePath))
          {
            Autotoggle eguard(prevent_enchange);
            update_path_edit_control(hDlg, IDC_DAGORPATH, util.filePath);
          }
          break;

        case IDC_DAGORPATH:
          switch (HIWORD(wParam))
          {
            case EN_SETFOCUS: DisableAccelerators(); break;
            case EN_KILLFOCUS: EnableAccelerators(); break;
            case EN_CHANGE:
              if (!prevent_enchange)
              {
                GetDlgItemText(hDlg, IDC_DAGORPATH, path, MAX_PATH);

                TSTR new_path = drop_quotation_marks(path);
                if (path != new_path)
                {
                  Autotoggle eguard(prevent_enchange);
                  update_path_edit_control(hDlg, IDC_DAGORPATH, new_path);
                }

                util.filePath = new_path;
              }
              break;
          }
          break;

        case IDC_OPENDIR: util.openImportDirectory(); break;

        default: break;
      }
      break;

    default: return FALSE;
  }
  return TRUE;
}


static INT_PTR CALLBACK regex_dlg_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  static const wchar_t *PROP_INCL = _T("INCL");
  static const wchar_t *PROP_EXCL = _T("EXCL");
  static TCHAR path[MAX_PATH];
  static bool prevent_enchange = false;

  switch (message)
  {
    case WM_INITDIALOG:
    {
      ImpUtil::Rules &rules = *reinterpret_cast<ImpUtil::Rules *>(lParam);
      SetProp(hDlg, PROP_INCL, (HANDLE) new WListView(GetDlgItem(hDlg, IDC_PATHLIST_INCL), rules.isWildcard, rules.incl));
      SetProp(hDlg, PROP_EXCL, (HANDLE) new WListView(GetDlgItem(hDlg, IDC_PATHLIST_EXCL), rules.isWildcard, rules.excl));
      Autotoggle guard(prevent_enchange);
      update_path_edit_control(hDlg, IDC_DAGORPATH, util.dirPath);
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_PATHLIST_ADD_INCL), TSTR(_T("Adds importer filter")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_PATHLIST_ADD_EXCL), TSTR(_T("Adds importer filter")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_PATHLIST_DEL_INCL), TSTR(_T("Removes importer filter")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_PATHLIST_DEL_EXCL), TSTR(_T("Removes importer filter")));

      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_PATHLIST_INCL), TSTR(_T("To edit a rule, click once and wait")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_PATHLIST_EXCL), TSTR(_T("To edit a rule, click once and wait")));

      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_DAGORPATH), TSTR(_T("Where search for .dag files?")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_SET_DAGORPATH), TSTR(_T("Open a directory browser")));
    }
      return FALSE; // don't set keyboard focus

    case WM_DESTROY:
      delete (WListView *)GetProp(hDlg, PROP_INCL);
      delete (WListView *)GetProp(hDlg, PROP_EXCL);
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_PATHLIST_ADD_INCL: ((WListView *)GetProp(hDlg, PROP_INCL))->AddRule(); break;
        case IDC_PATHLIST_ADD_EXCL: ((WListView *)GetProp(hDlg, PROP_EXCL))->AddRule(); break;
        case IDC_PATHLIST_DEL_INCL: ((WListView *)GetProp(hDlg, PROP_INCL))->DelRule(); break;
        case IDC_PATHLIST_DEL_EXCL: ((WListView *)GetProp(hDlg, PROP_EXCL))->DelRule(); break;

        case IDC_SET_DAGORPATH:
        {
          _tcscpy(path, util.dirPath.data());
          util.ip->ChooseDirectory(hDlg, GetString(IDS_CHOOSE_DAGOR_PATH), path);
          if (path[0])
          {
            util.dirPath = path;
            Autotoggle eguard(prevent_enchange);
            update_path_edit_control(hDlg, IDC_DAGORPATH, util.dirPath);
          }
        }
        break;

        case IDC_DAGORPATH:
          switch (HIWORD(wParam))
          {
            case EN_SETFOCUS: DisableAccelerators(); break;
            case EN_KILLFOCUS: EnableAccelerators(); break;
            case EN_CHANGE:
              if (!prevent_enchange)
              {
                GetDlgItemText(hDlg, IDC_DAGORPATH, path, MAX_PATH);

                TSTR new_path = drop_quotation_marks(path);
                if (path != new_path)
                {
                  Autotoggle eguard(prevent_enchange);
                  update_path_edit_control(hDlg, IDC_DAGORPATH, new_path);
                }

                DWORD attr = GetFileAttributes(new_path.data());
                if (!(attr & FILE_ATTRIBUTE_DIRECTORY))
                {
                  TSTR dir, filename, ext;
                  SplitFilename(new_path.data(), &dir, &filename, &ext);
                  filename += ext;

                  Autotoggle eguard(prevent_enchange);
                  update_path_edit_control(hDlg, IDC_DAGORPATH, new_path);

                  util.dirPath = dir;

                  util.wildcard.clear();
                  util.regex.clear();

                  util.wildcard.incl.push_back(filename.data());
                  util.regex.incl.push_back((TSTR(_T("^")) + filename + TSTR(_T("$"))).data());

                  ((WListView *)GetProp(hDlg, PROP_INCL))->UpdateView();
                  ((WListView *)GetProp(hDlg, PROP_EXCL))->UpdateView();
                }
                else
                  util.dirPath = new_path;
              }
              break;
          }
          break;

        case IDC_OPENDIR: util.openImportDirectory(); break;

        default: break;
      }
      break;

    case WM_NOTIFY:
      if (LOWORD(wParam) == IDC_PATHLIST_INCL)
        return ((WListView *)GetProp(hDlg, PROP_INCL))->EditRule(lParam);
      if (LOWORD(wParam) == IDC_PATHLIST_EXCL)
        return ((WListView *)GetProp(hDlg, PROP_EXCL))->EditRule(lParam);
      break;

    default: return FALSE;
  }
  return TRUE;
}

static INT_PTR CALLBACK imp_help_dlg_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG: return FALSE;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_OPEN_DOC: util.openDocumentation(); break;
      }
      break;

    default: return FALSE;
  }
  return TRUE;
}

static INT_PTR CALLBACK imp_param_dlg_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
      CheckDlgButton(hDlg, IDC_SEARCH_IN_SUBFOLDERS, DagImp::searchInSubfolders);
      CheckDlgButton(hDlg, IDC_REIMPORT_EXISTING, DagImp::reimportExisting);
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_SEARCH_IN_SUBFOLDERS), TSTR(_T("Search for .dag in subfolders as well")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_REIMPORT_EXISTING),
        TSTR(_T("Replace dags that already exist in scene by imported versions")));
      return FALSE;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_SEARCH_IN_SUBFOLDERS: DagImp::searchInSubfolders = IsDlgButtonChecked(hDlg, IDC_SEARCH_IN_SUBFOLDERS); break;

        case IDC_REIMPORT_EXISTING: DagImp::reimportExisting = IsDlgButtonChecked(hDlg, IDC_REIMPORT_EXISTING); break;
      }
      break;

    default: return FALSE;
  }
  return TRUE;
}

static INT_PTR CALLBACK imp_dlg_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
    {
      util.hTab = GetDlgItem(hDlg, IDC_TAB1);
      assert(util.hTab);

      TCITEM tie = {0};
      tie.mask = TCIF_TEXT;
      tie.pszText = (LPWSTR) _T("Legacy");
      TabCtrl_InsertItem(util.hTab, 0, &tie);
      tie.pszText = (LPWSTR) _T("Standard");
      TabCtrl_InsertItem(util.hTab, 1, &tie);
      tie.pszText = (LPWSTR) _T("Wildcard");
      TabCtrl_InsertItem(util.hTab, 2, &tie);
      tie.pszText = (LPWSTR) _T("Regex");
      TabCtrl_InsertItem(util.hTab, 3, &tie);

      TabCtrl_SetCurSel(util.hTab, DagImp::useLegacyImport ? 0 : 1);
      util.onTabChanged(hDlg);

      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_IMPORT), TSTR(_T("Import DAG file(s)")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_OPENDIR), TSTR(_T("Open a path in a file browser")));
    }
      return FALSE; // don't set keyboard focus

    case WM_NOTIFY:
    {
      LPNMHDR tc = (LPNMHDR)lParam;

      if (tc->code == TCN_SELCHANGE && tc->hwndFrom == util.hTab)
        util.onTabChanged(hDlg);

      else if (tc->code == TTN_GETDISPINFO)
      {
        LPNMTTDISPINFO tdi = (LPNMTTDISPINFO)lParam;
        wcscpy_s(tdi->szText, ARRAYSIZE(tdi->szText), util.tabHint(tdi->hdr.idFrom));
      }
    }
    break;

    case LAYOUT_MESSAGE: util.updateTab(); break;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_IMPORT: util.doImport(); break;
      }
      break;

    default: return FALSE;
  }

  return TRUE;
}

void ImpUtil::BeginEditParams(Interface *ip, IUtil *iu)
{
  this->iu = iu;
  this->ip = ip;

  util.tooltipExtender.RemoveToolTips();

  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES;
  InitCommonControlsEx(&icex);

  hImpHelp = add_rollup_page(ip, IDD_IMPUTIL_HELP, imp_help_dlg_proc, GetString(IDS_IMPUTIL_HELP_ROLL), 0, APPENDROLL_CLOSED);
  hImpParam = add_rollup_page(ip, IDD_IMPUTIL_PARAM, imp_param_dlg_proc, GetString(IDS_IMPUTIL_PARAM_ROLL));
  hImpPanel = add_rollup_page(ip, IDD_IMPUTIL, imp_dlg_proc, GetString(IDS_IMPUTIL_ROLL));
}

void ImpUtil::EndEditParams(Interface *ip, IUtil *iu)
{
  this->iu = NULL;
  this->ip = NULL;
  delete_rollup_page(ip, &hImpHelp);
  delete_rollup_page(ip, &hImpParam);
  delete_rollup_page(ip, &hImpPanel);
}

class ImpUtilDesc : public ClassDesc
{
public:
  int IsPublic() override { return 1; }
  void *Create(BOOL loading = FALSE) override { return &util; }
  const TCHAR *ClassName() override { return GetString(IDS_IMPUTIL_NAME); }
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  const MCHAR *NonLocalizedClassName() override { return ClassName(); }
#else
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
#endif
  SClass_ID SuperClassID() override { return UTILITY_CLASS_ID; }
  Class_ID ClassID() override { return ImpUtil_CID; }
  const TCHAR *Category() override { return GetString(IDS_UTIL_CAT); }
  BOOL NeedsToSave() override { return FALSE; }
};

static ImpUtilDesc utilDesc;
ClassDesc *GetImpUtilCD() { return &utilDesc; }

//===============================================================================//

struct ImpMat
{
  DagMater m;
  TCHAR *name;
  TCHAR *clsname;
  TCHAR *script;
  Mtl *mtl;
  ImpMat()
  {
    name = clsname = script = NULL;
    mtl = NULL;
  }
};

struct Impnode
{
  ImpNode *in;
  INode *n;
  //      ushort pid;
  Mtl *mtl;
};

static Tab<TCHAR *> tex;
static Tab<ImpMat> mat;
// static Tab<Impnode> node;
static Tab<TCHAR *> keylabel;
static Tab<DefNoteTrack *> ntrack;
// static Tab<DagNoteKey*> ntrack;
// static Tab<int> ntrack_knum;

static TSTR scene_name = _T("");

static void cleanup(int err = 0)
{
  int i;
  for (i = 0; i < tex.Count(); ++i)
    if (tex[i])
      free(tex[i]);
  tex.ZeroCount();
  for (i = 0; i < mat.Count(); ++i)
  {
    if (mat[i].name)
      free(mat[i].name);
    if (mat[i].clsname)
      free(mat[i].clsname);
    if (mat[i].script)
      free(mat[i].script);
  }
  if (err)
  {
    for (i = 0; i < mat.Count(); ++i)
      if (mat[i].mtl)
        mat[i].mtl->DeleteThis();
    /*              for(i=0;i<node.Count();++i) if(node[i].mtl) {
                            for(int j=0;j<mat.Count();++j) if(mat[j].mtl==node[i].mtl) break;
                            if(j>=mat.Count()) node[i].mtl->DeleteThis();
                    }
    */
  }
  mat.ZeroCount();
  //      node.ZeroCount();
  for (i = 0; i < keylabel.Count(); ++i)
    if (keylabel[i])
      free(keylabel[i]);
  keylabel.ZeroCount();
  // for(i=0;i<ntrack.Count();++i) if(ntrack[i]) free(ntrack[i]);
  for (i = 0; i < ntrack.Count(); ++i)
    if (ntrack[i])
      ntrack[i]->DeleteThis();
  ntrack.ZeroCount();
  // ntrack_knum.ZeroCount();
}

static void make_mtl(ImpMat &m, int ind, Class_ID cid)
{
  m.mtl = (Mtl *)CreateInstance(MATERIAL_CLASS_ID, cid);
  assert(m.mtl);
  IDagorMat *d = (IDagorMat *)m.mtl->GetInterface(I_DAGORMAT);
  assert(d);
  d->set_amb(m.m.amb);
  d->set_diff(m.m.diff);
  d->set_emis(m.m.emis);
  d->set_spec((m.m.power == 0.0f) ? Color(0, 0, 0) : Color(m.m.spec));
  d->set_power(m.m.power);
  d->set_classname(m.clsname);
  d->set_script(m.script);

  char noname = 0;
  if (!m.name)
    noname = 1;
  else if (m.name[0] == 0)
    noname = 1;
  if (!noname)
    m.mtl->SetName(m.name);

  bool real2sided = m.script ? _tcsstr(m.script, _T("real_two_sided=yes")) : false;
  if (real2sided)
  {
    d->set_2sided(IDagorMat::Sides::RealDoubleSided);
    if (m.m.flags & DAG_MF_2SIDED)
    {
      DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Warning,
        _T("The material '%s' is \"real 2-sided\". The \"2-sided\" flag turned off.\r\n"), m.mtl->GetName());
      DagorLogWindow::show();
    }
  }
  else if (m.m.flags & DAG_MF_2SIDED)
    d->set_2sided(IDagorMat::Sides::DoubleSided);
  else
    d->set_2sided(IDagorMat::Sides::OneSided);

  for (int i = 0; i < DAGTEXNUM; ++i)
  {
    TCHAR *s = NULL;
    if (m.m.texid[i] < tex.Count() && (i < 8 || (m.m.flags & DAG_MF_16TEX)))
      s = tex[m.m.texid[i]];
    d->set_texname(i, s);
    if (i == 0 && noname)
    {
      if (s)
      {
        TSTR nm;
        SplitPathFile(TSTR(s), NULL, &nm);
        m.mtl->SetName(nm);
      }
      else
      {
        TSTR nm;
        nm.printf(_T("%s #%02d"), scene_name, ind);
        m.mtl->SetName(nm);
      }
    }
    d->set_param(i, 0);
  }
  m.mtl->ReleaseInterface(I_DAGORMAT, d);
}

static void adj_wtm(Matrix3 &tm)
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

static void adj_pos(Point3 &p)
{
  float a = p.y;
  p.y = p.z;
  p.z = a;
}

static void adj_rot(Quat &q)
{
  float a = q.y;
  q.y = -q.z;
  q.z = -a;
  q.x = -q.x;
  q = Quat(-0.7071067811865476, 0., 0., 0.7071067811865476) * q;
}

static void adj_scl(Point3 &p) { p.z = -p.z; }

#define rd(p, l)                \
  {                             \
    if (fread(p, l, 1, h) != 1) \
      goto read_err;            \
  }
#define bblk          \
  {                   \
    if (!begin_blk()) \
      goto read_err;  \
  }
#define eblk         \
  {                  \
    if (!end_blk())  \
      goto read_err; \
  }

static void flip_normals(Mesh &m)
{
  for (int i = 0; i < m.numFaces; ++i)
    m.FlipNormal(i);
}

static int load_node(INode *pnode, FILE *h, ImpInterface *ii, Interface *ip, Tab<SkinData *> &skin_data, Tab<NodeId> &node_id)
{
  float master_scale = get_master_scale();
  bool need_rescale = fabs(master_scale - 1.f) > 1e-4f;

  ImpNode *in = NULL;
  INode *n = NULL;
  if (pnode)
  {
    in = ii->CreateNode();
    assert(in);
    ii->AddNodeToScene(in);
    n = in->GetINode();
    assert(n);
    // pnode->AttachChild(n,0);
  }
  else
  {
    n = ip->GetRootNode();
    assert(n);
  }
  ushort nid = ~0;
  Mtl *mtl = NULL;
  Object *obj = NULL;
  int nflg = 0;
  while (blk_rest() > 0)
  {
    bblk;

    DebugPrint(_T("load_node %d\n"), blk_type());

    if (blk_type() == DAG_NODE_DATA)
    {
      DagNodeData d;
      rd(&d, sizeof(d));
      nid = d.id;
      n->SetRenderable(d.flg & DAG_NF_RENDERABLE ? 1 : 0);
      n->SetCastShadows(d.flg & DAG_NF_CASTSHADOW ? 1 : 0);
      n->SetRcvShadows(d.flg & DAG_NF_RCVSHADOW ? 1 : 0);
      NodeId id(nid, n);
      node_id.Append(1, &id);
      nflg = d.flg;
      int l = blk_rest();
      if (l > 0 && in)
      {

        TCHAR *nm = ReadCharString(l, h);
        in->SetName(nm, DagImp::useLegacyImport);
        DebugPrint(_T("node <%s>\n"), nm);
        free(nm);
      }
    }
    else if (blk_type() == DAG_NODE_TM && in)
    {
      Matrix3 tm;
      rd(tm.GetAddr(), 4 * 3 * 4);
      tm.SetNotIdent();
      if (need_rescale)
        tm.SetTrans(tm.GetTrans() * master_scale);
      if (pnode->IsRootNode())
        adj_wtm(tm);
      in->SetTransform(0, tm);
      // n->SetNodeTM(0,tm);
    }
    else if (blk_type() == DAG_NODE_ANIM && in)
    {
      int adj = pnode->IsRootNode();
      ushort ntrkid;
      rd(&ntrkid, 2);
      // note track
      if (ntrkid < ntrack.Count())
      {
        DefNoteTrack *nt = ntrack[ntrkid];
        if (nt)
        {
          DefNoteTrack *nnt = (DefNoteTrack *)NewDefaultNoteTrack();
          assert(nnt);
          nnt->keys = nt->keys;
          n->AddNoteTrack(nnt);
        }
      }
      /*
      if(ntrkid<ntrack.Count()) {
          int knum=ntrack_knum[ntrkid];
          DagNoteKey *nk=ntrack[ntrkid];
              DefNoteTrack *nt=(DefNoteTrack*)NewDefaultNoteTrack();
              if(nt) {
              for(int i=0;i<knum;++i) {
                  char *nm="";
                  if(nk[i].id<keylabel.Count()) nm=keylabel[nk[i].id];
                  NoteKey *k=new NoteKey(nk[i].t,nm);
                  nt->keys.Append(1,&k);
              }
              n->AddNoteTrack(nt);
              }
      }
      */
      // create PRS controller
      Control *prs = CreatePRSControl();
      assert(prs);
      n->SetTMController(prs);
      Control *pc = CreateInterpPosition();
      assert(pc);
      verify(prs->SetPositionController(pc));
      Control *rc = CreateInterpRotation();
      assert(rc);
      verify(prs->SetRotationController(rc));
      Control *sc = CreateInterpScale();
      assert(sc);
      verify(prs->SetScaleController(sc));
      // pos track
      IKeyControl *ik = GetKeyControlInterface(pc);
      assert(ik);
      ushort numk;
      rd(&numk, 2);
      if (numk == 1)
      {
        IBezPoint3Key k;
        rd(&k.val, 12);
        if (adj)
          adj_pos(k.val);
        k.intan = Point3(0, 0, 0);
        k.outtan = Point3(0, 0, 0);
        ik->SetNumKeys(1);
        ik->SetKey(0, &k);
      }
      else
      {
        ik->SetNumKeys(numk);
        IBezPoint3Key k;
        k.flags = BEZKEY_XBROKEN | BEZKEY_YBROKEN | BEZKEY_ZBROKEN;
        DagPosKey *dk = new DagPosKey[numk];
        assert(dk);
        rd(dk, sizeof(*dk) * numk);
        for (int i = 0; i < numk; ++i)
        {
          k.time = dk[i].t;
          k.val = dk[i].p;
          float dt;
          if (i > 0)
            dt = (dk[i].t - dk[i - 1].t) / 3.f;
          else
            dt = 1;
          k.intan = dk[i].i / dt;
          if (i + 1 < numk)
            dt = (dk[i + 1].t - dk[i].t) / 3.f;
          else
            dt = 1;
          k.outtan = dk[i].o / dt;
          if (adj)
            adj_pos(k.val);
          if (adj)
            adj_pos(k.intan);
          if (adj)
            adj_pos(k.outtan);
          ik->SetKey(i, &k);
        }
        delete[] dk;
        ik->SortKeys();
      }
      // rot track
      SuspendAnimate();
      AnimateOn();
      ik = GetKeyControlInterface(rc);
      assert(ik);
      rd(&numk, 2);
      if (numk == 1)
      {
        Quat q;
        rd(&q, 16);
        q = Conjugate(q);
        if (adj)
          adj_rot(q);
        rc->SetValue(0, &q);
      }
      else
      {
        DagRotKey *dk = new DagRotKey[numk];
        assert(dk);
        rd(dk, sizeof(*dk) * numk);
        for (int i = 0; i < numk; ++i)
        {
          dk[i].p = Conjugate(dk[i].p);
          if (adj)
            adj_rot(dk[i].p);
          rc->SetValue(dk[i].t, &dk[i].p);
        }
        delete[] dk;
      }
      ResumeAnimate();
      // scl track
      ik = GetKeyControlInterface(sc);
      assert(ik);
      rd(&numk, 2);
      if (numk == 1)
      {
        IBezScaleKey k;
        Point3 p;
        rd(&p, 12);
        if (adj)
          adj_scl(p);
        k.val = p;
        k.intan = Point3(0, 0, 0);
        k.outtan = Point3(0, 0, 0);
        ik->SetNumKeys(1);
        ik->SetKey(0, &k);
      }
      else
      {
        ik->SetNumKeys(numk);
        IBezScaleKey k;
        k.flags = BEZKEY_XBROKEN | BEZKEY_YBROKEN | BEZKEY_ZBROKEN;
        DagPosKey *dk = new DagPosKey[numk];
        assert(dk);
        rd(dk, sizeof(*dk) * numk);
        for (int i = 0; i < numk; ++i)
        {
          k.time = dk[i].t;
          k.val = dk[i].p;
          float dt;
          if (i > 0)
            dt = (dk[i].t - dk[i - 1].t) / 3.f;
          else
            dt = 1;
          k.intan = dk[i].i / dt;
          if (i + 1 < numk)
            dt = (dk[i + 1].t - dk[i].t) / 3.f;
          else
            dt = 1;
          k.outtan = dk[i].o / dt;
          if (adj)
            adj_scl(k.val.s);
          if (adj)
            adj_scl(k.intan);
          if (adj)
            adj_scl(k.outtan);
          ik->SetKey(i, &k);
        }
        delete[] dk;
        ik->SortKeys();
      }
    }
    else if (blk_type() == DAG_NODE_SCRIPT && in)
    {
      DebugPrint(_T("   node script...\n"));
      int l = blk_rest();
      if (l > 0)
      {
        TCHAR *s = ReadCharString(l, h);
        n->SetUserPropBuffer(s);
        free(s);
      }
      DebugPrint(_T("   node script ok\n"));
    }
    else if (blk_type() == DAG_NODE_MATER && in)
    {
      DebugPrint(_T("   node material...\n"));
      int num = blk_len() / 2;
      if (num > 1)
      {
        MultiMtl *mm = NewDefaultMultiMtl();
        assert(mm);
        mm->SetNumSubMtls(num);
        for (int i = 0; i < num; ++i)
        {
          ushort id;
          rd(&id, 2);
          if (id < mat.Count())
          {
            if (!mat[id].mtl)
              make_mtl(mat[id], id, DagorMat_CID);
            if (mat[id].mtl)
              mm->SetSubMtl(i, mat[id].mtl);
          }
        }
        mm->SetName(n->GetName());
        mtl = mm;
      }
      else if (num == 1)
      {
        ushort id;
        rd(&id, 2);
        if (id < mat.Count())
        {
          if (!mat[id].mtl)
            make_mtl(mat[id], id, DagorMat2_CID);
          if (mat[id].mtl)
            mtl = mat[id].mtl;
        }
      }
      DebugPrint(_T("   node materials ok\n"));
    }
    else if (blk_type() == DAG_NODE_OBJ && in)
    {
      DebugPrint(_T("   obj mesh...\n"));
      while (blk_rest() > 0)
      {
        bblk;
        if (blk_type() == DAG_OBJ_BIGMESH)
        {
          TriObject *tri = CreateNewTriObject();
          assert(tri);
          Mesh &m = tri->mesh;
          uint nv = 0;
          rd(&nv, 4);

          verify(m.setNumVerts(nv));
          rd(m.verts, nv * 12);
          if (need_rescale)
            for (int i = 0; i < nv; ++i)
              m.verts[i] *= master_scale;

          uint nf = 0;
          rd(&nf, 4);
          verify(m.setNumFaces(nf));

          DebugPrint(_T("   big faces: %d\n"), nf);

          int i;
          for (i = 0; i < nf; ++i)
          {
            DagBigFace f;
            rd(&f, sizeof(f));
            m.faces[i].v[0] = f.v[0];
            m.faces[i].v[2] = f.v[1];
            m.faces[i].v[1] = f.v[2];
            m.faces[i].setMatID(f.mat);
            m.faces[i].smGroup = f.smgr;
            m.faces[i].flags |= EDGE_ALL;
          }
          uchar numch;
          rd(&numch, 1);
#if MAX_RELEASE >= 3000
          for (int ch = 0; ch < numch; ++ch)
          {
            uint ntv = 0;
            rd(&ntv, 4);
            uchar tcsz, chid;
            rd(&tcsz, 1);
            rd(&chid, 1);
            m.setMapSupport(chid);
            m.setNumMapVerts(chid, ntv);
            Point3 *tv = m.mapVerts(chid);
            for (; ntv; --ntv, ++tv)
            {
              for (i = 0; i < tcsz && i < 3; ++i)
                rd(&tv[0][i], 4);
              for (; i < 3; ++i)
                tv[0][i] = 0;
            }
            TVFace *tf = m.mapFaces(chid);
            for (i = 0; i < nf; ++i)
            {
              DagBigTFace f;
              rd(&f, sizeof(f));
              tf[i].t[0] = f.t[0];
              tf[i].t[2] = f.t[1];
              tf[i].t[1] = f.t[2];
            }
          }
#else
          if (numch >= 1)
          {
            uint ntv = 0;
            rd(&ntv, 4);
            uchar tcsz, chid;
            rd(&tcsz, 1);
            rd(&chid, 1);
            verify(m.setNumTVFaces(nf));
            verify(m.setNumTVerts(ntv));
            Point3 *tv = m.tVerts;
            for (; ntv; --ntv, ++tv)
            {
              for (i = 0; i < tcsz && i < 3; ++i)
                rd((float *)*tv + i, 4);
              for (; i < 3; ++i)
                ((float *)*tv)[i] = 0;
            }
            TVFace *tf = m.tvFace;
            for (i = 0; i < nf; ++i)
            {
              DagBigTFace f;
              rd(&f, sizeof(f));
              tf[i].t[0] = f.t[0];
              tf[i].t[2] = f.t[1];
              tf[i].t[1] = f.t[2];
            }
          }
          if (numch >= 2)
          {
            uint ntv = 0;
            rd(&ntv, 4);
            uchar tcsz, chid;
            rd(&tcsz, 1);
            rd(&chid, 1);
            verify(m.setNumVCFaces(nf));
            verify(m.setNumVertCol(ntv));
            Point3 *tv = m.vertCol;
            for (; ntv; --ntv, ++tv)
            {
              for (i = 0; i < tcsz && i < 3; ++i)
                rd((float *)*tv + i, 4);
              for (; i < 3; ++i)
                ((float *)*tv)[i] = 0;
            }
            TVFace *tf = m.vcFace;
            for (i = 0; i < nf; ++i)
            {
              DagBigTFace f;
              rd(&f, sizeof(f));
              tf[i].t[0] = f.t[0];
              tf[i].t[2] = f.t[1];
              tf[i].t[1] = f.t[2];
            }
          }
#endif
          if (n->GetNodeTM(0).Parity())
            flip_normals(m);
          obj = tri;
        }
        else if (blk_type() == DAG_OBJ_MESH)
        {
          TriObject *tri = CreateNewTriObject();
          assert(tri);
          Mesh &m = tri->mesh;
          uint nv = 0;
          rd(&nv, 2);
          verify(m.setNumVerts(nv));
          if (nv)
          {
            rd(m.verts, nv * 12);
            if (need_rescale)
              for (int i = 0; i < nv; ++i)
                m.verts[i] *= master_scale;
          }

          uint nf = 0;
          rd(&nf, 2);
          verify(m.setNumFaces(nf));
          DebugPrint(_T("   faces: %d\n"), nf);
          int i;
          for (i = 0; i < nf; ++i)
          {
            DagFace f;
            rd(&f, sizeof(f));
            m.faces[i].v[0] = f.v[0];
            m.faces[i].v[2] = f.v[1];
            m.faces[i].v[1] = f.v[2];
            m.faces[i].setMatID(f.mat);
            m.faces[i].smGroup = f.smgr;
            m.faces[i].flags |= EDGE_ALL;
          }
          uchar numch;
          rd(&numch, 1);
#if MAX_RELEASE >= 3000
          for (int ch = 0; ch < numch; ++ch)
          {
            uint ntv = 0;
            rd(&ntv, 2);
            uchar tcsz, chid;
            rd(&tcsz, 1);
            rd(&chid, 1);
            m.setMapSupport(chid);
            m.setNumMapVerts(chid, ntv);
            Point3 *tv = m.mapVerts(chid);
            for (; ntv; --ntv, ++tv)
            {
              for (i = 0; i < tcsz && i < 3; ++i)
                rd(&tv[0][i], 4);
              for (; i < 3; ++i)
                tv[0][i] = 0;
            }
            TVFace *tf = m.mapFaces(chid);
            for (i = 0; i < nf; ++i)
            {
              DagTFace f;
              rd(&f, sizeof(f));
              tf[i].t[0] = f.t[0];
              tf[i].t[2] = f.t[1];
              tf[i].t[1] = f.t[2];
            }
          }
#else
          if (numch >= 1)
          {
            uint ntv = 0;
            rd(&ntv, 2);
            uchar tcsz, chid;
            rd(&tcsz, 1);
            rd(&chid, 1);
            verify(m.setNumTVFaces(nf));
            verify(m.setNumTVerts(ntv));
            Point3 *tv = m.tVerts;
            for (; ntv; --ntv, ++tv)
            {
              for (i = 0; i < tcsz && i < 3; ++i)
                rd((float *)*tv + i, 4);
              for (; i < 3; ++i)
                ((float *)*tv)[i] = 0;
            }
            TVFace *tf = m.tvFace;
            for (i = 0; i < nf; ++i)
            {
              DagTFace f;
              rd(&f, sizeof(f));
              tf[i].t[0] = f.t[0];
              tf[i].t[2] = f.t[1];
              tf[i].t[1] = f.t[2];
            }
          }
          if (numch >= 2)
          {
            uint ntv = 0;
            rd(&ntv, 2);
            uchar tcsz, chid;
            rd(&tcsz, 1);
            rd(&chid, 1);
            verify(m.setNumVCFaces(nf));
            verify(m.setNumVertCol(ntv));
            Point3 *tv = m.vertCol;
            for (; ntv; --ntv, ++tv)
            {
              for (i = 0; i < tcsz && i < 3; ++i)
                rd((float *)*tv + i, 4);
              for (; i < 3; ++i)
                ((float *)*tv)[i] = 0;
            }
            TVFace *tf = m.vcFace;
            for (i = 0; i < nf; ++i)
            {
              DagTFace f;
              rd(&f, sizeof(f));
              tf[i].t[0] = f.t[0];
              tf[i].t[2] = f.t[1];
              tf[i].t[1] = f.t[2];
            }
          }
#endif
          if (n->GetNodeTM(0).Parity())
            flip_normals(m);
          obj = tri;
#if MAX_RELEASE >= 12000
        }
        else if (blk_type() == DAG_OBJ_BONES)
        {
          SkinData *sd = new SkinData;
          sd->skinNode = n;
          rd(&sd->numb, 2);
          sd->bones.SetCount(sd->numb);
          rd(&sd->bones[0], sd->bones.Count() * sizeof(DagBone));
          rd(&sd->numvert, 4);
          sd->wt.SetCount(sd->numvert * sd->numb);
          rd(&sd->wt[0], sd->wt.Count() * sizeof(float));
          skin_data.Append(1, &sd);
#endif
        }
        else if (blk_type() == DAG_OBJ_SPLINES)
        {
          SplineShape *shape = new SplineShape;
          assert(shape);
          BezierShape &shp = shape->shape;
          int numspl;
          rd(&numspl, 4);
          for (; numspl > 0; --numspl)
          {
            Spline3D &s = *shp.NewSpline();
            assert(&s);
            char flg;
            rd(&flg, 1);
            int numk;
            rd(&numk, 4);
            for (; numk > 0; --numk)
            {
              char ktype;
              rd(&ktype, 1);
              Point3 i, p, o;
              rd(&i, 12);
              rd(&p, 12);
              rd(&o, 12);
              if (need_rescale)
              {
                i *= master_scale;
                p *= master_scale;
                o *= master_scale;
              }
              SplineKnot k(KTYPE_BEZIER_CORNER, LTYPE_CURVE, p, i, o);
              s.AddKnot(k);
            }
            s.SetClosed(flg & DAG_SPLINE_CLOSED ? 1 : 0);
            //                                              s.InvalidateGeomCache();
            s.ComputeBezPoints();
          }
          shp.UpdateSels();
          shp.InvalidateGeomCache();
          shp.InvalidateCapCache();
          obj = shape;
        }
        else if (blk_type() == DAG_OBJ_LIGHT)
        {
          DagLight dl;
          DagLight2 dl2;
          rd(&dl, sizeof(dl));
          Color c(dl.r, dl.g, dl.b);
          if (blk_rest() == sizeof(DagLight2))
          {
            rd(&dl2, sizeof(dl2));
          }
          else
          {
            dl2.mult = MaxVal(c);
            dl2.type = DAG_LIGHT_OMNI;
            dl2.hotspot = 0;
            dl2.falloff = 0;
          }
          TimeValue time = ip->GetTime();
          GenLight &lt = *(GenLight *)ip->CreateInstance(LIGHT_CLASS_ID,
            Class_ID((dl2.type == DAG_LIGHT_SPOT ? FSPOT_LIGHT_CLASS_ID
                                                 : (dl2.type == DAG_LIGHT_DIR ? DIR_LIGHT_CLASS_ID : OMNI_LIGHT_CLASS_ID)),
              0));
          assert(&lt);
          float mc = dl2.mult;
          lt.SetIntensity(time, mc);
          if (mc == 0)
            mc = 1;
          lt.SetRGBColor(time, Point3((float *)(c / mc)));
          if (dl2.type != DAG_LIGHT_OMNI)
          {
            lt.SetHotspot(time, dl2.hotspot);
            lt.SetFallsize(time, dl2.falloff);
          }
          lt.SetAtten(time, ATTEN_START, dl.range);
          lt.SetAtten(time, ATTEN_END, dl.range);
          lt.SetUseAtten(TRUE);
          // lt.SetAttenDisplay(TRUE);
          lt.SetDecayType(dl.decay);
          lt.SetDecayRadius(time, dl.drad);
          lt.SetShadow(nflg & DAG_NF_CASTSHADOW ? 1 : 0);
          lt.SetUseLight(TRUE);
          lt.Enable(TRUE);
          obj = &lt;
        }
        else if (blk_type() == DAG_OBJ_FACEFLG && obj && obj->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID, 0)))
        {
          TriObject *tri = (TriObject *)obj;
          Mesh &m = tri->mesh;
          if (n->GetNodeTM(0).Parity())
            flip_normals(m);
          if (m.numFaces == blk_rest())
          {
            for (int i = 0; i < m.numFaces; ++i)
            {
              Face &f = m.faces[i];
              char ef;
              rd(&ef, 1);
              f.flags &= ~(EDGE_ALL | FACE_HIDDEN);
              if (ef & DAG_FACEFLG_EDGE2)
                f.flags |= EDGE_A;
              if (ef & DAG_FACEFLG_EDGE1)
                f.flags |= EDGE_B;
              if (ef & DAG_FACEFLG_EDGE0)
                f.flags |= EDGE_C;
              if (ef & DAG_FACEFLG_HIDDEN)
                f.flags |= FACE_HIDDEN;
            }
          }
          if (n->GetNodeTM(0).Parity())
            flip_normals(m);
        }
        else if (blk_type() == DAG_OBJ_NORMALS && obj && obj->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID, 0)))
        {
          Mesh &m = ((TriObject *)obj)->mesh;

          if (!m.GetSpecifiedNormals())
          {
            m.SpecifyNormals();
          }
          MeshNormalSpec *normalSpec = m.GetSpecifiedNormals();
          if (normalSpec)
          {
            int numNormals;
            rd(&numNormals, 4);

            int rest = blk_rest();

            if (numNormals > 0 && rest == numNormals * 12 + m.numFaces * 12)
            {
              normalSpec->SetParent(&m);
              normalSpec->CheckNormals();
              normalSpec->SetNumNormals(numNormals);

              normalSpec->SetAllExplicit(true);

              rd(normalSpec->GetNormalArray(), numNormals * 12);

              int *indices = new int[m.numFaces * 3];
              rd(indices, m.numFaces * 3 * 4);

              for (int i = 0; i < m.numFaces; i++)
              {
                normalSpec->SetNormalIndex(i, 0, indices[i * 3]);
                normalSpec->SetNormalIndex(i, 1, indices[i * 3 + 2]);
                normalSpec->SetNormalIndex(i, 2, indices[i * 3 + 1]);
              }

              delete[] indices;
            }
          }
        }

        eblk;
      }
      DebugPrint(_T("   obj mesh ok\n"));
    }
    else if (blk_type() == DAG_NODE_CHILDREN)
    {
      while (blk_rest() > 0)
      {
        bblk;
        if (blk_type() == DAG_NODE)
        {
          if (!load_node(n, h, ii, ip, skin_data, node_id))
            goto read_err;
        }
        else if (blk_type() == DAG_MATER)
          goto read_err;
        eblk;
      }
    }
    eblk;
  }

  if (in)
  {
    if (!obj)
    {
      //                      obj=(Object*)ii->Create(HELPER_CLASS_ID,Class_ID(DUMMY_CLASS_ID,0));
      obj = (Object *)ii->Create(GEOMOBJECT_CLASS_ID, Dummy_CID);
      assert(obj);
    }
    in->Reference(obj);
    pnode->AttachChild(n, 0);
  }
  if (mtl)
    n->SetMtl(mtl);
  return 1;
read_err:
  DebugPrint(_T("read error in load_node() at %X of %s\n"), ftell(h), scene_name.data());
  return 0;
}

#if defined(MAX_RELEASE_R13) && MAX_RELEASE >= MAX_RELEASE_R13
static bool trail_stricmp(const TCHAR *str, const TCHAR *str2)
{
  size_t l = _tcslen(str), l2 = _tcslen(str2);
  return (l >= l2) ? _tcsncicmp(str + l - l2, str2, l2) == 0 : false;
}

static bool find_co_files(const TCHAR *fname, Tab<TCHAR *> &fnames)
{
  if (!trail_stricmp(fname, _T(".lod00.dag")))
    return false;
  int base_len = int(_tcslen(fname) - _tcslen(_T(".lod00.dag")));

  TCHAR *p[2];
  p[0] = _tcsdup(fname);
  p[1] = _tcsdup(_T("LOD00"));
  fnames.Append(2, p);

  TCHAR str_buf[512];
  int i;
  for (i = 1; i < 16; i++)
  {
    _stprintf(str_buf, _T("%.*s.lod%02d.dag"), base_len, fname, i);
    if (::_taccess(str_buf, 4) == 0)
    {
      p[0] = _tcsdup(str_buf);
      _stprintf(str_buf, _T("LOD%02d"), i);
      p[1] = _tcsdup(str_buf);
      fnames.Append(2, p);
    }
  }

  _stprintf(str_buf, _T("%.*s_destr.lod00.dag"), base_len, fname);
  if (::_taccess(str_buf, 4) == 0)
  {
    p[0] = _tcsdup(str_buf);
    p[1] = _tcsdup(_T("DESTR"));
    fnames.Append(2, p);
  }

  // if (base_len > 2 && _tcsncmp(&fname[base_len-2], _T("_a"), 2)==0)
  //   base_len -= 2;

  _stprintf(str_buf, _T("%.*s_dm.dag"), base_len, fname);
  if (::_taccess(str_buf, 4) == 0)
  {
    p[0] = _tcsdup(str_buf);
    p[1] = _tcsdup(_T("DM"));
    fnames.Append(2, p);
  }

  for (i = 0; i < 16; i++)
  {
    _stprintf(str_buf, _T("%.*s_dmg.lod%02d.dag"), base_len, fname, i);
    if (::_taccess(str_buf, 4) == 0)
    {
      p[0] = _tcsdup(str_buf);
      _stprintf(str_buf, _T("DMG_LOD%02d"), i);
      p[1] = _tcsdup(str_buf);
      fnames.Append(2, p);
    }
  }

  for (i = 0; i < 16; i++)
  {
    _stprintf(str_buf, _T("%.*s_dmg2.lod%02d.dag"), base_len, fname, i);
    if (::_taccess(str_buf, 4) == 0)
    {
      p[0] = _tcsdup(str_buf);
      _stprintf(str_buf, _T("DMG2_LOD%02d"), i);
      p[1] = _tcsdup(str_buf);
      fnames.Append(2, p);
    }
  }

  for (i = 0; i < 16; i++)
  {
    _stprintf(str_buf, _T("%.*s_expl.lod%02d.dag"), base_len, fname, i);
    if (::_taccess(str_buf, 4) == 0)
    {
      p[0] = _tcsdup(str_buf);
      _stprintf(str_buf, _T("EXPL_LOD%02d"), i);
      p[1] = _tcsdup(str_buf);
      fnames.Append(2, p);
    }
  }

  _stprintf(str_buf, _T("%.*s_xray.dag"), base_len, fname);
  if (::_taccess(str_buf, 4) == 0)
  {
    p[0] = _tcsdup(str_buf);
    p[1] = _tcsdup(_T("XRAY"));
    fnames.Append(2, p);
  }

  if (fnames.Count() > 1)
    return true;

  for (i = 0; i < fnames.Count(); i++)
    free(fnames[i]);
  fnames.ZeroCount();
  return false;
}
#endif

bool DagImp::separateLayers = true;
bool DagImp::useLegacyImport = false;
bool DagImp::searchInSubfolders = false;
bool DagImp::reimportExisting = false;

bool DagImp::nonInteractive = false;
bool DagImp::calledFromBatchImport = false;

DagImp::Categories DagImp::checked = {true, true, true, true};
DagImp::Categories DagImp::detected = {false, false, false, false};
ToolTipExtender DagImp::tooltipExtender;

std::vector<TSTR> DagImp::batchImportFiles;
std::unordered_set<ImportedFile, ImportedFileHash> DagImp::importedFiles;

static BOOL CALLBACK ImportOptDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
    {
      CheckDlgButton(hwndDlg, IDC_USE_LEGACY_IMPORT, DagImp::useLegacyImport);

      CheckDlgButton(hwndDlg, IDC_IMPORT_LOD, DagImp::checked.lod);
      EnableWindow(GetDlgItem(hwndDlg, IDC_IMPORT_LOD), !DagImp::useLegacyImport && DagImp::detected.lod);

      CheckDlgButton(hwndDlg, IDC_IMPORT_DP, DagImp::checked.dp);
      EnableWindow(GetDlgItem(hwndDlg, IDC_IMPORT_DP), !DagImp::useLegacyImport && DagImp::detected.dp);

      CheckDlgButton(hwndDlg, IDC_IMPORT_DMG, DagImp::checked.dmg);
      EnableWindow(GetDlgItem(hwndDlg, IDC_IMPORT_DMG), !DagImp::useLegacyImport && DagImp::detected.dmg);

      CheckDlgButton(hwndDlg, IDC_IMPORT_DESTR, DagImp::checked.destr);
      EnableWindow(GetDlgItem(hwndDlg, IDC_IMPORT_DESTR), !DagImp::useLegacyImport && DagImp::detected.destr);

      CheckDlgButton(hwndDlg, (DagImp::reimportExisting ? IDC_REPLACE_EXISTING_LAYER : IDC_RENAME_NEW_LAYER), true);
      EnableWindow(GetDlgItem(hwndDlg, IDC_RENAME_NEW_LAYER), !DagImp::useLegacyImport);
      EnableWindow(GetDlgItem(hwndDlg, IDC_REPLACE_EXISTING_LAYER), !DagImp::useLegacyImport);

      DagImp::tooltipExtender.RemoveToolTips();
      DagImp::tooltipExtender.SetToolTip(GetDlgItem(hwndDlg, IDC_USE_LEGACY_IMPORT), TSTR(_T("The previous import method.")));
      DagImp::tooltipExtender.SetToolTip(GetDlgItem(hwndDlg, IDC_IMPORT_LOD), TSTR(_T("Import \"*.lodNN.dag\" files.")));
      DagImp::tooltipExtender.SetToolTip(GetDlgItem(hwndDlg, IDC_IMPORT_DP), TSTR(_T("Import \"*_dp_NN.lodNN.dag\" files.")));
      DagImp::tooltipExtender.SetToolTip(GetDlgItem(hwndDlg, IDC_IMPORT_DMG),
        TSTR(_T("Import \"*_dmg.lodNN.dag\" and \"*_dp_NN_dmg.lodNN.dag\" files.")));
      DagImp::tooltipExtender.SetToolTip(GetDlgItem(hwndDlg, IDC_IMPORT_DESTR),
        TSTR(_T("Import \"*_destr.lod00.dag\" and \"*_dp_NN_destr.lod00.dag\" files.")));
      DagImp::tooltipExtender.SetToolTip(GetDlgItem(hwndDlg, IDC_RENAME_NEW_LAYER),
        TSTR(_T("Generate an unique name \"asset_NNN.lods\" for the new layer.")));
      DagImp::tooltipExtender.SetToolTip(GetDlgItem(hwndDlg, IDC_REPLACE_EXISTING_LAYER),
        TSTR(_T("Destroy nodes on existing layer and place imported nodes on it.")));
      SendMessage(DagImp::tooltipExtender.GetToolTipHWND(), TTM_SETDELAYTIME, TTDT_AUTOMATIC, 0);

      HWND focused = GetDlgItem(hwndDlg, lParam);
      if (focused)
        SetFocus(focused);

      update_export_mode(DagImp::useLegacyImport);
    }
      return FALSE;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_USE_LEGACY_IMPORT:
          DagImp::useLegacyImport = IsDlgButtonChecked(hwndDlg, IDC_USE_LEGACY_IMPORT);
          EnableWindow(GetDlgItem(hwndDlg, IDC_IMPORT_LOD), !DagImp::useLegacyImport && DagImp::detected.lod);
          EnableWindow(GetDlgItem(hwndDlg, IDC_IMPORT_DP), !DagImp::useLegacyImport && DagImp::detected.dp);
          EnableWindow(GetDlgItem(hwndDlg, IDC_IMPORT_DMG), !DagImp::useLegacyImport && DagImp::detected.dmg);
          EnableWindow(GetDlgItem(hwndDlg, IDC_IMPORT_DESTR), !DagImp::useLegacyImport && DagImp::detected.destr);
          EnableWindow(GetDlgItem(hwndDlg, IDC_RENAME_NEW_LAYER), !DagImp::useLegacyImport);
          EnableWindow(GetDlgItem(hwndDlg, IDC_REPLACE_EXISTING_LAYER), !DagImp::useLegacyImport);
          update_export_mode(DagImp::useLegacyImport);
          break;

        case IDOK:
          DagImp::checked.lod = IsDlgButtonChecked(hwndDlg, IDC_IMPORT_LOD);
          DagImp::checked.dp = IsDlgButtonChecked(hwndDlg, IDC_IMPORT_DP);
          DagImp::checked.dmg = IsDlgButtonChecked(hwndDlg, IDC_IMPORT_DMG);
          DagImp::checked.destr = IsDlgButtonChecked(hwndDlg, IDC_IMPORT_DESTR);
          DagImp::reimportExisting = IsDlgButtonChecked(hwndDlg, IDC_REPLACE_EXISTING_LAYER);
          EndDialog(hwndDlg, wParam);
          return TRUE;

        case IDCANCEL: EndDialog(hwndDlg, wParam); return TRUE;
      }
      break;

    case WM_SYSCOMMAND:
      if ((wParam & 0xFFF0) == SC_CLOSE)
      {
        EndDialog(hwndDlg, FALSE);
        return TRUE;
      }
      break;
  }
  return FALSE;
}

static TSTR filenameToRegex(const TSTR &filename, bool dp, bool lods, bool destr, bool dmg, TSTR &base_name, bool &exact_match)
{
  static const int dag_length = 4; // ".dag"

  base_name = _T("");
  exact_match = false;

  static std::wregex reg(_T("\\.lod\\d\\d\\.dag$"), std::regex_constants::icase);
  std::wsmatch lod_info;
  std::wstring fn = filename.data();
  if (!std::regex_search(fn, lod_info, reg))
  {
    TSTR re;
    re.printf(_T("^%s$"), filename);
    exact_match = true;
    return re;
  }

  base_name = lod_info.prefix().str().data();
  TSTR base_lod = lod_info[0].str().data();
  base_lod = base_lod.Substr(0, base_lod.Length() - dag_length);

  TSTR dp_re = dp ? _T("(_dp_\\d\\d|)") : _T("()");
  TSTR dmg_re = dmg ? _T("(_dmg|)") : _T("()");
  TSTR lods_re = lods ? _T("\\.lod\\d\\d") : base_lod.data();

  TSTR variants_re;
  variants_re.printf(_T("^%s%s%s%s\\.dag$"), base_name, dp_re, dmg_re, lods_re);
  if (!destr)
    return variants_re;

  TSTR destr_re;
  destr_re.printf(_T("^%s%s%s_destr\\.lod00\\.dag$"), base_name, dp_re, dmg_re);

  TSTR combined_re;
  combined_re.printf(_T("(%s|%s)"), variants_re, destr_re);
  return combined_re;
}

static void deleteNode(INode *node)
{
  while (node->NumChildren() > 0)
    deleteNode(node->GetChildNode(0));

  GetCOREInterface()->DeleteNode(node, true);
};

// asset.lodXX --> asset_001.lodXX
static TSTR makeMangledLayerName(TSTR name) // by val
{
  ILayerManager *manager = GetCOREInterface13()->GetLayerManager();

  ILayer *existingLayer = manager->GetLayer(name);
  if (!existingLayer)
    return name;

  int found = name.first(_T('.'));
  if (found == -1)
  {
    TSTR basename = name;
    unsigned num = 1;
    do
    {
      name.printf(_T("%s_%03d"), basename, num++);
    } while (manager->GetLayer(name));
    return name;
  }

  TSTR basename = name.Substr(0, found);
  TSTR suffix = name.Substr(found, name.length() - found);
  unsigned num = 1;
  do
  {
    name.printf(_T("%s_%03d%s"), basename, num++, suffix);
  } while (manager->GetLayer(name));
  return name;
}

struct ResolvedLayer
{
  ILayer *layer;
  const TSTR name;

  ResolvedLayer(ILayer *l, const TSTR &n) : layer(l), name(n) {}
};

static ResolvedLayer resolveLayerNameCollision(const TSTR &layerName)
{
  ILayerManager *manager = GetCOREInterface13()->GetLayerManager();
  assert(manager);

  ILayer *layer = manager->GetLayer(layerName);

  // no collision, just create a new layer
  if (!layer)
  {
    layer = manager->CreateLayer(const_cast<MSTR &>(layerName)); // MAX2016 has non-const here
    if (layer)
    {
      manager->AddLayer(layer);
      manager->SetCurrentLayer(layer->GetName());
    }
    return ResolvedLayer(layer, layerName);
  }

  // collision: replace existing layer
  if (DagImp::reimportExisting)
  {
    manager->SetCurrentLayer(layer->GetName());

    ILayerProperties *layerProp = (ILayerProperties *)layer->GetInterface(LAYERPROPERTIES_INTERFACE);
    assert(layerProp);
    Tab<INode *> nodes;
    layerProp->Nodes(nodes);
    for (int i = 0; i < nodes.Count(); ++i)
      deleteNode(nodes[i]);

    return ResolvedLayer(layer, layerName);
  }

  // collision: create a new layer with mangled name
  TSTR mangledLayerName = makeMangledLayerName(layerName);
  layer = manager->CreateLayer(mangledLayerName);
  if (layer)
  {
    manager->AddLayer(layer);
    manager->SetCurrentLayer(layer->GetName());
  }
  return ResolvedLayer(layer, mangledLayerName);
}

void DagImp::makeHierLayer(const std::vector<TSTR> &fnames, ImpInterface *ii, Interface *ip, bool nomsg)
{
  static const int dag_length = 4; // ".dag"
  static const int ext_length = 6; // ".lodNN"

  ILayerManager *manager = GetCOREInterface13()->GetLayerManager();

  for (const TSTR &fn : fnames)
  {
    TSTR basename = extract_basename(fn);
    TSTR layerName = basename.Substr(0, basename.length() - dag_length);

    ResolvedLayer resolved = resolveLayerNameCollision(layerName);
    ILayer *subLayer = resolved.layer;
    layerName = resolved.name;

    TSTR root_layerName = layerName.Substr(0, layerName.length() - ext_length) + _T(".lods");
    ILayer *rootLayer = manager->GetLayer(root_layerName);
    bool rootLayerJustCreated = false;

    if (!rootLayer)
    {
      ResolvedLayer root_resolved = resolveLayerNameCollision(root_layerName);
      rootLayer = root_resolved.layer;
      root_layerName = root_resolved.name;
      rootLayerJustCreated = true;
    }

    if (rootLayer && subLayer)
      subLayer->SetParentLayer(rootLayer);

    manager->SetCurrentLayer(layerName);

    if (!doImportOne(fn, ii, ip, nomsg))
    {
      manager->DeleteLayer(layerName);
      if (rootLayerJustCreated)
        manager->DeleteLayer(root_layerName);
      DebugPrint(_T("import error '%s'\n"), fn);
    }
  }
  manager->SetCurrentLayer();
}

void DagImp::makeHierLayer(const TSTR &fname, ImpInterface *ii, Interface *ip, bool nomsg)
{
  static const int dag_length = 4; // ".dag"

  ILayerManager *manager = GetCOREInterface13()->GetLayerManager();

  TSTR basename = extract_basename(fname);
  TSTR layerName = basename.Substr(0, basename.length() - dag_length);

  ResolvedLayer resolved = resolveLayerNameCollision(layerName);
  ILayer *rootLayer = resolved.layer;
  layerName = resolved.name;

  if (!doImportOne(fname, ii, ip, nomsg))
  {
    manager->DeleteLayer(layerName);
    DebugPrint(_T("import error '%s'\n"), fname);
  }

  manager->SetCurrentLayer();
}

int DagImp::doHierImport(const TSTR &fname, ImpInterface *ii, Interface *ip, bool nomsg)
{
  // show UI even on drag&drop
  if (!nonInteractive)
    nomsg = false;

  TSTR dir, filename, ext;
  SplitFilename(fname, &dir, &filename, &ext);
  filename.Append(ext);

  // investigate all files to hilite UI options
  TSTR basename;
  bool exact_match;
  TSTR rex = filenameToRegex(filename, true, true, true, true, basename, exact_match);
  std::vector<TSTR> files = files_include_re(glob(dir, searchInSubfolders), rex);

  TSTR lod_re;
  lod_re.printf(_T("^%s\\.lod\\d\\d\\.dag$"), basename);
  detected.lod = probe_match_re(files, lod_re);

  TSTR dmg_re;
  dmg_re.printf(_T("^%s_dmg\\.lod\\d\\d\\.dag$"), basename);
  detected.dmg = probe_match_re(files, dmg_re);

  TSTR destr_re;
  destr_re.printf(_T("^%s_destr\\.lod00\\.dag$"), basename);
  detected.destr = probe_match_re(files, destr_re);

  TSTR dp_re;
  dp_re.printf(_T("^%s_dp_\\d\\d\\.lod\\d\\d\\.dag$"), basename);
  detected.dp = probe_match_re(files, dp_re);

  TSTR dp_dmg_re;
  dp_dmg_re.printf(_T("^%s_dp_\\d\\d_dmg\\.lod\\d\\d\\.dag$"), basename);
  detected.dmg |= probe_match_re(files, dp_dmg_re);

  TSTR dp_destr_re;
  dp_destr_re.printf(_T("^%s_dp_\\d\\d_destr\\.lod\\d\\d\\.dag$"), basename);
  detected.destr |= probe_match_re(files, dp_destr_re);

  if (!nomsg)
  {
    DagImp::useLegacyImport = false;
    if (IDOK != DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_IMPORT), NULL, (DLGPROC)ImportOptDlgProc, (LPARAM)IDOK))
      return 1;
  }
  if (DagImp::useLegacyImport)
    return 0;

  // get proper list of files based on actual user's choice
  rex = filenameToRegex(filename, checked.dp, checked.lod, checked.destr, checked.dmg, basename, exact_match);
  files = files_include_re(glob(dir, searchInSubfolders), rex);

  if (exact_match)
  {
    if (files.size())
    {
      makeHierLayer(files[0], ii, ip, nomsg);
      return 1;
    }
    return 0;
  }

  std::vector<TSTR> layer_files;
  layer_files.reserve(32);

  if (checked.lod)
    layer_files = files_include_re(files, lod_re);
  else
  {
    layer_files.clear();
    layer_files.push_back(dir + TSTR(_T("\\")) + filename);
  }
  if (!layer_files.empty())
    makeHierLayer(layer_files, ii, ip, nomsg);

  layer_files = files_include_re(files, dmg_re);
  if (!layer_files.empty())
    makeHierLayer(layer_files, ii, ip, nomsg);

  layer_files = files_include_re(files, destr_re);
  if (!layer_files.empty())
    makeHierLayer(layer_files, ii, ip, nomsg);

  for (int i = 0; i < 100; ++i)
  {
    TSTR re;

    re.printf(_T("^%s_dp_%02d.lod\\d\\d\\.dag$"), basename, i);
    layer_files = files_include_re(files, re);
    if (!layer_files.empty())
      makeHierLayer(layer_files, ii, ip, nomsg);

    re.printf(_T("^%s_dp_%02d_dmg.lod\\d\\d\\.dag$"), basename, i);
    layer_files = files_include_re(files, re);
    if (!layer_files.empty())
      makeHierLayer(layer_files, ii, ip, nomsg);

    re.printf(_T("^%s_dp_%02d_destr.lod\\d\\d\\.dag$"), basename, i);
    layer_files = files_include_re(files, re);
    if (!layer_files.empty())
      makeHierLayer(layer_files, ii, ip, nomsg);
  }

  return 1;
}


int DagImp::doLegacyImport(const TCHAR *fname, ImpInterface *ii, Interface *ip, bool nomsg)
{
  Tab<TCHAR *> fnames;

  if (!find_co_files(fname, fnames))
    return doImportOne(fname, ii, ip, nomsg);

  TCHAR buf[1024];
  int fn_len = (int)_tcslen(fname);
  _stprintf(buf, _T("We detected that %s%s\nhas %d linked DAGs.\nLoad them at once into separate layers?"),
    fn_len > 64 ? _T("...") : _T(""), fn_len > 64 ? fname + fn_len - 64 : fname, fnames.Count() / 2 - 1);
  if ((nomsg && !DagImp::separateLayers) ||
      (!nomsg && MessageBox(GetFocus(), buf, _T("Import layered DAGs"), MB_YESNO | MB_ICONQUESTION) != IDYES))
  {
    for (int i = 0; i < fnames.Count(); i++)
      free(fnames[i]);
    return doImportOne(fname, ii, ip, nomsg);
  }

  ILayerManager *manager = GetCOREInterface13()->GetLayerManager();
  manager->Reset();
  for (int i = 0; i < fnames.Count(); i += 2)
  {
    TSTR layer_nm(fnames[i + 1]);
    manager->AddLayer(manager->CreateLayer(layer_nm));
    manager->SetCurrentLayer(layer_nm);
    free(fnames[i + 1]);
    if (!doImportOne(fnames[i], ii, ip, nomsg))
    {
      free(fnames[i]);
      return 0;
    }
    free(fnames[i]);
  }
  manager->SetCurrentLayer();
  return 1;
}


// signle fname is ignored, source files are passed via DagImp::batchImportFiles
int DagImp::doBatchImport(const TSTR & /* ignored */, ImpInterface *ii, Interface *ip, bool nomsg)
{
  static std::wregex re(_T("^.*\\.lod\\d\\d\\.dag$"), std::regex_constants::icase);

  int res = 1;
  Categories bkChecked = checked;
  checked.enableAll();

  Autotoggle guard(nonInteractive);

  std::vector<TSTR> tmp_files;
  tmp_files.reserve(1);

  importedFiles.clear();
  for (const TSTR &fname : batchImportFiles)
  {
    if (reimportExisting && isNamesake(fname))
      continue;

    std::wstring fn = fname.data();
    if (std::regex_match(fn, re))
    {
      tmp_files.clear();
      tmp_files.push_back(fname);
      makeHierLayer(tmp_files, ii, ip, true);
    }
    else
      makeHierLayer(fname, ii, ip, true);
  }

  checked = bkChecked;
  importedFiles.clear();
  batchImportFiles.clear();
  return res;
}

int DagImp::doMaxscriptImport(const TSTR &fname, ImpInterface *ii, Interface *ip, bool nomsg)
{
  return doHierImport(fname, ii, ip, nomsg);
}

//
// Main entry point for GetCOREInterface()->ImportFromFile()
//
int DagImp::DoImport(const TCHAR *fname, ImpInterface *ii, Interface *ip, BOOL nomsg)
{
#if defined(MAX_RELEASE_R13) && MAX_RELEASE >= MAX_RELEASE_R13

  if (useLegacyImport)
    return doLegacyImport(fname, ii, ip, nomsg);

  if (nonInteractive)
    return doMaxscriptImport(fname, ii, ip, nomsg);

  if (calledFromBatchImport)
    return doBatchImport(_T(""), ii, ip, nomsg);

  // else called from File->Import or drag&drop:

  // examine files and display the options window
  int res = doHierImport(fname, ii, ip, nomsg);

  // update batch UI
  util.filePath = fname;
  TabCtrl_SetCurSel(util.hTab, useLegacyImport ? 0 : 1);
  util.onTabChanged(util.hImpPanel);

  // user selected the Legacy option
  if (useLegacyImport)
    return doLegacyImport(fname, ii, ip, nomsg);

  return res;

#else
  return doImportOne(fname, ii, ip, nomsg);
#endif
}

bool DagImp::isNamesake(const TSTR &fname) const
{
  const TSTR basename = extract_basename(fname);

  auto it = std::find_if(importedFiles.begin(), importedFiles.end(),
    [&basename](const ImportedFile &imp) { return imp.equalBasename(basename); });

  if (it != importedFiles.end())
  {
    if (fname.data() != it->fullpath)
    {
      DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Warning, _T("ignore duplicated \"%s\" (already have \"%s\")\r\n"), fname,
        it->fullpath.data());
      DagorLogWindow::show();
    }
    return true;
  }

  return false;
}


#define rd(p, l)                \
  {                             \
    if (fread(p, l, 1, h) != 1) \
      goto read_err;            \
  }


int DagImp::doImportOne(const TCHAR *fname, ImpInterface *ii, Interface *ip, BOOL nomsg)
{
  DebugPrint(_T("import...\n"));

  cleanup();
  Tab<SkinData *> skin_data;
  Tab<NodeId> node_id;
  SplitFilename(TSTR(fname), NULL, &scene_name, NULL);
  FILE *h = _tfopen(fname, _T("rb"));
  if (!h)
  {
    if (!nomsg)
      ip->DisplayTempPrompt(GetString(IDS_FILE_OPEN_ERR), ERRMSG_DELAY);
    return 0;
  }
  {
    int id;
    rd(&id, 4);
    if (id != DAG_ID)
    {
      fclose(h);
      if (!nomsg)
        ip->DisplayTempPrompt(GetString(IDS_INVALID_DAGFILE), ERRMSG_DELAY);
      return 0;
    }
  }

  init_blk(h);
  bblk;
  DebugPrint(_T("imp_a\n"));
  if (blk_type() != DAG_ID)
  {
    fclose(h);
    if (!nomsg)
      ip->DisplayTempPrompt(GetString(IDS_INVALID_DAGFILE), ERRMSG_DELAY);
    return 0;
  }
  for (; blk_rest() > 0;)
  {
    bblk;
    DebugPrint(_T("imp %d\n"), blk_type());
    if (blk_type() == DAG_END)
    {
      eblk;
      break;
    }
    if (blk_type() == DAG_TEXTURES)
    {
      int n = 0;
      rd(&n, 2);
      tex.SetCount(n);
      for (int i = 0; i < n; ++i)
      {
        int l = 0;
        rd(&l, 1);

        tex[i] = ReadCharString(l, h);
        if (!tex[i])
          goto read_err;

        /*tex[i] = (TCHAR *) malloc(l + 1);
        assert(tex[i]);
        if (l)
        {
          rd(tex[i], l);
        }
        tex[i][l] = 0;*/
      }
    }
    else if (blk_type() == DAG_MATER)
    {
      ImpMat m;
      int l = 0;
      rd(&l, 1);
      if (l > 0)
      {
        /* m.name = (TCHAR *) malloc(l + 1);
         assert(m.name);
         rd(m.name, l);
         m.name[l] = 0;*/

        m.name = ReadCharString(l, h);
        if (!m.name)
          goto read_err;
      }
      rd(&m.m, sizeof(m.m));
      l = 0;
      rd(&l, 1);
      if (l > 0)
      {
        /* m.clsname = (TCHAR *) malloc(l + 1);
         assert(m.clsname);
         rd(m.clsname, l);
         m.clsname[l] = 0;*/
        m.clsname = ReadCharString(l, h);
        if (!m.clsname)
          goto read_err;
      }
      l = blk_rest();
      if (l > 0)
      {
        /* m.script = (TCHAR *) malloc(l + 1);
         assert(m.script);
         rd(m.script, l);
         m.script[l] = 0;*/

        m.script = ReadCharString(l, h);
        if (!m.script)
          goto read_err;
      }
      m.mtl = NULL;
      mat.Append(1, &m);
    }
    else if (blk_type() == DAG_KEYLABELS)
    {
      int n = 0;
      rd(&n, 2);
      keylabel.SetCount(n);
      int i;
      for (i = 0; i < n; ++i)
        keylabel[i] = NULL;
      for (i = 0; i < n; ++i)
      {
        int l = 0;
        rd(&l, 1);

        /*char *s = (char *) malloc(l + 1);
        assert(s);
        rd(s, l); s[l] = 0;
        keylabel[i] = s;*/

        keylabel[i] = ReadCharString(l, h);
        if (!keylabel[i])
          goto read_err;
      }
    }
    else if (blk_type() == DAG_NOTETRACK)
    {
      int n = blk_rest() / sizeof(DagNoteKey);
      if (n > 0)
      {
        DagNoteKey *nk = (DagNoteKey *)malloc(n * sizeof(DagNoteKey));
        assert(nk);
        rd(nk, n * sizeof(*nk));
        /*
        ntrack.Append(1,&nk);
        ntrack_knum.Append(1,&n);
        */
        DefNoteTrack *nt = (DefNoteTrack *)NewDefaultNoteTrack();
        if (nt)
        {
          for (int i = 0; i < n; ++i)
          {
            const TCHAR *nm = _T("");
            if (nk[i].id < keylabel.Count())
              nm = keylabel[nk[i].id];
            NoteKey *k = new NoteKey(nk[i].t, nm);
            nt->keys.Append(1, &k);
          }
        }
        ntrack.Append(1, &nt);
        free(nk);
      }
      else
      {
        DefNoteTrack *nt = (DefNoteTrack *)NewDefaultNoteTrack();
        ntrack.Append(1, &nt);
      }
    }
    else if (blk_type() == DAG_NODE)
    {
      if (!load_node(NULL, h, ii, ip, skin_data, node_id))
        goto read_err;
    }
    DebugPrint(_T("imp_b\n"));
    eblk;
  }
  eblk;
#undef rd
#undef bblk
#undef eblk
  fclose(h);

#if MAX_RELEASE >= 12000
  for (int i = 0; i < skin_data.Count(); i++)
  {
    SkinData &sd = *skin_data[i];
    Modifier *skinMod = (Modifier *)CreateInstance(OSM_CLASS_ID, SKIN_CLASSID);
    GetCOREInterface12()->AddModifier(*sd.skinNode, *skinMod);

    ISkin *iskin = (ISkin *)skinMod->GetInterface(I_SKIN);
    ISkinImportData *iskinImport = (ISkinImportData *)skinMod->GetInterface(I_SKINIMPORTDATA);
    assert(iskin);
    assert(iskinImport);

    Tab<INode *> bn;
    Tab<float> wt;
    bn.SetCount(sd.numb);
    wt.SetCount(sd.numb);
    for (int j = 0; j < bn.Count(); j++)
    {
      bn[j] = NULL;
      for (int k = 0; k < node_id.Count(); k++)
        if (sd.bones[j].id == node_id[k].id)
        {
          bn[j] = node_id[k].node;
          break;
        }
      assert(bn[j]);
      iskinImport->AddBoneEx(bn[j], j + 1 == sd.numb);
    }

    for (int j = 0; j < sd.numvert; j++)
    {
      memcpy(&wt[0], &sd.wt[j * wt.Count()], wt.Count() * sizeof(float));
      iskinImport->AddWeights(sd.skinNode, j, bn, wt);
    }

    delete skin_data[i];
  }
#endif

  DebugPrint(_T("clean up\n"));
  cleanup();
  //  collapse_materials(ip);
  //  DebugPrint(_T("collapsed\n"));
  convertnew(ip, true);

  DebugPrint(_T("import ok\n"));

  DagImp::importedFiles.insert(ImportedFile(fname));

  return 1;
read_err:
  DebugPrint(_T("read error at %X of %s\n"), ftell(h), scene_name.data());
  fclose(h);
  cleanup(1);
  if (!nomsg)
    ip->DisplayTempPrompt(GetString(IDS_FILE_READ_ERR), ERRMSG_DELAY);
  return 0;
}

//==========================================================================//

enum ImpOps
{
  fun_import
};

class IDagorImportUtil : public FPStaticInterface
{
public:
  DECLARE_DESCRIPTOR(IDagorImportUtil)
  BEGIN_FUNCTION_MAP FN_9(fun_import, TYPE_BOOL, import_dag, TYPE_STRING, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL,
    TYPE_BOOL, TYPE_BOOL, TYPE_BOOL) END_FUNCTION_MAP

    BOOL import_dag(const TCHAR *fn, bool separateLayers, bool suppressPrompts, bool renameLayerCollision, bool useLegacyImport,
      bool importLod, bool importDestr, bool importDp, bool importDmg)
  {
    DagImp::Categories bkChecked = DagImp::checked;
    bool bkLegacy = DagImp::useLegacyImport;
    bool bkReimportExisting = DagImp::reimportExisting;
    bool bkSeparate = DagImp::separateLayers;

    DagImp::separateLayers = separateLayers;

    DagImp::useLegacyImport = useLegacyImport;
    DagImp::reimportExisting = !renameLayerCollision;
    DagImp::checked.lod = importLod;
    DagImp::checked.destr = importDestr;
    DagImp::checked.dp = importDp;
    DagImp::checked.dmg = importDmg;

    Autotoggle guard(DagImp::nonInteractive);
    BOOL result = GetCOREInterface()->ImportFromFile(fn, suppressPrompts);

    DagImp::reimportExisting = bkReimportExisting;
    DagImp::useLegacyImport = bkLegacy;
    DagImp::checked = bkChecked;
    DagImp::separateLayers = bkSeparate;

    return result;
  }
};

static IDagorImportUtil dagorimputiliface(Interface_ID(0x20906172, 0x435c11e0), _T("dagorImport"), IDS_DAGOR_IMPORT_IFACE, NULL,
  FP_CORE, fun_import, _T("import"), -1, TYPE_BOOL, 0, 9, _T("filename"), -1, TYPE_STRING,
  // f_keyArgDefault marks an optional keyArg param. The value
  // after that is its default value.
  _T("separateLayers"), -1, TYPE_BOOL, f_keyArgDefault, true, _T("suppressPrompts"), -1, TYPE_BOOL, f_keyArgDefault, false,
  _T("renameLayerCollision"), -1, TYPE_BOOL, f_keyArgDefault, true, _T("useLegacyImport"), -1, TYPE_BOOL, f_keyArgDefault, true,
  _T("importLod"), -1, TYPE_BOOL, f_keyArgDefault, true, _T("importDestr"), -1, TYPE_BOOL, f_keyArgDefault, true, _T("importDp"), -1,
  TYPE_BOOL, f_keyArgDefault, true, _T("importDmg"), -1, TYPE_BOOL, f_keyArgDefault, true,
#if defined(MAX_RELEASE_R15) && MAX_RELEASE >= MAX_RELEASE_R15
  p_end
#else
  end
#endif
);
