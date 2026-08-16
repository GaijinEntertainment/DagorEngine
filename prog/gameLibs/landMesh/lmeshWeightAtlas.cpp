// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <landMesh/lmeshWeightAtlas.h>
#include <3d/ddsxTex.h>
#include <3d/ddsFormat.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_driverDesc.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_shaderConstants.h>
#include <ioSys/dag_memIo.h>
#include <loadDDSx/uniTexCrd.h>
#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h> // saturate
#include <math/dag_color.h>
#include <image/dag_dxtCompress.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <convert/fastDXT/rygDXT.h>
#include <EASTL/unique_ptr.h>

static constexpr int DET_NUM = LandWeightAtlas::DET_NUM;

#define GLOBAL_VARS_LIST          \
  VAR(land_weight_cells_const_no) \
  VAR(land_weight_uv_const_no)    \
  VAR(land_weight_ofs_const_no)   \
  VAR(land_weight_cell_const_no)  \
  VAR(land_weight_fold_const_no)  \
  VAR(land_weight_src_const_no)   \
  VAR(land_weight_src2_const_no)  \
  VAR(land_weight_pack_tc)        \
  VAR(land_weight_pack_ch)        \
  VAR(land_weight_pack_num_tex)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

// The layout constants sit at hardcoded ps registers (see land_weight_inc.dshl),
// so they are written straight there rather than through the shader var system.
static void set_ps_const4(const ShaderVariableInfo &slot, float x, float y, float z, float w)
{
  const Color4 v(x, y, z, w);
  if (slot.get_var_id() >= 0)
    d3d::set_ps_const(slot.get_int(), &v.r, 1);
}

bool land_weight_atlas_cpu_pack()
{
  return d3d::check_texformat(TEXFMT_DXT1) &&
         !dgs_get_settings()->getBlockByNameEx("graphics")->getBool("landWeightAtlasForceGpu", false);
}

// what the exported pair of weight textures holds: four channels in the first,
// two of the second's; any channel a cell blends past those is not stored
static constexpr int LEGACY_TEX1_CHANNELS = 4, LEGACY_TEX2_CHANNELS = 2;
G_STATIC_ASSERT(LEGACY_TEX1_CHANNELS + LEGACY_TEX2_CHANNELS < DET_NUM);

// ---- atlas -------------------------------------------------------------------

LandWeightAtlas::LandWeightAtlas(int cells_x, int cells_y, int elem_size, unsigned tex_cflg, bool reserve_for_edit) :
  cellsX(cells_x),
  cellsY(cells_y),
  elemW(elem_size),
  pages(midmem),
  pageScratch(midmem),
  freePages(midmem),
  reserved(reserve_for_edit),
  extraTexCflg(tex_cflg)
{
  // sample centers span [border, border + elem); the widest bilinear footprint
  // reaches one texel out on each side, which the border covers - so the page
  // is exactly data plus borders, block aligned
  pageW = (elem_size + 2 * LAND_WEIGHT_BORDER + 3) & ~3;
  // where DXT1 exists the CPU packs into it; where it does not, the pages are
  // drawn into an uncompressed target instead
  texFmt = land_weight_atlas_cpu_pack() ? TEXFMT_DXT1 : (TEXFMT_A8R8G8B8 | TEXCF_RTARGET);
  pageBytes = (pageW / 4) * (pageW / 4) * 8; // DXT1, the only thing the CPU packs
  records.resize(cells_x * cells_y * WORDS_PER_CELL);
  mem_set_0(records);
  if (packedOnCpu()) // otherwise the pages are rendered straight into the atlas
  {
    pages.resize(cells_x * cells_y * LAND_WEIGHT_PAGES_PER_CELL * pageBytes);
    mem_set_0(pages);
    pageScratch.resize(pageW * pageW * 4);
  }
}

// A device reset recreates the buffer empty. The records are the one thing we
// always keep on the CPU (the pages are not), so this is free; the atlas
// texture comes back from its system copy or from a reload of the level.
// The atlas owns the callback: a driver that takes it only calls destroySelf()
// when a second one replaces it, so handing ownership over would leak it.
struct LandWeightAtlas::CellsReload final : public Sbuffer::IReloadData
{
  const LandWeightAtlas &atlas;
  CellsReload(const LandWeightAtlas &a) : atlas(a) {}
  void reloadD3dRes(Sbuffer *sb) override { sb->updateData(0, data_size(atlas.records), atlas.records.data(), VBLOCK_WRITEONLY); }
  void destroySelf() override {}
};

