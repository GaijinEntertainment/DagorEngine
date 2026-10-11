// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <algorithm>
#include <functional>
#include <regex>
#include <format>
#include <unordered_set>
#include <vector>
#include <string_view>
#include <filesystem>
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

namespace fs = std::filesystem;

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

// isolate struct Block to avoid linker confusion with struct Block in expUtil.cpp

namespace
{
struct Block
{
  int ofs, len;
  int t;
};
} // namespace

static Tab<Block> blk;
static FILE *fileh = NULL;
static long file_len = 0;

static void init_blk(FILE *h)
{
  blk.SetCount(0);
  fileh = h;
  file_len = 0;

  // block lengths are declared by the file; measure it once so they can be held to it
  const long at = ftell(h);
  if (at < 0)
    return;
  if (fseek(h, 0, SEEK_END) == 0)
    file_len = ftell(h);
  fseek(h, at, SEEK_SET);
}

static int begin_blk()
{
  Block b;
  if (fread(&b.len, 4, 1, fileh) != 1)
    return 0;
  if (fread(&b.t, 4, 1, fileh) != 1)
    return 0;
  if (b.len < 4)
    return 0;
  b.len -= 4;
  b.ofs = ftell(fileh);
  if (b.ofs < 0 || file_len - b.ofs < b.len)
    return 0;
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

// How many records of item_sz bytes are still left in the current block. Only reached right after a
// read_blk_field() that fit, which leaves what remains of the block non-negative.
static uint blk_rest_items(int item_sz) { return uint(blk_rest()) / uint(item_sz); }

// Reads one count or header field of the current block. begin_blk() holds a block to the file, but
// nothing holds a read to the block, so a field the block does not have would be taken from
// whatever follows it and a zero there would pass for a real count. Once it fits, only the storage
// can leave the read short, so the result is still worth asking for.
static bool read_blk_field(void *dest, int size, FILE *h) { return blk_rest() >= size && fread(dest, size, 1, h) == 1; }

static bool read_char_string(int len, FILE *h, std::wstring &out)
{
  out.clear();
  if (len < 0)
    return false;
  if (len == 0)
    return true;
  std::string s(len, 0);
  if (!read_blk_field(&s[0], len, h))
    return false;
  out = strToWide(s);
  return true;
}

//===============================================================================//

static void report_regex_error(std::wstring_view subject, const std::regex_error &e, const wchar_t *err_title)
{
  std::wstring msg = std::format(L"{}\n{}", subject, strToWide(e.what()));
  MessageBox(NULL, msg.c_str(), err_title, MB_ICONERROR | MB_OK);
}

// a rule may be written with or without the .dag extension
static bool match_re(const std::wstring &stem, const std::wstring &name, const std::wregex &reg)
{
  return std::regex_match(stem, reg) || std::regex_match(name, reg);
}

static bool probe_match_re(const std::vector<fs::path> &files, const std::wstring &re)
{
  const std::wregex reg(re, std::regex_constants::icase);
  for (const fs::path &f : files)
    if (match_re(f.stem().wstring(), f.filename().wstring(), reg))
      return true;
  return false;
}

static std::wstring fnmatch_to_regex(std::wstring_view wildcard)
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

    if (iswalnum(c))
    {
      result += c;
      continue;
    }

    result += _T("\\");
    result += c;
  }

  return result;
}

// makes a literal text safe to splice into a regex
static std::wstring escape_regex(std::wstring_view text)
{
  static constexpr std::wstring_view meta = L".^$|()[]{}*+?\\";

  std::wstring result;
  result.reserve(text.size());

  for (wchar_t c : text)
  {
    if (meta.find(c) != std::wstring_view::npos)
      result += L'\\';
    result += c;
  }

  return result;
}

//===============================================================================//

struct ImportedFile
{
  fs::path fullpath;
  fs::path basename;

  explicit ImportedFile(const TCHAR *fname) : fullpath(fname), basename(fullpath.filename()) {}
  bool operator==(const ImportedFile &other) const { return fullpath == other.fullpath; }
  bool equalBasename(const fs::path &bname) const { return basename == bname; }
};

struct ImportedFileHash
{
  size_t operator()(const ImportedFile &imp) const
  {
    size_t h1 = std::hash<std::wstring>()(imp.fullpath.wstring());
    size_t h2 = std::hash<std::wstring>()(imp.basename.wstring());
    return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
  }
};

//===============================================================================//

class DagImp : public SceneImport
{
public:
  DagImp() {}
  ~DagImp() override = default;

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

  static bool separateLayers;

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
    bool dm;
  };

  static struct Categories checked;
  static struct Categories detected;
  static ToolTipExtender tooltipExtender;

  static std::vector<fs::path> batchImportFiles;
  static std::unordered_set<ImportedFile, ImportedFileHash> importedFiles;

private:
  int doHierImport(const fs::path &fname, ImpInterface *ii, Interface *ip, bool suppressPrompts);
  void makeHierLayer(const std::vector<fs::path> &fnames, ImpInterface *ii, Interface *ip, bool nomsg);
  void makeHierLayer(const fs::path &fname, ImpInterface *ii, Interface *ip, bool nomsg);

  int doLegacyImport(const TCHAR *fname, ImpInterface *ii, Interface *ip, bool nomsg);
  int doBatchImport(const TSTR &fname, ImpInterface *ii, Interface *ip, bool nomsg);
  int doMaxscriptImport(const TSTR &fname, ImpInterface *ii, Interface *ip, bool nomsg);

  bool isNamesake(const fs::path &fname) const;
};

class DagImpCD : public ClassDesc
{
public:
  int IsPublic() override { return TRUE; }
  void *Create(BOOL loading) override { return new DagImp; }
  const TCHAR *ClassName() override { return GetString(IDS_DAGIMP); }
  const MCHAR *NonLocalizedClassName() override { return ClassName(); }
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
    std::vector<std::wstring> incl;
    std::vector<std::wstring> excl;

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

  fs::path dirPath;
  fs::path filePath;

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
  updateTab();

  SetWindowText(GetDlgItem(hImpHelp, IDC_STATIC1), tabHelp());
  EnableWindow(GetDlgItem(hImpHelp, IDC_OPEN_DOC), selectedTab >= 2);

  EnableWindow(GetDlgItem(hImpParam, IDC_SEARCH_IN_SUBFOLDERS), selectedTab >= 1);
  EnableWindow(GetDlgItem(hImpParam, IDC_REIMPORT_EXISTING), selectedTab >= 1);
}

static bool do_open_import_directory(fs::path &path)
{
  if (!fs::is_directory(path))
    path = path.parent_path();

  INT_PTR result = (INT_PTR)ShellExecute(NULL, _T("explore"), path.c_str(), NULL, NULL, SW_SHOW);
  if (result <= 32)
    return false;

  return true;
}

