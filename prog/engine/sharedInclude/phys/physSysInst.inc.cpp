#include <phys/dag_physSysInst.h>
#include <phys/dag_physics.h>
#include <debug/dag_log.h>
#include <scene/dag_physMat.h>
#include <osApiWrappers/dag_localConv.h>
#include <debug/dag_debug.h>
#include <phys/dag_physDecl.h>
#include <vecmath/dag_vecMath.h>

inline const TMatrix &unscale_tm(TMatrix &tm) // Faster than tm.orthonormalize; equivalent for quat-based orientations (i.e. Jolt)
{
  tm.setcol(0, normalize(tm.getcol(0)));
  tm.setcol(1, normalize(tm.getcol(1)));
  tm.setcol(2, normalize(tm.getcol(2)));
  return tm;
}

PhysSystemInstance::PhysSystemInstance(PhysicsResource *res, PhysWorld *world, const TMatrix &tm, void *userData, uint16_t fgroup,
  uint16_t fmask, bool add_to_world) :
  resource(res), bodies(midmem), joints(midmem)
{
  G_ASSERT(res);
  G_ASSERT(world);

  float sx = length(tm.getcol(0)), sy = length(tm.getcol(1)), sz = length(tm.getcol(2));
  scaleTm.setcol(0, Point3(sx, 0.f, 0.f));
  scaleTm.setcol(1, Point3(0.f, sy, 0.f));
  scaleTm.setcol(2, Point3(0.f, 0.f, sz));
  scaleTm.setcol(3, ZERO<Point3>());
  const float scale = cbrtf(sx * sy * sz);

  G_ASSERTF(scale > /* 1% */ 1e-2f, "Unreasonable scale=%f, tm=%@", scale, tm);

  bodies.reserve(resource->getBodies().size());
  int jcnt = resource->getRdBallJoints().size() + resource->getRdHingeJoints().size() + resource->getRevoluteJoints().size() +
             resource->getSphericalJoints().size();
  for (auto &rb : resource->getBodies())
    bodies.emplace_back(rb, world, tm, userData, fgroup, fmask, jcnt > 0, add_to_world, scale);

  joints.reserve(jcnt);

  auto jointPos = [&](const Point3 &p) { return tm * p; };
  auto jointDir = [&](const Point3 &d) { return normalize(tm % d); };

  for (const auto &jnt : resource->getRdBallJoints())
  {
    TMatrix jtm = tm * jnt.tm;
    unscale_tm(jtm);
    PhysJoint *j = world->createRagdollBallJoint(bodies[jnt.body1].body.get(), bodies[jnt.body2].body.get(), jtm, jnt.minLimit,
      jnt.maxLimit, jnt.damping, jnt.twistDamping, jnt.stiffness);

    if (!j)
      logerr("can't create RagBall Joint");
    joints.push_back(j);
  }

  for (const auto &jnt : resource->getRdHingeJoints())
  {
    PhysJoint *j = world->createRagdollHingeJoint(bodies[jnt.body1].body.get(), bodies[jnt.body2].body.get(), jointPos(jnt.pos),
      jointDir(jnt.axis), jointDir(jnt.midAxis), jointDir(jnt.xAxis), jnt.angleLimit, jnt.damping, jnt.stiffness);

    if (!j)
      logerr("can't create Hinge Joint");
    joints.push_back(j);
  }

  for (const auto &jnt : resource->getRevoluteJoints())
  {
    PhysJoint *j = world->createRevoluteJoint(bodies[jnt.body1].body.get(), bodies[jnt.body2].body.get(), jointPos(jnt.pos),
      jointDir(jnt.axis), jnt.minAngle, jnt.maxAngle, jnt.minRestitution, jnt.maxRestitution, jnt.spring, jnt.projType, jnt.projAngle,
      jnt.projDistance, jnt.damping, jnt.flags);

    if (!j)
      logerr("can't create Revolution Joint");
    joints.push_back(j);
  }

  for (const auto &jnt : resource->getSphericalJoints())
  {
    PhysJoint *j = world->createSphericalJoint(bodies[jnt.body1].body.get(), bodies[jnt.body2].body.get(), jointPos(jnt.pos),
      jointDir(jnt.dir), jointDir(jnt.axis), jnt.minAngle, jnt.maxAngle, jnt.minRestitution, jnt.maxRestitution, jnt.swingValue,
      jnt.swingRestitution, jnt.spring, jnt.damping, jnt.swingSpring, jnt.swingDamping, jnt.twistSpring, jnt.twistDamping,
      jnt.projType, jnt.projDistance, jnt.flags);

    if (!j)
      logerr("can't create Spherical Joint");
    joints.push_back(j);
  }
#if defined(USE_JOLT_PHYSICS)
  if (joints.size())
  {
    static volatile int phys_sys_group_idx = 0;
    phys_jolt_setup_collision_groups_for_joints(interlocked_increment(phys_sys_group_idx), bodies, resource);
  }
#endif
}


