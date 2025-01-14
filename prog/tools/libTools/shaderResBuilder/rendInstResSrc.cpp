// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_mesh.h>
#include <shaders/dag_rendInstRes.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_genIo.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <startup/dag_restart.h>
#include <debug/dag_log.h>
#include <libTools/util/meshUtil.h>
#include <libTools/shaderResBuilder/rendInstResSrc.h>
#include <libTools/shaderResBuilder/lodsEqMatGather.h>
#include <libTools/shaderResBuilder/matSubst.h>
#include <generic/dag_tabUtils.h>
#include <generic/dag_smallTab.h>
#include <libTools/shaderResBuilder/globalVertexDataConnector.h>
#include <libTools/util/makeBindump.h>
#include <math/dag_boundingSphere.h>
#include <math/dag_scanLineRasterizer.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_plane3.h>
#include <math/dag_convexHullComputer.h>
#include <image/dag_texPixel.h>
#include <image/dag_tga.h>
#include <generic/dag_initOnDemand.h>
#include "tristrip/tristrip.h"
#include "collapseData.h"
#include <debug/dag_debug.h>
#include <util/dag_bitArray.h>
#include <obsolete/dag_cfg.h>
#include <sceneRay/dag_sceneRay.h>
#include <libTools/ambientOcclusion/ambientOcclusion.h>
#include <supp/dag_alloca.h>

// RenderableInstanceLodsResSrc //
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>

#define ISSUE_FATAL(...) log ? log->addMessage(ILogWriter::FATAL, __VA_ARGS__) : DAG_FATAL(__VA_ARGS__)

extern bool shadermeshbuilder_strip_d3dres;
bool RenderableInstanceLodsResSrc::optimizeForCache = true;
float RenderableInstanceLodsResSrc::sideWallAreaThresForOccluder = 10;
DataBlock *RenderableInstanceLodsResSrc::buildResultsBlk = NULL;
bool RenderableInstanceLodsResSrc::sepMatToBuildResultsBlk = false;
eastl::vector<DeleteParametersFromLod> RenderableInstanceLodsResSrc::deleteParameters;
bool generate_quads = false;
bool generate_strips = false;

static DagorMatShaderSubstitute matSubst;
static const char *curDagFname = NULL;

extern void prepare_billboard_mesh(Mesh &mesh, const TMatrix &wtm, dag::ConstSpan<Point3> org_verts);


void RenderableInstanceLodsResSrc::splitRealTwoSided(Mesh &m, Bitarray &is_material_real_two_sided_array)
{
  split_real_two_sided(m, is_material_real_two_sided_array, -1);
}


