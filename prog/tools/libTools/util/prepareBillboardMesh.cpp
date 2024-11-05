// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_mesh.h>
#include <math/dag_Point4.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/dag_TMatrix.h>
#include <math/dag_bounds3.h>
#include <generic/dag_sort.h>
#include <math/random/dag_random.h>
#include <shaders/dag_shaderCommon.h>
#include <fx/dag_leavesWind.h>
#include <util/dag_bitArray.h>
#include <util/dag_globDef.h>

bool ignore_mapping_in_prepare_billboard_mesh = false;
bool generate_extra_in_prepare_billboard_mesh = false;

static void prepare_billboard_mesh_extra(MeshData &mesh, const TMatrix &wtm, dag::ConstSpan<Point3> org_verts)
{
  // generate wind groups
  int seed = 1234567;

  real scale = (length(wtm.getcol(0)) + length(wtm.getcol(1)) + length(wtm.getcol(2))) / 3;

  int chi = mesh.add_extra_channel(MeshData::CHT_FLOAT4, SCUSAGE_EXTRA, 0);
  G_ASSERT(chi >= 0);
  MeshData::ExtraChannel &ch = mesh.extra[chi];

  Bitarray faceUsed(tmpmem);
  faceUsed.resize(mesh.face.size());

  F2V_Map vertFaces;
  mesh.get_vert2facemap_fast(vertFaces);

  Tab<int> faceList(tmpmem);

  Tab<Point3> newNormals(tmpmem);
  Tab<Point3> centers(tmpmem);

  ch.resize_verts(mesh.vert.size());
  Point4 *deltas = (Point4 *)&ch.vt[0];

  for (int fi = 0; fi < mesh.face.size(); ++fi)
  {
    if (faceUsed[fi])
      continue;

    // get all faces linked to this one
    faceList.clear();

    faceUsed.set(fi);
    faceList.push_back(fi);

    for (int nf = 0; nf < faceList.size(); ++nf)
    {
      int nfi = faceList[nf];

      for (int j = 0; j < 3; ++j)
      {
        int vi = mesh.face[nfi].v[j];

        int numf = vertFaces.get_face_num(vi);
        for (int k = 0; k < numf; ++k)
        {
          int f = vertFaces.get_face_ij(vi, k);
          if (faceUsed[f])
            continue;

          faceUsed.set(f);
          faceList.push_back(f);
        }
      }
    }

    // calc center pos
    Point3 localCenter;
    // no mapping for centers - calc some "default" center
    BBox3 box;
    for (int i = 0; i < faceList.size(); ++i)
    {
      Face &f = mesh.face[faceList[i]];
      box += org_verts[f.v[0]];
      box += org_verts[f.v[1]];
      box += org_verts[f.v[2]];
    }

    localCenter = box.center();

    // replace indices
    int ci = centers.size();
    centers.push_back(wtm * localCenter);

    for (int i = 0; i < faceList.size(); ++i)
    {
      int fi = faceList[i];
      for (int vi = 0; vi < 3; ++vi)
      {
        Point3 vrt = (org_verts[mesh.face[fi].v[vi]] - localCenter) * scale;
        deltas[mesh.face[fi].v[vi]] = Point4(vrt.x, vrt.y, vrt.z, (_rnd(seed) % LeavesWindEffect::WIND_GRP_NUM) * 3);
      }

      ch.fc[fi].t[0] = mesh.face[fi].v[0];
      ch.fc[fi].t[1] = mesh.face[fi].v[1];
      ch.fc[fi].t[2] = mesh.face[fi].v[2];


      mesh.face[fi].v[0] = ci;
      mesh.face[fi].v[1] = ci;
      mesh.face[fi].v[2] = ci;
    }

    // force single averaged normal
    Point3 sumNormal(0, 0, 0);

    int ni = append_items(newNormals, 1);

    for (int i = 0; i < faceList.size(); ++i)
    {
      int fi = faceList[i];

      sumNormal += mesh.vertnorm[mesh.facengr[fi][0]];
      sumNormal += mesh.vertnorm[mesh.facengr[fi][1]];
      sumNormal += mesh.vertnorm[mesh.facengr[fi][2]];

      mesh.facengr[fi][0] = ni;
      mesh.facengr[fi][1] = ni;
      mesh.facengr[fi][2] = ni;
    }

    newNormals[ni] = normalize(sumNormal);
  }

  // replace old vertices with centers
  mesh.vert = centers;

  // replace old normals with new averaged ones
  mesh.vertnorm = newNormals;
}


