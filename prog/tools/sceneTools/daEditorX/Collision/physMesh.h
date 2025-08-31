// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_hierGrid.h>
#include <libTools/staticGeom/matFlags.h>

#define TWO_SIDED_SHIFT 0.002f


struct PhysMesh
{
  struct Face
  {
    int v[3];
    int matId;
  };

  Tab<Face> face;
  Tab<Point3> vert;

public:
  PhysMesh() : face(tmpmem), vert(tmpmem) {}

  void attachMesh(const PhysMesh &m, bool search_vert = false)
  {
    int oldVertCnt = search_vert ? vert.size() : 0;

    vmap.resize(m.vert.size());
    mem_set_ff(vmap);

    int start_face = append_items(face, m.face.size());
    for (int i = 0; i < m.face.size(); i++)
    {
      Face &f = face[start_face + i];

      for (int vi = 0; vi < 3; vi++)
      {
        int ov = m.face[i].v[vi];
        int nv = vmap[ov];

        if (nv == -1)
        {
          if (oldVertCnt)
          {
            const Point3 *__restrict pv = vert.data(), *__restrict pv_end = pv + oldVertCnt;
            const Point3 *__restrict pnv = &m.vert[ov];
            for (; pv < pv_end; pv++)
              if (memcmp(pv, pnv, sizeof(*pnv)) == 0)
              {
                vmap[ov] = nv = pv - vert.data();
                break;
              }
          }
          if (nv == -1)
          {
            vmap[ov] = nv = vert.size();
            vert.push_back(m.vert[ov]);
          }
        }
        f.v[vi] = nv;
      }
      f.matId = m.face[i].matId;
    }
    vmap.clear();
  }

  void attachMeshSel(const PhysMesh &m, dag::ConstSpan<int> sel)
  {
    vmap.resize(m.vert.size());
    mem_set_ff(vmap);

    int start_face = append_items(face, sel.size());
    for (int si = 0; si < sel.size(); si++)
    {
      int i = sel[si];
      Face &f = face[start_face + si];

      for (int vi = 0; vi < 3; vi++)
      {
        int ov = m.face[i].v[vi];
        int nv = vmap[ov];

        if (nv == -1)
        {
          vmap[ov] = nv = vert.size();
          vert.push_back(m.vert[ov]);
        }
        f.v[vi] = nv;
      }
      f.matId = m.face[i].matId;
    }
    vmap.clear();
  }

  void calcBbox(BBox3 &b, int v_start = 0, int v_end = -1)
  {
    if (v_end == -1)
      v_end = vert.size();
    for (int i = v_start; i < v_end; i++)
      b += vert[i];
  }

  void buildFromMesh(const Mesh &m, MaterialDataList *mat_list, int physmat_id, bool pmid_force, StaticGeometryMesh *sgm)
  {
    String name;
    int matNum = mat_list ? mat_list->subMatCount() : -1;
    int matId;

    if (physmat_id == PHYSMAT_INVALID)
      matId = sceneMatNames.addNameId("::default");
    else
    {
      const PhysMat::MaterialData &pm = PhysMat::getMaterial(physmat_id);
      matId = sceneMatNames.addNameId(pm.name);
    }

    face.resize(m.face.size());
    vert = m.vert;

    // copy mesh data
    if (pmid_force)
    {
      for (int i = 0; i < face.size(); i++)
      {
        face[i].v[0] = m.face[i].v[0];
        face[i].v[1] = m.face[i].v[1];
        face[i].v[2] = m.face[i].v[2];
        face[i].matId = matId;
      }
    }
    else
    {
      if (matNum > 0)
      {
        pmmap.resize(matNum);
        mem_set_ff(pmmap);
      }
      else if (sgm)
      {
        pmmap.resize(sgm->mats.size());
        mem_set_ff(pmmap);
      }
      else
      {
        pmmap.resize(1);
        pmmap[0] = matId;
      }

      for (int i = 0; i < face.size(); i++)
      {
        face[i].v[0] = m.face[i].v[0];
        face[i].v[1] = m.face[i].v[1];
        face[i].v[2] = m.face[i].v[2];

        int matn = m.face[i].mat;
        if (matn >= pmmap.size())
          matn = pmmap.size() - 1;

        if (pmmap[matn] == -1)
        {
          if (matNum > 0)
          {
            if (::getPhysMatNameFromMatName(mat_list->getSubMat(matn), name))
              pmmap[matn] = sceneMatNames.addNameId(name);
            else
              pmmap[matn] = matId;
          }
          else if (sgm)
          {
            if (sgm->mats[matn] && ::getPhysMatNameFromMatName(sgm->mats[matn]->name, name))
              pmmap[matn] = sceneMatNames.addNameId(name);
            else
              pmmap[matn] = matId;
          }
        }

        face[i].matId = pmmap[matn];
      }
    }

    if (!sgm)
      return;

    // duplicate faces with 2SIDED material
    for (int i = 0; i < m.face.size(); ++i)
    {
      int matn = m.face[i].mat;
      if (matn >= sgm->mats.size())
        matn = sgm->mats.size() - 1;

      if (sgm->mats[matn] && sgm->mats[matn]->flags & MatFlags::FLG_2SIDED)
      {
        Face &f = face.push_back();

        Point3 v0 = m.vert[m.face[i].v[0]];
        Point3 v1 = m.vert[m.face[i].v[2]];
        Point3 v2 = m.vert[m.face[i].v[1]];

        Point3 dnorm = normalize((v1 - v0) % (v2 - v0)) * TWO_SIDED_SHIFT;

        f.v[0] = vert.size() + 0;
        f.v[1] = vert.size() + 1;
        f.v[2] = vert.size() + 2;
        vert.push_back(v0 + dnorm);
        vert.push_back(v1 + dnorm);
        vert.push_back(v2 + dnorm);
        f.matId = face[i].matId;
      }
    }
  }

public:
  //! temporary vertex map
  static Tab<int> vmap;
  //! temporary phys mat map
  static Tab<int> pmmap;
  //! temporary face/vert count per phys material
  static Tab<int> facePerMat;
  static Tab<int> vertPerMat;
  static Tab<int> objPerMat;
  static FastNameMapEx sceneMatNames;

  static void clearTemp()
  {
    clear_and_shrink(vmap);
    clear_and_shrink(pmmap);
    clear_and_shrink(facePerMat);
    clear_and_shrink(vertPerMat);
    clear_and_shrink(objPerMat);
  }
};

namespace cook
{
extern float gridStep, minSmallOverlap, minMutualOverlap;
extern int minFaceCnt;
} // namespace cook