// add mesh node
void RenderableInstanceLodsResSrc::addMeshNode(Lod &lod, Node *n_, Node *key_node, LodsEqualMaterialGather &mat_gather,
  StaticSceneRayTracer *ao_tracer)
{
  if (!n_)
    return;
  Node &n = *n_;

  for (const char *noRenderableNodeName : {"occluder_box", "floater_box"})
    if ((n.flags & NODEFLG_RENDERABLE) && strcmp(n.name, noRenderableNodeName) == 0)
      logwarn("node <%s> is flagged renderable", noRenderableNodeName);

  if ((n.flags & NODEFLG_RENDERABLE) && n.mat && n.mat->subMatCount() && n.obj && n.obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj &mh = *(MeshHolderObj *)n.obj;
    if (!mh.bones && mh.mesh)
    {
      // add mesh
      Mesh &mesh = *mh.mesh;

      for (unsigned int materialNo = 0; materialNo < n.mat->subMatCount(); materialNo++)
        if (strstr(n.mat->getSubMat(materialNo)->className, "rendinst_impostor") || //-V522
            strstr(n.mat->getSubMat(materialNo)->className, "rendinst_baked_impostor"))
          hasImpostor = true;

      if (hasImpostor)
        hasOccl = false;
      if (shadermeshbuilder_strip_d3dres)
      {
        hasOccl = false;
        TMatrix tm = inverse(key_node->wtm) * n.wtm;
        mesh.transform(tm);
        for (int i = 0; i < mesh.getVert().size(); ++i)
        {
          lod.bbox += mesh.getVert()[i];
          float dist2 = lengthSq(mesh.getVert()[i]);
          if (lod.bs0rad2 < dist2)
            lod.bs0rad2 = dist2;
        }
        lod.bsph += ::mesh_bounding_sphere(mesh.getVert().data(), mesh.getVert().size());

        for (int i = 0; i < n.child.size(); ++i)
          addMeshNode(lod, n.child[i], key_node, mat_gather, NULL);
        return;
      }

      // Process real_two_sided.

      Bitarray isMaterialRealTwoSidedArray;
      isMaterialRealTwoSidedArray.resize(n.mat->subMatCount());
      for (unsigned int materialNo = 0; materialNo < n.mat->subMatCount(); materialNo++)
      {
        Ptr<MaterialData> subMat = n.mat->getSubMat(materialNo);
        if (!subMat)
        {
          isMaterialRealTwoSidedArray.reset(materialNo);
          continue;
        }

        matSubst.substMatClass(*subMat);
        subMat = processMaterial(subMat, false);
        CfgReader c;
        c.getdiv_text(String(128, "[q]\r\n%s\r\n", subMat->matScript.str()), "q");

        if (c.getbool("real_two_sided", false))
          isMaterialRealTwoSidedArray.set(materialNo);
        else
          isMaterialRealTwoSidedArray.reset(materialNo);
      }

      splitRealTwoSided(mesh, isMaterialRealTwoSidedArray);


      G_ASSERT(key_node);

      TMatrix tm = inverse(key_node->wtm) * n.wtm;
      bool billboard = false;
      const char *normalsType = NULL;
      Point3 normals_dir;
      DataBlock blk;

      if (dblk::load_text(blk, make_span_const(n.script), dblk::ReadFlag::ROBUST, curDagFname))
      {
        // read node flags and other properties
        billboard = blk.getBool("billboard", false);
        normalsType = blk.getStr("normals", NULL);
        normals_dir = blk.getPoint3("dir", Point3(0, 1, 0));
      }

      // transform mesh to key node space
      int i;
      // prepare mesh for building
      int numMat = n.mat->subMatCount();
      mesh.clampMaterials(numMat - 1);

      G_VERIFY(mesh.sort_faces_by_mat());

      mesh.optimize_tverts();
      if (!mesh.vertnorm.size())
      {
        G_VERIFY(mesh.calc_ngr());
        G_VERIFY(mesh.calc_vertnorms());
      }
      if (tm.det() > 0)
      {
        for (int i = 0; i < mesh.vertnorm.size(); ++i)
          mesh.vertnorm[i] = -mesh.vertnorm[i];
        for (int i = 0; i < mesh.facenorm.size(); ++i)
          mesh.facenorm[i] = -mesh.facenorm[i];
      }
      mesh.transform(tm);

      if (hasOccl && !explicitOccl && &lod == &lods[0])
      {
        Mesh m2;
        m2.vert = mesh.vert;
        m2.face.reserve(mesh.face.size());
        int lastMatId = -1;
        bool skip_last_mat = true;

        for (i = 0; i < mesh.face.size(); ++i)
        {
          // debug("f[%d] m=%d v=%d,%d,%", i, mesh.face[i].mat, mesh.face[i].v[0], mesh.face[i].v[1], mesh.face[i].v[2]);
          if (lastMatId != mesh.face[i].mat)
          {
            lastMatId = mesh.face[i].mat;
            Ptr<MaterialData> mat = n.mat->getSubMat(lastMatId);
            matSubst.substMatClass(*mat);
            mat = processMaterial(mat, false);
            if (const char *p = strstr(mat->matScript, "atest="))
              skip_last_mat = atoi(p + 6);
            else
              skip_last_mat = false;
            // debug("  mat %d %s %s <%s> -> skip=%d", lastMatId, mat->matName, mat->className, mat->matScript, skip_last_mat);
          }
          if (!skip_last_mat)
            append_items(m2.face, 1, &mesh.face[i]);
        }
        m2.face.shrink_to_fit();
        m2.kill_unused_verts(0.0001);
        m2.kill_bad_faces();
        occlMesh.attach(m2);
        // debug("mesh(%d,%d) -> m2(%d,%d) -> occlMesh(%d,%d)",
        //   mesh.vert.size(), mesh.face.size(), m2.vert.size(), m2.face.size(), occlMesh.vert.size(), occlMesh.face.size());
      }

      struct GatherExtraChanUsage : public ShaderChannelsEnumCB
      {
        Tab<int> extraIdx;

        void enum_shader_channel(int u, int ui, int /*t*/, int /*vbu*/, int /*vbui*/, ChannelModifier /*mod*/, int /*stream*/) override
        {
          if (u == SCUSAGE_EXTRA)
            if (find_value_idx(extraIdx, ui) < 0)
              extraIdx.push_back(ui);
        }
      } extra_chans_used;

      Tab<ShaderMaterial *> shmat(tmpmem);
      shmat.resize(numMat);
      mem_set_0(shmat);

      for (i = 0; i < numMat; ++i)
      {
        Ptr<MaterialData> subMat = n.mat->getSubMat(i);
        if (!subMat)
        {
          logwarn("invalid sub-material #%d in '%s'", i + 1, lod.fileName);
          continue;
        }
        if (!matSubst.substMatClass(*subMat))
          continue;
        subMat = processMaterial(subMat);

        shmat[i] = mat_gather.addEqual(subMat);
        if (shmat[i])
        {
          int flags = 0;
          shmat[i]->enum_channels(extra_chans_used, flags);
        }
      }

      if (find_value_idx(extra_chans_used.extraIdx, 53) >= 0)
        create_vertex_color_data(mesh, SCUSAGE_EXTRA, 53);

      if (find_value_idx(extra_chans_used.extraIdx, 56) >= 0 && find_value_idx(extra_chans_used.extraIdx, 57) >= 0)
        add_per_vertex_domain_uv(mesh, SCUSAGE_EXTRA, 56, 57);


      for (i = 0; i < mesh.getVert().size(); ++i)
      {
        lod.bbox += mesh.getVert()[i];
        float dist2 = lengthSq(mesh.getVert()[i]);
        if (lod.bs0rad2 < dist2)
          lod.bs0rad2 = dist2;
      }
      lod.bsph += ::mesh_bounding_sphere(mesh.getVert().data(), mesh.getVert().size());

      if (billboard)
      {
        SmallTab<Point3, TmpmemAlloc> org_verts;
        org_verts = mesh.getVert();
        prepare_billboard_mesh(mesh, key_node->wtm, org_verts);
      }

      if (normalsType)
      {
        enum
        {
          NORMAL,
          SPHERICAL,
          WORLD,
          LOCAL
        };
        int type = NORMAL;
        if (strcmp(normalsType, "spherical") == 0)
          type = SPHERICAL;
        else if (strcmp(normalsType, "world") == 0)
          type = WORLD;
        else if (strcmp(normalsType, "local") == 0)
          type = LOCAL;
        if (type == SPHERICAL)
        {
          Point3 center = tm.getcol(3);
          for (int i = 0; i < mesh.getMeshData().facengr.size(); ++i)
            for (int j = 0; j < 3; ++j)
              mesh.getMeshData().facengr[i][j] = mesh.getFace()[i].v[j];
          mesh.getMeshData().vertnorm.resize(mesh.getVert().size());
          for (int i = 0; i < mesh.getMeshData().vertnorm.size(); ++i)
            mesh.getMeshData().vertnorm[i] = normalize(mesh.getVert()[i] - center);
        }
        else if (type == WORLD || type == LOCAL)
        {
          Point3 dir;
          if (type == WORLD)
            dir = normals_dir;
          else
            dir = tm % normals_dir;
          dir.normalize();
          mesh.getMeshData().vertnorm.resize(1);
          mesh.getMeshData().vertnorm[0] = dir;

          mesh.getMeshData().facengr.resize(mesh.getFace().size());
          for (int i = 0; i < mesh.getMeshData().facengr.size(); ++i)
            for (int j = 0; j < 3; ++j)
              mesh.getMeshData().facengr[i][j] = 0;
        }
      }

      // build ShaderMeshData
      ShaderMeshData &md = lod.rigid.meshData;

      ::collapse_materials(mesh, shmat);
      if (buildImpostorDataNow)
        processImpostorMesh(mesh, shmat);

      md.build(mesh, shmat.data(), numMat, IdentColorConvert::object, false);

      if (::generate_quads && billboard)
      {
        for (unsigned int elemNo = 0; elemNo < md.elems.size(); elemNo++)
        {
          if (md.elems[elemNo].optDone)
            continue;

          ShaderMeshData::RElem &elem = md.elems[elemNo];
          md.elems[elemNo].optDone = 1;

          // debug(
          //   "generate_quads: elemNo = %d, vertexData = 0x%08x, vdOrderIndex = %d, "
          //   "sv = %d, numv = %d, si = %d, numf = %d",
          //   elemNo,
          //   elem.vertexData.get(),
          //   elem.vdOrderIndex,
          //   elem.sv,
          //   elem.numv,
          //   elem.si,
          //   elem.numf);

          G_ASSERT(elem.vertexData);
          G_ASSERT(elem.vertexData->partCount == 1);
          G_ASSERT(!elem.vertexData->iData.empty());
          G_ASSERT(elem.vertexData->iData32.empty());
          G_ASSERT(elem.vdOrderIndex == 0);
          G_ASSERT(elem.sv == 0);
          G_ASSERT(elem.si == 0);
          G_ASSERT(elem.numf > 0);


          if (elem.numf % 2 != 0)
          {
            DAG_FATAL("Invalid number of faces (%d) in billboard '%s' - should be multiple of 2", elem.numf, lod.fileName);
          }

          if (elem.numv % 4 != 0)
          {
            DAG_FATAL("Invalid number of vertices (%d) in billboard '%s' - should be multiple of 4", elem.numv, lod.fileName);
          }

          if (elem.numv / 4 != elem.numf / 2)
          {
            DAG_FATAL("Invalid number of vertices (%d) or faces (%d) in billboard '%s' - should be 4 vertices per 2 faces", elem.numv,
              elem.numf, lod.fileName);
          }

          unsigned char *quadedVertices = new unsigned char[(int)(elem.numf / 2) * 4 * elem.vertexData->stride];

          unsigned char *curVertex = quadedVertices;

          // for (unsigned int quadNo = 0; quadNo < elem.numf / 2; quadNo++)
          //{
          //   for (unsigned int indexNo = 0; indexNo < 6; indexNo++)
          //   {
          //     debug("q = %d, i = %d", quadNo, elem.vertexData->iData[6 * quadNo + indexNo]);
          //   }
          // }

          // for (unsigned int vertNo = 0; vertNo < elem.vertexData->numv; vertNo++)
          //{
          //   debug("v = %d, wind = 0x%08x",
          //     vertNo,
          //     *(unsigned int *)(elem.vertexData->vData.data() +
          //     vertNo * elem.vertexData->stride + 8 * sizeof(float)));
          // }


          Tab<bool> faceQuadded(tmpmem);
          faceQuadded.resize(elem.numf);
          mem_set_0(faceQuadded);

          for (unsigned int quadNo = 0; quadNo < elem.numf / 2; quadNo++)
          {
            unsigned int usedVertices[6];
            unsigned int numUsedVerticesByIndex[6];
            unsigned int numUsedVertices = 0;

            unsigned int firstFaceNo;
            for (firstFaceNo = 0; firstFaceNo < elem.numf; firstFaceNo++)
            {
              if (!faceQuadded[firstFaceNo])
              {
                for (unsigned int indexNo = 0; indexNo < 3; indexNo++)
                {
                  unsigned int testVertexNo;
                  for (testVertexNo = 0; testVertexNo < numUsedVertices; testVertexNo++)
                  {
                    if (usedVertices[testVertexNo] == elem.vertexData->iData[3 * firstFaceNo + indexNo])
                    {
                      numUsedVerticesByIndex[testVertexNo]++;
                      goto Found;
                    }
                  }
                  usedVertices[numUsedVertices] = elem.vertexData->iData[3 * firstFaceNo + indexNo];
                  numUsedVerticesByIndex[numUsedVertices] = 1;
                  numUsedVertices++;
                Found:;
                }

                if (numUsedVertices != 3)
                {
                  DAG_FATAL("Degenerate triangle in billboard '%s'", lod.fileName);
                }

                faceQuadded[firstFaceNo] = true;
                break;
              }
            }

            G_ASSERT(firstFaceNo < elem.numf);

            unsigned int secondFaceNo;
            for (secondFaceNo = 0; secondFaceNo < elem.numf; secondFaceNo++)
            {
              if (!faceQuadded[secondFaceNo])
              {
                unsigned int numUsedVerticesInSecondFace = 0;
                for (unsigned int indexNo = 0; indexNo < 3; indexNo++)
                {
                  for (unsigned int testVertexNo = 0; testVertexNo < numUsedVertices; testVertexNo++)
                  {
                    if (usedVertices[testVertexNo] == elem.vertexData->iData[3 * secondFaceNo + indexNo])
                    {
                      goto Found2;
                    }
                  }
                  numUsedVerticesInSecondFace++;
                Found2:;
                }

                if (numUsedVerticesInSecondFace == 1)
                {
                  for (unsigned int indexNo = 0; indexNo < 3; indexNo++)
                  {
                    unsigned int testVertexNo;
                    for (testVertexNo = 0; testVertexNo < numUsedVertices; testVertexNo++)
                    {
                      if (usedVertices[testVertexNo] == elem.vertexData->iData[3 * secondFaceNo + indexNo])
                      {
                        numUsedVerticesByIndex[testVertexNo]++;
                        goto Found3;
                      }
                    }
                    usedVertices[numUsedVertices] = elem.vertexData->iData[3 * secondFaceNo + indexNo];
                    numUsedVerticesByIndex[numUsedVertices] = 1;
                    numUsedVertices++;
                  Found3:;
                  }

                  faceQuadded[secondFaceNo] = true;
                  break;
                }
              }
            }

            if (secondFaceNo == elem.numf)
            {
              DAG_FATAL("Second face for quad in '%s' not found", lod.fileName);
            }

            if (numUsedVertices != 4)
            {
              DAG_FATAL("Invalid quad with %d vertices in billboard '%s' - should be 4 vertices per quad", numUsedVertices,
                lod.fileName);
            }

            unsigned int vertexOrder[4];
            if (numUsedVerticesByIndex[0] == 1)
            {
              vertexOrder[0] = usedVertices[0];
              vertexOrder[1] = usedVertices[1];
              vertexOrder[2] = usedVertices[3];
              vertexOrder[3] = usedVertices[2];
            }
            else if (numUsedVerticesByIndex[1] == 1)
            {
              vertexOrder[0] = usedVertices[0];
              vertexOrder[1] = usedVertices[1];
              vertexOrder[2] = usedVertices[2];
              vertexOrder[3] = usedVertices[3];
            }
            else
            {
              vertexOrder[0] = usedVertices[0];
              vertexOrder[1] = usedVertices[3];
              vertexOrder[2] = usedVertices[1];
              vertexOrder[3] = usedVertices[2];
            }

            for (unsigned int vertexNo = 0; vertexNo < 4; vertexNo++)
            {
              memcpy(curVertex, elem.vertexData->vData.data() + vertexOrder[vertexNo] * elem.vertexData->stride,
                elem.vertexData->stride);
              curVertex += elem.vertexData->stride;
            }
          }

          elem.numv = (int)(elem.numf / 2) * 4;
          elem.si = 0;
          elem.numf = (int)0xFFFFFFFD; // Quad magic.
          elem.vertexData->vData = make_span_const(quadedVertices, elem.numv * elem.vertexData->stride);
          elem.vertexData->numv = elem.numv;
          clear_and_shrink(elem.vertexData->iData);
          clear_and_shrink(elem.vertexData->iData32);
          elem.vertexData->numf = 0;

          delete[] quadedVertices;


          // for (unsigned int vertNo = 0; vertNo < elem.vertexData->numv; vertNo++)
          //{
          //   debug("v = %d, wind = 0x%08x",
          //     vertNo,
          //     *(unsigned int *)(elem.vertexData->vData.data() +
          //     vertNo * elem.vertexData->stride + 8 * sizeof(float)));
          // }
        }
      }
      else if (::generate_strips)
      {
        for (unsigned int elemNo = 0; elemNo < md.elems.size(); elemNo++)
        {
          if (md.elems[elemNo].optDone)
            continue;

          ShaderMeshData::RElem &elem = md.elems[elemNo];
          md.elems[elemNo].optDone = 1;

          // debug(
          //   "generate_strips: elemNo = %d, vertexData = 0x%08x, vdOrderIndex = %d, "
          //   "sv = %d, numv = %d, si = %d, numf = %d",
          //   elemNo,
          //   elem.vertexData.get(),
          //   elem.vdOrderIndex,
          //   elem.sv,
          //   elem.numv,
          //   elem.si,
          //   elem.numf);

          G_ASSERT(elem.vertexData);
          G_ASSERT(elem.vertexData->partCount == 1);
          G_ASSERT(!elem.vertexData->iData.empty());
          G_ASSERT(elem.vertexData->iData32.empty());
          G_ASSERT(elem.vdOrderIndex == 0);
          G_ASSERT(elem.sv == 0);
          G_ASSERT(elem.si == 0);
          G_ASSERT(elem.numv == elem.vertexData->numv);
          G_ASSERT(elem.numf > 0);
          G_ASSERT(elem.numf == elem.vertexData->numf);

          PublicStripifier::SetCacheSize(1000000); // Set infinite cache to minimize the number of degenerate triangles.
          PublicStripifier::SetStitchStrips(true);
          PublicStripifier::SetMinStripSize(0);
          PublicStripifier::SetListsOnly(false);

          PublicStripifier::PrimitiveGroup *primGroups;
          unsigned short int numGroups;

          // Workaround NVTriStrip inability to stripify multitriangle edges generated by real_two_sided with spherical normals.

          G_ASSERT(elem.numv < 32768);
          unsigned int numDuplicatedTris = 0;
          for (unsigned int faceNo = 1; faceNo < elem.vertexData->numf; faceNo++)
          {
            for (unsigned int testFaceNo = 0; testFaceNo < faceNo; testFaceNo++)
            {
              bool duplicated = true;
              for (unsigned int pointNo = 0; pointNo < 3; pointNo++)
              {
                bool found = false;
                for (unsigned int testPointNo = 0; testPointNo < 3; testPointNo++)
                {
                  if (elem.vertexData->iData[3 * faceNo + pointNo] == elem.vertexData->iData[3 * testFaceNo + testPointNo])
                  {
                    found = true;
                    break;
                  }
                }

                if (!found)
                {
                  duplicated = false;
                  break;
                }
              }

              if (duplicated)
              {
                elem.vertexData->iData[3 * faceNo] += 32768;
                elem.vertexData->iData[3 * faceNo + 1] += 32768;
                elem.vertexData->iData[3 * faceNo + 2] += 32768;
                numDuplicatedTris++;
                break;
              }
            }
          }

          // debug("numDuplicatedTris = %d", numDuplicatedTris);


          PublicStripifier::GenerateStrips(elem.vertexData->iData.data(), elem.numf * 3, &primGroups, &numGroups);
          G_ASSERT(numGroups == 1);
          G_ASSERT(primGroups[0].type == TriStripifier::PT_STRIP);
          G_ASSERT(primGroups[0].indices.size() > 0);

          for (unsigned int indexNo = 0; indexNo < primGroups[0].indices.size(); indexNo++)
          {
            if (primGroups[0].indices[indexNo] >= 32768)
              primGroups[0].indices[indexNo] -= 32768;
          }

          unsigned char *stripifiedVertices =
            new unsigned char[(primGroups[0].indices.size() + 2 + primGroups[0].indices.size() % 2) * elem.vertexData->stride];

          unsigned char *curVertex = stripifiedVertices;

          G_ASSERT(primGroups[0].indices[0] < elem.numv);

          memcpy(curVertex, elem.vertexData->vData.data() + primGroups[0].indices[0] * elem.vertexData->stride,
            elem.vertexData->stride);
          curVertex += elem.vertexData->stride;

          for (unsigned int indexNo = 0; indexNo < primGroups[0].indices.size(); indexNo++)
          {
            G_ASSERT(primGroups[0].indices[indexNo] < elem.numv);

            memcpy(curVertex, elem.vertexData->vData.data() + primGroups[0].indices[indexNo] * elem.vertexData->stride,
              elem.vertexData->stride);
            curVertex += elem.vertexData->stride;
          }

          memcpy(curVertex,
            elem.vertexData->vData.data() + primGroups[0].indices[primGroups[0].indices.size() - 1] * elem.vertexData->stride,
            elem.vertexData->stride);
          curVertex += elem.vertexData->stride;

          if (primGroups[0].indices.size() % 2 == 1)
          {
            memcpy(curVertex,
              elem.vertexData->vData.data() + primGroups[0].indices[primGroups[0].indices.size() - 1] * elem.vertexData->stride,
              elem.vertexData->stride);
            curVertex += elem.vertexData->stride;
          }

          elem.numv = primGroups[0].indices.size() + 2 + primGroups[0].indices.size() % 2;
          elem.si = 0;
          elem.numf = (int)0xFFFFFFFE; // Strip magic.
          elem.vertexData->vData = make_span_const(stripifiedVertices, elem.numv * elem.vertexData->stride);
          elem.vertexData->numv = elem.numv;
          clear_and_shrink(elem.vertexData->iData);
          clear_and_shrink(elem.vertexData->iData32);
          elem.vertexData->numf = 0;

          // debug("generate_strips: numv = %d", elem.numv);

          delete[] primGroups;
          delete[] stripifiedVertices;
        }
      }
      else if (optimizeForCache)
      {
        md.optimizeForCache();
      }
    }
  }
  else if (!(n.flags & NODEFLG_RENDERABLE) && hasOccl && !explicitOccl && strcmp(n.name, "occluder_box") == 0 &&
           (!n.obj || !n.obj->isSubOf(OCID_MESHHOLDER) || !((MeshHolderObj *)n.obj)->mesh->vert.size()))
  {
    explicitOccl = true;
    hasOccl = false;
    debug("explicitly disabled occluder_box");
  }
  else if (!(n.flags & NODEFLG_RENDERABLE) && hasOccl && !explicitOccl && n.obj && n.obj->isSubOf(OCID_MESHHOLDER) &&
           strcmp(n.name, "occluder_box") == 0)
  {
    explicitOccl = true;
    clear_and_shrink(occlMesh.vert);
    clear_and_shrink(occlMesh.face);

    MeshHolderObj &mh = *(MeshHolderObj *)n.obj;
    Mesh &mesh = *mh.mesh;

    TMatrix tm = inverse(key_node->wtm) * n.wtm;
    mesh.transform(tm);
    occlBb.setempty();
    if (mesh.vert.size() == 4 && mesh.face.size() == 2)
    {
      {
        int i0 = -1, i1 = -1, i2 = -1, i3 = -1;
        for (int v1 = 0; v1 < 3; v1++)
          for (int v2 = 0; v2 < 3; v2++)
            if (mesh.face[0].v[v1] == mesh.face[1].v[(v2 + 1) % 3] && mesh.face[0].v[(v1 + 1) % 3] == mesh.face[1].v[v2])
            {
              i0 = mesh.face[0].v[(v1 + 2) % 3];
              i1 = mesh.face[0].v[v1];
              i2 = mesh.face[1].v[(v2 + 2) % 3];
              i3 = mesh.face[0].v[(v1 + 1) % 3];
              v1 = v2 = 4;
            }
        G_ASSERTF(i0 != -1, "(%d,%d,%d), (%d,%d,%d) -> i=%d,%d,%d,%d", mesh.face[0].v[0], mesh.face[0].v[1], mesh.face[0].v[2],
          mesh.face[1].v[0], mesh.face[1].v[1], mesh.face[1].v[2], i0, i1, i2, i3);

        occlMesh.vert.resize(4);
        occlMesh.vert[0] = mesh.vert[i0];
        occlMesh.vert[1] = mesh.vert[i1];
        occlMesh.vert[2] = mesh.vert[i2];
        occlMesh.vert[3] = mesh.vert[i3];
        mesh.vert = occlMesh.vert;
      }
      occlQuad = true;

      Plane3 p(mesh.vert[0], mesh.vert[1], mesh.vert[2]);
      if (p.getNormal().length() < 1e-6)
      {
        p.set(mesh.vert[0], mesh.vert[1], mesh.vert[3]);
        if (p.getNormal().length() < 1e-6)
        {
          DAG_FATAL("degenerate occluder quad %@,%@,%@,%@", mesh.vert[0], mesh.vert[1], mesh.vert[2], mesh.vert[3]);
          clear_and_shrink(occlMesh.vert);
        }
      }
      else if (p.distance(mesh.vert[3]) > 1e-3 * p.getNormal().length()) // with normalization for big quads
      {
        DAG_FATAL("non-planar occluder quad %@,%@,%@,%@ dist=%.7f", mesh.vert[0], mesh.vert[1], mesh.vert[2], mesh.vert[3],
          p.distance(mesh.vert[3]));
        clear_and_shrink(occlMesh.vert);
      }

      if (occlMesh.vert.size())
      {
        Point3 c = (mesh.vert[0] + mesh.vert[1] + mesh.vert[2] + mesh.vert[3]) * 0.25;
        real sm1 = ((mesh.vert[0] - c) % (mesh.vert[1] - c)) * p.getNormal();
        real sm2 = ((mesh.vert[1] - c) % (mesh.vert[2] - c)) * p.getNormal();
        real sm3 = ((mesh.vert[2] - c) % (mesh.vert[3] - c)) * p.getNormal();
        real sm4 = ((mesh.vert[3] - c) % (mesh.vert[0] - c)) * p.getNormal();
        if ((sm1 <= 0 && sm2 <= 0 && sm3 <= 0 && sm4 <= 0) || (sm1 >= 0 && sm2 >= 0 && sm3 >= 0 && sm4 >= 0))
          ; // ok, quad is convex
        else
        {
          DAG_FATAL("non-convex occluder quad %@,%@,%@,%@ (%.3f %.3f %.3f %.3f)", //
            mesh.vert[0], mesh.vert[1], mesh.vert[2], mesh.vert[3], sm1, sm2, sm3, sm4);
          clear_and_shrink(occlMesh.vert);
        }
      }

      if (occlMesh.vert.size())
        debug("explicit occluder_quad=%@ %@ %@ %@", occlMesh.vert[0], occlMesh.vert[1], occlMesh.vert[2], occlMesh.vert[3]);
      else
        hasOccl = false;
    }
    else
    {
      for (int i = 0; i < mesh.getVert().size(); ++i)
        occlBb += mesh.getVert()[i];
      debug("explicit occluder_box=%@", occlBb);
    }
  }
  else if (!(n.flags & NODEFLG_RENDERABLE) && n.obj && n.obj->isSubOf(OCID_MESHHOLDER) && strcmp(n.name, "floater_box") == 0)
  {
    hasFloatingBox = true;

    MeshHolderObj &mh = *(MeshHolderObj *)n.obj;
    Mesh &mesh = *mh.mesh;

    TMatrix tm = inverse(key_node->wtm) * n.wtm;
    mesh.transform(tm);

    floatingBox.setempty();
    for (int i = 0; i < mesh.getVert().size(); ++i)
      floatingBox += mesh.getVert()[i];

    debug("floater_box=%@", floatingBox);
  }

  for (int i = 0; i < n.child.size(); ++i)
  {
    addMeshNode(lod, n.child[i], key_node, mat_gather, ao_tracer);
  }
}

