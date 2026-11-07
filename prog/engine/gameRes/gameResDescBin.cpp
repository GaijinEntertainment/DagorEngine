// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_gameResHooks.h>
#include <util/dag_fileMd5Validate.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_rwSpinLock.h>
#include <osApiWrappers/dag_rwLock.h>
#include <util/dag_string.h>
#include <EASTL/utility.h>

DataBlock gameres_rendinst_desc;
DataBlock gameres_dynmodel_desc;

// loading is serialized on one job manager thread, but the main thread may pull a resource and read the index meanwhile;
// the lock suits that ratio, rare writes against thousands of reads
static NoWritersSpinLockReadWriteLock riDescIndexLock;
static Tab<int> riDescIndex(inimem);     // block index, -1 for none
static uint32_t riDescIndexedBlocks = 0; // 0 while the index holds nothing

void gameres_invalidate_ri_desc_index(const DataBlock &desc)
{
  if (&desc != &gameres_rendinst_desc)
    return;
  ScopedLockWriteTemplate<NoWritersSpinLockReadWriteLock> lock(riDescIndexLock);
  clear_and_shrink(riDescIndex);
  riDescIndexedBlocks = 0;
}

static void build_ri_desc_index()
{
  ScopedLockWriteTemplate<NoWritersSpinLockReadWriteLock> lock(riDescIndexLock);
  const uint32_t blocks = gameres_rendinst_desc.blockCount();
  int maxNameId = -1;
  for (uint32_t i = 0; i < blocks; i++)
    maxNameId = max(maxNameId, gameres_rendinst_desc.getBlock(i)->getBlockNameId());
  riDescIndex.resize(maxNameId + 1);
  mem_set_ff(riDescIndex); // block 0 is valid, so -1 is the empty slot
  for (uint32_t i = 0; i < blocks; i++)
  {
    const DataBlock *b = gameres_rendinst_desc.getBlock(i);
    if (int &slot = riDescIndex[b->getBlockNameId()]; slot < 0)
      slot = i;
    else if (DAGOR_DBGLEVEL > 0)
      logerr("riDesc: duplicate block <%s>, only the first one is used", b->getBlockName());
  }
  riDescIndexedBlocks = blocks;
}

const DataBlock *gameres_find_ri_desc_block(const char *name)
{
  ScopedLockReadTemplate<NoWritersSpinLockReadWriteLock> lock(riDescIndexLock);
  const int nameId = gameres_rendinst_desc.getNameId(name); // -1 for a null name too
  if (nameId < 0)
    return nullptr;
  // by name id, so the dev-only singleBlockChecking pass over every block is skipped
  if (riDescIndexedBlocks != gameres_rendinst_desc.blockCount())
    return gameres_rendinst_desc.getBlockByNameId(nameId);
  if (nameId >= riDescIndex.size() || riDescIndex[nameId] < 0)
    return nullptr;
  const DataBlock *b = gameres_rendinst_desc.getBlock(riDescIndex[nameId]);
  if (DAGOR_LIKELY(b->getBlockNameId() == nameId))
    return b;
  // a mutation that keeps the block count leaves the epoch above satisfied and the slot pointing at whatever block took that position;
  // say so in a dev build, and answer from the desc rather than answer a block under another name
  G_ASSERTF(false, "riDesc index is stale for <%s>", name);
  return gameres_rendinst_desc.getBlockByNameId(nameId);
}

