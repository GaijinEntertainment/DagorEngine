//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <debug/dag_assert.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>
#include <math/dag_adjpow2.h>
#include <math/dag_mathUtils.h>
#include <math/dag_math2d.h>
#include <math/integer/dag_IBBox2.h>
#include <dag/dag_vector.h>
#include <dag/dag_relocatable.h>
#include <EASTL/bitvector.h>

#define VERIFY_ALL                               0
#define EXTRA_SMALL_BRANCHES_AND_64K_LEAFS_LIMIT 0

#if VERIFY_ALL
#define LG_VERIFY(x)    G_ASSERT(x)
#define LG_VERIFYF(...) G_ASSERTF(__VA_ARGS__)
#else
#define LG_VERIFY(x)
#define LG_VERIFYF(...)
#endif

#if defined(__GNUC__) || defined(__clang__)
#define FORCEINLINE_ATTR __attribute__((always_inline))
#else
#define FORCEINLINE_ATTR
#endif

const unsigned LINEAR_GRID_DEFAULT_CELL_SIZE = 128;
const unsigned LINEAR_GRID_SUBGRID_WIDTH = 8;
const unsigned LINEAR_GRID_SUBGRID_WIDTH_LOG2 = get_const_log2(LINEAR_GRID_SUBGRID_WIDTH);

#if EXTRA_SMALL_BRANCHES_AND_64K_LEAFS_LIMIT
typedef uint16_t leaf_id_t;
#else
typedef uint32_t leaf_id_t;
#endif
const leaf_id_t EMPTY_LEAF = leaf_id_t(~0);

template <typename ObjectType>
class LinearGrid;

struct alignas(EA_CACHE_LINE_SIZE) LinearGridRay
{
  LinearGridRay(vec3f ray_start, vec3f ray_dir, vec4f ray_len, vec4f ray_radius) :
    start(ray_start), dir(ray_dir), len(ray_len), radius(ray_radius)
  {}
  const vec3f start;
  const vec3f dir;
  vec4f len;
  const vec4f radius;
};

struct UnpackedBranch
{
  bbox3f leftBox;
  bbox3f rightBox;
  leaf_id_t leftIdx;
  leaf_id_t rightIdx;
};

#ifdef _MSC_VER
#pragma warning(disable : 4582)
#pragma warning(disable : 4583)
#endif

template <typename ObjectType>
union alignas(16) LinearGridLeaf // 16 bytes [extend branches to 32?]
{
  LinearGridLeaf() { memset(this, 0, sizeof(*this)); }
  ~LinearGridLeaf() {}

  dag::Vector<ObjectType, MidmemAlloc> objects; // lnode

#if EXTRA_SMALL_BRANCHES_AND_64K_LEAFS_LIMIT
  struct // branch node
  {
    uint16_t packedMinCorrection[3];
    leaf_id_t left;
    uint16_t packedMaxCorrection[3];
    leaf_id_t right;
  };
#else
  struct // half of branch node
  {
    float packedCorrection[3];
    leaf_id_t idx;
  };
#endif
};

template <typename ObjectType>
struct alignas(16) LinearGridSubCell // 32 bytes
{
  bool isEmpty() const { return empty; }
  const bbox3f &getBBox() const
  {
    static_assert(offsetof(LinearGridSubCell<ObjectType>, bboxMin) == 0 && offsetof(LinearGridSubCell<ObjectType>, bboxMax) == 16);
    return *(bbox3f *)&bboxMin.x;
  }
  void setBBox(bbox3f bbox)
  {
    v_st(&bboxMin.x, v_perm_xyzd(bbox.bmin, v_ld(&bboxMin.x)));
    v_st(&bboxMax.x, v_perm_xyzd(bbox.bmax, v_ld(&bboxMax.x)));
  }
  void addBBox(bbox3f bbox) { setBBox(v_bbox3_sum(getBBox(), bbox)); }

  Point3 bboxMin = Point3(FLT_MAX, FLT_MAX, FLT_MAX);
  leaf_id_t rootLeaf = EMPTY_LEAF;
#if EXTRA_SMALL_BRANCHES_AND_64K_LEAFS_LIMIT
  uint16_t _unused_padding = 0;
#endif
  Point3 bboxMax = Point3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
  uint32_t empty = 1; // replace by uint64_t mask in grid?
};

template <typename ObjectType>
class LinearSubGrid // 2048+16 bytes
{
public:
  typedef LinearGridSubCell<ObjectType> CellType;

  bool isEmpty() const { return false; } // actually impossible
  unsigned getGridWidth() const { return LINEAR_GRID_SUBGRID_WIDTH; }
  LinearGridSubCell<ObjectType> &getCellByIds(vec4i v_sub_cell_ids) { return cellsData[getCellOffset(v_sub_cell_ids)]; } // from zero
  const LinearGridSubCell<ObjectType> &getCellByOffset(unsigned offset) const { return cellsData[offset]; } // from min corner
  LinearGridSubCell<ObjectType> &getCellByOffset(unsigned offset) { return cellsData[offset]; }             // from min corner
  const auto &getCells() const { return cellsData; }
  vec4f getLowestCellBox() const { return lowestCellBox; }
  void setLowestCellBox(vec4f box) { lowestCellBox = box; }

  __forceinline static bool shouldUseWooRay(vec4i clamped_limits)
  {
    LG_VERIFYF(v_signmask(v_and(v_cmp_gti(clamped_limits, v_splatsi(-1)),
                 v_cmp_lti(clamped_limits, v_splatsi(LINEAR_GRID_SUBGRID_WIDTH)))) == 0b1111,
      "Not clamped limits");
    const int MIN_SIDE = 4;
    uint64_t from = v_extract_xi64(clamped_limits);
    uint64_t to = v_extract_yi64(clamped_limits);
    uint64_t size = to - from;
    bool zcheck = unsigned(size >> 32) >= MIN_SIDE;
    bool xcheck = unsigned(size) >= MIN_SIDE;
    return EASTL_UNLIKELY(zcheck && xcheck);
  }

  static unsigned getCellOffset(vec4i v_sub_ids)
  {
    uint64_t mask = LINEAR_GRID_SUBGRID_WIDTH - 1;
    uint64_t offsetXZ = v_extract_xi64(v_sub_ids) & ((mask << 32) | mask);
    return unsigned(offsetXZ >> 32) * LINEAR_GRID_SUBGRID_WIDTH + unsigned(offsetXZ);
  }

  void insertAt(LinearGrid<ObjectType> *parent_grid, ObjectType object, vec4i v_sub_cell_ids, bbox3f wbb)
  {
    unsigned offset = getCellOffset(v_sub_cell_ids);
    LinearGridSubCell<ObjectType> &cell = cellsData[offset];
    if (cell.rootLeaf == EMPTY_LEAF)
    {
      cell.rootLeaf = parent_grid->createLeaf(false /*is_branch*/);
      if (EASTL_UNLIKELY(cell.rootLeaf == EMPTY_LEAF))
        return;
    }
    cell.empty = 0;
    bbox3f oldCellBox = cell.getBBox();
    cell.setBBox(v_bbox3_sum(oldCellBox, wbb));
    leaf_insert_object(parent_grid, cell.rootLeaf, oldCellBox, wbb, object);
  }

  bool eraseAt(LinearGrid<ObjectType> *parent_grid, ObjectType object, vec4i v_sub_cell_ids)
  {
    LinearGridSubCell<ObjectType> &cell = getCellByIds(v_sub_cell_ids);
    if (cell.rootLeaf != EMPTY_LEAF && leaf_erase_object(parent_grid, cell.rootLeaf, object))
    {
      cell.empty = (!parent_grid->isBranch(cell.rootLeaf) && parent_grid->getLeaf(cell.rootLeaf).objects.empty()) ? 1 : 0;
      return true;
    }
    return false;
  }

private:
  vec4f lowestCellBox;
  eastl::array<LinearGridSubCell<ObjectType>, LINEAR_GRID_SUBGRID_WIDTH * LINEAR_GRID_SUBGRID_WIDTH> cellsData;
};

template <typename ObjectType>
struct alignas(16) LinearGridMainCell // 32 bytes
{
  bool isEmpty() const { return false; } // 99-100% cases
  const bbox3f &getBBox() const
  {
    static_assert(offsetof(LinearGridSubCell<ObjectType>, bboxMin) == 0 && offsetof(LinearGridSubCell<ObjectType>, bboxMax) == 16);
    return *(bbox3f *)&bboxMin.x;
  }
  void setBBox(bbox3f bbox)
  {
    v_st(&bboxMin.x, v_perm_xyzd(bbox.bmin, v_ld(&bboxMin.x)));
    v_st(&bboxMax.x, v_perm_xyzd(bbox.bmax, v_ld(&bboxMax.x)));
  }
  void addBBox(bbox3f bbox) { setBBox(v_bbox3_sum(getBBox(), bbox)); }

  Point3 bboxMin = Point3(FLT_MAX, FLT_MAX, FLT_MAX);
  leaf_id_t rootLeaf = EMPTY_LEAF;
#if EXTRA_SMALL_BRANCHES_AND_64K_LEAFS_LIMIT
  uint16_t _unused_padding = 0;
#endif
  Point3 bboxMax = Point3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
  int32_t subGridIdx = -1;
};

// We have two boxes and know their sum on packing/unpacking
// At least 6 of 12 boxes dimensions should be equal to parent box
// We encode unequal dimensions as offset directed to box center
// Less significant bit in offset means that offseted dimension should be used for left box
__forceinline static void unpack_bboxes(bbox3f &left_bbox, bbox3f &right_bbox, bbox3f parent_bbox, vec4f min_offset, vec4f min_mask,
  vec4f max_offset, vec4f max_mask)
{
  left_bbox.bmin = v_add(parent_bbox.bmin, v_and(min_mask, min_offset));
  right_bbox.bmin = v_add(parent_bbox.bmin, v_andnot(min_mask, min_offset));
  left_bbox.bmax = v_sub(parent_bbox.bmax, v_and(max_mask, max_offset));
  right_bbox.bmax = v_sub(parent_bbox.bmax, v_andnot(max_mask, max_offset));

  LG_VERIFYF(!v_bbox3_is_empty(parent_bbox), "Empty=" FMT_B3, VB3D(parent_bbox));
  LG_VERIFYF(!v_bbox3_is_empty(left_bbox), "Empty=" FMT_B3, VB3D(left_bbox));
  LG_VERIFYF(!v_bbox3_is_empty(right_bbox), "Empty=" FMT_B3, VB3D(right_bbox));
  LG_VERIFY(v_bbox3_test_box_inside(parent_bbox, left_bbox));
  LG_VERIFY(v_bbox3_test_box_inside(parent_bbox, right_bbox));
}

