// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_info.h>
#include <perfMon/dag_statDrv.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IBBox3.h>
#include <vecmath/dag_vecMath.h>
#include <generic/dag_staticTab.h>
#include <generic/dag_carray.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/bit.h>
#include <util/dag_convar.h>
#include <render/subtract_ibbox3.h>
#include <render/globTMVars.h>
#include <daSDF/worldSDF.h>
#include "shaders/world_sdf.hlsli"

CONSOLE_BOOL_VAL("render", world_sdf_invalidate, false);
CONSOLE_BOOL_VAL("render", world_sdf_update_one_mip, true);
CONSOLE_INT_VAL("render", world_sdf_ping_pong_iterations, 1, 0, 8);
CONSOLE_INT_VAL("render", world_sdf_invalidate_clip, -1, -1, 8);
CONSOLE_INT_VAL("render", world_sdf_depth_mark_pass, 16384, 0, 131072);
#define GLOBAL_VARS_LIST                          \
  VAR(world_sdf_update_current_frame)             \
  VAR(world_sdf_update_old)                       \
  VAR(world_sdf_update_lt_coord)                  \
  VAR(world_sdf_update_sz_coord)                  \
  VAR(world_sdf_raster_lt_coord)                  \
  VAR(world_sdf_raster_sz_coord)                  \
  VAR(world_sdf_to_atlas_decode__gradient_offset) \
  VAR(sdf_grid_culled_instances_grid_lt)          \
  VAR(sdf_grid_culled_instances_res)              \
  VAR(sdf_grid_cell_size)                         \
  VAR(world_sdf_cull_update_lt)                   \
  VAR(world_sdf_cull_update_rb)                   \
  VAR(world_sdf_cull_grid_boxes_count)            \
  VAR(world_sdf_res)                              \
  VAR(world_sdf_res_np2)                          \
  VAR(world_sdf_update_mip)                       \
  VAR(world_sdf_use_grid)                         \
  VAR(world_sdf_clipmap_samplerstate)             \
  VAR(world_sdf_clipmap_rasterize)

#define OPTIONAL_VARS_LIST        \
  VAR(world_sdf_support_uav_load) \
  VAR(world_sdf_inv_size)

static ShaderVariableInfo world_sdf_cull_grid_ltVarId[6], world_sdf_cull_grid_rbVarId[6];
#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
OPTIONAL_VARS_LIST
#undef VAR

static constexpr bool debugGrid = false;

inline IPoint3 round_up(const IPoint3 &lt, const IPoint3 &cell)
{
  IPoint3 div((lt.x + cell.x - 1) / cell.x, (lt.y + cell.y - 1) / cell.y, (lt.z + cell.z - 1) / cell.z);
  return IPoint3(div.x * cell.x, div.y * cell.y, div.z * cell.z);
}

WorldSDF::~WorldSDF() {}
float WorldSDF::get_voxel_size(float voxel0Size, uint32_t i) { return voxel0Size * (1 << i); }

VECTORCALL VECMATH_FINLINE vec4f v_perm_xzyw(vec4f a) { return v_perm_xzac(a, v_perm_yzwx(a)); }

struct WorldSDFParams
{
  Point4 world_sdf_lt[MAX_WORLD_SDF_CLIPS];
  Point4 world_sdf_lt_invalid;
  Point4 world_sdf_to_tc_add[MAX_WORLD_SDF_CLIPS];
  Point4 world_sdf_to_tc_add_invalid;
};

struct WorldSDFImpl final : public WorldSDF
{
  WorldSDFParams world_sdf_params;
  eastl::array<IPoint4, MAX_WORLD_SDF_CLIPS> world_sdf_coord_lt;
  PostFxRenderer sdf_world_debug;
  UniqueTexHolder world_sdf_clipmap;
  UniqueBufHolder world_sdf_coord_lt_buf;
  UniqueBufHolder world_sdf_params_buf;
  UniqueBufHolder culled_sdf_instances, culled_sdf_instances_grid;
  UniqueBuf world_sdf_clipmap_rasterize_owned;
  Sbuffer *world_sdf_clipmap_rasterize = nullptr;
  eastl::unique_ptr<ComputeShaderElement> world_sdf_update_cs, world_sdf_cull_instances_cs, world_sdf_cull_instances_clear_cs,
    world_sdf_cull_instances_grid_cs, world_sdf_clear_cs, world_sdf_ping_pong_cs, world_sdf_copy_slice_cs,
    world_sdf_ping_pong_final_cs;

  eastl::unique_ptr<ComputeShaderElement> world_sdf_from_gbuf_cs, world_sdf_from_gbuf_remove_cs;

  int w = 128, d = 128;
  uint32_t updatedMask = 0;
  bool cbuffersDirty = true;
  float clip0VoxelSize = 0.5;
  float temporalSpeed = 0.5f;
  uint32_t currentInstancesBufferSz = 0;
  bool supportUnorderedLoad = true;

  struct ClipmapMip
  {
    IPoint3 worldCoordLT = {-100000, 100000, 100000};
    float voxelSize = 1e9f;
    Point3 lastPreCachePos = {0, 0, 0};
    float lastPreCacheVoxelSize = 0;
  };
  dag::Vector<ClipmapMip> clipmap;
  carray<da_profiler::desc_id_t, MAX_WORLD_SDF_CLIPS> world_sdf_mip_dap, world_sdf_clip_cull_dap, world_sdf_clip_dap;

  void fixup_settings(uint16_t &cw, uint16_t &cd, uint8_t &c) const
  {
    eastl::array<uint16_t, 3> updateTsz{SDF_UPDATE_WARP_X, SDF_UPDATE_WARP_Y, SDF_UPDATE_WARP_Z};
    if (world_sdf_update_cs)
      updateTsz = world_sdf_update_cs->getThreadGroupSizes();

#if HAS_OBJECT_SDF

    G_ASSERTF(WORLD_SDF_TEXELS_ALIGNMENT_XZ % updateTsz[0] == 0 && WORLD_SDF_TEXELS_ALIGNMENT_XZ % updateTsz[1] == 0 &&
                WORLD_SDF_TEXELS_ALIGNMENT_ALT % updateTsz[2] == 0,
      "We are note checking out of bounds in update, so alignment %dx%dx%d should be multiples of thread count %dx%dx%d."
      " Adding bounds check is easy, so if you actually need smaller alignment just uncommented in shader and remove assert",
      WORLD_SDF_TEXELS_ALIGNMENT_XZ, WORLD_SDF_TEXELS_ALIGNMENT_XZ, WORLD_SDF_TEXELS_ALIGNMENT_ALT, updateTsz[0], updateTsz[1],
      updateTsz[2]);
#endif

    const uint32_t xy = max(updateTsz[0], updateTsz[1]);
    cw = ((cw + xy - 1) / xy) * xy;
    cd = ((cd + updateTsz[2] - 1) / updateTsz[2]) * updateTsz[2];
    if constexpr (!WORLD_SDF_ALLOW_NON_POW2)
    {
      cw = get_bigger_pow2(cw);
      cd = get_bigger_pow2(cd);
    }
    cw = clamp<uint16_t>(cw, 32, 1024);
    cd = clamp<uint16_t>(cd, 32, 1024);
    c = clamp<uint8_t>(c, 1, MAX_WORLD_SDF_CLIPS);
  }
  void initHistory(uint32_t clips, bool invalidate_current = true)
  {
    clipmap.clear();
    clipmap.resize(clips);
    if (invalidate_current)
    {
      for (int clip = 0; clip < MAX_WORLD_SDF_CLIPS; ++clip)
      {
        world_sdf_params.world_sdf_lt[clip] = Point4(-1e9f, 1e9f, 1e9f, 0);
        world_sdf_params.world_sdf_to_tc_add[clip] = Point4::ZERO;
      }
      world_sdf_params.world_sdf_lt_invalid = Point4(-1e9f, 1e9f, 1e9f, 0);
      world_sdf_params.world_sdf_to_tc_add_invalid = Point4::ZERO;
      std::memset(world_sdf_coord_lt.data(), 0, sizeof(world_sdf_coord_lt));
      cbuffersDirty = true;
      updatedMask = 0;
    }
  }

