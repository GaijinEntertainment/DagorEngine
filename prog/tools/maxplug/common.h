// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vector>
#include <string_view>
#include <filesystem>
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


// file extension is ".dag"
bool is_dag_file(const std::filesystem::path &filename);

// list of all files in the dir
std::vector<std::filesystem::path> glob(const std::filesystem::path &dir, bool recursive);


std::wstring strToWide(const char *sz);
std::wstring strToWide(std::string_view sv);
std::string wideToStr(const wchar_t *sw);
std::string wideToStr(std::wstring_view sv);

std::wstring format_str(const TCHAR *fmt, ...);

// "\foo\bar.baz" --> \foo\bar.baz
std::wstring drop_quotation_marks(std::wstring_view s);

// full text of a window/control; empty string if there is none
std::wstring get_window_text(HWND hw);

// read a numeric edit control, false if it does not hold a valid number
bool get_edint(ICustEdit *e, int &a);
bool get_edfloat(ICustEdit *e, float &a);

void update_path_edit_control(HWND hDlg, int id, const std::filesystem::path &path);

int get_save_filename(HWND owner, const TCHAR *title, FilterList &filter, const TCHAR *def_ext, std::filesystem::path &exp_fname,
  bool init_with_previous = true);
bool get_open_filename(HWND owner, const TCHAR *title, FilterList &filter, const TCHAR *def_ext, std::filesystem::path &imp_fname);
bool get_open_filename(HWND owner, std::filesystem::path &imp_fname);

std::filesystem::path get_cfg_filename(const TCHAR *cfg);

std::vector<std::wstring> split(std::wstring_view text, const wchar_t delim);
std::wstring replace_all(std::wstring str, std::wstring_view from, std::wstring_view to);

// "a<sep><sep><sep>b" --> "a<sep>b"
std::wstring collapse_repeats(std::wstring str, std::wstring_view seq);

void trim(std::wstring &str);

bool isProxymatName(std::wstring_view mat_name);

inline bool iequal(std::wstring_view s1, std::wstring_view s2)
{
  return s1.size() == s2.size() && (s1.empty() || _wcsnicmp(s1.data(), s2.data(), s1.size()) == 0);
}

template <typename T>
inline T clamp(T v, T min, T max)
{
  if (v < min)
    return min;
  if (v > max)
    return max;
  return v;
}

std::wstring simplifyRN(std::wstring_view from);
std::wstring trim_params(std::wstring_view from);

std::string escape_string(std::string_view input);

template <typename T>
T parse_param_value(std::wstring_view in)
{
  std::wistringstream str{std::wstring(in)};
  str.imbue(std::locale::classic());
  T out = 0;
  str >> out;
  return out;
}

template <typename T, int C>
T parse_param_value(std::wstring_view in)
{
  std::wistringstream str{std::wstring(in)};
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