void ImpUtil::openImportDirectory() const
{
  fs::path path = (selectedTab <= 1) ? filePath : dirPath;
  if (path.empty() || !do_open_import_directory(path))
  {
    std::wstring msg = L"Failed to open directory: \"" + path.wstring() + L"\"";
    MessageBox(NULL, msg.c_str(), _T("Error"), MB_ICONERROR | MB_OK);
  }
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

static bool is_regex_empty(std::wstring_view re)
{
  // rule that consists solely of spaces is invalid
  for (wchar_t c : re)
    if (c != L' ')
      return false;

  return true;
}

static std::wregex make_rule_regex(bool isWildcard, std::wstring_view text)
{
  return std::wregex(isWildcard ? fnmatch_to_regex(text) : std::wstring(text), std::regex_constants::icase);
}

static bool is_regex_valid(bool isWildcard, std::wstring_view re)
{
  if (is_regex_empty(re))
    return false;

  try
  {
    make_rule_regex(isWildcard, re);
  }
  catch (const std::regex_error &)
  {
    return false;
  }

  return true;
}

struct CompiledRule
{
  std::wregex re;
  std::wstring text; // the rule as it was written, to name it in an error message
};

struct RuleSet
{
  std::vector<CompiledRule> rule;
  bool any = false; // there was at least one non-blank rule, even if it failed to compile
};

enum class RuleFilterMode
{
  Include, // keep the files that any rule matches
  Exclude, // keep the files that no rule matches
};

static RuleSet compile_rules(bool isWildcard, const std::vector<std::wstring> &rules, const wchar_t *err_title)
{
  RuleSet result;
  result.rule.reserve(rules.size());

  for (const std::wstring &rule : rules)
  {
    if (is_regex_empty(rule))
      continue;

    result.any = true;

    try
    {
      result.rule.push_back({make_rule_regex(isWildcard, rule), rule});
    }
    catch (const std::regex_error &e)
    {
      report_regex_error(rule, e, err_title);
    }
  }

  return result;
}

// a rule that fails to compile is dropped and the remaining ones still apply, but a rule that compiles and
// then throws while matching voids the whole pass: whether it throws depends on the file it is run against,
// so keeping it for the files it survives would make the result depend on the order they are enumerated in.
// either way an unusable rule selects nothing, so a broken include rule never pulls in the whole folder and
// a broken exclude rule never drops anything
static std::vector<fs::path> filter_files_re(const std::vector<fs::path> &files, bool isWildcard,
  const std::vector<std::wstring> &rules, RuleFilterMode mode)
{
  const bool keep_matched = mode == RuleFilterMode::Include;
  const wchar_t *err_title = keep_matched ? L"Include error" : L"Exclude error";

  const RuleSet ruleSet = compile_rules(isWildcard, rules, err_title);
  if (!ruleSet.any)
    return files;

  std::vector<fs::path> result;
  result.reserve(files.size());

  for (const fs::path &f : files)
  {
    const std::wstring stem = f.stem().wstring(), name = f.filename().wstring();

    bool matched = false;
    for (const CompiledRule &rule : ruleSet.rule)
    {
      try
      {
        matched = match_re(stem, name, rule.re);
      }
      catch (const std::regex_error &e)
      {
        // the matcher throws error_complexity/error_stack on its own, on patterns that compiled just fine
        report_regex_error(std::format(L"{}\n{}", rule.text, name), e, err_title);

        if (keep_matched)
          return {};
        return files;
      }

      if (matched)
        break;
    }

    if (matched == keep_matched)
      result.push_back(f);
  }

  return result;
}

static std::vector<fs::path> files_include_re(const std::vector<fs::path> &files, const std::wstring &re)
{
  return filter_files_re(files, false, std::vector<std::wstring>{re}, RuleFilterMode::Include);
}

void ImpUtil::doStandardImport() const
{
  Autotoggle guard(DagImp::nonInteractive);
  GetCOREInterface()->ImportFromFile(filePath.c_str(), true);
}

void ImpUtil::doRegexImport(const Rules &rules)
{
  std::vector<fs::path> files = glob(dirPath, DagImp::searchInSubfolders);

  files = filter_files_re(files, rules.isWildcard, rules.incl, RuleFilterMode::Include);
  files = filter_files_re(files, rules.isWildcard, rules.excl, RuleFilterMode::Exclude);

  DagImp::batchImportFiles = std::move(files);
  Autotoggle guard(DagImp::calledFromBatchImport);
  GetCOREInterface()->ImportFromFile(_T("ignored.dag"), true);
}

void ImpUtil::doImport()
{
  assert(selectedTab < 4);
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
  std::vector<std::wstring> &rules;
  // compiling a regex is far too slow to do per row per repaint, so validity is kept alongside
  // the rules and refreshed only where they change
  std::vector<bool> ruleValid;

public:
  WListView(HWND hw_, bool isWildcard_, std::vector<std::wstring> &rules_);

  void AddRule();
  void DelRule();
  INT_PTR EditRule(LPARAM lParam);
  void UpdateView();

private:
  bool checkRule(std::wstring_view rule) const { return is_regex_valid(isWildcard, rule); }
  COLORREF ruleColor(int index) const;
};

WListView::WListView(HWND hw_, bool isWildcard_, std::vector<std::wstring> &rules_) : hw(hw_), isWildcard(isWildcard_), rules(rules_)
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
  rules.emplace_back();
  ruleValid.push_back(checkRule(rules.back()));
}

void WListView::DelRule()
{
  int i = ListView_GetNextItem(hw, -1, LVNI_SELECTED);
  if (i != -1)
  {
    ListView_DeleteItem(hw, i);
    rules.erase(rules.begin() + i);
    ruleValid.erase(ruleValid.begin() + i);
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
    ruleValid[item.iItem] = checkRule(rules[item.iItem]);
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
      lplvcd->clrTextBk = ruleColor(iItem);
    }
    SetWindowLong(GetParent(hw), DWLP_MSGRESULT, CDRF_DODEFAULT);
    return CDRF_DODEFAULT;
  }

  return TRUE;
}

void WListView::UpdateView()
{
  ListView_DeleteAllItems(hw);
  ruleValid.clear();
  ruleValid.reserve(rules.size());
  for (std::wstring &s : rules)
  {
    LVITEM lvi;
    memset(&lvi, 0, sizeof(lvi));
    lvi.mask = LVIF_TEXT;
    lvi.cchTextMax = MAX_PATH;
    lvi.iItem = ListView_GetItemCount(hw);
    lvi.iSubItem = 0;
    lvi.pszText = s.data();
    ListView_InsertItem(hw, &lvi);
    ruleValid.push_back(checkRule(s));
  }
}

COLORREF WListView::ruleColor(int index) const
{
  const bool valid = index >= 0 && index < int(ruleValid.size()) && ruleValid[index];
  return valid ? VALID_REGEX_COLOR_BG : INVALID_REGEX_COLOR_BG;
}

//==========================================================================//

