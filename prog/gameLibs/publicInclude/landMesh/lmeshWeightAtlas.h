//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "../../landMesh/shaders/land_weight_atlas.hlsli"
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <generic/dag_span.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint2.h>
#include <util/dag_stdint.h>

class BaseTexture;
typedef BaseTexture Texture;
class Sbuffer;
class PostFxRenderer;
class IGenLoad;
class IGenSave;
struct TexPixel32;

// One atlas texture replacing all per-cell land detail weight textures.
// The per-cell entry layout and how a weight is read from it live in
// shaders/land_weight_atlas.hlsli, shared with the sampling code.
// Pages carry a border with the neighbouring cells' weights so bilinear
// filtering is seamless across cell boundaries (legacy clamped per cell).
// The exporter packs the atlas and the level ships it (savePacked); older
// level binaries ship per-cell weight textures, which the legacy conversions
// pack at load until every location is re-exported.
// The class spans several translation units: lmeshWeightAtlas.cpp (records,
// page packing, the packed form) and lmeshWeightAtlasLegacyCpu.cpp stay
// driver-free, so the exporter can link them alone; the public constructor,
// upload(), bindCells(), setCellMapping(), createLoaded(), Builder::finish()
// and land_weight_atlas_cpu_pack() live in lmeshWeightAtlasGpu.cpp, and
// setCellRecord()/renderCell() in lmeshWeightAtlasLegacyGpu.cpp, which need
// the driver and shader libraries.
struct LandWeightAtlas
{
  static constexpr int DET_NUM = LAND_WEIGHT_DET_NUM;
  static constexpr int WORDS_PER_CELL = LAND_WEIGHT_CELL_STRIDE / 4;
  // the per-cell source texture size a packed level writes in its detail map
  // header: it ships the atlas instead, and the legacy export never wrote 0
  static constexpr int PACKED_TEX_SIZE = 0;
  // the largest atlas side: every target's driver holds it, so the export packs
  // under it, and the runtime packing caps its row search with it too
  static constexpr int MAX_TEX_SIDE = 8192;

  Texture *tex = nullptr;
  Sbuffer *cellsBuf = nullptr; // the records again, indexed by cell, so a shader can resolve any
                               // pixel's weights without that cell's draw being current
  SmallTab<uint32_t> records;  // cellsX * cellsY * WORDS_PER_CELL, see land_weight_atlas.hlsli
  int cellsX = 0, cellsY = 0;

  // A cell never needs more than 2 pages, so reserving 2 per cell makes the texture large enough for any later edit,
  // and it is never recreated. The reserve stops at pageBudget(): a map that blends more than that cannot ship either,
  // so the editor paints exactly what the export can pack. tex_cflg is OR'd into the atlas texture creation:
  // an owner that keeps a system copy for device resets passes TEXCF_SYSTEXCOPY|TEXCF_LOADONCE,
  // one that rebuilds the atlas itself passes 0 (see LandMeshReset in lmeshManager.h)
  LandWeightAtlas(int cells_x, int cells_y, int elem_size, unsigned tex_cflg, bool reserve_for_edit = false);
  ~LandWeightAtlas();
  const uint32_t *cellRecord(int cx, int cy) const { return &records[(cy * cellsX + cx) * WORDS_PER_CELL]; }
  // the landclasses the record blends: one where the pages could not hold the cell, and one where the atlas does not cover it at all
  int cellBlendCount(int cx, int cy) const
  {
    if (uint32_t(cx) >= uint32_t(cellsX) || uint32_t(cy) >= uint32_t(cellsY))
      return 1;
    return ((*cellRecord(cx, cy) >> LAND_WEIGHT_COUNT_SHIFT) & LAND_WEIGHT_COUNT_MASK) + 1;
  }
  // the cell's DET_NUM landclass id bytes, after the pages dword of its record
  static uint8_t *recordLandclassIds(uint32_t *rec) { return (uint8_t *)rec + LAND_WEIGHT_CELL_IDS_OFS; }
  static const uint8_t *recordLandclassIds(const uint32_t *rec) { return (const uint8_t *)rec + LAND_WEIGHT_CELL_IDS_OFS; }
  // The atlas a level shipped packed (readPacked): its records and texture as
  // they are, nothing is re-encoded. Owns the texture from the call on, failure
  // returns included; null when the texture does not hold the pages this build
  // packs (the page size changed since the export) or the records cannot be
  // published.
  static LandWeightAtlas *createLoaded(int cells_x, int cells_y, int elem_size, int pages_per_row, int page_count,
    SmallTab<uint32_t> &&packed_records, Texture *tex);

