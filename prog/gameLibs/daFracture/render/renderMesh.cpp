// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daFracture/render/renderMesh.h>

#include <ska_hash_map/flat_hash_map2.hpp>
#include <util/dag_hash.h>
#include <util/dag_stlqsort.h>
#include <memory/dag_framemem.h>
#include <vecmath/dag_vecMath.h>

#include <daFracture/render/unitedGeomBuffers.h>
#include <daFracture/render/renderContext.h>

#include "vertexDataPacking.h"


namespace frx
{

RenderMesh::~RenderMesh()
{
  GlobalRenderContext &ctx = get_render_ctx();
  for (RElem &elem : elems)
  {
    ctx.unifiedBufs.free(elem.vbAlloc);
    ctx.unifiedBufs.free(elem.ibAlloc);
  }
}

void add_destr_geometry(DestrContext &ctx, dag::Span<ShaderGeomLoadRequest> requests)
{
  TIME_PROFILE(add_destr_geometry)
  FRAMEMEM_REGION;

  struct RawMeshDataSpan
  {
    int vOfs = 0, vSz = 0;
    int iOfs = 0, iSz = 0;
  };
  eastl::vector_map<const GlobalVertexData *, RawMeshDataSpan> rawMeshData;

  dag::Vector<uint8_t, framemem_allocator> rawMeshDataBuffer;
  for (ShaderGeomLoadRequest &req : requests)
  {
    for (const ShaderMesh::RElem &srcElem : req.shaderMesh->getAllElems())
      rawMeshData.emplace(srcElem.vertexData, RawMeshDataSpan());
    req.smvd->unpackVData([&](ShaderMatVDataReadCbSrc &reader, const GlobalVertexData *vd, bool is_ib) {
      const auto it = rawMeshData.find(vd);
      if (it == rawMeshData.end())
        return;
      RawMeshDataSpan &span = it->second;
      int &offs = is_ib ? span.iOfs : span.vOfs;
      int &sz = is_ib ? span.iSz : span.vSz;
      G_ASSERT_RETURN(sz == 0, );
      offs = rawMeshDataBuffer.size();
      sz = reader.getReadSize();
      rawMeshDataBuffer.resize(offs + sz);
      reader.read(rawMeshDataBuffer.data() + offs);
    });
  }

  for (ShaderGeomLoadRequest &req : requests)
  {
    mat44f pieceTm44, pieceTmInv44, tm44, toLocal44;
    v_mat44_make_from_43cu(pieceTm44, req.outMesh->tm.array);
    v_mat44_make_from_43cu(tm44, req.tm.array);
    v_mat44_inverse43(pieceTmInv44, pieceTm44);
    v_mat44_mul43(toLocal44, pieceTmInv44, tm44);

    for (int elemStage = 0; elemStage < ShaderMesh::STG_COUNT; elemStage++)
    {
      for (const ShaderMesh::RElem &srcElem : req.shaderMesh->getElems(elemStage))
      {
        const uint16_t materialId = add_render_material(ctx, srcElem.mat,
          {.shElem = srcElem.e, .stage = ShaderMesh::Stage(elemStage), .instData = req.instData});
        const auto &rMat = ctx.renderMats[materialId];
        const uint32_t vStride = rMat.channels.vStride;
        G_ASSERT_CONTINUE(srcElem.si >= 0);

        GlobalVertexData *vdata = srcElem.vertexData;
        const int indexSize = vdata->getIbElemSz();
        G_ASSERT_CONTINUE(indexSize == 2);

        G_ASSERT_CONTINUE(rawMeshData.find(vdata) != rawMeshData.end());
        const RawMeshDataSpan &span = rawMeshData[vdata];
        G_LOGERR_AND_DO(span.iSz, continue, "span.iSz not found");
        G_LOGERR_AND_DO(span.vSz, continue, "span.vSz not found");

        const auto vdIOfs = srcElem.vertexData->getIOffs();
        const auto vdVOfs = srcElem.vertexData->getVOffs();
        const uint8_t *indSrcPtr = rawMeshDataBuffer.data() + span.iOfs;
        const uint8_t *vertSrcPtr = rawMeshDataBuffer.data() + span.vOfs;
        vertSrcPtr += (srcElem.baseVertex - vdVOfs + srcElem.sv) * vStride;
        indSrcPtr += (srcElem.si - vdIOfs) * sizeof(uint16_t);

        uint32_t pushVertBase = req.outMesh->verts.size();
        for (int i = 0; i < srcElem.numf * 3; i += 3)
        {
          auto &face = req.outMesh->faces.push_back();
          face.mat = materialId;
          for (int j = 0; j < 3; j++)
          {
            uint32_t idx = ((uint16_t *)indSrcPtr)[i + j] - srcElem.sv;
            G_ASSERT(idx < srcElem.numv);
            face.idx[j] = idx + pushVertBase;
          }
        }

        const auto &channels = rMat.channels;
        for (int i = 0; i < srcElem.numv; i++)
        {
          auto &v = req.outMesh->verts.emplace_back(parse_vertex(channels, vertSrcPtr + i * vStride));
          v_stu_p3(&v.pos.x, v_mat44_mul_vec3p(toLocal44, v_ldu(&v.pos.x)));
          v_stu_p3(&v.norm.x, v_mat44_mul_vec3v(toLocal44, v_ldu(&v.norm.x)));
        }
      }
    }
  }
}


void upload_mesh_batch(const DestrContext &ctx, dag::Span<MeshUploadRequest> meshes)
{
  TIME_PROFILE(upload_mesh_batch);
  FRAMEMEM_REGION;

  uint32_t maxFacesPerMesh = 0, maxVertsPerMesh = 0;
  for (const MeshUploadRequest &rec : meshes)
  {
    maxFacesPerMesh = eastl::max(maxFacesPerMesh, rec.srcGeom->faces.size());
    maxVertsPerMesh = eastl::max(maxVertsPerMesh, rec.srcGeom->verts.size());
  }

  struct RElemUpload
  {
    RenderMesh *mesh = nullptr;
    dag::Vector<uint8_t, framemem_allocator> vData;
    dag::Vector<uint16_t, framemem_allocator> iData;
    uint16_t matId = 0;
  };
  dag::Vector<RElemUpload, framemem_allocator> elems;
  elems.reserve(meshes.size() * 4); // estimate

  dag::Vector<dag::Vector<DestrMesh::Face, framemem_allocator>, framemem_allocator> matToFaces(ctx.materials.size());
  ska::flat_hash_map<uint32_t, int32_t, eastl::hash<uint32_t>, eastl::equal_to<uint32_t>, framemem_allocator> toLocalIdx;
  toLocalIdx.reserve(maxFacesPerMesh * 3);
  dag::Vector<uint8_t, framemem_allocator> vDataTmp;
  dag::Vector<uint16_t, framemem_allocator> iDataTmp;
  iDataTmp.reserve(maxFacesPerMesh * 3);
  vDataTmp.reserve(maxVertsPerMesh * 24); // estimate

  for (MeshUploadRequest &rec : meshes)
  {
    rec.dstMesh->initialPos = rec.srcGeom->tm.getcol(3);
    float bSphereRadSq = sqr(rec.dstMesh->bSphereRad);

    const DestrMesh::Vertex *__restrict verts = rec.srcGeom->verts.data();
    for (auto &faces : matToFaces)
      faces.clear();
    for (const auto &face : rec.srcGeom->faces)
      matToFaces[face.mat].push_back(face);

    for (uint16_t matId = 0; matId != matToFaces.size(); matId++)
    {
      const auto &faces = matToFaces[matId];
      const auto &rMat = ctx.renderMats[matId];
      // required for keeping original local vertex positions, see MeshRenderList::prepare() for more info
      Point3 vertOfs;
      v_stu_p3(&vertOfs.x, v_sub(v_ldu_p3(&rec.dstMesh->initialPos.x), rMat.elem.instData.basePosAndHash));

      uint32_t i = 0;
      while (i != faces.size())
      {
        uint16_t nextLocalIdx = 0;
        toLocalIdx.clear();
        vDataTmp.clear();
        iDataTmp.clear();

        for (; i != faces.size(); i++)
        {
          for (uint32_t idx : faces[i].idx)
          {
            const auto [it, emplaced] = toLocalIdx.emplace(idx, nextLocalIdx);
            if (emplaced)
            {
              G_ASSERT(nextLocalIdx * rMat.channels.vStride == vDataTmp.size());
              append_items(vDataTmp, rMat.channels.vStride);
              DestrMesh::Vertex vert = verts[idx];
              bSphereRadSq = max(lengthSq(vert.pos), bSphereRadSq);
              vert.pos += vertOfs;
              pack_vertex(rMat.channels, vert, vDataTmp.end() - rMat.channels.vStride);
              nextLocalIdx++;
            }
            iDataTmp.emplace_back(it->second);
          }
          if (nextLocalIdx >= (((0xFFFFu - 3u) / 3u) * 3u)) // overflow, need write another elem
          {
            i++;
            break;
          }
        }

        // push elem
        RElemUpload &elem = elems.push_back();
        elem.mesh = rec.dstMesh;
        elem.vData = vDataTmp;
        elem.iData = iDataTmp;
        elem.matId = matId;
      }
    }

    rec.dstMesh->bSphereRad = sqrtf(bSphereRadSq);
  }

  dag::Vector<UnifiedGeomBuffers::AllocRequest, framemem_allocator> allocs;
  allocs.resize(elems.size() * 2);
  for (int i = 0; i < elems.size(); i++)
  {
    auto &elem = elems[i];
    auto &vb = allocs[i * 2];
    vb.bufProp.type = UnifiedGeomBuffers::BufferType::VB;
    vb.bufProp.elemSize = ctx.renderMats[elem.matId].channels.vStride;
    vb.data = (const uint8_t *)elem.vData.data();
    vb.elemCnt = elem.vData.size() / vb.bufProp.elemSize;
    auto &ib = allocs[i * 2 + 1];
    ib.bufProp.type = UnifiedGeomBuffers::BufferType::IB;
    ib.bufProp.elemSize = sizeof(uint16_t);
    ib.data = (const uint8_t *)elem.iData.data();
    ib.elemCnt = elem.iData.size();
  }

  {
    auto &rctx = get_render_ctx();
    rctx.unifiedBufs.allocate(make_span(allocs));
  }

  for (int i = 0; i < elems.size(); i++)
  {
    auto &e = elems[i];
    auto &material = ctx.renderMats[e.matId];
    RenderMesh::RElem &elem = e.mesh->elems.push_back();
    elem.mat = material.elem;

    auto &vb = allocs[i * 2];
    auto &ib = allocs[i * 2 + 1];
    elem.vStride = material.channels.vStride;
    elem.fCnt = ib.elemCnt / 3u;
    elem.ibAlloc = ib.allocId;
    elem.vbAlloc = vb.allocId;
  }
}


} // namespace frx