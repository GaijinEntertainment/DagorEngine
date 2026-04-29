// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/assetHlp.h>
#include <libTools/util/iLogWriter.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>

static inline const char *read_str(const DataBlock *b, const char *target_str, const char *profile, const char *def)
{
  if (!b || b == &DataBlock::emptyBlock)
    return def;
  if (profile && *profile)
    return b->getStr(String(64, "%s~%s", target_str, profile), def);
  return b->getStr(target_str, def);
}

static bool build_patch_dir(String &out_dir, const char *dir)
{
  if (!dir)
    return false;

  char buf[256] = {};
  const char *res = strchr(&dir[1], '/');
  if (res != nullptr)
  {
    int sz = res - dir;

    if (sz > sizeof(buf) - 1)
      return false;

    strncpy(buf, dir, sz);
    buf[sz] = '\0';
    out_dir.printf(0, "%s/patch/%s", buf, res + 1);
  }
  else
    out_dir.printf(0, "%s/patch/", dir);

  return true;
}

bool assethlp::validate_exp_blk(bool has_pkgs, const DataBlock &expBlk, const char *target_str, const char *profile, ILogWriter &log)
{
  const DataBlock *destBlk = expBlk.getBlockByNameEx("destination");
  if (!read_str(destBlk, target_str, profile, NULL))
  {
    log.addMessage(log.ERROR, "destination not specified for target %s", target_str);
    return false;
  }
  if (has_pkgs && !read_str(destBlk->getBlockByName("additional"), target_str, profile, NULL))
  {
    log.addMessage(log.ERROR, "destination for additional packages not specified for target %s", target_str);
    return false;
  }
  return true;
}

void assethlp::build_package_dest_strings(String &out_dest_base, String &out_pack_fname_prefix, const DataBlock &expBlk,
  const char *pkg_name, const char *app_dir, const char *target_str, const char *profile, bool patch_build)
{
  const DataBlock *destBlk = expBlk.getBlockByNameEx("destination");
  String patch_dir;
  if (!pkg_name || strcmp(pkg_name, "*") == 0 || strcmp(pkg_name, ".") == 0)
  {
    const char *dest_dir = read_str(destBlk, target_str, profile, NULL);
    if (patch_build && build_patch_dir(patch_dir, dest_dir))
      out_dest_base.printf(260, "%s/%s/", app_dir, patch_dir.str());
    else
      out_dest_base.printf(260, "%s/%s/", app_dir, dest_dir);
    out_pack_fname_prefix = "";
  }
  else
  {
    const DataBlock *addBlk = destBlk->getBlockByNameEx("additional");
    const DataBlock *pkgBlk = expBlk.getBlockByNameEx("packages")->getBlockByNameEx(pkg_name);

    const char *add_packages_folder = read_str(addBlk, target_str, profile, NULL);
    const char *add_packages_suffix = addBlk->getStr("suffix", "");
    const char *gluePackageStr = pkgBlk->getStr("gluePackage", addBlk->getStr("gluePackage", NULL));

    if (patch_build && build_patch_dir(patch_dir, add_packages_folder))
      out_dest_base.printf(128, "%s/%s%s/%s/%s", app_dir, patch_dir.str(), pkgBlk->getStr("destSuffix", ""), pkg_name,
        add_packages_suffix);
    else
      out_dest_base.printf(128, "%s/%s%s/%s/%s", app_dir, add_packages_folder, pkgBlk->getStr("destSuffix", ""), pkg_name,
        add_packages_suffix);

    if (gluePackageStr && *gluePackageStr)
      out_pack_fname_prefix.printf(128, "%s%s", pkg_name, gluePackageStr);
    else
      out_pack_fname_prefix = "";
  }
  simplify_fname(out_dest_base);
}

void assethlp::build_package_dest(String &out_dest_base, const DataBlock &expBlk, const char *pkg_name, const char *app_dir,
  const char *target_str, const char *profile, bool patch_build)
{
  String unused_prefix;
  assethlp::build_package_dest_strings(out_dest_base, unused_prefix, expBlk, pkg_name, app_dir, target_str, profile, patch_build);
}

void assethlp::build_package_pack_fname_prefix(String &out_pack_fname_prefix, const DataBlock &expBlk, const char *pkg_name,
  const char *app_dir, const char *target_str, const char *profile)
{
  String unused_dest;
  assethlp::build_package_dest_strings(unused_dest, out_pack_fname_prefix, expBlk, pkg_name, app_dir, target_str, profile);
}
