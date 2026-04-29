// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sstream>
#include <iomanip>

#include "common.h"
#include "layout.h"
#include "resource.h"

extern TCHAR *GetString(int id);

HWND add_rollup_page(Interface *ip, int resource_id, DLGPROC proc, const TCHAR *title, LPARAM param, DWORD flags)
{
  const WCHAR *tpl = MAKEINTRESOURCE(resource_id);
  HWND hw = ip->AddRollupPage(hInstance, tpl, proc, title, 0, flags);
  // attach_layout_to_rollup(hw, tpl);
  return hw;
}

void delete_rollup_page(Interface *ip, HWND *hw)
{
  assert(hw);

  if (!*hw)
    return;

  ip->DeleteRollupPage(*hw);
  *hw = 0;
}

float get_master_scale()
{
  float k = 1.0f;
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  k = (float)GetSystemUnitScale(UNITS_METERS);
#else
  k = (float)GetMasterScale(UNITS_METERS);
#endif
  return k ? (1.f / k) : 1.f;
}


TSTR extract_directory(const TSTR &path)
{
  TSTR dir;
  SplitFilename(path, &dir, 0, 0);
  return dir;
}

TSTR extract_filename(const TSTR &path)
{
  TSTR filename;
  SplitFilename(path, 0, &filename, 0);
  return filename;
}

TSTR extract_basename(const TSTR &path)
{
  TSTR filename, ext;
  SplitFilename(path, 0, &filename, &ext);
  filename.Append(ext);
  return filename;
}

bool is_dag_file(const TSTR &path)
{
  TSTR ext;
  SplitFilename(path, 0, 0, &ext);
  ext.toLower();
  return ext == _T(".dag");
}

static bool do_glob(std::vector<TSTR> &files, const TSTR &dir, bool recursive)
{
  WIN32_FIND_DATA file;
  HANDLE findhandle = FindFirstFile((dir + _T("\\*")).data(), &file);
  if (findhandle == INVALID_HANDLE_VALUE)
    return false;

  do
  {
    TSTR fullpath = dir + _T("\\") + file.cFileName;
    if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      if (recursive && _tcscmp(file.cFileName, _T(".")) != 0 && _tcscmp(file.cFileName, _T("..")) != 0)
        do_glob(files, fullpath, recursive);
    }
    else
    {
      if (is_dag_file(file.cFileName))
        files.push_back(fullpath);
    }

  } while (FindNextFile(findhandle, &file) != 0);

  FindClose(findhandle);
  return true;
}

std::vector<TSTR> glob(const TSTR &dir, bool recursive)
{
  std::vector<TSTR> files;
  files.reserve(32);
  do_glob(files, dir, recursive);
  return files;
}


M_STD_STRING strToWide(const char *sz)
{
#ifdef _UNICODE
  size_t n = strlen(sz) + 1;
  TCHAR *sw = new TCHAR[n];
  mbstowcs(sw, sz, n);

  M_STD_STRING res(sw);
  delete[] sw;
#else
  M_STD_STRING res(sz);
#endif
  return res;
}


std::string wideToStr(const TCHAR *sw)
{
#ifdef _UNICODE
  size_t n = _tcslen(sw) * 3 + 1;
  char *sz = new char[n];
  _locale_t locale = _create_locale(LC_ALL, "ru-RU");
  _wcstombs_l(sz, sw, n, locale);
  _free_locale(locale);
  std::string res(sz);
  delete[] sz;
#else
  std::string res(sw);
#endif
  return res;
}


TSTR drop_quotation_marks(const TSTR &s)
{
  if (s.Length() >= 2 && s[0] == _T('"') && s[s.Length() - 1] == _T('"'))
    return s.Substr(1, s.Length() - 2);

  return s;
}


void update_path_edit_control(HWND hw, int id, const TSTR &path)
{
  SetDlgItemText(hw, id, path.data());
  SendMessage(GetDlgItem(hw, id), EM_SETSEL, (WPARAM)path.Length(), (LPARAM)path.Length());
}


int get_save_filename(HWND owner, const TCHAR *title, FilterList &filter, const TCHAR *def_ext, TSTR &exp_fname,
  bool init_with_previous)
{
  static TCHAR fname[MAX_PATH];
  if (init_with_previous)
    _tcscpy_s(fname, MAX_PATH, exp_fname.data());

  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(ofn));

  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = owner;
  ofn.lpstrFilter = filter;
  ofn.lpstrFile = fname;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrInitialDir = _T("");
  ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
  ofn.lpstrDefExt = def_ext;
  ofn.lpstrTitle = title;