// Unpack from 16-byte branch
// It contain 3xUINT16 in low 64-bit part for box.bmin offsets, and same in high part for box.bmax
__forceinline static void unpack_bboxes_16b(bbox3f &left_bbox, bbox3f &right_bbox, bbox3f parent_bbox, vec4i packed_correction)
{
  vec4i one = v_subi(v_zeroi(), v_set_all_bitsi());
  vec4f unpackStep = v_mul(v_bbox3_size(parent_bbox), v_splats(1.f / UINT16_MAX));
  vec4i unpackedMinCorrection = v_cvt_lo_ush_vec4i(packed_correction);
  vec4i unpackedMaxCorrection = v_cvt_hi_ush_vec4i(packed_correction);
  vec4i minMask = v_cmp_eqi(v_andi(unpackedMinCorrection, one), one);
  vec4i maxMask = v_cmp_eqi(v_andi(unpackedMaxCorrection, one), one);
  vec4f minOffset = v_mul(v_cvti_vec4f(unpackedMinCorrection), unpackStep);
  vec4f maxOffset = v_mul(v_cvti_vec4f(unpackedMaxCorrection), unpackStep);
  unpack_bboxes(left_bbox, right_bbox, parent_bbox, minOffset, v_cast_vec4f(minMask), maxOffset, v_cast_vec4f(maxMask));
}

// Unpack from 32-byte branch
// Correction bit stored in sign
__forceinline static void unpack_bboxes_32b(bbox3f &left_bbox, bbox3f &right_bbox, bbox3f parent_bbox, vec4f min_correction,
  vec4f max_correction)
{
  vec4f minMask = v_cmp_lt(min_correction, v_zero());
  vec4f maxMask = v_cmp_lt(max_correction, v_zero());
  left_bbox.bmin = v_sub(parent_bbox.bmin, v_and(minMask, min_correction));
  right_bbox.bmin = v_add(parent_bbox.bmin, v_andnot(minMask, min_correction));
  left_bbox.bmax = v_add(parent_bbox.bmax, v_and(maxMask, max_correction));
  right_bbox.bmax = v_sub(parent_bbox.bmax, v_andnot(maxMask, max_correction));

  LG_VERIFYF(!v_bbox3_is_empty(parent_bbox), "Empty=" FMT_B3, VB3D(parent_bbox));
  LG_VERIFYF(!v_bbox3_is_empty(left_bbox), "Empty=" FMT_B3, VB3D(left_bbox));
  LG_VERIFYF(!v_bbox3_is_empty(right_bbox), "Empty=" FMT_B3, VB3D(right_bbox));
  LG_VERIFY(v_bbox3_test_box_inside(parent_bbox, left_bbox));
  LG_VERIFY(v_bbox3_test_box_inside(parent_bbox, right_bbox));
}

__forceinline static void pack_bboxes(vec4f &min_offset, vec4f &min_mask, vec4f &max_offset, vec4f &max_mask, bbox3f left_bbox,
  bbox3f right_bbox, bbox3f parent_bbox)
{
  LG_VERIFY(!v_bbox3_is_empty(parent_bbox));
  LG_VERIFY(!v_bbox3_is_empty(left_bbox));
  LG_VERIFY(!v_bbox3_is_empty(right_bbox));
  LG_VERIFY(v_bbox3_test_box_inside(parent_bbox, left_bbox));
  LG_VERIFY(v_bbox3_test_box_inside(parent_bbox, right_bbox));

  bbox3f innerBox = v_bbox3_get_box_intersection(left_bbox, right_bbox);
  min_offset = v_sub(innerBox.bmin, parent_bbox.bmin);
  max_offset = v_sub(parent_bbox.bmax, innerBox.bmax);
  min_mask = v_cmp_ge(left_bbox.bmin, right_bbox.bmin);
  max_mask = v_cmp_le(left_bbox.bmax, right_bbox.bmax);
}

__forceinline static vec4i pack_bboxes_16b(bbox3f left_bbox, bbox3f right_bbox, bbox3f parent_bbox)
{
  vec4f minOffset, minMask, maxOffset, maxMask;
  pack_bboxes(minOffset, minMask, maxOffset, maxMask, left_bbox, right_bbox, parent_bbox);
  vec4f packStep = v_mul(v_bbox3_size(parent_bbox), v_splats(1.f / UINT16_MAX));
  vec4f unpackedMinCorrection = v_div(minOffset, packStep);
  vec4f unpackedMaxCorrection = v_div(maxOffset, packStep);
#if VERIFY_ALL
  vec4f packOverflow =
    v_or(v_cmp_gt(unpackedMinCorrection, v_splats(UINT16_MAX - 1)), v_cmp_gt(unpackedMaxCorrection, v_splats(UINT16_MAX - 1)));
  LG_VERIFYF((v_signmask(packOverflow) & 0b111) == 0, "uint16 overflow on box packing");
#endif
  // floori makes inner boxes clother to parent (bigger)
  vec4i one = v_subi(v_zeroi(), v_set_all_bitsi());
  vec4i packLow = v_cvt_floori(unpackedMinCorrection);
  vec4i packHigh = v_cvt_floori(unpackedMaxCorrection);
  vec4i minZero = v_cmp_eqi(packLow, v_zeroi());
  vec4i maxZero = v_cmp_eqi(packHigh, v_zeroi());
  packLow = v_subi(packLow, one); // fix float type rounding
  packHigh = v_subi(packHigh, one);
  vec4i fixMin = v_andi(v_xori(packLow, v_cast_vec4i(minMask)), one);
  vec4i fixMax = v_andi(v_xori(packHigh, v_cast_vec4i(maxMask)), one);
  return v_packus(v_andnoti(minZero, v_subi(packLow, fixMin)), v_andnoti(maxZero, v_subi(packHigh, fixMax)));
}

__forceinline static void pack_bboxes_32b(vec4f &min_correction, vec4f &max_correction, bbox3f left_bbox, bbox3f right_bbox,
  bbox3f parent_bbox)
{
  vec4f minOffset, minMask, maxOffset, maxMask;
  pack_bboxes(minOffset, minMask, maxOffset, maxMask, left_bbox, right_bbox, parent_bbox);
  minOffset = v_andnot(v_cmp_eqi(minOffset, v_zero()), v_cast_vec4f(v_addi(v_cast_vec4i(minOffset), v_set_all_bitsi()))); // fix float
  maxOffset = v_andnot(v_cmp_eqi(maxOffset, v_zero()), v_cast_vec4f(v_addi(v_cast_vec4i(maxOffset), v_set_all_bitsi()))); // rounding
  min_correction = v_or(minOffset, v_and(minMask, V_CI_SIGN_MASK));
  max_correction = v_or(maxOffset, v_and(maxMask, V_CI_SIGN_MASK));
}

template <typename ObjectType, typename ObjectsIterator>
__forceinline eastl::pair<bool, ObjectType> find_object_intersection(const dag::Vector<ObjectType, MidmemAlloc> &objects, bbox3f box,
  const ObjectsIterator &__restrict objects_iterator)
{
  for (ObjectType object : objects)
  {
    if (objects_iterator.checkObjectBounding(object, box) && objects_iterator.predFunc(object))
      return eastl::make_pair(true, object);
  }
  return eastl::make_pair(false, ObjectType::null());
}

template <typename ObjectType, typename ObjectsIterator>
__forceinline eastl::pair<bool, ObjectType> find_object_intersection(const dag::Vector<ObjectType, MidmemAlloc> &objects,
  const LinearGridRay *__restrict ray, const ObjectsIterator &__restrict objects_iterator)
{
  for (ObjectType object : objects)
  {
    if (objects_iterator.checkObjectBounding(object, ray->start, ray->dir, ray->len, ray->radius) && objects_iterator.predFunc(object))
      return eastl::make_pair(true, object);
  }
  return eastl::make_pair(false, ObjectType::null());
}

template <typename ObjectType, typename FilterType, typename ObjectsIterator>
VECTORCALL DAGOR_NOINLINE static ObjectType leaf_iterate_intersected(const LinearGrid<ObjectType> *__restrict grid, leaf_id_t leaf_idx,
  bbox3f parent_leaf_box, FilterType bbox_or_ray, const ObjectsIterator &__restrict objects_iterator)
{
  LG_VERIFY(leaf_idx != EMPTY_LEAF);
  if (grid->isBranch(leaf_idx))
  {
    UnpackedBranch branch = grid->unpackBranch(leaf_idx, parent_leaf_box);
    bool leftIntersected, rightIntersected;
    if constexpr (eastl::is_same_v<FilterType, bbox3f>)
    {
      leftIntersected = v_bbox3_test_box_intersect(bbox_or_ray, branch.leftBox);
      rightIntersected = v_bbox3_test_box_intersect(bbox_or_ray, branch.rightBox);
    }
    else
    {
      bbox3f extLeftBox = branch.leftBox;
      bbox3f extRightBox = branch.rightBox;
      if (objects_iterator.isCapsule())
      {
        v_bbox3_extend(extLeftBox, bbox_or_ray->radius);
        v_bbox3_extend(extRightBox, bbox_or_ray->radius);
      }
      leftIntersected = v_test_ray_box_intersection_unsafe(bbox_or_ray->start, bbox_or_ray->dir, bbox_or_ray->len, extLeftBox);
      rightIntersected = v_test_ray_box_intersection_unsafe(bbox_or_ray->start, bbox_or_ray->dir, bbox_or_ray->len, extRightBox);
    }

    if (leftIntersected)
    {
      ObjectType object = leaf_iterate_intersected(grid, branch.leftIdx, branch.leftBox, bbox_or_ray, objects_iterator);
      if (object != ObjectType::null())
        return object;
    }
    if (rightIntersected)
    {
      ObjectType object = leaf_iterate_intersected(grid, branch.rightIdx, branch.rightBox, bbox_or_ray, objects_iterator);
      if (object != ObjectType::null())
        return object;
    }
  }
  else // lnode
  {
    eastl::pair<bool, ObjectType> isect = find_object_intersection(grid->getLeaf(leaf_idx).objects, bbox_or_ray, objects_iterator);
    if (isect.first)
      return isect.second;
  }
  return ObjectType::null();
}

template <typename ObjectType, typename ObjectsIterator>
DAGOR_NOINLINE static ObjectType leaf_iterate_all(const LinearGrid<ObjectType> *__restrict grid, leaf_id_t leaf_idx,
  ObjectsIterator &__restrict objects_iterator)
{
  LG_VERIFY(leaf_idx != EMPTY_LEAF);
  const LinearGridLeaf<ObjectType> &leaf = grid->getLeaf(leaf_idx);
  if (grid->isBranch(leaf_idx))
  {
    UnpackedBranch branch = grid->unpackBranch(leaf_idx);
    ObjectType object = leaf_iterate_all(grid, branch.leftIdx, objects_iterator);
    if (object != ObjectType::null())
      return object;
    object = leaf_iterate_all(grid, branch.rightIdx, objects_iterator);
    if (object != ObjectType::null())
      return object;
  }
  else // lnode
  {
    for (ObjectType object : leaf.objects)
    {
      if (objects_iterator.predFunc(object))
        return object;
    }
  }
  return ObjectType::null();
}

