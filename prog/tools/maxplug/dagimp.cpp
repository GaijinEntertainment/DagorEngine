// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <algorithm>
#include <functional>
#include <regex>
#include <format>
#include <unordered_set>
#include <vector>
#include <memory>
#include <type_traits>
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

namespace
{
// vc16 (Max2023, Max2024) has no std::wformat_string, there the format is checked at run time
#if __cpp_lib_format >= 202207L
template <class... Args>
using WFormat = std::wformat_string<Args...>;

template <class... Args>
std::wstring wformat(WFormat<Args...> fmt, Args &&...args)
{
  return std::format(fmt, std::forward<Args>(args)...);
}
#else
template <class... Args>
using WFormat = std::type_identity_t<std::wstring_view>;

template <class... Args>
std::wstring wformat(WFormat<Args...> fmt, Args &&...args)
{
  return std::vformat(fmt, std::make_wformat_args(args...));
}
#endif

// The file does not hold what its blocks declare.
struct DagError
{
  std::wstring what;
};

template <class... Args>
[[noreturn]] void reject(WFormat<Args...> fmt, Args &&...args)
{
  throw DagError{wformat<Args...>(fmt, std::forward<Args>(args)...)};
}

// Every read is held to the current block, and every block to the one that holds it: a field the
// block does not have would otherwise be taken from whatever follows it, and a zero there would
// pass for a real count.
class DagReader
{
public:
  // opens a block on construction and skips to its end on destruction
  class Block
  {
  public:
    explicit Block(DagReader &r) : reader(r) { reader.begin(); }
    ~Block() { reader.end(); }
    Block(const Block &) = delete;
    Block &operator=(const Block &) = delete;

  private:
    DagReader &reader;
  };

  explicit DagReader(FILE *f) : file(f), pos(ftell(f))
  {
    // block lengths are declared by the file; measure it once so they can be held to it
    if (pos < 0)
      return;
    if (fseek(file, 0, SEEK_END) == 0)
      file_len = ftell(file);
    fseek(file, pos, SEEK_SET);
  }

  int type() const { return blocks.back().type; }

  // never negative: every read is held to the block and end() leaves the reader at its end
  int rest() const { return int(blocks.back().end - pos); }

  // how many records of item_size bytes are still left in the current block
  size_t restItems(size_t item_size) const { return size_t(rest()) / item_size; }

  void field(void *dest, size_t size, const wchar_t *what = L"a field")
  {
    if (!size)
      return;
    if (size > size_t(rest()))
      reject(L"{} is past the end of its block", what);
    raw(dest, size);
  }

  template <class T>
  T get(const wchar_t *what = L"a field")
  {
    static_assert(std::is_trivially_copyable_v<T>);
    T v{};
    field(&v, sizeof v, what);
    return v;
  }

  // the count comes out of the file, so it is held to the block before it sizes an allocation
  template <class T>
  std::vector<T> getArray(size_t count, std::wstring_view what)
  {
    static_assert(std::is_trivially_copyable_v<T>);
    if (count > restItems(sizeof(T)))
      reject(L"{} {} do not fit the block", count, what);
    std::vector<T> v(count);
    if (count)
      raw(v.data(), count * sizeof(T));
    return v;
  }

  std::wstring string(size_t len)
  {
    if (!len)
      return {};
    std::string s(len, '\0');
    field(s.data(), s.size(), L"a string");
    return strToWide(s);
  }

private:
  struct Range
  {
    long end;
    int type;
  };

  void raw(void *dest, size_t size)
  {
    if (fread(dest, size, 1, file) != 1)
      reject(L"the file could not be read");
    pos += long(size);
  }

  void begin()
  {
    const long outer_end = blocks.empty() ? file_len : blocks.back().end;
    int blk_len = 0, blk_type = 0;
    if (pos < 0 || outer_end - pos < long(sizeof blk_len + sizeof blk_type))
      reject(L"a block header runs past the block or file that holds it");
    raw(&blk_len, sizeof blk_len);
    raw(&blk_type, sizeof blk_type);
    if (blk_len < 4)
      reject(L"a block declares {} bytes, less than its own header", blk_len);
    blk_len -= 4;
    if (outer_end - pos < blk_len)
      reject(L"a block of {} bytes runs past the block or file that holds it", blk_len);
    blocks.push_back({pos + blk_len, blk_type});
  }

  void end()
  {
    pos = blocks.back().end;
    fseek(file, pos, SEEK_SET);
    blocks.pop_back();
  }

  FILE *file;
  long file_len = 0;
  long pos;
  std::vector<Range> blocks;
};
} // namespace

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
  bool makeHierLayer(const std::vector<fs::path> &fnames, ImpInterface *ii, Interface *ip, bool nomsg);
  bool makeHierLayer(const fs::path &fname, ImpInterface *ii, Interface *ip, bool nomsg);

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