static MeshData *mesh_to_sort = NULL;
static int sort_faces_by_windgroup(const int *left, const int *right)
{
  return (mesh_to_sort->tvert[1][mesh_to_sort->tface[1][*left].t[0]].x < mesh_to_sort->tvert[1][mesh_to_sort->tface[1][*right].t[0]].x)
           ? -1
           : 1;
}

void prepare_billboard_mesh(Mesh &mesh1, const TMatrix &wtm, dag::ConstSpan<Point3> org_verts)
{
  MeshData &mesh = (MeshData &)mesh1;
  if (::generate_extra_in_prepare_billboard_mesh)
  {
    prepare_billboard_mesh_extra(mesh, wtm, org_verts);
    return;
  }
  int seed = 1234567;
  real scale = (length(wtm.getcol(0)) + length(wtm.getcol(1)) + length(wtm.getcol(2))) / 3;

  int chi = mesh.add_extra_channel(MeshData::CHT_FLOAT4, SCUSAGE_EXTRA, 0);
  G_ASSERT(chi >= 0);
  MeshData::ExtraChannel &ch = mesh.extra[chi];
  Bitarray faceUsed(tmpmem);
  faceUsed.resize(mesh.face.size());

  F2V_Map vertFaces;
  mesh.get_vert2facemap_fast(vertFaces);

  Tab<int> faceList(tmpmem);

  Tab<Point3> newNormals(tmpmem);
  Tab<Point3> centers(tmpmem);
  Tab<int> windgroup(tmpmem);

  ch.resize_verts(mesh.vert.size());
  Point4 *deltas = (Point4 *)&ch.vt[0];

  for (int fi = 0; fi < mesh.face.size(); ++fi)
  {
    if (faceUsed[fi])
      continue;

    // get all faces linked to this one
    faceList.clear();

    faceUsed.set(fi);
    faceList.push_back(fi);

    for (int nf = 0; nf < faceList.size(); ++nf)
    {
      int nfi = faceList[nf];

      for (int j = 0; j < 3; ++j)
      {
        int vi = mesh.face[nfi].v[j];

        int numf = vertFaces.get_face_num(vi);
        for (int k = 0; k < numf; ++k)
        {
          int f = vertFaces.get_face_ij(vi, k);
          if (faceUsed[f])
            continue;

          faceUsed.set(f);
          faceList.push_back(f);
        }
      }
    }

    // calc center pos
    Point3 localCenter;
    bool gotCenter = false;

    if (mesh.tface[1].size() && !ignore_mapping_in_prepare_billboard_mesh)
    {
      // calc center pos from first face mapping
      real u0 = mesh.tvert[1][mesh.tface[1][fi].t[0]].x;
      real v0 = mesh.tvert[1][mesh.tface[1][fi].t[0]].y;
      real u1 = mesh.tvert[1][mesh.tface[1][fi].t[1]].x;
      real v1 = mesh.tvert[1][mesh.tface[1][fi].t[1]].y;
      real u2 = mesh.tvert[1][mesh.tface[1][fi].t[2]].x;
      real v2 = mesh.tvert[1][mesh.tface[1][fi].t[2]].y;

      v0 = 1 - v0;
      v1 = 1 - v1;
      v2 = 1 - v2;

      real det = u1 * v2 - v1 * u2 + u0 * v1 - u0 * v2 - v0 * u1 + v0 * u2;

      if (float_nonzero(det))
      {
        Point3 p0 = org_verts[mesh.face[fi].v[0]];
        Point3 p1 = org_verts[mesh.face[fi].v[1]];
        Point3 p2 = org_verts[mesh.face[fi].v[2]];

        localCenter = (p0 * (u1 * v2 - v1 * u2) + p1 * (v0 * u2 - u0 * v2) + p2 * (u0 * v1 - v0 * u1)) / det;
        gotCenter = true;
      }
    }

    if (!gotCenter)
    {
      // no mapping for centers - calc some "default" center
      BBox3 box;
      for (int i = 0; i < faceList.size(); ++i)
      {
        Face &f = mesh.face[faceList[i]];
        box += org_verts[f.v[0]];
        box += org_verts[f.v[1]];
        box += org_verts[f.v[2]];
      }

      localCenter = box.center();
    }

    // replace indices
    int ci = centers.size();
    centers.push_back(wtm * localCenter);
    int cwindgroup = (_rnd(seed) % LeavesWindEffect::WIND_GRP_NUM) * 3;
    int wi = windgroup.size();
    windgroup.push_back(cwindgroup);
    G_ASSERT(ci == wi);


    for (int i = 0; i < faceList.size(); ++i)
    {
      int fi = faceList[i];

      deltas[mesh.face[fi].v[0]] = Point4::xyzV((org_verts[mesh.face[fi].v[0]] - localCenter) * scale, cwindgroup);
      deltas[mesh.face[fi].v[1]] = Point4::xyzV((org_verts[mesh.face[fi].v[1]] - localCenter) * scale, cwindgroup);
      deltas[mesh.face[fi].v[2]] = Point4::xyzV((org_verts[mesh.face[fi].v[2]] - localCenter) * scale, cwindgroup);

      ch.fc[fi].t[0] = mesh.face[fi].v[0];
      ch.fc[fi].t[1] = mesh.face[fi].v[1];
      ch.fc[fi].t[2] = mesh.face[fi].v[2];

      mesh.face[fi].v[0] = ci;
      mesh.face[fi].v[1] = ci;
      mesh.face[fi].v[2] = ci;
    }

    // force single averaged normal
    Point3 sumNormal(0, 0, 0);

    int ni = append_items(newNormals, 1);

    for (int i = 0; i < faceList.size(); ++i)
    {
      int fi = faceList[i];

      sumNormal += mesh.vertnorm[mesh.facengr[fi][0]];
      sumNormal += mesh.vertnorm[mesh.facengr[fi][1]];
      sumNormal += mesh.vertnorm[mesh.facengr[fi][2]];

      mesh.facengr[fi][0] = ni;
      mesh.facengr[fi][1] = ni;
      mesh.facengr[fi][2] = ni;
    }

    newNormals[ni] = normalize(sumNormal);
  }

  // replace old vertices with centers
  mesh.vert = centers;

  // replace old normals with new averaged ones
  mesh.vertnorm = newNormals;

  // generate wind groups

  mesh.tvert[1].resize(windgroup.size());
  for (int i = 0; i < mesh.tvert[1].size(); ++i)
    mesh.tvert[1][i] = Point2(windgroup[i], 0);

  mesh.tface[1].resize(mesh.face.size());
  for (int i = 0; i < mesh.tface[1].size(); ++i)
  {
    mesh.tface[1][i].t[0] = mesh.face[i].v[0];
    mesh.tface[1][i].t[1] = mesh.face[i].v[1];
    mesh.tface[1][i].t[2] = mesh.face[i].v[2];
  }


  // Sort faces by windgroup to avoid constant waterfalling on Xbox360.

  Tab<int> reorder(tmpmem);
  reorder.resize(mesh.face.size());
  for (int faceNo = 0; faceNo < mesh.face.size(); faceNo++)
    reorder[faceNo] = faceNo;

  mesh_to_sort = &mesh;
  sort(reorder, &sort_faces_by_windgroup);

  mesh.reorder_faces(reorder.data());
}