void RenderableInstanceLodsResSrc::addNode(Lod &lod, Node *n, Node *key_node, LodsEqualMaterialGather &mat_gather,
  StaticSceneRayTracer *ao_tracer)
{
  // enum meshes
  addMeshNode(lod, n, key_node, mat_gather, ao_tracer);
}

static const int AO_INDEX = 100;

class HasAOChanCB : public ShaderChannelsEnumCB
{
public:
  bool hasAO;
  int aoIndex;
  HasAOChanCB(int ind) : hasAO(false), aoIndex(ind) {}

  void enum_shader_channel(int u, int ui, int t, int vbu, int vbui, ChannelModifier mod, int /*stream*/) override
  {
    if (u == SCUSAGE_EXTRA && ui == aoIndex)
      hasAO = true;
  }
};

static bool has_ao(Node &n, RenderableInstanceLodsResSrc *ri)
{
  if (n.mat)
    for (unsigned int materialNo = 0; materialNo < n.mat->subMatCount(); materialNo++)
    {
      Ptr<MaterialData> subMat = n.mat->getSubMat(materialNo);
      if (!subMat)
        continue;
      matSubst.substMatClass(*subMat);
      subMat = ri->processMaterial(subMat, false);
      Ptr<ShaderMaterial> m = new_shader_material(*subMat, true, false);
      if (m)
      {
        HasAOChanCB hasAOcb(AO_INDEX);
        int flags = 0;
        m->enum_channels(hasAOcb, flags);
        if (hasAOcb.hasAO)
          return true;
      }
    }

  for (int i = 0; i < n.child.size(); ++i)
    if (has_ao(*n.child[i], ri))
      return true;
  return false;
}