  // texels a cell wants per side, its border included; the page it is packed
  // into happens to be that size today, but it does not have to stay so
  int getCellTexSize() const { return pageW; }
  // a cell needs a page per three channels, bar the last one, which is derived
  static int pagesFor(int num_tex) { return (num_tex + 1) / LAND_WEIGHT_CHANNELS_PER_PAGE; }
  // the pages of page_w texels the largest atlas holds, as far as a record can address them
  static int pageBudget(int page_w);
  // cells that got no page since the last report: each keeps its first landclass alone, and the export refuses a map that needs them.
  // upload() reports and clears the count unless the owner took it to report itself
  int getCellsPastBudget() const { return cellsPastBudget; }
  int takeCellsPastBudget()
  {
    const int n = cellsPastBudget;
    cellsPastBudget = 0;
    return n;
  }
  // (re)packs one cell from its blended landclasses' derived weight planes (they
  // sum to 1) of getCellTexSize()^2 bytes each, border included. Pages are
  // reallocated as needed, call upload() once after a batch of cells.
  void setCellWeights(int index, const uint8_t *const planes[DET_NUM], const uint8_t *det_tex_ids, int num_tex);
  // where DXT1 is missing (mobile, ASTC sources) nothing is decoded or read
  // back: the record follows from the cell's landclass list alone, and the
  // pages are rendered from the legacy textures into the uncompressed atlas
  void setCellRecord(int index, const uint8_t *det_tex_ids, int num_tex);
  // false when nothing is made resident from here: an export atlas never
  // uploads, a loaded one came resident, a CPU-packed one only until dropCpuPages()
  bool upload();
  void bindCells() const; // binds cellsBuf at land_weight_cells_const_no
  // the shader resolves the cell from the world position, so it needs the same
  // mapping the renderer uses to place cells
  void setCellMapping(float cell_size, float grid_cell_size, const Point3 &mesh_offset, const IPoint2 &cell_origin) const;
  // Reads a cell's weights back exactly as land_weight_inc.dshl does, from the
  // CPU page copy: what the shader will see, decoded by the code that wrote it.
  // x, y are texels inside the cell; false once the pages are gone.
  bool sampleCell(int index, int x, int y, float w[DET_NUM]) const;
  void dropCpuPages(); // release the CPU page copy; no setCell()/upload() afterwards

  // The export atlas (Builder::finishForExport): its pages composed as the
  // texture the level ships, dst sized to atlas width x height texels with the
  // three weights in rgb, for the exporter to encode. Then the packed form: the
  // legacy per-cell table a loader from before the packed form expects, every
  // offset (relative to base_ofs, like the level's) naming one empty record, so
  // that loader loads the level without blending and logs instead of seeking
  // wild; then pages per row and count, the records as the shader reads them,
  // the encoded atlas as ddsx. readPacked() takes it back; a null ddsx skips
  // the texture.
  void getAtlasSize(int &w, int &h) const { w = atlasW, h = atlasH; }
  void composeRawImage(Tab<TexPixel32> &dst) const;
  void savePacked(IGenSave &cwr, int base_ofs, dag::ConstSpan<uint8_t> ddsx) const;
  static bool readPacked(IGenLoad &crd, int cells, int &pages_per_row, int &page_count, SmallTab<uint32_t> &packed_records,
    Tab<uint8_t> *ddsx);

