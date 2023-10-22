#include <gameRes/dag_gameResOptionalPacks.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_gameResSystem.h>
#include "grpData.h"
#include <3d/dag_texMgr.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_fastNameMapTS.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_critSec.h>
#include <memory/dag_framemem.h>
#include <EASTL/vector_map.h>

namespace gameresprivate
{
void scanGameResPack(const char *filename);
void scanDdsxTexPack(const char *filename);
void registerOptionalGameResPack(const char *filename);
void registerOptionalDdsxTexPack(const char *filename);
void dumpOptionalMappigs(const char *pkg_name);
extern FastNameMapTS<false> resNameMap;
extern bool gameResPatchInProgress;
extern Tab<int> patchedGameResIds;
} // namespace gameresprivate
using gameres_optional::PackId;

MulticastEvent<void(const GameResource *, int res_id, const char *res_name)> gameres_optional::on_res_updated;

static FastNameMapTS<false> packNames;
static eastl::vector_map<unsigned, PackId, eastl::less<unsigned>> resToGrp;
static eastl::vector_map<unsigned, PackId, eastl::less<unsigned>> texToDxpBQ, texToDxpHQ;

void gameresprivate::registerOptionalGameResPack(const char *filename)
{
  int base_fnlen = strlen(filename) - 9;
  G_ASSERTF(strcmp(filename + base_fnlen, "cache.bin") == 0, "filename=%s base_fnlen=%d", filename, base_fnlen);
  VromReadHandle dump_data = ::vromfs_get_file_data(filename);
  if (!dump_data.data())
    return;
  InPlaceMemLoadCB crd(dump_data.data(), dump_data.size());

  using namespace gamerespackbin;
  GrpHeader ghdr;
  crd.read(&ghdr, sizeof(ghdr));
  if (ghdr.label != _MAKE4C('GRP2') && ghdr.label != _MAKE4C('GRP3'))
  {
    debug("no GRP2 label (hdr: 0x%x 0x%x 0x%x 0x%x)", ghdr.label, ghdr.descOnlySize, ghdr.fullDataSize, ghdr.restFileSize);
    return;
  }

  int pnid = packNames.addNameId(filename, base_fnlen);
  PackId packId = (PackId)pnid;
  G_ASSERTF_RETURN(pnid >= 0 && pnid == (int)packId, , "pnid=%d (-> %d) for %.*s", pnid, (int)packId, base_fnlen, filename);

  GrpData *gdata = (GrpData *)memalloc(ghdr.descOnlySize, tmpmem);
  crd.read(gdata, ghdr.descOnlySize);
  gdata->patchDescOnly(ghdr.label);

  // scan real-res
  for (const ResEntry &rre : gdata->resTable)
  {
    int id = resNameMap.getNameId(gdata->getName(rre.resId));
    if (id >= 0)
      resToGrp[id] = packId;
  }
  memfree(gdata, tmpmem);
}
void gameresprivate::registerOptionalDdsxTexPack(const char *filename)
{
  int base_fnlen = strlen(filename) - 9;
  G_ASSERTF(strcmp(filename + base_fnlen, "cache.bin") == 0, "filename=%s base_fnlen=%d", filename, base_fnlen);
  VromReadHandle dump_data = ::vromfs_get_file_data(filename);
  if (!dump_data.data())
    return;
  InPlaceMemLoadCB crd(dump_data.data(), dump_data.size());

  unsigned hdr[4];
  crd.read(hdr, sizeof(hdr));
  if (hdr[0] != _MAKE4C('DxP2') || (hdr[1] != 2 && hdr[1] != 3))
  {
    debug("bad DXP header 0x%x 0x%x 0x%x 0x%x at '%s'", hdr[0], hdr[1], hdr[2], hdr[3], filename);
    return;
  }

  RoNameMap *texNames = (RoNameMap *)memalloc(hdr[3], tmpmem);
  crd.read(texNames, hdr[3]);
  texNames->patchData(texNames);

  int pnid = packNames.addNameId(filename, base_fnlen);
  PackId packId = (PackId)pnid;
  G_ASSERTF_RETURN(pnid >= 0 && pnid == (int)packId, , "pnid=%d (-> %d) for %.*s", pnid, (int)packId, base_fnlen, filename);

  for (auto &nm : texNames->map)
    if (const char *nm_suffix = strchr(nm, '$'))
    {
      bool force_uhq = strncmp(nm_suffix + 1, "uhq", 3) == 0;
      if (!force_uhq && strncmp(nm_suffix + 1, "hq", 2) != 0)
        continue;
      if (TEXTUREID tid = get_managed_texture_id(String(0, "%.*s*", nm_suffix - nm.get(), nm.get())))
        if (force_uhq || texToDxpHQ.find(unsigned(tid)) == texToDxpHQ.cend())
          texToDxpHQ[unsigned(tid)] = packId;
    }
    else if (TEXTUREID tid = get_managed_texture_id(nm))
      texToDxpBQ[unsigned(tid)] = packId;

  memfree(texNames, tmpmem);
}
void gameresprivate::dumpOptionalMappigs(const char *pkg_name)
{
  debug("%s: optional %d res and %d(%d) tex in %d packs", //
    pkg_name, resToGrp.size(), texToDxpBQ.size(), texToDxpHQ.size(), packNames.nameCount());
}
static bool gameres_update_desc(DataBlock &desc, const char *desc_fn, const char *pkg_folder, //
  dag::ConstSpan<const char *> grp_fn, size_t prefix_len)
{
  if (dd_file_exists(desc_fn))
  {
    DataBlock blk;
    if (blk.load(desc_fn))
    {
      if (!blk.getBool("patchMode", false))
        return false;
      const char *fn = dd_get_fname(desc_fn), *fn_ext = dd_get_fname_ext(fn);
      int pack_nid = blk.getNameId("__pack"), grp_marked_cnt = 0;

      SmallTab<bool, framemem_allocator> grp_to_update;
      grp_to_update.reserve(blk.paramCount() - 1);
      dblk::iterate_params_by_name_id_and_type(blk, pack_nid, DataBlock::TYPE_STRING, [&](int pidx) {
        if (grp_marked_cnt >= grp_fn.size())
          return;
        const char *fn0 = blk.getStr(pidx);
        bool found = false;
        for (const char *fn : grp_fn)
          if (strcmp(fn0, fn + prefix_len) == 0)
          {
            found = true;
            break;
          }
        grp_to_update.push_back(found);
        if (found)
          grp_marked_cnt++;
      });
      if (grp_to_update.empty())
      {
        debug("%d GRP(s) not found in %s/%s", grp_fn.size(), pkg_folder, desc_fn);
        return false;
      }
      int processed_cnt = 0, replaced_cnt = 0;
      for (int j = 0; j < blk.blockCount(); j++)
      {
        unsigned pack_idx = blk.getBlock(j)->getIntByNameId(pack_nid, -1);
        if (pack_idx >= grp_to_update.size() || !grp_to_update[pack_idx])
          continue;
        if (desc.removeBlock(blk.getBlock(j)->getBlockName()))
          replaced_cnt++;
        desc.addNewBlock(blk.getBlock(j), blk.getBlock(j)->getBlockName());
        processed_cnt++;
      }
      desc.setBool("optimized", false);

      G_UNUSED(fn);
      G_UNUSED(fn_ext);
      debug("%.*s: added %d names and %d updated (%s) -> processed %d of %d total", fn_ext ? fn_ext - fn : strlen(fn), fn,
        processed_cnt - replaced_cnt, replaced_cnt, pkg_folder, processed_cnt, desc.blockCount());
      return replaced_cnt > 0;
    }
  }
  return false;
}