bool LandWeightAtlas::packedOnCpu() const { return (texFmt & TEXFMT_MASK) == TEXFMT_DXT1; }

LandWeightAtlas::~LandWeightAtlas()
{
  del_d3dres(tex);
  del_d3dres(cellsBuf);
  delete cellsReload; // the buffer only hands it back when another one replaces it
  delete packRenderer;
}

// A raw bind, like the landmesh textures: the slot is above the samplers the
// renderer nulls per cell, so nothing in a land pass overwrites it. The atlas
// layout is published from here rather than at upload, so that the atlas being
// rendered from owns the globals even while another one is built (the editor
// rebuilding a map, a test packing its own).
void LandWeightAtlas::bindCells() const
{
  setShaderVars();
  if (cellsBuf && land_weight_cells_const_noVarId.get_var_id() >= 0)
    d3d::set_buffer(STAGE_PS, land_weight_cells_const_noVarId.get_int(), cellsBuf);
}

int LandWeightAtlas::allocPage()
{
  if (freePages.size())
  {
    const int page = freePages.back();
    freePages.pop_back();
    return page;
  }
  if (pages.size() && (pageCount + 1) * pageBytes > pages.size())
  {
    logerr("land weight atlas: out of pages (%d)", pageCount);
    return -1;
  }
  if (pageCount > LAND_WEIGHT_PAGE_MASK) // a cell record holds a page number in 12 bits
  {
    logerr("land weight atlas: more than %d pages, the map has too many blended cells", LAND_WEIGHT_PAGE_MASK + 1);
    return -1;
  }
  return pageCount++;
}

int LandWeightAtlas::addPage(const uint8_t *const planes[DET_NUM], int first_ch, int used)
{
  const int page = allocPage();
  if (page < 0) // a page every failing cell shares would be worse than none
    return page;

  for (int t = 0; t < pageW * pageW; t++)
  {
    uint8_t *d = &pageScratch[t * 4]; // rygDXT wants BGRA bytes
    for (int c = 0; c < LAND_WEIGHT_CHANNELS_PER_PAGE; c++)
      d[2 - c] = c < used ? planes[first_ch + c][t] : 0;
    d[3] = 255;
  }
  rygDXT::CompressImageDXT1(pageScratch.data(), &pages[page * pageBytes], pageW, pageW, rygDXT::STB_DXT_HIGHQUAL, (pageW / 4) * 8);
  return page;
}

void LandWeightAtlas::pageTexel(int page, int x, int y, float rgb[LAND_WEIGHT_CHANNELS_PER_PAGE]) const
{
  uint8_t bgra[4 * 16];
  decompress_dxt(bgra, 4, 4, 4 * 4, (unsigned char *)&pages[page * pageBytes + ((y / 4) * (pageW / 4) + x / 4) * 8], true);
  const uint8_t *t = &bgra[((y & 3) * 4 + (x & 3)) * 4];
  for (int c = 0; c < LAND_WEIGHT_CHANNELS_PER_PAGE; c++)
    rgb[c] = t[2 - c] / 255.f; // bgra
}

bool LandWeightAtlas::sampleCell(int index, int x, int y, float w[DET_NUM]) const
{
  for (int ch = 0; ch < DET_NUM; ch++)
    w[ch] = 0;
  if (uint32_t(index) >= uint32_t(cellsX * cellsY) || !pages.size())
    return false;

  const uint32_t rec = records[index * WORDS_PER_CELL];
  const int count = ((rec >> LAND_WEIGHT_COUNT_SHIFT) & LAND_WEIGHT_COUNT_MASK) + 1;
  if (count == 1)
  {
    w[0] = 1;
    return true;
  }
  const int px = clamp(x, 0, elemW - 1) + LAND_WEIGHT_BORDER, py = clamp(y, 0, elemW - 1) + LAND_WEIGHT_BORDER;
  float sum = 0;
  for (int c = 0; c < count - 1; c++) // the last channel is what the others leave of 1
  {
    const int page = (rec >> ((c / LAND_WEIGHT_CHANNELS_PER_PAGE) * LAND_WEIGHT_PAGE1_SHIFT)) & LAND_WEIGHT_PAGE_MASK;
    float rgb[LAND_WEIGHT_CHANNELS_PER_PAGE];
    pageTexel(page, px, py, rgb);
    w[c] = rgb[c % LAND_WEIGHT_CHANNELS_PER_PAGE];
    sum += w[c];
  }
  w[count - 1] = clamp(1.f - sum, 0.f, 1.f);
  return true;
}

