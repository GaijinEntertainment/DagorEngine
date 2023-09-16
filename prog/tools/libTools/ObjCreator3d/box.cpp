#include <libTools/ObjCreator3d/objCreator3d.h>


void ObjCreator3d::appendQuad(int x1, int x2, int x3, int x4, Tab<Face> &face, int &face_id, bool is_extern)
{
  if (!is_extern)
  {
    face[face_id++].set(x3, x4, x1);
    face[face_id++].set(x1, x2, x3);
  }
  else
  {
    face[face_id++].set(x1, x4, x3);
    face[face_id++].set(x3, x2, x1);
  }
}


void ObjCreator3d::appendBoxRingVertex(real z_offset, const IPoint2 &count, Tab<Point3> &vert)
{
  const real offset = 0.5f;
  const real yStep = 1.0f / (real)(count.y - 1);
  const real xStep = 1.0f / (real)(count.x - 1);

  for (int x = 0; x < count.x; ++x)
    vert.push_back(Point3((real)x * xStep - offset, 0.0f, z_offset));
  for (int y = 1; y < count.y; ++y)
    vert.push_back(Point3(offset, (real)y * yStep, z_offset));
  for (int x = count.x - 2; x >= 0; --x)
    vert.push_back(Point3((real)x * xStep - offset, 1.0f, z_offset));
  for (int y = count.y - 2; y >= 1; --y)
    vert.push_back(Point3(-offset, (real)y * yStep, z_offset));
}


void ObjCreator3d::appendBoxRingFaces(int ring1_vert_offset, int ring2_vert_offset, int count, Tab<Face> &face, int &face_id)
{
  int x1, x2, x3, x4;
  x1 = ring1_vert_offset + count - 1;
  x4 = ring2_vert_offset + count - 1;
  for (int i = 0; i < count; ++i)
  {
    x2 = ring1_vert_offset + i;
    x3 = ring2_vert_offset + i;
    appendQuad(x1, x2, x3, x4, face, face_id, false);
    x1 = x2;
    x4 = x3;
  }
}


void ObjCreator3d::appendBoxPlaneFaces(const Point3 &f, int ring_vert_offset, int vert_in_ring_count, int plane_vert_offset,
  int vert_in_plane_count, Tab<Face> &face, int &face_id, bool is_extern)
{
  int x1, x2, x3, x4;

  if (f.y == 1)
  {
    x1 = ring_vert_offset;
    x4 = x1 + vert_in_ring_count - 1;
    for (int i = 0; i < f.x; ++i)
    {
      x2 = x1 + 1;
      x3 = x4 - 1;
      appendQuad(x1, x2, x3, x4, face, face_id, is_extern);
      x1 = x2;
      x4 = x3;
    }
  }
  else
  {
    // first row
    x1 = ring_vert_offset;
    x4 = x1 + vert_in_ring_count - 1;
    for (int x = 0; x < (f.x - 1); ++x)
    {
      x2 = x1 + 1;
      x3 = plane_vert_offset + x;
      appendQuad(x1, x2, x3, x4, face, face_id, is_extern);
      x1 = x2;
      x4 = x3;
    }
    x2 = ring_vert_offset + f.x;
    x3 = x2 + 1;
    appendQuad(x1, x2, x3, x4, face, face_id, is_extern);

    // other rows except first and last
    for (int y = 0; y < (f.y - 2); ++y)
    {
      x1 = ring_vert_offset + vert_in_ring_count - 1 - y;
      x4 = x1 - 1;
      for (int x = 0; x < (f.x - 1); ++x)
      {
        x2 = plane_vert_offset + x + (y * (f.x - 1));
        x3 = x2 + (f.x - 1);
        appendQuad(x1, x2, x3, x4, face, face_id, is_extern);
        x1 = x2;
        x4 = x3;
      }
      x2 = ring_vert_offset + f.x + 1 + y;
      x3 = x2 + 1;
      appendQuad(x1, x2, x3, x4, face, face_id, is_extern);
    }

    // last row
    x1 = ring_vert_offset + vert_in_ring_count - 1 - (f.y - 2);
    x4 = x1 - 1;
    for (int x = 0; x < (f.x - 1); ++x)
    {
      x2 = plane_vert_offset + x + (vert_in_plane_count - (f.x - 1));
      x3 = x4 - 1;
      appendQuad(x1, x2, x3, x4, face, face_id, is_extern);
      x1 = x2;
      x4 = x3;
    }
    x2 = x4 - 2;
    x3 = x2 + 1;
    appendQuad(x1, x2, x3, x4, face, face_id, is_extern);
  }
}