PhysSystemInstance::~PhysSystemInstance()
{
  for (int i = 0; i < joints.size(); ++i)
    del_it(joints[i]);
}

void PhysSystemInstance::setGroupAndLayerMask(unsigned group, unsigned mask)
{
  for (auto &b : bodies)
    if ((b.body->getGroupMask() | b.body->getInteractionLayer()) != 0) // Ignore empty collision
      b.body->setGroupAndLayerMask(group, mask);
}

template <typename T>
static inline bool match_joint(const T &jnt, int body1, int body2)
{
  return (jnt.body1 == body1 && jnt.body2 == body2) || (jnt.body1 == body2 && jnt.body2 == body1);
}

static inline bool is_no_collision_pair(const PhysicsResource &res, int body1, int body2)
{
  for (const auto &pr : res.getNoCollisionPairs())
    if ((pr.body0 == body1 && pr.body1 == body2) || (pr.body0 == body2 && pr.body1 == body1))
      return true;
  return false;
}

int PhysSystemInstance::findBodyIdByName(const char *name) const
{
  dag::ConstSpan<PhysicsResource::Body> resbodies = resource->getBodies();
  for (int i = 0; i < resbodies.size(); ++i)
    if (dd_stricmp(resbodies[i].name, name) == 0)
      return i;
  return -1;
}

void PhysSystemInstance::setJointsMotorSettings(float twistFrequeny, float twistDamping, float swingFrequeny, float swingDamping)
{
  for (int i = 0; i < joints.size(); ++i)
  {
    PhysRagdollBallJoint *joint = PhysRagdollBallJoint::cast(joints[i]);
    if (joint == nullptr || joints[i]->jointType != PhysJoint::PJ_CONE_TWIST)
      continue;

    joint->setTwistSwingMotorSettings(twistFrequeny, twistDamping, swingFrequeny, swingDamping);
  }
}

void PhysSystemInstance::disableJointsMotors()
{
  for (int i = 0; i < joints.size(); ++i)
  {
    PhysRagdollBallJoint *joint = PhysRagdollBallJoint::cast(joints[i]);
    if (joint == nullptr || joints[i]->jointType != PhysJoint::PJ_CONE_TWIST)
      continue;

    joint->disableMotors();
  }
}