template <class... Args>
static void log_import(DagorLogWindow::LogLevel level, INode *node, WFormat<Args...> fmt, Args &&...args)
{
  const std::wstring msg = wformat<Args...>(fmt, std::forward<Args>(args)...);
  if (node)
    DagorLogWindow::addToLog(level, L"%s: '%s': %s\r\n", scene_name.data(), node->GetName(), msg.c_str());
  else
    DagorLogWindow::addToLog(level, L"%s: %s\r\n", scene_name.data(), msg.c_str());
  DagorLogWindow::show();
}

static void flip_normals(Mesh &m)
{
  for (int i = 0; i < m.numFaces; ++i)
    m.FlipNormal(i);
}

// The two mesh blocks of DAG_NODE_OBJ differ only in the widths of their counts and indices.
namespace
{
struct NarrowMesh
{
  using Count = ushort;
  using Face = DagFace;
  using TFace = DagTFace;
};

struct BigMesh
{
  using Count = uint;
  using Face = DagBigFace;
  using TFace = DagBigTFace;
};
} // namespace

// Reads the map channel records that follow the faces of a mesh block. It is the block that bounds
// them: a record read past the block would import as plausible garbage. The format gives every
// channel one map face per mesh face, so that count comes from the mesh the records are read into.
template <class Fmt>
static void read_map_channels(DagReader &r, Mesh &m)
{
  const int nf = m.numFaces;
  const int numch = r.get<uchar>(L"the map channel count");
  for (int ch = 0; ch < numch; ++ch)
  {
    const uint ntv = r.get<typename Fmt::Count>(L"a map channel header");
    const int tcsz = r.get<uchar>(L"a map channel header");
    const int chid = r.get<uchar>(L"a map channel header");

    // the channel id and the coordinate count come out of the file too: a mesh has MAX_MESHMAPS
    // channels and a map vertex holds three coordinates
    if (chid >= MAX_MESHMAPS || tcsz < 1 || tcsz > 3)
      reject(L"map channel record {} declares id {} and {} coordinates per vertex", ch, chid, tcsz);
    const auto coords = r.getArray<float>(size_t(ntv) * tcsz, std::format(L"coordinates of map channel {}", chid));
    const auto tfaces = r.getArray<typename Fmt::TFace>(nf, std::format(L"faces of map channel {}", chid));

    m.setMapSupport(chid);
    m.setNumMapVerts(chid, ntv);
    Point3 *tv = m.mapVerts(chid);
    if (ntv && !tv)
      reject(L"cannot allocate {} vertices of map channel {}", ntv, chid);
    for (uint v = 0; v < ntv; ++v)
      for (int i = 0; i < 3; ++i)
        tv[v][i] = i < tcsz ? coords[size_t(v) * tcsz + i] : 0.f;

    TVFace *tf = m.mapFaces(chid);
    if (nf && !tf)
      reject(L"cannot allocate the faces of map channel {}", chid);
    for (int f = 0; f < nf; ++f)
    {
      const auto &t = tfaces[f];
      // the record fits the block, its indices still have to fit the channel they point into
      if (t.t[0] >= ntv || t.t[1] >= ntv || t.t[2] >= ntv)
        reject(L"face {} of map channel {} points outside its {} vertices", f, chid, ntv);
      tf[f].t[0] = t.t[0];
      tf[f].t[2] = t.t[1];
      tf[f].t[1] = t.t[2];
    }
  }
}

// Reads a DAG_OBJ_MESH or DAG_OBJ_BIGMESH payload. This is where the bound policy of a mesh block
// lives: every count comes out of the file, so each is held to what the block still holds before
// it is used as a length or an allocation size.
//
// A refusal can come after the mesh is allocated and part filled, so the object holding it has to be
// discarded, which NodeLoader does for any failed node.
template <class Fmt>
static void read_mesh(DagReader &r, Mesh &m)
{
  const uint nv = r.get<typename Fmt::Count>(L"the vertex count");
  if (nv > r.restItems(sizeof(Point3)))
    reject(L"{} vertices do not fit the block", nv);
  if (!m.setNumVerts(nv))
    reject(L"cannot allocate {} vertices", nv);
  r.field(m.verts, size_t(nv) * sizeof(Point3), L"the vertices");

  const auto faces = r.getArray<typename Fmt::Face>(r.get<typename Fmt::Count>(L"the face count"), L"faces");
  const int nf = int(faces.size());
  if (!m.setNumFaces(nf))
    reject(L"cannot allocate {} faces", nf);

  DebugPrint(_T("   faces: %d\n"), nf);

  for (int i = 0; i < nf; ++i)
  {
    const auto &f = faces[i];
    // the record fits the block, its indices still have to fit the mesh they point into
    if (f.v[0] >= nv || f.v[1] >= nv || f.v[2] >= nv)
      reject(L"face {} points outside the {} vertices of the mesh", i, nv);
    m.faces[i].v[0] = f.v[0];
    m.faces[i].v[2] = f.v[1];
    m.faces[i].v[1] = f.v[2];
    m.faces[i].setMatID(f.mat);
    m.faces[i].smGroup = f.smgr;
    m.faces[i].flags |= EDGE_ALL;
  }

  read_map_channels<Fmt>(r, m);
}

