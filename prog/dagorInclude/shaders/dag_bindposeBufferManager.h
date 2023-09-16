//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_linearMatrixBufferAllocator.h>
#include <memory/dag_linearHeapAllocator.h>

#include <generic/dag_DObject.h>
#include <generic/dag_ptrTab.h>


class BindPoseBufferManager;

class BindPoseElem : public DObject
{
  friend BindPoseBufferManager;

public:
  using Region = LinearHeapAllocatorMatrixBuffer::Region;
  using RegionId = LinearHeapAllocatorMatrixBuffer::RegionId;

  BindPoseElem(BindPoseBufferManager &manager, RegionId region_id);
  BindPoseElem(const BindPoseElem &) = delete;
  ~BindPoseElem();

  int getBufferOffset() const { return cachedRegionOffset; }

  bool checkBindPoseArr(dag::ConstSpan<TMatrix> target_bind_pose_arr) const;

protected:
  RegionId regionId;
  int cachedRegionOffset = 0; // must be updated after defrag (not used now)
  BindPoseBufferManager *manager;
};


class BindPoseBufferManager
{
public:
  using Region = LinearHeapAllocatorMatrixBuffer::Region;
  using RegionId = LinearHeapAllocatorMatrixBuffer::RegionId;

  BindPoseBufferManager();

  Ptr<BindPoseElem> createOrGetBindposeElem(dag::ConstSpan<TMatrix> target_bind_pose_arr);

  void closeElem(RegionId region_id);

  Region getRegion(RegionId region_id) const;

  bool checkBindPoseArr(RegionId region_id, dag::ConstSpan<TMatrix> target_bind_pose_arr) const;


protected:
  void resizeIncrement(size_t min_size_increment);

  LinearHeapAllocatorMatrixBuffer bindposeAllocator;
  PtrTab<BindPoseElem> bindposeElemArr;
};
