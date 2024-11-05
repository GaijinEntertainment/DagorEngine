// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/ObjCreator3d/objCreator3d.h>
#include <util/dag_string.h>
#include <3d/dag_materialData.h>
#include <debug/dag_debug.h>

static __forceinline real clampf(real v, real minV, real maxV)
{
  if (v < minV)
    return minV;
  if (v > maxV)
    return maxV;
  return v;
}

void ObjCreator3d::setColorAndOrientation(Mesh *retMesh, const Color4 col, bool for_spiral)
{
  MeshData *mesh = (MeshData *)retMesh;
  // set colors
  mesh->cvert.resize(1);
  mesh->cvert[0] = col;
  mesh->cface.resize(mesh->face.size());

  for (int fi = 0; fi < mesh->face.size(); ++fi)
  {
    if (!for_spiral)
      mesh->face[fi].set(mesh->face[fi].v[0], mesh->face[fi].v[2], mesh->face[fi].v[1]);
    mesh->cface[fi].t[0] = 0;
    mesh->cface[fi].t[1] = 0;
    mesh->cface[fi].t[2] = 0;
  }

  mesh->calc_vertnorms();
  if (for_spiral)
    return;

  // change orientation
  for (int i = 0; i < mesh->vert.size(); i++)
  {
    mesh->vert[i] -= Point3(0.5, 0, 0.5);
    real t = mesh->vert[i].x;
    mesh->vert[i].x = -mesh->vert[i].z;
    mesh->vert[i].z = -t;
  }
}

Mesh *ObjCreator3d::generateBoxSpiralStair(int steps, real w, real arc, const Color4 col)
{
  Mesh *retMesh = generateStair(steps, steps * 2, steps * 4 + 2, true);
  MeshData *mesh = (MeshData *)retMesh;
  if (!mesh)
    return NULL;

  for (int i = 0; i < mesh->vert.size(); i++)
  {
    real fy = -(float)(i / 4 * 8) / steps * arc / 8;
    real fx = mesh->vert[i].x;
    real fz = mesh->vert[i].z * w + 1 - w;
    mesh->vert[i].x = fx * cos(fy) - fz * sin(fy);
    mesh->vert[i].z = fx * sin(fy) + fz * cos(fy);
  }
  for (int i = 0; i < steps; i++)
  {
    int vi = mesh->vert.size() - steps * 2 + i * 2;
    int fi = mesh->face.size() - steps * 4 + i * 4;
    mesh->vert[vi] = mesh->vert[i * 4 + 4];
    mesh->vert[vi].y = 0;
    mesh->vert[vi + 1] = mesh->vert[i * 4 + 5];
    mesh->vert[vi + 1].y = 0;

    mesh->face[i * 6 + 4].set(vi, i * 4 + 2, i * 4 + 4);
    mesh->face[i * 6 + 5].set(vi + 1, i * 4 + 5, i * 4 + 3);

    mesh->face[fi].set(vi, i ? vi - 2 : 0, i * 4 + 2);
    mesh->face[fi + 1].set(vi + 1, i * 4 + 3, i ? vi - 1 : 1);
    mesh->face[fi + 2].set(vi, i ? vi - 1 : 1, i ? vi - 2 : 0);
    mesh->face[fi + 3].set(vi, vi + 1, i ? vi - 1 : 1);
  }
  mesh->face[mesh->face.size() - steps * 4 - 2].set(mesh->vert.size() - 2, steps * 4 + 1, mesh->vert.size() - 1);
  mesh->face[mesh->face.size() - steps * 4 - 1].set(mesh->vert.size() - 2, steps * 4, steps * 4 + 1);
  setColorAndOrientation(retMesh, col, true);
  return retMesh;
}

