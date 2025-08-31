//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/fixed_function.h>
#include <daSDF/update_sdf_request.h>

class Point3;
class BBox3;
struct bbox3f;
class BaseTexture;
class Sbuffer;
namespace resptr_detail
{
template <typename ResType>
class ManagedRes;
}
using ManagedBuf = resptr_detail::ManagedRes<Sbuffer>;
typedef eastl::fixed_function<64,
  UpdateSDFQualityStatus(const bbox3f *box, int box_cnt, UpdateSDFQualityRequest quality, int clip_no, uint32_t &instances_cnt)>
  request_instances_cb;

// true if requested
typedef eastl::fixed_function<64, bool(const bbox3f &box, int clip_no)> request_prefetch_cb;

// true if rendered
typedef eastl::fixed_function<64, bool(int clip_no, const BBox3 &box, float voxelSize, int max_instances)> render_instances_cb;

struct WorldSDF
{
  static float get_voxel_size(float voxel0Size, uint32_t clip);
  virtual ~WorldSDF();
  virtual void debugRender() = 0;
  virtual void update(const Point3 &pos, const request_instances_cb &cb, const request_prefetch_cb &rcb,
    const render_instances_cb &render_cb, uint32_t allow_update_mask = ~0u) = 0;
  virtual bool shouldUpdate(const Point3 &pos, uint32_t clip) const = 0;
  virtual void fixup_settings(uint16_t &w, uint16_t &d, uint8_t &c) const = 0;
  virtual void setValues(uint8_t clips, uint16_t width, uint16_t height, float voxel0_size) = 0;
  virtual uint32_t getRequiredTemporalBufferSize(uint16_t w, uint16_t d, uint8_t c) const = 0;
  virtual void setTemporalBuffer(const ManagedBuf &buf) = 0;
  virtual int getClipsCount() const = 0;
  virtual int getTexelMoveThresholdXZ() const = 0;  // Movement within this distance won't cause re-render
  virtual int getTexelMoveThresholdAlt() const = 0; // Movement within this distance won't cause re-render
  virtual void getResolution(int &width, int &height) const = 0;
  virtual float getVoxelSize(uint32_t clip) const = 0;
  virtual BBox3 getClipBoxAt(const Point3 &pos, uint32_t clip) const = 0;
  virtual BBox3 getClipBox(uint32_t clip) const = 0;
  virtual void markFromGbuffer() = 0;               // updates sdf from gbuffer
  virtual void removeFromDepth() = 0;               // removes sdf from depth (requires close_downsampled_depth)
  virtual void setTemporalSpeed(float speed) = 0;   // setTemporalSpeed sdf from gbuffer
  virtual int rasterizationVoxelAround() const = 0; // when using raseterization we can cover up to N voxels around triangle. without
                                                    // typedload it is 0
  virtual void fullReset(bool invalidate_current = true) = 0;
  virtual uint32_t getClipsReady() const = 0;
};

WorldSDF *new_world_sdf();