static void remove_parameter_from_mat_script(eastl::string &script, const eastl::string &parameter_name)
{
  size_t start = script.find(parameter_name);
  if (start == eastl::string::npos)
    return;

  size_t end = script.find("\n", start);
  script.erase(start, end - start);
}

bool RenderableInstanceLodsResSrc::addLod(int lod_index, const char *filename, real range, LodsEqualMaterialGather &mat_gather,
  Tab<AScene *> &scene_list, const DataBlock &material_overrides, const char *add_mat_script)
{
  struct RenderSW : public RenderSWNoInterp<512, 512>
  {
    float viewCx, viewCy, viewWx, viewWy;

    RenderSW() : viewCx(0), viewCy(0), viewWx(1), viewWy(1) {}

    void setView(mat44f &gtm, vec4f fwd, vec4f up, vec4f pos, float cx, float cy, float wx, float wy, float wz)
    {
      viewCx = cx, viewCy = cy, viewWx = wx, viewWy = wy;

      mat44f ptm, vtm;
      vtm.col2 = fwd;
      vtm.col0 = v_norm3(v_cross3(up, vtm.col2));
      vtm.col1 = v_cross3(vtm.col2, vtm.col0);
      vtm.col3 = pos;
      v_mat44_orthonormal_inverse43_to44(vtm, vtm);
      v_mat44_make_ortho(ptm, wx, wy, 0, wz * 2);
      v_mat44_mul(gtm, ptm, vtm);
    }

    bool detectFilledBox(int &out_x0, int &out_y0, int &out_x1, int &out_y1)
    {
      // find start point near to center
      int x = WIDTH / 2, y = HEIGHT / 2, cnt = 1, max_cnt = min(WIDTH, HEIGHT) * 2, dx = 1, dy = 0;
      double sum_x = 0, sum_y = 0, sum_cnt = 0;
      for (y = 0; y < HEIGHT; y++)
        for (x = 0; x < WIDTH; x++)
          if (checkPixel(x, y))
            sum_x += x, sum_y += y, sum_cnt++;
      if (!sum_cnt)
      {
        out_x0 = out_y0 = 0;
        out_x1 = out_y1 = -1;
        return false;
      }
      x = sum_x / sum_cnt;
      y = sum_y / sum_cnt;
      // debug("wt center at %d,%d", x, y);
      while (cnt < max_cnt)
      {
        for (int i = 0; i < cnt; i++, x += dx, y += dy)
          if (checkPixelSafe(x, y))
            goto found_start;
        if (dx)
        {
          dy = dx;
          dx = 0;
        }
        else
        {
          dx = -dy;
          dy = 0;
          cnt++;
        }
        // debug ("%d,%d  dir(%d,%d) cnt=%d", x, y, dx, dy, cnt);
      }
      LOGERR_CTX("pixel not found");
      out_x0 = out_y0 = 0;
      out_x1 = out_y1 = -1;
      return false;

    found_start:
      int x0 = x, y0 = y, x1 = x0, y1 = y0, last_state = 0, next_state = 0;
      // debug("start at %d,%d", x, y);
      for (;; last_state = next_state) // infinite with explicit break
      {
        bool ln_l = x0 > 0, ln_t = y0 > 0, ln_r = x1 + 1 < WIDTH, ln_b = y1 + 1 < HEIGHT;
        if (ln_t)
          for (int i = x0 + 1; i < x1; i++)
            if (!checkPixel(i, y0 - 1))
            {
              ln_t = false;
              break;
            }
        if (ln_b)
          for (int i = x0 + 1; i < x1; i++)
            if (!checkPixel(i, y1 + 1))
            {
              ln_b = false;
              break;
            }
        if (ln_l)
          for (int i = y0 + 1; i < y1; i++)
            if (!checkPixel(x0 - 1, i))
            {
              ln_l = false;
              break;
            }
        if (ln_r)
          for (int i = y0 + 1; i < y1; i++)
            if (!checkPixel(x1 + 1, i))
            {
              ln_r = false;
              break;
            }

        next_state = 0;
        if (ln_l && ln_t && checkPixel(x0 - 1, y0) && checkPixel(x0 - 1, y0 - 1) && checkPixel(x0, y0 - 1))
        {
          x0--;
          y0--;
          // DEBUG_CTX("%d,%d - %d,%d", x0, y0, x1, y1);
          continue;
        }
        if (ln_r && ln_t && checkPixel(x1, y0 - 1) && checkPixel(x1 + 1, y0 - 1) && checkPixel(x1 + 1, y0))
        {
          x1++;
          y0--;
          // DEBUG_CTX("%d,%d - %d,%d", x0, y0, x1, y1);
          continue;
        }
        if (ln_r && ln_b && checkPixel(x1 + 1, y1) && checkPixel(x1 + 1, y1 + 1) && checkPixel(x1, y1 + 1))
        {
          x1++;
          y1++;
          // DEBUG_CTX("%d,%d - %d,%d", x0, y0, x1, y1);
          continue;
        }
        if (ln_l && ln_b && checkPixel(x0 - 1, y1) && checkPixel(x0 - 1, y1 + 1) && checkPixel(x0, y1 + 1))
        {
          x0--;
          y1++;
          // DEBUG_CTX("%d,%d - %d,%d", x0, y0, x1, y1);
          continue;
        }

        if (ln_l && ln_r && checkPixel(x0 - 1, y0) && checkPixel(x0 - 1, y1) && checkPixel(x1 + 1, y0) && checkPixel(x1 + 1, y1))
        {
          x0--;
          x1++;
          // DEBUG_CTX("%d,%d - %d,%d", x0, y0, x1, y1);
          continue;
        }
        if (ln_t && ln_b && checkPixel(x0, y0 - 1) && checkPixel(x0, y1 + 1) && checkPixel(x1, y0 - 1) && checkPixel(x1, y1 + 1))
        {
          y0--;
          y1++;
          // DEBUG_CTX("%d,%d - %d,%d", x0, y0, x1, y1);
          continue;
        }

        if (ln_l && ln_r && x1 - x0 < y1 - y0 && last_state != 2)
        {
          x0--;
          y0++;
          x1++;
          y1--;
          next_state = 1;
          // DEBUG_CTX("%d,%d - %d,%d", x0, y0, x1, y1);
          continue;
        }
        if (ln_t && ln_b && x1 - x0 > y1 - y0 && last_state != 1)
        {
          x0++;
          y0--;
          x1--;
          y1++;
          next_state = 2;
          // DEBUG_CTX("%d,%d - %d,%d", x0, y0, x1, y1);
          continue;
        }

        if (ln_l && checkPixel(x0 - 1, y0) && checkPixel(x0 - 1, y1))
        {
          x0--;
          // DEBUG_CTX("%d,%d - %d,%d", x0, y0, x1, y1);
          continue;
        }
        if (ln_r && checkPixel(x1 + 1, y0) && checkPixel(x1 + 1, y1))
        {
          x1++;
          // DEBUG_CTX("%d,%d - %d,%d", x0, y0, x1, y1);
          continue;
        }
        if (ln_t && checkPixel(x0, y0 - 1) && checkPixel(x1, y0 - 1))
        {
          y0--;
          // DEBUG_CTX("%d,%d - %d,%d", x0, y0, x1, y1);
          continue;
        }
        if (ln_b && checkPixel(x0, y1 + 1) && checkPixel(x1, y1 + 1))
        {
          y1++;
          // DEBUG_CTX("%d,%d - %d,%d", x0, y0, x1, y1);
          continue;
        }
        break;
      }
      out_x0 = x0;
      out_y0 = y0;
      out_x1 = x1;
      out_y1 = y1;
      return true;
    }

    Point2 fromScr(int x, int y) const
    {
      return Point2((x - WIDTH / 2) * viewWx / WIDTH + viewCx, (y - HEIGHT / 2) * viewWy / HEIGHT + viewCy);
    }
    Point2 pixelSz() const { return Point2(viewWx / WIDTH, viewWy / HEIGHT); }

    void saveTga(const char *fn, int x0, int y0, int x1, int y1)
    {
      Tab<TexPixel32> pix;
      pix.resize(WIDTH * HEIGHT);
      for (int i = 0; i < pix.size(); i++)
        pix[i].r = pix[i].g = pix[i].b = depth[i], pix[i].a = 255;

      for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
          pix[y * WIDTH + x].r = 128;

      save_tga32(fn, pix.data(), WIDTH, HEIGHT, WIDTH * 4);
    }
  };

  PtrTab<MaterialData> matList;
  AScene &sc = *scene_list.push_back(new AScene());
  if (!load_ascene((char *)filename, sc, LASF_NULLMATS | LASF_MATNAMES, false, &matList))
  {
    ISSUE_FATAL("Failed to load %s", filename);
    return false;
  }

  override_materials(matList, material_overrides, nullptr);

  bool hasAO = has_ao(*sc.root, this);

  if (add_mat_script)
  {
    for (auto &mat : matList)
    {
      eastl::string script = mat->matScript.c_str();
      script += add_mat_script;
      mat->matScript = script.c_str();
    }
  }

  if (lod_index != 0)
  {
    for (auto &mat : matList)
    {
      eastl::string script = mat->matScript.c_str();
      remove_parameter_from_mat_script(script, "material_pn_triangulation");
      mat->matScript = script.c_str();
    }
  }

  BuildableStaticSceneRayTracer *rayTracer = NULL;
  sc.root->calc_wtm();
  if (log)
    if (!matSubst.checkMatClasses(sc.root, filename, *log))
      return false;
  if (hasAO)
    rayTracer = create_buildable_staticmeshscene_raytracer(Point3(1, 1, 1), 7); // fixme

  int lodId = append_items(lods, 1);
  Lod &lod = lods[lodId];

  for (const auto &delete_parameters_from_lod : deleteParameters)
  {
    if (delete_parameters_from_lod.lod != lodId)
      continue;

    for (auto &mat : matList)
    {
      if (!delete_parameters_from_lod.shader_name.empty() && delete_parameters_from_lod.shader_name != mat->className.c_str())
        continue;

      eastl::string script = mat->matScript.c_str();

      CfgReader cfg_reader;
      cfg_reader.getdiv_text(String(128, "[q]\r\n%s\r\n", script.c_str()), "q");
      if (!cfg_reader.getbool("autoLodOptimization", true))
        continue;

      for (const auto &param : delete_parameters_from_lod.parameters)
      {
        remove_parameter_from_mat_script(script, param);
      }
      mat->matScript = script.c_str();
    }
  }

  lod.fileName = filename;
  lod.range = range;

  curDagFname = filename;
  OAHashNameMap<true> skip_nodes;
  skip_nodes.addNameId("occluder_box");
  skip_nodes.addNameId("floater_box");
  ::collapse_nodes(sc.root, sc.root, false, false, curDagFname, skip_nodes, true);
  addNode(lod, sc.root, sc.root, mat_gather, rayTracer);
  curDagFname = NULL;

  if (hasOccl && !explicitOccl && lodId == 0)
  {
    BBox3 bb;
    bb.setempty();
    for (int i = 0; i < occlMesh.vert.size(); i++)
      bb += occlMesh.vert[i];
    Point3 bw = bb.width();
    Point3 bc = bb.center();
    if (occlMesh.vert.size())
      debug("occlMesh(%d,%d)  bbox=%@", occlMesh.vert.size(), occlMesh.face.size(), bb);

    if (occlMesh.vert.size() && (bw.x * bw.y > sideWallAreaThresForOccluder || bw.y * bw.z > sideWallAreaThresForOccluder))
    {
      RenderSW *r = new RenderSW;
      Tab<vec4f> vert;
      Tab<int> face;
      mat44f gtm;
      BBox2 yz, zx, xy;
      int x0, y0, x1, y1;

      vert.resize(occlMesh.vert.size());
      face.resize(occlMesh.face.size() * 3);
      for (int i = 0; i < vert.size(); i++)
        vert[i] = v_and(v_ldu(&occlMesh.vert[i].x), (vec4f)V_CI_MASK1110);
      for (int i = 0; i < face.size(); i++)
        face[i] = occlMesh.face[i / 3].v[i % 3];

      debug("bc=%@  bw%@", bc, bw);
      bc.x = floorf(bc.x + 0.5f);
      bc.y = floorf(bc.y + 0.5f);
      bc.z = floorf(bc.z + 0.5f);
      bw.x = ceilf(bw.x) + 2;
      bw.y = ceilf(bw.y) + 2;
      bw.z = ceilf(bw.z) + 2;
      debug("-> bc=%@  bw%@", bc, bw);

      // render along X
      r->setView(gtm, V_C_UNIT_1000, V_C_UNIT_0010, v_make_vec4f(bc.x - bw.x, bc.y, bc.z, 1), bc.y, bc.z, bw.y, bw.z, bw.x);
      r->renderMesh(gtm, vert.data(), vert.size(), face.data(), occlMesh.face.size());
      yz.setempty();
      if (r->detectFilledBox(x0, y0, x1, y1))
      {
        yz += r->fromScr(x0, y0);
        yz += r->fromScr(x1, y0);
        yz += r->fromScr(x0, y1);
        yz += r->fromScr(x1, y1);
        yz[0] += r->pixelSz();
        yz[1] -= r->pixelSz();
        // r->saveTga(String(0, "%s_yz.tga", dd_get_fname(filename)), x0, y0, x1, y1);
      }

      // render along Y
      r->resetDepth();
      r->setView(gtm, V_C_UNIT_0100, V_C_UNIT_1000, v_make_vec4f(bc.x, bc.y - bw.y, bc.z, 1), bc.z, bc.x, bw.z, bw.x, bw.y);
      r->renderMesh(gtm, vert.data(), vert.size(), face.data(), occlMesh.face.size());
      zx.setempty();
      if (r->detectFilledBox(x0, y0, x1, y1))
      {
        zx += r->fromScr(x0, y0);
        zx += r->fromScr(x1, y0);
        zx += r->fromScr(x0, y1);
        zx += r->fromScr(x1, y1);
        zx[0] += r->pixelSz();
        zx[1] -= r->pixelSz();
        // r->saveTga(String(0, "%s_zx.tga", dd_get_fname(filename)), x0, y0, x1, y1);
      }

      // render along Z
      r->resetDepth();
      r->setView(gtm, V_C_UNIT_0010, V_C_UNIT_0100, v_make_vec4f(bc.x, bc.y, bc.z - bw.z, 1), bc.x, bc.y, bw.x, bw.y, bw.z);
      r->renderMesh(gtm, vert.data(), vert.size(), face.data(), occlMesh.face.size());
      xy.setempty();
      if (r->detectFilledBox(x0, y0, x1, y1))
      {
        xy += r->fromScr(x0, y0);
        xy += r->fromScr(x1, y0);
        xy += r->fromScr(x0, y1);
        xy += r->fromScr(x1, y1);
        xy[0] += r->pixelSz();
        xy[1] -= r->pixelSz();
        // r->saveTga(String(0, "%s_xy.tga", dd_get_fname(filename)), x0, y0, x1, y1);
      }

      occlBb[0].x = max(zx[0][1], xy[0][0]);
      occlBb[0].y = max(yz[0][0], xy[0][1]);
      occlBb[0].z = max(yz[0][1], zx[0][0]);
      occlBb[1].x = min(zx[1][1], xy[1][0]);
      occlBb[1].y = min(yz[1][0], xy[1][1]);
      occlBb[1].z = min(yz[1][1], zx[1][0]);
      debug("yz=%@ zx=%@ xy=%@  -> occlBb=%@", yz, zx, xy, occlBb);

      delete r;
      bw = occlBb.width();
      if (bw.x * bw.y > sideWallAreaThresForOccluder || bw.y * bw.z > sideWallAreaThresForOccluder)
        ; // OK, will use occluder
      else
      {
        hasOccl = false;
        debug("skip too small occluder box");
      }
    }
    else
      hasOccl = false;
  }
  // for (unsigned int elemNo = 0; elemNo < lod.rigid.meshData.elem.size(); elemNo++)
  //{
  //   debug("elemNo = %d, vertexData = 0x%08x, vdOrderIndex = %d, sv = %d, numv = %d, si = %d, "
  //     "numf = %d",
  //     elemNo,
  //     lod.rigid.meshData.elem[elemNo].vertexData.get(),
  //     lod.rigid.meshData.elem[elemNo].vdOrderIndex,
  //     lod.rigid.meshData.elem[elemNo].sv,
  //     lod.rigid.meshData.elem[elemNo].numv,
  //     lod.rigid.meshData.elem[elemNo].si,
  //     lod.rigid.meshData.elem[elemNo].numf);
  // }
  del_it(rayTracer);
  return true;
}