template <typename ObjectType>
__forceinline void leaf_insert_object_to_better_branch(LinearGrid<ObjectType> *__restrict grid, leaf_id_t leaf_idx,
  UnpackedBranch branch, bbox3f wbb, ObjectType object)
{
  bbox3f extLeft = v_bbox3_sum(branch.leftBox, wbb);
  bbox3f extRight = v_bbox3_sum(branch.rightBox, wbb);
  bbox3f extLeftIsect = v_bbox3_get_box_intersection(extLeft, branch.rightBox);
  bbox3f extRightIsect = v_bbox3_get_box_intersection(branch.leftBox, extRight);
  vec4f leftIsectVol = v_hmul3(v_max(v_bbox3_size(extLeftIsect), v_zero()));
  vec4f rightIsectVol = v_hmul3(v_max(v_bbox3_size(extRightIsect), v_zero()));
  vec4f leftBetter = v_cmp_le(leftIsectVol, rightIsectVol);
  if (v_test_vec_x_eq(leftIsectVol, rightIsectVol))
  {
    vec4f leftGrow = v_add(v_sub(extLeft.bmax, branch.leftBox.bmax), v_sub(branch.leftBox.bmin, extLeft.bmin));
    vec4f rightGrow = v_add(v_sub(extRight.bmax, branch.rightBox.bmax), v_sub(branch.rightBox.bmin, extRight.bmin));
    leftBetter = v_cmp_le(v_length3_sq(leftGrow), v_length3_sq(rightGrow));
  }
  bbox3f oldBox = v_bbox3_sel(branch.rightBox, branch.leftBox, leftBetter);
  bbox3f leftBox = v_bbox3_sel(branch.leftBox, extLeft, leftBetter);
  bbox3f rightBox = v_bbox3_sel(extRight, branch.rightBox, leftBetter);
  bbox3f newParentBox = v_bbox3_sum(extLeft, extRight);
  LG_VERIFY(v_bbox3_test_box_inside(newParentBox, wbb));
  LG_VERIFY(v_bbox3_test_box_inside(leftBox, wbb) || v_bbox3_test_box_inside(rightBox, wbb));

  grid->packBranch(leaf_idx, leftBox, rightBox, newParentBox);
  leaf_id_t destIdx = v_extract_xi(v_cast_vec4i(leftBetter)) ? branch.leftIdx : branch.rightIdx;
  leaf_insert_object(grid, destIdx, oldBox, wbb, object);
}

template <typename ObjectType>
VECTORCALL DAGOR_NOINLINE static void leaf_insert_object(LinearGrid<ObjectType> *__restrict grid, leaf_id_t leaf_idx,
  bbox3f old_parent_leaf_box, bbox3f wbb, ObjectType object)
{
  LG_VERIFY(leaf_idx != EMPTY_LEAF);
  if (grid->isBranch(leaf_idx))
  {
    UnpackedBranch branch = grid->unpackBranch(leaf_idx, old_parent_leaf_box);
    if (v_bbox3_test_box_inside(branch.leftBox, wbb))
      return leaf_insert_object(grid, branch.leftIdx, branch.leftBox, wbb, object);
    if (v_bbox3_test_box_inside(branch.rightBox, wbb))
      return leaf_insert_object(grid, branch.rightIdx, branch.rightBox, wbb, object);
    leaf_insert_object_to_better_branch(grid, leaf_idx, branch, wbb, object);
  }
  else // lnode
  {
    LinearGridLeaf<ObjectType> &leaf = grid->getLeaf(leaf_idx);
    if (leaf.objects.size() == leaf.objects.capacity() && grid->isOptimized())
      leaf.objects.reserve(leaf.objects.size() + grid->getObjectsGrowStep());
    leaf.objects.emplace_back(object);
    if (grid->needCreateBranch(leaf.objects.size()))
    {
      dag::Vector<ObjectType, MidmemAlloc> objects = eastl::move(leaf.objects);
      grid->createBranchOnLeaf(leaf_idx, eastl::move(objects), v_bbox3_sum(old_parent_leaf_box, wbb));
    }
  }
}

template <typename ObjectType>
DAGOR_NOINLINE static bool leaf_erase_object(LinearGrid<ObjectType> *__restrict grid, leaf_id_t leaf_idx, ObjectType object)
{
  LG_VERIFY(leaf_idx != EMPTY_LEAF);
  if (grid->isBranch(leaf_idx))
  {
    UnpackedBranch branch = grid->unpackBranch(leaf_idx);
    return leaf_erase_object(grid, branch.leftIdx, object) || leaf_erase_object(grid, branch.rightIdx, object);
  }

  LinearGridLeaf<ObjectType> &leaf = grid->getLeaf(leaf_idx);
  auto it = eastl::find(leaf.objects.begin(), leaf.objects.end(), object);
  if (it != leaf.objects.end())
  {
    leaf.objects.erase(it);
    return true;
  }
  return false;
}

template <typename ObjectType>
DAGOR_NOINLINE static unsigned leaf_count_objects(const LinearGrid<ObjectType> *__restrict grid, leaf_id_t leaf_idx)
{
  LG_VERIFY(leaf_idx != EMPTY_LEAF);
  if (grid->isBranch(leaf_idx)) // bnode
  {
    UnpackedBranch branch = grid->unpackBranch(leaf_idx);
    return leaf_count_objects(grid, branch.leftIdx) + leaf_count_objects(grid, branch.rightIdx);
  }
  return grid->getLeaf(leaf_idx).objects.size();
}

template <typename ObjectType, typename T>
VECTORCALL DAGOR_NOINLINE static void leaf_iterate_branch_boxes(const LinearGrid<ObjectType> *__restrict grid, leaf_id_t leaf_idx,
  bbox3f parent_leaf_box, int min_depth, const T &cb)
{
  LG_VERIFY(leaf_idx != EMPTY_LEAF);
  if (grid->isBranch(leaf_idx)) // bnode
  {
    UnpackedBranch branch = grid->unpackBranch(leaf_idx, parent_leaf_box);
    if (min_depth <= 0)
    {
      cb(branch.leftBox, leaf_count_objects(grid, branch.leftIdx), leaf_idx);
      cb(branch.rightBox, leaf_count_objects(grid, branch.rightIdx), leaf_idx);
    }
    else
    {
      leaf_iterate_branch_boxes(grid, branch.leftIdx, branch.leftBox, min_depth - 1, cb);
      leaf_iterate_branch_boxes(grid, branch.rightIdx, branch.rightBox, min_depth - 1, cb);
    }
  }
  else
    cb(parent_leaf_box, grid->getLeaf(leaf_idx).objects.size(), leaf_idx);
}

template <typename ObjectType>
VECTORCALL DAGOR_NOINLINE static void leaf_repack_bboxes(LinearGrid<ObjectType> *__restrict grid, leaf_id_t leaf_idx,
  bbox3f old_parent_box, bbox3f new_parent_box)
{
  if (grid->isBranch(leaf_idx))
  {
    UnpackedBranch oldBranch = grid->unpackBranch(leaf_idx, old_parent_box);
    grid->packBranch(leaf_idx, oldBranch.leftBox, oldBranch.rightBox, new_parent_box);
    UnpackedBranch newBranch = grid->unpackBranch(leaf_idx, new_parent_box);
    leaf_repack_bboxes(grid, newBranch.leftIdx, oldBranch.leftBox, newBranch.leftBox);
    leaf_repack_bboxes(grid, newBranch.rightIdx, oldBranch.rightBox, newBranch.rightBox);
  }
}

template <typename ObjectType>
VECTORCALL DAGOR_NOINLINE static bool leaf_verify_bboxes(LinearGrid<ObjectType> *__restrict grid, leaf_id_t leaf_idx,
  bbox3f parent_leaf_box, ObjectType ignore)
{
  LG_VERIFY(leaf_idx != EMPTY_LEAF);
  if (grid->isBranch(leaf_idx))
  {
    UnpackedBranch branch = grid->unpackBranch(leaf_idx, parent_leaf_box);
    return leaf_verify_bboxes(grid, branch.leftIdx, branch.leftBox, ignore) &&
           leaf_verify_bboxes(grid, branch.rightIdx, branch.rightBox, ignore);
  }
  else // lnode
  {
    for (ObjectType obj : grid->getLeaf(leaf_idx).objects)
    {
      if (obj == ignore) // ignore currently updating object
        continue;
      vec4f wbsph = obj.getWBSph();
      bbox3f sphBox;
      v_bbox3_init_by_bsph(sphBox, wbsph, v_bsph_radius(wbsph));
      bbox3f bbox = v_bbox3_get_box_intersection(sphBox, obj.getWBBox());
      if (!v_bbox3_test_box_inside(parent_leaf_box, bbox))
        return false;
    }
    return true;
  }
}

template <typename GridType, typename ObjectType>
class LinearGridBoxIteratorImpl
{
public:
  typedef typename GridType::CellType CellType;
  template <typename>
  friend class LinearGridBoxIterator;

  template <typename T>
  __forceinline ObjectType foreachCell(T cell_callback)
  {
    if (EASTL_LIKELY(!grid->isEmpty()))
    {
      do
      {
        unsigned offset = unsigned(xz >> 32) * grid->getGridWidth() + unsigned(xz);
        const CellType &cv = grid->getCellByOffset(offset);
        if (cv.isEmpty())
          continue;
        ObjectType obj = cell_callback(xz, cv);
        if (EASTL_UNLIKELY(obj != ObjectType::null()))
          return obj;
      } while (advance());
    }
    return ObjectType::null();
  }

private:
  __forceinline LinearGridBoxIteratorImpl(const GridType *p_grid, vec4i limits, bbox3f bbox) : grid(p_grid), queryBox(bbox)
  {
    xz = v_extract_xi64(limits);
    maxXmaxZ = v_extract_yi64(limits);
    nextRow = (int64_t(1) << 32) + int(xz) - int(maxXmaxZ);
  }

#if defined _MSC_VER && !defined(__clang__) && !defined(_TARGET_64BIT) // fix 32-bit msvc compiler bug with .x bad optimization
  DAGOR_NOINLINE
#else
  __forceinline
#endif
  bool advance()
  {
    if (EASTL_UNLIKELY(xz == maxXmaxZ))
      return false;
    bool rowEnded = unsigned(xz) == unsigned(maxXmaxZ);
    uint64_t add = rowEnded ? 0 : 1;
    if (rowEnded)
      add = nextRow;
    xz += add;
    return true;
  }

  const GridType *__restrict grid;
  bbox3f queryBox;
  uint64_t xz;
  uint64_t maxXmaxZ;
  uint64_t nextRow;
};

template <typename ObjectType>
class LinearGridBoxIterator
{
public:
  template <typename>
  friend class LinearGrid;