static Class_ID light_class(int dag_light_type)
{
  switch (dag_light_type)
  {
    case DAG_LIGHT_SPOT: return Class_ID(FSPOT_LIGHT_CLASS_ID, 0);
    case DAG_LIGHT_DIR: return Class_ID(DIR_LIGHT_CLASS_ID, 0);
    default: return Class_ID(OMNI_LIGHT_CLASS_ID, 0);
  }
}

static constexpr int MAX_NODE_DEPTH = 1000;

namespace
{
struct ImportCtx
{
  ImpInterface *ii;
  Interface *ip;
  std::vector<SkinData> skin_data;
  Tab<NodeId> node_id;
  INode *failed = nullptr; // the innermost node that did not load
  const float master_scale = get_master_scale();
  const bool need_rescale = fabs(master_scale - 1.f) > 1e-4f;
};

// Reads the DAG_NODE block the reader stands in and builds the node of it. A node that fails to
// load is left as a dummy under its parent and its object is discarded, so that whoever catches
// the error finds a consistent scene.
class NodeLoader
{
public:
  NodeLoader(ImportCtx &import_ctx, DagReader &reader, INode *parent_node, int node_depth) :
    ctx(import_ctx), r(reader), parent(parent_node), depth(node_depth)
  {
    if (parent)
    {
      in = ctx.ii->CreateNode();
      assert(in);
      ctx.ii->AddNodeToScene(in);
      n = in->GetINode();
    }
    else
      n = ctx.ip->GetRootNode();
    assert(n);
  }

  NodeLoader(const NodeLoader &) = delete;
  NodeLoader &operator=(const NodeLoader &) = delete;

  ~NodeLoader()
  {
    if (done)
      return;
    if (in && !ctx.failed)
      ctx.failed = n;
    if (obj)
      obj->DeleteMe();
    if (in)
      attach(makeDummy());
  }

  void load()
  {
    readBlocks();
    if (in)
    {
      if (!obj)
        obj = makeDummy();
      attach(obj);
    }
    if (mtl)
      n->SetMtl(mtl);
    done = true;
  }

private:
  Object *makeDummy() const
  {
    Object *dummy = (Object *)ctx.ii->Create(GEOMOBJECT_CLASS_ID, Dummy_CID);
    assert(dummy);
    return dummy;
  }

  void attach(Object *o)
  {
    in->Reference(o);
    parent->AttachChild(n, 0);
  }

  void setObject(Object *o)
  {
    if (obj)
      obj->DeleteMe();
    obj = o;
  }