Mesh *ObjCreator3d::generateClosedSpiralStair(int steps, real w, real arc, const Color4 col)
{
  Mesh *retMesh = generateStair(steps, steps * 2, steps * 4 + 2, true);
  MeshData *mesh = (MeshData *)retMesh;
  if (!mesh)
    return NULL;

  for (int i = 0; i < mesh->vert.size() - steps * 2; i++)
  {
    real fy = -(float)(i / 4 * 8) / steps * arc / 8;
    real fx = mesh->vert[i].x;
    real fz = mesh->vert[i].z * w + 1 - w;
    mesh->vert[i].x = fx * cos(fy) - fz * sin(fy);
    mesh->vert[i].z = fx * sin(fy) + fz * cos(fy);
  }
  for (int i = 0; i < steps; i++)
  {
    int vi = mesh->vert.size() - steps * 2 + i * 2;
    int fi = mesh->face.size() - steps * 4 + i * 4;
    mesh->vert[vi] = mesh->vert[i * 4 + 4];
    mesh->vert[vi].y -= 1.f / steps;
    mesh->vert[vi + 1] = mesh->vert[i * 4 + 5];
    mesh->vert[vi + 1].y -= 1.f / steps;

    mesh->face[i * 6 + 4].set(vi, i * 4 + 2, i * 4 + 4);
    mesh->face[i * 6 + 5].set(vi + 1, i * 4 + 5, i * 4 + 3);

    mesh->face[fi].set(vi, i ? vi - 2 : 0, i * 4 + 2);
    mesh->face[fi + 1].set(vi + 1, i * 4 + 3, i ? vi - 1 : 1);
    mesh->face[fi + 2].set(vi, i ? vi - 1 : 1, i ? vi - 2 : 0);
    mesh->face[fi + 3].set(vi, vi + 1, i ? vi - 1 : 1);
  }
  mesh->face[mesh->face.size() - steps * 4 - 2].set(mesh->vert.size() - 2, steps * 4 + 1, mesh->vert.size() - 1);
  mesh->face[mesh->face.size() - steps * 4 - 1].set(mesh->vert.size() - 2, steps * 4, steps * 4 + 1);

  setColorAndOrientation(retMesh, col, true);
  return retMesh;
}

Mesh *ObjCreator3d::generateOpenSpiralStair(int steps, real w, real h, real arc, const Color4 col)
{
  Mesh *retMesh = generateOpenStair(steps, h, col, true);
  MeshData *mesh = (MeshData *)retMesh;
  if (!mesh)
    return NULL;

  for (int i = 0; i < mesh->vert.size(); i++)
  {
    real fy = -(float)((i + 4) / 8 * 8) / steps * arc / 8;
    real fx = mesh->vert[i].x;
    real fz = mesh->vert[i].z * w + 1 - w;
    mesh->vert[i].x = fx * cos(fy) - fz * sin(fy);
    mesh->vert[i].z = fx * sin(fy) + fz * cos(fy);
  }
  setColorAndOrientation(retMesh, col, true);
  return retMesh;
}

Mesh *ObjCreator3d::generateOpenStair(int steps, real h, const Color4 col, bool for_spiral)
{
  if (!steps)
    return NULL;

  Mesh *retMesh = new (midmem) Mesh;
  MeshData *mesh = (MeshData *)retMesh;
  if (!mesh)
    return NULL;

  h = clampf(h, 0, 1);
  mesh->vert.resize(steps * 8);
  for (int i = 0; i < steps; i++)
  {
    float fx = for_spiral ? 0 : (float)i;
    float fy = (float)i;
    mesh->vert[i * 8] = Point3(fx / steps, (fy + 1 - h) / steps, 0);
    mesh->vert[i * 8 + 1] = Point3(fx / steps, (fy + 1 - h) / steps, 1);
    mesh->vert[i * 8 + 2] = Point3(fx / steps, (fy + 1) / steps, 0);
    mesh->vert[i * 8 + 3] = Point3(fx / steps, (fy + 1) / steps, 1);
    fx = for_spiral ? 0 : (float)i + 1;
    mesh->vert[i * 8 + 4] = Point3(fx / steps, (fy + 1) / steps, 0);
    mesh->vert[i * 8 + 5] = Point3(fx / steps, (fy + 1) / steps, 1);
    mesh->vert[i * 8 + 6] = Point3(fx / steps, (fy + 1 - h) / steps, 0);
    mesh->vert[i * 8 + 7] = Point3(fx / steps, (fy + 1 - h) / steps, 1);
  }
  mesh->face.resize(steps * 12);

  for (int n = 0; n < steps; n++)
  {
    mesh->face[n * 12].set(n * 8, n * 8 + 1, n * 8 + 3);
    mesh->face[n * 12 + 1].set(n * 8, n * 8 + 3, n * 8 + 2);
    mesh->face[n * 12 + 2].set(n * 8 + 2, n * 8 + 3, n * 8 + 5);
    mesh->face[n * 12 + 3].set(n * 8 + 2, n * 8 + 5, n * 8 + 4);
    mesh->face[n * 12 + 4].set(n * 8, n * 8 + 2, n * 8 + 4);
    mesh->face[n * 12 + 5].set(n * 8 + 1, n * 8 + 5, n * 8 + 3);
    mesh->face[n * 12 + 6].set(n * 8 + 6, n * 8 + 5, n * 8 + 7);
    mesh->face[n * 12 + 7].set(n * 8 + 6, n * 8 + 4, n * 8 + 5);
    mesh->face[n * 12 + 8].set(n * 8, n * 8 + 7, n * 8 + 1);
    mesh->face[n * 12 + 9].set(n * 8, n * 8 + 6, n * 8 + 7);
    mesh->face[n * 12 + 10].set(n * 8, n * 8 + 4, n * 8 + 6);
    mesh->face[n * 12 + 11].set(n * 8 + 1, n * 8 + 7, n * 8 + 5);
  }
  if (for_spiral)
    return retMesh;

  setColorAndOrientation(retMesh, col, for_spiral);
  return retMesh;
}