void RenderableInstanceLodsResSrc::setupMatSubst(const DataBlock &blk) { matSubst.setupMatSubst(blk); }

static RenderableInstanceLodsResSrc::Lod merge_lods(const RenderableInstanceLodsResSrc::Lod &lod1,
  const RenderableInstanceLodsResSrc::Lod &lod2)
{
  RenderableInstanceLodsResSrc::Lod result = lod1;
  for (const ShaderMeshData::RElem &elem : lod2.rigid.meshData.elems)
  {
    ShaderMeshData::RElem *newElem = result.rigid.meshData.addElem(elem.getStage());
    *newElem = elem;
  }

  return result;
}

bool RenderableInstanceLodsResSrc::build(const DataBlock &blk)
{
  // debug("building rendInst from %s", (char*)inputFileName);
  buildImpostorDataNow = false;
  impConvexPts.clear();
  impShadowPts.clear();
  impMinY = MAX_REAL, impMaxY = MIN_REAL, impCylRadSq = 0.f;
  impMaxFacingLeavesDelta = 0;
  impBboxAspectRatio = 1;

  bboxFromLod = blk.getInt("bboxFromLod", -1);
  useSymTex = blk.getBool("useSymTex", false);
  forceLeftSideMatrices = blk.getBool("forceLeftSideMatrices", false);
  doNotSplitLods = blk.getBool("dont_split_lods", false);

  int lodNameId = blk.getNameId("lod");
  int plodNameId = blk.getNameId("plod");

  LodsEqualMaterialGather matGather;
  Tab<AScene *> curScenes(tmpmem);

  int numBlk = blk.blockCount();
  int lastLodBlkIdx = -1, beforeLastLodBlkIdx = -1;
  for (int blkId = 0; blkId < numBlk; ++blkId)
    if (blk.getBlock(blkId)->getBlockNameId() == lodNameId)
    {
      beforeLastLodBlkIdx = lastLodBlkIdx;
      lastLodBlkIdx = blkId;
    }

  int lodId = 0;
  for (int blkId = 0; blkId < numBlk; ++blkId)
  {
    const DataBlock &lodBlk = *blk.getBlock(blkId);
    if (lodBlk.getBlockNameId() != lodNameId && lodBlk.getBlockNameId() != plodNameId)
      continue;

    const char *fileName = lodBlk.getStr("scene", NULL);
    real range = lodBlk.getReal("range", MAX_REAL);
    const DataBlock *materialOverrides = lodBlk.getBlockByNameEx("materialOverrides");

    buildImpostorDataNow = (blkId == beforeLastLodBlkIdx);
    if (!addLod(lodId, fileName, range, matGather, curScenes, *materialOverrides))
    {
      clear_all_ptr_items(curScenes);
      return false;
    }
    lodId++;
  }

  if (hasImpostor && blk.getNameId("transition_lod") != -1)
  {
    const DataBlock *block = blk.getBlockByName("transition_lod");
    int transitionRangeNameId = block->getNameId("transition_range");
    float transitionRange = block->getRealByNameId(transitionRangeNameId, 6.0f);

    String addMatScript;

    float transitionLodRange = 0;
    int lodId = 0;
    for (int blkId = 0; blkId < numBlk; ++blkId)
    {
      const DataBlock &lodBlk = *blk.getBlock(blkId);
      if (lodBlk.getBlockNameId() != lodNameId)
        continue;

      const DataBlock *materialOverrides = lodBlk.getBlockByNameEx("materialOverrides");
      const char *fileName = lodBlk.getStr("scene", nullptr);

      if (blkId == beforeLastLodBlkIdx)
      {
        transitionLodRange = lodBlk.getReal("range", MAX_REAL) + transitionRange;
        float crossDissolveMul = safediv(1.0f, transitionRange);
        float crossDissolveAdd = safediv(-(transitionLodRange - transitionRange), transitionRange);
        addMatScript = String(128, "\r\nuse_cross_dissolve=1\r\ncross_dissolve_mul=%f\r\ncross_dissolve_add=%f\r\n", crossDissolveMul,
          crossDissolveAdd);
        addLod(lodId, fileName, transitionLodRange, matGather, curScenes, *materialOverrides, addMatScript.c_str());
      }
      else if (blkId == lastLodBlkIdx)
      {
        addLod(lodId, fileName, transitionLodRange, matGather, curScenes, *materialOverrides, addMatScript.c_str());
      }
      lodId++;
    }

    int lodNum = lods.size();
    Lod tmpImpostorLod = lods[lodNum - 3];
    Lod &transitionLod = lods[lodNum - 3];
    Lod &impostorLod = lods[lodNum - 2];
    Lod &lastMeshLod = lods[lodNum - 4];

    transitionLod = merge_lods(lods[lodNum - 2], lods[lodNum - 1]);
    impostorLod = eastl::move(tmpImpostorLod);
    lods.pop_back();

    transitionLod.range = transitionLodRange;
  }

  matGather.copyToMatSaver(matSaver, texSaver);
  clear_all_ptr_items(curScenes);

  int elemnum = 0;
  for (int i = 0; i < lods.size(); i++)
    elemnum += lods[i].rigid.meshData.elems.size();

  debug("%d lods, %d mats, %d texs, %d elems", lods.size(), matSaver.mats.size(), texSaver.textures.size(), elemnum);

  if (const DataBlock *texBlk = blk.getBlockByName("textures"))
  {
    int id = texBlk->getNameId("tex");

    for (int i = 0; i < texBlk->paramCount(); ++i)
    {
      if (texBlk->getParamType(i) != DataBlock::TYPE_STRING || texBlk->getParamNameId(i) != id)
        continue;
      const char *name = texBlk->getStr(i);
      textureSlots.push_back() = name;
    }
  }
  return true;
}