void PhysSystemInstance::driveBodiesToAnimcharPose(const dag::Span<mat44f *> &skeletonWtms)
{
  dag::ConstSpan<PhysicsResource::RdBallJoint> res_joints = resource->getRdBallJoints();
  for (int i = 0; i < joints.size(); ++i)
  {
    PhysRagdollBallJoint *joint = PhysRagdollBallJoint::cast(joints[i]);

    // Only PJ_CONE_TWIST supports driving to pose for now
    // Luckily this can be enough for ragdolls
    if (joint == nullptr || joints[i]->jointType != PhysJoint::PJ_CONE_TWIST)
      continue;

    const int &b1idx = res_joints[i].body1;
    const int &b2idx = res_joints[i].body2;

    if (b1idx >= skeletonWtms.size() || b2idx >= skeletonWtms.size())
    {
      LOGERR_ONCE(
        "driveBodiesToAnimcharPose: Joint body index out of range. Joint: %d, Body1 idx: %d, Body2 idx: %d, skeletonWtms.size: %d", i,
        b1idx, b2idx, skeletonWtms.size());
      continue;
    }

    // Animations are done on the skeleton and we want to drive physics bodies attached to the skeleton to the pose from skeletonWtms.
    // Joint's 'setTargetOrientation' function expects the orientation of body2 in body1's space.
    // physSystem resource has a tmHelper for each body which is defined like this:
    // skeletonWtm = bodyWtm * helperTm
    //
    // So the naive solution looks like this:
    // bodyWTMFromSkeleton1 = skeletonWtm1 * inverse(helperTm1)
    // bodyWTMFromSkeleton2 = skeletonWtm2 * inverse(helperTm2)
    // orientation = inverse(bodyWTMFromSkeleton1) * bodyWTMFromSkeleton2
    //
    // But this has one additional maxtrix inverse which we can simplify out
    // orientation = inverse(skeletonWtm1 * inverse(helperTm1)) * bodyWTMFromSkeleton2
    // orientation = inverse(inverse(helperTm1)) * inverse(skeletonWtm1) * bodyWTMFromSkeleton2
    // orientation = helperTm1 * inverse(skeletonWtm1) * bodyWTMFromSkeleton2
    // orientation = helperTm1 * inverse(skeletonWtm1) * skeletonWtm2 * inverse(helperTm2)

    const mat44f *s1 = skeletonWtms[b1idx];
    const mat44f *s2 = skeletonWtms[b2idx];

    // We might have some bodies in the resource which are missing a skeleton node.
    // These nodes have a nullptr in the skeletonWtms array, skip them.
    if (!s1 || !s2)
      continue;

    // One body might guide multiple skeleton nodes, i.e. hand body has all finger nodes and the hand itself.
    // But the first helper is always for the node we are attached to, hence the 0 index.
    const TMatrix &helper1Tm = resource->getBodies()[res_joints[i].body1].tmHelpers[0].tm;
    const TMatrix &helper2Tm = resource->getBodies()[res_joints[i].body2].tmHelpers[0].tm;

    mat44f res, h1, h2;
    v_mat44_make_from_43cu(h1, helper1Tm.array);
    v_mat44_make_from_43cu(h2, helper2Tm.array);

    v_mat44_inverse43(res, *s1);
    v_mat44_inverse43(h2, h2);

    // helperTm1 * inverse(skeletonWtm1)
    // res contains inverse(skeletonWtm1)
    v_mat44_mul43(res, h1, res);
    // * skeletonWtm2
    v_mat44_mul43(res, res, *s2);
    // * inverse(helperTm2)
    v_mat44_mul43(res, res, h2);

    TMatrix tm;
    v_mat_43cu_from_mat44(tm.array, res);
    joint->setTargetOrientation(tm);
  }
}


template <typename Pred>
bool PhysSystemInstance::removeJointsIf(Pred pred)
{
  bool removed = false;
  int j = 0;
  auto removeFrom = [&](const auto &res_joints) {
    for (const auto &jnt : res_joints)
    {
      PhysJoint *&joint = joints[j++];
      if (!joint || !pred(jnt))
        continue;
      del_it(joint);
      removed = true;
#if defined(USE_JOLT_PHYSICS)
      if (!is_no_collision_pair(*resource, jnt.body1, jnt.body2))
        phys_jolt_enable_collision_between_bodies(bodies, jnt.body1, jnt.body2);
#endif
    }
  };
  removeFrom(resource->getRdBallJoints());
  removeFrom(resource->getRdHingeJoints());
  removeFrom(resource->getRevoluteJoints());
  removeFrom(resource->getSphericalJoints());
  return removed;
}

bool PhysSystemInstance::removeJoint(const char *body1, const char *body2)
{
  int bodyIdx1 = findBodyIdByName(body1);
  int bodyIdx2 = findBodyIdByName(body2);

  if (bodyIdx1 < 0 || bodyIdx2 < 0)
    return false;

  return removeJointsIf([&](const auto &jnt) { return match_joint(jnt, bodyIdx1, bodyIdx2); });
}

void PhysSystemInstance::removeJoint(const char *name)
{
  removeJointsIf([&](const auto &jnt) { return dd_stricmp(jnt.name, name) == 0; });
}

