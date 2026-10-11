// Copyright (C) Gaijin Games KFT.  All rights reserved.

// The driver-free half of the atlas: the records, the page packing and the
// packed form the level ships, which is what the exporter links; the GPU
// residency lives in lmeshWeightAtlasGpu.cpp (the link rule sits in
// lmeshWeightAtlasInternal.h).

#include "lmeshWeightAtlasInternal.h"
#include <drv/3d/dag_tex3d.h>
#include <ioSys/dag_genIo.h>
#include <image/dag_dxtCompress.h>
#include <image/dag_texPixel.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <convert/fastDXT/rygDXT.h>
#include <EASTL/unique_ptr.h>

static constexpr int DET_NUM = LandWeightAtlas::DET_NUM;
static constexpr int RAW_TEXEL_BYTES = LAND_WEIGHT_CHANNELS_PER_PAGE; // a Raw page keeps just the three weights

// ---- atlas -------------------------------------------------------------------

LandWeightAtlas::LandWeightAtlas(Pages pages_kind, int cells_x, int cells_y, int elem_size, unsigned tex_cflg, bool reserve_for_edit,
  int page_capacity) :
  cellsX(cells_x),
  cellsY(cells_y),
  elemW(elem_size),
  kind(pages_kind),
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
  pageBytes = kind == Pages::Raw ? pageW * pageW * RAW_TEXEL_BYTES : (pageW / 4) * (pageW / 4) * 8;
  records.resize(cells_x * cells_y * WORDS_PER_CELL);
  mem_set_0(records);
  if (kind == Pages::Dxt1 || kind == Pages::Raw) // otherwise the pages are rendered into the atlas or shipped in it
  {
    // a reserve past the budget would size an atlas that never uploads, and then nothing paints at all
    const int capacity = page_capacity > 0 ? page_capacity : min(cells_x * cells_y * LAND_WEIGHT_PAGES_PER_CELL, pageBudget(pageW));
    pages.resize(capacity * pageBytes);
    mem_set_0(pages);
    if (kind == Pages::Dxt1)
      pageScratch.resize(pageW * pageW * 4);
  }
}

LandWeightAtlas::~LandWeightAtlas()
{
  del_d3dres(tex);
  del_d3dres(cellsBuf);
  delete cellsReload; // the buffer only hands it back when another one replaces it
}

int LandWeightAtlas::pageBudget(int page_w)
{
  const int perSide = page_w > 0 ? MAX_TEX_SIDE / page_w : 0;
  return min(perSide * perSide, int(LAND_WEIGHT_PAGE_MASK) + 1); // a cell record holds a page number in 14 bits
}

// -1 past the CPU pages or the record's page range; not logged here, writeRecord() counts the cell for upload() to report
int LandWeightAtlas::allocPage()
{
  if (freePages.size())
  {
    const int page = freePages.back();
    freePages.pop_back();
    return page;
  }
  if (pages.size() && (pageCount + 1) * pageBytes > pages.size())
    return -1;
  if (pageCount > LAND_WEIGHT_PAGE_MASK)
    return -1;
  return pageCount++;
}

int LandWeightAtlas::addPage(const uint8_t *const planes[DET_NUM], int first_ch, int used)
{
  const int page = allocPage();
  if (page < 0) // a page every failing cell shares would be worse than none
    return page;

  if (kind == Pages::Raw)
  {
    uint8_t *d = &pages[page * pageBytes];
    for (int t = 0; t < pageW * pageW; t++, d += RAW_TEXEL_BYTES)
      for (int c = 0; c < LAND_WEIGHT_CHANNELS_PER_PAGE; c++)
        d[c] = c < used ? planes[first_ch + c][t] : 0;
    return page;
  }
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
  if (kind == Pages::Raw)
  {
    const uint8_t *t = &pages[page * pageBytes + (y * pageW + x) * RAW_TEXEL_BYTES];
    for (int c = 0; c < LAND_WEIGHT_CHANNELS_PER_PAGE; c++)
      rgb[c] = t[c] / 255.f;
    return;
  }
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
      cellsPastBudget++;
      break;
    }
    *rec |= uint32_t(page) << (p * LAND_WEIGHT_PAGE1_SHIFT);
  }
  // the packed-level loader reads these for the cells' landclass lists and
  // land_weight_cell_slot_id() resolves a slot's de-tiling record from them
  for (int ch = 0; ch < DET_NUM; ch++)
    recordLandclassIds(rec)[ch] = ch < count && det_tex_ids ? det_tex_ids[ch] : 0xFF;
}

