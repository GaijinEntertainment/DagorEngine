#include <gameRes/dag_stdGameRes.h>
#include <util/dag_fileMd5Validate.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_string.h>

void gameres_append_desc(DataBlock &desc, const char *desc_fn, const char *pkg_folder, bool allow_override)
{
  if (dd_file_exists(desc_fn) && !desc.getBool(pkg_folder, false))
  {
    DataBlock blk;
    if (blk.load(desc_fn))
    {
      const char *fn = dd_get_fname(desc_fn), *fn_ext = dd_get_fname_ext(fn);
      bool patch_mode = blk.getBool("patchMode", false);
      int pack_nid = -1;
      Tab<bool> grp_present;
      if (patch_mode)
      {
        pack_nid = blk.getNameId("__pack");
        grp_present.reserve(blk.paramCount() - 1);
        dblk::iterate_params_by_name_id_and_type(blk, pack_nid, DataBlock::TYPE_STRING, [&](int pidx) {
          String grp_fn(0, "%.*s%s", fn - desc_fn, desc_fn, blk.getStr(pidx));
          grp_present.push_back(dd_file_exists(grp_fn));
          // debug("%s=%d", grp_fn, grp_present.back());
        });
      }
      int processed_cnt = 0, replaced_cnt = 0;
      for (int j = 0; j < blk.blockCount(); j++)
      {
        if (patch_mode)
        {
          int pack_idx = blk.getBlock(j)->getIntByNameId(pack_nid, -1);
          if (pack_idx < 0 || pack_idx >= grp_present.size() || !grp_present[pack_idx])
            continue;
        }
        if (allow_override || patch_mode)
          if (desc.removeBlock(blk.getBlock(j)->getBlockName()))
            replaced_cnt++;
        desc.addNewBlock(blk.getBlock(j), blk.getBlock(j)->getBlockName());
        processed_cnt++;
      }
      desc.setBool(pkg_folder, true);
      desc.setBool("optimized", false);

      G_UNUSED(fn);
      G_UNUSED(fn_ext);
      debug("%.*s: added %d names and %d replaced (%s) -> %d total", fn_ext ? fn_ext - fn : strlen(fn), fn,
        processed_cnt - replaced_cnt, replaced_cnt, pkg_folder, desc.blockCount());
    }
  }
}
void gameres_patch_desc(DataBlock &desc, const char *patch_desc_fn, const char *pkg_folder, const char *desc_fn)
{
  if (dd_file_exists(patch_desc_fn))
  {
    DataBlock blk;
    if (blk.load(patch_desc_fn) && !blk.isEmpty() &&
        validate_file_md5_hash(desc_fn, blk.getStr("base_md5", ""), "validation failed for patch: "))
    {
      for (int j = 0; j < blk.blockCount(); j++)
        if (!blk.getBlock(j)->isEmpty())
          desc.addBlock(blk.getBlock(j)->getBlockName())->setFrom(blk.getBlock(j));
        else
          desc.removeBlock(blk.getBlock(j)->getBlockName());
      desc.setBool("optimized", false);

      const char *fn = dd_get_fname(desc_fn), *fn_ext = dd_get_fname_ext(fn);
      G_UNUSED(fn);
      G_UNUSED(fn_ext);
      G_UNUSED(pkg_folder);
      debug("%.*s: patched %d names (%s) -> %d total", fn_ext ? fn_ext - fn : strlen(fn), fn, blk.blockCount(), pkg_folder,
        desc.blockCount());
    }
  }
}
void gameres_final_optimize_desc(DataBlock &desc, const char *label)
{
  if (desc.getBool("optimized", true) || desc.blockCount() + desc.paramCount() == 0)
    return;

  MemorySaveCB cwr(64 << 10);
  desc.removeParam("optimized");
  if (!desc.saveToStream(cwr))
  {
    logerr("%s: failed to save %s=%p to memory stream", __FUNCTION__, label, &desc);
    return;
  }
  desc.reset();

  MemoryLoadCB crd(cwr.takeMem(), true);
  if (desc.loadFromStream(crd, label))
    debug("%s: optimized to ROM BLK format, size=%dK, %d names", label, crd.getTargetDataSize() >> 10, desc.blockCount());
  else
    logerr("%s: failed to reload %s=%p from memory stream (sz=%d)", __FUNCTION__, label, &desc, crd.getTargetDataSize());
}