void RenderableInstanceLodsResSrc::processImpostorMesh(Mesh &m, dag::ConstSpan<ShaderMaterial *> mat)
{
  enum
  {
    MAT_SKIP = 0,
    MAT_OPAQUE = 1,
    MAT_FACING_LEAVES = 2
  };
  uint8_t *mat_type = (uint8_t *)alloca(mat.size());
  int flId = m.find_extra_channel(SCUSAGE_EXTRA, 0);

  for (int i = 0; i < mat.size(); i++)
  {
    int flg = 0;
    struct ECB : public ShaderChannelsEnumCB
    {
      void enum_shader_channel(int, int, int, int, int, ChannelModifier, int) override {}
    } cb;
    if (!mat[i] || !mat[i]->enum_channels(cb, flg))
    {
      mat_type[i] = MAT_SKIP;
      continue;
    }

    if ((flg & SC_STAGE_IDX_MASK) > ShaderMesh::STG_decal)
      mat_type[i] = MAT_SKIP;
    else if (flId >= 0 && strstr(mat[i]->getInfo().str(), "facing_leaves"))
      mat_type[i] = MAT_FACING_LEAVES;
    else
      mat_type[i] = MAT_OPAQUE;
  }

  // prepare vertex bitmarks (to prevent processing and adding the same vertex more than once)
  impTmp_vmark.resize(0);
  impTmp_vmark.resize(m.vert.size());
  impConvexPts.reserve(impConvexPts.size() + m.vert.size());

  // prepare data for facing leaves
  const MeshData::ExtraChannel *ec = nullptr;
  int ec_numverts = 0;
  if (flId >= 0)
  {
    ec = &m.getExtra(flId);
    G_ASSERTF(ec->type == MeshData::CHT_FLOAT3 || ec->type == MeshData::CHT_FLOAT4, "facing_leaves: chanId=%d type=%d", flId,
      ec->type);
    ec_numverts = ec->vsize ? ec->vt.size() / ec->vsize : 0;

    // prepare vertex bitmarks for facing_leaves (to prevent processing and adding the same vertex more than once)
    impTmp_vmark_fl.resize(0);
    impTmp_vmark_fl.resize(m.vert.size() * ec_numverts);
    impShadowPts.reserve(impShadowPts.size() + m.vert.size() * ec_numverts);
  }

  for (int i = 0; i < m.face.size(); i++)
  {
    // skip faces with non-relevant materials
    if (m.face[i].mat >= mat.size())
      continue;
    uint8_t mt = mat_type[m.face[i].mat];
    if (mt == MAT_SKIP)
      continue;

    for (int j = 0; j < 3; j++)
    {
      int vidx = m.face[i].v[j], eidx = -1;
      if (mt == MAT_OPAQUE)
      {
        if (impTmp_vmark[vidx])
          continue;
        impTmp_vmark.set(vidx);
      }
      else if (ec && mt == MAT_FACING_LEAVES)
      {
        G_ASSERT(ec);
        G_ANALYSIS_ASSUME(ec);

        eidx = ec->fc[i].t[j];
        if (impTmp_vmark_fl[vidx * ec_numverts + eidx])
          continue;
        impTmp_vmark_fl.set(vidx * ec_numverts + eidx);
      }

      Point3_vec4 point = m.vert[vidx];
      point.y = max(point.y, 0.f);
      float pointY = point.y;
      impMinY = min(pointY, impMinY);
      impConvexPts.push_back(point);

      if (mt == MAT_FACING_LEAVES)
      {
        Point3 delta = *(Point3 *)&ec->vt[eidx * ec->vsize];
        float maxDelta = max(fabsf(delta.x), fabsf(delta.y));
        impMaxY = max(pointY + maxDelta, impMaxY);
        impMaxFacingLeavesDelta = max(impMaxFacingLeavesDelta, maxDelta);
        impShadowPts.push_back(BBox3(point, delta));
      }
      else
        impMaxY = max(pointY, impMaxY);

      impCylRadSq = max(impCylRadSq, point.x * point.x + point.z * point.z);
    }
  }
}