  int getPageCount() const { return pageCount; }
  int getFreePageCount() const { return freePages.size(); }

private:
  // where the pages live and what puts them there
  enum class Pages : uint8_t
  {
    Dxt1,     // packed on the CPU and uploaded from here: legacy conversion, daEditor painting
    Rendered, // drawn into an uncompressed target by the legacy GPU conversion
    Raw,      // uncompressed on the CPU for the exporter to encode; there is no texture
    Loaded    // the level shipped the texture; only the records are on the CPU
  };
  // page_capacity: the CPU pages a Raw export atlas holds; 0 keeps the two per
  // cell the edit reserve takes
  LandWeightAtlas(Pages kind, int cells_x, int cells_y, int elem_size, unsigned tex_cflg, bool reserve_for_edit,
    int page_capacity = 0);
  int pageW = 0, elemW = 0, pageBytes = 0, pageCount = 0, pagesPerRow = 0, atlasW = 0, atlasH = 0;
  int cellsPastBudget = 0;
  Pages kind = Pages::Dxt1;
  bool reserved = false;
  unsigned extraTexCflg = 0;
  Tab<uint8_t> pages, pageScratch; // empty when the pages live only on the GPU
  Tab<uint16_t> freePages;
  struct CellsReload;
  CellsReload *cellsReload = nullptr; // owned here: drivers differ on whether they ever free one
  int addPage(const uint8_t *const planes[DET_NUM], int first_ch, int used);
  void pageTexel(int page, int x, int y, float rgb[LAND_WEIGHT_CHANNELS_PER_PAGE]) const;
  void setShaderVars() const;
  int allocPage();
  bool publishRecords();
  bool chooseLayout(int max_tex); // export: the row length that packs the pages tightest under the cap
  void writeRecord(int index, const uint8_t *det_tex_ids, int num_tex, const uint8_t *const planes[DET_NUM]);
  // draws the cell's pages with the land_weight_pack shader the legacy GPU
  // conversion owns; only that conversion may drive it
  bool renderCell(int index, int num_tex, BaseTexture *tex1, BaseTexture *tex2, int src_size, PostFxRenderer &pack);
  friend LandWeightAtlas *render_land_weight_atlas(dag::Span<Tab<uint8_t>> records, int cells_x, int cells_y, int tex_size,
    int elem_size);
  friend class LandWeightAtlasBuilder;
};

// Packs cells into a LandWeightAtlas with page borders taken from the
// neighbouring cells. Feed every cell in index order, then finish(): the
// legacy per-cell ddsx weight streams of old level binaries convert at load
// through addCell(), the exporter packs the painted map through
// addCellWeights() and finishForExport().
class LandWeightAtlasBuilder
{
public:
  // tex_size is the side of the per-cell source the planes come in, elem_size
  // the texels of it a cell owns; only the legacy load path has them apart
  LandWeightAtlasBuilder(int cells_x, int cells_y, int tex_size, int elem_size, unsigned tex_cflg);
  // export packing: the map hands over exactly the cell's texels, and no
  // runtime texture is made
  LandWeightAtlasBuilder(int cells_x, int cells_y, int elem_size) :
    LandWeightAtlasBuilder(cells_x, cells_y, elem_size, elem_size, /*tex_cflg*/ 0)
  {}
  ~LandWeightAtlasBuilder();
  bool addCell(int index, const uint8_t *det_tex_ids, dag::ConstSpan<uint8_t> tex1_ddsx, dag::ConstSpan<uint8_t> tex2_ddsx);
  // the cell's weights as one plane of elem_size^2 bytes per landclass of
  // det_tex_ids; planes past its landclass count are not read
  bool addCellWeights(int index, const uint8_t *det_tex_ids, const uint8_t *const planes[LandWeightAtlas::DET_NUM]);
  LandWeightAtlas *finish();
  // The packed atlas with its pages kept uncompressed on the CPU and its
  // layout chosen under max_tex per side; the fed cells are consumed. Null,
  // with the cause logged, when the pages do not fit or a cell had to drop
  // its blending: shipping either is worse than no export.
  LandWeightAtlas *finishForExport(int max_tex);

private:
  struct Cell
  {
    uint8_t detTexIds[LandWeightAtlas::DET_NUM];
    SmallTab<uint8_t> chan[LandWeightAtlas::DET_NUM]; // derived weights, texSize*texSize, empty when inactive
  };
  const uint8_t *cellTexel(int cx, int cy, int lc_id, int x, int y) const;
  void packCells(LandWeightAtlas &a) const;
  void selfCheck(const LandWeightAtlas &a) const;
  Tab<Cell> cells;
  int cellsX, cellsY, texSize, elemSize;
  unsigned texCflg;
  int undecodedCells = 0;
};

// true where the sources can be decoded and DXT1 packed on the CPU; false
// selects the render path (mobile). graphics{landWeightAtlasForceGpu:b=yes}
// forces it off so the render path can be checked on a desktop build.
bool land_weight_atlas_cpu_pack();