  template <typename ObjectsIterator>
  __forceinline ObjectType foreach(const ObjectsIterator &objects_iterator)
  {
    eastl::pair<bool, ObjectType> isect = find_object_intersection(grid->oversizeObjects, queryBox, objects_iterator);
    if (EASTL_UNLIKELY(isect.first))
      return isect.second;

    LinearGridBoxIteratorImpl<LinearGrid<ObjectType>, ObjectType> mainLayerIterator(grid, mainLimits, queryBox);
    return mainLayerIterator.foreachCell([&](uint64_t xz, const LinearGridMainCell<ObjectType> &__restrict cv) FORCEINLINE_ATTR {
      if (!objects_iterator.checkBoxBounding(cv.getBBox(), queryBox))
        return ObjectType::null();
      if (EASTL_LIKELY(cv.rootLeaf != EMPTY_LEAF))
      {
        ObjectType object = leaf_iterate_intersected(grid, cv.rootLeaf, cv.getBBox(), queryBox, objects_iterator);
        if (EASTL_UNLIKELY(object != ObjectType::null()))
          return object;
      }
      if (const LinearSubGrid<ObjectType> *__restrict subGrid = grid->getSubGrid(cv.subGridIdx))
      {
        uint64_t subgridOffset = xz << LINEAR_GRID_SUBGRID_WIDTH_LOG2;
        vec4i relOffsets = v_subi(subLimits, v_splatsi64(subgridOffset));
        vec4i haveIntersection = v_cmp_lti(relOffsets, v_make_vec4i(LINEAR_GRID_SUBGRID_WIDTH, LINEAR_GRID_SUBGRID_WIDTH, 0, 0));
        // sometimes we intersect only main cell extension, but subgrid's is smaller
        if (v_signmask(v_cast_vec4f(haveIntersection)) != 0b0011)
          return ObjectType::null();
        vec4i subgridLimits = v_clampi(relOffsets, v_zeroi(), v_splatsi(LINEAR_GRID_SUBGRID_WIDTH - 1));
        LinearGridBoxIteratorImpl<LinearSubGrid<ObjectType>, ObjectType> subLayerIterator(subGrid, subgridLimits, queryBox);
        return subLayerIterator.foreachCell([&](uint64_t, const LinearGridSubCell<ObjectType> &__restrict cv) FORCEINLINE_ATTR {
          if (!objects_iterator.checkBoxBounding(cv.getBBox(), queryBox))
            return ObjectType::null();
          return leaf_iterate_intersected(grid, cv.rootLeaf, cv.getBBox(), queryBox, objects_iterator);
        });
      }
      return ObjectType::null();
    });
  }

private:
  __forceinline LinearGridBoxIterator(const LinearGrid<ObjectType> *p_grid, bbox3f bbox, bool with_extension) :
    grid(p_grid), queryBox(bbox)
  {
    bbox3f mainBox = bbox;
    bbox3f subBox = bbox;
    if (with_extension)
    {
      v_bbox3_extend(mainBox, v_splats(grid->maxMainExtension));
      v_bbox3_extend(subBox, v_splats(grid->maxSubExtension));
    }
    mainLimits = grid->getClampedOffsets(mainBox, false /*subgrid*/);
    subLimits = grid->getClampedOffsets(subBox, true /*subgrid*/);
  }

  const LinearGrid<ObjectType> *__restrict grid;
  bbox3f queryBox;
  vec4i mainLimits;
  vec4i subLimits;
};

template <typename GridType, typename ObjectType>
class LinearGridDirectedRayIteratorImpl
{
public:
  typedef typename GridType::CellType CellType;
  template <typename>
  friend class LinearGridRayIterator;

  template <typename T>
  __forceinline ObjectType foreachCell(T cell_callback)
  {
    if (EASTL_LIKELY(!grid->isEmpty()))
    {
      do
      {
        unsigned offset = z * grid->getGridWidth() + x;
        const CellType &cv = grid->getCellByOffset(offset);
        if (EASTL_LIKELY(cv.isEmpty()))
          continue;
        uint64_t xz = (uint64_t(z) << 32) | x;
        ObjectType obj = cell_callback(xz, cv);
        if (EASTL_UNLIKELY(obj != ObjectType::null()))
          return obj;
      } while (advance());
    }
    return ObjectType::null();
  }

private:
  __forceinline LinearGridDirectedRayIteratorImpl(const GridType *p_grid, vec4i limits, const LinearGridRay *p_ray, vec4i ray_dir_xz) :
    grid(p_grid), ray(p_ray), rayDirXZ(v_extract_xi64(ray_dir_xz))
  {
    vec4i invDir = v_cmp_lti(ray_dir_xz, v_zeroi());
    limits = v_seli(limits, v_roti_2(limits), invDir);
    x = v_extract_xi(limits);
    z = v_extract_yi(limits);
    maxXmaxZ = v_extract_yi64(limits);
    minX = x;
  }

#if defined _MSC_VER && !defined(__clang__) && !defined(_TARGET_64BIT) // fix 32-bit msvc compiler bug with .x bad optimization
  DAGOR_NOINLINE
#else
  __forceinline
#endif
  bool advance()
  {
    if (x == unsigned(maxXmaxZ))
    {
      if (z == unsigned(maxXmaxZ >> 32))
        return false;
      z += unsigned(rayDirXZ >> 32);
      x = minX - unsigned(rayDirXZ);
    }
    x += unsigned(rayDirXZ);
    return true;
  }

  const GridType *__restrict grid;
  const LinearGridRay *__restrict ray; // only for leafs iterator
  unsigned x, z;
  uint64_t maxXmaxZ; // combined 2x32bit limits
  unsigned minX;
  uint64_t rayDirXZ;
};

template <typename GridType, typename ObjectType>
class LinearGridWooRayIteratorImpl
{
public:
  typedef typename GridType::CellType CellType;
  template <typename>
  friend class LinearGridRayIterator;

  template <typename T>
  __forceinline ObjectType foreachCell(T cell_callback)
  {
    if (EASTL_LIKELY(!grid->isEmpty()))
    {
      do
      {
        unsigned offset = z * grid->getGridWidth() + x;
        const CellType &cv = grid->getCellByOffset(offset);
        uint64_t xz = (uint64_t(z) << 32) | x;
        if (!checkWooIntersection(xz) || EASTL_LIKELY(cv.isEmpty()))
          continue;
        ObjectType obj = cell_callback(xz, cv);
        if (EASTL_UNLIKELY(obj != ObjectType::null()))
          return obj;
      } while (advance());
    }
    return ObjectType::null();
  }

private:
  __forceinline LinearGridWooRayIteratorImpl(const GridType *p_grid, vec4i limits, const LinearGridRay *p_ray, vec4i ray_dir_xz,
    vec4f lowest_cell_box, unsigned cell_size_log2) :
    grid(p_grid),
    ray(p_ray),
    rayDirXZ(v_extract_xi64(ray_dir_xz)),
    rowIntersectionFound(false),
    extLowestGridCellBox(lowest_cell_box),
    cellSizeLog2(cell_size_log2)
  {
    vec4i invDir = v_cmp_lti(ray_dir_xz, v_zeroi());
    limits = v_seli(limits, v_roti_2(limits), invDir);
    x = v_extract_xi(limits);
    z = v_extract_yi(limits);
    maxXmaxZ = v_extract_yi64(limits);
    minX = x;
  }

  __forceinline bool checkWooIntersection(uint64_t xz)
  {
    // get 2d box of cell extended by ray radius, 3d box projection is always smaller
    vec4i cellBoxOffset = v_splatsi64(xz << cellSizeLog2);
    vec4f bbox2d = v_add(v_cvt_vec4f(cellBoxOffset), extLowestGridCellBox);
    vec3f rayEnd = v_madd(ray->dir, ray->len, ray->start);
    bool isCellHit = test_2d_line_rect_intersection_b(v_perm_xzxz(ray->start), v_perm_xzxz(rayEnd), bbox2d);
    if (isCellHit)
    {
      if (!rowIntersectionFound)
        minX = x;
      rowIntersectionFound = true;
    }
    else
    {
      if (rowIntersectionFound)
        x = unsigned(maxXmaxZ);
    }
    return isCellHit;
  }

#if defined _MSC_VER && !defined(__clang__) && !defined(_TARGET_64BIT) // fix 32-bit msvc compiler bug with .x bad optimization
  DAGOR_NOINLINE
#else
  __forceinline
#endif
  bool advance()
  {
    if (EASTL_UNLIKELY(x == unsigned(maxXmaxZ)))
    {
      if (EASTL_UNLIKELY(z == unsigned(maxXmaxZ >> 32)))
        return false;
      z += unsigned(rayDirXZ >> 32);
      x = minX - unsigned(rayDirXZ);
      rowIntersectionFound = false;
    }
    x += unsigned(rayDirXZ);
    return true;
  }

  const GridType *__restrict grid;
  const LinearGridRay *__restrict ray;
  unsigned cellSizeLog2;
  unsigned x, z;
  uint64_t maxXmaxZ; // combined 2x32bit limits
  unsigned minX;
  uint64_t rayDirXZ;
  bool rowIntersectionFound;
  vec4f extLowestGridCellBox;
};

template <typename ObjectType>
class LinearGridRayIterator
{
public:
  template <typename>
  friend class LinearGrid;

  template <typename ObjectsIterator>
  __forceinline ObjectType foreach(const ObjectsIterator &objects_iterator)
  {
    eastl::pair<bool, ObjectType> isect = find_object_intersection(grid->oversizeObjects, &ray, objects_iterator);
    if (EASTL_UNLIKELY(isect.first))
      return isect.second;

    auto mainLayerCb = [this, &objects_iterator](uint64_t xz, const LinearGridMainCell<ObjectType> &__restrict cv) FORCEINLINE_ATTR {
      if (!objects_iterator.checkBoxBounding(cv.getBBox(), false /*is_safe*/, ray.start, ray.dir, ray.len, ray.radius))
        return ObjectType::null();
      if (EASTL_LIKELY(cv.rootLeaf != EMPTY_LEAF)) // TODO: separate sub and main boxes
      {
        const LinearGridRay *__restrict rayPtr = &ray;
        ObjectType object = leaf_iterate_intersected(grid, cv.rootLeaf, cv.getBBox(), rayPtr, objects_iterator);
        if (EASTL_UNLIKELY(object != ObjectType::null()))
          return object;
      }
      if (const LinearSubGrid<ObjectType> *__restrict subGrid = grid->getSubGrid(cv.subGridIdx))
      {
        uint64_t subgridOffset = xz << LINEAR_GRID_SUBGRID_WIDTH_LOG2;
        vec4i relOffsets = v_subi(subLimits, v_splatsi64(subgridOffset));
        vec4i haveIntersection = v_cmp_lti(relOffsets, v_make_vec4i(LINEAR_GRID_SUBGRID_WIDTH, LINEAR_GRID_SUBGRID_WIDTH, 0, 0));
        // sometimes we intersect only main cell extension, but subgrid's is smaller
        if (v_signmask(v_cast_vec4f(haveIntersection)) != 0b0011)
          return ObjectType::null();
        vec4i clampedSubLimits = v_clampi(relOffsets, v_zeroi(), v_splatsi(LINEAR_GRID_SUBGRID_WIDTH - 1));
        auto subLayerCb = [this, &objects_iterator](uint64_t, const LinearGridSubCell<ObjectType> &__restrict cv) FORCEINLINE_ATTR {
          if (!objects_iterator.checkBoxBounding(cv.getBBox(), true /*is_safe*/, ray.start, ray.dir, ray.len, ray.radius))
            return ObjectType::null();
          const LinearGridRay *__restrict rayPtr = &ray;
          return leaf_iterate_intersected(grid, cv.rootLeaf, cv.getBBox(), rayPtr, objects_iterator);
        };

        if (subGrid->shouldUseWooRay(clampedSubLimits))
        {
          vec4f extLowestCellBox = v_bbox2_extend(subGrid->getLowestCellBox(), subExt);
          LinearGridWooRayIteratorImpl<LinearSubGrid<ObjectType>, ObjectType> subWooRayIterator(subGrid, clampedSubLimits, &ray,
            rayDirXZ, extLowestCellBox, grid->cellSizeLog2 - LINEAR_GRID_SUBGRID_WIDTH_LOG2);
          return subWooRayIterator.foreachCell(subLayerCb);
        }
        else
        {
          LinearGridDirectedRayIteratorImpl<LinearSubGrid<ObjectType>, ObjectType> subDirectedRayIterator(subGrid, clampedSubLimits,
            &ray, rayDirXZ);
          return subDirectedRayIterator.foreachCell(subLayerCb);
        }
      }
      return ObjectType::null();
    };

    if (grid->shouldUseWooRay(mainLimits))
    {
      vec4f extLowestCellBox = v_bbox2_extend(grid->getLowestCellBox(), mainExt);
      LinearGridWooRayIteratorImpl<LinearGrid<ObjectType>, ObjectType> mainWooRayIterator(grid, mainLimits, &ray, rayDirXZ,
        extLowestCellBox, grid->cellSizeLog2);
      return mainWooRayIterator.foreachCell(mainLayerCb);
    }
    else
    {
      LinearGridDirectedRayIteratorImpl<LinearGrid<ObjectType>, ObjectType> mainSimpleRayIterator(grid, mainLimits, &ray, rayDirXZ);
      return mainSimpleRayIterator.foreachCell(mainLayerCb);
    }
  }

private:
  __forceinline LinearGridRayIterator(const LinearGrid<ObjectType> *p_grid, vec3f ray_start, vec3f ray_dir, vec4f ray_len,
    vec4f ray_radius, bool with_extension) :
    ray(ray_start, ray_dir, ray_len, ray_radius), grid(p_grid)
  {
    vec4f rayDirMask = v_cmp_ge(ray_dir, v_zero());
    vec4i rayDirMaskXz = v_cast_vec4i(v_perm_xzxz(rayDirMask));
    rayDirXZ = v_seli(v_set_all_bitsi(), v_negi(rayDirMaskXz), rayDirMaskXz);

    bbox3f bbox;
    v_bbox3_init_by_ray(bbox, ray_start, ray_dir, ray_len);
    bbox3f mainBox = bbox;
    bbox3f subBox = bbox;
    mainExt = v_add(v_splats(with_extension ? grid->maxMainExtension : 0.f), ray_radius);
    subExt = v_add(v_splats(with_extension ? grid->maxSubExtension : 0.f), ray_radius);
    v_bbox3_extend(mainBox, mainExt);
    v_bbox3_extend(subBox, subExt);
    mainLimits = grid->getClampedOffsets(mainBox, false /*subgrid*/);
    subLimits = grid->getClampedOffsets(subBox, true /*subgrid*/);
  }

