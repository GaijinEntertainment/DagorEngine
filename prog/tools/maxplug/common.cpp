// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstdarg>

#include "common.h"
#include "dagor.h"
#include "layout.h"
#include "resource.h"

namespace fs = std::filesystem;

HWND add_rollup_page(Interface *ip, int resource_id, DLGPROC proc, const TCHAR *title, LPARAM param, DWORD flags)
{
  const WCHAR *tpl = MAKEINTRESOURCE(resource_id);
  HWND hw = ip->AddRollupPage(hInstance, tpl, proc, title, 0, flags);
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
  float k = static_cast<float>(GetSystemUnitScale(UNITS_METERS));
  return k ? (1.f / k) : 1.f;
}


bool is_dag_file(const fs::path &filename)
{
  std::wstring ext = filename.extension().wstring();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
  return ext == L".dag";
}

std::vector<fs::path> glob(const fs::path &dir, bool recursive)
{
  std::vector<fs::path> files;
  files.reserve(32);

  std::error_code ec;
  auto collect = [&](const fs::directory_entry &entry) {
    if (entry.is_regular_file(ec) && is_dag_file(entry.path().filename()))
      files.push_back(entry.path());
  };

  if (recursive)
  {
    for (const auto &entry : fs::recursive_directory_iterator(dir,
           fs::directory_options::skip_permission_denied | fs::directory_options::follow_directory_symlink, ec))
      collect(entry);
  }
  else
  {
    for (const auto &entry : fs::directory_iterator(dir, fs::directory_options::skip_permission_denied, ec))
      collect(entry);
  }

  return files;
}


std::wstring strToWide(const char *sz)
{
  int len = MultiByteToWideChar(CP_UTF8, 0, sz, -1, NULL, 0);
  if (len <= 0)
    return std::wstring();
  std::wstring res(len - 1, 0);
  MultiByteToWideChar(CP_UTF8, 0, sz, -1, &res[0], len);
  return res;
}


std::string wideToStr(const wchar_t *sw)
{
  int len = WideCharToMultiByte(CP_UTF8, 0, sw, -1, NULL, 0, NULL, NULL);
  std::string res(len - 1, 0);
  WideCharToMultiByte(CP_UTF8, 0, sw, -1, &res[0], len, NULL, NULL);
  return res;
}


std::wstring strToWide(std::string_view sv)
{
  if (sv.empty())
    return std::wstring();
  int len = MultiByteToWideChar(CP_UTF8, 0, sv.data(), (int)sv.size(), NULL, 0);
  if (len <= 0)
    return std::wstring();
  std::wstring res(len, 0);
  MultiByteToWideChar(CP_UTF8, 0, sv.data(), (int)sv.size(), &res[0], len);
  return res;
}


std::string wideToStr(std::wstring_view sv)
{
  if (sv.empty())
    return std::string();
  int len = WideCharToMultiByte(CP_UTF8, 0, sv.data(), (int)sv.size(), NULL, 0, NULL, NULL);
  if (len <= 0)
    return std::string();
  std::string res(len, 0);
  WideCharToMultiByte(CP_UTF8, 0, sv.data(), (int)sv.size(), &res[0], len, NULL, NULL);
  return res;
}


std::wstring format_str(const TCHAR *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  TSTR res;
  res.vprintf(fmt, args);
  va_end(args);
  return std::wstring(res.data());
}


std::wstring drop_quotation_marks(std::wstring_view s)
{
  if (s.length() >= 2 && s.front() == L'"' && s.back() == L'"')
    return std::wstring(s.substr(1, s.length() - 2));

  return std::wstring(s);
}


std::wstring get_window_text(HWND hw)
{
  const int length = GetWindowTextLength(hw);
  if (length <= 0)
    return std::wstring();

  std::wstring text(size_t(length) + 1, L'\0');
  const int copied = GetWindowText(hw, text.data(), length + 1);
  text.resize(copied);
  return text;
}


void update_path_edit_control(HWND hw, int id, const fs::path &path)
{
  SetDlgItemText(hw, id, path.c_str());
  SendMessage(GetDlgItem(hw, id), EM_SETSEL, (WPARAM)path.native().length(), (LPARAM)path.native().length());
}