static INT_PTR CALLBACK legacy_dlg_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
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
                const std::wstring path = get_window_text(GetDlgItem(hDlg, IDC_DAGORPATH));

                std::wstring new_path = drop_quotation_marks(path);
                util.filePath = new_path;
                if (path != new_path)
                {
                  Autotoggle eguard(prevent_enchange);
                  update_path_edit_control(hDlg, IDC_DAGORPATH, util.filePath);
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
  static bool prevent_enchange = false;

  switch (message)
  {
    case WM_INITDIALOG:
    {
      CheckDlgButton(hDlg, IDC_IMPORT_LOD, DagImp::checked.lod);
      CheckDlgButton(hDlg, IDC_IMPORT_DP, DagImp::checked.dp);
      CheckDlgButton(hDlg, IDC_IMPORT_DMG, DagImp::checked.dmg);
      CheckDlgButton(hDlg, IDC_IMPORT_DESTR, DagImp::checked.destr);
      CheckDlgButton(hDlg, IDC_IMPORT_DM, DagImp::checked.dm);
      Autotoggle guard(prevent_enchange, true);
      update_path_edit_control(hDlg, IDC_DAGORPATH, util.filePath);
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_IMPORT_LOD), TSTR(_T("Search for other levels of detail")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_IMPORT_DP), TSTR(_T("Search for related Damage Parts")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_IMPORT_DMG), TSTR(_T("Search for Damaged versions")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_IMPORT_DESTR), TSTR(_T("Search for dynamic destr asset")));
      util.tooltipExtender.SetToolTip(GetDlgItem(hDlg, IDC_IMPORT_DM), TSTR(_T("Search for Damage Model")));
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
        DagImp::checked.dm = IsDlgButtonChecked(hDlg, IDC_IMPORT_DM);
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
                const std::wstring path = get_window_text(GetDlgItem(hDlg, IDC_DAGORPATH));

                std::wstring new_path = drop_quotation_marks(path);
                util.filePath = new_path;
                if (path != new_path)
                {
                  Autotoggle eguard(prevent_enchange);
                  update_path_edit_control(hDlg, IDC_DAGORPATH, util.filePath);
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


static INT_PTR CALLBACK regex_dlg_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  static const wchar_t *PROP_INCL = _T("INCL");
  static const wchar_t *PROP_EXCL = _T("EXCL");
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
          TCHAR dir[MAX_PATH] = {};

          _tcsncpy_s(dir, _countof(dir), util.dirPath.c_str(), _TRUNCATE);
          util.ip->ChooseDirectory(hDlg, GetString(IDS_CHOOSE_DAGOR_PATH), dir);
          if (dir[0])
          {
            util.dirPath = dir;
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
                const std::wstring path = get_window_text(GetDlgItem(hDlg, IDC_DAGORPATH));

                const fs::path new_path = drop_quotation_marks(path);
                if (path != new_path.native())
                {
                  Autotoggle eguard(prevent_enchange);
                  update_path_edit_control(hDlg, IDC_DAGORPATH, new_path);
                }

                std::error_code ec;
                const fs::file_status status = fs::status(new_path, ec);
                if (fs::exists(status) && !fs::is_directory(status))
                {
                  util.dirPath = new_path.parent_path();

                  Autotoggle eguard(prevent_enchange);
                  update_path_edit_control(hDlg, IDC_DAGORPATH, util.dirPath);

                  util.wildcard.clear();
                  util.regex.clear();

                  const std::wstring filename = new_path.filename();
                  util.wildcard.incl.push_back(filename);
                  util.regex.incl.push_back(L'^' + filename + L'$');

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

    case WM_DESTROY:
      util.hTab = 0;
      util.hTabVisible = 0;
      break;

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

  hImpHelp = add_rollup_page(ip, IDD_IMPUTIL_HELP, imp_help_dlg_proc, GetString(IDS_IMPUTIL_HELP_ROLL), APPENDROLL_CLOSED);
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
  const MCHAR *NonLocalizedClassName() override { return ClassName(); }
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
  std::wstring name;
  std::wstring clsname;
  std::wstring script;
  Mtl *mtl;
  ImpMat() : mtl(nullptr) {}
};

static std::vector<std::wstring> tex;
static std::vector<ImpMat> mat;
static std::vector<Mtl *> multi_mat;

static TSTR scene_name = _T("");

static void cleanup()
{
  tex.clear();
  for (size_t i = 0; i < mat.size(); ++i)
    if (mat[i].mtl)
      mat[i].mtl->MaybeAutoDelete();
  mat.clear();
  for (size_t i = 0; i < multi_mat.size(); ++i)
    if (multi_mat[i])
      multi_mat[i]->MaybeAutoDelete();
  multi_mat.clear();
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
  d->set_classname(m.clsname.data());
  d->set_script(m.script.data());

  bool noname = m.name.empty();
  if (!noname)
    m.mtl->SetName(m.name.data());

  bool real2sided = m.script.find(_T("real_two_sided=yes")) != std::wstring::npos;
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
    const TCHAR *s = nullptr;
    if (m.m.texid[i] < tex.size() && (i < 8 || (m.m.flags & DAG_MF_16TEX)))
    {
      const std::wstring &ts = tex[m.m.texid[i]];
      if (!ts.empty())
        s = ts.data();
    }
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
        std::wstring nm = std::format(_T("{} #{:02}"), scene_name.data(), ind);
        m.mtl->SetName(nm.c_str());
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

// The two mesh blocks of DAG_NODE_OBJ differ only in these widths. They are derived from the block
// type once and carried together: a bound that worked one of them out on its own could fall out of
// step with the reads it is there to guard.
struct MeshFormat
{
  bool big;     // DAG_OBJ_BIGMESH: four byte counts and 32 bit indices
  int count_sz; // one count field
  int face_sz;  // one face record
  int tface_sz; // one map face record

  explicit MeshFormat(bool big_) :
    big(big_),
    count_sz(big_ ? 4 : 2),
    face_sz(int(big_ ? sizeof(DagBigFace) : sizeof(DagFace))),
    tface_sz(int(big_ ? sizeof(DagBigTFace) : sizeof(DagTFace)))
  {}
};

// Reads the map channel records that follow the faces of a mesh block. False if the block does not
// hold what the records declare, and it is the block that bounds them: fread() stops at the end of
// the file, end_blk() then seeks back over an overrun, so a record read past the block would import
// as plausible garbage. The format gives every channel one map face per mesh face, so that count
// comes from the mesh the records are read into. `err` names what stopped a read, for the caller to
// log against the node: an aborted import is otherwise silent in a batch, where the prompt is off.
// On false the mesh keeps the channels that were read before the refusal.
#define fail(...)                   \
  {                                 \
    err = std::format(__VA_ARGS__); \
    return false;                   \
  }

// The counts and the indices below are checked against the block and against each other. A record
// read needs none of that: it is already inside its block by the count bound above it, and
// begin_blk() keeps a block inside the file. What a bound cannot rule out is the storage failing
// mid-file, which leaves any read short, so read_data() still asks, the way rd() does for every
// other block of load_node().
#define read_data(dest, size)                     \
  do                                              \
  {                                               \
    const size_t nbytes = (size);                 \
    if (nbytes && fread(dest, nbytes, 1, h) != 1) \
      fail(_T("the file could not be read"));     \
  } while (0)

static bool read_map_channels(Mesh &m, FILE *h, const MeshFormat &fmt, std::wstring &err)
{
  const int nf = m.numFaces;
  const int tface_bytes = nf * fmt.tface_sz;

  uchar numch = 0;
  if (!read_blk_field(&numch, 1, h))
    fail(_T("the map channel count is past the end of its block"));

  for (int ch = 0; ch < numch; ++ch)
  {
    uint ntv = 0;
    uchar tcsz = 0, chid = 0;
    if (!read_blk_field(&ntv, fmt.count_sz, h) || !read_blk_field(&tcsz, 1, h) || !read_blk_field(&chid, 1, h))
      fail(_T("map channel record {} has no header left in its block"), ch);

    // the channel id and the coordinate count come out of the file too, and the map verts are
    // allocated from ntv before anything is read into them. setNumMapVerts() reports nothing, so
    // the counts are all there is to check. A wider coordinate than the three read below would
    // leave the rest of them in the stream and desynchronise the records that follow.
    const int rest = blk_rest();
    if (chid >= MAX_MESHMAPS || tcsz < 1 || tcsz > 3)
      fail(_T("map channel record {} declares id {} and {} coordinates per vertex"), ch, chid, tcsz);
    if (tface_bytes > rest)
      fail(_T("the {} faces of map channel {} do not fit its block"), nf, chid);
    if (ntv > uint(rest - tface_bytes) / uint(tcsz * 4))
      fail(_T("map channel {} declares {} vertices, more than its block holds"), chid, ntv);

    m.setMapSupport(chid);
    m.setNumMapVerts(chid, ntv);
    Point3 *tv = m.mapVerts(chid);
    if (ntv && !tv)
      fail(_T("cannot allocate {} vertices of map channel {}"), ntv, chid);
    for (uint v = 0; v < ntv; ++v, ++tv)
    {
      int i = 0;
      for (; i < tcsz; ++i)
        read_data(&tv[0][i], 4);
      for (; i < 3; ++i)
        tv[0][i] = 0;
    }

    TVFace *tf = m.mapFaces(chid);
    if (nf && !tf)
      fail(_T("cannot allocate the faces of map channel {}"), chid);
    for (int f = 0; f < nf; ++f)
    {
      DagBigTFace t;
      if (fmt.big)
        read_data(&t, sizeof(DagBigTFace));
      else
      {
        DagTFace narrow;
        read_data(&narrow, sizeof(DagTFace));
        t.t[0] = narrow.t[0];
        t.t[1] = narrow.t[1];
        t.t[2] = narrow.t[2];
      }
      // the record fits the block, its indices still have to fit the channel they point into
      if (t.t[0] >= ntv || t.t[1] >= ntv || t.t[2] >= ntv)
        fail(_T("face {} of map channel {} points outside its {} vertices"), f, chid, ntv);
      tf[f].t[0] = t.t[0];
      tf[f].t[2] = t.t[1];
      tf[f].t[1] = t.t[2];
    }
  }
  return true;
}

// Reads a DAG_OBJ_MESH or DAG_OBJ_BIGMESH payload, the two differing only in the width of their
// counts and indices. This is where the bound policy of a mesh block lives: every count comes out
// of the file, so each is held to what the block still holds before it is used as a length or an
// allocation size.
//
// Neither this nor read_map_channels() unwinds what it has already built, and a refusal can come
// after the mesh is allocated and part filled. On false the object holding it has to be discarded,
// the way load_node() does at read_err.
static bool read_mesh(Mesh &m, FILE *h, const MeshFormat &fmt, std::wstring &err)
{
  uint nv = 0;
  if (!read_blk_field(&nv, fmt.count_sz, h))
    fail(_T("the vertex count is past the end of its block"));
  if (nv > blk_rest_items(sizeof(Point3)))
    fail(_T("{} vertices do not fit the block"), nv);
  if (!m.setNumVerts(nv))
    fail(_T("cannot allocate {} vertices"), nv);
  read_data(m.verts, size_t(nv) * sizeof(Point3));

  uint nf = 0;
  if (!read_blk_field(&nf, fmt.count_sz, h))
    fail(_T("the face count is past the end of its block"));
  if (nf > blk_rest_items(fmt.face_sz))
    fail(_T("{} faces do not fit the block"), nf);
  if (!m.setNumFaces(nf))
    fail(_T("cannot allocate {} faces"), nf);

  DebugPrint(_T("   faces: %d\n"), nf);

  for (uint i = 0; i < nf; ++i)
  {
    DagBigFace f;
    if (fmt.big)
      read_data(&f, sizeof(DagBigFace));
    else
    {
      DagFace narrow;
      read_data(&narrow, sizeof(DagFace));
      f.v[0] = narrow.v[0];
      f.v[1] = narrow.v[1];
      f.v[2] = narrow.v[2];
      f.smgr = narrow.smgr;
      f.mat = narrow.mat;
    }
    // the record fits the block, its indices still have to fit the mesh they point into
    if (f.v[0] >= nv || f.v[1] >= nv || f.v[2] >= nv)
      fail(_T("face {} points outside the {} vertices of the mesh"), i, nv);
    m.faces[i].v[0] = f.v[0];
    m.faces[i].v[2] = f.v[1];
    m.faces[i].v[1] = f.v[2];
    m.faces[i].setMatID(f.mat);
    m.faces[i].smGroup = f.smgr;
    m.faces[i].flags |= EDGE_ALL;
  }

  return read_map_channels(m, h, fmt, err);
}

#undef read_data
#undef fail

static int load_node(INode *pnode, FILE *h, ImpInterface *ii, Interface *ip, std::vector<SkinData> &skin_data, Tab<NodeId> &node_id)
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
  }
  else
  {
    n = ip->GetRootNode();
    assert(n);
  }
  ushort nid = ~0;
  Mtl *mtl = NULL;
  Object *obj = NULL;
  MultiMtl *mm = nullptr;
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

        std::wstring nm;
        read_char_string(l, h, nm);
        in->SetName(nm.data(), DagImp::useLegacyImport);
        DebugPrint(_T("node <%s>\n"), nm.data());
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
    }
    else if (blk_type() == DAG_NODE_SCRIPT && in)
    {
      DebugPrint(_T("   node script...\n"));
      int l = blk_rest();
      if (l > 0)
      {
        std::wstring s;
        read_char_string(l, h, s);
        std::wstring trimmed = trim_params(s);
        n->SetUserPropBuffer(trimmed.data());
      }
      DebugPrint(_T("   node script ok\n"));
    }
    else if (blk_type() == DAG_NODE_MATER && in)
    {
      DebugPrint(_T("   node material...\n"));
      int num = blk_len() / 2;
      if (num > 1)
      {
        mm = NewDefaultMultiMtl();
        assert(mm);
        multi_mat.push_back(mm);
        mm->SetNumSubMtls(num);
        for (int i = 0; i < num; ++i)
        {
          ushort id;
          rd(&id, 2);
          if (id < mat.size())
          {
            if (!mat[id].mtl)
              make_mtl(mat[id], id, DagorMat2_CID);
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
        if (id < mat.size())
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
        // These two blocks are the object, unlike FACEFLG, NORMALS and BONES further down, which
        // annotate one that already exists and can each be dropped on their own. A count that does
        // not fit is found part way through building the mesh, so dropping the block would leave the
        // node holding half a mesh or none, and the scene would come back quietly missing geometry
        // rather than missing a detail. A mesh that does not fit its block fails the import instead.
        if (blk_type() == DAG_OBJ_BIGMESH || blk_type() == DAG_OBJ_MESH)
        {
          const MeshFormat fmt(blk_type() == DAG_OBJ_BIGMESH);
          std::wstring err;
          TriObject *tri = CreateNewTriObject();
          bool ok = tri != nullptr;
          if (!ok)
            err = _T("cannot create the mesh object");
          else
          {
            obj = tri;
            Mesh &m = tri->mesh;
            ok = read_mesh(m, h, fmt, err);
            if (ok)
            {
              if (need_rescale)
                for (int i = 0; i < m.numVerts; ++i)
                  m.verts[i] *= master_scale;
              if (n->GetNodeTM(0).Parity())
                flip_normals(m);
            }
          }

          // obj already holds the mesh, so the refusal has to be carried by the reader's own answer
          // rather than by whether it named a reason: one that returned false without naming one
          // would otherwise leave a part built mesh on the node, unlogged
          if (!ok)
          {
            DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Error, _T("%s: '%s': %s, import aborted\r\n"), scene_name.data(),
              n->GetName(), err.c_str());
            DagorLogWindow::show();
            goto read_err;
          }
        }
        else if (blk_type() == DAG_OBJ_BONES)
        {
          ushort numb = 0;
          rd(&numb, 2);

          // A block that does not add up costs its own skin and not the whole scene: eblk skips the
          // rest of it either way, the way the FACEFLG, NORMALS and LIGHT blocks of this loop
          // already behave. A block with no bones carries no skin and no weight matrix at all.
          if (numb)
          {
            // rd() reads the file and stops only at EOF, so the bones and the vertex count behind
            // them have to be measured against what the block still holds. blk_rest() has already
            // gone negative if the two bytes above came from outside it.
            if (blk_rest() < int(numb * sizeof(DagBone)) + 4)
            {
              DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Warning,
                L"%s: '%s': the table of %d bones does not fit its block, skinning skipped\r\n", scene_name.data(), n->GetName(),
                numb);
              DagorLogWindow::show();
            }
            else
            {
              skin_data.emplace_back();
              SkinData &sd = skin_data.back();
              sd.skinNode = n;
              sd.numb = numb;
              sd.bones.SetCount(numb);
              rd(&sd.bones[0], numb * sizeof(DagBone));

              rd(&sd.numvert, 4);

              // the numb by numvert matrix of floats has to fit in what is left of the block too,
              // which also keeps numvert * numb inside an int
              const int maxvert = blk_rest() / int(numb * sizeof(float));
              if (sd.numvert < 0 || sd.numvert > maxvert)
              {
                DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Warning,
                  L"%s: '%s': the weight matrix of %d vertices does not fit its block, skinning skipped\r\n", scene_name.data(),
                  n->GetName(), sd.numvert);
                DagorLogWindow::show();
                skin_data.pop_back();
              }
              else if (sd.numvert)
              {
                sd.wt.SetCount(sd.numvert * numb);
                rd(&sd.wt[0], sd.wt.Count() * sizeof(float));
              }
            }
          }
        }
        else if (blk_type() == DAG_OBJ_SPLINES)
        {
          SplineShape *shape = new SplineShape;
          assert(shape);
          obj = shape;
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
            s.ComputeBezPoints();
          }
          shp.UpdateSels();
          shp.InvalidateGeomCache();
          shp.InvalidateCapCache();
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

            const int rest = blk_rest();

            if (numNormals > 0 && rest == int64_t(numNormals) * 12 + int64_t(m.numFaces) * 12)
            {
              std::vector<Point3> normals(numNormals);
              rd(normals.data(), numNormals * 12);

              std::vector<int> indices(m.numFaces * 3);
              rd(indices.data(), sizeof(int) * indices.size());

              const auto bad = std::ranges::find_if(indices, [numNormals](int id) { return id < 0 || id >= numNormals; });
              if (bad != indices.end())
              {
                DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Warning,
                  L"%s: '%s': face %d points outside the %d normals, normals skipped\r\n", scene_name.data(), n->GetName(),
                  int(bad - indices.begin()) / 3, numNormals);
                DagorLogWindow::show();
              }
              else
              {
                normalSpec->SetParent(&m);
                normalSpec->CheckNormals();
                normalSpec->SetNumNormals(numNormals);

                normalSpec->SetAllExplicit(true);

                std::copy(normals.begin(), normals.end(), normalSpec->GetNormalArray());

                for (int i = 0; i < m.numFaces; i++)
                {
                  normalSpec->SetNormalIndex(i, 0, indices[i * 3]);
                  normalSpec->SetNormalIndex(i, 1, indices[i * 3 + 2]);
                  normalSpec->SetNormalIndex(i, 2, indices[i * 3 + 1]);
                }
              }
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
  if (obj)
    obj->DeleteMe();
  return 0;
}

static bool find_co_files(const fs::path &fname, std::vector<std::wstring> &fnames)
{
  if (!is_dag_file(fname) || !iequal(fname.stem().extension().c_str(), _T(".lod00")))
    return false;
  const fs::path base = fname.parent_path() / fname.stem().stem();

  fnames.push_back(fname);
  fnames.push_back(_T("LOD00"));

  std::wstring formatted_name;
  int i;
  for (i = 1; i < 16; i++)
  {
    formatted_name = std::format(_T("{}.lod{:02}.dag"), base.c_str(), i);
    if (fs::exists(formatted_name))
    {
      fnames.push_back(formatted_name);
      formatted_name = std::format(_T("LOD{:02}"), i);
      fnames.push_back(formatted_name);
    }
  }

  formatted_name = std::format(_T("{}_destr.lod00.dag"), base.c_str());
  if (fs::exists(formatted_name))
  {
    fnames.push_back(formatted_name);
    fnames.push_back(_T("DESTR"));
  }

  formatted_name = std::format(_T("{}_dm.dag"), base.c_str());
  if (fs::exists(formatted_name))
  {
    fnames.push_back(formatted_name);
    fnames.push_back(_T("DM"));
  }

  for (i = 0; i < 16; i++)
  {
    formatted_name = std::format(_T("{}_dmg.lod{:02}.dag"), base.c_str(), i);
    if (fs::exists(formatted_name))
    {
      fnames.push_back(formatted_name);
      formatted_name = std::format(_T("DMG_LOD{:02}"), i);
      fnames.push_back(formatted_name);
    }
  }

  for (i = 0; i < 16; i++)
  {
    formatted_name = std::format(_T("{}_dmg2.lod{:02}.dag"), base.c_str(), i);
    if (fs::exists(formatted_name))
    {
      fnames.push_back(formatted_name);
      formatted_name = std::format(_T("DMG2_LOD{:02}"), i);
      fnames.push_back(formatted_name);
    }
  }

  for (i = 0; i < 16; i++)
  {
    formatted_name = std::format(_T("{}_expl.lod{:02}.dag"), base.c_str(), i);
    if (fs::exists(formatted_name))
    {
      fnames.push_back(formatted_name);
      formatted_name = std::format(_T("EXPL_LOD{:02}"), i);
      fnames.push_back(formatted_name);
    }
  }

  formatted_name = std::format(_T("{}_xray.dag"), base.c_str());
  if (fs::exists(formatted_name))
  {
    fnames.push_back(formatted_name);
    fnames.push_back(_T("XRAY"));
  }

  if (fnames.size() > 1)
    return true;

  fnames.clear();
  return false;
}

bool DagImp::separateLayers = true;
bool DagImp::useLegacyImport = false;
bool DagImp::searchInSubfolders = false;
bool DagImp::reimportExisting = false;

bool DagImp::nonInteractive = false;
bool DagImp::calledFromBatchImport = false;

DagImp::Categories DagImp::checked = {true, true, true, true, true};
DagImp::Categories DagImp::detected = {false, false, false, false, false};
ToolTipExtender DagImp::tooltipExtender;

std::vector<fs::path> DagImp::batchImportFiles;
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

      CheckDlgButton(hwndDlg, IDC_IMPORT_DM, DagImp::checked.dm);
      EnableWindow(GetDlgItem(hwndDlg, IDC_IMPORT_DM), !DagImp::useLegacyImport && DagImp::detected.dm);

      CheckDlgButton(hwndDlg, (DagImp::reimportExisting ? IDC_REPLACE_EXISTING_LAYER : IDC_RENAME_NEW_LAYER), true);
      EnableWindow(GetDlgItem(hwndDlg, IDC_RENAME_NEW_LAYER), !DagImp::useLegacyImport);
      EnableWindow(GetDlgItem(hwndDlg, IDC_REPLACE_EXISTING_LAYER), !DagImp::useLegacyImport);

      DagImp::tooltipExtender.RemoveToolTips();
      DagImp::tooltipExtender.SetToolTip(GetDlgItem(hwndDlg, IDC_USE_LEGACY_IMPORT), TSTR(_T("The previous import method.")));
      DagImp::tooltipExtender.SetToolTip(GetDlgItem(hwndDlg, IDC_IMPORT_LOD), TSTR(_T("Import \"*.lodNN.dag\" files.")));
      DagImp::tooltipExtender.SetToolTip(GetDlgItem(hwndDlg, IDC_IMPORT_DP), TSTR(_T("Import \"*_dp_NN.lodNN.dag\" files.")));
      DagImp::tooltipExtender.SetToolTip(GetDlgItem(hwndDlg, IDC_IMPORT_DM), TSTR(_T("Import \"*_dm.dag\" files.")));
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
    }
      return FALSE;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_USE_LEGACY_IMPORT:
        {
          const bool legacy = IsDlgButtonChecked(hwndDlg, IDC_USE_LEGACY_IMPORT);
          EnableWindow(GetDlgItem(hwndDlg, IDC_IMPORT_LOD), !legacy && DagImp::detected.lod);
          EnableWindow(GetDlgItem(hwndDlg, IDC_IMPORT_DP), !legacy && DagImp::detected.dp);
          EnableWindow(GetDlgItem(hwndDlg, IDC_IMPORT_DMG), !legacy && DagImp::detected.dmg);
          EnableWindow(GetDlgItem(hwndDlg, IDC_IMPORT_DESTR), !legacy && DagImp::detected.destr);
          EnableWindow(GetDlgItem(hwndDlg, IDC_IMPORT_DM), !legacy && DagImp::detected.dm);
          EnableWindow(GetDlgItem(hwndDlg, IDC_RENAME_NEW_LAYER), !legacy);
          EnableWindow(GetDlgItem(hwndDlg, IDC_REPLACE_EXISTING_LAYER), !legacy);
          break;
        }

        case IDOK:
          DagImp::useLegacyImport = IsDlgButtonChecked(hwndDlg, IDC_USE_LEGACY_IMPORT);
          update_export_mode(DagImp::useLegacyImport);
          DagImp::checked.lod = IsDlgButtonChecked(hwndDlg, IDC_IMPORT_LOD);
          DagImp::checked.dp = IsDlgButtonChecked(hwndDlg, IDC_IMPORT_DP);
          DagImp::checked.dmg = IsDlgButtonChecked(hwndDlg, IDC_IMPORT_DMG);
          DagImp::checked.destr = IsDlgButtonChecked(hwndDlg, IDC_IMPORT_DESTR);
          DagImp::checked.dm = IsDlgButtonChecked(hwndDlg, IDC_IMPORT_DM);
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

static std::wstring filenameToRegex(const fs::path &filename, bool dp, bool lods, bool destr, bool dmg, bool dm,
  std::wstring &base_name_re, bool &exact_match)
{
  static const int dag_length = 4; // ".dag"

  base_name_re.clear();
  exact_match = false;

  static const std::wregex reg(L"\\.lod\\d\\d\\.dag$", std::regex_constants::icase);
  std::wsmatch lod_info;
  const std::wstring fn = filename.wstring();
  if (!std::regex_search(fn, lod_info, reg))
  {
    exact_match = true;
    return std::format(L"^{}$", escape_regex(fn));
  }

  base_name_re = escape_regex(lod_info.prefix().str());

  const std::wstring lod_suffix = lod_info[0].str(); // ".lodNN.dag"
  const std::wstring base_lod_re = escape_regex(std::wstring_view(lod_suffix).substr(0, lod_suffix.size() - dag_length));

  const wchar_t *dp_re = dp ? L"(_dp_\\d\\d|)" : L"()";
  const wchar_t *dmg_re = dmg ? L"(_dmg|)" : L"()";
  const wchar_t *destr_re = destr ? L"(_destr|)" : L"";
  const std::wstring lods_re = lods ? std::wstring(L"\\.lod\\d\\d") : base_lod_re;

  const std::wstring variants_re = std::format(L"^{}{}{}{}{}\\.dag$", base_name_re, dp_re, dmg_re, destr_re, lods_re);

  if (dm)
    return std::format(L"({})|(^{}_dm\\.dag$)", variants_re, base_name_re);

  return variants_re;
}

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
      name = std::format(_T("{}_{:03}"), basename.data(), num++).c_str();
    } while (manager->GetLayer(name));
    return name;
  }

  TSTR basename = name.Substr(0, found);
  TSTR suffix = name.Substr(found, name.length() - found);
  unsigned num = 1;
  do
  {
    name = std::format(_T("{}_{:03}{}"), basename.data(), num++, suffix.data()).c_str();
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
    layer = manager->CreateLayer(layerName);
    if (layer)
      manager->SetCurrentLayer(layerName);
    return ResolvedLayer(layer, layerName);
  }

  // collision: replace existing layer
  if (DagImp::reimportExisting)
  {
    manager->SetCurrentLayer(layer->GetName());

    ILayerProperties *layerProp = (ILayerProperties *)layer->GetInterface(LAYERPROPERTIES_INTERFACE);
    assert(layerProp);
    INodeTab nodes;
    layerProp->Nodes(nodes);
    if (nodes.Count())
      GetCOREInterface13()->DeleteNodes(nodes);

    return ResolvedLayer(layer, layerName);
  }

  // collision: create a new layer with mangled name
  TSTR mangledLayerName = makeMangledLayerName(layerName);
  layer = manager->CreateLayer(mangledLayerName);
  if (layer)
    manager->SetCurrentLayer(mangledLayerName);
  return ResolvedLayer(layer, mangledLayerName);
}

