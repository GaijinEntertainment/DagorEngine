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

int get_save_filename(HWND owner, const TCHAR *title, FilterList &filter, const TCHAR *def_ext, TSTR &exp_fname);
bool get_open_filename(HWND owner, TSTR &imp_fname);
