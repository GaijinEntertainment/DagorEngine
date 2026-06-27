// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_shaders.h>
#include <EditorCore/ec_wndGlobal.h>

#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <libTools/util/appDirRelativePath.h>

namespace tools3d
{

bool get_snapshot_path(const DataBlock &blk, String &out_path)
{
  if (const char *snapshot = blk.getStr("dngSnapshot", nullptr))
  {
    String dir = make_eff_app_relative_path(snapshot, true);
    if (alefind_t fs; dd_find_first(dir + "*.shdump.bin", 0, &fs))
    {
      dd_find_close(&fs);
      out_path = dir;
      return true;
    }
  }
  return false;
}

String get_shaders_path(const DataBlock &blk, bool use_dng)
{
  if (use_dng)
  {
    String dir;
    if (get_snapshot_path(blk, dir))
    {
      dd_add_base_path(dir);
      dir.append("game");
      return dir;
    }
  }

  if (const char *shfn = blk.getStr("shaders", nullptr))
    return make_eff_app_relative_path(use_dng ? blk.getStr("dngShaders", shfn) : shfn);
  return {};
}

} // namespace tools3d
