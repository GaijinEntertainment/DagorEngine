// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define __UNLIMITED_BASE_PATH     1
#define __SUPPRESS_BLK_VALIDATION 1
#include <startup/dag_mainCon.inc.cpp>
#include <ioSys/dag_findFiles.h>
#include <osApiWrappers/dag_vromfs.h>
#include <util/dag_strUtil.h>
#include <util/dag_oaHashNameMap.h>
#include <libTools/util/genericCache.h>

static void print_header()
{
  printf("GRP/DxP cleanup util v1.0\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
}

static bool clean_extra_res_files(const char *root_dir, const char *vrom_name, const char *res_blk_name, bool clean, bool validate)
{
  int root_prefix_len = (int)strlen(root_dir) + 1;
  String vfs_fn(0, "%s/%s", root_dir, vrom_name);
  VirtualRomFsData *vfs = load_vromfs_dump(vfs_fn, tmpmem);
  if (!vfs)
  {
    printf("ERR: failed to load new VROMFS %s\n", vfs_fn.str());
    return false;
  }

  add_vromfs(vfs, true, nullptr);
  DataBlock resList;
  if (!resList.load(res_blk_name))
  {
    printf("ERR: cannot read %s from VROMFS %s\n", res_blk_name, vfs_fn.str());
    return false;
  }
  remove_vromfs(vfs);

  OAHashNameMap<true> used_res_fn;
  int nid = resList.getNameId("pack");
  if (const DataBlock *b = resList.getBlockByName("ddsxTexPacks"))
    for (int i = 0; i < b->paramCount(); i++)
      if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == nid)
        used_res_fn.addNameId(b->getStr(i));

  if (const DataBlock *b = resList.getBlockByName("gameResPacks"))
    for (int i = 0; i < b->paramCount(); i++)
      if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == nid)
        used_res_fn.addNameId(b->getStr(i));

  Tab<SimpleString> fn_list;
  find_files_in_folder(fn_list, root_dir, ".dxp.bin", false, true, true);
  find_files_in_folder(fn_list, root_dir, ".grp", false, true, true);

  nid = resList.getNameId("unused");
  for (int i = 0; i < resList.paramCount(); i++)
    if (resList.getParamType(i) == DataBlock::TYPE_STRING && resList.getParamNameId(i) == nid)
      find_files_in_folder(fn_list, root_dir, resList.getStr(i), false, true, true);

  const DataBlock &bHash = *resList.getBlockByNameEx("hash_md5");
  for (int i = 0; i < fn_list.size(); i++)
  {
    if (used_res_fn.getNameId(fn_list[i] + root_prefix_len) < 0)
    {
      debug("unref: %s", fn_list[i] + root_prefix_len);
      printf("%s\n", fn_list[i].str());
      if (clean)
      {
        unlink(fn_list[i]);
        debug("delete: %s", fn_list[i]);
      }
    }
    else
      debug("used: %s", fn_list[i] + root_prefix_len);
  }
  bool ret_ok = true;
  if (validate)
    iterate_names_in_lexical_order(used_res_fn, [&](int, const char *name) {
      if (const char *md5sum = bHash.getStr(name, NULL))
      {
        String fn(0, "%s/%s", root_dir, name);
        unsigned char hash[GenericBuildCache::HASH_SZ];
        if (GenericBuildCache::getFileHash(fn, hash))
        {
          String stor;
          if (stricmp(data_to_str_hex(stor, hash, sizeof(hash)), md5sum) == 0)
            debug("validated %s", fn.str());
          else
          {
            debug("validation failed: %s,  %s != %s", fn.str(), stor, md5sum);
            printf("ERR: validation failed: %s,  %s != %s\n", fn.str(), stor.str(), md5sum);
            ret_ok = false;
          }
        }
        else
        {
          debug("failed to calc: %s", fn);
          printf("ERR: failed to calc: %s\n", fn.str());
          ret_ok = false;
        }
      }
      else
        debug("can't validate[no md5]: %s", name);
    });
  return ret_ok;
}

int DagorWinMain(bool debugmode)
{
  debug_enable_timestamps(false);
  bool clean = false;
  bool validate = false;
  const char *vfs_fn = "grp_hdr.vromfs.bin";
  const char *reslist_fn = "resPacks.blk";

  for (int i = 1; i < __argc; i++)
    if (__argv[i][0] == '-')
    {
      if (stricmp(__argv[i], "-clean") == 0)
        clean = true;
      else if (stricmp(__argv[i], "-validate") == 0)
        validate = true;
      else if (strnicmp(__argv[i], "-vfs:", 5) == 0)
        vfs_fn = __argv[i] + 5;
      else if (strnicmp(__argv[i], "-resBlk:", 8) == 0)
        reslist_fn = __argv[i] + 8;
      else
        printf("ERR: unknown option %s, skipping\n", __argv[i]);

      memmove(__argv + i, __argv + i + 1, (__argc - i - 1) * sizeof(__argv[0]));
      __argc--;
      i--;
    }

  if (__argc < 2)
  {
    print_header();
    printf("usage: resDiffUtil-dev.exe [switches] <vrom_mount_dir_root>\n\n"
           "switches are:\n"
           "  -clean\n"
           "  -vfs:<grp_hdr.vromfs.bin>\n"
           "  -resBlk:<resPacks.blk>\n"
           "\n\n");
    return -1;
  }

  String root(__argv[1]);
  simplify_fname(root);
  return clean_extra_res_files(__argv[1], vfs_fn, reslist_fn, clean, validate) ? 0 : 13;
}