int get_save_filename(HWND owner, const TCHAR *title, FilterList &filter, const TCHAR *def_ext, fs::path &exp_fname,
  bool init_with_previous)
{
  static TCHAR fname[MAX_PATH];
  if (init_with_previous)
    _tcscpy_s(fname, MAX_PATH, exp_fname.c_str());

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
      SetSaveRequiredFlag();
    }
    return 1;
  }
  return 0;
}


bool get_open_filename(HWND owner, const TCHAR *title, FilterList &filter, const TCHAR *def_ext, fs::path &imp_fname)
{
  static TCHAR path[MAX_PATH];

  _tcscpy_s(path, MAX_PATH, imp_fname.c_str());

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

bool get_open_filename(HWND owner, fs::path &imp_fname)
{
  FilterList fl;
  fl.Append(GetString(IDS_SCENE_FILES));
  fl.Append(L"*.dag");
  return get_open_filename(owner, GetString(IDS_OPEN_SCENE_TITLE), fl, L"dag", imp_fname);
}

fs::path get_cfg_filename(const TCHAR *cfg)
{
  TCHAR modulePath[MAX_PATH];
  GetModuleFileName(hInstance, modulePath, MAX_PATH);
  fs::path moduleDir = fs::path(modulePath).parent_path();

  fs::path cfgPath = moduleDir / L".." / cfg; // search one dir up first
  if (!fs::exists(cfgPath))
    cfgPath = moduleDir / cfg; // if not found then get in module dir

  return cfgPath;
}


std::vector<std::wstring> split(std::wstring_view text, const wchar_t delim)
{
  std::vector<std::wstring> tokens;
  std::wistringstream iss(std::wstring(text) + delim);
  std::wstring tok;
  while (std::getline(iss, tok, delim))
    tokens.push_back(tok);
  return tokens;
}

std::wstring replace_all(std::wstring str, std::wstring_view from, std::wstring_view to)
{
  if (from.empty())
    return str;
  for (size_t pos = 0; (pos = str.find(from, pos)) != std::wstring::npos; pos += to.size())
    str.replace(pos, from.size(), to);
  return str;
}

std::wstring collapse_repeats(std::wstring str, std::wstring_view seq)
{
  if (seq.empty())
    return str;
  for (size_t pos = 0; (pos = str.find(seq, pos)) != std::wstring::npos; pos += seq.size())
  {
    size_t run_end = pos + seq.size();
    while (str.find(seq, run_end) == run_end)
      run_end += seq.size();
    str.erase(pos + seq.size(), run_end - pos - seq.size());
  }
  return str;
}

void trim(std::wstring &str)
{
  str.erase(str.find_last_not_of(L' ') + 1);
  str.erase(0, str.find_first_not_of(L' '));
}

bool isProxymatName(std::wstring_view mat_name)
{
  static const std::wstring proxymat = L":proxymat";
  if (mat_name.size() < proxymat.size())
    return false;
  return mat_name.compare(mat_name.size() - proxymat.size(), proxymat.size(), proxymat) == 0;
}

std::wstring simplifyRN(std::wstring_view from)
{
  std::wstring s(from);
  size_t pos = 0;
  while ((pos = s.find(_T("\r\n"), pos)) != std::wstring::npos)
    s.replace(pos, 2, _T("\n"));
  return s;
}

std::wstring trim_params(std::wstring_view from)
{
  std::wstring s;
  std::vector<std::wstring> lines = split(simplifyRN(from), L'\n');

  for (const std::wstring &line : lines)
  {
    const size_t eq = line.find(L'=');

    std::wstring name = line.substr(0, eq);
    trim(name);

    if (eq == std::wstring::npos)
    {
      if (name.empty())
        continue;

      s += name;
      s += L"\r\n";
      continue;
    }

    std::wstring value = line.substr(eq + 1);
    trim(value);

    s += name;
    s += L'=';
    s += value;
    s += L"\r\n";
  }

  return s;
}

std::string escape_string(std::string_view input)
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
