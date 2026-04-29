//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <supp/dag_define_KRNLIMP.h>


namespace dag
{
// remove directory tree recursively
KRNLIMP bool remove_dirtree(const char *path);

// copy file from src_path to dest_path
KRNLIMP bool copy_file(const char *src_path, const char *dest_path);
// copy folder recursively from src_folder to dst_folder
KRNLIMP bool copy_folder(const char *src_folder, const char *dst_folder);
// move folder from src_folder to dst_folder
KRNLIMP bool move_folder(const char *src_folder, const char *dst_folder);
} // namespace dag

#undef DAG_CANNOT_SET_FTIME