Mesh *ObjCreator3d::generateStair(int steps, int add_verts, int add_faces, bool for_spiral)
{
  if (!steps)
    return NULL;

  Mesh *retMesh = new (midmem) Mesh;
  MeshData *mesh = (MeshData *)retMesh;
  if (!mesh)
    return NULL;

  mesh->vert.resize(steps * 4 + 2 + add_verts);
  for (int i = 0; i < steps; i++)
  {
    float fx = for_spiral ? 0 : (float)i;
    float fy = (float)i;
    mesh->vert[i * 4] = Point3(fx / steps, fy / steps, 0);
    mesh->vert[i * 4 + 1] = Point3(fx / steps, fy / steps, 1);
    mesh->vert[i * 4 + 2] = Point3(fx / steps, (fy + 1) / steps, 0);
    mesh->vert[i * 4 + 3] = Point3(fx / steps, (fy + 1) / steps, 1);
  }
  mesh->vert[steps * 4] = Point3(for_spiral ? 0 : 1, 1, 0);
  mesh->vert[steps * 4 + 1] = Point3(for_spiral ? 0 : 1, 1, 1);
  mesh->face.resize(steps * 4 + steps * 2 + add_faces);

  for (int n = 0; n < steps; n++)
  {
    mesh->face[n * 6].set(n * 4, n * 4 + 1, n * 4 + 3);
    mesh->face[n * 6 + 1].set(n * 4, n * 4 + 3, n * 4 + 2);
    mesh->face[n * 6 + 2].set(n * 4 + 2, n * 4 + 3, n * 4 + 5);
    mesh->face[n * 6 + 3].set(n * 4 + 2, n * 4 + 5, n * 4 + 4);
    mesh->face[n * 6 + 4].set(n * 4, n * 4 + 2, n * 4 + 4);
    mesh->face[n * 6 + 5].set(n * 4 + 1, n * 4 + 5, n * 4 + 3);
  }

  return retMesh;
}

Mesh *ObjCreator3d::generateClosedStair(int steps, const Color4 col)
{
  if (!steps)
    return NULL;

  Mesh *retMesh = generateStair(steps, 4, steps * 2 + 8);
  MeshData *mesh = (MeshData *)retMesh;
  if (!mesh)
    return NULL;

  mesh->vert[mesh->vert.size() - 4] = Point3(1.f / steps, 0, 0);
  mesh->vert[mesh->vert.size() - 3] = Point3(1.f / steps, 0, 1);
  mesh->vert[mesh->vert.size() - 2] = Point3(1.f, 1 - 1.f / steps, 0);
  mesh->vert[mesh->vert.size() - 1] = Point3(1.f, 1 - 1.f / steps, 1);
  for (int n = 0; n < steps; n++)
  {
    mesh->face[mesh->face.size() - steps * 2 - 8 + n * 2].set(mesh->vert.size() - 4, n * 4, n * 4 + 4);
    mesh->face[mesh->face.size() - steps * 2 - 7 + n * 2].set(mesh->vert.size() - 3, n * 4 + 5, n * 4 + 1);
  }
  mesh->face[mesh->face.size() - 8].set(mesh->vert.size() - 3, 1, mesh->vert.size() - 4);
  mesh->face[mesh->face.size() - 7].set(mesh->vert.size() - 4, 1, 0);
  mesh->face[mesh->face.size() - 6].set(mesh->vert.size() - 1, mesh->vert.size() - 2, steps * 4);
  mesh->face[mesh->face.size() - 5].set(mesh->vert.size() - 1, steps * 4, steps * 4 + 1);

  mesh->face[mesh->face.size() - 4].set(mesh->vert.size() - 4, steps * 4, mesh->vert.size() - 2);
  mesh->face[mesh->face.size() - 3].set(mesh->vert.size() - 3, mesh->vert.size() - 1, steps * 4 + 1);
  mesh->face[mesh->face.size() - 2].set(mesh->vert.size() - 4, mesh->vert.size() - 2, mesh->vert.size() - 3);
  mesh->face[mesh->face.size() - 1].set(mesh->vert.size() - 2, mesh->vert.size() - 1, mesh->vert.size() - 3);

  setColorAndOrientation(retMesh, col, false);
  return retMesh;
}


