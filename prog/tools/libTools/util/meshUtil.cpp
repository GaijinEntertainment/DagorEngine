// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/util/meshUtil.h>
#include <libTools/util/iLogWriter.h>

#include <math/dag_mesh.h>
#include <util/dag_bitArray.h>

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>

#include <float.h>


bool validate_mesh(const char *node_name, const Mesh &mesh, ILogWriter &log)
{
  bool ret = true;

  for (int ci = 0; ci < NUMMESHTCH; ++ci)
  {
    int invalid = 0;

    for (int ti = 0; ti < mesh.getTVert(ci).size(); ++ti)
    {
      Point2 tc = mesh.getTVert(ci)[ti];
      if (!std::isfinite(tc.x) || !std::isfinite(tc.y) || std::isnan(tc.x) || std::isnan(tc.y))
        ++invalid;
    }

    if (invalid)
    {
      log.addMessage(ILogWriter::ERROR, "Node \"%s\" contains %i vertexes having invalid texture coordinates in channel #%i",
        node_name, invalid, ci);

      ret = false;
    }

    invalid = 0;

    for (int fi = 0; fi < mesh.getTFace(ci).size(); ++fi)
      for (int vi = 0; vi < 3; ++vi)
        if (mesh.getTFace(ci)[fi].t[vi] >= mesh.getTVert(ci).size())
          ++invalid;

    if (invalid)
    {
      log.addMessage(ILogWriter::ERROR, "Node \"%s\" contains %i invalid texture coordinates in channel #%i", node_name, invalid, ci);

      ret = false;
    }
  }

  return ret;
}

void split_real_two_sided(Mesh &mMesh, Bitarray &is_material_real_two_sided_array, int side_channel_id)
{
  int numFaces = mMesh.getFace().size();
  if (!is_material_real_two_sided_array.size())
    return;

  unsigned int usedSmoothGroups = 0;
  for (unsigned int faceNo = 0; faceNo < numFaces; faceNo++)
    usedSmoothGroups |= mMesh.getFaceSMGR(faceNo);

  for (int fi = 0; fi < numFaces;)
  {
    int mi = mMesh.getFaceMaterial(fi);
    if (mi >= is_material_real_two_sided_array.size())
      mi = is_material_real_two_sided_array.size() - 1;

    int nextf;
    for (nextf = fi + 1; nextf < numFaces; ++nextf)
      if (mMesh.getFaceMaterial(nextf) != mi)
        break;

    if (is_material_real_two_sided_array.get(mi))
    {
      MeshData &m = mMesh.getMeshData();
      // duplicate faces
      int nf = m.face.size();
      int numf = nextf - fi;
      G_VERIFY(m.setnumfaces(nf + numf));
      for (int i = fi; i < nextf; ++i)
      {
        m.face[nf] = m.face[i];

        for (unsigned int extraChannelNo = 0; extraChannelNo < m.extra.size(); extraChannelNo++)
          m.extra[extraChannelNo].fc[nf] = m.extra[extraChannelNo].fc[i];

        if (m.face[nf].smgr)
          m.face[nf].smgr = ~usedSmoothGroups;

        if (m.cface.size())
        {
          G_ASSERT(m.cface.size() == m.face.size());
          m.cface[nf] = m.cface[i];
        }

        if (m.facengr.size())
        {
          G_ASSERT(m.facengr.size() == m.face.size());
          m.facengr[nf][0] = m.facengr[i][0] + m.vertnorm.size();
          m.facengr[nf][1] = m.facengr[i][1] + m.vertnorm.size();
          m.facengr[nf][2] = m.facengr[i][2] + m.vertnorm.size();
        }

        for (int t = 0; t < NUMMESHTCH; ++t)
          if (m.tface[t].size())
          {
            G_ASSERT(m.tface[t].size() == m.face.size());
            m.tface[t][nf] = m.tface[t][i];
          }
        ++nf;
      }
    }
    fi = nextf;
  }

  if (mMesh.getFace().size() == numFaces)
    return;

  MeshData &m = mMesh.getMeshData();
  int n = m.vertnorm.size();
  append_items(m.vertnorm, n);
  for (int i = 0; i < n; ++i)
    m.vertnorm[i + n] = m.vertnorm[i];

  // Duplicate ngr to avoid vertnorm recalculation or accidental crash.
  n = m.ngr.size();
  append_items(m.ngr, n);
  for (int i = 0; i < n; ++i)
    m.ngr[i + n] = m.ngr[i];

  m.flip_normals(numFaces, mMesh.getFace().size() - numFaces);

  if (side_channel_id >= 0 && side_channel_id < m.extra.size() && m.extra[side_channel_id].fc.size() == m.face.size())
  {
    for (unsigned int faceNo = numFaces; faceNo < m.face.size(); faceNo++)
    {
      for (unsigned int cornerNo = 0; cornerNo < 3; cornerNo++)
        m.extra[side_channel_id].fc[faceNo].t[cornerNo] = 1;
    }
  }
}