static void logImportError(const fs::path &fname, const TSTR &layerName)
{
  DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Error, _T("%s: import failed, layer '%s' is kept\r\n"), fname.c_str(),
    layerName.data());
  DagorLogWindow::show();
}

void DagImp::makeHierLayer(const std::vector<fs::path> &fnames, ImpInterface *ii, Interface *ip, bool nomsg)
{
  static const int ext_length = 6; // ".lodNN"

  ILayerManager *manager = GetCOREInterface13()->GetLayerManager();

  for (const fs::path &fn : fnames)
  {
    TSTR layerName = fn.stem().c_str();

    ResolvedLayer resolved = resolveLayerNameCollision(layerName);
    ILayer *subLayer = resolved.layer;
    layerName = resolved.name;

    TSTR root_layerName = layerName.Substr(0, layerName.length() - ext_length) + _T(".lods");
    ILayer *rootLayer = manager->GetLayer(root_layerName);

    if (!rootLayer)
      rootLayer = resolveLayerNameCollision(root_layerName).layer;

    if (rootLayer && subLayer)
      subLayer->SetParentLayer(rootLayer);

    manager->SetCurrentLayer(layerName);

    if (!doImportOne(fn.c_str(), ii, ip, nomsg))
      logImportError(fn, layerName);
  }
}

