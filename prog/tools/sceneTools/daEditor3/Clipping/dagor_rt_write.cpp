#include "clipping_builder.h"

#include <de3_interface.h>
#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/binDumpUtil.h>

#include <scene/dag_physMat.h>
#include <sceneRay/dag_sceneRay.h>

#include <osApiWrappers/dag_direct.h>
#include <util/dag_oaHashNameMap.h>

#include <debug/dag_debug.h>
#include <debug/dag_log.h>


class DagorRayTracerBuilder : public IClippingDumpBuilder
{
  BuildableStaticSceneRayTracer *rt;
  NameMap matName;
  Tab<unsigned char> matId;
  Mesh boxMesh;
  int fcnt, vcnt;

public:
  DagorRayTracerBuilder() : rt(NULL), matId(tmpmem)
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

    fcnt = 0;
    vcnt = 0;
  }

  ~DagorRayTracerBuilder() { clear(); }

  void clear()
  {
    del_it(rt);
    matName.clear();
    clear_and_shrink(matId);
  }

  virtual void destroy() { delete this; }

  virtual void start(const CollisionBuildSettings &stg) { rt = new BuildableStaticSceneRayTracer(stg.leafSize(), stg.levels); }

  virtual void addCapsule(Capsule &c, int obj_id, int obj_flags, const char *physmat)
  {
    if (!rt)
    {
      debug_ctx("addCapsule without start");
      return;
    }

    debug_ctx("addCapsule not implemented!");
    // rt->addcapsule ( c, obj_id, obj_flags, physmat_id );
  }
  virtual void addSphere(const Point3 &c, float rad, int obj_id, int obj_flags, const char *physmat)
  {
    G_ASSERT(0 && "addSphere not implemented");
  }
  virtual void addBox(const TMatrix &box_tm, int obj_id, int obj_flags, const char *physmat)
  {
    int pm_id = PhysMat::getMaterialId(physmat ? physmat : "::default");
    const PhysMat::MaterialData &pm = PhysMat::getMaterial(pm_id);

    if (pm.camera_collision)
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
  virtual void addConvexHull(Mesh &m, MaterialDataList *mat, TMatrix &wtm, int obj_id, int obj_flags, int physmat_id, bool pmid_force,
    StaticGeometryMesh *sgm = NULL)
  {
    G_ASSERT(0 && "addSphere not implemented");
  }

  virtual void addMesh(Mesh &m, MaterialDataList *mat, const TMatrix &wtm, int obj_id, int obj_flags, int physmat_id, bool pmid_force,
    StaticGeometryMesh *sgm)
  {
    if (!rt)
    {
      debug_ctx("addMesh without start");
      return;
    }

    if (&wtm != &TMatrix::IDENT && wtm != TMatrix::IDENT)
    {
      for (int i = 0; i < m.vert.size(); ++i)
        m.vert[i] = wtm * m.vert[i];
    }

    fcnt += m.face.size();
    vcnt += m.vert.size();

    rt->addmesh(m.vert.data(), m.vert.size(), (unsigned *)&m.face[0], elem_size(m.face), m.face.size(), NULL, false);

    // setup pmid for each face
    int base_f = append_items(matId, m.face.size());
    String pmname;

    const PhysMat::MaterialData &pm = PhysMat::getMaterial(physmat_id);
    physmat_id = matName.addNameId((physmat_id == PHYSMAT_INVALID) ? "::default" : pm.name);

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
            // if ( PhysMat::getMaterialId ( pmname ) == PHYSMAT_INVALID )
            //   logerr ...
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

  virtual bool finishAndWrite(const char * /*temp_fname*/, IGenSave &cwr, unsigned target_code)
  {
    if (!rt)
    {
      debug_ctx("finishAndWrite without start");
      return false;
    }

    bool big_endian = dagor_target_code_be(target_code);

    rt->rebuild();

    // write FRT dump
    mkbindump::BinDumpSaveCB bdcwr(256 << 10, target_code, big_endian);
    bdcwr.writeFourCC(MAKE4C(0xFF, 'v', '1', 0xFF));

    int dumplen = 0;
    if (!rt->serialize(bdcwr.getRawWriter(), big_endian, &dumplen))
    {
      logerr("Can't save raytracer dump");
      return false;
    }
    bdcwr.writeTabDataRaw(matId); // because of 8-bit elemsize
    bdcwr.align4();

    bdcwr.beginBlock();
    bdcwr.setOrigin();
    bdcwr.writeRef(bdcwr.TAB_SZ, matName.nameCount());
    bdcwr.writeZeroes(bdcwr.PTR_SZ * matName.nameCount());
    for (int i = 0; i < matName.nameCount(); i++)
    {
      bdcwr.writeInt32eAt(bdcwr.tell(), bdcwr.TAB_SZ + bdcwr.PTR_SZ * i);
      bdcwr.writeRaw(matName.getName(i), (int)strlen(matName.getName(i)) + 1);
    }
    bdcwr.align4();
    bdcwr.popOrigin();
    bdcwr.endBlock();

    unsigned physmats_used = matName.nameCount();
    for (unsigned i = 0; i < physmats_used; i++)
      debug("  physmat[%d]=%s", i, matName.getName(i));
    clear();

    // try load dump (only when built for PC)
    if (target_code == _MAKE4C('PC'))
    {
      DeserializedStaticSceneRayTracer *drt = new DeserializedStaticSceneRayTracer;
      MemoryLoadCB crd(bdcwr.getMem(), false);
      crd.seekrel(4);

      if (!drt->serializedLoad(crd))
      {
        logerr("Can't load raytracer dump from");
        delete drt;
        return false;
      }

      delete drt;
    }

    if (fcnt && vcnt)
      bdcwr.copyDataTo(cwr);
    DAEDITOR3.conNote("FRT dump size: %dK, packed: %dK;  %d faces, %d verts (%d phys mats used)", dumplen >> 10, bdcwr.getSize() >> 10,
      fcnt, vcnt, physmats_used);

    return true;
  }

  virtual IClippingDumpBuilder *getSeparateRayTracer() { return NULL; }
};


IClippingDumpBuilder *create_dagor_raytracer_dump_builder() { return new (tmpmem) DagorRayTracerBuilder; }