  LinearGridRay ray;
  const LinearGrid<ObjectType> *__restrict grid;
  vec4i rayDirXZ;
  vec4i mainLimits;
  vec4i subLimits;
  vec4f mainExt;
  vec4f subExt;
};

template <typename ObjectType>
class alignas(EA_CACHE_LINE_SIZE) LinearGrid
{
public:
  typedef LinearGridMainCell<ObjectType> CellType;
  friend LinearGridBoxIterator<ObjectType>;
  friend LinearGridRayIterator<ObjectType>;
  friend LinearGridWooRayIteratorImpl<LinearGrid<ObjectType>, ObjectType>;
  static_assert(sizeof(ObjectType) <= sizeof(uint64_t)); // Objects always passed by copy
  static_assert(sizeof(LinearGridSubCell<ObjectType>) == sizeof(bbox3f));
  static_assert(sizeof(LinearGridMainCell<ObjectType>) == sizeof(bbox3f));

  template <typename, typename>
  friend class LinearGridBoxIteratorImpl;

  LinearGrid()
  {
    gridSize = IBBox2(IPoint2(), IPoint2());
    lowestCellBox = v_zero();
    cellsData = nullptr;
    leafsData = nullptr;
    cellSizeLog2 = get_const_log2(LINEAR_GRID_DEFAULT_CELL_SIZE);
    gridWidth = 0;
    maxMainExtension = 0.f;
    maxSubExtension = 0.f;
    optimized = false;
  }

  ~LinearGrid() { clear(); }

  __forceinline auto getBoxIterator(bbox3f bbox, bool with_extension) const
  {
    return LinearGridBoxIterator<ObjectType>(this, bbox, with_extension);
  }

  __forceinline auto getRayIterator(vec3f from, vec3f dir, vec4f len, vec4f radius, bool with_extension) const
  {
    return LinearGridRayIterator<ObjectType>(this, from, dir, len, radius, with_extension);
  }

  __forceinline static bool shouldUseWooRay(vec4i limits)
  {
    const int MIN_SIDE = 4;
    uint64_t from = v_extract_xi64(limits);
    uint64_t to = v_extract_yi64(limits);
    uint64_t size = to - from;
    bool zcheck = unsigned(size >> 32) >= MIN_SIDE;
    bool xcheck = unsigned(size) >= MIN_SIDE;
    return EASTL_UNLIKELY(zcheck && xcheck);
  }

  bool isEmpty() const
  {
    vec4f vGridSize = v_cast_vec4f(v_ldi(&gridSize.lim[0].x));
    return v_test_all_bits_zeros(vGridSize);
  }
  bool isOptimized() const { return optimized; }
  const LinearSubGrid<ObjectType> *getSubGrid(int32_t idx) const { return (idx >= 0) ? subGrids.data()[idx] : nullptr; }
  LinearSubGrid<ObjectType> *getSubGrid(int32_t idx) { return (idx >= 0) ? subGrids.data()[idx] : nullptr; }
  bool isBranch(leaf_id_t leaf) const { return branches[leaf]; }
  void setBranch(leaf_id_t leaf, bool value) { branches[leaf] = value; }
  const LinearGridLeaf<ObjectType> &getLeaf(leaf_id_t leaf) const { return leafsData[leaf]; }
  LinearGridLeaf<ObjectType> &getLeaf(leaf_id_t leaf) { return leafsData[leaf]; }
  leaf_id_t createLeaf(bool is_branch)
  {
    uint32_t idx = leafs.size();
    if (EASTL_UNLIKELY(idx >= eastl::numeric_limits<leaf_id_t>::max()))
    {
      logerr("%s: can't create leaf because %u limit reached. Increase objects limit for leaf.", __FUNCTION__,
        eastl::numeric_limits<leaf_id_t>::max());
      return EMPTY_LEAF;
    }
    leafs.emplace_back();
    leafsData = leafs.data();
    branches.push_back(is_branch);
    return leaf_id_t(idx);
  };
  __forceinline UnpackedBranch unpackBranch(leaf_id_t leaf_idx) const
  {
    LG_VERIFY(isBranch(leaf_idx));
    UnpackedBranch ret;
#if EXTRA_SMALL_BRANCHES_AND_64K_LEAFS_LIMIT
    const LinearGridLeaf<ObjectType> &leaf = getLeaf(leaf_idx);
    ret.leftIdx = leaf.left;
    ret.rightIdx = leaf.right;
#else
    const LinearGridLeaf<ObjectType> &minLeaf = getLeaf(leaf_idx);
    const LinearGridLeaf<ObjectType> &maxLeaf = getLeaf(leaf_idx + 1);
    ret.leftIdx = minLeaf.idx;
    ret.rightIdx = maxLeaf.idx;
#endif
    return ret;
  }
  __forceinline UnpackedBranch unpackBranch(leaf_id_t leaf_idx, bbox3f parent_leaf_box) const
  {
    LG_VERIFY(isBranch(leaf_idx));
    UnpackedBranch ret;
#if EXTRA_SMALL_BRANCHES_AND_64K_LEAFS_LIMIT
    const LinearGridLeaf<ObjectType> &leaf = getLeaf(leaf_idx);
    vec4i packedCorrection = v_ldi((const int *)leaf.packedMinCorrection);
    unpack_bboxes_16b(ret.leftBox, ret.rightBox, parent_leaf_box, packedCorrection);
    ret.leftIdx = leaf.left;
    ret.rightIdx = leaf.right;
#else
    const LinearGridLeaf<ObjectType> &minLeaf = getLeaf(leaf_idx);
    const LinearGridLeaf<ObjectType> &maxLeaf = getLeaf(leaf_idx + 1);
    vec4f packedMinCorrection = v_ld(minLeaf.packedCorrection);
    vec4f packedMaxCorrection = v_ld(maxLeaf.packedCorrection);
    unpack_bboxes_32b(ret.leftBox, ret.rightBox, parent_leaf_box, packedMinCorrection, packedMaxCorrection);
    ret.leftIdx = minLeaf.idx;
    ret.rightIdx = maxLeaf.idx;
#endif
    return ret;
  }
  // leaf indices remain unchanged
  __forceinline void packBranch(leaf_id_t leaf_idx, bbox3f left_box, bbox3f right_box, bbox3f parent_leaf_box)
  {
#if EXTRA_SMALL_BRANCHES_AND_64K_LEAFS_LIMIT
    vec4i newPackedCorrection = pack_bboxes_16b(left_box, right_box, parent_leaf_box);
    vec4i mask = v_make_vec4i(UINT32_MAX, UINT16_MAX, UINT32_MAX, UINT16_MAX);
    LinearGridLeaf<ObjectType> &leaf = getLeaf(leaf_idx);
    v_sti((int *)leaf.packedMinCorrection, v_btseli(v_ldi((const int *)leaf.packedMinCorrection), newPackedCorrection, mask));
#else
    vec4f minCorrection, maxCorrection;
    pack_bboxes_32b(minCorrection, maxCorrection, left_box, right_box, parent_leaf_box);
    LinearGridLeaf<ObjectType> &minLeaf = getLeaf(leaf_idx);
    LinearGridLeaf<ObjectType> &maxLeaf = getLeaf(leaf_idx + 1);
    v_st(minLeaf.packedCorrection, v_perm_xyzd(minCorrection, v_ld(minLeaf.packedCorrection)));
    v_st(maxLeaf.packedCorrection, v_perm_xyzd(maxCorrection, v_ld(maxLeaf.packedCorrection)));
#endif
  }

  DAGOR_NOINLINE void insert(ObjectType object, vec4f wbsph, bbox3f wbb, bool initial)
  {
    vec4i subIds = calcCellIds(wbsph, cellSizeLog2 - LINEAR_GRID_SUBGRID_WIDTH_LOG2);
    vec4i mainIds = v_srai(subIds, LINEAR_GRID_SUBGRID_WIDTH_LOG2);
    bbox3f sphBox;
    v_bbox3_init_by_bsph(sphBox, wbsph, v_splat_w(wbsph));
    insertAt(object, mainIds, subIds, v_bbox3_get_box_intersection(sphBox, wbb), initial);
    LG_VERIFYF(leafs.size() == branches.size(), "New leaf added without branches resize! %i != %i", leafs.size(), branches.size());
  }

  DAGOR_NOINLINE void erase(ObjectType object, vec4f wbsph)
  {
    vec4i subIds = calcCellIds(wbsph, cellSizeLog2 - LINEAR_GRID_SUBGRID_WIDTH_LOG2);
    vec4i mainIds = v_srai(subIds, LINEAR_GRID_SUBGRID_WIDTH_LOG2);
    eraseAt(object, mainIds, subIds, wbsph);
  }

  DAGOR_NOINLINE void update(ObjectType object, vec4f old_wbsph, vec4f new_wbsph, bbox3f wbb)
  {
    vec4i oldSubIds = calcCellIds(old_wbsph, cellSizeLog2 - LINEAR_GRID_SUBGRID_WIDTH_LOG2);
    vec4i newSubIds = calcCellIds(new_wbsph, cellSizeLog2 - LINEAR_GRID_SUBGRID_WIDTH_LOG2);
    vec4i oldIds = v_srai(oldSubIds, LINEAR_GRID_SUBGRID_WIDTH_LOG2);
    vec4i newIds = v_srai(newSubIds, LINEAR_GRID_SUBGRID_WIDTH_LOG2);
    eraseAt(object, oldIds, oldSubIds, old_wbsph);
    insertAt(object, newIds, newSubIds, wbb);
  }