static void get_missing_packs_for_tex(TEXTUREID tid, Tab<PackId> &out_packs, bool incl_hq)
{
  auto found = texToDxpBQ.find(unsigned(tid));
  if (found != texToDxpBQ.cend() && find_value_idx(out_packs, found->second) < 0)
    out_packs.push_back(found->second);
  if (incl_hq)
  {
    found = texToDxpHQ.find(unsigned(tid));
    if (found != texToDxpHQ.cend() && find_value_idx(out_packs, found->second) < 0)
      out_packs.push_back(found->second);
  }
}
static bool get_missing_packs_for_res(dag::ConstSpan<int> res_ids, Tab<PackId> &out_packs, bool incl_model_tex)
{
  unsigned initial_size = out_packs.size();
  for (int id : res_ids)
  {
    auto found = resToGrp.find(id);
    if (found != resToGrp.cend() && find_value_idx(out_packs, found->second) < 0)
      out_packs.push_back(found->second);
  }
  if (incl_model_tex)
  {
    for (int id : res_ids)
    {
      const char *nm = gameresprivate::resNameMap.getName(id);
      const DataBlock *b = gameres_dynmodel_desc.getBlockByName(nm);
      if (!b)
        b = gameres_rendinst_desc.getBlockByName(nm);
      if (!b)
        continue;
      int tex_nid = b->getNameId("tex");
      if (const DataBlock *tex_b = b->getBlockByNameId(tex_nid))
        dblk::iterate_params_by_name_id_and_type(*tex_b, tex_nid, DataBlock::TYPE_STRING, [&](int param_idx) {
          if (TEXTUREID tid = get_managed_texture_id(tex_b->getStr(param_idx)))
            get_missing_packs_for_tex(tid, out_packs, true);
        });
    }
  }
  return out_packs.size() > initial_size;
}
bool gameres_optional::get_missing_packs_for_res(dag::ConstSpan<const char *> res_names, Tab<PackId> &out_packs, bool incl_model_tex)
{
  WinAutoLock lock(get_gameres_main_cs());
  SmallTab<int, framemem_allocator> res_ids;
  res_ids.reserve(res_names.size());
  for (const char *nm : res_names)
  {
    int id = gameresprivate::resNameMap.getNameId(nm);
    if (id >= 0)
      res_ids.push_back(id);
  }
  return get_missing_packs_for_res(res_ids, out_packs, incl_model_tex);
}
bool gameres_optional::get_missing_packs_for_res(const FastNameMap &res_names, Tab<PackId> &out_packs, bool incl_model_tex)
{
  WinAutoLock lock(get_gameres_main_cs());
  SmallTab<int, framemem_allocator> res_ids;
  res_ids.reserve(res_names.nameCount());
  iterate_names(res_names, [&res_ids](int, const char *nm) {
    int id = gameresprivate::resNameMap.getNameId(nm);
    if (id >= 0)
      res_ids.push_back(id);
  });
  return get_missing_packs_for_res(res_ids, out_packs, incl_model_tex);
}