void DagImp::makeHierLayer(const fs::path &fname, ImpInterface *ii, Interface *ip, bool nomsg)
{
  TSTR layerName = fname.stem().c_str();

  ResolvedLayer resolved = resolveLayerNameCollision(layerName);
  layerName = resolved.name;

  if (!doImportOne(fname.c_str(), ii, ip, nomsg))
    logImportError(fname, layerName);
}

int DagImp::doHierImport(const fs::path &fname, ImpInterface *ii, Interface *ip, bool nomsg)
{
  // show UI even on drag&drop
  if (!nonInteractive && !ip->GetQuietMode())
    nomsg = false;

  const fs::path dirPath = fname.parent_path();

  // investigate all files to hilite UI options
  std::wstring basename_re;
  bool exact_match;
  std::wstring rex = filenameToRegex(fname.filename(), true, true, true, true, true, basename_re, exact_match);
  std::vector<fs::path> files = files_include_re(glob(dirPath, searchInSubfolders), rex);

  std::wstring lod_re = std::format(L"^{}\\.lod\\d\\d\\.dag$", basename_re);
  detected.lod = probe_match_re(files, lod_re);

  std::wstring dmg_re = std::format(L"^{}_dmg\\.lod\\d\\d\\.dag$", basename_re);
  detected.dmg = probe_match_re(files, dmg_re);

  std::wstring destr_re = std::format(L"^{}_destr\\.lod\\d\\d\\.dag$", basename_re);
  detected.destr = probe_match_re(files, destr_re);

  std::wstring dp_re = std::format(L"^{}_dp_\\d\\d\\.lod\\d\\d\\.dag$", basename_re);
  detected.dp = probe_match_re(files, dp_re);

  std::wstring dp_dmg_re = std::format(L"^{}_dp_\\d\\d_dmg\\.lod\\d\\d\\.dag$", basename_re);
  detected.dmg |= probe_match_re(files, dp_dmg_re);

  std::wstring dp_destr_re = std::format(L"^{}_dp_\\d\\d_destr\\.lod\\d\\d\\.dag$", basename_re);
  detected.destr |= probe_match_re(files, dp_destr_re);

  std::wstring dm_re = std::format(L"^{}_dm\\.dag$", basename_re);
  detected.dm = probe_match_re(files, dm_re);

  if (!nomsg)
  {
    DagImp::useLegacyImport = false;
    if (IDOK != DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_IMPORT), NULL, (DLGPROC)ImportOptDlgProc, (LPARAM)IDOK))
      return 1;
  }
  if (DagImp::useLegacyImport)
    return 0;

  if (exact_match)
  {
    std::error_code ec;
    if (!fs::is_regular_file(fname, ec))
      return 0;
    makeHierLayer(fname, ii, ip, nomsg);
    return 1;
  }

  // get proper list of files based on actual user's choice
  rex = filenameToRegex(fname.filename(), checked.dp, checked.lod, checked.destr, checked.dmg, checked.dm, basename_re, exact_match);
  files = files_include_re(glob(dirPath, searchInSubfolders), rex);

  std::vector<fs::path> layer_files;
  layer_files.reserve(32);

  if (checked.lod)
    layer_files = files_include_re(files, lod_re);
  else
  {
    layer_files.clear();
    layer_files.push_back(fname);
  }
  if (!layer_files.empty())
    makeHierLayer(layer_files, ii, ip, nomsg);

  layer_files = files_include_re(files, dm_re);
  if (!layer_files.empty())
    makeHierLayer(layer_files.front(), ii, ip, nomsg);

  layer_files = files_include_re(files, dmg_re);
  if (!layer_files.empty())
    makeHierLayer(layer_files, ii, ip, nomsg);

  layer_files = files_include_re(files, destr_re);
  if (!layer_files.empty())
    makeHierLayer(layer_files, ii, ip, nomsg);

  for (int i = 0; i < 100; ++i)
  {
    std::wstring re = std::format(L"^{}_dp_{:02}\\.lod\\d\\d\\.dag$", basename_re, i);
    layer_files = files_include_re(files, re);
    if (!layer_files.empty())
      makeHierLayer(layer_files, ii, ip, nomsg);

    re = std::format(L"^{}_dp_{:02}_dmg\\.lod\\d\\d\\.dag$", basename_re, i);
    layer_files = files_include_re(files, re);
    if (!layer_files.empty())
      makeHierLayer(layer_files, ii, ip, nomsg);

    re = std::format(L"^{}_dp_{:02}_destr\\.lod\\d\\d\\.dag$", basename_re, i);
    layer_files = files_include_re(files, re);
    if (!layer_files.empty())
      makeHierLayer(layer_files, ii, ip, nomsg);
  }

  return 1;
}


