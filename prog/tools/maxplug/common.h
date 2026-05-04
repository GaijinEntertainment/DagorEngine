// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vector>
#include <max.h>

// boolean guard
class Autotoggle
{
  bool &f;

public:
  explicit Autotoggle(bool &f_) : f(f_) { f = true; }
  Autotoggle(bool &f_, bool v_) : f(f_) { f = v_; }
  ~Autotoggle() { f = !f; }
};


// add rollup page with layout
HWND add_rollup_page(Interface *ip, int resource_id, DLGPROC proc, const TCHAR *title, LPARAM param = 0, DWORD flags = 0);

// delete rollup page and reset hw
void delete_rollup_page(Interface *ip, HWND *hw);


//
float get_master_scale();


// \foo\bar.baz --> \foo
TSTR extract_directory(const TSTR &path);

// \foo\bar.baz --> bar
TSTR extract_filename(const TSTR &path);

// \foo\bar.baz --> bar.baz
TSTR extract_basename(const TSTR &path);

// file extension is ".dag"
bool is_dag_file(const TSTR &path);

// list of all files in the dir
std::vector<TSTR> glob(const TSTR &dir, bool recursive);


M_STD_STRING strToWide(const char *sz);
std::string wideToStr(const TCHAR *sw);

// "\foo\bar.baz" --> \foo\bar.baz
TSTR drop_quotation_marks(const TSTR &s);

void update_path_edit_control(HWND hDlg, int id, const TSTR &path);

int get_save_filename(HWND owner, const TCHAR *title, FilterList &filter, const TCHAR *def_ext, TSTR &exp_fname,
  bool init_with_previous = true);
bool get_open_filename(HWND owner, const TCHAR *title, FilterList &filter, const TCHAR *def_ext, TSTR &imp_fname);
bool get_open_filename(HWND owner, TSTR &imp_fname);

std::wstring get_cfg_filename(const TCHAR *cfg);

std::vector<std::wstring> split(const std::wstring &text, const wchar_t delim);
std::wstring replace_all(std::wstring str, const std::wstring &from, const std::wstring &to);

void trim(std::wstring &str);

bool isProxymatName(const TCHAR *mat_name);

template <typename T, typename TT>
inline bool iequal(const T &s1, const TT &s2)
{
  return !_tcsicmp(s1.data(), s2.data());
}

template <typename T>
inline bool iequal(const T &s1, const TCHAR *s2)
{
  return !_tcsicmp(s1.data(), s2);
}

inline bool iequal(const TCHAR *s1, const TCHAR *s2) { return !_tcsicmp(s1, s2); }

template <typename T>
inline T clamp(T v, T min, T max)
{
  if (v < min)
    return min;
  if (v > max)
    return max;
  return v;
}

std::wstring simplifyRN(const std::wstring &from);
std::wstring trim_params(const std::wstring &from);

std::string escape_string(const std::string &input);

template <typename T>
T parse_param_value(const std::wstring &in)
{
  M_STD_ISTRINGSTREAM str(in);
  str.imbue(std::locale::classic());
  T out = 0;
  str >> out;
  return out;
}

template <typename T, int C>
T parse_param_value(const std::wstring &in)
{
  M_STD_ISTRINGSTREAM str(in);
  str.imbue(std::locale::classic());
  T out;
  for (int i = 0; i < C; ++i)
  {
    str >> out[i];
    while (str.peek() == L' ' || str.peek() == L',')
      str.ignore();
  }
  return out;
}
