// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/ObjCreator3d/objCreator3d.h>


void ObjCreator3d::generateCap(int cap_vertex_offset, int ring_vertex_offset, int sides, Tab<Face> &face, int &face_id, bool is_extern)
{
  int x1, x2, x3;
  x1 = cap_vertex_offset;
  x2 = ring_vertex_offset;
  for (int i = 0; i < sides - 1; ++i)
  {
    x3 = x2 + 1;
    appendTriangle(x1, x2, x3, face, face_id, is_extern);
    x2 = x3;
  }
  appendTriangle(x1, x2, ring_vertex_offset, face, face_id, is_extern);
}


void ObjCreator3d::appendTriangle(int x1, int x2, int x3, Tab<Face> &face, int &face_id, bool is_extern)
{
  if (!is_extern)
    face[face_id++].set(x1, x2, x3);
  else
    face[face_id++].set(x3, x2, x1);
}

void ObjCreator3d::generateRing(int sides, float y, float radius, Tab<Point3> &vert)
{
  float angle = 0;
  const float step = TWOPI / sides;
  for (int i = 0; i < sides; ++i, angle += step)
    vert.push_back(Point3(radius * cos(angle), y, radius * sin(angle)));
}

Mesh *ObjCreator3d::generateCylinderMesh(int sides, int height_segments, int cap_segments)
{
  if ((sides < 3) || (height_segments < 1) || (cap_segments < 1))
    return NULL;

  Mesh *retMesh = new (midmem) Mesh;
  MeshData *mesh = (MeshData *)retMesh;
  if (!mesh)
    return NULL;
  mesh->vert.reserve(((sides * cap_segments + 1) * 2) + (sides * (height_segments - 1)));
  mesh->face.resize((sides + (sides * (cap_segments - 1) * 2) + (sides * height_segments)) * 2);


  // generate vertex
  const float capStep = 1.0f / (float)cap_segments;
  for (int i = 1; i <= cap_segments; ++i)
    generateRing(sides, 1.0f, capStep * (float)i, mesh->vert);

  const float heightStep = 1.0f / (float)height_segments;
  for (int i = height_segments - 1; i > 0; --i)
    generateRing(sides, heightStep * (float)i, 1.0f, mesh->vert);

  for (int i = cap_segments; i > 0; --i)
    generateRing(sides, 0.0f, capStep * (float)i, mesh->vert);

  mesh->vert.push_back(Point3(0, 1.0f, 0));
  mesh->vert.push_back(Point3(0, 0.0f, 0));

  mesh->tvert[0].resize(mesh->vert.size());
  for (int i = 0; i < mesh->vert.size(); ++i)
  {
    mesh->tvert[0][i].x = mesh->vert[i].x;
    mesh->tvert[0][i].y = mesh->vert[i].y;
  }


  // generate faces
  int faceId = 0;
  int x1, x2, x3, x4;
  for (int i = 0; i < cap_segments * 2.0f + height_segments - 2; ++i)
  {
    x1 = sides * i;
    x2 = x1 + sides;
    for (int j = 0; j < sides - 1; ++j)
    {
      x3 = x2 + 1;
      x4 = x1 + 1;
      appendQuad(x1, x2, x3, x4, mesh->face, faceId, true);
      x1 = x4;
      x2 = x3;
    }
    appendQuad(x1, x2, sides * (i + 1), sides * i, mesh->face, faceId, true);
  }
  const float capVertexOffset = sides * (cap_segments * 2.0f + height_segments - 1);
  generateCap(capVertexOffset, 0, sides, mesh->face, faceId, true);
  generateCap(capVertexOffset + 1, capVertexOffset - sides, sides, mesh->face, faceId, false);

  G_ASSERT(faceId == mesh->face.size());

  mesh->tface[0].resize(mesh->face.size());
  for (int i = 0; i < mesh->face.size(); ++i)
  {
    mesh->tface[0][i].t[0] = mesh->face[i].v[0];
    mesh->tface[0][i].t[1] = mesh->face[i].v[1];
    mesh->tface[0][i].t[2] = mesh->face[i].v[2];
  }
  return retMesh;
}