// Any row length under the caps can hold the pages, so the row length is free
// to be chosen for packing: the tail of the last row is the only waste, and a
// row longer than needed only grows it (at 16K wide a tail can strand a whole
// megabyte). Take the length that packs n pages tightest.
int land_weight_choose_pages_per_row(int n, int page_w, int max_w, int max_h)
{
  const int maxPpr = clamp(max_w / page_w, 1, n);
  const int minPpr = clamp((n + max_h / page_w - 1) / (max_h / page_w), 1, maxPpr); // rows must fit the height
  int pagesPerRow = maxPpr;
  for (int c = maxPpr, best = maxPpr * ((n + maxPpr - 1) / maxPpr); c >= minPpr && best > n; c--)
  {
    const int cap = c * ((n + c - 1) / c);
    if (cap < best)
    {
      best = cap;
      pagesPerRow = c;
    }
  }
  return pagesPerRow;
}

bool LandWeightAtlas::chooseLayout(int max_tex)
{
  G_ASSERT_RETURN(kind == Pages::Raw, false); // export staging only: a resident atlas's layout is its texture's
  const int n = max(pageCount, 1);
  if (pageW > max_tex)
    return false;
  pagesPerRow = land_weight_choose_pages_per_row(n, pageW, max_tex, max_tex);
  atlasW = pagesPerRow * pageW;
  atlasH = ((n + pagesPerRow - 1) / pagesPerRow) * pageW;
  return atlasH <= max_tex;
}

void LandWeightAtlas::composeRawImage(Tab<TexPixel32> &dst) const
{
  G_ASSERT_RETURN(kind == Pages::Raw && pagesPerRow > 0, );
  dst.resize(atlasW * atlasH);
  mem_set_0(dst); // past the last page nothing is read, but ship deterministic
  for (int p = 0; p < pageCount; p++)
  {
    const int gx = (p % pagesPerRow) * pageW, gy = (p / pagesPerRow) * pageW;
    const uint8_t *src = &pages[p * pageBytes];
    for (int y = 0; y < pageW; y++)
      for (int x = 0; x < pageW; x++, src += RAW_TEXEL_BYTES)
      {
        TexPixel32 &d = dst[(gy + y) * atlasW + gx + x];
        d.r = src[0];
        d.g = src[1];
        d.b = src[2];
        d.a = 255;
      }
  }
}

void LandWeightAtlas::savePacked(IGenSave &cwr, int base_ofs, dag::ConstSpan<uint8_t> ddsx) const
{
  G_ASSERT(pagesPerRow > 0 && atlasH > 0); // finishForExport() chose the layout the ddsx was composed in
  // the legacy table first: a loader from before the packed form seeks every
  // cell to the one empty record behind it and, at a zero source texture
  // size, packs no atlas - the level loads without blending, and says so
  const int cells = cellsX * cellsY, stubOfs = cwr.tell() + cells * (int)sizeof(int) - base_ofs;
  for (int i = 0; i < cells; i++)
    cwr.writeInt(stubOfs);
  uint8_t stub[LEGACY_CELL_HDR];
  memset(stub, 0xFF, DET_NUM);                       // no landclass in any slot
  memset(stub + DET_NUM, 0, sizeof(stub) - DET_NUM); // and no texture bytes
  cwr.write(stub, sizeof(stub));
  cwr.writeInt(pagesPerRow);
  cwr.writeInt(pageCount);
  cwr.write(records.data(), data_size(records)); // the byte image the shader loads, see land_weight_atlas.hlsli
  cwr.writeInt(ddsx.size());
  cwr.write(ddsx.data(), ddsx.size());
}

