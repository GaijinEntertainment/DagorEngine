// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <contentUpdater/fsUtils.h>

#include <debug/dag_debug.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_pathDelim.h>
#include <memory/dag_framemem.h>
#include <util/dag_baseDef.h>

#include <EASTL/array.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>


eastl::string updater::fs::simplify_path(const char *path)
{
  eastl::array<char, DAGOR_MAX_PATH> buffer;
  eastl::fill(buffer.begin(), buffer.end(), 0);

  ::strcpy(buffer.data(), path);

  dd_simplify_fname_c(buffer.data());

  return eastl::string{buffer.data()};
}


eastl::string updater::fs::join_path(std::initializer_list<const char *> parts)
{
  if (parts.size() == 0)
    return {};

  using TmpString = eastl::basic_string<char, framemem_allocator>;

  TmpString path(*parts.begin());
  for (auto it = parts.begin() + 1, end = parts.end(); it != end; ++it)
    path.append_sprintf("%c%s", PATH_DELIM, *it);

  return simplify_path(path.c_str());
}


eastl::string updater::fs::normalize_path(const char *path)
{
  eastl::string res = simplify_path(path);
  if (res.back() != PATH_DELIM)
    res.push_back(PATH_DELIM);
  return res;
}


const char *updater::fs::get_filename_relative_to_path(const eastl::string &base_path, const char *path)
{
  const size_t basePathOffset = base_path.length() + (base_path.back() == PATH_DELIM ? 0 : 1);
  return path + basePathOffset;
}

eastl::string updater::fs::read_file_content(const char *path)
{
  eastl::unique_ptr<void, DagorFileCloser> fp{df_open(path, DF_READ | DF_IGNORE_MISSING)};
  if (fp)
  {
    eastl::vector<char> buffer(size_t(df_length(fp.get())));
    if (df_read(fp.get(), buffer.data(), buffer.size()) == buffer.size())
      return eastl::string{buffer.data(), buffer.size()};
  }
  return {};
}


bool updater::fs::write_file_with_content(const char *path, const char *content)
{
  eastl::unique_ptr<void, DagorFileCloser> fp{df_open(path, DF_WRITE | DF_CREATE)};
  if (fp)
  {
    const size_t len = ::strlen(content);
    return df_write(fp.get(), content, len) == len;
  }
  return false;
}
