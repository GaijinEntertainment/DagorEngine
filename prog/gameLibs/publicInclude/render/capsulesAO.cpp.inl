#pragma once

#include <render/capsulesAO.h>
#include <math/dag_TMatrix.h>
#include <math/dag_geomTree.h>
#include <math/dag_frustum.h>
#include <EASTL/functional.h>
#include <generic/dag_span.h>
#include <3d/dag_resPtr.h>
#include <math/dag_bits.h>
#include <EASTL/sort.h>
#include <EASTL/vector.h>
#include <memory/dag_framemem.h>
#include <math/dag_hlsl_floatx.h>
#include <3d/dag_drv3d.h>
#include <shaders/dag_shaders.h>

struct CapsulesAOHolder
{
  static constexpr int MAX_AO_UNITS = 32;

  CapsulesAOHolder(uint32_t max_ao_units_count = MAX_AO_UNITS)
  {
    capsuled_units_ao_countVarId = get_shader_variable_id("capsuled_units_ao_count", true);
    capsuled_units_ao_world_to_grid_mul_addVarId = get_shader_variable_id("capsuled_units_ao_world_to_grid_mul_add", true);
    if (capsuled_units_ao_countVarId < 0 && capsuled_units_ao_world_to_grid_mul_addVarId < 0)
      return;
    max_ao_units_count = clamp(max_ao_units_count, uint32_t(1), uint32_t(MAX_AO_UNITS));
    capsuled_units_indirection = dag::create_sbuffer(sizeof(uint), UNITS_AO_GRID_SIZE * UNITS_AO_GRID_SIZE,
      SBCF_DYNAMIC | SBCF_MAYBELOST | SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED, 0,
      "capsuled_units_indirection");
    capsuled_units_ao = dag::create_sbuffer(sizeof(CapsuledAOUnit), maxAOUnitsCount = max_ao_units_count,
      SBCF_DYNAMIC | SBCF_MAYBELOST | SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED, 0, "capsuled_units_ao");
  }

  ~CapsulesAOHolder()
  {
    ShaderGlobal::set_color4(capsuled_units_ao_world_to_grid_mul_addVarId, 0, 0, -1, -1);
    ShaderGlobal::set_int(capsuled_units_ao_countVarId, 0);
  }

  UniqueBufHolder capsuled_units_ao, capsuled_units_indirection;
  uint32_t maxAOUnitsCount = 0;
  int capsuled_units_ao_countVarId = -1;
  int capsuled_units_ao_world_to_grid_mul_addVarId = -1;
};


template <class N>
struct GetCapsuleCB
{
  GetCapsuleCB(const Point3 &in_stg_cam_pos, const Frustum &in_frustum, CapsulesAOHolder &in_capsules_ao,
    eastl::vector<CapsuledAOUnit, framemem_allocator> &in_units_ao) :
    stg_cam_pos(in_stg_cam_pos), frustum(in_frustum), capsules_ao(in_capsules_ao), unitsAo(in_units_ao)
  {}

