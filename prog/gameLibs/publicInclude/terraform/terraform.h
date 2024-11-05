//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <math/integer/dag_IBBox2.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <generic/dag_carray.h>
#include <EASTL/vector.h>
#include <memory/dag_memPtrAllocator.h>


class HeightmapHandler;

class Terraform final
{
public:
  using alt_hash_map_t = ska::flat_hash_map<uint32_t, float, eastl::hash<uint32_t>, eastl::equal_to<uint32_t>, dag::MemPtrAllocator>;

  static const int PATCH_SIZE = 256;
  static const int PATCH_SIZE_SQ = PATCH_SIZE * PATCH_SIZE;
  static const float PATCH_SIZE_INV;
  static const uint8_t PATCH_ALT_MAX = UINT8_MAX;
  static const uint16_t HMAP_ALT_MAX = UINT16_MAX;
  static const uint8_t ALT_PRECISION = 100;
  static const float PATCH_ALT_MAX_INV;
  static const int MIN_DELTA_ALT_BITS = 10;
  static const int MAX_CELLS_SIZE = 65536;

  struct Pcd
  {
    __forceinline Pcd(uint32_t index) : patchNo(extractPatchNo(index)), dataIndex(extractDataIndex(index)) {}
    __forceinline Pcd(uint16_t in_patch_no, uint16_t in_data_index) : patchNo(in_patch_no), dataIndex(in_data_index) {}
    uint16_t patchNo;
    uint16_t dataIndex;

    __forceinline uint32_t toInt() const { return patchNo | dataIndex << 16; }

    static __forceinline uint16_t extractPatchNo(uint32_t index) { return index & 0xFFFF; }
    static __forceinline uint16_t extractDataIndex(uint32_t index) { return index >> 16; }
  };
  struct PcdAlt
  {
    Pcd pcd = Pcd(0, 0);
    uint8_t alt = 0;
  };
  class Renderer
  {
  public:
    virtual void invalidatePatch(int patch_no) = 0;
    virtual void invalidatePrims() = 0;
  };
  class Listener
  {
  public:
    virtual void allocPatch(int patch_no) = 0;
    virtual void storePatchAlt(const Pcd &pcd, uint8_t alt) = 0;
  };

  struct Desc
  {
    int cellsPerMeter = 4;
    float minLevel = -2.0f;
    float minDigLevel = -2.0f;
    float maxLevel = 1.5f;
    int maxSpreadRadius = 8;

    __forceinline float getMinLevel() const { return minLevel; }
    __forceinline float getMinDigLevel() const { return minDigLevel; }
    __forceinline float getMaxLevel() const { return maxLevel; }
  };
  enum PrimMode
  {
    DYN_REPLACE,
    DYN_ADDITIVE,
    DYN_MIN,
    DYN_MAX
  };
  struct QuadData
  {
    IPoint2 vertices[4];
    float diffAlt;

    __forceinline bool operator==(const QuadData &ref) const
    {
      return fabsf(diffAlt - ref.diffAlt) < 0.001f && vertices[0] == ref.vertices[0] && vertices[1] == ref.vertices[1] &&
             vertices[2] == ref.vertices[2] && vertices[3] == ref.vertices[3];
    }
  };

  Terraform(HeightmapHandler &in_hmap, const Desc &in_desc);
  ~Terraform();

  void storePatchAlt(const Pcd &pcd, uint8_t alt);
  uint8_t getAltModePacked(Pcd pcd, float alt, PrimMode mode) const;

  void invalidate();
  void invalidatePrims();

  float sampleHeightCur(const Point2 &coord) const;
  float sampleHmapHeightCur(const Point2 &coord) const;
  float sampleHmapHeightOrig(const Point2 &coord) const;
  float getHmapHeightCurVal(const IPoint2 &hmap_coord) const;
  float getHmapHeightOrigVal(const IPoint2 &hmap_coord) const;

  dag::ConstSpan<int> getPatchChanges() const;
  const IBBox2 &getPatchBBChanges(int patch_no) const;
  const BBox2 &getPatchWorldBB(int patch_no) const;
  void clearPatchChanges();

  bool submitQuad(const QuadData &quad, uint16_t &prim_id);
  void removePrim(uint16_t prim_id);
  void storePrim(uint16_t prim_id);
  void getPrimData(uint16_t prim_id, alt_hash_map_t &out_data) const;
  float getPrimLeftAlt(uint16_t prim_id) const;
  float getPrimMaxAlt(uint16_t prim_id) const;