  struct OptimizationStats
  {
    uint32_t totalObjects = 0;
    uint32_t mainObjects = 0;
    uint32_t emptyCells = 0;
    uint32_t emptySubCells = 0;
    uint32_t branchesCount = 0;
  };

  bool isCellOptimized(const LinearGridMainCell<ObjectType> &cell) const
  {
    if (cell.subGridIdx >= 0)
      return true;
    if (cell.rootLeaf != EMPTY_LEAF)
    {
      if (isBranch(cell.rootLeaf))
        return true;
      const LinearGridLeaf<ObjectType> &leaf = getLeaf(cell.rootLeaf);
      return leaf.objects.size() < min(configMaxLeafObjects, configObjectsToCreateSubGrid);
    }
    return true;
  }

  DAGOR_NOINLINE void optimizeCells()
  {
    if (isEmpty())
      return;
    G_ASSERT(leafs.size() == branches.size());
    int64_t ref = ref_time_ticks();
    if (!optimized)
    {
      dag::Vector<LinearGridLeaf<ObjectType>, MidmemAlloc> oldLeafs = eastl::move(leafs); // reindex all for better cache locality
      leafs.clear();
      leafs.reserve(oldLeafs.size());
      leafsData = nullptr;
      branches.clear();
      for (LinearGridMainCell<ObjectType> &cell : cells)
      {
        if (cell.rootLeaf == EMPTY_LEAF)
          continue;
        dag::Vector<ObjectType, MidmemAlloc> objects = eastl::move(oldLeafs.data()[cell.rootLeaf].objects);
        cell.rootLeaf = EMPTY_LEAF; // create new leaf after global clear
        refillCell(cell, eastl::move(objects), optimizationStats);
      }
    }
    else
    {
      for (LinearGridMainCell<ObjectType> &cell : cells)
      {
        if (cell.rootLeaf == EMPTY_LEAF || isCellOptimized(cell))
          continue;
        // reuse cell.rootLeaf index
        dag::Vector<ObjectType, MidmemAlloc> objects = eastl::move(getLeaf(cell.rootLeaf).objects);
        refillCell(cell, eastl::move(objects), optimizationStats);
      }
    }
    int result = get_time_usec(ref);
    debug("Grid optimizing finished in %.2f msec", result / 1000.f);
    if (!optimized)
    {
      printStats();
      optimized = true;
    }
#if VERIFY_ALL
    for (const LinearGridMainCell<ObjectType> &cell : cells)
    {
      if (cell.rootLeaf != EMPTY_LEAF)
        leaf_verify_bboxes(this, cell.rootLeaf, cell.getBBox(), ObjectType::null());
      if (const LinearSubGrid<ObjectType> *subGrid = getSubGrid(cell.subGridIdx))
      {
        for (const LinearGridSubCell<ObjectType> &subCell : subGrid->getCells())
        {
          if (subCell.rootLeaf != EMPTY_LEAF)
            leaf_verify_bboxes(this, subCell.rootLeaf, subCell.getBBox(), ObjectType::null());
        }
      }
    }
#endif
  }

  void printStats() const { printStats(optimizationStats); }

  DAGOR_NOINLINE void printStats(const OptimizationStats &stats) const
  {
    double kb = 1024.0;
    double mb = 1024.0 * 1024.0;
    unsigned allocationCost = 32;
    unsigned totalCells = getGridHeight() * getGridWidth();
    unsigned objectsMem = stats.totalObjects * sizeof(ObjectType) + allocationCost;
    unsigned cellsMem = totalCells * sizeof(LinearGridMainCell<ObjectType>) + allocationCost;
    unsigned leafsMem = leafs.size() * sizeof(LinearGridLeaf<ObjectType>) + allocationCost;
    leafsMem += (leafs.size() - stats.branchesCount) * allocationCost;
    leafsMem += data_size(branches) + allocationCost;
    unsigned subGridsMem = subGrids.size() * (sizeof(LinearSubGrid<ObjectType>) + sizeof(void *)) + allocationCost;
    unsigned oversizeMem = data_size(oversizeObjects) + allocationCost;
    debug("Total objects: %u [%.1f Mb]", stats.totalObjects, objectsMem / mb);
    debug("Cells: %ux%u=%u (%.1fx%.1f km); Empty: %u (%.1f%%) [%.1f Kb]", getGridHeight(), getGridWidth(), totalCells,
      (getGridHeight() * getCellSize()) / 1000.f, (getGridWidth() * getCellSize()) / 1000.f, stats.emptyCells,
      safediv(double(stats.emptyCells), totalCells) * 100.0, cellsMem / kb);
    debug("Leafs: %u; Branches: %u (%.1f%%) [%.1f Kb]", leafs.size(), stats.branchesCount,
      safediv(double(stats.branchesCount), leafs.size()) * 100.0, leafsMem / kb);
    debug("SubGrids: %u; (%.1f%% of cells) empty subCells: %.1f%% [%.1f Kb]", subGrids.size(),
      safediv(double(subGrids.size()), totalCells) * 100.0,
      safediv(double(stats.emptySubCells), subGrids.size() * sqr(LINEAR_GRID_SUBGRID_WIDTH)) * 100.0, subGridsMem / kb);
    debug("Main ext: %.1f; Sub ext: %.1f", maxMainExtension, maxSubExtension);
    debug("Oversize objects: %u", oversizeObjects.size());
    debug("Total memory: %.2f Mb",
      (sizeof(LinearGrid<ObjectType>) + objectsMem + cellsMem + leafsMem + subGridsMem + oversizeMem) / mb);
  }

  void setCellSizeWithoutObjectsReposition(unsigned cellSize)
  {
    G_ASSERT(isEmpty());
    if (is_pow2(cellSize))
      cellSizeLog2 = get_log2i_of_pow2(cellSize);
    else
      logerr("LinearGrid: %s(%u) error, new value should be power of 2", __FUNCTION__, cellSize);
  }

  float getMaxExtension() const { return maxMainExtension; }
  uint32_t getObjectsGrowStep() const { return configReserveObjectsOnGrow; };
  unsigned getCellSize() const { return 1 << cellSizeLog2; }
  unsigned getGridWidth() const { return gridWidth; }
  unsigned getGridHeight() const { return gridSize.width().y; }
  const LinearGridMainCell<ObjectType> &getCellByOffset(unsigned offset) const { return cells[offset]; };
  const LinearGridMainCell<ObjectType> *getCellByPosDebug(const Point3 &pos, IPoint2 &out_coords) const
  {
    vec4i cellIds = calcCellIds(v_ldu(&pos.x), cellSizeLog2);
    if (!isInGrid(cellIds))
      return nullptr;
    v_stui_half(&out_coords.x, cellIds);
    return &getCellByIds(cellIds);
  };

  bool needCreateBranch(uint32_t leaf_objects_count) const { return false && optimized && leaf_objects_count > configMaxLeafObjects; }

  VECTORCALL DAGOR_NOINLINE void createBranchOnLeaf(leaf_id_t leaf_idx, dag::Vector<ObjectType, MidmemAlloc> &&objects,
    bbox3f parent_box)
  {
    G_ASSERT(!isBranch(leaf_idx) && getLeaf(leaf_idx).objects.empty());
    dag::RelocatableFixedVector<eastl::pair<ObjectType, bbox3f>, 64, true, MidmemAlloc> bboxes;
    bboxes.reserve(objects.size());
    bboxes.resize(objects.size());
    unsigned bboxesCount = 0;
    for (ObjectType obj : objects)
    {
      vec4f wbsph = obj.getWBSph();
      // for rotated objects aabb of world sphere can be smaller than tm*obb, but usually bigger
      bbox3f sphBox;
      v_bbox3_init_by_bsph(sphBox, wbsph, v_splat_w(wbsph));
      bbox3f bbox = v_bbox3_get_box_intersection(sphBox, obj.getWBBox());
      bboxes.data()[bboxesCount++] = eastl::make_pair(obj, bbox);
      LG_VERIFY(v_bbox3_test_box_inside(parent_box, bbox));
    }
    OptimizationStats stats;
    reCreateBalancedLeaf(leaf_idx, eastl::move(objects), bboxes, parent_box, stats);
  }

  DAGOR_NOINLINE void clear()
  {
    gridSize = IBBox2(IPoint2(), IPoint2());
    lowestCellBox = v_zero();
    cellsData = nullptr;
    leafsData = nullptr;
    gridWidth = 0;
    maxMainExtension = 0.f;
    maxSubExtension = 0.f;
    for (uint32_t leafIdx = 0; leafIdx < leafs.size(); leafIdx++)
    {
      if (!isBranch(leafIdx))
        eastl::destroy_at(&leafs.data()[leafIdx].objects);
    }
    cells.clear();
    leafs.clear();
    branches.clear();
    for (LinearSubGrid<ObjectType> *subGrid : subGrids)
      delete subGrid;
    subGrids.clear();
    oversizeObjects.clear();
  }

  template <typename T>
  void foreachObjectInCellDebug(const LinearGridMainCell<ObjectType> &cell, const T &cb) const
  {
    struct ObjectsIterator
    {
      const T &predFunc;
    } iterator{cb};
    if (cell.rootLeaf != EMPTY_LEAF)
      leaf_iterate_all(this, cell.rootLeaf, iterator);
    if (const LinearSubGrid<ObjectType> *subGrid = getSubGrid(cell.subGridIdx))
    {
      for (const LinearGridSubCell<ObjectType> &subCell : subGrid->getCells())
      {
        if (subCell.rootLeaf != EMPTY_LEAF)
          leaf_iterate_all(this, subCell.rootLeaf, iterator);
      }
    }
  }

  void countCellObjectsDebug(const LinearGridMainCell<ObjectType> &cell, uint32_t &main, uint32_t &sub) const
  {
    main = cell.rootLeaf != EMPTY_LEAF ? leaf_count_objects(this, cell.rootLeaf) : 0;
    sub = 0;
    if (const LinearSubGrid<ObjectType> *subGrid = getSubGrid(cell.subGridIdx))
    {
      for (const LinearGridSubCell<ObjectType> &subCell : subGrid->getCells())
        sub += subCell.rootLeaf != EMPTY_LEAF ? leaf_count_objects(this, subCell.rootLeaf) : 0;
    }
  }

private:
  VECTORCALL void insertAt(ObjectType object, vec4i v_main_cell_ids, vec4i v_sub_cell_ids, bbox3f wbb, bool initial = false)
  {
    float mainExt = getMaxExtension(v_main_cell_ids, cellSizeLog2, wbb);
    if (EASTL_UNLIKELY(mainExt > configMaxMainExtension))
    {
      oversizeObjects.emplace_back(object);
      return;
    }
    if (EASTL_UNLIKELY(!isInGrid(v_main_cell_ids)))
      grow(v_main_cell_ids);
    LinearGridMainCell<ObjectType> &cell = getCellByIds(v_main_cell_ids);
    bbox3f oldMainBox = cell.getBBox();
    cell.setBBox(v_bbox3_sum(oldMainBox, wbb));
    maxMainExtension = max(maxMainExtension, mainExt);
    if (LinearSubGrid<ObjectType> *subGrid = getSubGrid(cell.subGridIdx))
    {
      float subExt = getMaxExtension(v_sub_cell_ids, cellSizeLog2 - LINEAR_GRID_SUBGRID_WIDTH_LOG2, wbb);
      if (subExt < configMaxSubExtension)
      {
        maxSubExtension = max(maxSubExtension, subExt);
        if (cell.rootLeaf != EMPTY_LEAF)
        {
          // It's very important and dangerous. Why:
          // 1) When cell box extended, 3 or 6 dimensions of chield boxes also extends
          // 2) packStep insreases with cell box size, offset from parent box can also increase, which makes child boxes smaller (!)
          leaf_repack_bboxes(this, cell.rootLeaf, oldMainBox, cell.getBBox());
        }
        subGrid->insertAt(this, object, v_sub_cell_ids, wbb);
        return;
      }
    }
    if (cell.rootLeaf == EMPTY_LEAF)
    {
      cell.rootLeaf = createLeaf(false /*is_branch*/);
      if (EASTL_UNLIKELY(cell.rootLeaf == EMPTY_LEAF))
        return;
    }
    if (EASTL_UNLIKELY(initial && (!optimized || !isCellOptimized(cell))))
    {
      LinearGridLeaf<ObjectType> &leaf = getLeaf(cell.rootLeaf);
      leaf.objects.emplace_back(object);
    }
    else
      leaf_insert_object(this, cell.rootLeaf, oldMainBox, wbb, object);
  }