bool gameres_optional::get_missing_packs_for_tex(dag::ConstSpan<TEXTUREID> tid_list, Tab<PackId> &out_packs, bool incl_hq)
{
  WinAutoLock lock(get_gameres_main_cs());
  unsigned initial_size = out_packs.size();
  for (TEXTUREID tid : tid_list)
    get_missing_packs_for_tex(tid, out_packs, incl_hq);
  return out_packs.size() > initial_size;
}
bool gameres_optional::get_missing_packs_for_used_tex(Tab<PackId> &out_packs, bool incl_hq)
{
  WinAutoLock lock(get_gameres_main_cs());
  unsigned initial_size = out_packs.size();
  for (TEXTUREID tid = first_managed_texture(1); tid != BAD_TEXTUREID; tid = next_managed_texture(tid, 1))
    get_missing_packs_for_tex(tid, out_packs, incl_hq);
  return out_packs.size() > initial_size;
}

const char *gameres_optional::get_pack_name(PackId pack_id) { return packNames.getName((int)pack_id); }
PackId gameres_optional::get_pack_name_id(const char *pack_name) { return (PackId)packNames.getNameId(pack_name); }

bool gameres_optional::update_gameres_from_ready_packs(dag::ConstSpan<PackId> pack_ids)
{
  WinAutoLock lock(get_gameres_main_cs());
  SmallTab<const char *, framemem_allocator> res_packs;
  res_packs.reserve(pack_ids.size());
  for (PackId pack_id : pack_ids)
    if (const char *pack_name = (unsigned)pack_id < packNames.nameCount() ? get_pack_name(pack_id) : nullptr)
    {
      const char *ext = dd_get_fname_ext(pack_name);
      if (ext && strcmp(ext, ".grp") == 0)
        res_packs.push_back(pack_name);
      else if (ext && strcmp(ext, ".bin") == 0)
      {
        gameresprivate::scanDdsxTexPack(pack_name);
        for (auto it = texToDxpBQ.cbegin(); it != texToDxpBQ.cend();)
          if (it->second == pack_id)
            it = texToDxpBQ.erase(it);
          else
            ++it;
        for (auto it = texToDxpHQ.cbegin(); it != texToDxpHQ.cend();)
          if (it->second == pack_id)
            it = texToDxpHQ.erase(it);
          else
            ++it;
      }
    }
  if (res_packs.size())
  {
    const char *pack_name = res_packs[0];
    const char *res_dir_substr = strstr(pack_name, "/res/");
    if (res_dir_substr)
    {
      String pkg_folder(0, "%.*s", int(res_dir_substr - pack_name), pack_name);
      res_dir_substr += 5;
      String riDesc_m(0, "%.*sriDesc.models.bin", int(res_dir_substr - pack_name), pack_name);
      String dmDesc_m(0, "%.*sdynModelDesc.models.bin", int(res_dir_substr - pack_name), pack_name);
      gameres_update_desc(gameres_rendinst_desc, riDesc_m, pkg_folder, res_packs, res_dir_substr - pack_name);
      gameres_update_desc(gameres_dynmodel_desc, dmDesc_m, pkg_folder, res_packs, res_dir_substr - pack_name);
    }

    gameresprivate::gameResPatchInProgress = true;
    for (const char *fn : res_packs)
    {
      gameresprivate::scanGameResPack(fn);
      PackId pack_id = (PackId)packNames.getNameId(fn);
      for (auto it = resToGrp.cbegin(); it != resToGrp.cend();)
        if (it->second == pack_id)
          it = resToGrp.erase(it);
        else
          ++it;
    }
    gameresprivate::gameResPatchInProgress = false;
    for (int rid : gameresprivate::patchedGameResIds)
      if (auto *f = find_gameres_factory(get_game_res_class_id(rid)))
        if (GameResource *r = f->discardOlderResAfterUpdate(rid))
          on_res_updated.fire(r, rid, gameresprivate::resNameMap.getName(rid));
    clear_and_shrink(gameresprivate::patchedGameResIds);
  }
  return true;
}

