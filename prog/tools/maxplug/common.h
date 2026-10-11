// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <charconv>
#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <filesystem>
#include <max.h>

#include "ci.h"

// boolean guard: sets the flag for the duration of the scope, then puts back what was there
class Autotoggle
{
  bool &f;
  const bool prev;

public:
  explicit Autotoggle(bool &f_) : f(f_), prev(f_) { f = true; }
  Autotoggle(bool &f_, bool v_) : f(f_), prev(f_) { f = v_; }
  ~Autotoggle() { f = prev; }

  Autotoggle(const Autotoggle &) = delete;
  Autotoggle &operator=(const Autotoggle &) = delete;
};


// one owner for a custom control interface: every Get* has to be paired with its own Release*
struct ControlReleaser
{
  void operator()(ICustEdit *c) const { ReleaseICustEdit(c); }
  void operator()(ICustButton *c) const { ReleaseICustButton(c); }
  void operator()(ISpinnerControl *c) const { ReleaseISpinner(c); }
  void operator()(IColorSwatch *c) const { ReleaseIColorSwatch(c); }
  void operator()(IRollupWindow *c) const { ReleaseIRollup(c); }
};

template <typename T>
using ControlPtr = std::unique_ptr<T, ControlReleaser>;


// add rollup page with layout
HWND add_rollup_page(Interface *ip, int resource_id, DLGPROC proc, const TCHAR *title, DWORD flags = 0);

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

// true while an export runs with prompts suppressed, or while Max runs in quiet mode (3dsmaxbatch,
// network rendering), where no one can answer a dialog
bool are_prompts_suppressed();

// the full path of a texture named in a dag or a proxymat
std::filesystem::path resolve_tex_path(const wchar_t *name);

std::vector<std::wstring> split(std::wstring_view text, const wchar_t delim);
std::wstring replace_all(std::wstring str, std::wstring_view from, std::wstring_view to);

// "a<sep><sep><sep>b" --> "a<sep>b"
std::wstring collapse_repeats(std::wstring str, std::wstring_view seq);

void trim(std::wstring &str);

bool isProxymatName(std::wstring_view mat_name);

inline bool iequal(std::string_view s1, std::string_view s2) { return CaseInsensitiveEqual{}(s1, s2); }
inline bool iequal(std::wstring_view s1, std::wstring_view s2) { return CaseInsensitiveEqualW{}(s1, s2); }

inline bool istarts_with(std::string_view s, std::string_view prefix) { return iequal(s.substr(0, prefix.size()), prefix); }
inline bool istarts_with(std::wstring_view s, std::wstring_view prefix) { return iequal(s.substr(0, prefix.size()), prefix); }

std::wstring simplifyRN(std::wstring_view from);
std::wstring trim_params(std::wstring_view from);

std::string escape_json_string(std::string_view input);

// consumes leading spaces and tabs, then one number; false if there is none
template <typename T>
bool parse_num(std::string_view &s, T &out)
{
  const size_t at = s.find_first_not_of(" \t");
  if (at == std::string_view::npos)
    return false;

  const char *begin = s.data() + at + (s[at] == '+' ? 1 : 0);
  const auto [p, ec] = std::from_chars(begin, s.data() + s.size(), out);
  if (ec != std::errc())
    return false;

  s.remove_prefix(p - s.data());
  return true;
}

// consumes the run of spaces, tabs and commas between two numbers; false if it holds none of `sep`
inline bool parse_sep(std::string_view &s, std::string_view sep)
{
  const std::string_view gap = s.substr(0, s.find_first_not_of(" \t,"));
  if (gap.find_first_of(sep) == std::string_view::npos)
    return false;

  s.remove_prefix(gap.size());
  return true;
}

// how many of the numbers were read, like the return value of scanf; base is always 10.
// consumes what it reads, so `s` is left holding the tail
template <typename... T>
int parse_nums_sep(std::string_view &s, std::string_view sep, T &...out)
{
  int parsed = 0;
  (void)(((parsed == 0 || parse_sep(s, sep)) && parse_num(s, out) && (++parsed, true)) && ...);
  return parsed;
}

// the numbers must be comma-separated, as in "1, 2, 3"
template <typename... T>
int parse_nums(std::string_view s, T &...out)
{
  return parse_nums_sep(s, ",", out...);
}

// the numbers must be separated by blanks, as in "1 2 3"
template <typename... T>
int parse_spaced_nums(std::string_view s, T &...out)
{
  return parse_nums_sep(s, " \t", out...);
}
