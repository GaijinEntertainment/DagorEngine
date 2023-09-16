#include <gameRes/dag_collisionResource.h>
#include <ecs/core/entityManager.h>
#include <scene/dag_physMat.h>

static CollisionResource *create_collres_from_ecs_object(const ecs::Array &desc)
{
  CollisionResource *collres = new (midmem->alloc(sizeof(CollisionResource)), _NEW_INPLACE) CollisionResource();
  BBox3 totalBBox;
  BSphere3 totalBSphere;
  for (const auto &nodeIt : desc)
  {
    const ecs::Object &nodeDesc = nodeIt.get<ecs::Object>();
    const ecs::string &name = nodeDesc[ECS_HASH("name")].get<ecs::string>();
    const ecs::string *material = nodeDesc[ECS_HASH("physMatId")].getNullable<ecs::string>();
    int nodeNo = collres->getAllNodes().size();
    CollisionNode &nn = collres->createNode();
    nn.name = name.c_str();
    nn.nodeIndex = nodeNo;
    nn.tm = TMatrix::IDENT;
    nn.physMatId = material ? PhysMat::getMaterialId(material->c_str()) : PHYSMAT_INVALID;
    nn.flags |= nn.TRANSLATE; // actually IDENT
    bool created = false;
    {
      if (const Point4 *bsph = nodeDesc[ECS_HASH("bsph")].getNullable<Point4>())
      {
        nn.boundingSphere = BSphere3(Point3::xyz(*bsph), bsph->w);
        nn.modelBBox = BBox3(nn.boundingSphere);
        nn.type = COLLISION_NODE_TYPE_SPHERE;
        created = true;
      }
    }
    if (!created)
    {
      const Point3 *bmin = nodeDesc[ECS_HASH("bbox_min")].getNullable<Point3>();
      const Point3 *bmax = nodeDesc[ECS_HASH("bbox_max")].getNullable<Point3>();
      if (bmin && bmax)
      {
        nn.modelBBox = BBox3(*bmin, *bmax);
        nn.boundingSphere = BSphere3(nn.modelBBox.center(), nn.modelBBox.width().length() / 2.f);
        nn.type = COLLISION_NODE_TYPE_BOX;
        created = true;
      }
    }
    if (!created)
    {
      const Point3 *p0 = nodeDesc[ECS_HASH("capsule_p0")].getNullable<Point3>();
      const Point3 *p1 = nodeDesc[ECS_HASH("capsule_p1")].getNullable<Point3>();
      const float *r = nodeDesc[ECS_HASH("capsule_r")].getNullable<float>();
      if (p0 && p1 && r)
      {
        nn.modelBBox = (BBox3(BSphere3(*p0, *r)) += BSphere3(*p1, *r));
        nn.boundingSphere = BSphere3((*p0 + *p1) / 2.f, (*p0 - *p1).length() / 2.f + *r);
        nn.capsule.set(*p0, *p1, *r);
        nn.type = COLLISION_NODE_TYPE_CAPSULE;
        created = true;
      }
    }
    if (!created)
    {
      logerr("Can't detect node <%s> type in collres desc", name.c_str());
      break;
    }

    totalBBox += nn.modelBBox;
    totalBSphere += nn.boundingSphere;
  }
  collres->vFullBBox = v_ldu_bbox3(totalBBox);
  collres->vBoundingSphere = v_perm_xyzd(v_ldu(&totalBSphere.c.x), v_splats(totalBSphere.r2));
  collres->sortNodesList();
  collres->rebuildNodesLL();
  return collres;
}

CollisionResource *create_collres_from_ecs_object(ecs::EntityManager &mgr, ecs::EntityId eid)
{
  const ecs::Array *desc = mgr.getNullable<ecs::Array>(eid, ECS_HASH("collres__desc"));
  return desc ? create_collres_from_ecs_object(*desc) : nullptr;
}