  __forceinline void operator()(dag::ConstSpan<CapsuleData> capsule_datas, const N &get_node_wtm, const TMatrix &transform) const
  {
    // we check if it is close enough, for performance reasons
    if (capsule_datas.empty())
      return;
    if (lengthSq(stg_cam_pos - transform.getcol(3)) > MAX_AO_CAPSULES_DISTANCE * MAX_AO_CAPSULES_DISTANCE)
      return;

    alignas(16) CapsuledAOUnit unit;
    int capsule = 0, end = min<int>(capsule_datas.size(), MAX_AO_CAPSULES);
    bbox3f box;
    v_bbox3_init_empty(box);
    vec4f aPts[MAX_AO_CAPSULES], bPts[MAX_AO_CAPSULES];
    for (; capsule < end; ++capsule)
    {
      const CapsuleData &data = capsule_datas[capsule];

      mat44f wtm = get_node_wtm(data.nodeIndex);
      vec4f a = v_ldu(&data.a.x), b = v_ldu(&data.b.x);
      vec3f rad = v_mul(v_splat_w(a), v_length3(wtm.col0)); // replace with squared length?
      a = v_mat44_mul_vec3p(wtm, a);
      b = v_mat44_mul_vec3p(wtm, b);
      a = v_perm_xyzd(a, rad);

      v_bbox3_add_pt(box, a);
      v_bbox3_add_pt(box, b);
      aPts[capsule] = a;
      bPts[capsule] = b;
      // pack to one uint4 (we HAVE pack with fixed point, but for now use floats)
      // vec4i ptA = v_float_to_half(a);
      // vec4i ptB = v_float_to_half(b);
      // pack to fixed point around unit
      // vec4i ptA = v_float_to_half(a);
      // vec4i ptB = v_float_to_half(b);
      // vec4i ptA_ptB = v_ori(ptA, v_slli(ptB, 16));
      // v_sti(&unit.capsules[capsule].ptARadius_ptB.x, ptA_ptB);
    }
    vec4f boxSize = v_sub(box.bmax, box.bmin);
    // expand box, currently 2 times + 2 meters.
    if (!frustum.testBoxExtent(v_add(box.bmin, box.bmax), v_add(v_add(boxSize, boxSize), v_splats(AO_AROUND_UNIT))))
      return;
    memset(&unit.capsules[0], 0, sizeof(unit.capsules));

    constexpr int packBitShift = AO_CAPSULES_PACK_BITS;
    constexpr int maxPackValue = (1 << packBitShift) - 1;
    vec4f mulPack = v_div(v_splats(maxPackValue), boxSize);
    vec4i maxVal = v_splatsi(maxPackValue);
    for (capsule = 0; capsule < end; capsule++)
    {
      // vec4i ptA = v_mini(maxVal, v_cvt_roundi(v_max(v_zero(), v_mul(v_sub(aPts[capsule], box.bmin), mulPack))));
      // vec4i ptB = v_mini(maxVal, v_cvt_roundi(v_max(v_zero(), v_mul(v_sub(bPts[capsule], box.bmin), mulPack))));
      // vec4i ptA_ptB = v_ori(ptA, v_slli(ptB, packBitShift));
      // v_sti(&unit.capsules[capsule/AO_CAPSULES_PACK_RATIO].ptARadius_ptB.x, ptA_ptB);
      vec4i ptA = v_mini(maxVal, v_cvt_roundi(v_max(v_zero(), v_mul(v_sub(aPts[capsule + 0], box.bmin), mulPack))));
      vec4i ptB = v_mini(maxVal, v_cvt_roundi(v_max(v_zero(), v_mul(v_sub(bPts[capsule + 0], box.bmin), mulPack))));

#if AO_CAPSULES_PACK_BITS == 8
      ptA = v_ori(ptA, v_cast_vec4i(v_rot_2(v_cast_vec4f(v_slli(ptA, packBitShift)))));
      ptB = v_ori(v_slli(ptB, packBitShift * 2), v_cast_vec4i(v_rot_2(v_cast_vec4f(v_slli(ptB, packBitShift * 3)))));
      vec4i ptA_ptB = v_ori(ptA, ptB);
      v_stui_half(&unit.capsules[capsule].ptARadius_ptB.x, ptA_ptB);
#else
      vec4i ptA_ptB = v_ori(ptA, v_slli(ptB, packBitShift));
      v_sti(&unit.capsules[capsule / AO_CAPSULES_PACK_RATIO].ptARadius_ptB.x, ptA_ptB);
#endif
    }
    // doesn't really make sense to check for occlusion - we only allow closest distances anyway
    v_stu(&unit.boxCenter.x, v_bbox3_center(box));
    v_stu(&unit.boxSize.x, boxSize);
    unitsAo.push_back(unit);
  }

  const Point3 &stg_cam_pos;
  const Frustum &frustum;
  CapsulesAOHolder &capsules_ao;
  eastl::vector<CapsuledAOUnit, framemem_allocator> &unitsAo;
};

