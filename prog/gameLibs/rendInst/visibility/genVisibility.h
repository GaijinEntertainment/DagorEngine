// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <rendInst/rendInstGenRender.h>
#include <rendInst/visibility.h>
#include "riGen/riGenData.h"
#include "visibility/extraVisibility.h"

#include <generic/dag_tab.h>
#include <generic/dag_carray.h>


namespace rendinst
{
extern Point3_vec4 dir_from_sun;
}

enum
{
  VIS_HAS_OPAQUE = 1,
  VIS_HAS_TRANSP = 2,
  VIS_HAS_IMPOSTOR = 4
};
struct RenderRanges
{
  void init()
  {
    mem_set_0(endCell);
    mem_set_0(startCell);
    vismask = 0;
  }
  RenderRanges() { init(); }

  carray<uint16_t, rendinst::render::MAX_LOD_COUNT_WITH_ALPHA> startCell;
  carray<uint16_t, rendinst::render::MAX_LOD_COUNT_WITH_ALPHA> endCell;
  uint8_t vismask;
  bool hasOpaque() const { return vismask & VIS_HAS_OPAQUE; }
  bool hasTransparent() const { return vismask & VIS_HAS_TRANSP; }
  bool hasImpostor() const { return vismask & VIS_HAS_IMPOSTOR; }
  bool hasCells(int lodI) const { return endCell[lodI] > startCell[lodI]; }
};

struct RiGenVisibility
{
  struct SubCellRange
  {
    uint16_t ofs, cnt;
    // NOTE: it seems that uninitialized fields are intended here, but
    // the code can probably be rewritten in a less confusing way.
    SubCellRange() {} // -V730
    SubCellRange(uint16_t ofs_, uint16_t cnt_) : ofs(ofs_), cnt(cnt_) {}
  };
  struct CellRange
  {
    int startVbOfs;
    // if ever assert appeared - change to bitfield!
    uint16_t startSubCell;    // in subCells array. Since subCells are not more than 64*MAX_VISIBLE_CELLS, 64*256 < 64k, in each render
                              // range. However, it really depends on render ranges count.
    uint16_t startSubCellCnt; // in subCells array, always <=64
    // change to it if assert!
    // uint32_t startSubCell:25;//
    // uint32_t startSubCellCnt:7;// in subCells array, always <=64.
    uint16_t x, z;
    // NOTE: it seems that uninitialized fields are intended here, but
    // the code can probably be rewritten in a less confusing way.
    CellRange() {} // -V730
    CellRange(int x_, int z_, int startVbOfs_, int startSubCell_, int startSubCellCnt_ = 0) :
      x(x_), z(z_), startVbOfs(startVbOfs_), startSubCell(startSubCell_), startSubCellCnt(startSubCellCnt_)
    {}
  };

  Tab<SubCellRange> subCells;
  Tab<RenderRanges> renderRanges;
  carray<Tab<CellRange>, rendinst::render::MAX_LOD_COUNT_WITH_ALPHA> cellsLod;
  carray<int, rendinst::render::MAX_LOD_COUNT_WITH_ALPHA + 1> instNumberCounter;

  int forcedLod = -1;
  enum
  {
    SKIP_NO_ATEST = 0,
    SKIP_ATEST = 1,
    RENDER_ALL = 2
  };
  int atest_skip_mask = RENDER_ALL;
  rendinst::VisibilityRenderingFlags rendering = rendinst::VisibilityRenderingFlag::All;
  bbox3f cameraBBox;
  int8_t stride = 0;
  uint8_t vismask = 0;

  RiGenExtraVisibility riex;

  bool hasCells(int ri, int lodI) const { return renderRanges[ri].hasCells(lodI); }
  void startRenderRange(int rri)
  {
    for (int i = 0; i < cellsLod.size(); ++i)
    {
      renderRanges[rri].startCell[i] = cellsLod[i].size();
      renderRanges[rri].endCell[i] = 0;
    }
  }
  void endRenderRange(int rri)
  {
    for (int i = 0; i < cellsLod.size(); ++i)
      renderRanges[rri].endCell[i] = cellsLod[i].size();
  }

  IMemAlloc *getmem() { return dag::get_allocator(renderRanges); }

  bool hasOpaque() const { return vismask & VIS_HAS_OPAQUE; }
  bool hasTransparent() const { return vismask & VIS_HAS_TRANSP; }
  bool hasImpostor() const { return vismask & VIS_HAS_IMPOSTOR; }

  RiGenVisibility(IMemAlloc *allocator) : renderRanges(allocator), subCells(allocator)
  {
    subCells.reserve(4096);
    mem_set_0(instNumberCounter);
    for (int i = 0; i < cellsLod.size(); ++i)
      dag::set_allocator(cellsLod[i], allocator);
  }
  void resizeRanges(int cnt, int mul)
  {
    if (renderRanges.size() != cnt)
    {
      clear_and_resize(crossDissolveRange, cnt);
      mem_set_0(crossDissolveRange);
      clear_and_resize(renderRanges, cnt);
    }
    mem_set_0(renderRanges); // for (int i = 0; i < renderRanges.size(); ++i) renderRanges[i].init();
    for (int i = 0; i < cellsLod.size(); ++i)
    {
      cellsLod[i].reserve(cnt * (i ? (mul << 1) : mul));
      cellsLod[i].clear();
    }
  }