tryAgain:
  if (!init_with_previous)
    memset(fname, 0, sizeof(fname));
  if (GetSaveFileName(&ofn))
  {
    if (DoesFileExist(fname))
    {
      TSTR buf;
      buf.printf(GetString(IDS_FILE_EXISTS), fname);
      if (IDYES != MessageBox(owner, buf, title, MB_YESNO | MB_ICONQUESTION))
        goto tryAgain;
    }
    if (exp_fname != fname)
    {
      exp_fname = fname;
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


bool get_open_filename(HWND owner, const TCHAR *title, FilterList &filter, const TCHAR *def_ext, TSTR &imp_fname)
{
  static TCHAR path[MAX_PATH];

  _tcscpy_s(path, MAX_PATH, imp_fname.data());

  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(ofn));

  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = owner;
  ofn.lpstrFilter = filter;
  ofn.lpstrFile = path;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrInitialDir = _T("");
  ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
  ofn.lpstrDefExt = def_ext;
  ofn.lpstrTitle = title;

  bool result = GetOpenFileName(&ofn);
  if (result)
    imp_fname = path;

  return result;
}

bool get_open_filename(HWND owner, TSTR &imp_fname)
{
  FilterList fl;
  fl.Append(GetString(IDS_SCENE_FILES));
  fl.Append(L"*.dag");
  return get_open_filename(owner, GetString(IDS_OPEN_SCENE_TITLE), fl, L"dag", imp_fname);
}

std::wstring get_cfg_filename(const TCHAR *blk)
{
  static TCHAR filename[MAX_PATH];
  static TCHAR drive[_MAX_DRIVE];
  static TCHAR dir[_MAX_DIR];
  static TCHAR fName[_MAX_FNAME];
  static TCHAR ext[_MAX_EXT];

  GetModuleFileName(hInstance, filename, MAX_PATH);
  _tsplitpath(filename, drive, dir, fName, ext);
  _tcscpy(filename, drive);
  _tcscat(filename, dir);
  _tcscat(filename, _T("../")); // search one dir up first
  _tcscat(filename, blk);
  if (FILE *fp = _tfopen(filename, _T("rb")))
    fclose(fp);
  else
  {
    // if not found then get in module dir
    _tcscpy(filename, drive);
    _tcscat(filename, dir);
    _tcscat(filename, blk);
  }
  return filename;
}


std::vector<std::wstring> split(const std::wstring &text, const wchar_t delim)
{
  std::vector<std::wstring> tokens;
  std::wistringstream iss(text + delim);
  std::wstring tok;
  while (std::getline(iss, tok, delim))
    tokens.push_back(tok);
  return tokens;
}

std::wstring replace_all(std::wstring str, const std::wstring &from, const std::wstring &to)
{
  if (from.empty())
    return str;
  for (size_t pos = 0; (pos = str.find(from, pos)) != std::wstring::npos; pos += to.size())
    str.replace(pos, from.size(), to);
  return str;
}

void trim(std::wstring &str)
{
  str.erase(str.find_last_not_of(L' ') + 1);
  str.erase(0, str.find_first_not_of(L' '));
}

bool isProxymatName(const TCHAR *mat_name)
{
#define PROXYMAT (L":proxymat")
  static const size_t proxymat_len = sizeof(PROXYMAT) / sizeof(TCHAR) - 1;

  if (!mat_name || !*mat_name)
    return false;

  size_t len = _tcslen(mat_name);

  if (len < proxymat_len)
    return false;

  return !_tcscmp(mat_name + len - proxymat_len, PROXYMAT);
#undef PROXYMAT
}

std::wstring simplifyRN(const std::wstring &from)
{
  std::wstring s = from;
  size_t pos = 0;
  while ((pos = s.find(_T("\r\n"), pos)) != std::wstring::npos)
    s.replace(pos, 2, _T("\n"));
  return s;
}

std::wstring trim_params(const std::wstring &from)
{
  std::wstring s;
  std::vector<std::wstring> lines = split(simplifyRN(from), L'\n');

  for (const std::wstring &line : lines)
  {
    std::vector<std::wstring> tokens = split(line, L'=');
    if (tokens.size() != 2)
      continue;

    trim(tokens[0]);
    trim(tokens[1]);

    s += tokens[0];
    s += L'=';
    s += tokens[1];
    s += L"\r\n";
  }

  return s;
}

std::string escape_string(const std::string &input)
{
  std::ostringstream escaped;
  for (unsigned char ch : input)
    switch (ch)
    {
      case '\\': escaped << "\\\\"; break;
      case '\"': escaped << "\\\""; break;
      case '\'': escaped << "\\\'"; break;
      case '\n': escaped << "\\n"; break;
      case '\r': escaped << "\\r"; break;
      case '\t': escaped << "\\t"; break;
      case '\b': escaped << "\\b"; break;
      case '\f': escaped << "\\f"; break;
      case '\v': escaped << "\\v"; break;
      case '\a': escaped << "\\a"; break;
      case '\?': escaped << "\\?"; break;
      default:
        // Handle non-printable characters using hex codes
        if (ch < 32 || ch > 126)
          escaped << "\\x" << std::hex << std::setw(2) << std::setfill('0') << (int)ch;
        else
          escaped << ch;
        break;
    }
  return escaped.str();
}