int DagImp::doLegacyImport(const TCHAR *fname, ImpInterface *ii, Interface *ip, bool nomsg)
{
  std::vector<std::wstring> fnames;

  if (!find_co_files(fname, fnames))
    return doImportOne(fname, ii, ip, nomsg);

  int fn_len = (int)_tcslen(fname);
  std::wstring buf = std::format(_T("We detected that {}{}\nhas {} linked DAGs.\nLoad them at once into separate layers?"),
    fn_len > 64 ? _T("...") : _T(""), fn_len > 64 ? fname + fn_len - 64 : fname, (int)fnames.size() / 2 - 1);
  if ((nomsg && !DagImp::separateLayers) ||
      (!nomsg && MessageBox(GetFocus(), buf.c_str(), _T("Import layered DAGs"), MB_YESNO | MB_ICONQUESTION) != IDYES))
    return doImportOne(fname, ii, ip, nomsg);

  ILayerManager *manager = GetCOREInterface13()->GetLayerManager();
  manager->Reset();
  for (size_t i = 0; i < fnames.size(); i += 2)
  {
    TSTR layer_nm(fnames[i + 1].data());
    manager->CreateLayer(layer_nm);
    manager->SetCurrentLayer(layer_nm);
    if (!doImportOne(fnames[i].data(), ii, ip, nomsg))
      return 0;
  }
  manager->SetCurrentLayer();
  return 1;
}