void RenderableInstanceLodsResSrc::RigidObj::presave(ShaderMeshDataSaveCB &mdcb) { meshData.enumVertexData(mdcb); }

int RenderableInstanceLodsResSrc::Lod::save(mkbindump::BinDumpSaveCB &cwr, ShaderMeshDataSaveCB &mdcb)
{
  int sz_pos = cwr.tell();
  cwr.writePtr64e(0);
  cwr.writeInt32eAt(rigid.meshData.save(cwr, mdcb, true), sz_pos);

  return 8;
}


void RenderableInstanceLodsResSrc::presave()
{
  // connect vertex datas for all LODs
  GlobalVertexDataConnector gvd;

  for (int i = lods.size() - 1; i >= 0; i--)
  {
    gvd.addMeshData(&lods[i].rigid.meshData, Point3(0, 0, 0), 0);
    if (doNotSplitLods && i > 0)
      continue;
    gvd.connectData(false, NULL, VDATA_PACKED_IB | ((i << __bsf(VDATA_LOD_MASK)) & VDATA_LOD_MASK));
    gvd.clear();
  }

  for (int i = lods.size() - 1; i >= 0; i--)
    lods[i].rigid.presave(matSaver);
}

bool RenderableInstanceLodsResSrc::save(mkbindump::BinDumpSaveCB &cwr, const LodValidationSettings *lvs, ILogWriter &log_writer)
{
  BBox3 bbox(Point3(0, 0, 0), 0);
  BSphere3 bsph(Point3(0, 0, 0), 0);
  float bs0rad2 = 0;

  bool hasImpostorData = hasImpostor;
  if (hasImpostor && !impConvexPts.size() && impShadowPts.size())
  {
    logwarn("hasImpostor flag was set, but no impostor data gathered, resetting hasImpostorData flag");
    hasImpostorData = false;
  }

  if (lods.size() > 0)
  {
    bbox = lods[0].bbox;
    bsph = lods[0].bsph;
    bs0rad2 = lods[0].bs0rad2;
  }
  for (int i = 1; i < lods.size(); ++i)
  {
    bbox += lods[i].bbox;
    bsph += lods[i].bsph;
    if (bs0rad2 < lods[i].bs0rad2)
      bs0rad2 = lods[i].bs0rad2;
  }
  if (bboxFromLod >= 0 && bboxFromLod < lods.size())
  {
    bbox = lods[bboxFromLod].bbox;
    debug("using bbox from lod %d: %@", bboxFromLod, bbox);
  }

  DEBUG_CTX("bsph: " FMT_P3 " rad=%.3f, bbox=" FMT_P3 "-" FMT_P3 ", bs0rad=%.3f", P3D(bsph.c), bsph.r, P3D(bbox[0]), P3D(bbox[1]),
    sqrt(bs0rad2));

  if (hasOccl && !occlQuad)
    if (occlBb[0].x >= occlBb[1].x || occlBb[0].y >= occlBb[1].y || occlBb[0].z >= occlBb[1].z)
    {
      debug("discard bad occluder-box: %@", occlBb);
      hasOccl = false;
    }
  presave();

  // validate LODs
  bool validation_passed = true;
  if (lvs)
    for (unsigned l = 0, prev_lod_faces = ~0u, faces = 0; l < lods.size(); l++)
    {
      if (!lvs->checkLodValid(l, faces = lods[l].countFaces(), prev_lod_faces, lods[l].range, bbox, lods.size(), log_writer))
        validation_passed = false;
      prev_lod_faces = faces;
    }
  if (!validation_passed)
    return false;

  if (buildResultsBlk)
  {
    buildResultsBlk->setInt("lods", lods.size());
    buildResultsBlk->setInt("faces", lods[0].rigid.meshData.countFaces());
    if (hasImpostor)
      buildResultsBlk->setBool("hasImpostor", hasImpostor);
    buildResultsBlk->setPoint3("bbox0", bbox[0]);
    buildResultsBlk->setPoint3("bbox1", bbox[1]);
    buildResultsBlk->setPoint4("bsph", Point4::xyzV(bsph.c, bsph.r));
  }

  texSaver.prepareTexNames(textureSlots);

  static const int FIXED_HDR_SZ = mkbindump::BinDumpSaveCB::TAB_SZ + sizeof(bbox) + sizeof(bsph.c) + 3 * sizeof(float);
  constexpr unsigned occlQuadSize = sizeof(Point3[4]);
  unsigned extraSize =
    hasFloatingBox ? (occlQuadSize + sizeof(floatingBox)) : (hasOccl ? (occlQuad ? occlQuadSize : sizeof(occlBb)) : 0);
  int hdr_sz = FIXED_HDR_SZ + extraSize;
  unsigned flags =
    (hasImpostor ? (1 << 24) : 0) | (hasImpostorData ? (1 << 27) : 0) | (hasOccl ? (occlQuad ? (1 << 26) : (1 << 25)) : 0);
  unsigned extraFlags = hasFloatingBox ? RenderableInstanceLodsResource::HAS_FLOATING_BOX_FLG : 0;
  int lods_ofs = hdr_sz;
  hdr_sz += lods.size() * cwr.TAB_SZ;

  if (hasImpostorData)
  {
    BBox3 bbox;
    Tab<Point3_vec4> points;
    points.resize(impConvexPts.size());
    for (int i = 0; i < impConvexPts.size(); i++)
      bbox += points[i] = impConvexPts[i];
    bbox[0] -= Point3(impMaxFacingLeavesDelta, impMaxFacingLeavesDelta, impMaxFacingLeavesDelta);
    bbox[1] += Point3(impMaxFacingLeavesDelta, impMaxFacingLeavesDelta, impMaxFacingLeavesDelta);
    impBboxAspectRatio = safediv(bbox.width().y, max(bbox.width().x, bbox.width().z));

    ConvexHullComputer daHull;
    daHull.compute((float *)points.data(), elem_size(points), points.size(), 0.f, 0.f, true);
    if (points.size() != daHull.vertices.size())
    {
      points.resize(daHull.vertices.size());
      G_ASSERT(elem_size(points) == elem_size(daHull.vertices)); // otherwise copy per element
      mem_copy_from(points, daHull.vertices.data());
      impConvexPts.resize(points.size());
      for (int i = 0; i < points.size(); i++)
        impConvexPts[i] = points[i];
    }

    hdr_sz += int(sizeof(RenderableInstanceLodsResource::ImpostorData) + data_size(impConvexPts) + data_size(impShadowPts));
  }

  // write RenderableInstanceLodsResource resSize/hasImpostorMat/_resv7b word
  cwr.writeInt32e(hdr_sz + flags);

  // write materials
  bool write_mat_sep = sepMatToBuildResultsBlk && buildResultsBlk != nullptr && matSaver.mats.size();
  if (write_mat_sep)
  {
    matSaver.writeMatVdataCombinedNoMat(cwr);
    texSaver.writeTexToBlk(*buildResultsBlk);
    matSaver.writeMatToBlk(*buildResultsBlk, texSaver, "mat");
  }
  else
  {
    matSaver.writeMatVdataHdr(cwr, texSaver);
    texSaver.writeTexStr(cwr);
    matSaver.writeMatVdata(cwr, texSaver);
  }


  {
    for (int lodIdx = 0; lodIdx < lods.size(); lodIdx++)
    {
      RenderableInstanceLodsResSrc::Lod &lod = lods[lodIdx];
      float minTextureScale = FLT_MAX;
      for (ShaderMeshData::RElem &elem : lod.rigid.meshData.elems)
      {
        minTextureScale = min(minTextureScale, elem.textureScale);
      }
      lod.texScale = 1.0f / (minTextureScale * minTextureScale);
    }

    if (write_mat_sep)
    {
      DataBlock *b = buildResultsBlk->addBlock("texScale_data");
      b->clearData();
      for (int lodIdx = 0; lodIdx < lods.size(); lodIdx++)
        b->setReal(String(0, "texScale%d", lodIdx), lods[lodIdx].texScale);
    }
  }

  // write RenderableInstanceLodsResource data
  cwr.writeRef(lods_ofs, lods.size());
  cwr.write32ex(&bbox, sizeof(bbox));
  cwr.write32ex(&bsph.c, sizeof(bsph.c));
  cwr.writeReal(bsph.r);
  cwr.writeReal(sqrt(bs0rad2));
  int impostor_data_ref_pos = cwr.tell();
  cwr.writeInt16e(0);
  cwr.writeInt16e(extraFlags);
  int floatingBoxPadBytes = occlQuadSize;
  if (hasOccl)
  {
    if (occlQuad)
    {
      G_ASSERT(occlMesh.vert.size() == 4);
      cwr.write32ex(occlMesh.vert.data(), occlQuadSize);
      floatingBoxPadBytes = 0;
    }
    else
    {
      cwr.write32ex(&occlBb, sizeof(occlBb));
      floatingBoxPadBytes -= sizeof(occlBb);
    }
  }
  if (hasFloatingBox)
  {
    for (int i = 0; i < floatingBoxPadBytes / 4; ++i) //-V1008
      cwr.writeInt32e(0);
    cwr.write32ex(&floatingBox, sizeof(floatingBox));
  }

  for (int i = 0; i < lods.size(); ++i)
  {
    lods[i].szPos = cwr.tell();

    cwr.writePtr64e(0);
    cwr.writeReal(lods[i].range);
    cwr.writeReal((!write_mat_sep) ? lods[i].texScale : 0);
  }

  if (hasImpostorData)
  {
    int data_ofs = lods_ofs + lods.size() * cwr.TAB_SZ;
    G_ASSERT(data_ofs < (1 << 16));
    cwr.writeInt16eAt(data_ofs, impostor_data_ref_pos);
    data_ofs += (int)sizeof(RenderableInstanceLodsResource::ImpostorData);

    // write ImpostorData data
    cwr.writePtr64e(data_ofs);                           // PatchablePtr<Point3> pointsPtr;
    cwr.writePtr64e(data_ofs + data_size(impConvexPts)); // PatchablePtr<BBox3> shadowPointsPtr;
    cwr.writeInt16e(impConvexPts.size());                // uint16_t pointsCount;
    cwr.writeInt16e(impShadowPts.size());                // uint16_t shadowPointsCount;
    cwr.writeReal(impMinY);                              // float minY;
    cwr.writeReal(impMaxY);                              // float maxY;
    cwr.writeReal(sqrtf(impCylRadSq));                   // float cylRad;
    cwr.writeReal(impMaxFacingLeavesDelta);              // float maxFacingLeavesDelta;
    cwr.writeReal(impBboxAspectRatio);                   // float bboxAspectRatio;

    cwr.write32ex(impConvexPts.data(), data_size(impConvexPts));
    cwr.write32ex(impShadowPts.data(), data_size(impShadowPts));

    if (0)
    {
      DEBUG_DUMP_VAR(impConvexPts.size());
      DEBUG_DUMP_VAR(impShadowPts.size());
      DEBUG_DUMP_VAR(impMinY);
      DEBUG_DUMP_VAR(impMaxY);
      DEBUG_DUMP_VAR(impCylRadSq);
      DEBUG_DUMP_VAR(impMaxFacingLeavesDelta);
      DEBUG_DUMP_VAR(impBboxAspectRatio);
      for (int i = 0; i < impConvexPts.size(); i++)
        debug("  convexPoints[%d]=%@", i, impConvexPts[i]);
      for (int i = 0; i < impShadowPts.size(); i++)
        debug("  shadowPoints[%d]=%@", i, impShadowPts[i]);
    }
  }

  // write referenced resources (rigids only)
  for (int i = 0; i < lods.size(); ++i)
    cwr.writeInt32eAt(lods[i].save(cwr, matSaver), lods[i].szPos);
  return true;
}