void PhysSystemInstance::addBodiesToWorld(dag::ConstSpan<int> body_ids, bool drop_joints_to_inactive)
{
  if (drop_joints_to_inactive && !body_ids.empty())
  {
    auto activated = [&](int b) { return find_value_idx(body_ids, b) >= 0; };
    auto inactive = [&](int b) { return !activated(b) && !bodies[b].body->isInWorld(); };
    removeJointsIf(
      [&](const auto &jnt) { return (activated(jnt.body1) && inactive(jnt.body2)) || (inactive(jnt.body1) && activated(jnt.body2)); });
  }

  auto addBody = [&](int b) {
    PhysBody *body = bodies[b].body.get();
    if (!body->isInWorld())
      body->getPhysWorld()->addBody(body, /* kinematic */ false);
  };
  if (body_ids.empty())
    for (int b = 0; b < bodies.size(); ++b)
      addBody(b);
  else
    for (int b : body_ids)
      addBody(b);
}


void PhysSystemInstance::resetTm(const TMatrix &tm)
{
  scaleTm.setcol(0, Point3(length(tm.getcol(0)), 0.f, 0.f));
  scaleTm.setcol(1, Point3(0.f, length(tm.getcol(1)), 0.f));
  scaleTm.setcol(2, Point3(0.f, 0.f, length(tm.getcol(2))));

  for (int i = 0; i < bodies.size(); ++i)
  {
    TMatrix utm = tm * resource->getBodies()[i].tm;
    bodies[i].body->setTm(unscale_tm(utm));
  }

  updateTms();
}


TMatrix *PhysSystemInstance::getTmHelper(const char *name, int *out_body_id)
{
  for (int i = 0; i < bodies.size(); ++i)
  {
    int j = resource->getBodies()[i].findTmHelper(name);
    if (j < 0)
      continue;
    if (out_body_id)
      *out_body_id = i;
    return &bodies[i].tmHelpers[j].wtm;
  }

  if (out_body_id)
    *out_body_id = -1;
  return NULL;
}


PhysBody *PhysSystemInstance::getBody(const char *name) const
{
  for (int i = 0; i < bodies.size(); ++i)
    if (dd_stricmp(resource->getBodies()[i].name, name) == 0)
      return bodies[i].body.get();

  return NULL;
}

void PhysSystemInstance::updateTms(dag::FunctionRef<TMatrix(int)> get_body_tm)
{
  for (int i = 0; i < bodies.size(); ++i)
  {
    TMatrix wtm;
    if (get_body_tm)
      wtm = get_body_tm(i);
    else
      bodies[i].body->getTm(wtm);
    wtm *= scaleTm;
    bodies[i].updateTms(resource->getBodies()[i], wtm);
  }
}


bool PhysSystemInstance::setBodyTmByTmHelper(const char *name, const TMatrix &wtm)
{
  for (int i = 0; i < bodies.size(); ++i)
  {
    int id = resource->getBodies()[i].findTmHelper(name);
    if (id < 0)
      continue;

    TMatrix tm = wtm * inverse(scaleTm * resource->getBodies()[i].tmHelpers[id].tm);
    bodies[i].body->setTm(unscale_tm(tm));

    return true;
  }

  return false;
}


bool PhysSystemInstance::setBodyTmAndVelByTmHelper(const char *name, const TMatrix &wtm_prev, const TMatrix &wtm, real dt)
{
  for (int i = 0; i < bodies.size(); ++i)
  {
    int id = resource->getBodies()[i].findTmHelper(name);
    if (id < 0)
      continue;

    TMatrix itm = inverse(scaleTm * resource->getBodies()[i].tmHelpers[id].tm);

    TMatrix tm_prev = wtm_prev * itm;
    TMatrix tm = wtm * itm;

    bodies[i].body->setTm(unscale_tm(tm_prev));
    bodies[i].body->setTmWithDynamics(unscale_tm(tm), dt);

    return true;
  }

  return false;
}


SimpleString PhysSystemInstance::getBodyMaterialName(PhysBody *body) const
{
  G_ASSERT(bodies.size() == resource->getBodies().size());

  int n;
  for (n = 0; n < bodies.size(); n++)
    if (bodies[n].body.get() == body)
      break;

  if (n == bodies.size())
    return SimpleString();

  return resource->getBodies()[n].materialName;
}