void LandWeightAtlas::setCellWeights(int index, const uint8_t *const planes[DET_NUM], const uint8_t *det_tex_ids, int num_tex)
{
  if (uint32_t(index) >= uint32_t(cellsX * cellsY) || !pages.size())
    return;
  writeRecord(index, det_tex_ids, num_tex, planes);
}

// planes are null when the pages are rendered on the GPU afterwards
void LandWeightAtlas::writeRecord(int index, const uint8_t *det_tex_ids, int num_tex, const uint8_t *const planes[DET_NUM])
{
  if (uint32_t(index) >= uint32_t(cellsX * cellsY))
    return;
  uint32_t *rec = &records[index * WORDS_PER_CELL];

  const int pageCnt = LandWeightAtlas::pagesFor((*rec >> LAND_WEIGHT_COUNT_SHIFT & LAND_WEIGHT_COUNT_MASK) + 1);
  for (int p = 0; p < pageCnt; p++) // recycle the pages the cell used before
    freePages.push_back((*rec >> (p * LAND_WEIGHT_PAGE1_SHIFT)) & LAND_WEIGHT_PAGE_MASK);
  mem_set_0(make_span(rec, WORDS_PER_CELL));

  const int count = clamp(num_tex, 1, (int)DET_NUM); // a cell always renders one landclass
  // Channels are dense, so a page is simply the next three of them. The last
  // channel is derived and needs none, but the ones that fit the pages already
  // claimed are packed anyway - reading a single weight is a sample cheaper.
  const int newPages = LandWeightAtlas::pagesFor(count), stored = min(count, newPages * LAND_WEIGHT_CHANNELS_PER_PAGE);
  *rec = uint32_t(count - 1) << LAND_WEIGHT_COUNT_SHIFT;
  for (int p = 0; p < newPages; p++)
  {
    const int firstCh = p * LAND_WEIGHT_CHANNELS_PER_PAGE, used = min(stored - firstCh, (int)LAND_WEIGHT_CHANNELS_PER_PAGE);
    const int page = planes ? addPage(planes, firstCh, used) : allocPage();
    if (page < 0) // out of pages: the cell keeps the one landclass it starts with
    {
      for (int r = 0; r < p; r++) // give back what this record already claimed
        freePages.push_back((*rec >> (r * LAND_WEIGHT_PAGE1_SHIFT)) & LAND_WEIGHT_PAGE_MASK);
      *rec = 0;
      break;
    }
    *rec |= uint32_t(page) << (p * LAND_WEIGHT_PAGE1_SHIFT);
  }
  // the ids the shader will bind the landclass textures with once it does so
  // itself; nothing reads them yet
  for (int ch = 0; ch < DET_NUM; ch++)
    ((uint8_t *)(rec + 1))[ch] = ch < count && det_tex_ids ? det_tex_ids[ch] : 0xFF;
}