  TriObject *triObject() const
  {
    return obj && obj->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID, 0)) ? static_cast<TriObject *>(obj) : nullptr;
  }

  bool mirrored() const { return n->GetNodeTM(0).Parity(); }

  void readBlocks()
  {
    while (r.rest() > 0)
    {
      DagReader::Block block(r);
      DebugPrint(_T("load_node %d\n"), r.type());

      switch (r.type())
      {
        case DAG_NODE_DATA: readData(); break;
        case DAG_NODE_TM:
          if (in)
            readTransform();
          break;
        case DAG_NODE_SCRIPT:
          if (in)
            readScript();
          break;
        case DAG_NODE_MATER:
          if (in)
            readMaterials();
          break;
        case DAG_NODE_OBJ:
          if (in)
            readObject();
          break;
        case DAG_NODE_CHILDREN: readChildren(); break;
      }
    }
  }

  void readData()
  {
    const auto d = r.get<DagNodeData>();
    n->SetRenderable(d.flg & DAG_NF_RENDERABLE ? 1 : 0);
    n->SetCastShadows(d.flg & DAG_NF_CASTSHADOW ? 1 : 0);
    n->SetRcvShadows(d.flg & DAG_NF_RCVSHADOW ? 1 : 0);
    NodeId id(d.id, n);
    ctx.node_id.Append(1, &id);
    flags = d.flg;

    const int name_len = r.rest();
    if (name_len > 0 && in)
    {
      const std::wstring name = r.string(name_len);
      in->SetName(name.data(), DagImp::useLegacyImport);
      DebugPrint(_T("node <%s>\n"), name.data());
    }
  }

  void readTransform()
  {
    Matrix3 tm;
    r.field(tm.GetAddr(), 4 * sizeof(MRow), L"the transform");
    tm.SetNotIdent();
    if (ctx.need_rescale)
      tm.SetTrans(tm.GetTrans() * ctx.master_scale);
    if (parent->IsRootNode())
      adj_wtm(tm);
    in->SetTransform(0, tm);
  }

  void readScript()
  {
    const int len = r.rest();
    if (len <= 0)
      return;
    const std::wstring trimmed = trim_params(r.string(len));
    n->SetUserPropBuffer(trimmed.data());
  }

  Mtl *material(ushort id)
  {
    if (id >= mat.size())
      return nullptr;
    if (!mat[id].mtl)
      make_mtl(mat[id], id, DagorMat2_CID);
    return mat[id].mtl;
  }

  void readMaterials()
  {
    const auto ids = r.getArray<ushort>(r.restItems(sizeof(ushort)), L"material ids");
    if (ids.size() > 1)
    {
      MultiMtl *mm = NewDefaultMultiMtl();
      assert(mm);
      multi_mat.push_back(mm);
      mm->SetNumSubMtls(int(ids.size()));
      for (int i = 0; i < int(ids.size()); ++i)
        if (Mtl *sub = material(ids[i]))
          mm->SetSubMtl(i, sub);
      mm->SetName(n->GetName());
      mtl = mm;
    }
    else if (ids.size() == 1)
    {
      if (Mtl *single = material(ids[0]))
        mtl = single;
    }
  }

  void readObject()
  {
    while (r.rest() > 0)
    {
      DagReader::Block block(r);
      switch (r.type())
      {
        // These two blocks are the object, unlike FACEFLG, NORMALS and BONES, which annotate one
        // that already exists and can each be dropped on their own. A count that does not fit is
        // found part way through building the mesh, so dropping the block would leave the node
        // holding half a mesh or none, and the scene would come back quietly missing geometry
        // rather than missing a detail. A mesh that does not fit its block fails the import instead.
        case DAG_OBJ_BIGMESH: readMesh<BigMesh>(); break;
        case DAG_OBJ_MESH: readMesh<NarrowMesh>(); break;
        case DAG_OBJ_BONES: readBones(); break;
        case DAG_OBJ_SPLINES: readSplines(); break;
        case DAG_OBJ_LIGHT: readLight(); break;
        case DAG_OBJ_FACEFLG:
          if (TriObject *tri = triObject())
            readFaceFlags(*tri);
          break;
        case DAG_OBJ_NORMALS:
          if (TriObject *tri = triObject())
            readNormals(*tri);
          break;
      }
    }
  }

  template <class Fmt>
  void readMesh()
  {
    TriObject *tri = CreateNewTriObject();
    if (!tri)
      reject(L"cannot create the mesh object");
    setObject(tri);

    Mesh &m = tri->mesh;
    read_mesh<Fmt>(r, m);
    if (ctx.need_rescale)
      for (int i = 0; i < m.numVerts; ++i)
        m.verts[i] *= ctx.master_scale;
    if (mirrored())
      flip_normals(m);
  }

  // A block that does not add up costs its own skin and not the whole scene: the block is skipped
  // either way, the way the FACEFLG and NORMALS blocks already behave. A block with no bones carries
  // no skin and no weight matrix at all.
  void readBones()
  {
    if (r.rest() < int(sizeof(ushort)))
    {
      log_import(DagorLogWindow::LogLevel::Warning, n, L"the bone block has no bone count, skinning skipped");
      return;
    }
    const ushort numb = r.get<ushort>();
    if (!numb)
      return;

    // the bones and the vertex count behind them have to be measured against what the block still holds
    if (r.rest() < int(numb * sizeof(DagBone)) + 4)
    {
      log_import(DagorLogWindow::LogLevel::Warning, n, L"the table of {} bones does not fit its block, skinning skipped", numb);
      return;
    }

    SkinData &sd = ctx.skin_data.emplace_back();
    sd.skinNode = n;
    sd.numb = numb;
    sd.bones.SetCount(numb);
    r.field(sd.bones.data(), numb * sizeof(DagBone), L"the bones");
    sd.numvert = r.get<int>(L"the skin vertex count");

    // the numb by numvert matrix of floats has to fit in what is left of the block too, which also
    // keeps numvert * numb inside an int
    const int maxvert = r.rest() / int(numb * sizeof(float));
    if (sd.numvert < 0 || sd.numvert > maxvert)
    {
      log_import(DagorLogWindow::LogLevel::Warning, n, L"the weight matrix of {} vertices does not fit its block, skinning skipped",
        sd.numvert);
      ctx.skin_data.pop_back();
      return;
    }
    if (sd.numvert)
    {
      sd.wt.SetCount(sd.numvert * numb);
      r.field(sd.wt.data(), sd.wt.Count() * sizeof(float), L"the skin weights");
    }
  }

  void readSplines()
  {
    SplineShape *shape = new SplineShape;
    setObject(shape);
    BezierShape &shp = shape->shape;

    for (int numspl = r.get<int>(L"the spline count"); numspl > 0; --numspl)
    {
      Spline3D &s = *shp.NewSpline();
      const char flg = r.get<char>(L"a spline header");
      for (int numk = r.get<int>(L"a spline header"); numk > 0; --numk)
      {
        [[maybe_unused]] const auto ktype = r.get<char>(L"a spline knot");
        Point3 i = r.get<Point3>(L"a spline knot");
        Point3 p = r.get<Point3>(L"a spline knot");
        Point3 o = r.get<Point3>(L"a spline knot");
        if (ctx.need_rescale)
        {
          i *= ctx.master_scale;
          p *= ctx.master_scale;
          o *= ctx.master_scale;
        }
        s.AddKnot(SplineKnot(KTYPE_BEZIER_CORNER, LTYPE_CURVE, p, i, o));
      }
      s.SetClosed(flg & DAG_SPLINE_CLOSED ? 1 : 0);
      s.ComputeBezPoints();
    }
    shp.UpdateSels();
    shp.InvalidateGeomCache();
    shp.InvalidateCapCache();
  }

  void readLight()
  {
    const auto dl = r.get<DagLight>(L"a light");
    const Color c(dl.r, dl.g, dl.b);
    DagLight2 dl2;
    if (r.rest() == int(sizeof(DagLight2)))
      dl2 = r.get<DagLight2>();
    else
    {
      dl2.mult = MaxVal(c);
      dl2.type = DAG_LIGHT_OMNI;
      dl2.hotspot = 0;
      dl2.falloff = 0;
    }

    const TimeValue time = ctx.ip->GetTime();
    auto *lt = static_cast<GenLight *>(ctx.ip->CreateInstance(LIGHT_CLASS_ID, light_class(dl2.type)));
    if (!lt)
      reject(L"cannot create the light object");
    float mc = dl2.mult;
    lt->SetIntensity(time, mc);
    if (mc == 0)
      mc = 1;
    lt->SetRGBColor(time, Point3((float *)(c / mc)));
    if (dl2.type != DAG_LIGHT_OMNI)
    {
      lt->SetHotspot(time, dl2.hotspot);
      lt->SetFallsize(time, dl2.falloff);
    }
    lt->SetAtten(time, ATTEN_START, dl.range);
    lt->SetAtten(time, ATTEN_END, dl.range);
    lt->SetUseAtten(TRUE);
    lt->SetDecayType(dl.decay);
    lt->SetDecayRadius(time, dl.drad);
    lt->SetShadow(flags & DAG_NF_CASTSHADOW ? 1 : 0);
    lt->SetUseLight(TRUE);
    lt->Enable(TRUE);
    setObject(lt);
  }

  void readFaceFlags(TriObject &tri)
  {
    Mesh &m = tri.mesh;
    if (m.numFaces != r.rest())
      return;

    // a mirrored mesh was flipped on load, and FlipNormal() swaps the first two vertices of a face
    // and with them its edges B and C
    const bool flip = mirrored();
    const DWORD edge1 = flip ? EDGE_C : EDGE_B;
    const DWORD edge0 = flip ? EDGE_B : EDGE_C;
    const auto ef = r.getArray<char>(m.numFaces, L"face flags");
    for (int i = 0; i < m.numFaces; ++i)
    {
      Face &f = m.faces[i];
      f.flags &= ~(EDGE_ALL | FACE_HIDDEN);
      if (ef[i] & DAG_FACEFLG_EDGE2)
        f.flags |= EDGE_A;
      if (ef[i] & DAG_FACEFLG_EDGE1)
        f.flags |= edge1;
      if (ef[i] & DAG_FACEFLG_EDGE0)
        f.flags |= edge0;
      if (ef[i] & DAG_FACEFLG_HIDDEN)
        f.flags |= FACE_HIDDEN;
    }
  }

  void readNormals(TriObject &tri)
  {
    Mesh &m = tri.mesh;
    if (!m.GetSpecifiedNormals())
      m.SpecifyNormals();
    MeshNormalSpec *normalSpec = m.GetSpecifiedNormals();
    if (!normalSpec || r.rest() < int(sizeof(int)))
      return;

    const int numNormals = r.get<int>();
    if (numNormals <= 0 || r.rest() != int64_t(numNormals) * 12 + int64_t(m.numFaces) * 12)
      return;

    const auto normals = r.getArray<Point3>(numNormals, L"normals");
    const auto indices = r.getArray<int>(size_t(m.numFaces) * 3, L"normal indices");

    const auto bad = std::ranges::find_if(indices, [numNormals](int id) { return id < 0 || id >= numNormals; });
    if (bad != indices.end())
    {
      log_import(DagorLogWindow::LogLevel::Warning, n, L"face {} points outside the {} normals, normals skipped",
        int(bad - indices.begin()) / 3, numNormals);
      return;
    }

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

  void readChildren()
  {
    while (r.rest() > 0)
    {
      DagReader::Block block(r);
      if (r.type() == DAG_NODE)
      {
        if (depth >= MAX_NODE_DEPTH)
          reject(L"the node hierarchy is deeper than {} levels", MAX_NODE_DEPTH);
        NodeLoader(ctx, r, n, depth + 1).load();
      }
      else if (r.type() == DAG_MATER)
        reject(L"a material block among the children of a node");
    }
  }

  ImportCtx &ctx;
  DagReader &r;
  INode *parent;
  const int depth;
  ImpNode *in = nullptr;
  INode *n = nullptr;
  Object *obj = nullptr;
  Mtl *mtl = nullptr;
  int flags = 0;
  bool done = false;
};
} // namespace

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

