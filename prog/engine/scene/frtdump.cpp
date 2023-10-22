#include <scene/dag_frtdumpInline.h>
#include <sceneRay/dag_sceneRay.h>

#include <scene/dag_physMat.h>
#include <util/dag_roNameMap.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>


int FastRtDump::customUsageMask[FastRtDump::CUSTOM__NUM] = {
  /*CUSTOM_Default*/ StaticSceneRayTracer::CULL_BOTH,
  /*CUSTOM_Bullets*/ StaticSceneRayTracer::USER_FLAG4,
  /*CUSTOM_Camera*/ StaticSceneRayTracer::USER_FLAG3,
  /*CUSTOM_Character*/ StaticSceneRayTracer::USER_VISIBLE,
  /*CUSTOM_Character2*/ StaticSceneRayTracer::USER_FLAG5,
  /*CUSTOM_Character3*/ StaticSceneRayTracer::USER_FLAG6,
};

int FastRtDump::customSkipMask[FastRtDump::CUSTOM__NUM] = {
  /*CUSTOM_Default*/ StaticSceneRayTracer::USER_INVISIBLE,
  /*CUSTOM_Bullets*/ 0,
  /*CUSTOM_Camera*/ 0,
  /*CUSTOM_Character*/ 0,
  /*CUSTOM_Character2*/ 0,
  /*CUSTOM_Character3*/ 0,
};


//==============================================================================
FastRtDump::FastRtDump() : rt(NULL), active(true) {}


//==============================================================================
FastRtDump::FastRtDump(IGenLoad &crd) : rt(NULL), active(true) { loadData(crd); }


//==============================================================================
FastRtDump::FastRtDump(StaticSceneRayTracer *_rt) : rt(NULL), active(true) { setData(_rt); }


//==============================================================================
void FastRtDump::unloadData()
{
  del_it(rt);
  clear_and_shrink(pmid);
}


//==============================================================================
void FastRtDump::loadData(IGenLoad &crd)
{
  unloadData();

  DeserializedStaticSceneRayTracer *drt = NULL;

  int versionId = crd.readInt();
  if (versionId == MAKE4C(0xff, 'v', '1', 0xff))
  {
    int start_pos = crd.tell();
    drt = new (midmem) DeserializedStaticSceneRayTracer;

    if (!drt->serializedLoad(crd))
    {
      delete drt;
      return;
    }

    // read phys mat id for faces
    clear_and_resize(pmid, drt->getFacesCount());
    crd.readTabData(pmid);

    // align on 4
    start_pos = crd.tell() - start_pos;
    if (start_pos & 3)
      crd.seekrel(4 - (start_pos & 3));

    // read mat names
    int nm_sz = crd.readInt();
    void *mem = memalloc(nm_sz, tmpmem);
    crd.read(mem, nm_sz);

    RoNameMap &nm = *(RoNameMap *)mem;
    nm.patchData(mem);

    // resolve phys materials
    signed char *matid = new signed char[nm.map.size()];
    for (int i = 0; i < nm.map.size(); ++i)
    {
      const char *matname = nm.map[i];
      int id = PhysMat::getMaterialId(matname);

      if (id == PHYSMAT_INVALID)
      {
        logerr("physmat %s not found; setting as default", matname);
        id = PHYSMAT_DEFAULT;
      }
      G_ASSERT(id >= 0 && id <= 255);
      matid[i] = id;
    }

    // remap phys mat id for faces
    for (int i = 0; i < pmid.size(); ++i)
    {
      G_ASSERT(pmid[i] < nm.map.size());
      pmid[i] = matid[pmid[i]];
    }

    // free temp data
    delete[] matid;
    memfree(mem, tmpmem);
  }

  if (!PhysMat::physMatCount())
    goto finish;

  // setup face flags
  for (int i = drt ? drt->getFaceIndicesCount() - 1 : -1; i >= 0; --i)
  {
    StaticSceneRayTracer::FaceIndex &fi = drt->faceIndices(i);
    fi.flags &= ~(StaticSceneRayTracer::USER_FLAG3 | StaticSceneRayTracer::USER_FLAG4 | StaticSceneRayTracer::USER_FLAG5 |
                  StaticSceneRayTracer::USER_FLAG6);

    const PhysMat::MaterialData &m = PhysMat::getMaterial(pmid[fi.index]);

    if (!m.physics_collision)
      fi.flags |= StaticSceneRayTracer::USER_INVISIBLE;
    if (m.characters_collision)
      fi.flags |= StaticSceneRayTracer::USER_VISIBLE;
    if (m.camera_collision)
      fi.flags |= StaticSceneRayTracer::USER_FLAG3;
    if (m.bullets_collision)
      fi.flags |= StaticSceneRayTracer::USER_FLAG4;
    if (m.characters_collision2)
      fi.flags |= StaticSceneRayTracer::USER_FLAG5;
    if (m.characters_collision3)
      fi.flags |= StaticSceneRayTracer::USER_FLAG6;
  }

finish:
  rt = drt;      // loading complete
  active = true; // reload - clear prev value
}

void FastRtDump::setData(StaticSceneRayTracer *_rt)
{
  unloadData();

  rt = _rt;
  clear_and_resize(pmid, rt->getFacesCount());
  for (int i = 0; i < pmid.size(); ++i)
    pmid[i] = PHYSMAT_DEFAULT;
}

void FastRtDump::activate(bool active_)
{
  // G_ASSERT(active_ != active);
  active = active_;
}