// signle fname is ignored, source files are passed via DagImp::batchImportFiles
int DagImp::doBatchImport(const TSTR & /* ignored */, ImpInterface *ii, Interface *ip, bool nomsg)
{
  static std::wregex re(_T("^.*\\.lod\\d\\d\\.dag$"), std::regex_constants::icase);

  int res = 1;

  Autotoggle guard(nonInteractive);

  std::vector<fs::path> tmp_files;
  tmp_files.reserve(1);

  importedFiles.clear();
  for (const fs::path &fname : batchImportFiles)
  {
    if (reimportExisting && isNamesake(fname))
      continue;

    if (std::regex_match(fname.wstring(), re))
    {
      tmp_files.clear();
      tmp_files.push_back(fname);
      makeHierLayer(tmp_files, ii, ip, true);
    }
    else
      makeHierLayer(fname, ii, ip, true);
  }

  importedFiles.clear();
  batchImportFiles.clear();
  return res;
}

int DagImp::doMaxscriptImport(const TSTR &fname, ImpInterface *ii, Interface *ip, bool nomsg)
{
  return doHierImport(fname.data(), ii, ip, nomsg);
}

//
// Main entry point for GetCOREInterface()->ImportFromFile()
//
int DagImp::DoImport(const TCHAR *fname, ImpInterface *ii, Interface *ip, BOOL nomsg)
{
  nomsg = nomsg || ip->GetQuietMode();

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
}