  static uint32_t get_required_temporal_buffer_size_no_fixup_settings(uint16_t cw, uint16_t cd)
  {
    const int N = WORLD_SDF_RASTERIZE_VOXELS_DIST * 2;
    return (cw + N) * (cw + N) * (cd + N);
  }

  uint32_t getRequiredTemporalBufferSize(uint16_t cw, uint16_t cd, uint8_t c) const override
  {
    fixup_settings(cw, cd, c);
    return get_required_temporal_buffer_size_no_fixup_settings(cw, cd);
  }

  uint32_t getRequiredTemporalBufferSize() const { return get_required_temporal_buffer_size_no_fixup_settings(w, d); }

  void setTemporalBuffer(const ManagedBuf &buf) override
  {
    if (!buf || buf->getNumElements() < getRequiredTemporalBufferSize())
    {
      if (buf)
      {
        logerr("WorldSDF: Temporal buffer is too small, expected at least %d, got %d", getRequiredTemporalBufferSize(),
          buf->getNumElements());
      }
      if (!world_sdf_clipmap_rasterize_owned)
        world_sdf_clipmap_rasterize = nullptr;
      return;
    }
    if (world_sdf_clipmap_rasterize_owned)
    {
      logwarn("WorldSDF: The own temporal buffer was created but it has been overridden by common");
      world_sdf_clipmap_rasterize_owned.close();
    }
    ShaderGlobal::set_buffer(world_sdf_clipmap_rasterizeVarId, buf);
    world_sdf_clipmap_rasterize = buf.getBuf();
  }

  void setValues(uint8_t clips, uint16_t w_, uint16_t h_, float voxel0) override
  {
    fixup_settings(w_, h_, clips);
    clip0VoxelSize = voxel0;
    if (clipmap.size() == clips && w == w_ && d == h_)
      return;
    const uint32_t texFmt = TEXFMT_R8;
    supportUnorderedLoad = d3d::get_texformat_usage(texFmt | TEXCF_UNORDERED) & d3d::USAGE_UNORDERED_LOAD;
    debug("world sdf inited with %dx%dx%d clips=%d, voxel %f dist = %f, uav load = %d", w_, h_, w_, clips, voxel0,
      0.5f * voxel0 * w_ * (1 << (clips - 1)), supportUnorderedLoad);
    w = w_;
    d = h_;
    initHistory(clips);
    world_sdf_clipmap.close();
    world_sdf_clipmap = dag::create_voltex(w, w, (d + 2) * clipmap.size(), texFmt | TEXCF_UNORDERED, 1, "world_sdf_clipmap");
    world_sdf_clipmap_samplerstateVarId.set_sampler(d3d::request_sampler({}));

    ShaderGlobal::set_int(world_sdf_support_uav_loadVarId, supportUnorderedLoad ? 1 : 0);
    ShaderGlobal::set_int4(world_sdf_resVarId, IPoint4(w, d, d + 2, clipmap.size()));

    constexpr int max_pos = (1 << 30) - 1;
    ShaderGlobal::set_int4(world_sdf_res_np2VarId, w, d, is_pow_of2(w) ? 0 : (max_pos / w) * w, is_pow_of2(d) ? 0 : (max_pos / d) * d);
    int fullTextureDepth = (d + 2) * clipmap.size();
    ShaderGlobal::set_color4(world_sdf_inv_sizeVarId, 1. / w, 1. / d, 1. / fullTextureDepth, float(d) / fullTextureDepth);

    float x = float(d) / fullTextureDepth;
    float dInv = 1. / d;
    float y = x * dInv * (1. / max(dInv, 0.00000001f) + 2.0f);
    ShaderGlobal::set_color4(world_sdf_to_atlas_decode__gradient_offsetVarId, x, y, 1.f / fullTextureDepth, 1.f / w);
  }