bool LandWeightAtlas::readPacked(IGenLoad &crd, int cells, int &pages_per_row, int &page_count, SmallTab<uint32_t> &packed_records,
  Tab<uint8_t> *ddsx)
{
  crd.seekrel(cells * (int)sizeof(int) + LEGACY_CELL_HDR); // the legacy table, see savePacked()
  crd.readInt(pages_per_row);
  crd.readInt(page_count);
  clear_and_resize(packed_records, cells * WORDS_PER_CELL);
  crd.read(packed_records.data(), data_size(packed_records));
  const int len = crd.readInt();
  // a corrupt length must fail like a short read, not as a giant allocation
  if (len < 0 || (crd.getTargetDataSize() >= 0 && len > crd.getTargetDataSize() - crd.tell()))
    return false;
  if (ddsx)
  {
    ddsx->resize(len);
    crd.read(ddsx->data(), len);
  }
  else
    crd.seekrel(len);
  return pages_per_row > 0 && page_count >= 0 && page_count <= LAND_WEIGHT_PAGE_MASK + 1;
}

void LandWeightAtlas::dropCpuPages()
{
  clear_and_shrink(pages);
  clear_and_shrink(pageScratch);
  clear_and_shrink(freePages);
}

// ---- packing cells with borders from their neighbours -------------------------

LandWeightAtlasBuilder::LandWeightAtlasBuilder(int cells_x, int cells_y, int tex_size, int elem_size, unsigned tex_cflg) :
  cells(tmpmem), cellsX(cells_x), cellsY(cells_y), texSize(tex_size), elemSize(elem_size), texCflg(tex_cflg)
{
  cells.resize(cells_x * cells_y);
  for (auto &c : cells)
    memset(c.detTexIds, 0xFF, sizeof(c.detTexIds));
}

LandWeightAtlasBuilder::~LandWeightAtlasBuilder() {}

bool LandWeightAtlasBuilder::addCellWeights(int index, const uint8_t *det_tex_ids, const uint8_t *const planes[DET_NUM])
{
  G_ASSERT_RETURN(texSize == elemSize, false); // the export constructor: a plane is exactly the cell's texels
  if (uint32_t(index) >= uint32_t(cells.size()))
    return false;
  Cell &c = cells[index];
  memcpy(c.detTexIds, det_tex_ids, sizeof(c.detTexIds));
  int numTex = 0;
  for (int i = 0; i < DET_NUM; i++)
    if (c.detTexIds[i] != 0xFF)
      numTex++;
  for (int ch = 0; ch < numTex; ch++)
  {
    c.chan[ch].resize(texSize * texSize);
    memcpy(c.chan[ch].data(), planes[ch], texSize * texSize);
  }
  return true;
}

// derived weight of landclass lc_id at texel (x, y) of cell (cx, cy);
// null when the cell is out of the map or does not blend that landclass
const uint8_t *LandWeightAtlasBuilder::cellTexel(int cx, int cy, int lc_id, int x, int y) const
{
  if (uint32_t(cx) >= uint32_t(cellsX) || uint32_t(cy) >= uint32_t(cellsY))
    return nullptr;
  const Cell &c = cells[cy * cellsX + cx];
  for (int ch = 0; ch < DET_NUM; ch++)
    if (c.detTexIds[ch] == lc_id && c.chan[ch].size())
      return &c.chan[ch][clamp(y, 0, texSize - 1) * texSize + clamp(x, 0, texSize - 1)];
  return nullptr;
}

void LandWeightAtlasBuilder::packCells(LandWeightAtlas &a) const
{
  const int pageW = a.getCellTexSize();
  // border texels come from the neighbor cell blending the same landclass;
  // a neighbor without it means the landclass genuinely fades to 0 there
  auto fetch = [&](int ci, int ch, int px, int py) -> uint8_t {
    int cx = ci % cellsX, cy = ci / cellsX;
    int k[2] = {px - LAND_WEIGHT_BORDER, py - LAND_WEIGHT_BORDER}, cc[2] = {cx, cy};
    for (int axis = 0; axis < 2; axis++)
    {
      // a cell owns elemSize texels; the exported texture is that rounded up to
      // a power of two, so anything past the cell belongs to the neighbour even
      // where the texture still has rows of its own
      if (k[axis] < 0)
        cc[axis]--, k[axis] += elemSize;
      else if (k[axis] >= elemSize)
        cc[axis]++, k[axis] -= elemSize;
    }
    if (cc[0] == cx && cc[1] == cy)
      return cells[ci].chan[ch][k[1] * texSize + k[0]];
    if (uint32_t(cc[0]) >= uint32_t(cellsX) || uint32_t(cc[1]) >= uint32_t(cellsY)) // map edge: clamp own cell
      return cells[ci]
        .chan[ch][clamp(py - LAND_WEIGHT_BORDER, 0, elemSize - 1) * texSize + clamp(px - LAND_WEIGHT_BORDER, 0, elemSize - 1)];
    const uint8_t *t = cellTexel(cc[0], cc[1], cells[ci].detTexIds[ch], k[0], k[1]);
    return t ? *t : 0;
  };

  Tab<uint8_t> planeBuf(framemem_ptr());
  planeBuf.resize(DET_NUM * pageW * pageW);
  for (int ci = 0; ci < cells.size(); ci++)
  {
    const uint8_t *planes[DET_NUM];
    int numTex = 0;
    for (int ch = 0; ch < DET_NUM; ch++)
    {
      uint8_t *p = planeBuf.data() + ch * pageW * pageW;
      planes[ch] = p;
      if (!cells[ci].chan[ch].size())
      {
        memset(p, 0, pageW * pageW);
        continue;
      }
      numTex = ch + 1;
      for (int py = 0; py < pageW; py++)
        for (int px = 0; px < pageW; px++)
          p[py * pageW + px] = fetch(ci, ch, px, py);
    }
    a.setCellWeights(ci, planes, cells[ci].detTexIds, numTex);
  }
}