bool DagImp::isNamesake(const fs::path &fname) const
{
  const fs::path basename = fname.filename();

  auto it = std::ranges::find_if(importedFiles, [&basename](const ImportedFile &imp) { return imp.equalBasename(basename); });

  if (it != importedFiles.end())
  {
    if (fname != it->fullpath)
    {
      DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Warning, _T("ignore duplicated \"%s\" (already have \"%s\")\r\n"),
        fname.c_str(), it->fullpath.c_str());
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
  std::vector<SkinData> skin_data;
  Tab<NodeId> node_id;
  scene_name = fs::path(fname).stem().c_str();
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
      tex.resize(n);
      for (int i = 0; i < n; ++i)
      {
        int l = 0;
        rd(&l, 1);

        if (!read_char_string(l, h, tex[i]))
          goto read_err;
      }
    }
    else if (blk_type() == DAG_MATER)
    {
      ImpMat m;
      int l = 0;
      rd(&l, 1);
      if (!read_char_string(l, h, m.name))
        goto read_err;
      rd(&m.m, sizeof(m.m));
      l = 0;
      rd(&l, 1);
      if (!read_char_string(l, h, m.clsname))
        goto read_err;
      l = blk_rest();
      if (!read_char_string(l, h, m.script))
        goto read_err;
      mat.push_back(m);
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

  for (size_t i = 0; i < skin_data.size(); i++)
  {
    SkinData &sd = skin_data[i];

    // dagfmt.h: "numv should be equal to number of mesh vertexes", and AddWeights() below indexes
    // the node's object with it. The object cannot be judged while the bone block is read: a later
    // block of the same node replaces it, and in->Reference() keeps the last one.
    Object *nodeObj = sd.skinNode->GetObjectRef();
    const int nodeVerts = (nodeObj && nodeObj->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID, 0))) ? ((TriObject *)nodeObj)->mesh.numVerts : 0;
    if (sd.numvert != nodeVerts)
    {
      DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Warning,
        L"%s: '%s': skin covers %d vertices but the object has %d, skinning skipped\r\n", scene_name.data(), sd.skinNode->GetName(),
        sd.numvert, nodeVerts);
      DagorLogWindow::show();
      continue;
    }

    // a bone that is not in the scene takes its share of every vertex weight with it, and a skin
    // that deforms wrong is worse than none, so the whole skin goes rather than that one bone
    Tab<INode *> bn;
    bn.SetCount(sd.numb);
    bool bonesResolved = true;
    for (int j = 0; j < bn.Count(); j++)
    {
      bn[j] = NULL;
      for (int k = 0; k < node_id.Count(); k++)
        if (sd.bones[j].id == node_id[k].id)
        {
          bn[j] = node_id[k].node;
          break;
        }
      if (!bn[j])
      {
        DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Warning,
          L"%s: '%s': skin refers to missing bone %04X, skinning skipped\r\n", scene_name.data(), sd.skinNode->GetName(),
          sd.bones[j].id);
        DagorLogWindow::show();
        bonesResolved = false;
        break;
      }
    }
    if (!bonesResolved)
      continue;

    // the interface is taken before the modifier is attached, so a refusal leaves nothing on the node
    Modifier *skinMod = (Modifier *)CreateInstance(OSM_CLASS_ID, SKIN_CLASSID);
    ISkinImportData *iskinImport = skinMod ? (ISkinImportData *)skinMod->GetInterface(I_SKINIMPORTDATA) : NULL;
    if (!iskinImport)
    {
      DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Warning, L"%s: '%s': the skin modifier is unavailable, skinning skipped\r\n",
        scene_name.data(), sd.skinNode->GetName());
      DagorLogWindow::show();
      if (skinMod)
        skinMod->MaybeAutoDelete();
      continue;
    }
    if (GetCOREInterface12()->AddModifier(*sd.skinNode, *skinMod) != Interface7::kRES_SUCCESS)
    {
      DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Warning,
        L"%s: '%s': the skin modifier does not apply to the object, skinning skipped\r\n", scene_name.data(), sd.skinNode->GetName());
      DagorLogWindow::show();
      skinMod->MaybeAutoDelete();
      continue;
    }

    // AddWeights() indexes the bone table, so one bone the modifier refuses reopens the hole the
    // resolution pass above closed, and the skin has to come off the node again
    bool skinFilled = true;
    std::wstring skinErr;
    for (int j = 0; j < bn.Count() && skinFilled; j++)
    {
      skinFilled = iskinImport->AddBoneEx(bn[j], j + 1 == sd.numb);
      if (!skinFilled)
        skinErr = std::format(L"the skin modifier did not take bone {:04X}", sd.bones[j].id);
    }

    Tab<float> wt;
    wt.SetCount(sd.numb);
    for (int j = 0; j < sd.numvert && skinFilled; j++)
    {
      for (int b = 0; b < sd.numb; b++)
        wt[b] = sd.wt[b * sd.numvert + j];
      skinFilled = iskinImport->AddWeights(sd.skinNode, j, bn, wt);
      if (!skinFilled)
        skinErr = std::format(L"the skin modifier did not take the weights of vertex {}", j);
    }

    if (!skinFilled)
    {
      DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Warning, L"%s: '%s': %s, skinning skipped\r\n", scene_name.data(),
        sd.skinNode->GetName(), skinErr.c_str());
      DagorLogWindow::show();
      GetCOREInterface12()->DeleteModifier(*sd.skinNode, *skinMod);
    }
  }

  DebugPrint(_T("clean up\n"));
  cleanup();
  convertnew(ip, true);

  DebugPrint(_T("import ok\n"));

  DagImp::importedFiles.insert(ImportedFile(fname));

  return 1;
read_err:
  DebugPrint(_T("read error at %X of %s\n"), ftell(h), scene_name.data());
  fclose(h);
  cleanup();
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
  BEGIN_FUNCTION_MAP FN_10(fun_import, TYPE_BOOL, import_dag, TYPE_STRING, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL,
    TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL) END_FUNCTION_MAP

    BOOL import_dag(const TCHAR *fn, bool separateLayers, bool suppressPrompts, bool renameLayerCollision, bool useLegacyImport,
      bool importLod, bool importDestr, bool importDp, bool importDmg, bool importDm)
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
    DagImp::checked.dm = importDm;

    Autotoggle guard(DagImp::nonInteractive);
    BOOL result = GetCOREInterface()->ImportFromFile(fn, suppressPrompts);

    DagImp::reimportExisting = bkReimportExisting;
    DagImp::useLegacyImport = bkLegacy;
    DagImp::checked = bkChecked;
    DagImp::separateLayers = bkSeparate;

    return result;
  }
};

// clang-format off
static IDagorImportUtil dagorimputiliface(Interface_ID(0x20906172, 0x435c11e0),
  _T("dagorImport"), IDS_DAGOR_IMPORT_IFACE, NULL, FP_CORE, fun_import,
  _T("import"), -1, TYPE_BOOL, 0,
  10,
  _T("filename"), -1, TYPE_STRING,
  // f_keyArgDefault marks an optional keyArg param. The value
  // after that is its default value.
  // separateLayers answers the layered DAGs question of the legacy import (useLegacyImport) when
  // suppressPrompts is true or Max runs in quiet mode (3dsmaxbatch); otherwise the user is asked
  _T("separateLayers"), -1, TYPE_BOOL, f_keyArgDefault, true,
  _T("suppressPrompts"), -1, TYPE_BOOL, f_keyArgDefault, false,
  _T("renameLayerCollision"), -1, TYPE_BOOL, f_keyArgDefault, true,
  _T("useLegacyImport"), -1, TYPE_BOOL, f_keyArgDefault, true,
  _T("importLod"), -1, TYPE_BOOL, f_keyArgDefault, true,
  _T("importDestr"), -1, TYPE_BOOL, f_keyArgDefault, true,
  _T("importDp"), -1, TYPE_BOOL, f_keyArgDefault, true,
  _T("importDmg"), -1, TYPE_BOOL, f_keyArgDefault, true,
  _T("importDm"), -1, TYPE_BOOL, f_keyArgDefault, true,
  p_end
);
// clang-format on