Mesh *ObjCreator3d::generateBoxMesh(const IPoint3 &segments_count, Color4 col)
{
  if ((segments_count.x <= 0) || (segments_count.y <= 0) || (segments_count.z <= 0))
    return NULL;

  Mesh *retMesh = new (midmem) Mesh;
  MeshData *mesh = (MeshData *)retMesh;
  if (!mesh)
    return NULL;

  const IPoint3 &f = segments_count;
  const int vertInRingCount = ((f.x + 1) * 2) + ((f.y - 1) * 2);
  const int vertInPlaneCount = (f.x - 1) * (f.y - 1);
  mesh->vert.reserve((vertInRingCount * (f.z + 1)) + (vertInPlaneCount * 2));
  mesh->face.resize((f.x * f.y * 4) + (f.y * f.z * 4) + (f.z * f.x * 4));

  // generate vertex
  const real offset = 0.5f;
  const real zStep = 1.0f / (real)f.z;
  for (int z = 0; z < (f.z + 1); ++z)
    appendBoxRingVertex((real)z * zStep - offset, IPoint2(f.x + 1, f.y + 1), mesh->vert);

  const real yStep = 1.0f / (real)f.y;
  const real xStep = 1.0f / (real)f.x;
  // near plane
  for (int y = 1; y < f.y; ++y)
    for (int x = 1; x < f.x; ++x)
      mesh->vert.push_back(Point3((real)x * xStep - offset, (real)y * yStep, -offset));
  // far plane
  for (int y = 1; y < f.y; ++y)
    for (int x = 1; x < f.x; ++x)
      mesh->vert.push_back(Point3((real)x * xStep - offset, (real)y * yStep, offset));

  mesh->tvert[0].resize(mesh->vert.size());
  for (int i = 0; i < mesh->vert.size(); ++i)
  {
    mesh->tvert[0][i].x = mesh->vert[i].x;
    mesh->tvert[0][i].y = mesh->vert[i].y;
  }


  // generate faces
  int faceId = 0;
  for (int z = 0; z < f.z; ++z)
    appendBoxRingFaces(vertInRingCount * z, vertInRingCount * (z + 1), vertInRingCount, mesh->face, faceId);

  // near plane
  const int nearRingVertOffset = 0;
  const int nearPlaneVertOffset = vertInRingCount * (f.z + 1);
  appendBoxPlaneFaces(f, nearRingVertOffset, vertInRingCount, nearPlaneVertOffset, vertInPlaneCount, mesh->face, faceId, true);
  // far plane
  const int farRingVertOffset = vertInRingCount * f.z;
  const int farPlaneVertOffset = (vertInRingCount * (f.z + 1)) + vertInPlaneCount;
  appendBoxPlaneFaces(f, farRingVertOffset, vertInRingCount, farPlaneVertOffset, vertInPlaneCount, mesh->face, faceId, false);

  G_ASSERT(mesh->face.size() == faceId);

  mesh->tface[0].resize(mesh->face.size());
  for (int i = 0; i < mesh->face.size(); ++i)
  {
    mesh->tface[0][i].t[0] = mesh->face[i].v[0];
    mesh->tface[0][i].t[1] = mesh->face[i].v[1];
    mesh->tface[0][i].t[2] = mesh->face[i].v[2];
  }
  // set colors
  mesh->cvert.resize(1);
  mesh->cvert[0] = col;

  mesh->cface.resize(mesh->face.size());
  for (int fi = 0; fi < mesh->face.size(); ++fi)
  {
    mesh->cface[fi].t[0] = 0;
    mesh->cface[fi].t[1] = 0;
    mesh->cface[fi].t[2] = 0;
  }

  return retMesh;
}
