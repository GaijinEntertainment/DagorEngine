// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "collision_builder.h"

#include <de3_interface.h>
#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/util/makeBindump.h>

#include <scene/dag_physMat.h>
#include <gameRes/collisionResourceBuilder.h>
#include <gameRes/collResStream.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_btagCompr.h>

#include <osApiWrappers/dag_direct.h>
#include <util/dag_oaHashNameMap.h>

#include <debug/dag_debug.h>


// constraint: getMaterial asserts and reads mats[0] on an empty table
static const char *phys_mat_name(int id)
{
  return PhysMat::getMaterials().empty() ? "::default" : PhysMat::getMaterial(id).name.str();
}
static bool phys_mat_camera_collision(int id) { return PhysMat::getMaterials().empty() || PhysMat::getMaterial(id).camera_collision; }

static bool write_collision_stream(IGenSave &cwr, const char *name, CollisionResourceBuilder &builder, bool check_jolt,
  bool fail_on_jolt_degenerate)
{
  const unsigned label = COLLRES_STREAM_MAGIC | COLLRES_STREAM_VERSION;
  mkbindump::BinDumpSaveCB mcwr(1 << 20, _MAKE4C('PC'), /*be_target*/ false);
  if (!builder.write(mcwr.getRawWriter(), name, &phys_mat_name))
  {
    DAEDITOR3.conError("%s: the stream cannot be written", name);
    return false;
  }

  mkbindump::BinDumpSaveCB out(mcwr.getSize() + 16, mcwr);
  out.writeInt32e(label);
  out.beginBlock();
  mkbindump::BinDumpSaveCB zcwr(mcwr.getSize(), mcwr);
  MemoryLoadCB mcrd(mcwr.getRawWriter().getMem(), false);
  if (mcwr.getSize() >= 512)
    zstd_compress_data(zcwr.getRawWriter(), mcrd, mcwr.getSize(), 256 << 10, 19);
  if (zcwr.getSize() && zcwr.getSize() < (int64_t)mcwr.getSize() * 8 / 10 && zcwr.getSize() + 256 < mcwr.getSize())
  {
    zcwr.copyDataTo(out.getRawWriter());
    out.endBlock(btag_compr::ZSTD);
  }
  else
  {
    mcwr.copyDataTo(out.getRawWriter());
    out.endBlock(btag_compr::NONE);
  }

  int landedNodes = 0;
  DAGOR_TRY
  {
    MemoryLoadCB acrd(out.getRawWriter().getMem(), false);
    CollisionResource back(acrd, /*res_id*/ -1, name);
    landedNodes = (int)back.getAllNodes().size();
    if (!landedNodes)
    {
      DAEDITOR3.conError("%s: the stream loads as an empty resource", name);
      return false;
    }
    if (check_jolt && !back.validateVerticesForJolt(name))
    {
      if (fail_on_jolt_degenerate)
      {
        DAEDITOR3.conError("%s: Jolt-degenerate faces, and joltDegenerativeTriFailExport is set", name);
        return false;
      }
      DAEDITOR3.conWarning("%s: Jolt-degenerate faces (see the log)", name);
    }
  }
  DAGOR_CATCH(IGenLoad::LoadException)
  {
    DAEDITOR3.conError("%s: the stream does not load back", name);
    return false;
  }
  out.copyDataTo(cwr);
  DAEDITOR3.conNote("%s: stream %dK, packed %dK, %d nodes", name, mcwr.getSize() >> 10, out.getSize() >> 10, landedNodes);
  return true;
}

static int fill_collision_builder(CollisionResourceBuilder &builder, const char *name, uint16_t behavior_flags,
  dag::ConstSpan<Point3_vec4> verts, dag::ConstSpan<uint32_t> idx, dag::ConstSpan<int16_t> pmid)
{
  const int parts = builder.addSplitMeshNodes(make_span_const(&behavior_flags, 1), verts, idx, pmid);
  if (parts <= 0)
  {
    DAEDITOR3.conError("%s: no node for %d faces, the split answered %d", name, (int)idx.size() / 3, parts);
    return -1;
  }
  builder.recomputeBounds();
  builder.collapse(name);
  builder.collisionFlags |= COLLISION_RES_FLAG_BLAS_TWO_SIDED;
  return parts;
}

