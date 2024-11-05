// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/ObjCreator3d/objCreator3d.h>


//==============================================================================
Mesh *ObjCreator3d::generatePlaneMesh(const Point2 &cell_size)
{
  if (!cell_size.x || !cell_size.y)
    return NULL;

  Mesh *retMesh = new (midmem) Mesh;
  MeshData *mesh = (MeshData *)retMesh;
  if (!mesh)
    return NULL;

  const Point2 cellSize(1.0f / cell_size.x, 1.0f / cell_size.y);
  const IPoint2 iCellCount(cell_size.x + 0.99999f, cell_size.y + 0.99999f);

  mesh->vert.resize((iCellCount.x + 1) * (iCellCount.y + 1));
  mesh->tvert[0].resize(mesh->vert.size());
  mesh->face.resize(iCellCount.x * iCellCount.y * 2);
  mesh->tface[0].resize(mesh->face.size());

  for (int y = 0; y <= iCellCount.y; ++y)
  {
    const unsigned vertOffset = y * (iCellCount.x + 1);
    const unsigned faceOffset = (y - 1) * iCellCount.x * 2;
    const real posY = (y == iCellCount.y) ? cellSize.y * cell_size.y : cellSize.y * (real)y;

    for (int x = 0; x <= iCellCount.x; ++x)
    {
      const unsigned vertId = vertOffset + x;
      const unsigned prevVertId = vertOffset - (iCellCount.x + 1) + x;
      real posX = (x == iCellCount.x) ? cellSize.x * cell_size.x : cellSize.x * (real)x;

      mesh->vert[vertId] = Point3(posX, 0, posY);
      mesh->tvert[0][vertId] = Point2(posX, posY);

      if (!x || !y)
        continue;

      const unsigned faceId = faceOffset + (x - 1) * 2;
      mesh->face[faceId].set(vertId - 1, prevVertId, prevVertId - 1);
      mesh->face[faceId + 1].set(prevVertId, vertId - 1, vertId);

      mesh->tface[0][faceId].t[0] = mesh->face[faceId].v[0];
      mesh->tface[0][faceId].t[1] = mesh->face[faceId].v[1];
      mesh->tface[0][faceId].t[2] = mesh->face[faceId].v[2];

      mesh->tface[0][faceId + 1].t[0] = mesh->face[faceId + 1].v[0];
      mesh->tface[0][faceId + 1].t[1] = mesh->face[faceId + 1].v[1];
      mesh->tface[0][faceId + 1].t[2] = mesh->face[faceId + 1].v[2];
    }
  }

  const Point3 pos(0.5f, 0, 0.5f);
  for (int i = 0; i < mesh->vert.size(); ++i)
    mesh->vert[i] -= pos;

  // set colors
  mesh->cvert.resize(1);
  mesh->cvert[0] = Color4(1, 1, 1, 1);

  return retMesh;
}
