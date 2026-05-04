// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_shaders.h>
#include <EditorCore/ec_wndGlobal.h>

#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>

#include <util/dag_string.h>

namespace tools3d
{

bool get_snapshot_path(const DataBlock &blk, const char *app_dir, String &out_path)
{
  if (const char *snapshot = blk.getStr("dngSnapshot", nullptr))
  {
    String dir(0, "%s/%s/", app_dir, snapshot);
    simplify_fname(dir);
    if (alefind_t fs; dd_find_first(dir + "*.shdump.bin", 0, &fs))
    {
      dd_find_close(&fs);
      out_path = dir;
      return true;
    }
  }
  return false;
}

String get_shaders_path(const DataBlock &blk, const char *app_dir, bool use_dng)
{
  if (use_dng)
  {
    String dir;
    if (get_snapshot_path(blk, app_dir, dir))
    {
      dd_add_base_path(dir);
      dir.append("game");
      return dir;
    }
  }

  if (const char *shfn = blk.getStr("shaders", nullptr))
    return String(260, "%s/%s", app_dir, use_dng ? blk.getStr("dngShaders", shfn) : shfn);
  else
    return String(260, "%s/compiledShaders/classic/tools", sgg::get_common_data_dir());
}

} // namespace tools3d
