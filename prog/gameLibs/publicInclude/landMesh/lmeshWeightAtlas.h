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

// One atlas texture replacing all per-cell land detail weight textures.
// The per-cell entry layout and how a weight is read from it live in
// shaders/land_weight_atlas.hlsli, shared with the sampling code.
// Pages carry a border with the neighbouring cells' weights so bilinear
// filtering is seamless across cell boundaries (legacy clamped per cell).
struct LandWeightAtlas
{
  static constexpr int DET_NUM = LAND_WEIGHT_DET_NUM;
  static constexpr int WORDS_PER_CELL = LAND_WEIGHT_CELL_STRIDE / 4;

  Texture *tex = nullptr;
  Sbuffer *cellsBuf = nullptr; // the records again, indexed by cell, so a shader can resolve any
                               // pixel's weights without that cell's draw being current
  SmallTab<uint32_t> records;  // cellsX * cellsY * WORDS_PER_CELL, see land_weight_atlas.hlsli
  int cellsX = 0, cellsY = 0;

  // A cell never needs more than 2 pages, so reserving 2 per cell makes the
  // texture large enough for any later edit and it is never recreated.
  // tex_cflg is OR'd into the atlas texture creation: an owner that keeps a
  // system copy for device resets passes TEXCF_SYSTEXCOPY|TEXCF_LOADONCE, one
  // that rebuilds the atlas itself passes 0 (see LandMeshReset in lmeshManager.h)
  LandWeightAtlas(int cells_x, int cells_y, int elem_size, unsigned tex_cflg, bool reserve_for_edit = false);
  ~LandWeightAtlas();
  const uint32_t *cellRecord(int cx, int cy) const { return &records[(cy * cellsX + cx) * WORDS_PER_CELL]; }

  // texels a cell wants per side, its border included; the page it is packed
  // into happens to be that size today, but it does not have to stay so
  int getCellTexSize() const { return pageW; }
  // a cell needs a page per three channels, bar the last one, which is derived
  static int pagesFor(int num_tex) { return (num_tex + 1) / LAND_WEIGHT_CHANNELS_PER_PAGE; }
  // (re)packs one cell from its blended landclasses' derived weight planes (they
  // sum to 1) of getCellTexSize()^2 bytes each, border included. Pages are
  // reallocated as needed, call upload() once after a batch of cells.
  void setCellWeights(int index, const uint8_t *const planes[DET_NUM], const uint8_t *det_tex_ids, int num_tex);
  // packs a cell from the detail texture map texels daEditor paints into
  void setCellFromDetailTexels(int index, const uint32_t *argb4, const uint32_t *rg8, const uint8_t *det_tex_ids, int num_tex);
  // where DXT1 is missing (mobile, ASTC sources) nothing is decoded or read
  // back: the record follows from the cell's landclass list alone, and the
  // pages are rendered from the legacy textures into the uncompressed atlas
  void setCellRecord(int index, const uint8_t *det_tex_ids, int num_tex);
  // false when no page can be rendered at all (no pack shader in this dump)
  bool renderCell(int index, int num_tex, BaseTexture *tex1, BaseTexture *tex2, int src_size);
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

  int getPageCount() const { return pageCount; }
  int getFreePageCount() const { return freePages.size(); }

private:
  int pageW = 0, elemW = 0, pageBytes = 0, pageCount = 0, pagesPerRow = 0, atlasW = 0, atlasH = 0;
  uint32_t texFmt = 0; // TEXFMT_DXT1 where the CPU packs, a render target where it cannot
  bool reserved = false;
  unsigned extraTexCflg = 0;
  Tab<uint8_t> pages, pageScratch; // empty when the pages live only on the GPU
  Tab<uint16_t> freePages;
  bool packedOnCpu() const;
  struct CellsReload;
  CellsReload *cellsReload = nullptr;     // owned here: drivers differ on whether they ever free one
  PostFxRenderer *packRenderer = nullptr; // only the rendered path builds one
  int addPage(const uint8_t *const planes[DET_NUM], int first_ch, int used);
  void pageTexel(int page, int x, int y, float rgb[LAND_WEIGHT_CHANNELS_PER_PAGE]) const;
  void setShaderVars() const;
  int allocPage();
  void writeRecord(int index, const uint8_t *det_tex_ids, int num_tex, const uint8_t *const planes[DET_NUM]);
};

// Converts the legacy per-cell ddsx weight streams (as stored in level
// binaries) into a LandWeightAtlas at load time. Feed every cell in index
// order, then finish().
class LandWeightAtlasBuilder
{
public:
  LandWeightAtlasBuilder(int cells_x, int cells_y, int tex_size, int elem_size, unsigned tex_cflg);
  ~LandWeightAtlasBuilder();
  bool addCell(int index, const uint8_t *det_tex_ids, dag::ConstSpan<uint8_t> tex1_ddsx, dag::ConstSpan<uint8_t> tex2_ddsx);
  LandWeightAtlas *finish();

private:
  struct Cell
  {
    uint8_t detTexIds[LandWeightAtlas::DET_NUM];
    SmallTab<uint8_t> chan[LandWeightAtlas::DET_NUM]; // derived weights, texSize*texSize, empty when inactive
  };
  const uint8_t *cellTexel(int cx, int cy, int lc_id, int x, int y) const;
  Tab<Cell> cells;
  int cellsX, cellsY, texSize, elemSize;
  unsigned texCflg;
  int undecodedCells = 0;
};

// true where the sources can be decoded and DXT1 packed on the CPU; false
// selects the render path (mobile). graphics{landWeightAtlasForceGpu:b=yes}
// forces it off so the render path can be checked on a desktop build.
bool land_weight_atlas_cpu_pack();