class DagorCollisionBuilder : public ICollisionDumpBuilder
{
protected:
  dag::Vector<Point3_vec4> verts;
  dag::Vector<uint32_t> faces; // index triples into verts
  NameMap matName;
  Tab<unsigned char> matId; // one per face: its physmat's index in matName
  Mesh boxMesh;
  String cookName; // each cook labels its own console lines

public:
  DagorCollisionBuilder(const char *cook_name) : matId(tmpmem), cookName(cook_name)
  {
    supportMask = SUPPORT_BOX;

    boxMesh.vert.resize(8);
    mem_set_0(boxMesh.vert);

    boxMesh.face.resize(12);
    boxMesh.face[0].set(0, 1, 2, 0, 1);
    boxMesh.face[1].set(2, 3, 0, 0, 1);
    boxMesh.face[2].set(4, 5, 6, 0, 1);
    boxMesh.face[3].set(6, 7, 4, 0, 1);
    boxMesh.face[4].set(2, 6, 5, 0, 1);
    boxMesh.face[5].set(5, 3, 2, 0, 1);
    boxMesh.face[6].set(0, 3, 5, 0, 1);
    boxMesh.face[7].set(5, 4, 0, 0, 1);
    boxMesh.face[8].set(0, 4, 7, 0, 1);
    boxMesh.face[9].set(7, 1, 0, 0, 1);
    boxMesh.face[10].set(1, 7, 6, 0, 1);
    boxMesh.face[11].set(6, 2, 1, 0, 1);
  }

  // destroy() is delete this, and the game clip builder derives from this one with members of its own.
  virtual ~DagorCollisionBuilder() = default;

  void destroy() override { delete this; }

protected:
  void resolvePhysMatIds(Tab<int16_t> &out_runtime_id) const
  {
    const int matCount = matName.nameCount();
    out_runtime_id.resize(matCount);
    if (!PhysMat::getMaterials().size()) // one warning for the table, not one per material
    {
      mem_set_0(out_runtime_id);
      if (matCount)
        DAEDITOR3.conWarning("%s: the physmat table is empty; every face takes the default", cookName.str());
      return;
    }
    for (int i = 0; i < matCount; ++i)
    {
      const int id = PhysMat::getMaterialId(matName.getName(i));
      if (id == PHYSMAT_DEFAULT && dd_stricmp(phys_mat_name(id), matName.getName(i)) != 0)
        DAEDITOR3.conWarning("%s: physmat <%s> is not in the table; its faces take the default", cookName.str(), matName.getName(i));
      debug("%s: physmat[%d] = %s -> %d", cookName.str(), i, matName.getName(i), id);
      out_runtime_id[i] = (int16_t)id;
    }
  }

public:
  void addBox(const TMatrix &box_tm, int obj_id, int obj_flags, const char *physmat) override
  {
    int pm_id = PhysMat::getMaterialId(physmat ? physmat : "::default");
    if (phys_mat_camera_collision(pm_id))
    {
      boxMesh.vert[0] = -box_tm.getcol(0) - box_tm.getcol(1) + box_tm.getcol(2);
      boxMesh.vert[1] = -box_tm.getcol(0) + box_tm.getcol(1) + box_tm.getcol(2);
      boxMesh.vert[2] = +box_tm.getcol(0) + box_tm.getcol(1) + box_tm.getcol(2);
      boxMesh.vert[3] = +box_tm.getcol(0) - box_tm.getcol(1) + box_tm.getcol(2);

      boxMesh.vert[4] = -box_tm.getcol(0) - box_tm.getcol(1) - box_tm.getcol(2);
      boxMesh.vert[5] = +box_tm.getcol(0) - box_tm.getcol(1) - box_tm.getcol(2);
      boxMesh.vert[6] = +box_tm.getcol(0) + box_tm.getcol(1) - box_tm.getcol(2);
      boxMesh.vert[7] = -box_tm.getcol(0) + box_tm.getcol(1) - box_tm.getcol(2);

      for (int i = 0; i < 8; i++)
        boxMesh.vert[i] += box_tm.getcol(3);

      addMesh(boxMesh, NULL, TMatrix::IDENT, obj_id, obj_flags, pm_id, true, NULL);
    }
  }

