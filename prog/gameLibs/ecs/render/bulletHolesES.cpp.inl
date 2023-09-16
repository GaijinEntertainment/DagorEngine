#include <ecs/render/bulletHolesES.h>

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <ecs/anim/anim.h>
#include <daECS/core/updateStage.h>
#include <ecs/render/decalsES.h>

#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#include <gameRes/dag_collisionResource.h>
#include <shaders/dag_dynSceneRes.h>
#include <math/dag_geomTree.h>
#include <math/random/dag_random.h>
#include <debug/dag_debug3d.h>
#include <util/dag_convar.h>
#include <util/dag_console.h>
#include <billboardDecals/billboardDecals.h>
#include <billboardDecals/matProps.h>
#include <decalMatrices/decalsMatrices.h>

#if HAS_ECS_PHYS
#include <ecs/phys/physEvents.h>
#include <ecs/phys/collRes.h>
#include <gamePhys/common/loc.h>
#include <gamePhys/props/atmosphere.h>
#include <gamePhys/ballistics/projectileBallisticProps.h>
#else
ECS_DECLARE_BOXED_TYPE(CollisionResource);
ECS_UNICAST_EVENT_TYPE(EventOnProjectileHit, Point3 /*hit_pos*/, Point3 /*norm*/, int /*shell_id*/, int /*phys_mat_id*/,
  int /*coll_node_id*/, ecs::EntityId /*projectile_eid*/)
#endif

namespace decals
{
void (*add_bullet_hole)(const Point3 &pos, const Point3 &norm, float rad, uint32_t id, uint32_t matrix_id) = nullptr;
}

struct BulletHoles
{
  struct DecalMatrixId
  {
    uint32_t id = DecalsMatrices::INVALID_MATRIX_ID;
    DecalMatrixId() { id = decals::add_new_matrix_id_bullet_hole(); }
    ~DecalMatrixId() { decals::del_matrix_id_bullet_hole(id); }
    DecalMatrixId(DecalMatrixId &&a)
    {
      id = a.id;
      a.id = DecalsMatrices::INVALID_MATRIX_ID;
    }
    DecalMatrixId &operator=(DecalMatrixId &&a)
    {
      id = a.id;
      a.id = DecalsMatrices::INVALID_MATRIX_ID;
      return *this;
    }
    DecalMatrixId(const DecalMatrixId &) = delete;
    DecalMatrixId &operator=(const DecalMatrixId &) = delete;
    bool operator!() const { return id == DecalsMatrices::INVALID_MATRIX_ID; }
  };
  struct MovingCollisionNode
  {
    int nodeId = -1;
    DecalMatrixId bulletHolesMatrixId;
  };
  eastl::vector<MovingCollisionNode> movingCollisionNodeIds;
  eastl::vector<int> ignoreCollisionNodeIds;
  DecalMatrixId bulletHolesMatrixId;

  bool onLoaded(const ecs::EntityManager &mgr, ecs::EntityId eid)
  {
    const CollisionResource *collision = mgr.getNullable<CollisionResource>(eid, ECS_HASH("collres"));

    const ecs::Array &movingCollisionNodes = mgr.get<ecs::Array>(eid, ECS_HASH("bullet_holes__movingCollisionNodes"));
    for (const auto &item : movingCollisionNodes)
    {
      const char *nodeName = item.get<ecs::string>().c_str();
      const int nodeId = collision->getNodeIndexByName(nodeName);
      if (nodeId < 0)
      {
        logerr("Collision node '%s' not found", nodeName);
        continue;
      }

      auto &mn = movingCollisionNodeIds.push_back();
      mn.nodeId = nodeId;
    }

    const ecs::Array &ignoreCollisionNodes = mgr.get<ecs::Array>(eid, ECS_HASH("bullet_holes__ignoreCollisionNodes"));
    for (const auto &item : ignoreCollisionNodes)
    {
      const char *nodeName = item.get<ecs::string>().c_str();
      const int nodeId = collision->getNodeIndexByName(nodeName);
      if (nodeId < 0)
      {
        logerr("Collision node '%s' not found", nodeName);
        continue;
      }

      ignoreCollisionNodeIds.push_back(nodeId);
    }

    return true;
  }
};