PhysSystemInstance::Body::Body(const PhysicsResource::Body &res_body, PhysWorld *world, const TMatrix &tm, void *userData,
  uint16_t fgroup, uint16_t fmask, bool has_joints, bool add_to_world, float scale)
{
  tmHelpers.resize(res_body.tmHelpers.size());
  for (int i = 0; i < tmHelpers.size(); ++i)
    tmHelpers[i].wtm = res_body.tm * res_body.tmHelpers[i].tm; // Note: scale will be applied on next `updateTms` call

  // create collision primitives
  PhysCompoundCollision coll;
  int cmat_id = -1;

  // sphere collisions
  for (int i = 0; i < res_body.sphColl.size(); ++i)
  {
    const PhysicsResource::SphColl &c = res_body.sphColl[i];

    TMatrix tm;
    tm.identity();
    tm.setcol(3, c.center * scale);

    cmat_id = c.materialName.length() ? (int)PhysMat::getMaterialId(c.materialName) : -1;
    if (IsPhysMatID_Valid(cmat_id))
      cmat_id = PhysMat::getMaterial(cmat_id).physBodyMaterial;
    else
      cmat_id = -1;

    coll.addChildCollision(new PhysSphereCollision(c.radius * scale), tm);
  }

  // box collisions
  for (int i = 0; i < res_body.boxColl.size(); ++i)
  {
    const PhysicsResource::BoxColl &c = res_body.boxColl[i];

    cmat_id = c.materialName.length() ? (int)PhysMat::getMaterialId(c.materialName) : -1;
    if (IsPhysMatID_Valid(cmat_id))
    {
      cmat_id = PhysMat::getMaterial(cmat_id).physBodyMaterial;
    }
    else
      cmat_id = -1;

    TMatrix scaledBoxTm = c.tm;
    scaledBoxTm.setcol(3, c.tm.getcol(3) * scale);
    coll.addChildCollision(new PhysBoxCollision(c.size.x * scale, c.size.y * scale, c.size.z * scale), scaledBoxTm);
  }

  // capsule collisions
  for (int i = 0; i < res_body.capColl.size(); ++i)
  {
    const PhysicsResource::CapColl &c = res_body.capColl[i];

    real ext = length(c.extent);

    TMatrix tm;
    tm.setcol(3, c.center * scale);

    Point3 dir = c.extent / ext;
    Point3 side = dir % Point3(1, 0, 0);
    if (side.length() < 0.1f)
      side = normalize(dir % Point3(0, 1, 0));
    else
      side.normalize();

    tm.setcol(0, dir);
    tm.setcol(1, side);
    tm.setcol(2, dir % side);

    cmat_id = c.materialName.length() ? (int)PhysMat::getMaterialId(c.materialName) : -1;
    if (IsPhysMatID_Valid(cmat_id))
      cmat_id = PhysMat::getMaterial(cmat_id).physBodyMaterial;
    else
      cmat_id = -1;

    auto *cap = new PhysCapsuleCollision(c.radius * scale, ext * 2 * scale, PhysCollision::detectDirAxisIndex(tm));
    coll.addChildCollision(cap, tm);
  }

  PhysBodyCreationData pbcd;
  if (coll.getChildrenCount() == 0)
  {
    // Create fake collision and disable it. Otherwise object will not move at all. To consider: set sensor mode?
    coll.addChildCollision(new PhysBoxCollision(2e-3f, 2e-3f, 2e-3f), TMatrix::IDENT); // 1mm rad
    pbcd.autoMask = false;
    pbcd.group = pbcd.mask = 0;
  }
  else if (fgroup)
  {
    pbcd.autoMask = false;
    pbcd.group = fgroup;
    pbcd.mask = fmask;
  }

  pbcd.momentOfInertia = res_body.momj;
  pbcd.allowFastInaccurateCollTm = !has_joints;
  pbcd.userPtr = userData;
  PhysMat::MatID matId = PhysMat::getMaterialId(res_body.materialName);
  if (IsPhysMatID_Valid(matId))
    pbcd.materialId = PhysMat::getMaterial(matId).physBodyMaterial;
  pbcd.addToWorld = add_to_world;

  TMatrix physBodyTm = tm * res_body.tm; // Note: `tm` is already pre-scaled
  unscale_tm(physBodyTm);

  body.reset(new PhysBody(world, res_body.mass, &coll, physBodyTm, pbcd));

  coll.clear();
}


void PhysSystemInstance::Body::updateTms(const PhysicsResource::Body &res_body, const TMatrix &wtm)
{
  for (int i = 0; i < tmHelpers.size(); ++i)
    tmHelpers[i].wtm = wtm * res_body.tmHelpers[i].tm;
}
