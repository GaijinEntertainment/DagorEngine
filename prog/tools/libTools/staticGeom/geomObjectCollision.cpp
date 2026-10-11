// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/collision/trace_mesh_soa4_blas.h>
#include <debug/dag_log.h>

void GeomObject::delCollision() { del_it(collision); }


bool GeomObject::reloadCollision()
{
  delCollision();
  return hasCollision();
}


void GeomObject::initCollision()
{
  if (!geom)
    return;
  collision = new MeshSoa4BLAS;

  Tab<Point3> verts(tmpmem);
  Tab<uint32_t> indices(tmpmem);
  const char *anyNode = nullptr;

  for (StaticGeometryNode *node : geom->nodes)
  {
    if (!node || node->isNonTopLod() || !(node->flags & StaticGeometryNode::FLG_COLLIDABLE))
      continue;
    if (!node->mesh)
    {
      logwarn("geomObject: collidable node <%s> carries no mesh, it is left out of the collision", node->name.str());
      continue;
    }

    const StaticGeometryMesh &smesh = *node->mesh;
    const Mesh &mesh = smesh.mesh;
    const int numMats = smesh.mats.size();
    if (!numMats)
    {
      logwarn("geomObject: node <%s> carries no material, it is left out of the collision", node->name.str());
      continue;
    }
    if (mesh.getFace().empty() || mesh.getVert().empty())
      continue;

    const int nodeVerts = mesh.getVert().size();
    const uint32_t base = (uint32_t)append_items(verts, nodeVerts);
    for (int vi = 0; vi < nodeVerts; ++vi)
      verts[base + vi] = node->wtm * mesh.getVert()[vi];

    dag::ConstSpan<Face> faces = mesh.getFace();
    int badFaces = 0;
    for (int fi = 0; fi < faces.size(); ++fi)
    {
      const Face &f = faces[fi];
      if (!smesh.mats[f.mat < numMats ? f.mat : numMats - 1])
        continue;
      // constraint: one bad vertex refuses the object's only BLAS, so the face goes instead
      if (f.v[0] >= nodeVerts || !v_test_xyz_finite(v_ldu_p3_safe(&verts[base + f.v[0]].x)) || f.v[1] >= nodeVerts ||
          !v_test_xyz_finite(v_ldu_p3_safe(&verts[base + f.v[1]].x)) || f.v[2] >= nodeVerts ||
          !v_test_xyz_finite(v_ldu_p3_safe(&verts[base + f.v[2]].x)))
      {
        ++badFaces;
        continue;
      }
      if (indices.empty())
        anyNode = node->name.str();
      for (int k = 0; k < 3; ++k)
        indices.push_back(base + f.v[k]);
    }
    if (badFaces)
      logwarn("geomObject <%s>: dropped %d faces with a corner past the node's verts or not finite", node->name.str(), badFaces);
  }

  if (indices.empty())
    return;

  if (!build_mesh_soa4_blas(*collision, make_span_const(verts), make_span_const(indices)))
    logerr("geomObject containing node <%s>: built no BLAS out of %d faces, it traces nothing", anyNode, (int)indices.size() / 3);
}

bool GeomObject::hasCollision()
{
  if (!collision)
    initCollision();
  return collision && !collision->empty();
}

bool GeomObject::traceRay(const Point3 &p, const Point3 &unnormalized_dir, real &maxt, Point3 *norm)
{
  return hasCollision() && trace_mesh_soa4_blas(*collision, p, unnormalized_dir, maxt, norm);
}