  void storeQuad(const QuadData &quad);
  void storeQuad(const Point2 &pos, float width, float height, float depth);
  void storeSphereAlt(const Point2 &pos, float radius, float alt, PrimMode mode);
  void queueElevationChange(const Point2 &pos, float radius, float alt);

  void updateInternal();
  void update()
  {
    if (updateFlags)
      updateInternal();
  }

  void setDesc(const Desc &in_desc);

  bool testRegionChanged(const IBBox2 &region) const;
  bool testRegionChanged(const BBox2 &region) const;

  void registerRenderer(Renderer *in_renderer)
  {
    G_ASSERT(!renderer);
    renderer = in_renderer;
  };
  void unregisterRenderer(Renderer *in_renderer)
  {
    G_ASSERT(renderer == in_renderer);
    G_UNUSED(in_renderer);
    renderer = NULL;
  }

  void registerListener(Listener *in_listener) { listeners.push_back(in_listener); };
  void unregisterListener(Listener *in_listener) { erase_item_by_value(listeners, in_listener); }

  void copyOriginalHeightMap(uint16_t *dst) const;
  void copy(const Terraform &rhs);
  bool isEqual(const Terraform &rhs) const;

  static uint8_t computeZeroAlt(int min_level_int, int max_level_int);
  static uint8_t computeZeroAltF(float min_level, float max_level);

  __forceinline HeightmapHandler &getHMap() const { return hmap; }
  __forceinline const Desc &getDesc() const { return desc; }
  __forceinline const IPoint2 &getResolution() const { return resolution; }

  __forceinline const Point2 &getPivot() const { return pivot; }
  __forceinline const Point2 &getWorldSize() const { return worldSize; }
  __forceinline const IPoint2 &getHMapSizes() { return hmapSizes; }
  __forceinline uint8_t getZeroAlt() const { return zeroAlt; }
  __forceinline float getMinDAlt() const { return minDAlt; }
  __forceinline int getPrimSpreadRadius() const { return primSpreadRadius; }

  __forceinline int getPatchNumX() const { return patchNumX; }
  __forceinline int getPatchNumY() const { return patchNumY; }
  __forceinline int getPatchNum() const { return patches.size(); }
  __forceinline bool isPatchExist(int patch_no) const { return patches[patch_no] != nullptr; }
  __forceinline dag::ConstSpan<uint8_t> getPatch(int patch_no) const
  {
    return patches[patch_no] ? patches[patch_no]->data : dag::ConstSpan<uint8_t>();
  }

  __forceinline uint8_t getPatchAlt(const Pcd &pcd) const
  {
    return patches[pcd.patchNo] ? patches[pcd.patchNo]->data[pcd.dataIndex] : zeroAlt;
  }
  __forceinline float getPatchAltUnpacked(const Pcd &pcd) const
  {
    return patches[pcd.patchNo] ? unpackAlt(patches[pcd.patchNo]->data[pcd.dataIndex]) : 0.0f;
  }
  __forceinline float getPatchAltUnpackedFromCoord(const IPoint2 &coord) const { return getPatchAltUnpacked(makePcdFromCoord(coord)); }
  __forceinline void storePatchAltUnpacked(const Pcd &pcd, float alt, PrimMode mode)
  {
    storePatchAlt(pcd, getAltModePacked(pcd, alt, mode));
  }

  __forceinline uint8_t packAlt(float value) const { return value * packAltScale + packAltOffset; }

  __forceinline float unpackAlt(uint8_t value) const { return value * unpackAltScale + unpackAltOffset; }

  __forceinline uint16_t addAltToHmapAlt(uint16_t hmap_alt, uint16_t alt) const
  {
    int addVal = ((int)alt * hmapAltScaleInt + hmapAltOffsetInt) / hmapScaleLevelInt;
    G_ASSERT((hmap_alt + addVal) >= 0 && (hmap_alt + addVal) <= HMAP_ALT_MAX);
    return (int)hmap_alt + addVal;
  }

  __forceinline IPoint2 getPatchXYFromCoord(const IPoint2 &coord) const { return IPoint2(coord.x / PATCH_SIZE, coord.y / PATCH_SIZE); }

  __forceinline int getPatchNoFromXY(const IPoint2 &patch_xy) const { return patch_xy.y * patchNumX + patch_xy.x; }

  __forceinline Pcd makePcdFromCoord(const IPoint2 &coord) const
  {
    IPoint2 patchXY = getPatchXYFromCoord(coord);
    return Pcd(getPatchNoFromXY(patchXY), coord.x + (coord.y - patchXY.x) * PATCH_SIZE - patchXY.y * PATCH_SIZE_SQ);
  }

