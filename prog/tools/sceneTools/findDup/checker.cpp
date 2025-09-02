// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "checker.h"

#include <stdio.h>

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_string.h>
#include <libTools/dtx/dtxHeader.h>
#include <perfMon/dag_cpuFreq.h>
#include <hash/md5.h>

static uint64_t processed_files = 0, processed_files_size = 0;
static int last_reported_t_msec = 0;

FileChecker::FileChecker(bool all_files) : files(midmem), fileDups(midmem), allFiles(all_files) {}
FileChecker::~FileChecker() { clear_all_ptr_items(files); }


void FileChecker::gatherFileNames(const char *dir_path)
{
  String mask(260, "%s/*.*", dir_path);

  for (const alefind_t &fileInfo : dd_find_iterator(mask, DA_FILE))
  {
    processFile(String(260, "%s/%s", dir_path, fileInfo.name));
  }

  for (const alefind_t &dirInfo : dd_find_iterator(mask, DA_SUBDIR))
  {
    checkDir(dirInfo.name, dir_path);
  }
}


void FileChecker::checkDir(const char *dir_name, const char *parent_dir)
{
  SimpleString dirName(dir_name);
  if (dirName == "CVS" || dirName == ".svn")
    return;

  gatherFileNames(String(260, "%s/%s", parent_dir, dir_name));
}


void FileChecker::processFile(const char *file_path)
{
  if (!allFiles)
  {
    const char *ext = dd_get_fname_ext(file_path);
    if (!ext || stricmp(ext, ".blk") == 0)
      return;
  }

  file_ptr_t file = ::df_open(file_path, DF_READ);
  if (file)
  {
    DagorStat stbuf;
    df_fstat(file, &stbuf);
    if (stbuf.size >= (2047 << 20))
    {
      processed_files++;
      printf(" - skipping huge file: %s sz=%lldM\n", file_path, stbuf.size >> 20);
      ::df_close(file);
      return;
    }

    int len = ::df_length(file);
    int offset = 0;
    // skip null files
    if (!len)
    {
      ::df_close(file);
      return;
    }

    unsigned char *data = new unsigned char[len];
    df_read(file, data, len);
    ::df_close(file);

    SimpleString file_ext(::dd_get_fname_ext(file_path));
    if (file_ext == ".dds" || file_ext == ".dtx")
    {
      int addr_mode_u = 0, addr_mode_v = 0;
      ddstexture::getInfo(data, len, &addr_mode_u, &addr_mode_v, &offset);
      len -= offset;

      // if (offset)
      //   printf("\n DTX with footer (%s)", file_path);
    }

    FileInfo *info = new FileInfo(file_path);
    files.push_back(info);

    calculateHash(info->hash, data + offset, len);
    processed_files_size += len;

    delete[] data;
  }
  processed_files++;
  if (last_reported_t_msec + 5000 < get_time_msec())
  {
    printf("...processed %lld files (%lldM)\n", processed_files, processed_files_size >> 20);
    last_reported_t_msec = get_time_msec();
  }
}


void FileChecker::calculateHash(unsigned char *md5_hash, unsigned char *data, int len)
{
  md5_state_t state;
  md5_init(&state);
  md5_append(&state, data, len);
  md5_finish(&state, md5_hash);
}


void FileChecker::checkForDuplicates()
{
  clear_and_shrink(fileDups);

  for (int i = 0; i < files.size(); ++i)
  {
    for (int j = i + 1; j < files.size(); ++j)
    {
      if (!memcmp(files[i]->hash, files[j]->hash, MD5_LEN))
        fileDups.push_back(FilePair(i, j));
    }
  }
}
