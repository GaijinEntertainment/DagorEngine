// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_bindposeBufferManager.h>
#include <vecmath/dag_vecMath.h>

#include <3d/dag_lockSbuffer.h>

#include <util/dag_convar.h>

CONSOLE_BOOL_VAL("bindpose", log_unique_bindpose_changes, false);


static bool compareBindPoseArrays(dag::ConstSpan<TMatrix> a, dag::ConstSpan<TMatrix> b)
{
  if (a.size() != b.size())
    return false;
  for (int i = 0; i < a.size(); ++i)
    if (a[i] != b[i])
      return false;
  return true;
}

static bool updateBindPoseBuffer(Sbuffer &sbuffer, dag::ConstSpan<TMatrix> bind_pose_arr, uint32_t mtx_offset)
{
  int boneCount = bind_pose_arr.size();
  G_ASSERT_RETURN(boneCount > 0, false);

  const TMatrix *boneOrgTms = bind_pose_arr.data();
  uint32_t ofs_bytes = mtx_offset * MatrixBufferHeapManager::MTX_SIZE;
  uint32_t size_bytes = boneCount * MatrixBufferHeapManager::MTX_SIZE;

  Tab<vec4f> tmpArr(framemem_ptr());
  tmpArr.resize(boneCount * 3);
  vec4f *__restrict data = (vec4f *__restrict)tmpArr.data();
  for (int i = 0; i < boneCount; i++)
  {
    mat44f result44, resultInv44;
    mat43f result43;
    v_mat44_make_from_43cu_unsafe(result44, boneOrgTms[i][0]);
    v_mat44_inverse43(resultInv44, result44);
    v_mat44_transpose_to_mat43(result43, resultInv44);
    *(data++) = result43.row0;
    *(data++) = result43.row1;
    *(data++) = result43.row2;
  }
  if (sbuffer.updateData(ofs_bytes, size_bytes, tmpArr.data(), VBLOCK_WRITEONLY))
    return true;

  logerr("Failed to update bindpose buffer!");
  return false;
}


BindPoseElem::BindPoseElem(BindPoseBufferManager &manager, RegionId region_id) : manager(&manager), regionId(region_id)
{
  cachedRegionOffset = manager.getRegion(region_id).offset;
}

BindPoseElem::~BindPoseElem()
{
  G_ASSERT(regionId);
  manager->closeElem(regionId);
}


bool BindPoseElem::checkBindPoseArr(dag::ConstSpan<TMatrix> target_bind_pose_arr) const
{
  return manager->checkBindPoseArr(regionId, target_bind_pose_arr);
}


BindPoseBufferManager::BindPoseBufferManager() : bindposeAllocator(MatrixBufferHeapManager("bindpose_arr_heap_mgr_")) {}


void BindPoseBufferManager::resizeIncrement(size_t min_size_increment)
{
  int sizeIncrement = bindposeAllocator.getHeapSize() / 4; // +25% capcacity
  int newCapacity = bindposeAllocator.getHeapSize() + max<int>(min_size_increment, sizeIncrement);
  bindposeAllocator.resize(newCapacity);

  LinearHeapAllocatorMatrixBuffer::Heap &heap = bindposeAllocator.getHeap();
  for (int i = 0, len = bindposeElemArr.size(); i < len; ++i)
  {
    Region region = bindposeAllocator.get(bindposeElemArr[i]->regionId);
    dag::ConstSpan<TMatrix> slice = make_span_const(heap.bindposeArr.data() + region.offset, region.size);
    updateBindPoseBuffer(*heap.buffer.getBuf(), slice, region.offset);
  }
}

Ptr<BindPoseElem> BindPoseBufferManager::createOrGetBindposeElem(dag::ConstSpan<TMatrix> target_bind_pose_arr)
{
  for (int i = 0, len = bindposeElemArr.size(); i < len; ++i)
  {
    Ptr<BindPoseElem> &bindPoseElem = bindposeElemArr[i];
    RegionId regionId = bindPoseElem->regionId;
    G_ASSERT(regionId);
    if (checkBindPoseArr(regionId, target_bind_pose_arr))
      return bindPoseElem;
  }

  if (!bindposeAllocator.canAllocate(target_bind_pose_arr.size()))
  {
    resizeIncrement(target_bind_pose_arr.size());
    ShaderGlobal::set_buffer(::get_shader_variable_id("bind_pose_tr_buffer", false), bindposeAllocator.getHeap().buffer);
  }
  RegionId regionId = bindposeAllocator.allocateInHeap(target_bind_pose_arr.size());
  G_ASSERT(regionId);

  LinearHeapAllocatorMatrixBuffer::Heap &heap = bindposeAllocator.getHeap();
  Region region = bindposeAllocator.get(regionId);

  G_ASSERT(region.offset + target_bind_pose_arr.size() <= heap.bindposeArr.size());
  mem_copy_to(target_bind_pose_arr, heap.bindposeArr.data() + region.offset);

  updateBindPoseBuffer(*heap.buffer.getBuf(), target_bind_pose_arr, region.offset);

  Ptr<BindPoseElem> bindPoseElem = new BindPoseElem(*this, regionId);

  bindposeElemArr.push_back(bindPoseElem);
  bindPoseElem.delRef(); // bindposeElemArr has basically weak pointers

  if (log_unique_bindpose_changes)
    debug("Unique bindpose inserted: %s -> %d", region.offset, region.size);

  return bindPoseElem;
}

void BindPoseBufferManager::closeElem(RegionId region_id)
{
  G_ASSERT_RETURN(region_id, );

  auto it = eastl::find_if(bindposeElemArr.begin(), bindposeElemArr.end(),
    [region_id](const Ptr<BindPoseElem> &ptr) { return ptr->regionId == region_id; });
  G_ASSERT_RETURN(it != bindposeElemArr.end(), );

  const Region region = bindposeAllocator.get(region_id);

  if (log_unique_bindpose_changes)
    debug("Unique bindpose removed: %s -> %d", region.offset, region.size);

  bindposeAllocator.free(region_id);
  erase_items(bindposeElemArr, it - bindposeElemArr.begin(), 1);

  if (bindposeElemArr.size() == 0)
    bindposeAllocator.resize(0); // to release buffer
}

BindPoseBufferManager::Region BindPoseBufferManager::getRegion(RegionId region_id) const
{
  G_ASSERT_RETURN(region_id, Region());
  return bindposeAllocator.get(region_id);
}

bool BindPoseBufferManager::checkBindPoseArr(RegionId region_id, dag::ConstSpan<TMatrix> target_bind_pose_arr) const
{
  G_ASSERT_RETURN(region_id, false);
  Region region = bindposeAllocator.get(region_id);
  dag::ConstSpan<TMatrix> curBindposeArr =
    make_span_const(bindposeAllocator.getHeap().bindposeArr.data() + region.offset, region.size);
  return compareBindPoseArrays(curBindposeArr, target_bind_pose_arr);
}