template <class N, class T>
__forceinline void set_capsules_ao(const Point3 &stg_cam_pos, const Frustum &frustum, CapsulesAOHolder &capsules_ao,
  const T &get_capsules, int capsules_ao__max_units_per_cell = 32)
{
  if (capsules_ao.capsuled_units_ao_countVarId < 0 && capsules_ao.capsuled_units_ao_world_to_grid_mul_addVarId < 0)
    return;
  // this should not be called at all in game. It is just for menu!
  // gather capsules
  eastl::vector<CapsuledAOUnit, framemem_allocator> unitsAo;
  get_capsules(GetCapsuleCB<N>(stg_cam_pos, frustum, capsules_ao, unitsAo));
  if (unitsAo.size() > capsules_ao.maxAOUnitsCount)
  {
    vec4f camPos = v_ldu(&stg_cam_pos.x);
    auto sortPredicate = [&camPos](const CapsuledAOUnit &a, const CapsuledAOUnit &b) {
      // it is more correct to use distance to box, not distance to box center.
      // but shouldn't matter mach and it is faster
      vec4f lenA = v_length3_sq_x(v_sub(camPos, v_ldu(&a.boxCenter.x)));
      vec4f lenB = v_length3_sq_x(v_sub(camPos, v_ldu(&b.boxCenter.x)));
      return v_test_vec_x_lt(lenA, lenB);
    };
    eastl::nth_element(unitsAo.begin(), unitsAo.begin() + capsules_ao.maxAOUnitsCount, unitsAo.end(), sortPredicate);
    unitsAo.resize(capsules_ao.maxAOUnitsCount);
    eastl::sort(unitsAo.begin(), unitsAo.end(), sortPredicate);
  }
  ShaderGlobal::set_color4(capsules_ao.capsuled_units_ao_world_to_grid_mul_addVarId, 0, 0, -1, -1);
  ShaderGlobal::set_int(capsules_ao.capsuled_units_ao_countVarId, 0);
  if (!unitsAo.size())
    return;
  bbox3f unitsBox;
  v_bbox3_init_empty(unitsBox);
  for (auto &unit : unitsAo)
  {
    vec4f boxCenter = v_perm_xzxz(v_ldu(&unit.boxCenter.x));
    vec4f boxSize = v_perm_xzxz(v_ldu(&unit.boxSize.x));
    // can be just one min, if we use neg for zw
    v_bbox3_add_pt(unitsBox, v_sub(boxCenter, v_add(boxSize, v_splats(AO_AROUND_UNIT))));
    v_bbox3_add_pt(unitsBox, v_add(boxCenter, v_add(boxSize, v_splats(AO_AROUND_UNIT))));
  }
  eastl::vector<uint32_t, framemem_allocator> indirection(UNITS_AO_GRID_SIZE * UNITS_AO_GRID_SIZE);
  vec4f cellSize = v_div(v_bbox3_size(unitsBox), v_splats(UNITS_AO_GRID_SIZE));
  cellSize = v_max(cellSize, v_splats(AO_AROUND_UNIT));
  vec4f worldToGridMul = v_rcp(cellSize);
  vec4f worldToGridAdd = v_mul(v_neg(unitsBox.bmin), worldToGridMul);
  vec4f worldToGridMulAdd = v_perm_xyab(worldToGridMul, worldToGridAdd);

  uint32_t unitMask = 1;
  G_ASSERT(unitsAo.size() <= sizeof(unitMask) * 8);
  for (auto &unit : unitsAo)
  {
    vec4f boxCenter = v_perm_xzxz(v_ldu(&unit.boxCenter.x));
    vec4f boxSize = v_perm_xzxz(v_ldu(&unit.boxSize.x));
    vec3f minBox = v_madd(v_sub(boxCenter, v_add(boxSize, v_splats(AO_AROUND_UNIT))), worldToGridMulAdd, v_rot_2(worldToGridMulAdd));
    vec3f maxBox = v_madd(v_add(boxCenter, v_add(boxSize, v_splats(AO_AROUND_UNIT))), worldToGridMulAdd, v_rot_2(worldToGridMulAdd));
    alignas(16) int gridBoxS[4];
    vec4i gridBox = v_cvt_vec4i(v_min(v_splats(UNITS_AO_GRID_SIZE - 1), v_max(v_zero(), v_perm_xyab(minBox, maxBox))));
    v_sti(gridBoxS, gridBox);
    for (int z = gridBoxS[1]; z <= gridBoxS[3]; ++z)
      for (int x = gridBoxS[0]; x <= gridBoxS[2]; ++x)
      {
        const int cellId = z * UNITS_AO_GRID_SIZE + x;
        auto &cell = indirection[cellId];
        cell |= unitMask;
        if (__popcount(cell) < capsules_ao__max_units_per_cell)
          continue;
        uint32_t cellMask = cell;
        int worstUnit = 0;
        vec4f cellCenter = v_div(v_sub(v_make_vec4f(x + 0.5f, z + 0.5f, 0, 0), v_rot_2(worldToGridMulAdd)), worldToGridMulAdd);
        vec4f distCell = v_zero();
        while (cellMask != 0)
        {
          const int unitNo = __bsf(cellMask);
          cellMask ^= (1 << unitNo);
          vec4f cDist = v_length2_sq_x(v_sub(cellCenter, v_perm_xzxz(v_ldu(&unitsAo[unitNo].boxCenter.x))));
          if (v_test_vec_x_gt(cDist, distCell))
          {
            distCell = cDist;
            worstUnit = unitNo;
          }
        }
        cell &= ~(1 << worstUnit);
      }
    unitMask <<= 1;
  }
  if (!capsules_ao.capsuled_units_indirection.getBuf()->updateDataWithLock(0, sizeof(indirection[0]) * indirection.size(),
        indirection.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD))
    return;
  if (!capsules_ao.capsuled_units_ao.getBuf()->updateDataWithLock(0, unitsAo.size() * sizeof(CapsuledAOUnit), unitsAo.data(),
        VBLOCK_WRITEONLY | VBLOCK_DISCARD))
    return;
  ShaderGlobal::set_int(capsules_ao.capsuled_units_ao_countVarId, unitsAo.size());
#define V4D(a) v_extract_x(a), v_extract_y(a), v_extract_z(a), v_extract_w(a)
  ShaderGlobal::set_color4(capsules_ao.capsuled_units_ao_world_to_grid_mul_addVarId, V4D(worldToGridMulAdd));
}