#include <util/dag_console.h>
static bool gameres_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("gameres", "update_pack", 2, 2)
  {
    const char *pack_name = argv[1];
    const char *ext = dd_get_fname_ext(pack_name);
    console::print_d("reload pack: %s", pack_name);

    auto pack_id = gameres_optional::get_pack_name_id(pack_name);
    if (!gameres_optional::update_gameres_from_ready_packs(make_span_const(&pack_id, 1)))
      console::print_d("unrecognized ext: %s", ext);
  }
  CONSOLE_CHECK_NAME("gameres", "list_missing_for_res", 2, 3)
  {
    const char *res_name = argv[1];
    Tab<gameres_optional::PackId> pack_ids;
    if (gameres_optional::get_missing_packs_for_res(make_span_const(&res_name, 1), pack_ids,
          argc > 2 ? console::to_bool(argv[2]) : true))
    {
      console::print_d("res %s: %d missing optional packs", res_name, pack_ids.size());
      for (auto id : pack_ids)
        console::print_d("  %s", gameres_optional::get_pack_name(id));
    }
    else
      console::print_d("no missing optional packs for res: %s", res_name);
  }
  CONSOLE_CHECK_NAME("gameres", "list_missing_for_tex", 2, 2)
  {
    const char *tex_name = argv[1];
    Tab<gameres_optional::PackId> pack_ids;
    if (TEXTUREID tid = get_managed_texture_id(tex_name))
    {
      if (gameres_optional::get_missing_packs_for_tex(make_span_const(&tid, 1), pack_ids))
      {
        console::print_d("tex %s: %d missing optional packs", tex_name, pack_ids.size());
        for (auto id : pack_ids)
          console::print_d("  %s", gameres_optional::get_pack_name(id));
      }
      else
        console::print_d("no missing optional packs for tex: %s", tex_name);
    }
    else
      console::print_d("unrecognized tex: %s", tex_name);
  }
  CONSOLE_CHECK_NAME("gameres", "list_missing_for_all_tex", 1, 2)
  {
    Tab<gameres_optional::PackId> pack_ids;
    if (gameres_optional::get_missing_packs_for_used_tex(pack_ids, argc < 2 || strcmp(argv[1], "hq") == 0))
    {
      console::print_d("used textures: %d missing optional packs", pack_ids.size());
      for (auto id : pack_ids)
        console::print_d("  %s", gameres_optional::get_pack_name(id));
    }
    else
      console::print_d("no missing optional packs");
  }
  return found;
}
REGISTER_CONSOLE_HANDLER(gameres_console_handler);