DataBlock *gameres_add_ri_desc_block(const char *name)
{
  // null rather than a block, in release too - G_ASSERTF_RETURN drops only the report there:
  // the caller faults on the return value instead of riDesc holding two blocks under one name,
  // which the index and a rebuild would answer differently. Neither case happens today
  G_ASSERTF_RETURN(name && *name && !gameres_find_ri_desc_block(name), nullptr, // find takes its own lock
    "bad or duplicate riDesc name <%s>", name);

  ScopedLockWriteTemplate<NoWritersSpinLockReadWriteLock> lock(riDescIndexLock);
  const uint32_t at = gameres_rendinst_desc.blockCount();
  // addNewBlock, not addBlock, which would scan every block to prove the name is absent;
  // it appends, so the new block sits at the old block count
  DataBlock *b = gameres_rendinst_desc.addNewBlock(name); // under the lock: interns a name find_block reads
  if (!b || !riDescIndexedBlocks)
    return b;
  const int nameId = b->getBlockNameId();
  if (nameId >= riDescIndex.size())
  {
    const int was = riDescIndex.size();
    riDescIndex.resize(nameId + 1);
    mem_set_ff(make_span(riDescIndex.data() + was, riDescIndex.size() - was));
  }
  riDescIndex[nameId] = at;
  riDescIndexedBlocks = gameres_rendinst_desc.blockCount();
  return b;
}

void gameres_append_desc(DataBlock &desc, const char *desc_fn, const char *pkg_folder, bool allow_override)
{
  if (gamereshooks::on_gameres_pack_load_confirm && !gamereshooks::on_gameres_pack_load_confirm(desc_fn, false))
    return;
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
      // here, not at entry: every exit above refuses the append without touching a block,
      // and dropping the index for one of those puts every later lookup on the scan path until the next optimize
      gameres_invalidate_ri_desc_index(desc);
      // an empty desc takes the file whole, so hand its storage over instead of rebuilding every block and param in place;
      // blk.paramCount() is in the test because the loop below copies blocks only, and a move brings the file's params too
      if (!patch_mode && desc.blockCount() == 0 && desc.paramCount() == 0 && blk.paramCount() == 0)
      {
        // blk is topmost, so the move swaps: desc takes the blocks and blk is left empty, which skips the loop below
        desc = eastl::move(blk);
        processed_cnt = desc.blockCount();
      }
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
  if (gamereshooks::on_gameres_pack_load_confirm && !gamereshooks::on_gameres_pack_load_confirm(patch_desc_fn, false))
    return;
  String resolved;
  if (!dd_file_exists(patch_desc_fn) && gamereshooks::on_missing_pack_resolve &&
      gamereshooks::on_missing_pack_resolve(patch_desc_fn, resolved))
    patch_desc_fn = resolved;
  if (dd_file_exists(patch_desc_fn))
  {
    DataBlock blk;
    if (blk.load(patch_desc_fn) && !blk.isEmpty() &&
        validate_file_md5_hash(desc_fn, blk.getStr("base_md5", ""), "validation failed for patch: "))
    {
      gameres_invalidate_ri_desc_index(desc); // here, not at entry: the exits above refuse the patch, blocks untouched
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
static void final_optimize_desc(DataBlock &desc, const char *label, dag::ConstSpan<const char *> strip_sub_blocks)
{
  if (desc.getBool("optimized", true) || desc.blockCount() + desc.paramCount() == 0)
    return;

  if (!strip_sub_blocks.empty())
  {
    int removed = 0;
    for (int i = 0, e = desc.blockCount(); i < e; i++)
    {
      bool any = false;
      for (const char *nm : strip_sub_blocks)
        any |= desc.getBlock(i)->removeBlock(nm);
      removed += any ? 1 : 0;
    }
    debug("%s: stripped sub-blocks from %d of %d blocks", label, removed, desc.blockCount());
  }

  MemorySaveCB cwr(64 << 10);
  desc.removeParam("optimized");
  if (!desc.saveToStreamDedup(cwr))
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

void gameres_final_optimize_desc(DataBlock &desc, const char *label, dag::ConstSpan<const char *> strip_sub_blocks)
{
  gameres_invalidate_ri_desc_index(desc); // the reload recreates every block and renumbers the name ids
  final_optimize_desc(desc, label, strip_sub_blocks);
  if (&desc == &gameres_rendinst_desc)
    build_ri_desc_index(); // only here: the optimize is the one point where the desc is settled
}

void gameres_reset_desc(DataBlock &desc)
{
  gameres_invalidate_ri_desc_index(desc);
  desc.reset();
}