  void addMesh(Mesh &m, MaterialDataList *mat, const TMatrix &wtm, int obj_id, int obj_flags, int physmat_id, bool pmid_force,
    StaticGeometryMesh *sgm) override
  {
    const bool placed = &wtm != &TMatrix::IDENT && wtm != TMatrix::IDENT;
    const uint32_t base = (uint32_t)verts.size();
    verts.reserve(base + m.vert.size());
    for (int i = 0; i < m.vert.size(); ++i)
    {
      const Point3 p = placed ? wtm * m.vert[i] : m.vert[i];
      Point3_vec4 &v = verts.push_back();
      v.x = p.x, v.y = p.y, v.z = p.z, v.resv = 1.f;
    }
    faces.reserve(faces.size() + m.face.size() * 3);
    for (int i = 0; i < m.face.size(); ++i)
      for (int k = 0; k < 3; ++k)
        faces.push_back(base + m.face[i].v[k]);

    int base_f = append_items(matId, m.face.size());
    String pmname;

    physmat_id = matName.addNameId((physmat_id == PHYSMAT_INVALID) ? "::default" : phys_mat_name(physmat_id));

    if (mat)
    {
      int matnum = mat->subMatCount();

      for (int i = 0; i < m.face.size(); i++)
      {
        int pmid = physmat_id;

        if (matnum > 0 && !pmid_force)
        {
          int matn = m.face[i].mat % matnum;
          if (::getPhysMatNameFromMatName(mat->getSubMat(matn), pmname))
          {
            pmid = matName.addNameId(pmname);
          }
        }

        matId[base_f + i] = pmid;
      }
    }
    else if (sgm)
    {
      for (int i = 0; i < m.face.size(); ++i)
      {
        int pmid = physmat_id;

        if (!pmid_force)
        {
          const int matIdx = m.face[i].mat;

          if (matIdx >= 0 && matIdx < sgm->mats.size())
          {
            if (sgm->mats[matIdx] && ::getPhysMatNameFromMatName(sgm->mats[matIdx]->name, pmname))
              pmid = matName.addNameId(pmname);
          }
          else
            debug("Material index [%i] in face #%i more then materials count [%i]", matIdx, i, sgm->mats.size());
        }

        matId[base_f + i] = pmid;
      }
    }
    else
    {
      for (int i = 0; i < m.face.size(); ++i)
        matId[base_f + i] = physmat_id;
    }
  }
};


// The game's static collision: the gathered scene written as a cooked collision asset is.
// Its water goes to a second stream of its own, which finishAndWriteWater hands over.
class StaticCollisionDumpBuilder final : public DagorCollisionBuilder
{
  String physmatPath;
  bool failOnJoltDegenerate;
  DynamicMemGeneralSaveCB waterStream;

  bool writeStream(IGenSave &cwr, const char *name, uint16_t behavior_flags, dag::ConstSpan<uint32_t> idx,
    dag::ConstSpan<int16_t> pmid)
  {
    CollisionResourceBuilder builder;
    const int parts = fill_collision_builder(builder, name, behavior_flags, make_span_const(verts), idx, pmid);
    const bool checkJolt = (behavior_flags & CollisionNode::PHYS_COLLIDABLE) != 0;
    if (parts < 0 || !write_collision_stream(cwr, name, builder, checkJolt, failOnJoltDegenerate))
      return false;
    DAEDITOR3.conNote("%s: %d faces -> %d parts", name, (int)idx.size() / 3, parts);
    return true;
  }

public:
  StaticCollisionDumpBuilder(const char *physmat_path, bool fail_on_jolt_degenerate) :
    DagorCollisionBuilder("static collision"),
    physmatPath(physmat_path),
    failOnJoltDegenerate(fail_on_jolt_degenerate),
    waterStream(tmpmem, 0, 64 << 10)
  {}
  bool finishAndWrite(IGenSave &cwr) override
  {
    DataBlock physmatBlk;
    const DataBlock *matsBlk = !physmatPath.empty() && physmatBlk.load(physmatPath) ? physmatBlk.getBlockByName("PhysMats") : nullptr;
    if (!matsBlk)
    {
      DAEDITOR3.conError("%s: no PhysMats in <%s>; a water face would ship as solid collision", cookName.str(),
        physmatPath.empty() ? "(no physmat blk configured)" : physmatPath.str());
      return false;
    }
    const int matCount = matName.nameCount();
    Tab<int16_t> matRuntimeId(tmpmem);
    Tab<uint8_t> matIsWater(tmpmem);
    resolvePhysMatIds(matRuntimeId);
    matIsWater.resize(matCount);
    for (int i = 0; i < matCount; ++i)
    {
      const DataBlock *matBlk = matsBlk->getBlockByName(matName.getName(i));
      matIsWater[i] = matBlk && matBlk->getBool("isWater", false);
    }
    const int faceCount = (int)faces.size() / 3;
    if (faceCount <= 0)
    {
      DAEDITOR3.conNote("%s: no faces, nothing written", cookName.str());
      return true;
    }
    // The scene and its water are two resources: the water is traced alone, and nothing else may trace or collide with it.
    Tab<uint32_t> sceneIdx(tmpmem), waterIdx(tmpmem);
    Tab<int16_t> scenePmid(tmpmem), waterPmid(tmpmem);
    for (int f = 0; f < faceCount; ++f)
    {
      const bool isWater = matIsWater[matId[f]] != 0;
      Tab<uint32_t> &idx = isWater ? waterIdx : sceneIdx;
      for (int k = 0; k < 3; ++k)
        idx.push_back(faces[f * 3 + k]);
      (isWater ? waterPmid : scenePmid).push_back(matRuntimeId[matId[f]]);
    }

    if (!waterIdx.empty() &&
        !writeStream(waterStream, "water", CollisionNode::TRACEABLE, make_span_const(waterIdx), make_span_const(waterPmid)))
      return false;
    if (sceneIdx.empty())
    {
      DAEDITOR3.conNote("%s: water only, no scene faces", cookName.str());
      return true;
    }
    if (!writeStream(cwr, "frt", CollisionNode::TRACEABLE | CollisionNode::PHYS_COLLIDABLE, make_span_const(sceneIdx),
          make_span_const(scenePmid)))
      return false;
    DAEDITOR3.conNote("%s: %d faces (%d water), %d verts, %d phys mats", cookName.str(), faceCount, (int)waterIdx.size() / 3,
      (int)verts.size(), matCount);
    return true;
  }