  VECTORCALL void eraseAt(ObjectType object, vec4i v_main_cell_ids, vec4i v_sub_cell_ids, vec4f wbsph)
  {
    if (EASTL_UNLIKELY(v_extract_w(wbsph) > configHalfMaxMainExtension))
    {
      auto it = eastl::find(oversizeObjects.begin(), oversizeObjects.end(), object);
      if (EASTL_UNLIKELY(it != oversizeObjects.end()))
      {
        oversizeObjects.erase(it);
        return;
      }
    }
    G_ASSERTF_RETURN(isInGrid(v_main_cell_ids), , "CellIds %i %i GridSize %i %i %i %i", v_extract_xi(v_main_cell_ids),
      v_extract_yi(v_main_cell_ids), gridSize.lim[0].x, gridSize.lim[0].y, gridSize.lim[1].x, gridSize.lim[1].y);
    LinearGridMainCell<ObjectType> &cell = getCellByIds(v_main_cell_ids);
    if (cell.rootLeaf != EMPTY_LEAF && leaf_erase_object(this, cell.rootLeaf, object))
      return;
    LinearSubGrid<ObjectType> *subGrid = getSubGrid(cell.subGridIdx);
    if (subGrid && subGrid->eraseAt(this, object, v_sub_cell_ids))
      return;
    G_ASSERTF(false, "Failed to remove object from grid. Erased twice or wrong old position.");
  }

  LinearGridMainCell<ObjectType> &getCellByIds(vec4i v_cell_ids)
  {
    uint64_t offsetXZ = v_extract_xi64(v_subi(v_cell_ids, v_ldui_half(&gridSize.lim[0].x)));
    return cellsData[unsigned(offsetXZ >> 32) * getGridWidth() + unsigned(offsetXZ)];
  }
  const LinearGridMainCell<ObjectType> &getCellByIds(vec4i v_cell_ids) const
  {
    uint64_t offsetXZ = v_extract_xi64(v_subi(v_cell_ids, v_ldui_half(&gridSize.lim[0].x)));
    return cellsData[unsigned(offsetXZ >> 32) * getGridWidth() + unsigned(offsetXZ)];
  }

  __forceinline static float getMaxExtension(vec4i v_cell_ids, unsigned cell_size_log2, bbox3f object_bbox)
  {
    vec4i bboxCells = v_permi_xyab(v_cell_ids, v_subi(v_cell_ids, v_set_all_bitsi()));
    vec4f bboxOffsets = v_cvt_vec4f(v_slli_n(bboxCells, cell_size_log2));
    vec4f boxesDiff = v_sub(bboxOffsets, v_perm_xzac(object_bbox.bmin, object_bbox.bmax));
    vec4f maxExt = v_hmax(v_perm_xycd(boxesDiff, v_sub(v_zero(), boxesDiff)));
    return v_extract_x(maxExt);
  }

  static vec4i calcCellIds(vec3f pos, unsigned cell_size_log2) // return [xzxz]
  {
    // v_cvt_floori returns 0x80000000 (negative number) on integer overflow. Just know it.
    vec4i vCellIds = v_srai_n(v_cvt_floori(pos), cell_size_log2);
    return v_permi_xzxz(vCellIds);
  }

  __forceinline vec4i getClampedOffsets(bbox3f bbox, bool is_subgrid) const
  {
    vec4i vCellIds = v_srai_n(v_cvt_floori(v_perm_xzac(bbox.bmin, bbox.bmax)), cellSizeLog2 - LINEAR_GRID_SUBGRID_WIDTH_LOG2);
    vec4i vGridSize = v_ldi(&gridSize.lim[0].x);
    if (is_subgrid)
      vGridSize = v_muli(vGridSize, v_splatsi(LINEAR_GRID_SUBGRID_WIDTH));
    else
      vCellIds = v_srai(vCellIds, LINEAR_GRID_SUBGRID_WIDTH_LOG2);
    vec4i vGridMin = v_permi_xyxy(vGridSize);
    vec4i vGridMax = v_addi(v_permi_zwzw(vGridSize), v_set_all_bitsi());
    vec4i vClampedCellIds = v_clampi(vCellIds, vGridMin, vGridMax);
    return v_subi(vClampedCellIds, vGridMin);
  }

  bool isInGrid(vec4i cell_ids) const
  {
    vec4i vGridSize = v_ldi(&gridSize.lim[0].x);
    vec4i cmp = v_cmp_lti(cell_ids, vGridSize);
    int mask = v_signmask(v_cast_vec4f(cmp));
    return mask == 0b1100;
  }

  vec4f getLowestCellBox() const { return lowestCellBox; }

  unsigned getBranchesCount() const
  {
    unsigned count = 0;
    for (auto u : branches.get_container())
    {
      count += __popcount(uint32_t(u));
      if (sizeof(u) == sizeof(uint64_t))
        count += __popcount(uint32_t(u >> 32));
    }
#if !EXTRA_SMALL_BRANCHES_AND_64K_LEAFS_LIMIT
    count /= 2;
#endif
    return count;
  }

  VECTORCALL DAGOR_NOINLINE void grow(vec4i v_cell_ids)
  {
    vec4i vCurGridSize = v_ldi(&gridSize.lim[0].x);
    vec4i vNewGridSize = v_permi_xycd(v_mini(v_cell_ids, vCurGridSize), v_maxi(v_addi(v_cell_ids, V_CI_1), vCurGridSize));
    IBBox2 curGridSize, newGridSize;
    v_stui(&curGridSize.lim[0].x, vCurGridSize);
    v_stui(&newGridSize.lim[0].x, vNewGridSize);
    unsigned newGridWidth = newGridSize.width().x;
    unsigned newGridHeight = newGridSize.width().y;
    unsigned newSquare = newGridWidth * newGridHeight;
    cells.resize(newSquare);
    unsigned curGridWidth = getGridWidth();
    unsigned curGridHeight = getGridHeight();

    for (int oldRowIdx = curGridHeight - 1; oldRowIdx >= 0; oldRowIdx--)
    {
      unsigned oldPos = oldRowIdx * curGridWidth;
      unsigned newRow = (curGridSize.getMin().y + oldRowIdx - newGridSize.getMin().y) * newGridWidth;
      unsigned newPos = newRow + (curGridSize.getMin().x - newGridSize.getMin().x);
      if (newPos != oldPos && curGridWidth)
      {
        memmove(cells.data() + newPos, cells.data() + oldPos, curGridWidth * sizeof(LinearGridMainCell<ObjectType>));
        unsigned releasedCells = min(curGridWidth, newPos - oldPos);
        for (unsigned i = 0; i < releasedCells; i++)
          new (cells.data() + oldPos + i) LinearGridMainCell<ObjectType>;
      }
    }

    v_sti(&gridSize.lim[0].x, vNewGridSize);
    vec4i lowestCell = v_permi_xyab(vNewGridSize, v_addi(vNewGridSize, V_CI_1));
    lowestCellBox = v_cvti_vec4f(v_slli_n(lowestCell, cellSizeLog2));
    cellsData = cells.data();
    gridWidth = newGridWidth;
  }

  DAGOR_NOINLINE void refillCell(LinearGridMainCell<ObjectType> &cell, dag::Vector<ObjectType, MidmemAlloc> &&objects,
    OptimizationStats &stats)
  {
    stats.totalObjects += objects.size();
    eastl::sort(objects.begin(), objects.end());

    dag::Vector<eastl::pair<ObjectType, bbox3f>, MidmemAlloc> bboxes;
    unsigned bboxesCount = 0;
    unsigned smallObjectsCount = 0;

    if (objects.size() > min(configObjectsToCreateSubGrid, configMaxLeafObjects))
    {
      bboxes.reserve(objects.size());
      bboxes.resize(objects.size());

      smallObjectsCount = eastl::count_if(objects.begin(), objects.end(), [&](ObjectType obj) {
        vec4f wbsph = obj.getWBSph();
        // for rotated objects aabb of world sphere can be smaller than tm*obb, but usually bigger
        bbox3f sphBox;
        v_bbox3_init_by_bsph(sphBox, wbsph, v_splat_w(wbsph));
        bbox3f bbox = v_bbox3_get_box_intersection(sphBox, obj.getWBBox());
        bboxes.data()[bboxesCount++] = eastl::make_pair(obj, bbox);

        float objRad = v_extract_w(wbsph);
        return objRad <= configMaxSubExtension;
      });
    }

    dag::Vector<ObjectType, MidmemAlloc> subgridObjects[LINEAR_GRID_SUBGRID_WIDTH * LINEAR_GRID_SUBGRID_WIDTH];
    LinearSubGrid<ObjectType> *subGrid = nullptr;

    // Create subgrid
    if (smallObjectsCount >= configObjectsToCreateSubGrid)
    {
      cell.subGridIdx = subGrids.size();
      subGrid = subGrids.emplace_back(new LinearSubGrid<ObjectType>);
      uint32_t cellOffset = uint32_t(&cell - cells.data());
      int z = (gridSize.getMin().y + cellOffset / getGridWidth()) * getCellSize();
      int x = (gridSize.getMin().x + cellOffset % getGridWidth()) * getCellSize();
      int subCellWidth = getCellSize() >> LINEAR_GRID_SUBGRID_WIDTH_LOG2;
      vec4i ibox = v_make_vec4i(x, z, x + subCellWidth, z + subCellWidth);
      subGrid->setLowestCellBox(v_cvti_vec4f(ibox));
      // Move small objects to sub grid
      for (unsigned objId = 0; objId < objects.size(); objId++)
      {
        ObjectType obj = objects.data()[objId];
        vec4f wbsph = obj.getWBSph(); // don't use pre-filter by radius, it's unprecise
        if (v_extract_w(wbsph) > configMaxSubRadius)
          continue;
        unsigned subGridCellSizeLog2 = cellSizeLog2 - LINEAR_GRID_SUBGRID_WIDTH_LOG2;
        vec4i subCellIds = calcCellIds(wbsph, subGridCellSizeLog2);
        bbox3f bbox = bboxes.data()[objId].second;
        float ext = getMaxExtension(subCellIds, subGridCellSizeLog2, bbox);
        if (ext > configMaxSubExtension)
          continue;
        maxSubExtension = max(maxSubExtension, ext);
        unsigned offset = subGrid->getCellOffset(subCellIds);
        subGrid->getCellByOffset(offset).addBBox(bbox);
        subgridObjects[offset].emplace_back(obj);
        objects.data()[objId] = ObjectType::null();
      }
      objects.erase(eastl::remove(objects.begin(), objects.end(), ObjectType::null()), objects.end());
    }

    if (!objects.empty())
    {
      stats.mainObjects += objects.size();
      cell.rootLeaf = reCreateBalancedLeaf(cell.rootLeaf, eastl::move(objects), bboxes, cell.getBBox(), stats);
    }
    else
    {
      stats.emptyCells++;
      cell.rootLeaf = EMPTY_LEAF;
    }

    // Create trees in subgrid
    if (subGrid)
    {
      for (unsigned offset = 0; offset < LINEAR_GRID_SUBGRID_WIDTH * LINEAR_GRID_SUBGRID_WIDTH; offset++)
      {
        if (subgridObjects[offset].empty())
        {
          stats.emptySubCells++;
          continue;
        }
        LinearGridSubCell<ObjectType> &subCell = subGrid->getCellByOffset(offset);
        subCell.rootLeaf =
          reCreateBalancedLeaf(EMPTY_LEAF, eastl::move(subgridObjects[offset]), make_span_const(bboxes), subCell.getBBox(), stats);
        subCell.empty = 0;
      }
    }
  }

