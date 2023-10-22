#include <3d/tql.h>
#include <3d/dag_drv3d.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <util/dag_string.h>
#include "loadDDSx/genmip.h"
#include "texMgrData.h"

static void init_ddsx_loader_and_genmip();
static void reload_texstubs_from_mem(bool full_reset = true);

namespace
{
static constexpr int MAX_EXPLICIT_STUBS = 14;
struct OnePixDDSx
{
  ddsx::Header hdr;
  E3DCOLOR p[8];
};
static OnePixDDSx opd[tql::IMPLICIT_STUBS + MAX_EXPLICIT_STUBS];
} // namespace

void tql::initTexStubs()
{
  if (!d3d::is_stub_driver())
    init_ddsx_loader_and_genmip();

  const DataBlock &b = *dgs_get_settings()->getBlockByNameEx("texStreaming");
  if (!b.getBool("enableStreaming", true))
    return;

  int gpu_mem_sz_kb = d3d::get_free_dedicated_gpu_memory_size_kb();
  int mem_reserve_promille = b.getInt("memReservePromille", 50);
  if (gpu_mem_sz_kb <= 0)
  {
    gpu_mem_sz_kb = d3d::get_dedicated_gpu_memory_size_kb();
    mem_reserve_promille *= 2;
  }

  memset(opd, 0, sizeof(opd));
  for (auto &d : opd)
  {
    d.hdr.label = _MAKE4C('DDSx');
    d.hdr.d3dFormat = 0x15;
    d.hdr.flags = 0x11;
    d.hdr.w = 1;
    d.hdr.h = 1;
    d.hdr.levels = 1;
    d.hdr.bitsPerPixel = 32;
  }

  // tex
  opd[RES3D_TEX].hdr.memSz = 4;

  // cube tex
  opd[RES3D_CUBETEX].hdr.flags |= ddsx::Header::FLG_CUBTEX;
  opd[RES3D_CUBETEX].hdr.memSz = 4 * 6;

  // vol tex
  opd[RES3D_VOLTEX].hdr.flags |= ddsx::Header::FLG_VOLTEX;
  opd[RES3D_VOLTEX].hdr.depth = 1;
  opd[RES3D_VOLTEX].hdr.memSz = 4;

  // arr tex
  opd[RES3D_ARRTEX].hdr.flags |= ddsx::Header::FLG_ARRTEX;
  opd[RES3D_ARRTEX].hdr.depth = 1;
  opd[RES3D_ARRTEX].hdr.memSz = 4;

  // reserve stubs
  texStub.reserve(IMPLICIT_STUBS + b.blockCount());
  texStub.resize(IMPLICIT_STUBS);

  int tex_flg = TEXCF_SRGBREAD | TEXCF_SRGBWRITE;
  E3DCOLOR defCol;
  defCol.u = 0x00808080; // gray, transparent
  // stubIdx=0: tex, transparent
  opd[RES3D_TEX].p[0] = b.getE3dcolor("defStubTexColor", defCol);
  texStub[RES3D_TEX] = d3d::alloc_ddsx_tex(opd[RES3D_TEX].hdr, tex_flg, 0, 1, "texStub:tex");

  // stubIdx=1: cubetex, transparent
  opd[RES3D_CUBETEX].p[0] = b.getE3dcolor("defStubCubeTexColor", defCol);
  texStub[RES3D_CUBETEX] = d3d::alloc_ddsx_tex(opd[RES3D_CUBETEX].hdr, tex_flg, 0, 1, "texStub:cube");

  // stubIdx=2: voltex, transparent
  opd[RES3D_VOLTEX].p[0] = b.getE3dcolor("defStubVolTexColor", defCol);
  texStub[RES3D_VOLTEX] = d3d::alloc_ddsx_tex(opd[RES3D_VOLTEX].hdr, tex_flg, 0, 1, "texStub:vol");

  // stubIdx=3: arrtex, transparent
  opd[RES3D_ARRTEX].p[0] = b.getE3dcolor("defStubArrTexColor", defCol);
  texStub[RES3D_ARRTEX] = d3d::alloc_ddsx_tex(opd[RES3D_ARRTEX].hdr, tex_flg, 0, 1, "texStub:arr");

  // app-defined stubs
  for (int i = 0, nid = b.getNameId("stubTex"); i < b.blockCount(); i++)
    if (b.getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock &b2 = *b.getBlock(i);
      int idx = b2.getInt("idx");
      const char *ttype = b2.getStr("type", "tex"); // "cube", "vol", "arr"
      E3DCOLOR c = b2.getE3dcolor("color", E3DCOLOR(0, 0, 0, 0));
      bool gamma1 = b2.getBool("gamma1", false);

      if (idx < 1 || idx > MAX_EXPLICIT_STUBS)
      {
        fatal("bad idx:i=%d in block #%d of texStreaming (1..%d allowed)", idx, i, MAX_EXPLICIT_STUBS);
        continue;
      }
      if (strcmp(ttype, "tex") != 0 && strcmp(ttype, "cube") != 0 && strcmp(ttype, "vol") != 0 && strcmp(ttype, "arr") != 0)
      {
        fatal("bad type:i=%s in block #%d of texStreaming", ttype, i);
        continue;
      }

      idx += IMPLICIT_STUBS - 1;
      while (idx >= texStub.size())
        texStub.push_back(NULL);
      if (texStub[idx])
      {
        if (!b2.getBool("alias", false))
          logerr("stubTex tag=%s tries to use already used idx=%d, skipped", b2.getStr("tag", NULL), idx - 2);
        continue;
      }

      int flg = (gamma1 ? 0 : TEXCF_SRGBREAD | TEXCF_SRGBWRITE);
      String name(0, "texStub%d:%s", idx - 2, ttype);

      if (strcmp(ttype, "tex") == 0)
        memcpy(&opd[idx], &opd[RES3D_TEX], sizeof(OnePixDDSx));
      else if (strcmp(ttype, "cube") == 0)
        memcpy(&opd[idx], &opd[RES3D_CUBETEX], sizeof(OnePixDDSx));
      else if (strcmp(ttype, "vol") == 0)
        memcpy(&opd[idx], &opd[RES3D_VOLTEX], sizeof(OnePixDDSx));
      else if (strcmp(ttype, "arr") == 0)
        memcpy(&opd[idx], &opd[RES3D_ARRTEX], sizeof(OnePixDDSx));
      else
        continue;

      if (gamma1)
        opd[idx].hdr.flags |= ddsx::Header::FLG_GAMMA_EQ_1;
      else
        opd[idx].hdr.flags &= ~ddsx::Header::FLG_GAMMA_EQ_1;
      opd[idx].p[0].u = c.u;
      texStub[idx] = d3d::alloc_ddsx_tex(opd[idx].hdr, flg, 0, 1, name);
    }

  texStub.shrink_to_fit();
  for (int i = 0; i < texStub.size(); i++)
    if (!texStub[i])
    {
      if (i >= IMPLICIT_STUBS)
        logerr("texStub %d is not defined, inited with default", i - IMPLICIT_STUBS);
      texStub[i] = texStub[0];
      memcpy(&opd[i], &opd[0], sizeof(*opd));
    }
  reload_texstubs_from_mem();

  // setup quota
  int mem_reserve_kb = b.getInt("memReserveKB", 128 << 10) + b.getInt("driverReserveKB", 0);
  if (int mem_mb = b.getInt("forceGpuMemMB", 0))
    if (mem_mb > 0)
      gpu_mem_sz_kb = mem_mb << 10;

  if (gpu_mem_sz_kb == 0)
    tql::mem_quota_kb = 0;
  else
    tql::mem_quota_kb = gpu_mem_sz_kb - max(gpu_mem_sz_kb * mem_reserve_promille / 1000, mem_reserve_kb);

  if (tql::mem_quota_kb < 0)
    tql::mem_quota_kb = 0;

  int min_q = b.getInt("minMemQuotaKB", 0);
  int max_q = b.getInt("maxMemQuotaKB", 8 << 20);
  if (tql::mem_quota_kb < min_q)
    tql::mem_quota_kb = min_q;
  if (tql::mem_quota_kb > max_q)
    tql::mem_quota_kb = max_q;
  if (int sz = b.getInt("forceMemQuotaKB", 0))
    tql::mem_quota_kb = sz;

  tql::mem_quota_limit_kb = INT32_MAX;
  if (b.paramExists("maxMemQuotaKB"))
  {
    tql::mem_quota_limit_kb = max_q;
  }
  if (b.paramExists("forceGpuMemMB") || b.paramExists("forceMemQuotaKB"))
  {
    tql::mem_quota_limit_kb = tql::mem_quota_kb;
  }

  tql::lq_not_more_than_split_bq = b.getBool("lqIsSplitBQ", tql::lq_not_more_than_split_bq);
  tql::lq_not_more_than_split_bq_difftex = b.getBool("lqIsSplitBQforDiffTex", tql::lq_not_more_than_split_bq_difftex);

  debug("initTexStubs: texStub.count=%d (+%d implicit), mem quota=%dK,  GPUmem=%dK, reserve max(%dK, %dK)",
    texStub.size() - IMPLICIT_STUBS, IMPLICIT_STUBS, tql::mem_quota_kb, gpu_mem_sz_kb, gpu_mem_sz_kb * mem_reserve_promille / 1000,
    mem_reserve_kb);
  tql::gpu_mem_reserve_kb = gpu_mem_sz_kb - tql::mem_quota_kb;
}