  bool finishAndWriteWater(IGenSave &cwr) override
  {
    if (!waterStream.size())
      return false;
    cwr.write(waterStream.data(), waterStream.size());
    return true;
  }
};


class EditorCollisionBuilder final : public DagorCollisionBuilder
{
  static constexpr uint16_t CLIP_BEHAVIOR = CollisionNode::TRACEABLE | CollisionNode::PHYS_COLLIDABLE;

  // parts of the cooked soup, 0 with no geometry and -1 for a soup that made no node
  int fillClipBuilder(CollisionResourceBuilder &builder)
  {
    const int faceCount = (int)faces.size() / 3;
    if (faceCount <= 0)
    {
      DAEDITOR3.conNote("%s: no geometry, nothing built", cookName.str());
      return 0;
    }
    Tab<int16_t> matRuntimeId(tmpmem);
    resolvePhysMatIds(matRuntimeId);
    Tab<int16_t> facePmid(tmpmem);
    facePmid.resize(faceCount);
    for (int f = 0; f < faceCount; ++f)
      facePmid[f] = matRuntimeId[matId[f]];
    const int parts = fill_collision_builder(builder, cookName, CLIP_BEHAVIOR, make_span_const(verts), make_span_const(faces),
      make_span_const(facePmid));
    if (parts > 0)
      DAEDITOR3.conNote("%s: %d faces, %d verts, %d phys mats -> %d parts", cookName.str(), faceCount, (int)verts.size(),
        matName.nameCount(), parts);
    return parts;
  }

public:
  EditorCollisionBuilder(const char *cook_name) : DagorCollisionBuilder(cook_name) {}

  bool finishAndWrite(IGenSave &cwr) override
  {
    CollisionResourceBuilder builder;
    const int parts = fillClipBuilder(builder);
    if (parts <= 0)
      return parts == 0;
    return write_collision_stream(cwr, cookName, builder, /*check_jolt*/ false, /*fail_on_jolt_degenerate*/ false);
  }

  CollisionResource *finishAndBuild() override
  {
    CollisionResourceBuilder builder;
    return fillClipBuilder(builder) > 0 ? builder.build(cookName) : nullptr;
  }
};


ICollisionDumpBuilder *create_editor_collision_builder(const char *cook_name)
{
  return new (tmpmem) EditorCollisionBuilder(cook_name);
}
ICollisionDumpBuilder *create_static_collision_builder(const char *physmat_path, bool fail_on_jolt_degenerate)
{
  return new (tmpmem) StaticCollisionDumpBuilder(physmat_path, fail_on_jolt_degenerate);
}