  void closeRanges(int lodI) // shrink
  {
    CellRange &cell = cellsLod[lodI].back();
    int at = cell.startSubCell + cell.startSubCellCnt;
    erase_items(subCells, at, subCells.size() - at);
  }
  unsigned getOfs(int lodI, int ci, int ri, int _stride) const
  {
    const CellRange &cell = cellsLod[lodI][ci];
    G_ASSERT(ri < cell.startSubCellCnt);
    return cell.startVbOfs + subCells[cell.startSubCell + ri].ofs * _stride;
  }

  void addRange(int lodI, unsigned ofs, unsigned sz)
  {
    CellRange &cell = cellsLod[lodI].back();
    ofs -= cell.startVbOfs;
    ofs /= stride;
    sz /= stride;
    int endSubCell = int(cell.startSubCell + cell.startSubCellCnt);
    int lastSubCell = endSubCell - 1;
    if (cell.startSubCellCnt && subCells[lastSubCell].ofs + subCells[lastSubCell].cnt == ofs)
    {
      G_ASSERT(subCells[lastSubCell].cnt + sz <= 0xFFFF);
      subCells[lastSubCell].cnt += sz;
      return;
    }
    subCells[endSubCell] = SubCellRange(ofs, sz);
    cell.startSubCellCnt++;
  }
  G_STATIC_ASSERT(rendinst::MAX_LOD_COUNT >= 2);
  // original lods + alpha
  static constexpr int PER_INSTANCE_LODS = rendinst::MAX_LOD_COUNT + 1;
  enum
  {
    PI_ALPHA_LOD = PER_INSTANCE_LODS - 1,
    PI_IMPOSTOR_LOD = PER_INSTANCE_LODS - 2,
    PI_LAST_MESH_LOD = PER_INSTANCE_LODS - 3
  };
  carray<Tab<IPoint2>, PER_INSTANCE_LODS> perInstanceVisibilityCells; // todo: replace with list of cells for each ri_idx
  carray<Tab<vec4f>, PER_INSTANCE_LODS> instanceData;                 // plus cross dissolve between 0 and 1 lod
  Tab<float> crossDissolveRange;
  int max_per_instance_instances = 0;

  void addTreeInstance(int ri_idx, int &internal_cell, vec4f instance_data, int lod)
  {
    if (internal_cell < 0)
    {
      internal_cell = perInstanceVisibilityCells[lod].size();
      perInstanceVisibilityCells[lod].push_back(IPoint2(ri_idx, instanceData[lod].size()));
    }
    instanceData[lod].push_back(instance_data);
  }

  int addTreeInstances(int ri_idx, int &internal_cell, const vec4f *instance_data, int cnt, int lod)
  {
    if (internal_cell < 0)
    {
      internal_cell = perInstanceVisibilityCells[lod].size();
      perInstanceVisibilityCells[lod].push_back(IPoint2(ri_idx, instanceData[lod].size()));
    }
    return append_items(instanceData[lod], cnt, instance_data);
  }
  void startTreeInstances()
  {
    for (int i = 0; i < perInstanceVisibilityCells.size(); ++i)
    {
      perInstanceVisibilityCells[i].clear();
      instanceData[i].clear();
    }
  }
  void closeTreeInstances()
  {
    for (int i = 0; i < perInstanceVisibilityCells.size(); ++i)
      if (perInstanceVisibilityCells[i].size())
        perInstanceVisibilityCells[i].push_back(IPoint2(-1, instanceData[i].size()));
  }

  int addCell(int lodI, int x, int z, int startVbOfs,
    int rangeCount = RendInstGenData::SUBCELL_DIV * RendInstGenData::SUBCELL_DIV / 2 + 1)
  {
    int startSubCell = append_items(subCells, rangeCount);
    G_ASSERT(subCells.size() <= 0xFFFF); // if ever appeared, change startSubCell, startSubCellCnt to bitfield (see above)
    int idx = cellsLod[lodI].size();
    cellsLod[lodI].push_back(CellRange(x, z, startVbOfs, startSubCell));
    return idx;
  }

  unsigned getCount(int lodI, int ci, int ri) const { return subCells[cellsLod[lodI][ci].startSubCell + ri].cnt; }
  unsigned getRangesCount(int lodI, int ci) const { return cellsLod[lodI][ci].startSubCellCnt; }
  int gpuObjectsCascadeId = -1;

  float riDistMul = 1.0f;
  void setRiDistMul(float mul) { riDistMul = mul; }
};
