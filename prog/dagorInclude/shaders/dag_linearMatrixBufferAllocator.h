//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_linearSbufferAllocator.h>

#include <math/dag_TMatrix.h>
#include <generic/dag_tab.h>


// A specialized SBuffer heap manager, which also has a CPU copy for mapping (transformations).
// The CPU copy uses TMatrix, the GPU one is packed as 3*float4 matrices, using float4 buffer elements in a StructuredBuffer.
struct MatrixBufferHeapManager
{
public:
  static constexpr uint32_t ELEM_SIZE = sizeof(float) * 4;
  static constexpr uint32_t MTX_SIZE = ELEM_SIZE * 3;

  struct Heap
  {
    UniqueBuf buffer;
    Tab<TMatrix> bindposeArr;
  };

  MatrixBufferHeapManager(const char *name) :
    sbufferHeapManager(name, ELEM_SIZE, SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED | SBCF_MAYBELOST, 0)
  {}

  void copy(Heap &to, size_t to_offset, const Heap &from, size_t from_offset, size_t len)
  {
    G_ASSERT(from_offset + len <= from.bindposeArr.size());
    G_ASSERT(to_offset + len <= to.bindposeArr.size());
    mem_copy_to(make_span_const(from.bindposeArr.data() + from_offset, len), to.bindposeArr.data() + to_offset);
    sbufferHeapManager.copy(to.buffer, to_offset * MTX_SIZE, from.buffer, from_offset * MTX_SIZE, len * MTX_SIZE);
  }
  bool canCopyInSameHeap() const { return sbufferHeapManager.canCopyInSameHeap(); }
  bool canCopyOverlapped() const { return sbufferHeapManager.canCopyOverlapped(); }
  bool create(Heap &h, size_t sz)
  {
    h.bindposeArr.resize(sz); // no clearing, keep data, so it can be copied over
    return sbufferHeapManager.create(h.buffer, sz * MTX_SIZE);
  }
  void orphan(Heap &h)
  {
    clear_and_shrink(h.bindposeArr);
    sbufferHeapManager.orphan(h.buffer);
  }
  const char *getName() const { return sbufferHeapManager.getName(); }

protected:
  SbufferHeapManager sbufferHeapManager;
};

template <class T>
struct LinearHeapAllocator;
typedef LinearHeapAllocator<MatrixBufferHeapManager> LinearHeapAllocatorMatrixBuffer;
