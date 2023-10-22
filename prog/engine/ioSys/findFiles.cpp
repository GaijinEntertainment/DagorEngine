#include <ioSys/dag_findFiles.h>
#include <util/dag_string.h>
#include <generic/dag_sort.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/basePath.h>
#include <EASTL/algorithm.h>
#include <debug/dag_debug.h>

static void find_real_files_in_folder(Tab<SimpleString> &out_list, const char *dir_path, const char *file_suffix_to_match,
  bool subdirs)
{
  String tmpPath(0, "%s/*%s", dir_path, file_suffix_to_match);
  alefind_t ff;
  if (::dd_find_first(tmpPath, 0, &ff))
  {
    do
      out_list.push_back() = String(0, "%s/%s", *dir_path ? dir_path : ".", ff.name);
    while (dd_find_next(&ff));
    dd_find_close(&ff);
  }

  if (subdirs)
  {
    tmpPath.printf(0, "%s/*", dir_path);
    if (::dd_find_first(tmpPath, DA_SUBDIR, &ff))
    {
      do
        if (ff.attr & DA_SUBDIR)
        {
          if (dd_stricmp(ff.name, "cvs") == 0 || dd_stricmp(ff.name, ".svn") == 0 || dd_stricmp(ff.name, ".git") == 0 ||
              dd_stricmp(ff.name, ".") == 0 || dd_stricmp(ff.name, "..") == 0)
            continue;

          find_real_files_in_folder(out_list, String(0, "%s/%s", dir_path, ff.name), file_suffix_to_match, true);
        }
      while (dd_find_next(&ff));
      dd_find_close(&ff);
    }
  }
}

static void find_vromfs_files_in_folder(Tab<SimpleString> &out_list, const char *dir_path, const char *file_suffix_to_match,
  bool subdirs)
{
  for (int bpi = -1; bpi < DF_MAX_BASE_PATH_NUM; bpi++)
  {
    const char *base_path_prefix = "";
    if (bpi >= 0)
    {
      // debug("df_base_path[%d]=%s df_base_path_vrom_mounted=%d", bpi, df_base_path[bpi], df_base_path_vrom_mounted[bpi]);
      if (!df_base_path[bpi])
        break;
      if (!df_base_path_vrom_mounted[bpi] || !*df_base_path[bpi])
        continue;
      base_path_prefix = df_base_path[bpi];
    }

    int suffix_len = i_strlen(file_suffix_to_match);
    String prefix(0, "%s%s/", base_path_prefix, *dir_path ? dir_path : ".");
    String suffix(file_suffix_to_match);
    dd_simplify_fname_c(prefix);
    prefix.resize(strlen(prefix) + 1);
    dd_strlwr(prefix);
    dd_strlwr(suffix);

    iterate_vroms([&](VirtualRomFsData *entry, size_t) {
      const char *mnt_path = get_vromfs_mount_path(entry);
      int mnt_path_len = mnt_path ? i_strlen(mnt_path) : 0;
      if (mnt_path_len && strncmp(mnt_path, prefix, mnt_path_len) != 0)
        return true;

      const char *pfx = prefix.str() + mnt_path_len;
      int pfx_len = prefix.length() - mnt_path_len;
      for (int j = 0; j < entry->files.map.size(); j++)
      {
        if (strncmp(entry->files.map[j], pfx, pfx_len) == 0 && (subdirs || !strchr(entry->files.map[j] + pfx_len, '/')))
        {
          int name_len = i_strlen(entry->files.map[j] + pfx_len);
          if (name_len >= suffix_len && strcmp(entry->files.map[j] + pfx_len + name_len - suffix_len, suffix) == 0)
          {
            if (mnt_path_len)
              out_list.push_back() = String(0, "%s%s", mnt_path, entry->files.map[j].get());
            else
              out_list.push_back() = entry->files.map[j];
          }
        }
      }
      return true;
    });
  }
}

static void remove_duplicates(Tab<SimpleString> &out_list)
{
  eastl::sort(out_list.begin(), out_list.end());
  const intptr_t removeFrom = eastl::distance(out_list.begin(), eastl::unique(out_list.begin(), out_list.end()));
  const intptr_t removeCount = out_list.size() - removeFrom;
  if (removeCount > 0)
    erase_items(out_list, removeFrom, removeCount);
}

int find_files_in_folder(Tab<SimpleString> &out_list, const char *dir_path, const char *file_suffix_to_match, bool vromfs, bool realfs,
  bool subdirs)
{
  if (strcmp(file_suffix_to_match, "*") == 0 || strcmp(file_suffix_to_match, "*.*") == 0)
    file_suffix_to_match = "";
  else if (
    strncmp(file_suffix_to_match, "*.", 2) == 0 && !strchr(file_suffix_to_match + 2, '*') && !strchr(file_suffix_to_match + 2, '?'))
    file_suffix_to_match++;
  if (strchr(file_suffix_to_match, '*') || strchr(file_suffix_to_match, '?'))
  {
    logerr("%s: bad file_suffix_to_match=\"%s\", no wildcard matching allowed!", __FUNCTION__, file_suffix_to_match);
    return 0;
  }

  String tmp_dir_path;
  if (dd_resolve_named_mount(tmp_dir_path, dir_path))
    dir_path = tmp_dir_path;

  int start_cnt = out_list.size();
  if (vromfs && vromfs_first_priority)
    find_vromfs_files_in_folder(out_list, dir_path, file_suffix_to_match, subdirs);
  if (realfs)
    find_real_files_in_folder(out_list, dir_path, file_suffix_to_match, subdirs);
  if (vromfs && !vromfs_first_priority)
    find_vromfs_files_in_folder(out_list, dir_path, file_suffix_to_match, subdirs);

  // In case if realfs is true and vromfs is true
  // We might get the same paths in the list:
  // Ones froms file system and ones from vroms.
  // So, ensure that we do not return duplicated paths.
  if (vromfs && realfs)
    remove_duplicates(out_list);

  return out_list.size() - start_cnt;
}

int find_file_in_vromfs(Tab<SimpleString> &out_list, const char *filename)
{
  int start_cnt = out_list.size();
  String fname;
  if (!dd_resolve_named_mount(fname, filename))
    fname = filename;

  dd_strlwr(fname);
  int len = fname.length();

  iterate_vroms([&](VirtualRomFsData *entry, size_t) {
    const char *mnt_path = get_vromfs_mount_path(entry);
    for (int j = 0; j < entry->files.map.size(); j++)
    {
      const char *f = entry->files.map[j];
      int flen = (int)strlen(f);
      if (flen < len)
        continue;
      if (strcmp(f + flen - len, fname) != 0)
        continue;
      if (mnt_path)
        out_list.push_back() = String(0, "%s%s", mnt_path, entry->files.map[j].get());
      else
        out_list.push_back() = entry->files.map[j];
    }
    return true;
  });

  return out_list.size() - start_cnt;
}

#define EXPORT_PULL dll_pull_iosys_findFiles
#include <supp/exportPull.h>