bool LandWeightAtlas::upload()
{
  if (packedOnCpu() && !pages.size()) // published already, the CPU copy is gone
    return false;
  // reserved atlases are sized for the worst case up front, so painting never
  // has to recreate the texture (and invalidate the renderer's cell states)
  const int n = max(reserved && pages.size() ? (int)pages.size() / pageBytes : pageCount, 1);
  // Any row length the driver allows can hold the format's 4096 pages in the
  // height it allows, so the row length is free to be chosen for packing: the
  // tail of the last row is the only waste, and a row longer than needed only
  // grows it (at 16K wide a tail can strand a whole megabyte). 8192 caps the
  // search; below it, take the length that packs n pages tightest. The same
  // cap on the height keeps the atlas valid when the desc overstates the
  // device limit.
  G_ASSERT_RETURN(pageW <= min(d3d::get_driver_desc().maxtexw, d3d::get_driver_desc().maxtexh), false);
  const int maxW = clamp(d3d::get_driver_desc().maxtexw, pageW, max(pageW, 8192)),
            maxH = clamp(d3d::get_driver_desc().maxtexh, pageW, max(pageW, 8192));
  const int maxPpr = clamp(maxW / pageW, 1, n);
  const int minPpr = clamp((n + maxH / pageW - 1) / (maxH / pageW), 1, maxPpr); // rows must fit the height
  pagesPerRow = maxPpr;
  for (int c = maxPpr, best = maxPpr * ((n + maxPpr - 1) / maxPpr); c >= minPpr && best > n; c--)
  {
    const int cap = c * ((n + c - 1) / c);
    if (cap < best)
    {
      best = cap;
      pagesPerRow = c;
    }
  }
  const int rows = (n + pagesPerRow - 1) / pagesPerRow;
  const int w = pagesPerRow * pageW, h = rows * pageW;
  if (h > maxH)
  {
    logerr("land weight atlas: %d pages need %dx%d, more than the driver's %dx%d", n, w, h, maxW, maxH);
    return false;
  }
  if (!tex)
  {
    // the rendered path can carry no system copy, so the owner's flags apply
    // only where the CPU packs
    tex = d3d::create_tex(nullptr, w, h, texFmt | (packedOnCpu() ? extraTexCflg : 0), 1, "land_weight_atlas");
  }
  if (!tex)
    return false;
  if (!cellsBuf)
  {
    cellsBuf = d3d::buffers::create_persistent_sr_byte_address(records.size(), "land_weight_cells");
    if (!cellsBuf) // nothing would bind the slot the shader reads its cell from
      return false;
    if (!cellsReload)
      cellsReload = new CellsReload(*this);
    cellsBuf->setReloadCallback(cellsReload);
  }
  cellsBuf->updateData(0, data_size(records), records.data(), VBLOCK_WRITEONLY);

  atlasW = w;
  atlasH = h;
  if (!packedOnCpu()) // renderCell() draws into it, there is nothing to copy up
    return true;

  uint8_t *dst = nullptr;
  int stride = 0;
  if (!tex->lockimg((void **)&dst, stride, 0, TEXLOCK_WRITE) || !dst)
    return false;
  const int rowsPerPage = pageW / 4;
  const int rowBytes = pageBytes / rowsPerPage;
  for (int p = 0; p < pageCount; p++)
  {
    const int gx = (p % pagesPerRow) * rowBytes, gy = (p / pagesPerRow) * rowsPerPage;
    const uint8_t *src = &pages[p * pageBytes];
    for (int row = 0; row < rowsPerPage; row++)
      memcpy(dst + (gy + row) * stride + gx, src + row * rowBytes, rowBytes);
  }
  tex->unlockimg();
  return true;
}

void LandWeightAtlas::setCellMapping(float cell_size, float grid_cell_size, const Point3 &mesh_offset,
  const IPoint2 &cell_origin) const
{
  const float invCell = cell_size > 0 ? 1.f / cell_size : 0.f;
  set_ps_const4(land_weight_cell_const_noVarId, invCell, -(mesh_offset.x * invCell + cell_origin.x),
    -(mesh_offset.z * invCell + cell_origin.y), (float)cellsX);
  // mirroring reflects about 0 and about half a grid cell inside the far edge,
  // which one triangle wave of this period does for any number of reflections
  const Point2 period(2 * (cellsX - 0.5f * grid_cell_size * invCell), 2 * (cellsY - 0.5f * grid_cell_size * invCell));
  set_ps_const4(land_weight_fold_const_noVarId, period.x, period.y, safeinv(period.x), safeinv(period.y));
}

void LandWeightAtlas::setShaderVars() const
{
  set_ps_const4(land_weight_uv_const_noVarId, (float)elemW / atlasW, (float)elemW / atlasH, (float)pageW / atlasW,
    (float)pageW / atlasH);
  set_ps_const4(land_weight_ofs_const_noVarId, (float)LAND_WEIGHT_BORDER / atlasW, (float)LAND_WEIGHT_BORDER / atlasH,
    (float)pagesPerRow, 0);
}

void LandWeightAtlas::dropCpuPages()
{
  clear_and_shrink(pages);
  clear_and_shrink(pageScratch);
  clear_and_shrink(freePages);
}