Mesh *ObjCreator3d::generateBoxStair(int steps, const Color4 col)
{
  if (!steps)
    return NULL;

  Mesh *retMesh = generateStair(steps, 2, steps * 2 + 4);
  MeshData *mesh = (MeshData *)retMesh;
  if (!mesh)
    return NULL;

  mesh->vert[mesh->vert.size() - 2] = Point3(1, 0, 0);
  mesh->vert[mesh->vert.size() - 1] = Point3(1, 0, 1);
  for (int n = 0; n < steps; n++)
  {
    mesh->face[mesh->face.size() - steps * 2 - 4 + n * 2].set(mesh->vert.size() - 2, n * 4, n * 4 + 4);
    mesh->face[mesh->face.size() - steps * 2 - 3 + n * 2].set(mesh->vert.size() - 1, n * 4 + 5, n * 4 + 1);
  }
  mesh->face[mesh->face.size() - 4].set(mesh->vert.size() - 1, 1, mesh->vert.size() - 2);
  mesh->face[mesh->face.size() - 3].set(mesh->vert.size() - 2, 1, 0);
  mesh->face[mesh->face.size() - 2].set(mesh->vert.size() - 1, mesh->vert.size() - 2, steps * 4);
  mesh->face[mesh->face.size() - 1].set(mesh->vert.size() - 1, steps * 4, steps * 4 + 1);

  setColorAndOrientation(retMesh, col, false);
  return retMesh;
}