  VECTORCALL DAGOR_NOINLINE leaf_id_t reCreateBalancedLeaf(leaf_id_t leaf_idx, dag::Vector<ObjectType, MidmemAlloc> &&objects,
    dag::ConstSpan<eastl::pair<ObjectType, bbox3f>> bboxes, bbox3f parent_box, OptimizationStats &stats)
  {
    G_ASSERT(leaf_idx == EMPTY_LEAF || !isBranch(leaf_idx));
    if (EASTL_UNLIKELY(objects.empty()))
      return leaf_idx;

    if (objects.size() <= configMaxLeafObjects)
    {
      if (leaf_idx == EMPTY_LEAF)
        leaf_idx = createLeaf(false);
      else
        G_ASSERT(getLeaf(leaf_idx).objects.empty());

      objects.shrink_to_fit();
      getLeaf(leaf_idx).objects = eastl::move(objects);
      return leaf_idx;
    }

    vec4f bboxSize = v_bbox3_size(parent_box);
    vec4f sideOversize = v_div(v_add(v_perm_yzxw(bboxSize), v_perm_zxyw(bboxSize)), bboxSize);
    vec4f sideOversizeCoef = v_min(V_C_ONE, sideOversize);

    // Find most balanced half cut
    dag::RelocatableFixedVector<uint8_t, 1024, true /*overflow*/, MidmemAlloc, uint32_t, false /*init*/> belowMasks;
    belowMasks.reserve(objects.size());
    belowMasks.resize(objects.size());
    vec4f center = v_bbox3_center(parent_box);
    vec4i belowCounters = v_zeroi();

    auto bboxIt = bboxes.begin();
    for (unsigned i = 0; EASTL_LIKELY(i < objects.size()); i++)
    {
      bboxIt = eastl::find_if(bboxIt, bboxes.end(),
        [obj = objects.data()[i]](const eastl::pair<ObjectType, bbox3f> &pair) { return pair.first == obj; });
      LG_VERIFY(bboxIt != bboxes.end());
      bbox3f bbox = bboxIt->second;
      vec4f belowOverflow = v_sub(bbox.bmax, center);
      vec4f aboveOverflow = v_sub(center, bbox.bmin);
      vec4f belowIsLess = v_cmp_lt(belowOverflow, aboveOverflow);
      belowCounters = v_subi(belowCounters, v_cast_vec4i(belowIsLess));
      belowMasks.data()[i] = v_signmask(belowIsLess);
    }
    vec4i halfObjects = v_splatsi(objects.size() / 2);
    vec4i absDiff = v_absi(v_subi(belowCounters, halfObjects));
    vec4f score = v_mul(v_cvt_vec4f(absDiff), sideOversizeCoef);
    vec3f isOneSideEmpty =
      v_cast_vec4f(v_ori(v_cmp_eqi(belowCounters, v_zeroi()), v_cmp_eqi(belowCounters, v_splatsi(objects.size()))));
    vec4f selectBest = v_andnot(isOneSideEmpty, v_cmp_eq(score, v_hmin3(score)));
    int bestMask = v_signmask(selectBest) & 0b111;
    if (EASTL_UNLIKELY(!bestMask)) // need alt algo
    {
      if (leaf_idx == EMPTY_LEAF)
        leaf_idx = createLeaf(false);
      objects.shrink_to_fit();
      getLeaf(leaf_idx).objects = eastl::move(objects);
      return leaf_idx;
    }
    int minIdx = __bsf_unsafe(bestMask);

    // Split to 2 bboxes and vectors
    int iBelowCounters[4];
    v_stui(iBelowCounters, belowCounters);
    unsigned belowCount = iBelowCounters[minIdx];
    unsigned aboveCount = objects.size() - belowCount;
#if VERIFY_ALL
    G_ASSERT_RETURN(belowCount && aboveCount, false);
#endif
    dag::Vector<ObjectType, MidmemAlloc> belowObjects = eastl::move(objects);
    dag::Vector<ObjectType, MidmemAlloc> aboveObjects;
    aboveObjects.reserve(aboveCount);
    aboveObjects.resize(aboveCount);
    bbox3f belowBox, aboveBox;
    v_bbox3_init_empty(belowBox);
    v_bbox3_init_empty(aboveBox);
    auto bboxIter = bboxes.begin();
    RiGridObject *writeBelowPos = belowObjects.data();
    RiGridObject *writeAbovePos = aboveObjects.data();
    const uint8_t *objBelowMask = belowMasks.data();
    for (const RiGridObject &obj : belowObjects)
    {
      bboxIter =
        eastl::find_if(bboxIter, bboxes.end(), [obj](const eastl::pair<ObjectType, bbox3f> &pair) { return pair.first == obj; });
      LG_VERIFY(bboxIter != bboxes.end());
      bbox3f bbox = bboxIter->second;
      bool isBelow = *objBelowMask++ & (1 << minIdx);
      if (isBelow)
      {
        *writeBelowPos++ = obj;
        v_bbox3_add_box(belowBox, bbox);
      }
      else
      {
        *writeAbovePos++ = obj;
        v_bbox3_add_box(aboveBox, bbox);
      }
    }
    LG_VERIFY(writeBelowPos = belowObjects.data() + belowCount);
    LG_VERIFY(writeAbovePos = aboveObjects.data() + aboveCount);
    belowObjects.resize(belowCount); // shrink to real size

    // Create childs
    leaf_id_t reuseIdx = EMPTY_LEAF;
#if EXTRA_SMALL_BRANCHES_AND_64K_LEAFS_LIMIT
    if (leaf_idx == EMPTY_LEAF)
      leaf_idx = createLeaf(true);
    else
      setBranch(leaf_idx, true);

    vec4i packedData = pack_bboxes_16b(belowBox, aboveBox, parent_box);
    unpack_bboxes_16b(belowBox, aboveBox, parent_box, packedData);
#else
    if (leaf_idx != EMPTY_LEAF)
    {
      eastl::swap(leaf_idx, reuseIdx);
      LG_VERIFY(!isBranch(reuseIdx));
    }
    leaf_idx = createLeaf(true); // min
    createLeaf(true);            // max
#endif

    leaf_id_t leftIdx = reCreateBalancedLeaf(reuseIdx, eastl::move(belowObjects), bboxes, belowBox, stats);
    leaf_id_t rightIdx = reCreateBalancedLeaf(EMPTY_LEAF, eastl::move(aboveObjects), bboxes, aboveBox, stats);
    LG_VERIFY(leftIdx != EMPTY_LEAF && rightIdx != EMPTY_LEAF);
    stats.branchesCount++;

    // Pack data
#if EXTRA_SMALL_BRANCHES_AND_64K_LEAFS_LIMIT
    LinearGridLeaf<ObjectType> &leaf = getLeaf(leaf_idx);
    ;
    v_sti((int *)leaf.packedMinCorrection, packedData);
    memcpy(&leaf.left, &leftIdx, sizeof(leaf_id_t));
    memcpy(&leaf.right, &rightIdx, sizeof(leaf_id_t));
#else
    vec4f minCorrection, maxCorrection;
    pack_bboxes_32b(minCorrection, maxCorrection, belowBox, aboveBox, parent_box);
    LinearGridLeaf<ObjectType> &minLeaf = getLeaf(leaf_idx);
    LinearGridLeaf<ObjectType> &maxLeaf = getLeaf(leaf_idx + 1);
    v_st(minLeaf.packedCorrection, v_perm_xyzd(minCorrection, v_cast_vec4f(v_splatsi(leftIdx))));
    v_st(maxLeaf.packedCorrection, v_perm_xyzd(maxCorrection, v_cast_vec4f(v_splatsi(rightIdx))));
#endif

    UnpackedBranch branch = unpackBranch(leaf_idx, parent_box);
    G_UNUSED(branch);
    LG_VERIFYF(v_bbox3_test_box_inside(branch.leftBox, belowBox), "Unpacked=" FMT_B3 " Orig=" FMT_B3, VB3D(branch.leftBox),
      VB3D(belowBox));
    LG_VERIFYF(v_bbox3_test_box_inside(branch.rightBox, aboveBox), "Unpacked=" FMT_B3 " Orig=" FMT_B3, VB3D(branch.rightBox),
      VB3D(aboveBox));
    v_bbox3_extend(parent_box, v_splats(0.0001f));
    LG_VERIFY(v_bbox3_test_box_inside(parent_box, branch.leftBox));
    LG_VERIFY(v_bbox3_test_box_inside(parent_box, branch.rightBox));
    return leaf_idx;
  }

  IBBox2 gridSize;
  LinearGridMainCell<ObjectType> *__restrict cellsData;
  LinearGridLeaf<ObjectType> *__restrict leafsData;
  unsigned cellSizeLog2;
  unsigned gridWidth; // cached
  float maxMainExtension;
  float maxSubExtension;
  eastl::bitvector<MidmemAlloc> branches; // 32 bytes
  vec4f lowestCellBox;
  dag::Vector<ObjectType, MidmemAlloc> oversizeObjects;
  dag::Vector<LinearGridMainCell<ObjectType>, MidmemAlloc> cells;
  dag::Vector<LinearGridLeaf<ObjectType>, MidmemAlloc> leafs;
  dag::Vector<LinearSubGrid<ObjectType> *, MidmemAlloc> subGrids;
  bool optimized;
  OptimizationStats optimizationStats;

public:
  float configMaxMainExtension = 40.0f;
  float configHalfMaxMainExtension = 20.f;
  float configMaxSubExtension = 3.f;
  float configMaxSubRadius = 4.f;
  unsigned configObjectsToCreateSubGrid = 200;
  unsigned configMaxLeafObjects = 40;
  unsigned configReserveObjectsOnGrow = 4;
};
