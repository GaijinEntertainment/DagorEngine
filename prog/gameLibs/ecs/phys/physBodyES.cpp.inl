#include <ecs/core/entityManager.h>
#include <ecs/phys/physBody.h>
#include <ecs/phys/collRes.h>
#include <debug/dag_assert.h>
#include <phys/dag_physics.h>
#include <gamePhys/collision/collisionLib.h>
#include <EASTL/unique_ptr.h>

PhysWorld *g_phys_world = NULL;

typedef PhysBody physbody_t;

#define ANAME(x) "phys_body_bt__" #x

static const char *const COLL_TYPES[] = {"sph", "box", "cap", "cyl", "plane"};
static constexpr const size_t MAX_COLL_OBJ_SZ = eastl::max( //
  eastl::max(sizeof(PhysPlaneCollision), sizeof(PhysCompoundCollision)),
  eastl::max(eastl::max(sizeof(PhysSphereCollision), sizeof(PhysBoxCollision)),
    eastl::max(sizeof(PhysCapsuleCollision), sizeof(PhysCylinderCollision))));


static PhysCollision *set_coll_type(ecs::EntityId eid, const char *coll_type_str, void *mem)
{
  Point4 cparams = g_entity_mgr->get<Point4>(eid, ECS_HASH(ANAME(coll_params)));
  TMatrix ctm = g_entity_mgr->getOr(eid, ECS_HASH(ANAME(coll_tm)), TMatrix::IDENT);
  int collTypeInt = -1;
  for (int i = 0; i < countof(COLL_TYPES) && collTypeInt < 0; ++i)
    if (strcmp(COLL_TYPES[i], coll_type_str) == 0)
      collTypeInt = i;
  bool ident_tm = (memcmp(&ctm, &TMatrix::IDENT, sizeof(ctm)) == 0); //-V1014
  if (!ident_tm && collTypeInt == 1 /*box*/)
  {
    Point3 ext = Point3::xyz(cparams);
    if (PhysCollision::normalizeBox(ext, ctm))
      cparams.set_xyz0(ext);
  }
  G_STATIC_ASSERT(sizeof(PhysCompoundCollision) <= MAX_COLL_OBJ_SZ);
#define SET_COLL(type, klass, ...)                                \
  case type:                                                      \
  {                                                               \
    G_STATIC_ASSERT(sizeof(klass) <= MAX_COLL_OBJ_SZ);            \
    if (ident_tm)                                                 \
      return new (mem, _NEW_INPLACE) klass(__VA_ARGS__);          \
    else                                                          \
    {                                                             \
      auto *coll = new (mem, _NEW_INPLACE) PhysCompoundCollision; \
      coll->addChildCollision(new klass(__VA_ARGS__), ctm);       \
      return coll;                                                \
    }                                                             \
  }                                                               \
  break
  switch (collTypeInt)
  {
    SET_COLL(0, PhysSphereCollision, cparams.x);
    SET_COLL(1, PhysBoxCollision, cparams.x, cparams.y, cparams.z);
    SET_COLL(2, PhysCapsuleCollision, cparams.x, cparams.y, ident_tm ? 0 : PhysCollision::detectDirAxisIndex(ctm));  //-V547
    SET_COLL(3, PhysCylinderCollision, cparams.x, cparams.y, ident_tm ? 0 : PhysCollision::detectDirAxisIndex(ctm)); //-V547
    SET_COLL(4, PhysPlaneCollision, Plane3(cparams.x, cparams.y, cparams.z, cparams.w));
    default: G_ASSERTF(0, "unknown phys collision type %s", coll_type_str);
  };
#undef SET_COLL
  return nullptr;
}

struct PhysBodyCreationContext
{
  TMatrix tm = TMatrix::IDENT;
  PhysCollision *coll = nullptr;
  char collStor[MAX_COLL_OBJ_SZ];
  PhysBodyCreationData pbcd;
  float mass = 0;

  PhysBodyCreationContext(const ecs::EntityManager &mgr, ecs::EntityId eid) //-V1077
  {
    const char *collType = mgr.getOr(eid, ECS_HASH(ANAME(coll_type)), ecs::nullstr);
    if (collType)
      coll = set_coll_type(eid, collType, collStor);

    if (auto massMx = mgr.getNullable<Point4>(eid, ECS_HASH(ANAME(mass_mx))))
      mass = massMx->x, pbcd.momentOfInertia = Point3(massMx->y, massMx->z, massMx->w);

    if (auto transform = mgr.getNullable<TMatrix>(eid, ECS_HASH("transform")))
      tm = *transform;

    const bool inferFromCollRes = mgr.getOr(eid, ECS_HASH("phys_body__inferFromCollRes"), false);
    const CollisionResource *collRes = mgr.getNullable<CollisionResource>(eid, ECS_HASH("collres"));
    if (inferFromCollRes && collRes)
    {
      // calculate collision, mass and tm based on what we have in collision resource
      dacoll::PhysBodyProperties physProps;
      PhysCompoundCollision *compound = new (collStor, _NEW_INPLACE) PhysCompoundCollision;
      dacoll::build_dynamic_collision_from_coll_resource(collRes, compound, physProps);

      const float density = mgr.getOr(eid, ECS_HASH("phys_body__density"), 1000.f); // water by default
      mass = physProps.volume * density;
      pbcd.momentOfInertia = physProps.momentOfInertia * mass;
      pbcd.materialId = PhysMat::getMaterial(PHYSMAT_DEFAULT).physBodyMaterial;
      pbcd.friction = 1.f;
      pbcd.restitution = 0.f;
      coll = compound;
    }
  }
  ~PhysBodyCreationContext()
  {
    if (coll)
      PhysCollision::clearAllocatedData(*coll);
  }
};

struct PhysBodyECSConstruct : public PhysBody
{
  PhysBodyECSConstruct(const PhysBodyCreationContext &pbcc) :
    PhysBody(dacoll::get_phys_world(), pbcc.mass, pbcc.coll, pbcc.tm, pbcc.pbcd)
  {}
  PhysBodyECSConstruct(const ecs::EntityManager &mgr, ecs::EntityId eid) : PhysBodyECSConstruct(PhysBodyCreationContext(mgr, eid))
  {
    if (auto use_gravity = mgr.getNullable<bool>(eid, ECS_HASH(ANAME(gravity))))
      disableGravity(!*use_gravity);
  }
};

ECS_REGISTER_MANAGED_TYPE(PhysBody, nullptr, typename ecs::CreatorSelector<PhysBody ECS_COMMA PhysBodyECSConstruct>::type);

ECS_AUTO_REGISTER_COMPONENT_DEPS(PhysBody, "phys_body", nullptr, 0, "?collres");
ECS_DEF_PULL_VAR(phys_body);

ECS_NO_ORDER
static void update_phys_body_transform_es(const ecs::UpdateStageInfoAct &, TMatrix &transform, const physbody_t &phys_body)
{
  phys_body.getTm(transform);
}
