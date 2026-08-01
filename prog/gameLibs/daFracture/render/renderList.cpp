// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daFracture/render/renderList.h>
#include <daFracture/render/renderContext.h>

#include <util/dag_stlqsort.h>
#include <vecmath/dag_vecMath.h>
#include <memory/dag_framemem.h>


namespace frx
{

void MeshRenderList::add(const RenderMesh &mesh, const TMatrix &tm, const TMatrix &prev_tm)
{
  G_ASSERT_RETURN(curState == PrepareState::NONE, );
  if (mesh.elems.empty())
    return;
  const int baseInstId = insts.size();
  for (auto &elem : mesh.elems)
  {
    int instId = baseInstId;
    for (; instId < insts.size(); instId++)
      if (insts[instId].instData == elem.mat.instData)
        break;
    if (instId == insts.size())
    {
      RInst &inst = insts.push_back();
      inst.tm = tm;
      inst.prevTm = prev_tm;
      inst.initialPos = mesh.initialPos;
      inst.instData = elem.mat.instData;
    }
    RElem &re = elems.push_back();
    re.stage = elem.mat.stage;
    re.shElem = elem.mat.shElem.get();
    re.ib = re.vb = nullptr;
    re.vbOfs = elem.vbAlloc;
    re.ibOfs = elem.ibAlloc;
    re.vStride = elem.vStride;
    re.fCnt = elem.fCnt;
    re.instId = instId;
  }
}

void MeshRenderList::prepare()
{
  G_ASSERT_RETURN(curState == PrepareState::NONE, );

  {
    auto &ctx = get_render_ctx();
    ScopedLockRead lock(ctx.unifiedBufs.bufLock);
    // rendinst shaders require to keep original local vertex positions, because layered shaders use them for computing overlays
    // to achieve it, upload_mesh_batch offsets all vertices by difference between base rendinst pos and destr mesh initial pos
    // here this offset added to instance transforms to compensate for this
    for (RInst &inst : insts)
    {
      Point3 vertOfs;
      v_stu_p3(&vertOfs.x, v_sub(v_ldu_p3(&inst.initialPos.x), inst.instData.basePosAndHash));
      inst.tm.setcol(3, inst.tm.getcol(3) - inst.tm % vertOfs);
      inst.prevTm.setcol(3, inst.prevTm.getcol(3) - inst.prevTm % vertOfs);
    }
    for (RElem &elem : elems)
    {
      const auto vb = ctx.unifiedBufs.get(elem.vbOfs);
      const auto ib = ctx.unifiedBufs.get(elem.ibOfs);
      elem.vb = vb.sb;
      elem.vbOfs = int(vb.ofs);
      elem.ib = ib.sb;
      elem.ibOfs = int(ib.ofs);
      G_ASSERT(elem.vStride == vb.elemSz);
    }
  }

  stlsort::sort(elems.begin(), elems.end(), [&](RElem &a, RElem &b) {
    if (a.stage != b.stage)
      return a.stage < b.stage;
    if (uintptr_t(a.shElem) != uintptr_t(b.shElem))
      return uintptr_t(a.shElem) < uintptr_t(b.shElem);
    if (a.vb != b.vb)
      return uintptr_t(a.vb) < uintptr_t(b.vb);
    if (a.ib != b.ib)
      return uintptr_t(a.ib) < uintptr_t(b.ib);
    return a.instId < b.instId;
  });

  for (int stage = 1; stage < ShaderMesh::STG_COUNT; stage++)
  {
    const auto it = eastl::lower_bound(elems.begin(), elems.end(), stage, [&](const RElem &elem, int) { return elem.stage < stage; });
    stageSpans[stage - 1] = it - elems.begin();
  }

  curState = PrepareState::PREPARED;
}

void MeshRenderList::finalizeForRi()
{
  G_ASSERT_RETURN(curState == PrepareState::PREPARED, );

  FRAMEMEM_REGION;
  dag::Vector<vec4f, framemem_allocator> transforms;
  transforms.resize(insts.size() * 4);
  vec4f *transformBufDst = transforms.data();
  for (const RInst &inst : insts)
  {
    mat44f m;
    v_mat44_make_from_43cu(m, inst.tm.array);
    v_mat44_transpose_to_mat43(*reinterpret_cast<mat43f *>(transformBufDst), m);
    // .xyw - initial pos, .z - hash
    transformBufDst[3] = v_perm_xycw(v_perm_xyzz(inst.instData.basePosAndHash), v_perm_wwww(inst.instData.basePosAndHash));
    transformBufDst += 4;
  }

  if (!transforms.empty())
  {
    if (!perFrameDataBuf || perFrameDataBuf->getSize() < data_size(transforms))
    {
      uint32_t capacity = eastl::max<uint32_t>(perFrameDataBuf ? perFrameDataBuf->getSize() * 2 : 0, data_size(transforms));
      perFrameDataBuf = dag::create_sbuffer(sizeof(vec4f), capacity / sizeof(vec4f),
        SBCF_DYNAMIC | SBCF_FRAMEMEM | SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES, TEXFMT_A32B32G32R32F, nullptr, RESTAG_RINGDYNBUF);
    }
    perFrameDataBuf->updateData(0, data_size(transforms), transforms.data(), VBLOCK_DISCARD);
  }
  curState = PrepareState::RI;
}

void MeshRenderList::clear()
{
  curState = PrepareState::NONE;
  elems.clear();
  insts.clear();
}

} // namespace frx