bool DagImp::makeHierLayer(const std::vector<fs::path> &fnames, ImpInterface *ii, Interface *ip, bool nomsg)
{
  static const int ext_length = 6; // ".lodNN"

  ILayerManager *manager = GetCOREInterface13()->GetLayerManager();

  bool imported = false;
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

    if (doImportOne(fn.c_str(), ii, ip, nomsg))
      imported = true;
    else
      logImportError(fn, layerName);
  }
  return imported;
}

bool DagImp::makeHierLayer(const fs::path &fname, ImpInterface *ii, Interface *ip, bool nomsg)
{
  TSTR layerName = fname.stem().c_str();

  ResolvedLayer resolved = resolveLayerNameCollision(layerName);
  layerName = resolved.name;

  if (doImportOne(fname.c_str(), ii, ip, nomsg))
    return true;

  logImportError(fname, layerName);
  return false;
}

static std::vector<fs::path> drop_namesakes(std::vector<fs::path> files)
{
  std::ranges::stable_sort(files, {}, [](const fs::path &f) { return std::distance(f.begin(), f.end()); });

  std::vector<fs::path> kept;
  kept.reserve(files.size());
  for (fs::path &f : files)
  {
    auto it = std::ranges::find_if(kept, [&f](const fs::path &k) { return iequal(k.filename().native(), f.filename().native()); });
    if (it == kept.end())
    {
      kept.push_back(std::move(f));
      continue;
    }
    DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Warning, L"ignore duplicated \"%s\" (already have \"%s\")\r\n", f.c_str(),
      it->c_str());
    DagorLogWindow::show();
  }
  return kept;
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
      return IMPEXP_CANCEL;
  }
  if (DagImp::useLegacyImport)
    return doLegacyImport(fname.c_str(), ii, ip, nomsg);

  if (exact_match)
  {
    std::error_code ec;
    if (!fs::is_regular_file(fname, ec))
      return IMPEXP_FAIL;
    return makeHierLayer(fname, ii, ip, nomsg) ? IMPEXP_SUCCESS : IMPEXP_FAIL;
  }

  // get proper list of files based on actual user's choice
  rex = filenameToRegex(fname.filename(), checked.dp, checked.lod, checked.destr, checked.dmg, checked.dm, basename_re, exact_match);
  files = files_include_re(glob(dirPath, searchInSubfolders), rex);
  if (reimportExisting)
    files = drop_namesakes(std::move(files));

  std::vector<fs::path> layer_files;
  layer_files.reserve(32);
  bool imported = false;

  if (checked.lod)
    layer_files = files_include_re(files, lod_re);
  else
  {
    layer_files.clear();
    layer_files.push_back(fname);
  }
  if (!layer_files.empty())
    imported |= makeHierLayer(layer_files, ii, ip, nomsg);

  layer_files = files_include_re(files, dm_re);
  if (!layer_files.empty())
    imported |= makeHierLayer(layer_files.front(), ii, ip, nomsg);

  layer_files = files_include_re(files, dmg_re);
  if (!layer_files.empty())
    imported |= makeHierLayer(layer_files, ii, ip, nomsg);

  layer_files = files_include_re(files, destr_re);
  if (!layer_files.empty())
    imported |= makeHierLayer(layer_files, ii, ip, nomsg);

  for (int i = 0; i < 100; ++i)
  {
    std::wstring re = std::format(L"^{}_dp_{:02}\\.lod\\d\\d\\.dag$", basename_re, i);
    layer_files = files_include_re(files, re);
    if (!layer_files.empty())
      imported |= makeHierLayer(layer_files, ii, ip, nomsg);

    re = std::format(L"^{}_dp_{:02}_dmg\\.lod\\d\\d\\.dag$", basename_re, i);
    layer_files = files_include_re(files, re);
    if (!layer_files.empty())
      imported |= makeHierLayer(layer_files, ii, ip, nomsg);

    re = std::format(L"^{}_dp_{:02}_destr\\.lod\\d\\d\\.dag$", basename_re, i);
    layer_files = files_include_re(files, re);
    if (!layer_files.empty())
      imported |= makeHierLayer(layer_files, ii, ip, nomsg);
  }

  return imported ? IMPEXP_SUCCESS : IMPEXP_FAIL;
}


