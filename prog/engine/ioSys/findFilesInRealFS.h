// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>
#include <osApiWrappers/basePath.h>
#include <EASTL/fixed_vector.h>
#include <debug/dag_debug.h>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

struct SearchFolder
{
  fs::path basePath;
  fs::path searchDirectory;
};

using BasePathFolders = eastl::fixed_vector<SearchFolder, 8>;

static BasePathFolders gather_folders_in_base_paths(const char *directory)
{
  BasePathFolders paths;
  std::error_code ec;
  if (is_path_abs(directory))
  {
    fs::path path(directory);
    if (fs::exists(path, ec))
      paths.push_back(SearchFolder{fs::path(), eastl::move(path)});
    else if (ec)
      logerr("gather_folders_in_base_paths: fs::exists(%s) failed: %s", path.string().c_str(), ec.message().c_str());
  }
  else
  {
    for (int i = 0; i < DF_MAX_BASE_PATH_NUM && df_base_path[i]; i++)
    {
      fs::path path(df_base_path[i]);
      path.concat(directory);
      if (fs::exists(path, ec))
        paths.push_back(SearchFolder{fs::path(df_base_path[i]), eastl::move(path)});
      else if (ec)
        logerr("gather_folders_in_base_paths: fs::exists(%s) failed: %s", path.string().c_str(), ec.message().c_str());
    }
  }
  return paths;
}

static bool check_file_suffix(const fs::path &path, const char *suffix, unsigned suffix_len)
{
  if (!*suffix)
    return true;
  if (path.native().size() < suffix_len)
    return false;
  std::string path_str = path.string();
  size_t path_str_len = path_str.length();
  return path_str_len >= suffix_len && strcmp(path_str.c_str() + (path_str_len - suffix_len), suffix) == 0;
}

static void add_directory_entry(Tab<SimpleString> &out_list, const std::filesystem::path &folder, const fs::directory_entry &entry,
  const char *suffix, unsigned suffix_len)
{
  std::error_code ec;
  bool isRegularFile = entry.is_regular_file(ec);
  if (ec)
    logerr("add_directory_entry: is_regular_file(%s) failed: %s", entry.path().string().c_str(), ec.message().c_str());
  else if (isRegularFile)
    if (check_file_suffix(entry.path(), suffix, suffix_len))
    {
      std::string relativePath = folder.empty() ? entry.path().string() : entry.path().lexically_relative(folder).string();
      eastl::replace(relativePath.begin(), relativePath.end(), '\\', '/'); // Normalize path separators
      out_list.push_back(SimpleString(relativePath.c_str()));
    }
}

static void find_real_files_in_folder_ex(Tab<SimpleString> &out_list, const char *dir_path, const char *suffix, bool subdirs)
{
  BasePathFolders folders = gather_folders_in_base_paths(dir_path);
  unsigned suffix_len = (unsigned)strlen(suffix);
  for (const auto &[base_path, folder] : folders)
    if (subdirs)
    {
      std::error_code ec;
      for (fs::recursive_directory_iterator it(folder, ec), end; !ec && it != end; it.increment(ec))
        add_directory_entry(out_list, base_path, *it, suffix, suffix_len);
      if (ec)
        logerr("find_real_files_in_folder_ex: recursive iteration of %s failed: %s", folder.string().c_str(), ec.message().c_str());
    }
    else
    {
      std::error_code ec;
      for (fs::directory_iterator it(folder, ec), end; !ec && it != end; it.increment(ec))
        add_directory_entry(out_list, base_path, *it, suffix, suffix_len);
      if (ec)
        logerr("find_real_files_in_folder_ex: iteration of %s failed: %s", folder.string().c_str(), ec.message().c_str());
    }
}