void tql::termTexStubs()
{
  BaseTexture *bt0 = texStub.size() ? texStub[0] : NULL;
  for (int i = 0; i < texStub.size(); i++)
    if (i == 0 || texStub[i] != bt0)
      del_d3dres(texStub[i]);
  clear_and_shrink(texStub);
  if (!d3d::is_stub_driver())
    d3d_genmip_reserve(0);
}

static void init_ddsx_loader_and_genmip()
{
  texmgr_internal::register_ddsx_load_implementation();

  auto [blockName, defaultValue] = d3d::get_driver_code()
                                     .map<eastl::pair<const char *, int>>(d3d::dx11, "directx", 0)
                                     .map(d3d::dx12, "dx12", 4 * 1024)
                                     .map(d3d::vulkan, "vulkan", 0)
                                     .map(d3d::metal, "video", 4096)
                                     .map(d3d::ps4, "ps4", 0)
                                     .map(d3d::ps5, "ps5", 2048)
                                     .map(d3d::any, nullptr, 0);
  int max_genmip_sz = 0;
  if (blockName)
  {
    max_genmip_sz = dgs_get_settings()->getBlockByNameEx(blockName)->getInt("max_genmip_tex_sz", defaultValue);
  }
  d3d_genmip_reserve(max_genmip_sz);
}

static void reload_texstubs_from_mem(bool full_reset)
{
  if (!full_reset)
    return;
  for (int i = 0; i < tql::texStub.size(); i++)
  {
    InPlaceMemLoadCB mcrd(&opd[i].p, sizeof(opd[i].p));
    unsigned u = opd[i].p[0].u;
    for (auto &p : opd[i].p)
      p.u = u;
    if (tql::texStub[i] && tql::texStub[i]->restype() == RES3D_ARRTEX)
      d3d_load_ddsx_to_slice(tql::texStub[i], 0, opd[0].hdr, mcrd, 0, 0, 0);
    else
      d3d::load_ddsx_tex_contents(tql::texStub[i], opd[i].hdr, mcrd, 0);
  }
}

#include <3d/dag_drv3dReset.h>
REGISTER_D3D_AFTER_RESET_FUNC(reload_texstubs_from_mem);
