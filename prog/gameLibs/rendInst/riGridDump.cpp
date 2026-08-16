// Copyright (C) Gaijin Games KFT.  All rights reserved.

// Producer side of the riExtra object dump: `rigrid.dump [path]` writes the live grid contents in
// the rigrid_dump format, so grid work can be replayed with no session at all. See
// prog/tools/miscUtils/riGridBench for the reader and rendInst/riGridDump.h for the format.

#include "riGen/riGrid.h"
#include "riGen/riExtraPool.h"

#include <rendInst/rendInstExtra.h>
#include <rendInst/riexHandle.h>
#include <rendInst/riGridDump.h>
#include <ioSys/dag_fileIo.h>
#include <dag/dag_vector.h>
#include <vecmath/dag_vecMath.h>
#include <util/dag_console.h>
#include <string.h>

#if DAGOR_DBGLEVEL > 0

namespace
{

// One object the grid is expected to hold: exactly those with a positive radius code, see
// RiExtraPool::isInGrid. Anything else would make the dump a different scene than the runtime.
struct SceneObject
{
  rendinst::riex_handle_t handle;
  vec4f wbsph;
};

static void collect_scene_objects(dag::Vector<SceneObject> &out)
{
  out.clear();
  for (uint32_t type = 0, typeEnd = rendinst::riExtra.size(); type < typeEnd; type++)
  {
    const rendinst::RiExtraPool &pool = rendinst::riExtra[type];
    for (int idx = 0, idxEnd = pool.riXYZR.size(); idx < idxEnd; idx++)
    {
      if (!pool.isValid(idx) || !pool.isInGrid(idx))
        continue;
      out.push_back(SceneObject{rendinst::make_handle(type, idx), pool.riXYZR[idx]});
    }
  }
}

static void dump_scene(const char *path)
{
  dag::Vector<SceneObject> scene;
  collect_scene_objects(scene);

  FullFileSaveCB cb(path);
  if (!cb.fileHandle)
  {
    console::print_d("rigrid.dump: cannot write '%s'", path);
    return;
  }

  const int poolCount = (int)rendinst::riExtra.size();
  rigrid_dump::Header hdr;
  memset(&hdr, 0, sizeof(hdr));
  hdr.magic = rigrid_dump::MAGIC;
  hdr.version = rigrid_dump::VERSION;
  hdr.poolCount = poolCount;
  hdr.instanceCount = (int)scene.size();
  rigrid_get_dump_config(hdr.config);
  cb.write(&hdr, sizeof(hdr));

  for (int i = 0; i < poolCount; i++)
  {
    const char *name = rendinst::getRIGenExtraName(i);
    const int32_t len = name ? (int32_t)strlen(name) : 0;
    cb.write(&len, sizeof(len));
    if (len)
      cb.write(name, len);
  }

  for (const SceneObject &o : scene)
  {
    const uint32_t riType = rendinst::handle_to_ri_type(o.handle);
    const uint32_t riInstance = rendinst::handle_to_ri_inst(o.handle);
    bbox3f wbbox = RiGridObject(o.handle).getWBBox();

    rigrid_dump::Instance rec;
    rec.handle = o.handle;
    // riTm is already the transposed matrix, and 43cu is the 4x3 unaligned layout the record wants,
    // so this needs neither a transpose nor per-column stores
    v_mat_43cu_from_mat43(rec.tm, rendinst::riExtra[riType].riTm[riInstance]);
    v_stu(rec.bsphere, o.wbsph);
    v_stu_p3(rec.bbox + 0, wbbox.bmin);
    v_stu_p3(rec.bbox + 3, wbbox.bmax);
    cb.write(&rec, sizeof(rec));
  }

  console::print_d("rigrid.dump: %d objects, %d pools, %.1f Mb -> %s", (int)scene.size(), poolCount,
    (sizeof(rigrid_dump::Header) + scene.size() * sizeof(rigrid_dump::Instance)) / (1024.0 * 1024.0), path);
}

static bool rigrid_dump_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("rigrid", "dump", 1, 2) { dump_scene(argc == 2 ? argv[1] : "riextra_grid.dump"); }
  return found;
}

} // namespace

// Must be at global scope: the macro defines a console-namespace pull var and references
// ::console::FuncLinkedList, both of which break inside an unnamed namespace.
REGISTER_CONSOLE_HANDLER(rigrid_dump_console_handler);

#endif // DAGOR_DBGLEVEL > 0