void LandWeightAtlasBuilder::selfCheck(const LandWeightAtlas &a) const
{
  // read every cell back the way the shader will: a change to how pages are
  // packed then shows up here, not as weights in the game. The error is what
  // DXT1 costs on three independent weight channels sharing a page - a cell
  // blending two landclasses packs about three times better than one blending
  // four, and the level's mix decides the number.
  double sse = 0, maxErr = 0;
  int64_t n = 0;
  for (int ci = 0; ci < cells.size(); ci++)
    for (int y = 0; y < elemSize; y += 5)
      for (int x = 0; x < elemSize; x += 5)
      {
        float w[DET_NUM];
        if (!a.sampleCell(ci, x, y, w))
          continue;
        for (int ch = 0; ch < DET_NUM; ch++)
        {
          const float ref = cells[ci].chan[ch].size() ? cells[ci].chan[ch][y * texSize + x] / 255.f : 0.f;
          const double err = fabsf(w[ch] - ref);
          sse += err * err;
          maxErr = max(maxErr, err);
          n++;
        }
      }
  debug("land weight atlas selfcheck: rmse %.5f max %.4f over %d samples", sqrt(sse / max<int64_t>(n, 1)), maxErr, (int)n);
}

LandWeightAtlas *LandWeightAtlasBuilder::finishForExport(int max_tex)
{
  int64_t reft = ref_time_ticks();
  if (texSize < 1 || elemSize < 1)
    return nullptr;
  int pagesWanted = 0;
  for (const Cell &c : cells)
  {
    int numTex = 0;
    while (numTex < DET_NUM && c.chan[numTex].size())
      numTex++;
    pagesWanted += LandWeightAtlas::pagesFor(numTex);
  }
  // exactly the pages the cells blend; a flat map still takes one, or no record is written
  eastl::unique_ptr<LandWeightAtlas> a(
    new LandWeightAtlas(LandWeightAtlas::Pages::Raw, cellsX, cellsY, elemSize, 0, false, max(pagesWanted, 1)));
  packCells(*a);
  clear_and_shrink(cells); // the planes live on in the pages; the image the exporter composes next is as large again
  // a cell past the record's page range keeps a single landclass; a load
  // degrades that way and reports, an export must refuse rather than ship it
  if (a->getPageCount() < pagesWanted)
  {
    logerr("land weight atlas: the map needs %d pages, a record addresses %d; export refused", pagesWanted, LAND_WEIGHT_PAGE_MASK + 1);
    return nullptr;
  }
  if (!a->chooseLayout(max_tex))
  {
    logerr("land weight atlas: %d pages of %d texels do not fit a %dx%d texture; export refused", a->getPageCount(),
      a->getCellTexSize(), max_tex, max_tex);
    return nullptr;
  }
  int w = 0, h = 0;
  a->getAtlasSize(w, h);
  debug("land weight atlas: %dx%d cells, page %d (elem %d + border), %d pages in %dx%d, packed for export in %d us", cellsX, cellsY,
    a->getCellTexSize(), elemSize, a->getPageCount(), w, h, (int)get_time_usec(reft));
  return a.release();
}