  WorldSDFImpl()
  {
#define VAR(a)     \
  if (!(a##VarId)) \
    logerr("mandatory shader variable is missing: %s", #a);
    GLOBAL_VARS_LIST
#undef VAR

    eastl::string str;
    world_sdf_coord_lt_buf = dag::buffers::create_persistent_cb(world_sdf_coord_lt.size(), "world_sdf_coord_lt_buf");
    static_assert(sizeof(WorldSDFParams) % d3d::buffers::CBUFFER_REGISTER_SIZE == 0);
    world_sdf_params_buf =
      dag::buffers::create_persistent_cb(sizeof(WorldSDFParams) / d3d::buffers::CBUFFER_REGISTER_SIZE, "world_sdf_params");
    for (int i = 0; i < 6; ++i)
    {
      str.sprintf("world_sdf_cull_grid_lt_%d", i);
      world_sdf_cull_grid_ltVarId[i] = get_shader_variable_id(str.c_str(), false);
      str.sprintf("world_sdf_cull_grid_rb_%d", i);
      world_sdf_cull_grid_rbVarId[i] = get_shader_variable_id(str.c_str(), false);
    }
    for (int i = 0; i < MAX_WORLD_SDF_CLIPS; ++i)
    {
      str.sprintf("world_sdf_mip%d", i);
      world_sdf_mip_dap[i] = DA_PROFILE_ADD_LOCAL_DESCRIPTION(0, str.c_str());
      str.sprintf("world_sdf_clip_cull%d", i);
      world_sdf_clip_cull_dap[i] = DA_PROFILE_ADD_LOCAL_DESCRIPTION(0, str.c_str());
      str.sprintf("world_sdf_clip%d", i);
      world_sdf_clip_dap[i] = DA_PROFILE_ADD_LOCAL_DESCRIPTION(0, str.c_str());
    }
#define CS_SHADER(name) name.reset(new_compute_shader(#name));
    ShaderGlobal::set_int(get_shader_variable_id("supports_sh_6_1", true), d3d::get_driver_desc().shaderModel >= 6.1_sm ? 1 : 0);

    CS_SHADER(world_sdf_ping_pong_final_cs);
    CS_SHADER(world_sdf_ping_pong_cs);
    CS_SHADER(world_sdf_from_gbuf_cs);
    CS_SHADER(world_sdf_from_gbuf_remove_cs);
    CS_SHADER(world_sdf_update_cs);
    CS_SHADER(world_sdf_copy_slice_cs);
    CS_SHADER(world_sdf_cull_instances_grid_cs);
    CS_SHADER(world_sdf_cull_instances_clear_cs);
    CS_SHADER(world_sdf_clear_cs);
    G_ASSERT(world_sdf_update_cs);
  }

  uint32_t getMinInstancesToUseGrid()
  {
    auto updateTsz = world_sdf_update_cs->getThreadGroupSizes();
    return updateTsz[0] * updateTsz[1] * updateTsz[2];
  }

  void ensureSdfObjects(uint32_t instances)
  {
#if !SDF_INSTANCES_CPU_CULLING
    world_sdf_cull_instances_cs.reset(new_compute_shader("world_sdf_cull_instances_cs"));
    if (currentInstancesBufferSz < instances)
    {
      currentInstancesBufferSz = (max<uint32_t>(instances + 1, SDF_MAX_CULLED_INSTANCES + 1) + 3) & ~3;
      culled_sdf_instances.close();
      / CVSculled_sdf_instances = dag::create_sbuffer(sizeof(uint32_t), currentInstancesBufferSz,
        (debugGrid ? SBCF_USAGE_READ_BACK : 0) | SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0,
        "culled_sdf_instances");
    }
#endif
    if (instances >= getMinInstancesToUseGrid() && !culled_sdf_instances_grid)
    {
      culled_sdf_instances_grid = dag::create_sbuffer(sizeof(uint32_t), (SDF_CULLED_INSTANCES_GRID_SIZE + 3) & ~3,
        (debugGrid ? SBCF_USAGE_READ_BACK : 0) | SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0,
        "culled_sdf_instances_grid");
    }
  }

  enum
  {
    MOVE_ALIGNMENT_THRESHOLD = 1
  };
  int getTexelMoveThresholdXZ() const override { return (MOVE_ALIGNMENT_THRESHOLD + 1) * WORLD_SDF_TEXELS_ALIGNMENT_XZ; }
  int getTexelMoveThresholdAlt() const override { return (MOVE_ALIGNMENT_THRESHOLD + 1) * WORLD_SDF_TEXELS_ALIGNMENT_ALT; }

  static bool shouldUpdateMovement(const IPoint3 &old_coord, float old_voxel_size, const IPoint3 &new_coord, float new_voxel_size)
  {
    if (old_voxel_size != new_voxel_size)
      return true;
    const IPoint3 move = abs(old_coord - new_coord);
    return !(max(move.x, move.y) <= MOVE_ALIGNMENT_THRESHOLD * WORLD_SDF_TEXELS_ALIGNMENT_XZ &&
             move.z <= MOVE_ALIGNMENT_THRESHOLD * WORLD_SDF_TEXELS_ALIGNMENT_ALT);
  }

  bool shouldUpdate(int i, const IPoint3 &center_coord, float voxelSize) const
  {
    if (world_sdf_invalidate)
      return true;
    if (world_sdf_invalidate_clip.get() == i)
      return true;
    auto &mip = clipmap[i];
    return shouldUpdateMovement(mip.worldCoordLT, mip.voxelSize, center_coord - IPoint3(w / 2, w / 2, d / 2), voxelSize);
  }
  bool shouldPrecache(const Point3 &pos, uint32_t i) const
  {
    IPoint3 new_center = clip_world_voxels_center_from_world_pos(i, pos);
    auto &mip = clipmap[i];
    IPoint3 old_center = clip_world_voxels_center_from_world_pos(i, mip.lastPreCachePos);
    return shouldUpdateMovement(old_center, mip.lastPreCacheVoxelSize, new_center, clip_voxel_size(i));
  }
  int moveBoxes(const IPoint3 &new_lt, const IPoint3 &old_lt, carray<IBBox3, 6> &rets) const
  {
    IBBox3 oldWorldBox = {old_lt, old_lt + IPoint3(w, w, d)};
    IBBox3 newWorldBox = {new_lt, new_lt + IPoint3(w, w, d)};
    dag::Span<IBBox3> retSpan = dag::Span<IBBox3>(rets.data(), rets.size());
    IBBox3 in;
    int cnt = same_size_box_subtraction(newWorldBox, oldWorldBox, in, retSpan);
#if DAGOR_DBGLEVEL > 0
    for (auto &ci : dag::Span<IBBox3>(rets.data(), cnt))
    {
      IPoint3 v = ci.width();
      G_ASSERT(v.x > 0 && v.y > 0 && v.z > 0);
    }
#endif
    return cnt;
  }
  bool teleported(const Point3 &voxels_abs_move) const
  {
    const Point3 absMovePart = div(voxels_abs_move, Point3(w, w, d));
    float maxMove = max(max(absMovePart.x, absMovePart.y), absMovePart.z), totalMove = absMovePart.x + absMovePart.y + absMovePart.z;
    // we have teleported in one direction or moved too far in all three (leaving less than 1/8 of volume unchanged)
    return maxMove >= 1.f || totalMove >= 1.5f;
  }

  enum class UpdateStatus
  {
    NotChanged,
    Updated,
    Postponed
  };

  void updateCBuffers()
  {
    if (!world_sdf_params_buf.getBuf()->updateData(0, sizeof(world_sdf_params), &world_sdf_params, VBLOCK_DISCARD))
    {
      logerr("WorldSDF: could not update buffer %s", world_sdf_params_buf.getBuf()->getBufName());
      return;
    }
    if (!world_sdf_coord_lt_buf.getBuf()->updateData(0, sizeof(world_sdf_coord_lt), world_sdf_coord_lt.data(), VBLOCK_DISCARD))
    {
      logerr("WorldSDF: could not update buffer %s", world_sdf_coord_lt_buf.getBuf()->getBufName());
      return;
    }
    cbuffersDirty = false;
  }

  UpdateStatus updateMip(int clip, const Point3 &originPos, IPoint3 center_coord, float voxelSize, const request_instances_cb &cb,
    const render_instances_cb &render_cb)
  {
    DA_PROFILE_GPU_EVENT_DESC(world_sdf_mip_dap[clip]);
    auto &mip = clipmap[clip];
    IPoint3 worldCoordLT = center_coord - IPoint3(w / 2, w / 2, d / 2);
    if (world_sdf_invalidate || world_sdf_invalidate_clip.get() == clip)
      mip.voxelSize = voxelSize + 1.;

    int worldAxis = 3;
    if (voxelSize == mip.voxelSize)
    {
      IPoint3 move = (worldCoordLT - mip.worldCoordLT), absMove = abs(move);
      if (!teleported(point3(absMove)))
      {
        IPoint3 volumeMove(w * d * absMove.x, w * d * absMove.y, w * w * absMove.z);
        if (volumeMove.x >= volumeMove.y && volumeMove.x >= volumeMove.z) // x axis
        {
          worldAxis = 0;
          worldCoordLT.y = mip.worldCoordLT.y, worldCoordLT.z = mip.worldCoordLT.z;
        }
        else if (volumeMove.y >= volumeMove.x && volumeMove.y >= volumeMove.z) // y axis
        {
          worldAxis = 2;
          worldCoordLT.x = mip.worldCoordLT.x, worldCoordLT.z = mip.worldCoordLT.z;
        }
        else if (volumeMove.z >= volumeMove.x && volumeMove.z >= volumeMove.y) // z axis
        {
          worldAxis = 1;
          worldCoordLT.y = mip.worldCoordLT.y, worldCoordLT.x = mip.worldCoordLT.x;
        }
      }
    }
    if (worldCoordLT == mip.worldCoordLT && voxelSize == mip.voxelSize)
      return UpdateStatus::NotChanged;

#if DAGOR_DBGLEVEL > 0
    if (!is_pow_of2(w) || !is_pow_of2(d))
    {
      IPoint4 l = world_sdf_res_np2VarId.get_int4();
      if (min(min(l.z + abs(worldCoordLT.x), l.z + abs(worldCoordLT.y)), l.w + abs(worldCoordLT.y)) < 0 || l.z < -worldCoordLT.x ||
          l.z < -worldCoordLT.y || l.w < -worldCoordLT.z)
      {
        LOGERR_ONCE("position %@ is too far from center, due to non-pow2 of clip size %dx%d."
                    "See magic_np2.txt",
          worldCoordLT, w, d);
      }
    }
#endif
    auto old = mip;

    if (old.voxelSize != voxelSize)
      old.worldCoordLT = worldCoordLT + IPoint3(w, w, d) * 2;

    carray<IBBox3, 6> rets;
    int cnt = moveBoxes(worldCoordLT, old.worldCoordLT, rets);
    carray<bbox3f, 6> worldBoxes;
    // encoded dist is up to MAX_WORLD_SDF_VOXELS_BAND. Since we check distance from CENTER of voxel, decrease by 0.5 voxel
    vec4f maxVoxelsDist = v_splats(MAX_WORLD_SDF_VOXELS_BAND - 0.5f);
    for (int bi = 0; bi < cnt; ++bi)
    {
      // full sizes, from top-left to bottom-right
      worldBoxes[bi].bmin = v_mul(v_sub(v_perm_xzyw(v_cvt_vec4f(v_ldui(&rets[bi][0].x))), maxVoxelsDist), v_splats(voxelSize));
      worldBoxes[bi].bmax = v_mul(v_add(v_perm_xzyw(v_cvt_vec4f(v_ldui(&rets[bi][1].x))), maxVoxelsDist), v_splats(voxelSize));
    }
    if (worldAxis < 3)
      mip.lastPreCachePos[worldAxis] = originPos[worldAxis];
    else
      mip.lastPreCachePos = originPos;
    uint32_t max_instances = 0;

    G_UNUSED(cb);
#if HAS_OBJECT_SDF
    auto ret = cb(worldBoxes.data(), cnt, UpdateSDFQualityRequest::ASYNC_REQUEST_BEST_ONLY, clip, max_instances);
    if (ret == UpdateSDFQualityStatus::NOT_READY)
      return UpdateStatus::Postponed;
    if (ret != UpdateSDFQualityStatus::ALL_OK)
    {
      // todo: schedule re-update same box when data is streamed ready!
      return UpdateStatus::Postponed;
    }
#endif


    eastl::string str;
    mip.worldCoordLT = worldCoordLT;
    mip.voxelSize = voxelSize;
    if (old.voxelSize != voxelSize)
      old.worldCoordLT = worldCoordLT + IPoint3(w, w, d);

    if (!cnt) // same box?
    {
      G_ASSERT(0);
      return UpdateStatus::Updated;
    }
    // debug("clip %d: %@->%@ count=%d, totalvoxels=%d", clip, oldWorldBox, newWorldBox, cnt, voxels);
    ShaderGlobal::set_int(world_sdf_cull_grid_boxes_countVarId, cnt);
    IBBox3 maxBox = rets[0];
    for (int bi = 0; bi < cnt; ++bi)
    {
      maxBox[0] = min(maxBox[0], rets[bi][0]);
      maxBox[1] = max(maxBox[1], rets[bi][1]);
      Point3 lt = point3(rets[bi][0]) * voxelSize, // full size, from top-left corner, hence no +0.5
        rb = point3(rets[bi][1]) * voxelSize;      // to bottom-right corner
      ShaderGlobal::set_color4(world_sdf_cull_grid_ltVarId[bi], lt.x, lt.z, lt.y, voxelSize);
      ShaderGlobal::set_color4(world_sdf_cull_grid_rbVarId[bi], rb.x, rb.z, rb.y, 1. / voxelSize);
    }

    {
      // full size from top-left bottom-right
      Point3 lt = point3(maxBox[0]) * voxelSize, rb = point3(maxBox[1]) * voxelSize;
      ShaderGlobal::set_color4(world_sdf_cull_update_ltVarId, lt.x, lt.z, lt.y, voxelSize);
      ShaderGlobal::set_color4(world_sdf_cull_update_rbVarId, rb.x, rb.z, rb.y, 1. / voxelSize);
    }

    ShaderGlobal::set_int(world_sdf_update_mipVarId, clip);
    ShaderGlobal::set_int4(world_sdf_update_oldVarId, old.worldCoordLT.x, old.worldCoordLT.y, old.worldCoordLT.z,
      bitwise_cast<int>(old.voxelSize));

    {
      const Point3 lt = Point3(worldCoordLT.x, worldCoordLT.z, worldCoordLT.y) * voxelSize;
      world_sdf_coord_lt[clip] = IPoint4(worldCoordLT.x, worldCoordLT.y, worldCoordLT.z, bitwise_cast<int>(voxelSize));
      world_sdf_params.world_sdf_lt[clip] = Point4(lt.x, lt.y, lt.z, voxelSize);
      world_sdf_params.world_sdf_to_tc_add[clip] =
        Point4(1. / (w * voxelSize), 1. / (w * voxelSize), 1. / (d * voxelSize), 1. / voxelSize);
    }

#if HAS_OBJECT_SDF
    auto updateTsz = world_sdf_update_cs->getThreadGroupSizes();
    if (max_instances)
    {
      ensureSdfObjects(max_instances);
      // instances for all
      DA_PROFILE_GPU_EVENT_DESC(world_sdf_clip_cull_dap[clip]);
#if !SDF_INSTANCES_CPU_CULLING
      {
        TIME_D3D_PROFILE(cull_instances);
        d3d::set_rwbuffer(STAGE_CS, 1, culled_sdf_instances_grid.getBuf());
        world_sdf_cull_instances_clear_cs->dispatchThreads(1, 1, 1);
      }
      d3d::resource_barrier({culled_sdf_instances.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
      world_sdf_cull_instances_cs->dispatchThreads(max_instances, 1, 1);
      d3d::resource_barrier({culled_sdf_instances.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
      if (debugGrid)
      {
        uint32_t *data;
        if (culled_sdf_instances->lock(0, SDF_MAX_CULLED_INSTANCES * 4, (void **)&data, VBLOCK_READONLY))
        {
          debug("mip %d culled instances %d", clip, *data);
          maxCount = *data;
          for (int j = 0; j < maxCount; ++j)
            if (data[j + 1] >= max_instances)
              debug("invalid instance %d at %d", data[j + 1], j);
          culled_sdf_instances->unlock();
        }
      }
#endif

      const bool useGrid = (max_instances >= getMinInstancesToUseGrid());
      ShaderGlobal::set_int(world_sdf_use_gridVarId, useGrid ? 1 : 0);
      if (useGrid)
      {
        TIME_D3D_PROFILE(cull_instances_grid);
        uint32_t maxCount = max_instances;
        // may be do not use grid if not enough instances?

        IPoint3 gridCellSz = IPoint3(updateTsz[0], updateTsz[1], updateTsz[2]) * SDF_GRID_CELL_SIZE_IN_TILES;
        IPoint3 gridLTInVoxels = maxBox[0];
        IPoint3 updateRegionInVoxels = round_up(maxBox.width(), gridCellSz);
        IPoint3 gridRes(updateRegionInVoxels.x / gridCellSz.x, updateRegionInVoxels.y / gridCellSz.y,
          updateRegionInVoxels.z / gridCellSz.z);
        ShaderGlobal::set_int4(sdf_grid_culled_instances_resVarId, IPoint4(gridRes.x, gridRes.y, gridRes.z, 0));

        const Point3 gridSzMeters = point3(gridCellSz) * voxelSize;
        // debug("grid update: box %@ grid start %@, meters = %@ res %@", maxBox, gridLTInVoxels, gridSzMeters, gridRes);
        ShaderGlobal::set_int4(sdf_grid_culled_instances_grid_ltVarId,
          IPoint4(gridLTInVoxels.x, gridLTInVoxels.y, gridLTInVoxels.z, 0));
        ShaderGlobal::set_color4(sdf_grid_cell_sizeVarId, gridSzMeters.x, gridSzMeters.z, gridSzMeters.y, voxelSize);
        // debug("grid %@ voxel %f", lt, voxelSize*div);
        // d3d::resource_barrier({culled_sdf_instances_grid.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
        d3d::set_rwbuffer(STAGE_CS, 1, culled_sdf_instances_grid.getBuf());
        world_sdf_cull_instances_grid_cs->dispatch(gridRes.x, gridRes.y, gridRes.z);
        d3d::resource_barrier({culled_sdf_instances_grid.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
        d3d::set_rwbuffer(STAGE_CS, 1, nullptr);
        if (debugGrid)
        {
          uint32_t *data;
          if (culled_sdf_instances_grid->lock(0, SDF_CULLED_INSTANCES_GRID_SIZE, (void **)&data, VBLOCK_READONLY))
          {
            debug("mip %d grid culled instances %d, gridRes = %@", clip, *data, gridRes);
            for (int gi = 0; gi < gridRes.x * gridRes.y * gridRes.z; ++gi)
            {
              uint32_t ofs = data[gi * 2 + 1], cnt = data[gi * 2 + 2];
              if (cnt > maxCount)
                debug("grid at %d has invalid cnt %d:%d", gi, ofs, cnt);
              if (ofs + cnt * 4 > 4 * (SDF_CULLED_INSTANCES_GRID_SIZE) || ofs % 4 != 0)
                debug("grid at %d has invalid ofs %d:%d", gi, ofs, cnt);
              if (ofs < (gridRes.x * gridRes.y * gridRes.z * 2 + 1) * 4)
                debug("grid at %d has small ofs %d:%d", gi, ofs, cnt);
              ofs = min<uint32_t>(ofs, SDF_CULLED_INSTANCES_GRID_SIZE);
              cnt = min<uint32_t>(cnt, (SDF_CULLED_INSTANCES_GRID_SIZE - ofs) / 4);
              for (int j = 0; j < cnt; ++j)
                if (data[ofs / 4 + j] >= max_instances)
                  debug("grid at %d, %d has invalid id %d:%d = %d", gi, j, ofs, cnt, data[ofs / 4 + j]);
            }
            culled_sdf_instances_grid->unlock();
          }
        }
      }
    }
#endif // #if HAS_OBJECT_SDF
    d3d::set_rwbuffer(STAGE_CS, 2, world_sdf_clipmap_rasterize);
    G_ASSERT(maxBox.width().x <= w && maxBox.width().y <= w && maxBox.width().z <= d);
    const IPoint3 voxelAround =
      IPoint3(WORLD_SDF_RASTERIZE_VOXELS_DIST, WORLD_SDF_RASTERIZE_VOXELS_DIST, WORLD_SDF_RASTERIZE_VOXELS_DIST);
    const IPoint3 rasterizeLt = maxBox[0] - voxelAround, rasterizeWidth = maxBox.width() + voxelAround * 2;
    ShaderGlobal::set_int4(world_sdf_raster_lt_coordVarId, rasterizeLt.x, rasterizeLt.y, rasterizeLt.z,
      bitwise_cast<int>(1.f / voxelSize));
    ShaderGlobal::set_int4(world_sdf_raster_sz_coordVarId, rasterizeWidth.x, rasterizeWidth.y, rasterizeWidth.z,
      bitwise_cast<int>(voxelSize));
    DA_PROFILE_GPU_EVENT_DESC(world_sdf_clip_dap[clip]);
    {
      // clear all rasterized buffer, extended by rasterizationVoxelAround
      // may be just clear all buffer instead?
      ShaderGlobal::set_int4(world_sdf_update_lt_coordVarId, maxBox[0].x, maxBox[0].y, maxBox[0].z, 0);                // remove me
      ShaderGlobal::set_int4(world_sdf_update_sz_coordVarId, maxBox.width().x, maxBox.width().y, maxBox.width().z, 0); // remove me
      world_sdf_clear_cs->dispatchThreads(rasterizeWidth.x * rasterizeWidth.y * rasterizeWidth.z, 1, 1);
    }

#if HAS_OBJECT_SDF
    if (max_instances)
    {
      for (int ui = 0; max_instances && ui < cnt; ui++)
      {
        TIME_D3D_PROFILE(dispatch_sdf);
        const IPoint3 updateSize = rets[ui].width();
        ShaderGlobal::set_int4(world_sdf_update_lt_coordVarId, rets[ui][0].x, rets[ui][0].y, rets[ui][0].z, 0);
        ShaderGlobal::set_int4(world_sdf_update_sz_coordVarId, updateSize.x, updateSize.y, updateSize.z, 0);
        // there are no checks inside shader so
        G_ASSERT(updateSize.x % updateTsz[0] == 0 && updateSize.y % updateTsz[1] == 0 && updateSize.z % updateTsz[2] == 0);
        world_sdf_update_cs->dispatchThreads(updateSize.x, updateSize.y, updateSize.z);
        if (cnt > 1)
        {
          d3d::resource_barrier({world_sdf_clipmap_rasterize, RB_NONE});
        }
      }
    }
#elif 0
    {
      TIME_D3D_PROFILE(toroidal_sdf);
      bool rendered = false;
      for (int ui = 0; ui < cnt; ui++)
      {
        TIME_D3D_PROFILE(dispatch_sdf);
        const IPoint3 updateSize = rets[ui].width();
        if (old.worldCoordLT.x == worldCoordLT.x && old.worldCoordLT.y == worldCoordLT.y && old.worldCoordLT.z < worldCoordLT.z)
        {
          // we are moving up, so we just ignore it
          continue;
        }

        ShaderGlobal::set_int4(world_sdf_update_lt_coordVarId, rets[ui][0].x, rets[ui][0].y, rets[ui][0].z, 0);
        ShaderGlobal::set_int4(world_sdf_update_sz_coordVarId, updateSize.x, updateSize.y, updateSize.z, 0);
        if (rendered)
          d3d::resource_barrier({world_sdf_clipmap_rasterize, RB_NONE});
        else
          d3d::resource_barrier({world_sdf_clipmap_rasterize, RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
        world_sdf_update_toroidal_cs->dispatchThreads(updateSize.x * updateSize.y, 1, 1);
        rendered = true;
      }
    }
#endif

    d3d::resource_barrier({world_sdf_clipmap_rasterize, RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

    d3d::set_rwbuffer(STAGE_CS, 2, nullptr); // this is dx11 workaround. it flushes CS UAVs incorrectly if it is set in PS and CS
                                             // simultaneously
    d3d::set_rwbuffer(STAGE_CS, 2, world_sdf_clipmap_rasterize);
    d3d::set_rwbuffer(STAGE_PS, 2, world_sdf_clipmap_rasterize);
    ShaderGlobal::set_int4(world_sdf_update_lt_coordVarId, maxBox[0].x, maxBox[0].y, maxBox[0].z, 0);
    ShaderGlobal::set_int4(world_sdf_update_sz_coordVarId, maxBox.width().x, maxBox.width().y, maxBox.width().z, 0);
    if (render_cb(clip,
          BBox3(Point3(maxBox[0].x, maxBox[0].z, maxBox[0].y) * voxelSize, Point3(maxBox[1].x, maxBox[1].z, maxBox[1].y) * voxelSize),
          // BBox3(Point3(rasterizeLt.x, rasterizeLt.z, rasterizeLt.y) * voxelSize, Point3::xzy(rasterizeLt + rasterizeWidth) *
          // voxelSize),
          voxelSize, max_instances))
    {
      if (world_sdf_ping_pong_cs)
      {
        DA_PROFILE_GPU_EVENT_DESC(world_sdf_clip_dap[clip]);
        d3d::set_rwbuffer(STAGE_CS, 2, nullptr); // this is dx11 workaround. it flushes CS UAVs incorrectly if it is set in PS and CS
                                                 // simultaneously
        d3d::set_rwbuffer(STAGE_CS, 2, world_sdf_clipmap_rasterize);
        for (int ui = 0; ui < cnt; ui++)
        {
          TIME_D3D_PROFILE(dispatch_ping_pong);
          const IPoint3 updateSize = rets[ui].width();
          // not a mistake that we use original boxes, we are interested only in inner surface
          // todo: probably better process extended box with a check inside shader
          ShaderGlobal::set_int4(world_sdf_update_lt_coordVarId, rets[ui][0].x, rets[ui][0].y, rets[ui][0].z, 0);
          ShaderGlobal::set_int4(world_sdf_update_sz_coordVarId, updateSize.x, updateSize.y, updateSize.z, 0);
          // if we do that more times we would got better results
          for (int i = 0, ie = world_sdf_ping_pong_iterations.get(); i < ie; ++i)
          {
            // d3d::resource_barrier({culled_sdf_instances.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
            world_sdf_ping_pong_cs->dispatchThreads(updateSize.x, updateSize.y, updateSize.z);
          }
        }
      }
    }

    d3d::set_rwbuffer(STAGE_PS, 2, nullptr);
    {
      d3d::set_rwbuffer(STAGE_CS, 0, nullptr); // this is dx11 workaround. it flushes CS UAVs incorrectly if it is set in PS and CS
                                               // simultaneously
      d3d::set_rwbuffer(STAGE_CS, 2, nullptr); // this is dx11 workaround. it flushes CS UAVs incorrectly if it is set in PS and CS
                                               // simultaneously
      d3d::set_rwtex(STAGE_CS, 0, world_sdf_clipmap.getVolTex(), 0, 0);
      d3d::set_rwbuffer(STAGE_CS, 2, world_sdf_clipmap_rasterize);
      {
        TIME_D3D_PROFILE(dispatch_ping_pong_final);
        for (int ui = 0; ui < cnt; ui++)
        {
          const IPoint3 updateSize = rets[ui].width();
          // not a mistake that we use original boxes, we are interested only in inner surface
          ShaderGlobal::set_int4(world_sdf_update_lt_coordVarId, rets[ui][0].x, rets[ui][0].y, rets[ui][0].z, 0);
          ShaderGlobal::set_int4(world_sdf_update_sz_coordVarId, updateSize.x, updateSize.y, updateSize.z, 0);
          world_sdf_ping_pong_final_cs->dispatchThreads(updateSize.x, updateSize.y, updateSize.z);
          if (supportUnorderedLoad && ui != cnt - 1)
            d3d::resource_barrier({world_sdf_clipmap.getVolTex(), RB_NONE, 0, 0}); // no overlap between regions
        }
      }
      TIME_D3D_PROFILE(copy_slice_sdf);

      d3d::set_rwbuffer(STAGE_CS, 2, nullptr);
      if (supportUnorderedLoad) // we have written results to world_sdf_clipmap texture only
        d3d::resource_barrier({world_sdf_clipmap.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
      else // we have written results for copies to world_sdf_clipmap_rasterize texture as well
        d3d::resource_barrier({world_sdf_clipmap_rasterize, RB_RO_SRV | RB_STAGE_COMPUTE});
      for (int ui = 0; ui < cnt; ui++)
      {
        // todo: check if rets box includes wrapped (0 | d-1) z coord at all, and skip otherwise (if we moved in vertical direction, it
        // won't always happen) todo:  we can now copy same slices twice, check if rets[ui][0].x, rets[ui][0].y already processed
        ShaderGlobal::set_int4(world_sdf_update_lt_coordVarId, rets[ui][0].x, rets[ui][0].y, 0, 0);
        const IPoint3 updateSize = rets[ui].width();
        ShaderGlobal::set_int4(world_sdf_update_sz_coordVarId, updateSize.x, updateSize.y, 0, 0);
        world_sdf_copy_slice_cs->dispatchThreads(updateSize.x, updateSize.y, 2);
        if (ui != cnt - 1)
          d3d::resource_barrier({world_sdf_clipmap.getVolTex(), RB_NONE, 0, 0}); // no overlap between regions
      }
      d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
      d3d::resource_barrier({world_sdf_clipmap.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
    }

    return UpdateStatus::Updated;
  }
  int rasterizationVoxelAround() const override
  {
    // todo: without tyoed load return 0
    return WORLD_SDF_RASTERIZE_VOXELS_DIST;
  }
  float clip_voxel_size(int i) const { return WorldSDF::get_voxel_size(clip0VoxelSize, i); }
  IPoint3 clip_world_voxels_center_from_world_pos(int i, const Point3 &pos) const
  {
    const float voxelSize = clip_voxel_size(i);
    const Point3 floorTo = Point3(voxelSize * WORLD_SDF_TEXELS_ALIGNMENT_XZ, voxelSize * WORLD_SDF_TEXELS_ALIGNMENT_ALT,
      voxelSize * WORLD_SDF_TEXELS_ALIGNMENT_XZ);
    IPoint3 r = ipoint3(floor(div(pos, floorTo) + Point3(0.5, 0.5, 0.5)));
    return IPoint3(r.x * WORLD_SDF_TEXELS_ALIGNMENT_XZ, r.z * WORLD_SDF_TEXELS_ALIGNMENT_XZ,
      r.y * WORLD_SDF_TEXELS_ALIGNMENT_ALT); // flip yz, as this is our coordinates
  }

  bool shouldUpdate(const Point3 &pos, uint32_t i) const override
  {
    return shouldUpdate(i, clip_world_voxels_center_from_world_pos(i, pos), clip_voxel_size(i));
  }
  bool shouldUpdate(const Point3 &pos) const
  {
    for (int i = clipmap.size() - 1; i >= 0; --i)
      if (shouldUpdate(pos, i))
        return true;
    return false;
  }
  void prefetch(int clip, const Point3 &pos, const request_prefetch_cb &rcb)
  {
    auto &mip = clipmap[clip];
    Point3 lastMove = pos - mip.lastPreCachePos;
    // const float moveDistSq = lengthSq(lastMove);
    const float voxelSize = clip_voxel_size(clip);
    /*if (moveDistSq < voxelSize*voxelSize) // we haven't moved much
    {
      if (i == e - 1)
        debug("moved %f", sqrtf(moveDistSq));
      continue;
    }*/
    const Point3 moveVoxels = lastMove / voxelSize;
    int axis = 3;
    BBox3 prefetchBox;
    if (!teleported(moveVoxels) && mip.lastPreCacheVoxelSize == voxelSize)
    {
      const float blockSizeXZ = getTexelMoveThresholdXZ();
      const float blockSizeALT = getTexelMoveThresholdAlt();
      Point3 blockSize = Point3(blockSizeXZ, blockSizeALT, blockSizeXZ);
      const Point3 move = div(moveVoxels, blockSize);
      const Point3 absMove = abs(move);
      const float maxMove = max(max(absMove.x, absMove.y), absMove.z);
      axis = absMove.x == maxMove ? 0 : absMove.z == maxMove ? 2 : 1;
      const bool shouldUpdateClip = shouldPrecache(pos, clip);
      if (maxMove < 0.25f && !shouldUpdateClip)
      {
        // debug("prefetch(%d) no move %@ %@-%@ box %@->%@",
        //   clip, mip.lastPreCachePos, pos, move, cBox, getClipBoxAt(pos, clip));
        return;
      }
      if (shouldUpdateClip) // nextBox != prefetchBox
      {
        prefetchBox = getClipBoxAt(mip.lastPreCachePos, clip);
        BBox3 nextBox = getClipBoxAt(pos, clip);
        // still choose one axis
        const Point3 boxMove = nextBox.center() - prefetchBox.center();
        const Point3 absBoxMove = abs(boxMove);
        const float maxBoxMove = max(max(absBoxMove.x, absBoxMove.y), absBoxMove.z);
        axis = absBoxMove.x == maxBoxMove ? 0 : absBoxMove.z == maxBoxMove ? 2 : 1;
        bool negSign = absBoxMove[axis] < 0;
        int moveLim = negSign ? 0 : 1;
        // debug("%d: moved beyond extrapolating old Box %@ nextBox %@ boxMove = %@[%d], orginal %@[%d]",
        //   clip, prefetchBox, nextBox, boxMove, axis, move, originalAxis);
        prefetchBox[1 - moveLim][axis] = prefetchBox[moveLim][axis];
        prefetchBox[moveLim][axis] = nextBox[moveLim][axis];
      }
      else
      {
        prefetchBox = getClipBox(clip);
        bool negSign = move[axis] < 0;
        int moveLim = negSign ? 0 : 1;
        // extrapolating one
        // debug("%d: extrapolating %@ maxMove = %f %@[%d]",
        //  clip, prefetchBox, maxMove, move, originalAxis);
        prefetchBox[1 - moveLim][axis] = prefetchBox[moveLim][axis];
        const float expanse = 1.f * voxelSize;
        prefetchBox[moveLim][axis] += expanse * (negSign ? -blockSize[axis] : blockSize[axis]);
      }
    }
    else
    {
      prefetchBox = getClipBoxAt(pos, clip);
    }
    // encoded dist is up to MAX_WORLD_SDF_VOXELS_BAND. Since we check distance from CENTER of voxel, decrease by 0.5 voxel
    Point3 maxVoxelsDist = (MAX_WORLD_SDF_VOXELS_BAND - 0.5f) * Point3(voxelSize, voxelSize, voxelSize);
    prefetchBox[0] -= maxVoxelsDist;
    prefetchBox[1] += maxVoxelsDist;
    bbox3f vbox;
    vbox.bmin = v_ldu(&prefetchBox[0].x);
    vbox.bmax = v_ldu(&prefetchBox[1].x);
    if (!rcb(vbox, clip))
      return;
    // debug("%d: axis %d; %@->%@", clip, axis, mip.lastPreCachePos, pos);
    if (axis < 3)
      mip.lastPreCachePos[axis] = pos[axis];
    else
      mip.lastPreCachePos = pos;
    mip.lastPreCacheVoxelSize = voxelSize;
  }
  void prefetchAll(const Point3 &pos, const request_prefetch_cb &rcb, uint32_t updated_mask)
  {
    for (int i = clipmap.size() - 1; i >= 0; --i)
      if (!(updated_mask & (1 << i)))
        prefetch(i, pos, rcb);
  }

  void ensureTemporalBufferSize()
  {
    const uint32_t sizeNeeded = getRequiredTemporalBufferSize();
    if (world_sdf_clipmap_rasterize && world_sdf_clipmap_rasterize->getNumElements() >= sizeNeeded)
      return;

    if (world_sdf_clipmap_rasterize && !world_sdf_clipmap_rasterize_owned)
    {
      logerr("WorldSDF: Common temporal buffer is failed to be reused (not enough size: %d)",
        world_sdf_clipmap_rasterize->getNumElements());
    }

    world_sdf_clipmap_rasterize_owned.close();
    world_sdf_clipmap_rasterize_owned = dag::create_sbuffer(sizeof(uint32_t), sizeNeeded,
      SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "world_sdf_clipmap_rasterize");
    world_sdf_clipmap_rasterize = world_sdf_clipmap_rasterize_owned.getBuf();
    logwarn("WorldSDF: Created a new temporal buffer (size: %d)", sizeNeeded);
  }

  uint32_t updateNoPrefetch(const Point3 &pos, const request_instances_cb &cb, const render_instances_cb &render_cb,
    uint32_t allow_update_mask)
  {
    uint32_t updated = 0;
    if (!world_sdf_update_cs)
      return 0;
    if (!shouldUpdate(pos))
      return 0;

    ensureTemporalBufferSize();

    DA_PROFILE_GPU;
    const bool allMips = world_sdf_invalidate || !world_sdf_update_one_mip;

    for (int i = clipmap.size() - 1; i >= 0; --i)
    {
      if (!(allow_update_mask & (1 << i)))
        continue;
      IPoint3 coord = clip_world_voxels_center_from_world_pos(i, pos);
      auto ret = updateMip(i, pos, coord, clip_voxel_size(i), cb, render_cb);
      if (ret != UpdateStatus::NotChanged)
        updated |= (1 << i);
      if (ret != UpdateStatus::NotChanged && !allMips)
        break;
    }
    if (updated)
      updateCBuffers();
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
    d3d::resource_barrier({world_sdf_clipmap.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
    return updated;
  }

  void update(const Point3 &pos, const request_instances_cb &cb, const request_prefetch_cb &rcb, const render_instances_cb &render_cb,
    uint32_t allow_update_mask) override
  {
    if (clipmap.empty())
      return;
    uint32_t mask = updateNoPrefetch(pos, cb, render_cb, allow_update_mask);
    if (cbuffersDirty)
      updateCBuffers();
    prefetchAll(pos, rcb, mask);
    updatedMask |= mask;
  }
  uint32_t getClipsReady() const override { return updatedMask; }
  void debugRender() override
  {
    TIME_D3D_PROFILE(world_sdf_render);
    TMatrix4 globtm;
    d3d::getglobtm(globtm);
    set_globtm_to_shader(globtm);
    if (!sdf_world_debug.getElem())
      sdf_world_debug.init("sdf_world_debug");
    sdf_world_debug.render();
  }
  int getClipsCount() const override { return clipmap.size(); }
  void getResolution(int &width, int &height) const override
  {
    width = w;
    height = d;
  }
  BBox3 getClipBoxAt(const Point3 &pos, uint32_t clip) const override
  {
    clip = min<uint32_t>(clip, clipmap.size() - 1);
    IPoint3 coord = clip_world_voxels_center_from_world_pos(clip, pos);
    const float voxelSize = clip_voxel_size(clip);
    IPoint3 worldCoordLT = coord - IPoint3(w / 2, w / 2, d / 2);
    Point3 lt = point3(worldCoordLT) * voxelSize;
    eastl::swap(lt.y, lt.z);
    return BBox3(lt, lt + Point3(w, d, w) * voxelSize);
  }
  BBox3 getClipBox(uint32_t clip) const override
  {
    clip = min<uint32_t>(clip, clipmap.size() - 1);
    Point3 lt = point3(clipmap[clip].worldCoordLT) * clipmap[clip].voxelSize;
    eastl::swap(lt.y, lt.z);
    return BBox3(lt, lt + Point3(w, d, w) * clipmap[clip].voxelSize);
  }
  float getVoxelSize(uint32_t clip) const override { return clip_voxel_size(clip); }
  uint16_t markedFrame = 0, removeFrame = 0;
  void removeFromDepth() override
  {
    const int markDispatchSize = world_sdf_depth_mark_pass.get() * temporalSpeed;
    if (!world_sdf_from_gbuf_remove_cs || markDispatchSize <= 0 || !supportUnorderedLoad)
      return;
    TIME_D3D_PROFILE(world_sdf_remove_from_gbuf_cs);
    ShaderGlobal::set_int(world_sdf_update_current_frameVarId, removeFrame++);
    d3d::set_rwtex(STAGE_CS, 0, world_sdf_clipmap.getVolTex(), 0, 0);
    world_sdf_from_gbuf_remove_cs->dispatchThreads(markDispatchSize, 1, 1);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  }
  void markFromGbuffer() override
  {
    const int markDispatchSize = world_sdf_depth_mark_pass.get() * temporalSpeed;
    if (!world_sdf_from_gbuf_cs || markDispatchSize <= 0)
      return;
    if (!supportUnorderedLoad) // currently it is not supported. Can be implemented via intermediary buffer.
      return;
    TIME_D3D_PROFILE(world_sdf_from_gbuf_cs);
    ShaderGlobal::set_int(world_sdf_update_current_frameVarId, markedFrame++);
    // ShaderGlobal::set_int(world_sdf_dispatch_sizeVarId, markDispatchSize);
    d3d::set_rwtex(STAGE_CS, 0, world_sdf_clipmap.getVolTex(), 0, 0);
    auto updateTsz = world_sdf_from_gbuf_cs->getThreadGroupSizes();
    world_sdf_from_gbuf_cs->dispatch((markDispatchSize + updateTsz[0] - 1) / updateTsz[0], 1, 1);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  }
  void setTemporalSpeed(float speed) { temporalSpeed = clamp<float>(speed, 0.f, 1.f); }
  void fullReset(bool invalidate_current) { initHistory(clipmap.size(), invalidate_current); }
};
WorldSDF *new_world_sdf() { return new WorldSDFImpl; }