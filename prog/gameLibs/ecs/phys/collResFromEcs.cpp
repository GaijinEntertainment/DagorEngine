// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_collisionResource.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <scene/dag_physMat.h>

void add_collres_nodes_from_ecs_object(CollisionResource *collres, const ecs::Array &desc)
{
  BBox3 totalBBox;
  BSphere3 totalBSphere;
  for (const auto &nodeIt : desc)
  {
    const ecs::Object &nodeDesc = nodeIt.get<ecs::Object>();
    const ecs::string &name = nodeDesc[ECS_HASH("name")].get<ecs::string>();
    const ecs::string *material = nodeDesc[ECS_HASH("physMatId")].getNullable<ecs::string>();
    int16_t matId = material ? PhysMat::getMaterialId(material->c_str()) : PHYSMAT_INVALID;
    int nodeId = -1;
    if (const Point4 *bsph = nodeDesc[ECS_HASH("bsph")].getNullable<Point4>())
    {
      nodeId = collres->addSphereNode(name.c_str(), matId, BSphere3(Point3::xyz(*bsph), bsph->w));
    }
    else if (const Point3 *bmin = nodeDesc[ECS_HASH("bbox_min")].getNullable<Point3>())
    {
      const Point3 *bmax = nodeDesc[ECS_HASH("bbox_max")].getNullable<Point3>();
      if (bmax)
        nodeId = collres->addBoxNode(name.c_str(), matId, BBox3(*bmin, *bmax));
    }
    if (nodeId < 0)
    {
      const Point3 *p0 = nodeDesc[ECS_HASH("capsule_p0")].getNullable<Point3>();
      const Point3 *p1 = nodeDesc[ECS_HASH("capsule_p1")].getNullable<Point3>();
      const float *r = nodeDesc[ECS_HASH("capsule_r")].getNullable<float>();
      if (p0 && p1 && r)
        nodeId = collres->addCapsuleNode(name.c_str(), matId, *p0, *p1, *r);
    }
    if (nodeId < 0)
    {
      logerr("Can't detect node <%s> type in collres desc", name.c_str());
      break;
    }

    totalBBox += collres->getNodeBBox(nodeId);
    totalBSphere += collres->getNodeBSphere(nodeId);
  }
  if (v_extract_w(collres->vBoundingSphere) > 0.f)
  {
    Point3_vec4 c;
    v_st(&c.x, collres->vBoundingSphere);
    BSphere3 oldBsph(c, sqrtf(v_extract_w(collres->vBoundingSphere)));
    totalBSphere += oldBsph;
  }
  v_bbox3_add_box(collres->vFullBBox, v_ldu_bbox3(totalBBox));
  collres->vBoundingSphere = v_perm_xyzd(v_ldu(&totalBSphere.c.x), v_splats(totalBSphere.r2));
  collres->sortNodesList();
  collres->rebuildNodesLL();
}

static CollisionResource *create_collres_from_ecs_object(const ecs::Array &desc)
{
  CollisionResource *collres = new (midmem->alloc(sizeof(CollisionResource)), _NEW_INPLACE) CollisionResource();
  v_bbox3_init_empty(collres->vFullBBox);
  collres->vBoundingSphere = v_zero();
  add_collres_nodes_from_ecs_object(collres, desc);
  return collres;
}

CollisionResource *create_collres_from_ecs_object(ecs::EntityManager &mgr, ecs::EntityId eid)
{
  const ecs::Array *desc = mgr.getNullable<ecs::Array>(eid, ECS_HASH("collres__desc"));
  return desc ? create_collres_from_ecs_object(*desc) : nullptr;
}
