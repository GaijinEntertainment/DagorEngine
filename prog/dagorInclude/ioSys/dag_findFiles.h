//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>

#include <supp/dag_define_COREIMP.h>

//! fills 'out_list' with filepathes of files in 'dir_path' with matching 'file_ext_to_match';
//! 'vromfs' controls search through loaded VROMFS data, 'realfs' controls search through ordinary filesystem;
//! 'subdirs' controls hierarchical search;
//! returns number of added filepathes;
KRNLIMP int find_files_in_folder(Tab<SimpleString> &out_list, const char *dir_path, const char *file_suffix_to_match = "",
  bool vromfs = true, bool realfs = true, bool subdirs = false);

//! fills 'out_list' with filepathes of files with specific name in all mounted vromfs
//! returns number of added filepathes;
KRNLIMP int find_file_in_vromfs(Tab<SimpleString> &out_list, const char *filename);


#include <supp/dag_undef_COREIMP.h>
