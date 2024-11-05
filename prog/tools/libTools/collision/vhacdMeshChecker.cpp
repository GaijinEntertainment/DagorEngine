// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/collision/vhacdMeshChecker.h>

bool check_face(const Tab<Point3> &verts, const Plane3 &plane, const BSphere3 &boundingSphere)
{
  for (unsigned int vertexNo = 0; vertexNo < verts.size(); vertexNo++)
  {
    float dist = plane.distance(verts[vertexNo]);
    float distEps = max(boundingSphere.r * 1e-3f, 1e-2f);
    if (dist > distEps)
      return false;
  }
  return true;
}

void fix_vhacd_faces(GeomMeshHelperDagObject &collision_object)
{
  BSphere3 boundingSphere;
  for (const auto &vert : collision_object.mesh.verts)
  {
    boundingSphere += vert;
  }
  for (int i = 0; i < collision_object.mesh.faces.size(); ++i)
  {
    Tab<GeomMeshHelper::Face> &faces = collision_object.mesh.faces;
    GeomMeshHelper::Face &face = faces[i];
    Tab<Point3> &verts = collision_object.mesh.verts;
    Plane3 plane(verts[face.v[0]], verts[face.v[2]], verts[face.v[1]]);
    plane.normalize();
    if (!check_face(verts, plane, boundingSphere))
    {
      Plane3 revertPlane(verts[face.v[0]], verts[face.v[1]], verts[face.v[2]]);
      const float length = revertPlane.n.length();
      revertPlane.normalize();
      if (!check_face(verts, revertPlane, boundingSphere) && length < 1e-7)
      {
        faces.erase(faces.begin() + i--);
      }
      else
      {
        eastl::swap(face.v[2], face.v[1]);
      }
    }
  }
}

bool is_vhacd_has_not_valid_faces(const Tab<Point3> &verts, const Tab<int> &indices)
{
  BSphere3 boundingSphere;
  for (const auto &vert : verts)
  {
    boundingSphere += vert;
  }
  for (int i = 0; i < indices.size(); i += 3)
  {
    Plane3 plane(verts[indices[i]], verts[indices[i + 2]], verts[indices[i + 1]]);
    plane.normalize();
    if (!check_face(verts, plane, boundingSphere))
    {
      Plane3 revertPlane(verts[indices[i]], verts[indices[i + 1]], verts[indices[i + 2]]);
      const float length = revertPlane.n.length();
      revertPlane.normalize();
      if (!check_face(verts, revertPlane, boundingSphere) && length > 1e-7)
      {
        return true;
      }
    }
  }
  return false;
}