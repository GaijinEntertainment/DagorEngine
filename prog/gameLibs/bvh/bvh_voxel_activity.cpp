// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <drv/3d/dag_texFlags.h>
#include <drv/3d/dag_buffers.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_mathBase.h>
#include <debug/dag_debug.h>

#include "bvh_context.h"
#include "bvh_voxel_activity.h"

namespace bvh::voxel_activity
{

// debug freeze: fill, decay, scroll and the ray shaders' marks all pause
bool freeze = false;

// Debug kill switch for the placement culling; the volume keeps measuring either way. The
// placement jobs never read it directly, only the copy update() takes into the cpu snapshot.
bool cull = true;

// Every shader built on the bvh tracing macros references the volume UAV, so a valid texture
// must stay bound while no context has the feature on; consumers gate on grid_size.w == 0.
static UniqueTex dummy;

static eastl::unique_ptr<ComputeShaderElement> fill_shader;
static eastl::unique_ptr<ComputeShaderElement> decay_shader;
static eastl::unique_ptr<ComputeShaderElement> scroll_shader;
static bool shaders_failed = false;

static ShaderVariableInfo bvh_voxel_activityVarId("bvh_voxel_activity", true);
static ShaderVariableInfo bvh_voxel_activity_targetVarId("bvh_voxel_activity_target", true);
static ShaderVariableInfo bvh_voxel_activity_sourceVarId("bvh_voxel_activity_source", true);
static ShaderVariableInfo bvh_voxel_activity_grid_originVarId("bvh_voxel_activity_grid_origin", true);
static ShaderVariableInfo bvh_voxel_activity_grid_sizeVarId("bvh_voxel_activity_grid_size", true);
static ShaderVariableInfo bvh_voxel_activity_scroll_deltaVarId("bvh_voxel_activity_scroll_delta", true);
static ShaderVariableInfo bvh_voxel_activity_readbackVarId("bvh_voxel_activity_readback", true);
static ShaderVariableInfo bvh_voxel_activity_readback_onVarId("bvh_voxel_activity_readback_on", true);

// When the ring has no free target, the decay kernel is told to skip its readback stores (out
// of bounds UAV stores are only defined on DX); this 1-element stand-in keeps the binding valid.
static UniqueBuf readback_sink;

static void disable_bind()
{
  bvh_voxel_activity_grid_sizeVarId.set_int4(0, 0, 0, 0);
  bvh_voxel_activityVarId.set_texture(dummy.getTexId());
}

void init()
{
  if (!dummy)
    dummy = dag::create_voltex(1, 1, 1, TEXFMT_R8UI | TEXCF_UNORDERED, 1, "bvh_voxel_activity_dummy");
  if (!readback_sink)
    readback_sink = dag::buffers::create_ua_sr_structured(sizeof(uint32_t), 1, "bvh_voxel_activity_readback_sink");
  disable_bind();
}

void teardown()
{
  fill_shader.reset();
  decay_shader.reset();
  scroll_shader.reset();
  shaders_failed = false;
  bvh_voxel_activityVarId.set_texture(BAD_TEXTUREID);
  bvh_voxel_activity_readbackVarId.set_buffer(BAD_D3DRESID);
  dummy.close();
  readback_sink.close();
}

static bool ensure_shaders()
{
  if (shaders_failed)
    return false;
  if (!fill_shader)
    fill_shader.reset(new_compute_shader("bvh_voxel_activity_fill"));
  if (!decay_shader)
    decay_shader.reset(new_compute_shader("bvh_voxel_activity_decay"));
  if (!scroll_shader)
    scroll_shader.reset(new_compute_shader("bvh_voxel_activity_scroll"));
  shaders_failed = !fill_shader || !decay_shader || !scroll_shader;
  if (shaders_failed)
    logerr("[BVH] voxel activity shaders are missing, the feature stays off");
  return !shaders_failed;
}

static void close_textures(ContextId context_id)
{
  auto &va = context_id->voxelActivity;
  va.tex[0].close();
  va.tex[1].close();
  va.needsFill = true;
  va.readback.close();
  va.cpu.ages.clear();
  va.cpu.valid = false;
}

static void reset_counters(dag::AtomicInteger<uint32_t> &considered, dag::AtomicInteger<uint32_t> &culled)
{
  considered.store(0);
  culled.store(0);
}

void begin_ri_gen_placement(ContextId id) { reset_counters(id->voxelActivity.riGenConsidered, id->voxelActivity.riGenCulled); }
void begin_ri_extra_placement(ContextId id) { reset_counters(id->voxelActivity.riExConsidered, id->voxelActivity.riExCulled); }
void begin_dynrend_placement(ContextId id) { reset_counters(id->voxelActivity.dynConsidered, id->voxelActivity.dynCulled); }

static void consume_readback(Context::VoxelActivity &va)
{
  int stride;
  uint32_t frame;
  if (uint8_t *data = va.readback.lock(stride, frame, true))
  {
    const int total = va.dims.x * va.dims.y * va.dims.z;
    va.cpu.ages.resize(total);
    memcpy(va.cpu.ages.data(), data, total);
    va.readback.unlock();
    const IPoint3 &origin = va.pendingOrigin[frame % va.readbackDepth];
    va.cpu.originVoxel = v_make_vec4i(origin.x, origin.y, origin.z, 0);
    va.cpu.dimsMinus1 = v_make_vec4i(va.dims.x - 1, va.dims.y - 1, va.dims.z - 1, 0x7FFFFFFF);
    va.cpu.rowStride = va.dims.x;
    va.cpu.sliceStride = va.dims.x * va.dims.y;
    va.cpu.invVoxelSize = v_splats(1.f / va.voxelSize);
    va.cpu.valid = true;
  }
}

static IPoint3 origin_voxel_for(const Context::VoxelActivity &va, const Point3 &camera_pos)
{
  auto floorDiv = [&](float v) { return int(floorf(v / va.voxelSize)); };
  return IPoint3(floorDiv(camera_pos.x), floorDiv(camera_pos.y), floorDiv(camera_pos.z)) - va.dims / 2;
}

static Point3 grid_origin_rel_for(const Context::VoxelActivity &va, const Point3 &camera_pos)
{
  return Point3(va.originVoxel.x, va.originVoxel.y, va.originVoxel.z) * va.voxelSize - camera_pos;
}

void update(ContextId context_id, const Point3 &camera_pos)
{
  auto &va = context_id->voxelActivity;
  if (!va.activeValue)
    return;

  CHECK_LOST_DEVICE_STATE();

  if (!ensure_shaders())
  {
    va.activeValue = 0;
    close_textures(context_id);
    disable_bind();
    return;
  }

  TIME_D3D_PROFILE(bvh_voxel_activity);

  // this frame's placement jobs are already waited on by build(); the next frame's jobs see
  // the snapshot writes below through their spawn
  va.statRiGenConsidered = va.riGenConsidered.load();
  va.statRiGenCulled = va.riGenCulled.load();
  va.statRiExConsidered = va.riExConsidered.load();
  va.statRiExCulled = va.riExCulled.load();
  va.statDynConsidered = va.dynConsidered.load();
  va.statDynCulled = va.dynCulled.load();
  va.cpu.frameSalt = va.cpu.frameSalt * 1664525u + 1013904223u; // one reshuffle of the keep set per frame
  va.cpu.cull = cull;

  consume_readback(va);

  if (freeze && va.tex[0] && va.tex[1])
  {
    // The grid is world anchored while the camera keeps moving, so the camera relative origin
    // must follow every build. The flush covers marks stored before the freeze flipped.
    d3d::resource_barrier({va.tex[va.current].getBaseTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_ALL_SHADERS | RB_STAGE_ALL_SHADERS, 0, 0});
    va.gridOriginRel = grid_origin_rel_for(va, camera_pos);
    return;
  }

  if (!va.tex[0] || !va.tex[1])
  {
    for (int i = 0; i < 2; ++i)
      va.tex[i] = dag::create_voltex(va.dims.x, va.dims.y, va.dims.z, TEXFMT_R8UI | TEXCF_UNORDERED, 1,
        ccn(context_id, i ? "voxel_activity_1" : "voxel_activity_0"));
    HANDLE_LOST_DEVICE_STATE(va.tex[0], );
    HANDLE_LOST_DEVICE_STATE(va.tex[1], );
    va.current = 0;
    va.needsFill = true;

    va.readback.close();
    va.readback.init(sizeof(uint32_t), va.dims.x * va.dims.y * va.dims.z / 4, va.readbackDepth,
      ccn(context_id, "voxel_activity_readback"), SBCF_UA_STRUCTURED_READBACK, 0, false);
  }

  const IPoint3 newOrigin = origin_voxel_for(va, camera_pos);

  bvh_voxel_activity_grid_sizeVarId.set_int4(va.dims.x, va.dims.y, va.dims.z, va.activeValue);

  // the last frame's marks must land before the kernels below read or rewrite the texels
  d3d::resource_barrier({va.tex[va.current].getBaseTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_ALL_SHADERS | RB_STAGE_COMPUTE, 0, 0});

  if (va.needsFill)
  {
    va.originVoxel = newOrigin;
    va.needsFill = false;
    bvh_voxel_activity_targetVarId.set_texture(va.tex[va.current].getTexId());
    fill_shader->dispatchThreads(va.dims.x, va.dims.y, va.dims.z);
    d3d::resource_barrier({va.tex[va.current].getBaseTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
  }
  else if (newOrigin != va.originVoxel)
  {
    const IPoint3 delta = newOrigin - va.originVoxel;
    bvh_voxel_activity_scroll_deltaVarId.set_int4(delta.x, delta.y, delta.z, 0);
    bvh_voxel_activity_sourceVarId.set_texture(va.tex[va.current].getTexId());
    bvh_voxel_activity_targetVarId.set_texture(va.tex[va.current ^ 1].getTexId());
    scroll_shader->dispatchThreads(va.dims.x, va.dims.y, va.dims.z);
    va.current ^= 1;
    va.originVoxel = newOrigin;
    d3d::resource_barrier({va.tex[va.current].getBaseTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
  }

  uint32_t readbackFrame = 0;
  Sbuffer *readbackTarget = (Sbuffer *)va.readback.getNewTarget(readbackFrame);

  bvh_voxel_activity_targetVarId.set_texture(va.tex[va.current].getTexId());
  bvh_voxel_activity_readbackVarId.set_buffer(readbackTarget ? readbackTarget : readback_sink.getBuf());
  bvh_voxel_activity_readback_onVarId.set_int(readbackTarget ? 1 : 0);
  decay_shader->dispatchThreads(va.dims.x / 4, va.dims.y, va.dims.z);
  d3d::resource_barrier({va.tex[va.current].getBaseTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_ALL_SHADERS, 0, 0});

  if (readbackTarget)
  {
    va.pendingOrigin[readbackFrame % va.readbackDepth] = va.originVoxel;
    va.readback.startCPUCopy();
  }

  va.gridOriginRel = grid_origin_rel_for(va, camera_pos);
}

void bind(ContextId context_id)
{
  const auto &va = context_id->voxelActivity;
  if (!va.activeValue || !va.tex[va.current])
  {
    disable_bind();
    return;
  }
  // the freeze pauses marking through the same w == 0 signal the off state uses
  bvh_voxel_activity_grid_sizeVarId.set_int4(va.dims.x, va.dims.y, va.dims.z, freeze ? 0 : va.activeValue);
  bvh_voxel_activity_grid_originVarId.set_float4(va.gridOriginRel.x, va.gridOriginRel.y, va.gridOriginRel.z, 1.f / va.voxelSize);
  bvh_voxel_activityVarId.set_texture(va.tex[va.current].getTexId());
}

// Dropping the volume, the ring and the snapshot together restarts fully dead, and a stale
// in-flight readback of the unloaded world cannot resurface.
void on_unload_scene(ContextId context_id) { close_textures(context_id); }

void remove(ContextId context_id)
{
  close_textures(context_id);
  context_id->voxelActivity.activeValue = 0;
  disable_bind();
}

} // namespace bvh::voxel_activity

namespace bvh
{

void set_voxel_activity(ContextId context_id, const VoxelActivitySettings &settings)
{
  if (context_id == InvalidContextId)
    return;

  const bool valid = settings.voxelSize > 0.f && settings.activeValue > 0 && settings.activeValue < 256 && settings.dims.x > 0 &&
                     settings.dims.y > 0 && settings.dims.z > 0 && settings.inactiveKeepFraction >= 0.f &&
                     settings.inactiveKeepFraction <= 1.f;
  if (!valid)
  {
    logerr("[BVH] set_voxel_activity: invalid settings: voxel size %f, dims %dx%dx%d, active value %d, keep fraction %f",
      settings.voxelSize, settings.dims.x, settings.dims.y, settings.dims.z, settings.activeValue, settings.inactiveKeepFraction);
    return;
  }

  auto &va = context_id->voxelActivity;
  // only x needs the multiple of 4: the decay kernel packs four voxels along x (256 is one)
  constexpr int maxDim = 256;
  const IPoint3 dims(min((settings.dims.x + 3) & ~3, maxDim), min(settings.dims.y, maxDim), min(settings.dims.z, maxDim));
  // An identical reconfigure (settings changes resend the same values) must not drop the
  // measured state; a new voxel size remaps the grid, so the old snapshot and readbacks go too.
  if (va.dims != dims || va.voxelSize != settings.voxelSize)
  {
    voxel_activity::close_textures(context_id);
    va.dims = dims;
    va.voxelSize = settings.voxelSize;
  }
  va.activeValue = settings.activeValue;
  va.cpu.keepThreshold = uint32_t(settings.inactiveKeepFraction * 65536.f + 0.5f);

  logdbg("[BVH] voxel activity for %s: %dx%dx%d voxels of %.1fm, active value %d, dead voxel keep fraction %.2f",
    context_id->name.c_str(), va.dims.x, va.dims.y, va.dims.z, va.voxelSize, va.activeValue, settings.inactiveKeepFraction);
}

void remove_voxel_activity(ContextId context_id)
{
  if (context_id == InvalidContextId)
    return;
  voxel_activity::remove(context_id);
}

} // namespace bvh
