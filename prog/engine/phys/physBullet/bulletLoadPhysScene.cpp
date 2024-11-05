// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <phys/dag_physics.h>
#include <scene/dag_frtdump.h>
#include <ioSys/dag_zlibIo.h>
#include <scene/dag_physMat.h>
#include <math/dag_Point4.h>
#include <util/dag_roNameMap.h>
#include <debug/dag_debug.h>
#include <btBulletWorldDeserialize.h>
#include "bulletPrivate.h"

#define BULLET_SCENE_CLIPPING_VERSION 1

#define TRACE_PHYS_SCENE_LOADING 0

void *PhysWorld::loadSceneCollision(IGenLoad &_crd, int /*scene_mat_id*/)
{
  int ver = _crd.readInt();
  BufferedZlibLoadCB *zlib_crd = NULL;
  if (ver & 0x80000000)
  {
    zlib_crd = new BufferedZlibLoadCB(_crd, _crd.getBlockRest());
    zlib_crd->readInt(ver);
  }

  IGenLoad &crd = zlib_crd ? *(IGenLoad *)zlib_crd : _crd;

  if (ver != BULLET_SCENE_CLIPPING_VERSION)
  {
    logerr("Wrong Bullet phys scene version: %d", ver);
    del_it(zlib_crd);
    return NULL;
  }

  // read mat names
  int nm_sz = crd.readInt();
  RoNameMap *nm = (RoNameMap *)memalloc(nm_sz, tmpmem);
  crd.read(nm, nm_sz);
  nm->patchData(nm);

  // get static primitives count
  int tri_mesh_actors = crd.readInt();
  int conv_mesh_actors = crd.readInt();
  int cyl_actors = crd.readInt();
  int box_actors = crd.readInt();
  int sph_actors = crd.readInt();
  int cap_actors = crd.readInt();

  bullet::StaticCollision *sc = new bullet::StaticCollision(this);
  sc->bodies.reserve(tri_mesh_actors + conv_mesh_actors + cyl_actors + box_actors + sph_actors + cap_actors);
  sc->dataBlocks.reserve(tri_mesh_actors * 7);

  // load static triangle meshes
  G_VERIFY(crd.readInt() == _MAKE4C('MESH'));
#if TRACE_PHYS_SCENE_LOADING
  DEBUG_CTX("Bullet loading: %d tr-meshesd", tri_mesh_actors);
#endif
  Tab<btCollisionShape *> shapes(tmpmem);
  Tab<char> triMeshData(tmpmem);
  btTransform tm0(btMatrix3x3(1, 0, 0, 0, 1, 0, 0, 0, 1), btVector3(0, 0, 0));

  for (int ai = 0; ai < tri_mesh_actors; ai++)
  {
    int mat_count = nm->map.size();
    shapes.clear();
    shapes.reserve(mat_count);
    for (int id = 0; id < mat_count; id++)
    {
      int fcnt = 0;
      crd.read(&fcnt, 4);
      if (!fcnt)
        continue;
      triMeshData.resize(crd.readInt());
      crd.read(triMeshData.data(), data_size(triMeshData));

      const char *mat_name = nm->map[id];
      int pmid = PhysMat::getMaterial(mat_name).id;
      int bfid = PhysMat::getMaterial(pmid).bfid;
      int nxid = PhysMat::getPhysBodyMaterial(pmid);
#if TRACE_PHYS_SCENE_LOADING
      debug("Bullet loading: %d faces with material %s pmid %d (bfid %d, NX mat %d) dump_sz=%d", fcnt,
        PhysMat::getMaterial(pmid).name.str(), pmid, bfid, nxid, data_size(triMeshData));
#endif

      btBulletWorldImporter imp;
      bParse::btBulletFile bulletFile2(triMeshData.data(), data_size(triMeshData));

      G_VERIFY(imp.loadFileFromMemory(&bulletFile2));
      G_VERIFY(imp.getNumCollisionShapes() == 1);
      shapes.push_back(imp.getCollisionShapeByIndex(0));
      append_items(sc->dataBlocks, bulletFile2.m_dataBlocks.size(), &bulletFile2.m_dataBlocks[0]);
      bulletFile2.m_dataBlocks.resize(0);
    }
    if (!shapes.size())
      continue;

    btRigidBody *body = NULL;
    if (shapes.size() == 1)
      body = new btRigidBody(0, NULL, shapes[0]);
    else
    {
      btCompoundShape *compound = new btCompoundShape();
      for (int i = 0; i < shapes.size(); i++)
        compound->addChildShape(tm0, shapes[i]);
      body = new btRigidBody(0, NULL, compound);
    }
    scene.addRigidBody(body, 0x3, 0x3);
    sc->bodies.push_back(body);

#if TRACE_PHYS_SCENE_LOADING
    DEBUG_CTX("Bullet collision loaded, actor #%d: %d shapes", ai, shapes.size());
#endif
  }
  clear_and_shrink(shapes);
  clear_and_shrink(triMeshData);


  // load static convex meshes
  G_VERIFY(crd.readInt() == _MAKE4C('CONV'));
  while (conv_mesh_actors > 0)
  {
    conv_mesh_actors--;
  }

  // load static cylinders
  G_VERIFY(crd.readInt() == _MAKE4C('CYL'));
  while (cyl_actors > 0)
  {
    cyl_actors--;
  }

  // load static boxes
  G_VERIFY(crd.readInt() == _MAKE4C('BOX'));
  while (box_actors > 0)
  {
    TMatrix box;

    const char *mat_name = nm->map[crd.readInt()];
    int cnt = crd.readInt();
    int pmid = PhysMat::getMaterial(mat_name).id;
    int bfid = PhysMat::getMaterial(pmid).bfid;
    int nxid = PhysMat::getPhysBodyMaterial(pmid);
#if TRACE_PHYS_SCENE_LOADING
    debug("Bullet loading: %d boxes with material %s pmid %d (bfid %d, NX mat %d)", cnt, PhysMat::getMaterial(pmid).name.str(), pmid,
      bfid, nxid);
#endif

    while (cnt > 0)
    {
      crd.read(&box, sizeof(TMatrix));

      btVector3 sz(length(box.getcol(0)), length(box.getcol(1)), length(box.getcol(2)));
      box.setcol(0, box.getcol(0) / sz.x());
      box.setcol(1, box.getcol(1) / sz.y());
      box.setcol(2, box.getcol(2) / sz.z());

      btRigidBody *body = new btRigidBody(0, NULL, new btBoxShape(sz));
      body->setWorldTransform(to_btTransform(box));
      scene.addRigidBody(body, 0x3, 0x3);
      sc->bodies.push_back(body);

      cnt--;
      box_actors--;
    }
  }

  // load static spheres
  G_VERIFY(crd.readInt() == _MAKE4C('SPH'));
  while (sph_actors > 0)
  {
    Point4 sph;

    const char *mat_name = nm->map[crd.readInt()];
    int cnt = crd.readInt();
    int pmid = PhysMat::getMaterial(mat_name).id;
    int bfid = PhysMat::getMaterial(pmid).bfid;
    int nxid = PhysMat::getPhysBodyMaterial(pmid);
#if TRACE_PHYS_SCENE_LOADING
    debug("Bullet loading: %d spheres with material %s pmid %d (bfid %d, NX mat %d)", cnt, PhysMat::getMaterial(pmid).name.str(), pmid,
      bfid, nxid);
#endif

    while (cnt > 0)
    {
      crd.read(&sph, sizeof(Point4));

      btRigidBody *body = new btRigidBody(0, NULL, new btSphereShape(sph.w));
      tm0.setOrigin(btVector3(sph.x, sph.y, sph.z));
      body->setWorldTransform(tm0);
      scene.addRigidBody(body, 0x3, 0x3);
      sc->bodies.push_back(body);

      cnt--;
      sph_actors--;
    }
  }

  // load static capsules
  G_VERIFY(crd.readInt() == _MAKE4C('CAP'));
  while (cap_actors > 0)
  {
    TMatrix cap;

    const char *mat_name = nm->map[crd.readInt()];
    int cnt = crd.readInt();
    int pmid = PhysMat::getMaterial(mat_name).id;
    int bfid = PhysMat::getMaterial(pmid).bfid;
    int nxid = PhysMat::getPhysBodyMaterial(pmid);
#if TRACE_PHYS_SCENE_LOADING
    debug("Bullet loading: %d capsules with material %s pmid %d (bfid %d, NX mat %d)", cnt, PhysMat::getMaterial(pmid).name.str(),
      pmid, bfid, nxid);
#endif

    while (cnt > 0)
    {
      crd.read(&cap, sizeof(TMatrix));

      float rad = length(cap.getcol(0));
      float ht = length(cap.getcol(1));
      cap.setcol(0, cap.getcol(0) / rad);
      cap.setcol(1, cap.getcol(1) / ht);

      btRigidBody *body = new btRigidBody(0, NULL, new btCapsuleShapeX(rad, ht));
      body->setWorldTransform(to_btTransform(cap));
      scene.addRigidBody(body, 0x3, 0x3);
      sc->bodies.push_back(body);

      cnt--;
      cap_actors--;
    }
  }

  memfree(nm, tmpmem);
  del_it(zlib_crd);

  bullet::swcRegistry.push_back(sc);
  return sc;
}

bool PhysWorld::unloadSceneCollision(void *handle)
{
  if (!handle)
    return false;

  for (int i = bullet::swcRegistry.size() - 1; i >= 0; i--)
    if (bullet::swcRegistry[i] == handle)
    {
      G_ASSERT(bullet::swcRegistry[i]->pw == this);
      for (int j = 0, je = bullet::swcRegistry[i]->bodies.size(); j < je; j++)
        scene.removeCollisionObject(bullet::swcRegistry[i]->bodies[j]);
      for (int j = 0, je = bullet::swcRegistry[i]->dataBlocks.size(); j < je; j++)
        delete[] bullet::swcRegistry[i]->dataBlocks[j];
      delete bullet::swcRegistry[i];
      erase_items(bullet::swcRegistry, i, 1);
      return true;
    }
  return false;
}