  __forceinline bool isValidPcd(const Pcd &pcd) const { return pcd.patchNo < (patchNumX * patchNumY); }

  __forceinline Point2 getCoordFromPosF(const Point2 &pos) const
  {
    return Point2(pos.x * posToCoordScale.x + posToCoordOffset.x, pos.y * posToCoordScale.y + posToCoordOffset.y);
  }

  __forceinline IPoint2 getCoordFromPos(const Point2 &pos) const { return IPoint2::xy(getCoordFromPosF(pos)); }

  __forceinline Point2 getPosFromCoord(const IPoint2 &coord) const
  {
    return Point2(coord.x * coordToPosScale.x + coordToPosOffset.x, coord.y * coordToPosScale.y + coordToPosOffset.y);
  }

  __forceinline IPoint2 getCoordFromPcd(const Pcd &pcd) const
  {
    int y = pcd.patchNo / patchNumX;
    int x = pcd.patchNo % patchNumX;
    return IPoint2(x * PATCH_SIZE + pcd.dataIndex % PATCH_SIZE, y * PATCH_SIZE + pcd.dataIndex / PATCH_SIZE);
  }

  __forceinline Point2 getHmapCoordFromCoordF(const Point2 &coord) const
  {
    return Point2(coord.x * hmapCellResolutionInv.x, coord.y * hmapCellResolutionInv.y);
  }

  __forceinline Point2 getHmapCoordFromCoordF(const IPoint2 &coord) const { return getHmapCoordFromCoordF(Point2::xy(coord)); }

  __forceinline IPoint2 getHmapCoordFromCoord(const IPoint2 &coord) const { return IPoint2::xy(getHmapCoordFromCoordF(coord)); }

  __forceinline IPoint2 getCoordFromHmapCoordF(const Point2 &hmap_coord) const
  {
    return IPoint2(hmap_coord.x * hmapCellResolution.x, hmap_coord.y * hmapCellResolution.y);
  }

  __forceinline IPoint2 getCoordFromHmapCoord(const IPoint2 &hmap_coord) const
  {
    return IPoint2(hmap_coord.x * hmapCellResolution.x, hmap_coord.y * hmapCellResolution.y);
  }

  __forceinline const alt_hash_map_t &getPrimAltMap() const { return primAltMap; }

private:
  struct Patch
  {
    carray<uint8_t, PATCH_SIZE * PATCH_SIZE> data;
    eastl::vector<uint16_t> hmapSaved;
    IBBox2 bbChanges;
    BBox2 worldBBPatch;
    bool invalidate;

    __forceinline Patch &operator=(const Patch &rhs)
    {
      invalidate = true;
      bbChanges = rhs.bbChanges;
      eastl::copy(rhs.data.begin(), rhs.data.end(), data.begin());
      hmapSaved = rhs.hmapSaved;
      return *this;
    }

    __forceinline bool operator==(const Patch &rhs) const
    {
      return bbChanges == rhs.bbChanges && eastl::equal(data.cbegin(), data.cend(), rhs.data.cbegin()) && hmapSaved == rhs.hmapSaved;
    }

    __forceinline bool operator!=(const Patch &rhs) const { return !(*this == rhs); }
  };

  struct QuadPrim
  {
    uint16_t id;
    Terraform::QuadData quad;
    float leftAlt;
    float maxAlt;
    Tab<IPoint2> cells;
    bool changed;
  };

  void allocPatch(int patch_no);
  void rasterizeQuad(const QuadPrim &prim, Tab<IPoint2> &out_cells) const;
  int getPrimIndex(uint16_t prim_id) const;

  template <typename Func>
  float sampleFunc(const Point2 &coord, Func func, const IPoint2 &sample_size) const;

  void handleCells(const Terraform::QuadPrim &prim, alt_hash_map_t &out_alt_map, float &out_left_alt, float &out_max_alt) const;

  __forceinline float getHmapAlt(const IPoint2 &hmap_coord) const;
  __forceinline float getHmapHeightCur(const IPoint2 &hmap_coord) const;
  __forceinline float getHmapHeightOrig(const IPoint2 &hmap_coord) const;
  __forceinline uint16_t getHmapHeightOrigPacked(const IPoint2 &hmap_coord) const;

  template <typename HS>
  void addOrigHmapAltSph(const Point2 &pos, float radius, float alt, HS &updated, IBBox2 &updatedCoord);

  float sampleHmapAlt(const Point2 &hmap_coord) const;

