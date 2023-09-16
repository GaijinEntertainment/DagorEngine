//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>

#include <util/dag_string.h>
#include <util/dag_stdint.h>


bool dag_get_file_time(const char *fname, int64_t &time_date);
bool dag_set_file_time(const char *fname, int64_t time_date);

int dag_get_file_size(const char *fname);

bool dag_copy_file(const char *src, const char *dest, bool overwrite = true);
bool dag_copy_folder_content(const char *src, const char *dest, const Tab<String> &ignore, bool copy_subfolders = true);

// removes folder and all its content including subfolders
bool dag_rmtree(const char *path);

// compare pathes
int dag_path_compare(const char *path1, const char *path2);

// return true if files are binary equal
bool dag_file_compare(const char *path1, const char *path2);

// load file to memory
bool dag_read_file_to_mem(const char *path, Tab<char> &buffer);
// load file to memory as string (zero-character ended)
bool dag_read_file_to_mem_str(const char *path, String &buffer);