static void clear_scene_layers()
{
  ILayerManagerEx *manager = (ILayerManagerEx *)GetCOREInterface13()->GetLayerManager();
  ILayer *root = manager->GetRootLayer();
  for (int i = 0; i < manager->GetLayerCount();)
  {
    ILayer *layer = manager->GetLayer(i);
    ILayer *parent = layer->GetParentLayer();
    if (layer != root && (!parent || parent == root) && manager->DeleteLayerHierarchy(layer, true) > 0)
      i = 0;
    else
      ++i;
  }
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

  clear_scene_layers();
  ILayerManager *manager = GetCOREInterface13()->GetLayerManager();
  for (size_t i = 0; i < fnames.size(); i += 2)
  {
    TSTR layer_nm(fnames[i + 1].data());
    manager->CreateLayer(layer_nm);
    manager->SetCurrentLayer(layer_nm);
    if (!doImportOne(fnames[i].data(), ii, ip, nomsg))
      return 0;
  }
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

namespace
{
class CurrentLayerKeeper
{
  ILayerManagerEx *manager = (ILayerManagerEx *)GetCOREInterface13()->GetLayerManager();
  ILayer *layer = manager->GetCurrentLayer();
  AnimHandle handle = layer ? Animatable::GetHandleByAnim(layer) : 0;

public:
  CurrentLayerKeeper() = default;
  ~CurrentLayerKeeper()
  {
    if (!layer)
      return;
    if (Animatable::GetAnimByHandle(handle) != layer || manager->GetLayer(layer->GetName()) != layer)
      layer = manager->GetRootLayer();
    if (layer)
      manager->SetCurrentLayer(layer);
  }
  CurrentLayerKeeper(const CurrentLayerKeeper &) = delete;
  CurrentLayerKeeper &operator=(const CurrentLayerKeeper &) = delete;
};
} // namespace

//
// Main entry point for GetCOREInterface()->ImportFromFile()
//
int DagImp::DoImport(const TCHAR *fname, ImpInterface *ii, Interface *ip, BOOL nomsg)
{
  nomsg = nomsg || ip->GetQuietMode();
  CurrentLayerKeeper currentLayerKeeper;

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


static void read_textures(DagReader &r)
{
  tex.resize(r.get<ushort>(L"the texture count"));
  for (std::wstring &name : tex)
    name = r.string(r.get<uchar>(L"a texture name length"));
}

static void read_material(DagReader &r)
{
  ImpMat m;
  m.name = r.string(r.get<uchar>(L"a material name length"));
  m.m = r.get<DagMater>(L"a material");
  m.clsname = r.string(r.get<uchar>(L"a material class name length"));
  m.script = r.string(r.rest());
  mat.push_back(std::move(m));
}

// False if the file is not a DAG at all. Throws DagError if it is one that cannot be read.
static bool read_dag(FILE *f, ImportCtx &ctx)
{
  uint id = 0;
  if (fread(&id, sizeof id, 1, f) != 1)
    reject(L"the file could not be read");
  if (id != DAG_ID)
    return false;

  DagReader r(f);
  DagReader::Block file_block(r);
  if (r.type() != DAG_ID)
    return false;

  while (r.rest() > 0)
  {
    DagReader::Block block(r);
    DebugPrint(_T("imp %d\n"), r.type());
    if (r.type() == DAG_END)
      break;

    switch (r.type())
    {
      case DAG_TEXTURES: read_textures(r); break;
      case DAG_MATER: read_material(r); break;
      case DAG_NODE: NodeLoader(ctx, r, nullptr, 0).load(); break;
    }
  }
  return true;
}

static INode *find_node(const Tab<NodeId> &node_id, int id)
{
  for (int i = 0; i < node_id.Count(); ++i)
    if (node_id[i].id == id)
      return node_id[i].node;
  return nullptr;
}

static void apply_skin(const SkinData &sd, const Tab<NodeId> &node_id)
{
  using LogLevel = DagorLogWindow::LogLevel;
  INode *node = sd.skinNode;

  // dagfmt.h: "numv should be equal to number of mesh vertexes", and AddWeights() below indexes
  // the node's object with it. The object cannot be judged while the bone block is read: a later
  // block of the same node replaces it, and in->Reference() keeps the last one.
  Object *nodeObj = node->GetObjectRef();
  const int nodeVerts = (nodeObj && nodeObj->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID, 0))) ? ((TriObject *)nodeObj)->mesh.numVerts : 0;
  if (sd.numvert != nodeVerts)
  {
    log_import(LogLevel::Warning, node, L"skin covers {} vertices but the object has {}, skinning skipped", sd.numvert, nodeVerts);
    return;
  }

  // a bone that is not in the scene takes its share of every vertex weight with it, and a skin
  // that deforms wrong is worse than none, so the whole skin goes rather than that one bone
  Tab<INode *> bn;
  bn.SetCount(sd.numb);
  for (int j = 0; j < sd.numb; ++j)
  {
    bn[j] = find_node(node_id, sd.bones[j].id);
    if (!bn[j])
    {
      log_import(LogLevel::Warning, node, L"skin refers to missing bone {:04X}, skinning skipped", sd.bones[j].id);
      return;
    }
  }

  // the interface is taken before the modifier is attached, so a refusal leaves nothing on the node
  Modifier *skinMod = (Modifier *)CreateInstance(OSM_CLASS_ID, SKIN_CLASSID);
  ISkinImportData *iskinImport = skinMod ? (ISkinImportData *)skinMod->GetInterface(I_SKINIMPORTDATA) : NULL;
  if (!iskinImport)
  {
    log_import(LogLevel::Warning, node, L"the skin modifier is unavailable, skinning skipped");
    if (skinMod)
      skinMod->MaybeAutoDelete();
    return;
  }
  if (GetCOREInterface12()->AddModifier(*node, *skinMod) != Interface7::kRES_SUCCESS)
  {
    log_import(LogLevel::Warning, node, L"the skin modifier does not apply to the object, skinning skipped");
    skinMod->MaybeAutoDelete();
    return;
  }

  // AddWeights() indexes the bone table, so one bone the modifier refuses reopens the hole the
  // resolution pass above closed, and the skin has to come off the node again
  std::wstring skinErr;
  for (int j = 0; j < sd.numb && skinErr.empty(); j++)
    if (!iskinImport->AddBoneEx(bn[j], j + 1 == sd.numb))
      skinErr = std::format(L"the skin modifier did not take bone {:04X}", sd.bones[j].id);

  Tab<float> wt;
  wt.SetCount(sd.numb);
  for (int j = 0; j < sd.numvert && skinErr.empty(); j++)
  {
    for (int b = 0; b < sd.numb; b++)
      wt[b] = sd.wt[b * sd.numvert + j];
    if (!iskinImport->AddWeights(node, j, bn, wt))
      skinErr = std::format(L"the skin modifier did not take the weights of vertex {}", j);
  }

  if (!skinErr.empty())
  {
    log_import(LogLevel::Warning, node, L"{}, skinning skipped", skinErr);
    GetCOREInterface12()->DeleteModifier(*node, *skinMod);
  }
}

