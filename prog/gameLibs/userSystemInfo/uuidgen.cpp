// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <openssl/rand.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_strUtil.h>
#include <util/dag_globDef.h>
#include <stdio.h> // for SNPRINTF


static const int UUID_LENGTH = 10;
static char uuid_strings[4][2 * UUID_LENGTH + 1];
static const char *const UUID_FILE_NAME = "/label.bin";
static bool uuids_inited = false;


static void init_as_unknown()
{
  for (int i = 0; i < countof(uuid_strings); i++)
    SNPRINTF(uuid_strings[i], sizeof(uuid_strings[i]), "unknown");
}

static String cache_dir;
void set_user_system_info_cache_dir(const char *dir) { cache_dir = dir; }

static void init_uuids()
{
  if (uuids_inited)
    return;
  init_as_unknown();
  String path = cache_dir;
  if (path.empty())
  {
    logerr("%s: failed to obtain label cache directory", __FUNCTION__);
    uuids_inited = true;
    return;
  }
  dd_mkdir(path);
  path += UUID_FILE_NAME;
  dd_simplify_fname_c(path);
  unsigned char binUuid[countof(uuid_strings) * UUID_LENGTH];
  bool needNewId = true;
  if (::dd_file_exist(path))
  {
    file_ptr_t f = ::df_open(path, DF_READ | DF_IGNORE_MISSING);
    if (!f)
    {
      logerr("%s: failed to open '%s' for reading", __FUNCTION__, path.str());
      uuids_inited = true;
      return;
    }
    int bytesRead = ::df_read(f, binUuid, sizeof(binUuid));
    ::df_close(f);
    if (bytesRead == sizeof(binUuid))
      needNewId = false;
  }
  if (needNewId)
  {
    RAND_bytes(binUuid, sizeof(binUuid));
    file_ptr_t f = ::df_open(path, DF_WRITE | DF_CREATE);
    if (f)
    {
      if (::df_write(f, binUuid, countof(uuid_strings) * UUID_LENGTH) != countof(uuid_strings) * UUID_LENGTH)
      {
        ::df_close(f);
        logerr("%s: failed to write %d bytes to %s", __FUNCTION__, countof(uuid_strings) * UUID_LENGTH, path.str());
        uuids_inited = true;
        return;
      }
      ::df_close(f);
    }
    else
    {
      logerr("%s: failed to open %s for writing", __FUNCTION__, path.str());
      uuids_inited = true;
      return;
    }
  }

  for (int i = 0; i < countof(uuid_strings); i++)
    data_to_str_hex_buf(uuid_strings[i], sizeof(uuid_strings[i]), &binUuid[i * UUID_LENGTH], UUID_LENGTH);

  uuids_inited = true;
}


namespace uuidgen
{

bool get_uuid(int index, String &dst)
{
#ifdef DEDICATED
  return false;
#else
  if (index < 0 || index >= countof(uuid_strings))
    return false;
  if (!uuids_inited)
    init_uuids();
  dst = uuid_strings[index];
  return true;
#endif
}

} // namespace uuidgen