ECS_DECLARE_RELOCATABLE_TYPE(BulletHoles);
ECS_REGISTER_RELOCATABLE_TYPE(BulletHoles, nullptr);
ECS_AUTO_REGISTER_COMPONENT_DEPS(BulletHoles, "bullet_holes", nullptr, 0, "collres");

ECS_REQUIRE_NOT(ecs::Tag disableUpdate)
ECS_NO_ORDER
static void bullet_holes_updater_es(const ecs::UpdateStageInfoAct &, const TMatrix &transform, const BulletHoles &bullet_holes,
  const AnimV20::AnimcharBaseComponent &animchar, CollisionResource &collres)
{
  if (!bullet_holes.bulletHolesMatrixId) // how that could happen?
    return;

  decals::update_bullet_hole(bullet_holes.bulletHolesMatrixId.id, transform);

  for (auto &n : bullet_holes.movingCollisionNodeIds)
  {
    if (!n.bulletHolesMatrixId)
      continue;

    TMatrix wtm;
    const CollisionNode *node = collres.getNode(n.nodeId);
    collres.getCollisionNodeTm(node, transform, &const_cast<GeomNodeTree &>(animchar.getNodeTree()), wtm);

    decals::update_bullet_hole(n.bulletHolesMatrixId.id, wtm);
  }
}

ECS_REQUIRE_NOT(ecs::Tag disableUpdate)
static inline void bullet_holes_es_event_handler(const EventOnProjectileHit &evt, const TMatrix &transform, BulletHoles &bullet_holes,
  const AnimV20::AnimcharBaseComponent &animchar, CollisionResource &collres)
{
  const Point3 &pos = evt.get<0>();
  const Point3 &normal = evt.get<1>();
  const int shellId = evt.get<2>();
  const int materialId = evt.get<3>();
  const int collNodeId = evt.get<4>();

  const int projectileType = get_remaped_decal_tex(materialId);

  if (projectileType < 0 || !bullet_holes.bulletHolesMatrixId)
    return;

#if HAS_ECS_PHYS
  const float radius = ballistics::ProjectileProps::get_props(shellId)->caliber * BillboardDecalsProps::get_props(materialId)->scale;
#else
  const float radius = 0.01;
#endif

  auto ignore = eastl::find(bullet_holes.ignoreCollisionNodeIds.begin(), bullet_holes.ignoreCollisionNodeIds.end(), collNodeId);
  if (ignore != bullet_holes.ignoreCollisionNodeIds.end())
    return;

  TMatrix itm;
  uint32_t bulletHolesMatrixId;

  auto res = eastl::find_if(bullet_holes.movingCollisionNodeIds.begin(), bullet_holes.movingCollisionNodeIds.end(),
    [collNodeId](const BulletHoles::MovingCollisionNode &n) { return n.nodeId == collNodeId; });
  if (res != bullet_holes.movingCollisionNodeIds.end())
  {
    TMatrix wtm;
    const CollisionNode *node = collres.getNode(res->nodeId);
    collres.getCollisionNodeTm(node, transform, &const_cast<GeomNodeTree &>(animchar.getNodeTree()), wtm);

    itm = inverse(wtm);
    bulletHolesMatrixId = res->bulletHolesMatrixId.id;
  }
  else
  {
    itm = inverse(transform);
    bulletHolesMatrixId = bullet_holes.bulletHolesMatrixId.id;
  }

  G_ASSERT_RETURN(decals::add_bullet_hole != nullptr, );
  decals::add_bullet_hole(itm * pos, itm % normal, radius, projectileType, bulletHolesMatrixId);
}
