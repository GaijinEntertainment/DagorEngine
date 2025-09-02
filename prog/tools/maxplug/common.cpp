// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "common.h"
#include "layout.h"
#include "resource.h"

extern TCHAR *GetString(int id);

HWND add_rollup_page(Interface *ip, int resource_id, DLGPROC proc, const TCHAR *title, LPARAM param, DWORD flags)
{
  const WCHAR *tpl = MAKEINTRESOURCE(resource_id);
  HWND hw = ip->AddRollupPage(hInstance, tpl, proc, title, 0, flags);
  attach_layout_to_rollup(hw, tpl);
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


int get_save_filename(HWND owner, const TCHAR *title, FilterList &filter, const TCHAR *def_ext, TSTR &exp_fname)
{
  static TCHAR fname[MAX_PATH];
  _tcscpy(fname, exp_fname);

  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(ofn));

  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = owner;
  ofn.lpstrFilter = filter;
  ofn.lpstrFile = fname;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrInitialDir = _T("");
  ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
  ofn.lpstrDefExt = def_ext;
  ofn.lpstrTitle = title;

tryAgain:
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


bool get_open_filename(HWND owner, TSTR &imp_fname)
{
  static TCHAR path[MAX_PATH];

  _tcscpy_s(path, MAX_PATH, imp_fname.data());

  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(ofn));
  FilterList fl;
  fl.Append(GetString(IDS_SCENE_FILES));
  fl.Append(_T("*.dag"));
  TSTR title = GetString(IDS_OPEN_SCENE_TITLE);

  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = owner;
  ofn.lpstrFilter = fl;
  ofn.lpstrFile = path;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrInitialDir = _T("");
  ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
  ofn.lpstrDefExt = _T("dag");
  ofn.lpstrTitle = title;

  bool result = GetOpenFileName(&ofn);
  if (result)
    imp_fname = path;

  return result;
}
