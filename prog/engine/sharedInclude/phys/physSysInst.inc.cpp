#include <phys/dag_physSysInst.h>
#include <phys/dag_physics.h>
#include <debug/dag_log.h>
#include <scene/dag_physMat.h>
#include <osApiWrappers/dag_localConv.h>
#include <debug/dag_debug.h>
#include <phys/dag_physDecl.h>
#include <vecmath/dag_vecMath.h>


PhysSystemInstance::PhysSystemInstance(PhysicsResource *res, PhysWorld *world, const TMatrix *tm, void *userData, uint16_t fgroup,
  uint16_t fmask) :
  resource(res), bodies(midmem), joints(midmem)
{
  G_ASSERT(res);
  G_ASSERT(world);

  if (tm)
  {
    scaleTm.setcol(0, Point3(length(tm->getcol(0)), 0.f, 0.f));
    scaleTm.setcol(1, Point3(0.f, length(tm->getcol(1)), 0.f));
    scaleTm.setcol(2, Point3(0.f, 0.f, length(tm->getcol(2))));
    scaleTm.setcol(3, ZERO<Point3>());
  }
  else
    scaleTm = TMatrix::IDENT;

  bodies.reserve(resource->bodies.size());
  int jcnt = resource->rdBallJoints.size() + resource->rdHingeJoints.size() + resource->revoluteJoints.size() +
             resource->sphericalJoints.size();
  for (auto &rb : resource->bodies)
    bodies.emplace_back(rb, world, tm, userData, fgroup, fmask, jcnt > 0);

  joints.reserve(jcnt);

  for (int i = 0; i < resource->rdBallJoints.size(); ++i)
  {
    PhysicsResource::RdBallJoint &jnt = resource->rdBallJoints[i];

    PhysJoint *j = world->createRagdollBallJoint(bodies[jnt.body1].body.get(), bodies[jnt.body2].body.get(), jnt.tm, jnt.minLimit,
      jnt.maxLimit, jnt.damping, jnt.twistDamping, jnt.stiffness);

    if (j)
      joints.push_back(j);
    else
      logerr("can't create RagBall Joint");
  }

  for (int i = 0; i < resource->rdHingeJoints.size(); ++i)
  {
    PhysicsResource::RdHingeJoint &jnt = resource->rdHingeJoints[i];

    PhysJoint *j = world->createRagdollHingeJoint(bodies[jnt.body1].body.get(), bodies[jnt.body2].body.get(), jnt.pos, jnt.axis,
      jnt.midAxis, jnt.xAxis, jnt.angleLimit, jnt.damping, jnt.stiffness);

    if (j)
      joints.push_back(j);
    else
      logerr("can't create Hinge Joint");
  }

  for (int i = 0; i < resource->revoluteJoints.size(); ++i)
  {
    PhysicsResource::RevoluteJoint &jnt = resource->revoluteJoints[i];

    PhysJoint *j = world->createRevoluteJoint(bodies[jnt.body1].body.get(), bodies[jnt.body2].body.get(), jnt.pos, jnt.axis,
      jnt.minAngle, jnt.maxAngle, jnt.minRestitution, jnt.maxRestitution, jnt.spring, jnt.projType, jnt.projAngle, jnt.projDistance,
      jnt.damping, jnt.flags);

    if (j)
      joints.push_back(j);
    else
      logerr("can't create Revolution Joint");
  }

  for (int i = 0; i < resource->sphericalJoints.size(); ++i)
  {
    PhysicsResource::SphericalJoint &jnt = resource->sphericalJoints[i];

    PhysJoint *j = world->createSphericalJoint(bodies[jnt.body1].body.get(), bodies[jnt.body2].body.get(), jnt.pos, jnt.dir, jnt.axis,
      jnt.minAngle, jnt.maxAngle, jnt.minRestitution, jnt.maxRestitution, jnt.swingValue, jnt.swingRestitution, jnt.spring,
      jnt.damping, jnt.swingSpring, jnt.swingDamping, jnt.twistSpring, jnt.twistDamping, jnt.projType, jnt.projDistance, jnt.flags);

    if (j)
      joints.push_back(j);
    else
      logerr("can't create Spherical Joint");
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

template <typename T>
static inline bool filter_out_joints(const T &res_joints, Tab<PhysJoint *> &joints, int body_idx1, int body_idx2, int &j)
{
  for (int i = 0; i < res_joints.size(); ++i, ++j)
    if (match_joint(res_joints[i], body_idx1, body_idx2))
    {
      del_it(joints[j]);
      return true;
    }
  return false;
}

int PhysSystemInstance::findBodyIdByName(const char *name) const
{
  auto &resbodies = resource->getBodies();
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

void PhysSystemInstance::driveBodiesToAnimcharPose(const dag::Span<mat44f *> &skeletonWtms)
{
  auto &res_joints = resource->rdBallJoints;
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
    TMatrix &helper1Tm = resource->bodies[res_joints[i].body1].tmHelpers[0].tm;
    TMatrix &helper2Tm = resource->bodies[res_joints[i].body2].tmHelpers[0].tm;

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


bool PhysSystemInstance::removeJoint(const char *body1, const char *body2)
{
  int bodyIdx1 = findBodyIdByName(body1);
  int bodyIdx2 = findBodyIdByName(body2);

  if (bodyIdx1 < 0 || bodyIdx2 < 0)
    return false;

  int j = 0;
  if (filter_out_joints(resource->rdBallJoints, joints, bodyIdx1, bodyIdx2, j))
    return true;
  if (filter_out_joints(resource->rdHingeJoints, joints, bodyIdx1, bodyIdx2, j))
    return true;
  if (filter_out_joints(resource->revoluteJoints, joints, bodyIdx1, bodyIdx2, j))
    return true;
  if (filter_out_joints(resource->sphericalJoints, joints, bodyIdx1, bodyIdx2, j))
    return true;

  return false;
}

void PhysSystemInstance::removeJoint(const char *name)
{
  int j = 0;

  for (int i = 0; i < resource->rdBallJoints.size(); ++i, ++j)
  {
    PhysicsResource::RdBallJoint &jnt = resource->rdBallJoints[i];
    if (dd_stricmp(jnt.name, name) == 0)
    {
      del_it(joints[j]);
      return;
    }
  }

  for (int i = 0; i < resource->rdHingeJoints.size(); ++i, ++j)
  {
    PhysicsResource::RdHingeJoint &jnt = resource->rdHingeJoints[i];
    if (dd_stricmp(jnt.name, name) == 0)
    {
      del_it(joints[j]);
      return;
    }
  }

  for (int i = 0; i < resource->revoluteJoints.size(); ++i, ++j)
  {
    PhysicsResource::RevoluteJoint &jnt = resource->revoluteJoints[i];
    if (dd_stricmp(jnt.name, name) == 0)
    {
      del_it(joints[j]);
      return;
    }
  }

  for (int i = 0; i < resource->sphericalJoints.size(); ++i, ++j)
  {
    PhysicsResource::SphericalJoint &jnt = resource->sphericalJoints[i];
    if (dd_stricmp(jnt.name, name) == 0)
    {
      del_it(joints[j]);
      return;
    }
  }
}


void PhysSystemInstance::resetTm(const TMatrix &tm)
{
  scaleTm.setcol(0, Point3(length(tm.getcol(0)), 0.f, 0.f));
  scaleTm.setcol(1, Point3(0.f, length(tm.getcol(1)), 0.f));
  scaleTm.setcol(2, Point3(0.f, 0.f, length(tm.getcol(2))));
  for (int i = 0; i < bodies.size(); ++i)
  {
    TMatrix unscaledTransform = tm * resource->bodies[i].tm;
    unscaledTransform.setcol(0, normalize(unscaledTransform.getcol(0)));
    unscaledTransform.setcol(1, normalize(unscaledTransform.getcol(1)));
    unscaledTransform.setcol(2, normalize(unscaledTransform.getcol(2)));
    bodies[i].body->setTm(unscaledTransform);
  }

  updateTms();
}


TMatrix *PhysSystemInstance::getTmHelper(const char *name)
{
  for (int i = 0; i < bodies.size(); ++i)
  {
    int j = resource->bodies[i].findTmHelper(name);
    if (j < 0)
      continue;

    return &bodies[i].tmHelpers[j].wtm;
  }

  return NULL;
}


PhysBody *PhysSystemInstance::getBody(const char *name) const
{
  for (int i = 0; i < bodies.size(); ++i)
    if (dd_stricmp(resource->bodies[i].name, name) == 0)
      return bodies[i].body.get();

  return NULL;
}

void PhysSystemInstance::updateTms()
{
  for (int i = 0; i < bodies.size(); ++i)
    bodies[i].updateTms(resource->bodies[i], scaleTm);
}


bool PhysSystemInstance::setBodyTmByTmHelper(const char *name, const TMatrix &wtm)
{
  for (int i = 0; i < bodies.size(); ++i)
  {
    int id = resource->bodies[i].findTmHelper(name);
    if (id < 0)
      continue;

    TMatrix tm = wtm * inverse(resource->bodies[i].tmHelpers[id].tm);
    bodies[i].body->setTm(tm);

    return true;
  }

  return false;
}


bool PhysSystemInstance::setBodyTmAndVelByTmHelper(const char *name, const TMatrix &wtm_prev, const TMatrix &wtm, real dt)
{
  for (int i = 0; i < bodies.size(); ++i)
  {
    int id = resource->bodies[i].findTmHelper(name);
    if (id < 0)
      continue;

    TMatrix itm = inverse(resource->bodies[i].tmHelpers[id].tm);

    TMatrix tm_prev = wtm_prev * itm;
    TMatrix tm = wtm * itm;

    bodies[i].body->setTm(tm_prev);
    bodies[i].body->setTmWithDynamics(tm, dt);

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


PhysSystemInstance::Body::Body(PhysicsResource::Body &res_body, PhysWorld *world, const TMatrix *tm, void *userData, uint16_t fgroup,
  uint16_t fmask, bool has_joints)
{
  tmHelpers.resize(res_body.tmHelpers.size());
  for (int i = 0; i < tmHelpers.size(); ++i)
    tmHelpers[i].wtm = res_body.tm * res_body.tmHelpers[i].tm;

  // create collision primitives
  PhysCompoundCollision coll;
  int cmat_id = -1;

  // sphere collisions
  for (int i = 0; i < res_body.sphColl.size(); ++i)
  {
    PhysicsResource::SphColl &c = res_body.sphColl[i];

    TMatrix tm;
    tm.identity();
    tm.setcol(3, c.center);

    cmat_id = c.materialName.length() ? (int)PhysMat::getMaterialId(c.materialName) : -1;
    if (IsPhysMatID_Valid(cmat_id))
      cmat_id = PhysMat::getMaterial(cmat_id).physBodyMaterial;
    else
      cmat_id = -1;

    coll.addChildCollision(new PhysSphereCollision(c.radius), tm);
  }

  // box collisions
  for (int i = 0; i < res_body.boxColl.size(); ++i)
  {
    PhysicsResource::BoxColl &c = res_body.boxColl[i];

    cmat_id = c.materialName.length() ? (int)PhysMat::getMaterialId(c.materialName) : -1;
    if (IsPhysMatID_Valid(cmat_id))
    {
      cmat_id = PhysMat::getMaterial(cmat_id).physBodyMaterial;
    }
    else
      cmat_id = -1;

    PhysCollision::normalizeBox(c.size, c.tm);
    coll.addChildCollision(new PhysBoxCollision(c.size.x, c.size.y, c.size.z), c.tm);
  }

  // capsule collisions
  for (int i = 0; i < res_body.capColl.size(); ++i)
  {
    PhysicsResource::CapColl &c = res_body.capColl[i];

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

    cmat_id = c.materialName.length() ? (int)PhysMat::getMaterialId(c.materialName) : -1;
    if (IsPhysMatID_Valid(cmat_id))
      cmat_id = PhysMat::getMaterial(cmat_id).physBodyMaterial;
    else
      cmat_id = -1;

    auto *cap = new PhysCapsuleCollision(c.radius, ext * 2, PhysCollision::detectDirAxisIndex(tm));
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
  pbcd.addToWorld = tm != nullptr;

  TMatrix physBodyTm = tm ? ((*tm) * res_body.tm) : res_body.tm;
  if (tm != nullptr)
  {
    physBodyTm.setcol(0, normalize(physBodyTm.getcol(0)));
    physBodyTm.setcol(1, normalize(physBodyTm.getcol(1)));
    physBodyTm.setcol(2, normalize(physBodyTm.getcol(2)));
  }

  body.reset(new PhysBody(world, res_body.mass, &coll, physBodyTm, pbcd));

  coll.clear();
}


void PhysSystemInstance::Body::updateTms(PhysicsResource::Body &res_body, const TMatrix &scale_tm)
{
  TMatrix wtm;
  body->getTm(wtm);

  for (int i = 0; i < tmHelpers.size(); ++i)
    tmHelpers[i].wtm = wtm * scale_tm * res_body.tmHelpers[i].tm;
}