  float getPrimAltDefMin(const IPoint2 &coord) const;

  HeightmapHandler &hmap;
  Desc desc;

  float packAltScale = 1;
  float packAltOffset = 1;
  float unpackAltScale = 1;
  float unpackAltOffset = 1;

  // might be negative and need 32 bits precision
  // (16 unsigned bits in fact + negative)
  int minLevelInt = 0;
  int maxLevelInt = 0;
  int hmapAltScaleInt = 0;
  int hmapAltOffsetInt = 0;
  int hmapScaleLevelInt = 0;

  Point2 posToCoordScale = Point2(1, 1);
  Point2 posToCoordOffset = Point2(0, 0);
  Point2 coordToPosScale = Point2(1, 1);
  Point2 coordToPosOffset = Point2(0, 0);

  uint8_t zeroAlt = 0;
  float minDAlt = 0.f;

  struct Elevation
  {
    Point2 pos;
    float radius;
    float alt;
  };

  enum
  {
    UPDFLAG_ELEVATIONS = 0x01,
    UPDFLAG_PRIMS = 0x02,
    UPDFLAG_PATCHES = 0x04,
  };
  int updateFlags = 0;

  Tab<Elevation> elevations;
  Tab<QuadPrim> quads;
  Tab<Patch *> patches;
  int patchNumX = 0;
  int patchNumY = 0;
  IPoint2 resolution;
  IPoint2 hmapSizes = {};
  IPoint2 hmapCellResolution = {};
  Point2 hmapCellResolutionInv = {};
  IPoint2 hmapPatchSize = {};
  uint16_t nextPrimId = 1;

  Point2 pivot = {};
  Point2 worldSize = {};
  int primSpreadRadius = 0;
  alt_hash_map_t primAltMap;
  Tab<int> patchChanges;

  Renderer *renderer = NULL;
  Tab<Listener *> listeners;
};

template <class Func>
__forceinline void rastr_tri(const IPoint2 &vt1, const IPoint2 &vt2, const IPoint2 &vt3, const Func &func)
{
#define CROSS_PRODUCT(a, b) ((a).x * (b).y - (a).y * (b).x)

  /* get the bounding box of the triangle */
  int maxX = max(vt1.x, max(vt2.x, vt3.x));
  int minX = min(vt1.x, min(vt2.x, vt3.x));
  int maxY = max(vt1.y, max(vt2.y, vt3.y));
  int minY = min(vt1.y, min(vt2.y, vt3.y));

  /* spanning vectors of edge (v1,v2) and (v1,v3) */
  IPoint2 vs1 = IPoint2(vt2.x - vt1.x, vt2.y - vt1.y);
  IPoint2 vs2 = IPoint2(vt3.x - vt1.x, vt3.y - vt1.y);

  int cp1 = CROSS_PRODUCT(vs1, vs2);
  int cp2 = CROSS_PRODUCT(vs1, vs2);
  if (cp1 == 0 || cp2 == 0)
    return;

  for (int x = minX; x <= maxX; x++)
  {
    for (int y = minY; y <= maxY; y++)
    {
      IPoint2 q = IPoint2(x - vt1.x, y - vt1.y);
      float s = (float)CROSS_PRODUCT(q, vs2) / (float)cp1;
      float t = (float)CROSS_PRODUCT(vs1, q) / (float)cp2;

      if ((s >= 0) && (t >= 0) && (s + t < 1))
      { /* inside triangle */
        func(x, y);
      }
    }
  }
}

template <class Func>
__forceinline void rastr_line(int x, int y, int x2, int y2, const Func &func)
{
  bool yLonger = false;
  int incrementVal, endVal;
  int shortLen = y2 - y;
  int longLen = x2 - x;
  if (abs(shortLen) > abs(longLen))
  {
    int swap = shortLen;
    shortLen = longLen;
    longLen = swap;
    yLonger = true;
  }
  endVal = longLen;
  if (longLen < 0)
  {
    incrementVal = -1;
    longLen = -longLen;
  }
  else
    incrementVal = 1;
  int decInc;
  if (longLen == 0)
    decInc = 0;
  else
    decInc = (shortLen << 16) / longLen;
  int j = 0;
  if (yLonger)
  {
    for (int i = 0; i != endVal; i += incrementVal)
    {
      func(x + (j >> 16), y + i);
      j += decInc;
    }
  }
  else
  {
    for (int i = 0; i != endVal; i += incrementVal)
    {
      func(x + i, y + (j >> 16));
      j += decInc;
    }
  }
}