int DagImp::doImportOne(const TCHAR *fname, ImpInterface *ii, Interface *ip, BOOL nomsg)
{
  DebugPrint(_T("import...\n"));

  cleanup();
  scene_name = fs::path(fname).stem().c_str();

  const auto prompt = [&](int string_id) {
    if (!nomsg)
      ip->DisplayTempPrompt(GetString(string_id), ERRMSG_DELAY);
    return 0;
  };

  std::unique_ptr<FILE, int (*)(FILE *)> file(_tfopen(fname, _T("rb")), fclose);
  if (!file)
    return prompt(IDS_FILE_OPEN_ERR);

  ImportCtx ctx{ii, ip};
  bool is_dag = false;
  try
  {
    is_dag = read_dag(file.get(), ctx);
  }
  catch (const DagError &e)
  {
    log_import(DagorLogWindow::LogLevel::Error, ctx.failed, L"{}, import aborted", e.what);
    cleanup();
    return prompt(IDS_FILE_READ_ERR);
  }
  file.reset();
  if (!is_dag)
    return prompt(IDS_INVALID_DAGFILE);

  for (const SkinData &sd : ctx.skin_data)
    apply_skin(sd, ctx.node_id);

  DebugPrint(_T("clean up\n"));
  cleanup();
  convertnew(ip, true);

  DebugPrint(_T("import ok\n"));

  DagImp::importedFiles.insert(ImportedFile(fname));

  return 1;
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