bool ObjCreator3d::generateBoxSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom, StaticGeometryMaterial *material,
  const Color4 col)
{
  Mesh *mesh = generateBoxSpiralStair(steps, w, arc, col);

  if (!mesh)
    return false;

  if (addNode(String(32, "BoxStair_%u", sphereIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}


bool ObjCreator3d::generateBoxSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom, MaterialDataList *material,
  const Color4 col)
{
  Mesh *mesh = generateBoxSpiralStair(steps, w, arc, col);

  if (!mesh)
    return false;

  if (addNode(String(32, "BoxStair_%u", sphereIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}


bool ObjCreator3d::generateBoxSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom)
{
  MaterialDataList mat;
  MaterialData *m = new MaterialData;

  m->className = "simple";
  m->mat.diff = Color4(1, 1, 1);
  mat.addSubMat(m);

  return generateBoxSpiralStair(steps, w, arc, geom, &mat);
}


bool ObjCreator3d::generateClosedSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom,
  StaticGeometryMaterial *material, const Color4 col)
{
  Mesh *mesh = generateClosedSpiralStair(steps, w, arc, col);

  if (!mesh)
    return false;

  if (addNode(String(32, "ClosedSpiralStair_%u", sphereIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}


bool ObjCreator3d::generateClosedSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom, MaterialDataList *material,
  const Color4 col)
{
  Mesh *mesh = generateClosedSpiralStair(steps, w, arc, col);

  if (!mesh)
    return false;

  if (addNode(String(32, "ClosedSpiralStair_%u", sphereIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}


bool ObjCreator3d::generateClosedSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom)
{
  MaterialDataList mat;
  MaterialData *m = new MaterialData;

  m->className = "simple";
  m->mat.diff = Color4(1, 1, 1);
  mat.addSubMat(m);

  return generateClosedSpiralStair(steps, w, arc, geom, &mat);
}


bool ObjCreator3d::generateOpenSpiralStair(int steps, real w, real h, real arc, StaticGeometryContainer &geom,
  StaticGeometryMaterial *material, const Color4 col)
{
  Mesh *mesh = generateOpenSpiralStair(steps, w, h, arc, col);

  if (!mesh)
    return false;

  if (addNode(String(32, "OpenSpiralStair_%u", sphereIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}


bool ObjCreator3d::generateOpenSpiralStair(int steps, real w, real h, real arc, StaticGeometryContainer &geom,
  MaterialDataList *material, const Color4 col)
{
  Mesh *mesh = generateOpenSpiralStair(steps, w, h, arc, col);

  if (!mesh)
    return false;

  if (addNode(String(32, "OpenSpiralStair_%u", sphereIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}


bool ObjCreator3d::generateOpenSpiralStair(int steps, real w, real h, real arc, StaticGeometryContainer &geom)
{
  MaterialDataList mat;
  MaterialData *m = new MaterialData;

  m->className = "simple";
  m->mat.diff = Color4(1, 1, 1);
  mat.addSubMat(m);

  return generateOpenSpiralStair(steps, w, h, arc, geom, &mat);
}


bool ObjCreator3d::generateOpenStair(int steps, real h, StaticGeometryContainer &geom, StaticGeometryMaterial *material,
  const Color4 col)
{
  Mesh *mesh = generateOpenStair(steps, h, col);

  if (!mesh)
    return false;

  if (addNode(String(32, "OpenStair_%u", sphereIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}


bool ObjCreator3d::generateOpenStair(int steps, real h, StaticGeometryContainer &geom, MaterialDataList *material, const Color4 col)
{
  Mesh *mesh = generateOpenStair(steps, h, col);

  if (!mesh)
    return false;

  if (addNode(String(32, "OpenStair_%u", sphereIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}


bool ObjCreator3d::generateOpenStair(int steps, real h, StaticGeometryContainer &geom)
{
  MaterialDataList mat;
  MaterialData *m = new MaterialData;

  m->className = "simple";
  m->mat.diff = Color4(1, 1, 1);
  mat.addSubMat(m);

  return generateOpenStair(steps, h, geom, &mat);
}


bool ObjCreator3d::generateClosedStair(int steps, StaticGeometryContainer &geom, StaticGeometryMaterial *material, const Color4 col)
{
  Mesh *mesh = generateClosedStair(steps, col);

  if (!mesh)
    return false;

  if (addNode(String(32, "OpenStair_%u", sphereIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}


bool ObjCreator3d::generateClosedStair(int steps, StaticGeometryContainer &geom, MaterialDataList *material, const Color4 col)
{
  Mesh *mesh = generateClosedStair(steps, col);

  if (!mesh)
    return false;

  if (addNode(String(32, "ClosedStair_%u", sphereIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}


bool ObjCreator3d::generateClosedStair(int steps, StaticGeometryContainer &geom)
{
  MaterialDataList mat;
  MaterialData *m = new MaterialData;

  m->className = "simple";
  m->mat.diff = Color4(1, 1, 1);
  mat.addSubMat(m);

  return generateClosedStair(steps, geom, &mat);
}


bool ObjCreator3d::generateBoxStair(int steps, StaticGeometryContainer &geom, StaticGeometryMaterial *material, const Color4 col)
{
  Mesh *mesh = generateBoxStair(steps, col);

  if (!mesh)
    return false;

  if (addNode(String(32, "BoxStair_%u", sphereIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}


bool ObjCreator3d::generateBoxStair(int steps, StaticGeometryContainer &geom, MaterialDataList *material, const Color4 col)
{
  Mesh *mesh = generateBoxStair(steps, col);

  if (!mesh)
    return false;

  if (addNode(String(32, "BoxStair_%u", sphereIdx++), mesh, material, geom))
    return true;
  else
  {
    del_it(mesh);
    return false;
  }
}


bool ObjCreator3d::generateBoxStair(int steps, StaticGeometryContainer &geom)
{
  MaterialDataList mat;
  MaterialData *m = new MaterialData;

  m->className = "simple";
  m->mat.diff = Color4(1, 1, 1);
  mat.addSubMat(m);

  return generateBoxStair(steps, geom, &mat);
}
