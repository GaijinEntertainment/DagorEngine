#include <phys/dag_simplePhysObject.h>
#include <phys/dag_physSysInst.h>
#include <phys/dag_physics.h>
#include <shaders/dag_dynSceneRes.h>
#include <scene/dag_physMat.h>
#include <math/dag_geomTree.h>
#include <generic/dag_tabUtils.h>
#include <debug/dag_debug.h>


template <>
SimplePhysObject::SimplePhysObjectClass() :
  body(NULL),
  nodeTree(NULL)
#ifndef NO_3D_GFX
  ,
  model(NULL),
  extRenderTm(NULL)
#endif
{}


template <>
SimplePhysObject::~SimplePhysObjectClass()
{
#ifndef NO_3D_GFX
  del_it(model);
#endif
  del_it(body);
  del_it(nodeTree);
}


template <>
void SimplePhysObject::resetTm(const TMatrix &tm)
{
  body->setTm(tm * tmModel2body[0]);
}

static int getMat(const char *name)
{
  int cmat_id = name[0] ? (int)PhysMat::getMaterialId(name) : -1;
  if (IsPhysMatID_Valid(cmat_id))
    return PhysMat::getMaterial(cmat_id).physBodyMaterial;
  return -1;
}

template <>
void SimplePhysObject::init(const DynamicPhysObjectData *data, PhysWorld *w)
{
  G_ASSERT(data->physRes && w);
  G_ASSERT(data->physRes->getBodies().size() == 1);
  G_ASSERT(data->models.size() == 1);

  physRes = data->physRes;

  const PhysicsResource::Body &res_body = data->physRes->getBodies()[0];

  tmModel2body = &res_body.tm;
  tmBody2model = &res_body.tmInvert;

  // create collision primitives
  PhysCompoundCollision coll;

  // sphere collisions
  for (int i = 0; i < res_body.sphColl.size(); ++i)
  {
    const PhysicsResource::SphColl &c = res_body.sphColl[i];

    TMatrix tm;
    tm.identity();
    tm.setcol(3, c.center);

    coll.addChildCollision(new PhysSphereCollision(c.radius), tm);
  }

  // box collisions
  for (int i = 0; i < res_body.boxColl.size(); ++i)
  {
    PhysicsResource::BoxColl c = res_body.boxColl[i];
    PhysCollision::normalizeBox(c.size, c.tm);
    coll.addChildCollision(new PhysBoxCollision(c.size.x, c.size.y, c.size.z), c.tm);
  }

  // capsule collisions
  for (int i = 0; i < res_body.capColl.size(); ++i)
  {
    const PhysicsResource::CapColl &c = res_body.capColl[i];

    real ext = length(c.extent);

    TMatrix tm;
    tm.setcol(3, c.center);

    Point3 dir = c.extent / ext;
    Point3 side = dir % Point3(1, 0, 0);
    if (side.length() < 0.1f)
      side = normalize(dir % Point3(0, 1, 0));
    else
      side.normalize();

    tm.setcol(0, dir);
    tm.setcol(1, side);
    tm.setcol(2, dir % side);

    auto *cap = new PhysCapsuleCollision(c.radius, ext * 2, PhysCollision::detectDirAxisIndex(tm));
    coll.addChildCollision(cap, tm);
  }

  PhysBodyCreationData pbcd;
  pbcd.momentOfInertia = res_body.momj;
  // pbcd.allowFastInaccurateCollTm = false;
  int mat = getMat(res_body.materialName);
  if (mat >= 0)
    pbcd.materialId = mat;
  body = new PhysBody(w, res_body.mass, &coll, res_body.tm, pbcd);
  coll.clear();

  // create node tree if needed
  if (data->nodeTree)
    nodeTree = new GeomNodeTree(*data->nodeTree);

#ifndef NO_3D_GFX
  model = new DynamicRenderableSceneInstance(data->models[0]);
  extRenderTm = NULL;

  const RoNameMapEx &map = model->getLodsResource()->getNames().node;
  if (nodeTree && map.nameCount() > 1)
  {
    clear_and_resize(treeHelpers, map.nameCount() - 1);

    iterate_names(map, [&](int i, const char *name) {
      if (i > 0)
      {
        treeHelpers[i - 1] = nodeTree->findNodeIndex(name);
        if (!treeHelpers[i - 1])
          DAG_FATAL("cannot find node <%s> in skeleton", name);
      }
    });
  }
#endif // NO_3D_GFX
}


#ifndef NO_3D_GFX
template <>
void SimplePhysObject::updateModelTms()
{
  TMatrix tm;
  getBodyVisualTm(tm);

  tm = tm * tmBody2model[0];
  model->setNodeWtm(0, tm);
  if (!nodeTree)
    return;

  nodeTree->setRootTmScalar(tm);
  nodeTree->invalidateWtm();
  nodeTree->calcWtm();
  for (int i = 0; i < treeHelpers.size(); ++i)
    model->setNodeWtm(i + 1, nodeTree->getNodeWtmRel(treeHelpers[i]));
}

#endif // NO_3D_GFX
