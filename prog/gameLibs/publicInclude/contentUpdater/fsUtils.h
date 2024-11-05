//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string.h>
#include <EASTL/initializer_list.h>


namespace updater
{
namespace fs
{
bool remove_folder(const char *folder);
bool remove_file(const char *path);
bool copy_file(const char *src, const char *dst);
bool copy_files(const char *src_folder, const char *dst_folder, const char *mask);
bool move_file(const char *src, const char *dst);
bool move_files(const char *src_folder, const char *dst_folder, const char *mask);
bool move_tree(const char *src_folder, const char *dst_folder);
bool remove_tree(const char *folder);

eastl::string read_file_content(const char *path);
bool write_file_with_content(const char *path, const char *content);

eastl::string join_path(std::initializer_list<const char *> parts);
eastl::string simplify_path(const char *path);
eastl::string normalize_path(const char *path);
const char *get_filename_relative_to_path(const eastl::string &base_path, const char *path);
} // namespace fs
} // namespace updater
