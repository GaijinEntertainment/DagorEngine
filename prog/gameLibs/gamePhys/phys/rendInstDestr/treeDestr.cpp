// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "treeDestr.h"

#include <math/dag_mathUtils.h>
#include <math/dag_TMatrix.h>
#include <vecmath/dag_vecMath.h>
#include <phys/dag_physDecl.h>
#include <phys/dag_physics.h>
#include <util/dag_convar.h>

#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/phys/dynamicPhysModel.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstDebris.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/rendInstProps.h>
#include <rendInst/treeDestr.h>

#include "../../../rendInst/riGen/riGenData.h"
#include "riDestrInternal.h"

#if RI_DESTR_DEBUG_ENABLED
CONSOLE_FLOAT_VAL("ridestr", groundOffsetMulOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", groundDampingOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", groundJointDampingOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", groundJointMoveDistanceOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", groundJointAngleLimitWholeOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", groundJointAngleLimitBottomOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", treeTopTrunkDampingLinOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", treeTopTrunkDampingAngOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopyDampingLinOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopyDampingAngOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopySpringDampingAngOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopyJointDampingOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopyStiffnessMultOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopyJointAngLimitOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", linearLimitOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", angleLimitOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopyRadiusMulOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopyImpulseMulOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopyUnderhangMulOverride, -1.0f);

namespace rendinstdestr
{
const int *link_tree_destr_imgui = &pull_tree_destr_imgui;
}
#endif

static bool is_branch_destruction_supported = true; // ULQ doesn't support it
static rendinstdestr::on_tree_destr_created_callback on_tree_destr_created_cb = NULL;
static ri_tree_sound_cb g_tree_sound_cb = nullptr;
static CollisionObject tree_collision;

namespace rendinstdestr
{
bool useDebugTreeInstData;
rendinst::TreeInstDebugData debugTreeInstData;
} // namespace rendinstdestr

void rendinstdestr::init_tree_destr(ri_tree_sound_cb tree_sound_cb, bool enable_branch_destruction)
{
  debugTreeInstData.branchDestrFromDamage = get_tree_destr().branchDestrFromDamage;
  debugTreeInstData.branchDestrOther = get_tree_destr().branchDestrOther;
  is_branch_destruction_supported = enable_branch_destruction;
  g_tree_sound_cb = tree_sound_cb;
}

CollisionObject &rendinstdestr::get_tree_collision() { return tree_collision; }

void rendinstdestr::clear_tree_collision()
{
  if (tree_collision)
    dacoll::destroy_dynamic_collision(tree_collision);
  tree_collision.clear_ptrs();
}

static CollisionObject nondestr_bush_collision;

CollisionObject &rendinstdestr::get_nondestr_bush_collision() { return nondestr_bush_collision; }

void rendinstdestr::clear_nondestr_bush_collision()
{
  if (nondestr_bush_collision)
    dacoll::destroy_dynamic_collision(nondestr_bush_collision);
  nondestr_bush_collision.clear_ptrs();
}

void rendinstdestr::set_on_tree_destr_created_cb(on_tree_destr_created_callback cb) { on_tree_destr_created_cb = cb; }

namespace rendinstdestr
{

static rendinst::RendInstDesc create_tree_rend_inst_destr_old(const rendinst::RendInstDesc &desc, bool add_restorable,
  const Point3 &impactPos, const Point3 &_impulse, bool create_phys, bool constrained_phys, float wanted_omega, float at_time,
  const rendinst::CollisionInfo *coll_info, bool create_destr, bool from_damage)
{
  G_UNUSED(impactPos);
  Point3 impulse = _impulse;
  if (check_nan(_impulse) || lengthSq(_impulse) > sqr(MAX_RI_DESTROY_IMPULSE))
  {
    logerr("Bad impulse %@ in create_tree_rend_inst_destr", _impulse);
    impulse = Point3();
  }

  const rendinst::RiDestrData riDestrData = rendinst::gather_ri_destr_data(desc, from_damage);
  if (!riDestrData.branchDestr)
    return desc;

  rendinst::RendInstBufferData buffer;
  if (!rendinst::fillRendInstBufferData(desc, buffer))
    return desc;

  // Rendinst system can invalidate static shadows for very large cells in `rendinst::doRIGenExternalControl` and
  // `rendinstdestr::destroyRendinst`. But it is not needed if we have special callback with invalidation for only
  // instance bbox
  if (destrSettings.hasStaticShadowsInvalidationCallback)
    rendinst::render::avoidStaticShadowRecalc = true;

  Tab<rendinst::DestroyedRi *> destroyedRis;
  if (create_destr)
    destroyedRis = rendinst::doRIGenExternalControl(desc, buffer, 1);
  rendinst::DestroyedRi *ri = destroyedRis.empty() ? nullptr : destroyedRis.back();
  if (ri && on_tree_destr_created_cb)
    on_tree_destr_created_cb(desc, ri->riHandle);
  Point2 impulseDir = normalizeDef(Point2::xz(impulse), {1.f, 0.f});
  rendinst::RendInstDesc offsetedDesc = desc;
  rendinst::DestrOptionFlags flags =
    rendinst::DestrOptionFlag::AddDestroyedRi | rendinst::DestrOptionFlag::ForceDestroy | rendinst::DestrOptionFlag::UseFullBbox;
  if (create_phys)
    offsetedDesc = rendinstdestr::destroyRendinst(desc, add_restorable, ZERO<Point3>(), ZERO<Point3>(), at_time, coll_info,
      create_destr, nullptr, -1, 1.0f, nullptr, flags);
  else if (create_destr && !desc.isRiExtra())
    rendinst::delRIGen(desc, false);

  if (destrSettings.hasStaticShadowsInvalidationCallback)
    rendinst::render::avoidStaticShadowRecalc = false;

  if (!ri)
    return offsetedDesc;

  const float treeHeightSpringThreshold = 3.f;
  const rendinstdestr::TreeDestr &treeDestr = rendinstdestr::get_tree_destr();

  const RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);
  const rendinst::props::RendinstProperties *riProp = nullptr;
  if (!desc.isRiExtra() && (uint32_t)desc.pool < rgl->rtData->riProperties.size())
    riProp = &rgl->rtData->riProperties[desc.pool];

  mat43f m43 = rendinst::getRIGenExtra43(ri->riHandle);
  mat44f m44;
  v_mat43_transpose_to_mat44(m44, m43);
  vec3f scaleSq = v_mat44_scale43_sq(m44);
  if (v_test_xyz_nan(scaleSq) || v_test_xyz_nan(m44.col3) || !v_test_xyz_abs_lt(m44.col3, v_splats(1e5f)) ||
      v_check_xyz_any_true(v_is_unsafe_divisor(scaleSq)))
  {
    logerr("rendinstdestr: failed to create %s[%d] phys body with bad tm " FMT_TM, desc.isRiExtra() ? "riExtra" : "riGen", desc.pool,
      VTMD(m44));
    return offsetedDesc;
  }
  TMatrix tmOriginal;
  v_mat_43cu_from_mat44(tmOriginal.array, m44);

  v_mat44_orthonormalize33(m44, m44);
  TMatrix tm;
  v_mat_43cu_from_mat44(tm.array, m44);
  BBox3 bbox = rendinst::getRIGenBBox(desc);
  Point3_vec4 scale;
  v_st(&scale.x, v_sqrt(scaleSq));

  BBox3 fullBb = rendinst::getRIGenFullBBox(desc);
  BBox3 canopyBox = bbox;
  if (riProp)
  {
    const bool hasCanopy = rendinst::getRIGenCanopyBBox(*riProp, fullBb, canopyBox);
    const float canopyCutHt = (canopyBox.width().y * 0.5f) * 0.95f; // take a height of 10% of its middle
    if (hasCanopy && canopyBox.lim[1].y > canopyCutHt)
    {
      canopyBox.lim[1].y -= canopyCutHt;
      canopyBox.lim[0].y += canopyCutHt;
    }
    else
    {
      canopyBox.lim[1].y = bbox.lim[1].y;
      canopyBox.lim[0].y = bbox.lim[1].y - bbox.width().y * 0.1f;
    }
  }
  canopyBox.lim[0] *= scale.y;
  canopyBox.lim[1] *= scale.y;
  bbox.lim[0] *= scale.y;
  bbox.lim[1] *= scale.y;

  const float fullTreeHt = canopyBox.lim[1].y;
  const float height = fullTreeHt * riDestrData.collisionHeightScale;
  if (height <= 0)
  {
    logerr("%s[%d] gives bbox=%@ and invalid height=%.1f (scale=%@ collisionHeightScale=%g)", desc.isRiExtra() ? "riExtra" : "riGen",
      desc.pool, bbox, height, scale, riDestrData.collisionHeightScale);
    return offsetedDesc;
  }
  const float bushRadiusMult = (riDestrData.bushBehaviour ? treeDestr.bushDestrRadiusMult : 1.0f);
  const Point3 boxSize = bbox.width();
  float radius = clamp_min(min(boxSize.x, boxSize.z) * 0.5f * bushRadiusMult, 0.1f);
  if (riProp && riProp->trunkRadius > 0.f)
    radius = riProp->trunkRadius;

  const float density = riDestrData.bushBehaviour ? treeDestr.bushDensity : treeDestr.treeDensity;
  const float mass = PI * sqr(radius) * height * density;
  const Point3 momentOfInertia = Point3(mass * (3.f * sqr(radius) + sqr(height)) / 12.f, mass * sqr(radius) * 0.5f,
    mass * (3.f * sqr(radius) + sqr(height)) / 12.f);
  const bool allowSpring = height < treeHeightSpringThreshold && desc.layer == 1;
  gamephys::DynamicPhysModel::PhysType physModelType =
    create_phys ? gamephys::DynamicPhysModel::E_PHYS_OBJECT
                : (allowSpring ? gamephys::DynamicPhysModel::E_ANGULAR_SPRING : gamephys::DynamicPhysModel::E_FORCED_POS_BY_COLLISION);

  RendInstPhys &phys =
    riPhys.emplace_back(RendInstPhysType::TREE, desc, tm, mass, momentOfInertia, scale, Point3(0.f, height * 0.5f, 0.f), physModelType,
      tmOriginal, rendinstdestr::get_destr_settings().rendInstMaxLifeTime, treeDestr.maxLifeDistance, ri);

  {
    rendinst::RiExtraPerInstanceGpuItem originalPosRenderData = {bitwise_cast<uint32_t>(tmOriginal.m[3][0]),
      bitwise_cast<uint32_t>(tmOriginal.m[3][1]), bitwise_cast<uint32_t>(tmOriginal.m[3][2]), bitwise_cast<uint32_t>(1.0f)};
    rendinst::setRiExtraPerInstanceRenderAdditionalDataDeferred(ri->riHandle, rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS,
      rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT, &originalPosRenderData, 1);
  }

  if (!allowSpring && rendinstdestr::get_destr_settings().hasSound)
    phys.treeSound.init(g_tree_sound_cb, tm, bbox.lim[1].y * scale.y, riDestrData.bushBehaviour);

  phys.descForDestr = offsetedDesc;

  if (phys.isPhysObject())
  {
    float canopyStartHt = canopyBox.lim[0].y;
    float trunkStartHt = (constrained_phys ? height * 0.035f : 0.f) + radius;
    float trunkHt = max(canopyStartHt * 0.9f - trunkStartHt, 2.f * radius);
    float trunkHalfHt = trunkHt * 0.5f;

    TMatrix fallDirTm;
    fallDirTm.setcol(0, Point3::x0y(impulseDir));
    fallDirTm.setcol(2, fallDirTm.getcol(0) % tm.getcol(1));
    fallDirTm.setcol(1, fallDirTm.getcol(2) % fallDirTm.getcol(0));
    fallDirTm.setcol(3, Point3(0, 0, 0));

    TMatrix invObjDir = inverse(tm);
    invObjDir.col[3].zero();
    TMatrix centerOfMassTm = fallDirTm * invObjDir;
    centerOfMassTm.setcol(3, Point3(0.f, trunkHalfHt + trunkStartHt, 0.f));
    phys.centerOfMassTm = centerOfMassTm;
    PhysWorld *physWorld = dacoll::get_phys_world();

    // trunk
    {
      // shape
      PhysBoxCollision trunkShape(radius * sqrt(2.f), trunkHt, radius * sqrt(2.f), false);

      PhysBodyCreationData pbcd;
      pbcd.momentOfInertia = momentOfInertia;
      if (constrained_phys)
      {
        phys.collisionDistLimit = 1.0f;
        pbcd.autoMask = false, pbcd.group = dacoll::EPL_DEFAULT, pbcd.mask = dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC; //-V1048
      }

      // body
      phys.physBody = new PhysBody(physWorld, mass, &trunkShape, tm * centerOfMassTm, pbcd);
    }

    // canopy
    float sphRad = (height - canopyStartHt) * 0.5f;
    float impulseDirMult = constrained_phys ? 0.1f * sphRad : 0.f;
    TMatrix sphTm = phys.centerOfMassTm;
    sphTm.setcol(3, Point3(impulseDir.x * impulseDirMult, canopyBox.center().y, impulseDir.y * impulseDirMult));
    const float canopyMass = canopyBox.volume() * treeDestr.canopyDensity;
    Point3 canopyInertia = treeDestr.canopyInertia * 0.4f * canopyMass;
    {
      // shape
      Point3 canopyBSize = canopyBox.width();
      PhysBoxCollision boxCanopyCollision(canopyBSize.x, canopyBSize.y, canopyBSize.z);

      PhysBodyCreationData pbcd;
      pbcd.momentOfInertia = canopyInertia;
      pbcd.rollingFriction = treeDestr.canopyRollingFriction;
      pbcd.restitution = treeDestr.canopyRestitution;
      pbcd.linearDamping = treeDestr.canopyLinearDamping;
      pbcd.angularDamping = treeDestr.canopyAngularDamping;
      if (constrained_phys)
        pbcd.autoMask = false, pbcd.group = dacoll::EPL_DEFAULT, pbcd.mask = dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC; //-V1048

      // canopy body
      phys.additionalBody = new PhysBody(physWorld, canopyMass, &boxCanopyCollision, tm * sphTm, pbcd);
    }

    // joint
    {
      TMatrix halfSphTm(1);
      halfSphTm.setcol(3, sphTm.getcol(3) * 0.5f);
      TMatrix canopyCoMTm = sphTm;
      canopyCoMTm.setcol(3, sphTm.getcol(3) * 0.5f);
      PhysJoint *joint = physWorld->create6DofSpringJoint(phys.physBody, phys.additionalBody, halfSphTm * inverse(phys.centerOfMassTm),
        inverse(canopyCoMTm));
      phys.joints.push_back() = joint;
      Phys6DofSpringJoint *dofSpring = Phys6DofSpringJoint::cast(joint);
      if (dofSpring)
      {
        const float cmLen = length(sphTm.getcol(3));
        const float stiffness = (mass + canopyMass * cmLen * 3.f) * treeDestr.canopyStiffnessMult;
        const float angularLimit = treeDestr.canopyAngularLimit;
        dofSpring->setLimit(0, Point2(0.f, 0.f));
        dofSpring->setLimit(1, Point2(0.f, 0.f));
        dofSpring->setLimit(2, Point2(0.f, 0.f));
        dofSpring->setSpring(3, true, stiffness, 0.005f, Point2(-angularLimit, angularLimit));
        dofSpring->setLimit(4, Point2(0.f, 0.f));
        dofSpring->setSpring(5, true, stiffness, 0.005f, Point2(-angularLimit, angularLimit));
        for (int i = 0; i < 6; ++i)
        {
#if defined(USE_BULLET_PHYSICS)
          dofSpring->setParam(BT_CONSTRAINT_STOP_ERP, 0.2, i);
#endif
          dofSpring->setAxisDamping(i, 0.5f);
        }
        dofSpring->setEquilibriumPoint();
      }
      phys.lastValidTm = tm;
    }

    // Impulse is calculated in such way it doesn't violate constraint
    // spdbot = omega * arm + vel
    // vel = omega * arm
    // omega = vel / arm
    // omega = imp * arm2 / inertia
    // vel = imp / mass
    // arm2 = omega * inertia / imp
    // arm2 = (vel / arm) * inertia / imp
    // arm2 = ((imp / mass) / arm) * inertia / imp
    // arm2 = (imp * inertia) / (mass * arm * imp)
    // arm2 = inertia / (mass * arm)

    // imp = omega * inertia / arm2
    float omega = clamp(wanted_omega, 0.3f, 1.f);
    float arm = safediv(momentOfInertia.x, mass * (trunkHalfHt + trunkStartHt));
    float vel = omega * arm;
    G_UNUSED(vel);
    float impulseVal = safediv(omega * momentOfInertia.x, arm);
    G_ASSERTF(!check_nan(impulseVal) && impulseVal < MAX_RI_DESTROY_IMPULSE, "impulseVal %f", impulseVal);

    if (constrained_phys)
    {
      PhysJoint *joint = physWorld->create6DofJoint(phys.physBody, NULL, inverse(phys.centerOfMassTm), TMatrix::IDENT);
      phys.joints.push_back() = joint;
      Phys6DofJoint *dofJoint = Phys6DofJoint::cast(joint);
      if (dofJoint)
      {
        dofJoint->setLimit(0, Point2(0.f, 0.f));
        dofJoint->setLimit(1, treeDestr.constraintLimitY);
        dofJoint->setLimit(2, Point2(0.f, 0.f));
#if defined(USE_BULLET_PHYSICS)
        for (int i = 0; i < 6; ++i)
          dofJoint->setParam(BT_CONSTRAINT_STOP_ERP, 0.2, i);
#endif
      }

      phys.physBody->addImpulse(tm * (phys.centerOfMassTm.getcol(3) + Point3(0.f, arm, 0.f)), Point3::x0y(impulseDir) * impulseVal);

      // omega = vel / arm
      // vel = omega * arm
      // imp = vel * mass
      // imp = omega * arm * mass
      float additionalImpulseVal = omega * (height - sphRad) * canopyMass;
      G_ASSERTF(!check_nan(additionalImpulseVal) && additionalImpulseVal < MAX_RI_DESTROY_IMPULSE, "additionalImpulseVal %f",
        additionalImpulseVal);
      phys.additionalBody->addImpulse((tm * sphTm).getcol(3), additionalImpulseVal * Point3::x0y(impulseDir));
    }
    phys.lastValidTm = tm;
    phys.treeInstData.creationTimeAt = at_time;
    const char *destrFxTemplate = coll_info ? rendinst::getRIGenDestrPropsFxTemplate(coll_info->desc) : "";
    if (!constrained_phys && coll_info && (coll_info->destrFxId >= 0 || *destrFxTemplate) && ri_effect_cb)
      ri_effect_cb(coll_info->destrFxId, TMatrix::IDENT, tm, coll_info->desc.pool, false, nullptr, destrFxTemplate);

    const bool enableBranchDestr =
      is_branch_destruction_supported && riDestrData.branchDestr->enableBranchDestruction && at_time > 0.0f;
    if (enableBranchDestr)
    {
      phys.treeInstData.branchDestr = *riDestrData.branchDestr;
      phys.treeInstData.impactXZ = impulseDir * vel;
      if (!rendinst::fillTreeInstData(desc, from_damage, phys.treeInstData))
        phys.treeInstData.branchDestr.enableBranchDestruction = false;
      else
        debugTreeInstData.last_object_was_from_damage = from_damage;
    }
  }
  else
  {
    float hh = height * 0.5f;
    TMatrix cotm = TMatrix::IDENT;
    cotm.m[3][1] = hh;
    phys.riColObj = dacoll::add_dynamic_cylinder_collision(cotm, radius, height, /*uptr*/ nullptr, /*add_to_world*/ false);
    dacoll::obj_apply_impulse(phys.riColObj, impulse * mass, Point3(0.f, hh, 0.f));
  }

  if (create_destr && rendinstdestr::get_destr_settings().createDestr && !create_phys && ri_effect_cb)
  {
    if (riDestrData.fxType != -1 || *riDestrData.fxTemplate != 0)
    {
      TMatrix fxTm = tm;
      fxTm *= riDestrData.fxScale;
      fxTm.setcol(3, tm.getcol(3));
      ri_effect_cb(riDestrData.fxType, TMatrix::IDENT, fxTm, desc.pool, false, nullptr, riDestrData.fxTemplate);
    }
  }

  return offsetedDesc;
}

static rendinst::RendInstDesc create_tree_rend_inst_destr_new(const rendinst::RendInstDesc &desc, bool add_restorable,
  const Point3 &impactPos, const Point3 &_impulse, bool create_phys, bool constrained_phys, float wanted_omega, float at_time,
  const rendinst::CollisionInfo *coll_info, bool create_destr, bool from_damage)
{
  debugTreeInstData.lastHitPos = impactPos;
  debugTreeInstData.impulsePositions.clear();

  Point3 impulse = _impulse;
  float impulseStrength = length(_impulse);
  if (check_nan(_impulse) || impulseStrength > MAX_RI_DESTROY_IMPULSE)
  {
    logerr("Bad impulse %@ in create_tree_rend_inst_destr", _impulse);
    impulseStrength = 0.0f;
    impulse = Point3();
  }

  const rendinstdestr::TreeDestr &treeDestr = rendinstdestr::get_tree_destr();
  const rendinst::RiDestrData riDestrData = rendinst::gather_ri_destr_data(desc, from_damage);
  if (!riDestrData.branchDestr)
    return desc;

  rendinst::RendInstBufferData buffer;
  if (!rendinst::fillRendInstBufferData(desc, buffer))
    return desc;

  static const float min_cut_height = 0.5f;

  const bool enableBranchDestr = is_branch_destruction_supported && riDestrData.branchDestr->enableBranchDestruction && at_time > 0.0f;
  bool cutTreeInHalf = enableBranchDestr && create_destr && from_damage && !riDestrData.bushBehaviour;
  if (from_damage)
  {
    // A big tank cannon does about 1000 damage, we want this to have a noticeable increase in rotation speed
    float damageEquivalent = impulseStrength / rendinstdestr::get_destr_settings().hitPointsToDestrImpulseMult;
    wanted_omega *= 1 + (damageEquivalent * 0.001f);

    float cutInHalfThreshold = riDestrData.branchDestr->cutInHalfDamageThreshold;
    if (useDebugTreeInstData)
      cutInHalfThreshold = debugTreeInstData.branchDestrFromDamage.cutInHalfDamageThreshold;
    if (cutInHalfThreshold < 0 || cutInHalfThreshold > damageEquivalent || currentCutTreeCount >= treeDestr.maxCutTreeCount)
      cutTreeInHalf = false;

    float cutHeight = impactPos.y - rendinst::getRIGenMatrix(desc).col[3].y;
    if (cutHeight < min_cut_height)
    {
      // this could happen if the collision geometry of the tree reaches below the model origin
      cutTreeInHalf = false;
    }
  }

  if (useDebugTreeInstData && debugTreeInstData.disableCut)
    cutTreeInHalf = false;


  // Rendinst system can invalidate static shadows for very large cells in `rendinst::doRIGenExternalControl` and
  // `rendinstdestr::destroyRendinst`. But it is not needed if we have special callback with invalidation for only
  // instance bbox
  if (destrSettings.hasStaticShadowsInvalidationCallback)
    rendinst::render::avoidStaticShadowRecalc = true;

  int sliceCount = cutTreeInHalf ? 2 : 1;

  Tab<rendinst::DestroyedRi *> ris =
    create_destr ? rendinst::doRIGenExternalControl(desc, buffer, sliceCount) : Tab<rendinst::DestroyedRi *>();
  if (on_tree_destr_created_cb)
    for (const rendinst::DestroyedRi *ri : ris)
      on_tree_destr_created_cb(desc, ri->riHandle);
  Point2 impulseDir = normalizeDef(Point2::xz(impulse), {1.f, 0.f});
  rendinst::RendInstDesc offsetedDesc = desc;
  rendinst::DestrOptionFlags flags =
    rendinst::DestrOptionFlag::AddDestroyedRi | rendinst::DestrOptionFlag::ForceDestroy | rendinst::DestrOptionFlag::UseFullBbox;
  if (create_phys)
    offsetedDesc = rendinstdestr::destroyRendinst(desc, add_restorable, ZERO<Point3>(), ZERO<Point3>(), at_time, coll_info,
      create_destr, nullptr, 1, 1.0f, nullptr, flags);
  else if (create_destr && !desc.isRiExtra())
    rendinst::delRIGen(desc, false);

  if (destrSettings.hasStaticShadowsInvalidationCallback)
    rendinst::render::avoidStaticShadowRecalc = false;

  if (ris.size() == 0)
    return offsetedDesc;

  const float treeHeightSpringThreshold = 3.f;

  mat43f m43 = rendinst::getRIGenExtra43(ris[0]->riHandle);
  mat44f m44;
  v_mat43_transpose_to_mat44(m44, m43);
  vec3f scaleSq = v_mat44_scale43_sq(m44);
  if (v_test_xyz_nan(scaleSq) || v_test_xyz_nan(m44.col3) || !v_test_xyz_abs_lt(m44.col3, v_splats(1e5f)) ||
      v_check_xyz_any_true(v_is_unsafe_divisor(scaleSq)))
  {
    logerr("rendinstdestr: failed to create %s[%d] phys body with bad tm " FMT_TM, desc.isRiExtra() ? "riExtra" : "riGen", desc.pool,
      VTMD(m44));
    return offsetedDesc;
  }

  if (cutTreeInHalf)
    currentCutTreeCount++;

  TMatrix tmOriginal;
  v_mat_43cu_from_mat44(tmOriginal.array, m44);

  v_mat44_orthonormalize33(m44, m44);
  TMatrix tmNoScale;
  v_mat_43cu_from_mat44(tmNoScale.array, m44);
  BBox3 bbox = rendinst::getRIGenBBox(desc);
  Point3_vec4 scale;
  v_st(&scale.x, v_sqrt(scaleSq));

  BBox3 canopyBboxLocal;
  rendinst::getRIGenCanopyBBox(desc, TMatrix::IDENT, canopyBboxLocal);

  const RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);
  const rendinst::props::RendinstProperties *riProp = nullptr;
  if (!desc.isRiExtra() && (uint32_t)desc.pool < rgl->rtData->riProperties.size())
    riProp = &rgl->rtData->riProperties[desc.pool];

  Point3 bboxSize = bbox.width();
  float totalTreeHeight = bboxSize.y * scale.y * riDestrData.collisionHeightScale;
  const float bushRadiusMult = (riDestrData.bushBehaviour ? treeDestr.bushDestrRadiusMult : 1.0f);

  const Point3 boxSize = bbox.width();
  const float maxTrunkRadius = 0.1f;
  float radius = clamp_min(min(boxSize.x, boxSize.z) * 0.5f * bushRadiusMult, maxTrunkRadius);
  if (riProp && riProp->trunkRadius > 0.f)
    radius = riProp->trunkRadius;

  const float density = riDestrData.bushBehaviour ? treeDestr.bushDensity : treeDestr.treeDensity;

#if RI_DESTR_DEBUG_ENABLED
  const float groundOffsetMul = groundOffsetMulOverride < 0.0f ? treeDestr.groundOffsetMul : groundOffsetMulOverride;
  const float groundDamping = groundDampingOverride < 0.0f ? treeDestr.groundDamping : groundDampingOverride;
  const float groundJointDamping = groundJointDampingOverride < 0.0f ? treeDestr.groundJointDamping : groundJointDampingOverride;
  const float groundJointMoveDistance =
    groundJointMoveDistanceOverride < 0.0f ? treeDestr.constraintLimitY.y : groundJointMoveDistanceOverride;
  const float groundJointAngleLimitWhole =
    groundJointAngleLimitWholeOverride < 0.0f ? treeDestr.groundJointAngleLimitWhole : groundJointAngleLimitWholeOverride;
  const float groundJointAngleLimitBottom =
    groundJointAngleLimitBottomOverride < 0.0f ? treeDestr.groundJointAngleLimitBottom : groundJointAngleLimitBottomOverride;
  const float treeTopTrunkDampingLin =
    treeTopTrunkDampingLinOverride < 0.0f ? treeDestr.treeDampingLinear : treeTopTrunkDampingLinOverride;
  const float treeTopTrunkDampingAng =
    treeTopTrunkDampingAngOverride < 0.0f ? treeDestr.treeDampingAngular : treeTopTrunkDampingAngOverride;
  const float canopyJointAngLimit = canopyJointAngLimitOverride < 0.0f ? treeDestr.canopyAngularLimit : canopyJointAngLimitOverride;
  const float canopyDampingLin = canopyDampingLinOverride < 0.0f ? treeDestr.canopyLinearDamping : canopyDampingLinOverride;
  const float canopyDampingAng = canopyDampingAngOverride < 0.0f ? treeDestr.canopyAngularDamping : canopyDampingAngOverride;
  const float canopyStiffnessMult = canopyStiffnessMultOverride < 0.0f ? treeDestr.canopyStiffnessMult : canopyStiffnessMultOverride;
  const float canopySpringDampingAng = canopySpringDampingAngOverride < 0.0f ? 0.005f : canopySpringDampingAngOverride;
  const float canopyJointDamping = canopyJointDampingOverride < 0.0f ? treeDestr.canopyJointDamping : canopyJointDampingOverride;
  const float canopyRadiusMul = canopyRadiusMulOverride >= 0.0f ? canopyRadiusMulOverride : 0.7f;
  const float canopyUnderhangMul = canopyUnderhangMulOverride >= 0.0f ? canopyUnderhangMulOverride : 0.8f;
  const float canopyImpulseMul = canopyImpulseMulOverride >= 0.0f ? canopyImpulseMulOverride : 1.0f;
  const float collisionDistLimit = linearLimitOverride >= 0.0f ? linearLimitOverride : treeDestr.collisionDistLimit;
  const float collisionAngleLimitCos =
    angleLimitOverride >= 0.0f ? cosf(angleLimitOverride * DEG_TO_RAD) : cosf(treeDestr.collisionAngleLimit * DEG_TO_RAD);
#else
  const float &groundOffsetMul = treeDestr.groundOffsetMul;
  const float &groundDamping = treeDestr.groundDamping;
  const float &groundJointDamping = treeDestr.groundJointDamping;
  const float &groundJointAngleLimitWhole = treeDestr.groundJointAngleLimitWhole;
  const float &groundJointAngleLimitBottom = treeDestr.groundJointAngleLimitBottom;
  const float &groundJointMoveDistance = treeDestr.constraintLimitY.y;
  const float &treeTopTrunkDampingLin = treeDestr.treeDampingLinear;
  const float &treeTopTrunkDampingAng = treeDestr.treeDampingAngular;
  const float &canopyJointAngLimit = treeDestr.canopyAngularLimit;
  const float &canopyDampingLin = treeDestr.canopyLinearDamping;
  const float &canopyDampingAng = treeDestr.canopyAngularDamping;
  const float &canopyStiffnessMult = treeDestr.canopyStiffnessMult;
  const float canopySpringDampingAng = 0.005f;
  const float &canopyJointDamping = treeDestr.canopyJointDamping;
  const float canopyRadiusMul = 0.7f;
  const float canopyUnderhangMul = 0.8f;
  const float canopyImpulseMul = 1.0f;
  const float &collisionDistLimit = treeDestr.collisionDistLimit;
  const float collisionAngleLimitCos = cosf(treeDestr.collisionAngleLimit * DEG_TO_RAD);
#endif

  const bool allowSpring = totalTreeHeight < treeHeightSpringThreshold && desc.layer == 1;
  gamephys::DynamicPhysModel::PhysType physModelType =
    create_phys ? gamephys::DynamicPhysModel::E_PHYS_OBJECT
                : (allowSpring ? gamephys::DynamicPhysModel::E_ANGULAR_SPRING : gamephys::DynamicPhysModel::E_FORCED_POS_BY_COLLISION);

  PhysWorld *physWorld = dacoll::get_phys_world();

  for (int iTreeSlice = 0; iTreeSlice < ris.size(); iTreeSlice++)
  {
    rendinst::DestroyedRi *ri = ris[iTreeSlice];

    Point3 localImpactPos = inverse(tmOriginal) * impactPos;
    float cutHeight = 0.0f;

    rendinst::TreeInstData::CutType cutType = rendinst::TreeInstData::CutType::None;
    if (cutTreeInHalf)
    {
      cutHeight = impactPos.y - tmOriginal.col[3].y;
      cutType = iTreeSlice == 0 ? rendinst::TreeInstData::CutType::Top : rendinst::TreeInstData::CutType::Bottom;
      cutHeight = clamp(cutHeight, 0.0f, totalTreeHeight);
    }

    float height = totalTreeHeight;
    if (cutType == rendinst::TreeInstData::CutType::Top)
      height = totalTreeHeight - cutHeight;
    else if (cutType == rendinst::TreeInstData::CutType::Bottom)
      height = cutHeight;

    if (cutType == rendinst::TreeInstData::CutType::Top)
      rendinst::setRiExtraPerInstanceOptionalFlag(ri->riHandle, rendinst::RiExtraOptionalFlag::HIDE_MARKED_MATERIAL);

    {
      rendinst::RiExtraPerInstanceGpuItem originalPosRenderData = {bitwise_cast<uint32_t>(tmOriginal.m[3][0]),
        bitwise_cast<uint32_t>(tmOriginal.m[3][1]), bitwise_cast<uint32_t>(tmOriginal.m[3][2]), bitwise_cast<uint32_t>(1.0f)};
      rendinst::setRiExtraPerInstanceRenderAdditionalData(ri->riHandle, rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS,
        rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT, &originalPosRenderData, 1);
    }

    if (totalTreeHeight <= 0 || height <= 0)
    {
      logerr("%s[%d] gives bbox=%@ and invalid height=%.1f (scale=%@ collisionHeightScale=%g totalTreeHeight=%0.1f)",
        desc.isRiExtra() ? "riExtra" : "riGen", desc.pool, bbox, height, scale, riDestrData.collisionHeightScale, totalTreeHeight);
      return offsetedDesc;
    }

    const float mass = PI * sqr(radius) * height * density;
    const Point3 momentOfInertia = Point3(mass * (3.f * sqr(radius) + sqr(height)) / 12.f, mass * sqr(radius) * 0.5f,
      mass * (3.f * sqr(radius) + sqr(height)) / 12.f);

    // Impulse is calculated in such way it doesn't violate constraint
    // spdbot = omega * arm + vel
    // vel = omega * arm
    // omega = vel / arm
    // omega = imp * arm2 / inertia
    // vel = imp / mass
    // arm2 = omega * inertia / imp
    // arm2 = (vel / arm) * inertia / imp
    // arm2 = ((imp / mass) / arm) * inertia / imp
    // arm2 = (imp * inertia) / (mass * arm * imp)
    // arm2 = inertia / (mass * arm)

    float omega = clamp(wanted_omega, 0.3f, 1.f);
    float arm = safediv(momentOfInertia.x, mass * height * 0.5f);
    float vel = omega * arm;
    float impulseVal = safediv(omega * momentOfInertia.x, arm);
    G_ASSERTF(!check_nan(impulseVal) && impulseVal < MAX_RI_DESTROY_IMPULSE, "impulseVal %f", impulseVal);

    RendInstPhys &phys =
      riPhys.emplace_back(RendInstPhysType::TREE, desc, tmNoScale, mass, momentOfInertia, scale, Point3(0.f, height * 0.5f, 0.f),
        physModelType, tmOriginal, rendinstdestr::get_destr_settings().rendInstMaxLifeTime, treeDestr.maxLifeDistance, ri);

    phys.treeInstData.creationTimeAt = at_time;
    phys.treeInstData.localImpactPos = localImpactPos;
    phys.treeInstData.cutType = cutType;

    phys.descForDestr = offsetedDesc;

    if (cutType != rendinst::TreeInstData::CutType::Bottom)
    {
      if (!allowSpring && rendinstdestr::get_destr_settings().hasSound)
        phys.treeSound.init(g_tree_sound_cb, tmNoScale, bbox.lim[1].y * scale.y, riDestrData.bushBehaviour);
    }

    if (phys.isPhysObject())
    {
      Point3 canopySizeScaled = canopyBboxLocal.width() * scale.y;
      canopySizeScaled.x *= canopyRadiusMul;
      canopySizeScaled.z *= canopyRadiusMul;
      float canopyCenterHt = canopyBboxLocal.center().y * scale.y;

      TMatrix fallDirTm;
      fallDirTm.setcol(0, Point3::x0y(impulseDir));
      fallDirTm.setcol(2, fallDirTm.getcol(0) % tmNoScale.getcol(1));
      fallDirTm.setcol(1, fallDirTm.getcol(2) % fallDirTm.getcol(0));
      fallDirTm.setcol(3, tmNoScale.getcol(3));
      TMatrix canopyImpulseDirTm = inverse(tmNoScale) % fallDirTm;

      PhysBodyCreationData pbcd;
      pbcd.friction = treeDestr.treeFriction;
      pbcd.rollingFriction = 0.95f;
      pbcd.autoInertia = true;
      if (constrained_phys && cutType == rendinst::TreeInstData::CutType::Bottom)
      {
        pbcd.linearDamping = groundDamping;
        pbcd.angularDamping = groundDamping;
      }
      else
      {
        pbcd.linearDamping = treeTopTrunkDampingLin;
        pbcd.angularDamping = treeTopTrunkDampingAng;
      }

      if (constrained_phys && cutType == rendinst::TreeInstData::CutType::Bottom)
      { // Bottom half won't collide with tanks. The ground joint will simply lose angle limits at tank collision.
        pbcd.autoMask = false;
        pbcd.mask = dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC;
      }
      else
      { // Free moving or whole parts collide with tanks until distance or angle limits are reached.
        phys.collisionDistLimit = collisionDistLimit;
        phys.collisionAngleLimitCos = collisionAngleLimitCos;
      }

      float trunkShapeHeight;
      if (riDestrData.bushBehaviour)
      {
        canopySizeScaled *= 0.6f; // bush canopy is smaller
        PhysBoxCollision boxShape(canopySizeScaled.x, canopySizeScaled.y, canopySizeScaled.z);
        trunkShapeHeight = canopySizeScaled.y;

        TMatrix centerOfMassTm = TMatrix::IDENT;
        centerOfMassTm.setcol(3, Point3(0.f, canopyCenterHt, 0.f));
        phys.centerOfMassTm = centerOfMassTm;

        float canopyMass = canopySizeScaled.x * canopySizeScaled.y * canopySizeScaled.z * treeDestr.canopyDensity;
        phys.physBody = new PhysBody(physWorld, canopyMass, &boxShape, tmNoScale * centerOfMassTm, pbcd);
      }
      else
      {
        float trunkStartHt = totalTreeHeight * groundOffsetMul + radius;
        float trunkEndHt = totalTreeHeight;
        if (cutType == rendinst::TreeInstData::CutType::Top)
          trunkStartHt = cutHeight + radius * 0.5f;
        else if (cutType == rendinst::TreeInstData::CutType::Bottom)
          trunkEndHt = cutHeight;

        float canopyBottom = canopyCenterHt - canopySizeScaled.y * 0.5f;
        if (canopyBottom < trunkStartHt)
        { // make sure the canopy doesn't start below the trunk
          float canopyUnderhang = (trunkStartHt - canopyBottom);
          float newCanopyHeight = (canopySizeScaled.y - canopyUnderhang) * canopyUnderhangMul;
          float offset = (canopySizeScaled.y - newCanopyHeight) * 0.5f;
          canopySizeScaled.y = newCanopyHeight;
          canopyCenterHt += offset;
        }

        trunkShapeHeight = max(trunkEndHt - trunkStartHt, 2.f * radius);

        TMatrix centerOfMassTm = TMatrix::IDENT;
        centerOfMassTm.setcol(3, Point3(0.f, trunkShapeHeight * 0.5f + trunkStartHt, 0.f));
        phys.centerOfMassTm = centerOfMassTm;

        // trunk
        {
          // shape
          PhysCapsuleCollision capsuleShape(radius, trunkShapeHeight, 1 /*Y*/);
          phys.physBody = new PhysBody(physWorld, mass, &capsuleShape, tmNoScale * centerOfMassTm, pbcd);
        }
      }

      TMatrix sphTm = TMatrix::IDENT;
      float canopyMass = canopySizeScaled.x * canopySizeScaled.y * canopySizeScaled.z * treeDestr.canopyDensity;
      if (!riDestrData.bushBehaviour &&
          (cutType == rendinst::TreeInstData::CutType::Top || cutType == rendinst::TreeInstData::CutType::None) && canopyMass > 0.0f)
      {
        // canopy
        float impulseDirMult = constrained_phys ? 0.1f * canopySizeScaled.y : 0.f;
        sphTm.setcol(3, Point3(impulseDir.x * impulseDirMult, canopyCenterHt, impulseDir.y * impulseDirMult));
        {
          // shape
          float canopyRadius = max(canopySizeScaled.x, canopySizeScaled.z) * 0.5f;
          float colSize = clamp(canopyRadius, 0.05f, treeDestr.canopyMaxRaduis);
          PhysSphereCollision sphCanopyCollision(colSize);

          float canopyMaxHorizontalSize = min(treeDestr.canopyMaxRaduis * 2.0f, canopySizeScaled.y * 0.8f); // make sure box is
                                                                                                            // vertical
          PhysBoxCollision boxCanopyCollision(clamp(canopySizeScaled.x, 0.05f, canopyMaxHorizontalSize), canopySizeScaled.y,
            clamp(canopySizeScaled.z, 0.05f, canopyMaxHorizontalSize));

          Tab<Point3> pyramidPoints(tmpmem);
          float canopyWidthX = canopyBboxLocal.width().x * 0.5f * scale.y * canopyRadiusMul;
          float canopyWidthZ = canopyBboxLocal.width().z * 0.5f * scale.y * canopyRadiusMul;
          float canopyBottom = canopyCenterHt - canopySizeScaled.y * 0.5f;
          float canopyBottomElevated = canopyBottom + canopySizeScaled.y * 0.2f;
          Point3 bottomCenter = {canopyBboxLocal.center().x, canopyBottomElevated, canopyBboxLocal.center().z};
          pyramidPoints.push_back(bottomCenter + Point3(-canopyWidthX, 0.f, -canopyWidthZ));
          pyramidPoints.push_back(bottomCenter + Point3(-canopyWidthX, 0.f, canopyWidthZ));
          pyramidPoints.push_back(bottomCenter + Point3(canopyWidthX, 0.f, canopyWidthZ));
          pyramidPoints.push_back(bottomCenter + Point3(canopyWidthX, 0.f, -canopyWidthZ));
          pyramidPoints.push_back(bottomCenter + Point3(0.f, canopySizeScaled.y, 0.f));
          bottomCenter.y = canopyBottom;
          pyramidPoints.push_back(bottomCenter);
          PhysConvexHullCollision pyramidCollision(pyramidPoints, false);

          const PhysCollision *selCanopyCollision = &sphCanopyCollision;
          if (treeDestr.useBoxAsCanopyCollision)
            selCanopyCollision = &boxCanopyCollision;

          bool usePyramidCanopy = riDestrData.canopyTriangle || riDestrData.branchDestr->forcePyramidCanopy;
          if (useDebugTreeInstData)
          {
            const rendinstdestr::BranchDestr &debugBranchDestr =
              from_damage ? debugTreeInstData.branchDestrFromDamage : debugTreeInstData.branchDestrOther;
            if (debugBranchDestr.forcePyramidCanopy)
              usePyramidCanopy = true;
          }
          if (usePyramidCanopy)
          {
            sphTm.setcol(3, Point3::ZERO);
            selCanopyCollision = &pyramidCollision;
            canopyMass *= 0.33f; // pyramid volume is 1/3 of the box
          }
          PhysBodyCreationData canopyPbcd;
          canopyPbcd.friction = treeDestr.treeFriction;
          canopyPbcd.rollingFriction = treeDestr.canopyRollingFriction;
          canopyPbcd.restitution = treeDestr.canopyRestitution;
          canopyPbcd.linearDamping = canopyDampingLin;
          canopyPbcd.angularDamping = canopyDampingAng;
          canopyPbcd.autoInertia = true;
          // canopy should never collide with tank or other trees
          canopyPbcd.autoMask = false;
          canopyPbcd.mask = dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC ^ dacoll::EPL_DEFAULT;

          // body
          phys.additionalBody = new PhysBody(physWorld, canopyMass, selCanopyCollision, fallDirTm * sphTm, canopyPbcd);
        }

        // joint
        {
          TMatrix physBodyTm, canopyBodyTm;
          phys.physBody->getTmInstant(physBodyTm);
          phys.additionalBody->getTmInstant(canopyBodyTm);
          physBodyTm.setcol(3, phys.physBody->getCenterOfMassPosition());
          canopyBodyTm.setcol(3, phys.additionalBody->getCenterOfMassPosition());

          TMatrix jointTm = canopyImpulseDirTm;
          jointTm.setcol(3, tmNoScale.getcol(3));
          jointTm.col[3].y += (canopyCenterHt - canopySizeScaled.y * 0.5f);

          TMatrix body0Frame = inverse(physBodyTm) * jointTm;
          TMatrix body1Frame = inverse(canopyBodyTm) * jointTm;

          PhysJoint *joint = physWorld->create6DofSpringJoint(phys.physBody, phys.additionalBody, body0Frame, body1Frame);
          phys.joints.push_back() = joint;
          Phys6DofSpringJoint *dofSpring = Phys6DofSpringJoint::cast(joint);
          if (dofSpring)
          {
            const float cmLen = length(sphTm.getcol(3));
            const float stiffness = (mass + canopyMass * cmLen * 3.f) * canopyStiffnessMult;
            const float angularLimit = canopyJointAngLimit;
            dofSpring->setLimit(0, Point2(0.f, 0.f));
            dofSpring->setLimit(1, Point2(0.f, 0.f));
            dofSpring->setLimit(2, Point2(0.f, 0.f));
            dofSpring->setSpring(3, true, stiffness, canopySpringDampingAng, Point2(-angularLimit, angularLimit));
            dofSpring->setLimit(4, Point2(0.f, 0.f));
            dofSpring->setSpring(5, true, stiffness, canopySpringDampingAng, Point2(-angularLimit, angularLimit));
            for (int i = 0; i < 6; ++i)
            {
#if defined(USE_BULLET_PHYSICS)
              dofSpring->setParam(BT_CONSTRAINT_STOP_ERP, 0.2, i);
#endif
              dofSpring->setAxisDamping(i, canopyJointDamping);
            }
            dofSpring->setEquilibriumPoint();
          }

          if (riDestrData.branchDestr->bendStrength > 0.0f && cutType == rendinst::TreeInstData::CutType::None)
          {
            // Bend helper body
            {
              PhysSphereCollision sphBendHelperShape(0.5f);
              TMatrix bendHelperTm = TMatrix::IDENT;
              bendHelperTm.setcol(3, Point3(0, totalTreeHeight, 0));

              PhysBodyCreationData bendHelperPbcd;
              bendHelperPbcd.linearDamping = treeDestr.bendLinDamping;
              bendHelperPbcd.angularDamping = treeDestr.bendAngDamping;
              bendHelperPbcd.autoInertia = true;
              // disable all collisions (TODO: see if ground collision is needed)
              bendHelperPbcd.autoMask = false;
              bendHelperPbcd.mask = 0;

              // body
              phys.bendHelperBody =
                new PhysBody(physWorld, treeDestr.bendMass, &sphBendHelperShape, fallDirTm * bendHelperTm, bendHelperPbcd);
            }

            // bend helper joint
            {
              TMatrix physBodyTm, bendHelperBodyTm;
              phys.physBody->getTmInstant(physBodyTm);
              phys.bendHelperBody->getTmInstant(bendHelperBodyTm);
              physBodyTm.setcol(3, phys.physBody->getCenterOfMassPosition());
              bendHelperBodyTm.setcol(3, phys.bendHelperBody->getCenterOfMassPosition());

              TMatrix jointTm = canopyImpulseDirTm;
              jointTm.setcol(3, tmNoScale.getcol(3));
              jointTm.col[3].y += totalTreeHeight;

              TMatrix body0Frame = inverse(physBodyTm) * jointTm;
              TMatrix body1Frame = inverse(bendHelperBodyTm) * jointTm;

              PhysJoint *joint = physWorld->create6DofSpringJoint(phys.physBody, phys.bendHelperBody, body0Frame, body1Frame);
              phys.joints.push_back() = joint;
              Phys6DofSpringJoint *dofSpring = Phys6DofSpringJoint::cast(joint);
              if (dofSpring)
              {
                const float stiffness = treeDestr.bendSpringStiffness;
                dofSpring->setSpring(0, true, stiffness, 0.0f, Point2::ZERO);
                dofSpring->setLimit(1, Point2::ZERO);
                dofSpring->setSpring(2, true, stiffness, 0.0f, Point2::ZERO);

                dofSpring->setSpring(3, true, stiffness, 0.0f, Point2::ZERO);
                dofSpring->setLimit(4, Point2::ZERO);
                dofSpring->setSpring(5, true, stiffness, 0.0f, Point2::ZERO);
                dofSpring->setEquilibriumPoint();
              }
            }
          }
        }
        phys.lastValidTm = tmNoScale;
      }

      if (constrained_phys && (cutType == rendinst::TreeInstData::CutType::Bottom || cutType == rendinst::TreeInstData::CutType::None))
      {
        PhysJoint *joint = physWorld->create6DofJoint(phys.physBody, NULL, inverse(phys.centerOfMassTm), TMatrix::IDENT);
        phys.joints.push_back() = joint;
        Phys6DofJoint *dofJoint = Phys6DofJoint::cast(joint);
        if (dofJoint)
        {
          for (int i = 0; i < 6; ++i)
          {
            dofJoint->setLimit(i, Point2(0.0f, 0.0f));
            dofJoint->setAxisDamping(i, groundJointDamping);
          }
          dofJoint->setLimit(1, Point2(0.0f, groundJointMoveDistance));

          if (cutType == rendinst::TreeInstData::CutType::Bottom)
          {
            dofJoint->setLimit(3, Point2(-groundJointAngleLimitBottom * DEG_TO_RAD, groundJointAngleLimitBottom * DEG_TO_RAD));
            dofJoint->setLimit(5, Point2(-groundJointAngleLimitBottom * DEG_TO_RAD, groundJointAngleLimitBottom * DEG_TO_RAD));
          }
          else
          {
            dofJoint->setLimit(3, Point2(-groundJointAngleLimitWhole * DEG_TO_RAD, groundJointAngleLimitWhole * DEG_TO_RAD));
            dofJoint->setLimit(4, Point2(-FLT_MAX, FLT_MAX));
            dofJoint->setLimit(5, Point2(-groundJointAngleLimitWhole * DEG_TO_RAD, groundJointAngleLimitWhole * DEG_TO_RAD));
          }

#if defined(USE_BULLET_PHYSICS)
          for (int i = 0; i < 6; ++i)
            dofJoint->setParam(BT_CONSTRAINT_STOP_ERP, 0.2, i);
#endif

          phys.groundJointLimitsEnabled = true;
          phys.groundJointExists = true;
        }
      }

      float impulseOffsetY = localImpactPos.y;
      if (phys.treeInstData.cutType == rendinst::TreeInstData::CutType::Top)
        impulseOffsetY = -trunkShapeHeight * 0.5f;
      else if (phys.treeInstData.cutType == rendinst::TreeInstData::CutType::Bottom)
        impulseOffsetY = trunkShapeHeight * 0.5f;

      const Point3 impulsePos = tmNoScale * (phys.centerOfMassTm.getcol(3) + Point3(0.0f, impulseOffsetY, 0.0f));
      debugTreeInstData.impulsePositions.push_back(impulsePos);
      phys.physBody->addImpulse(impulsePos, Point3::x0y(impulseDir) * impulseVal);

      if (phys.additionalBody && (cutType == rendinst::TreeInstData::CutType::Top || cutType == rendinst::TreeInstData::CutType::None))
      {
        // canopy impulse
        // omega = vel / arm
        // vel = omega * arm
        // imp = vel * mass
        // imp = omega * arm * mass
        // Try to just not add impulse to the canopy separately
        float additionalImpulseVal = omega * canopySizeScaled.y * 0.5f * canopyMass * canopyImpulseMul;
        G_ASSERTF(!check_nan(additionalImpulseVal) && additionalImpulseVal < MAX_RI_DESTROY_IMPULSE, "additionalImpulseVal %f",
          additionalImpulseVal);
        phys.additionalBody->addImpulse((tmNoScale * sphTm).getcol(3), additionalImpulseVal * Point3::x0y(impulseDir));
      }

      phys.treeInstData.impactXZ = impulseDir * vel;

      if (enableBranchDestr)
      {
        phys.treeInstData.branchDestr = *riDestrData.branchDestr;
        if (!rendinst::fillTreeInstData(desc, from_damage, phys.treeInstData))
          phys.treeInstData.branchDestr.enableBranchDestruction = false;
        else
          debugTreeInstData.last_object_was_from_damage = from_damage;
      }
      phys.lastValidTm = tmNoScale;

      const char *destrFxTemplate = coll_info ? rendinst::getRIGenDestrPropsFxTemplate(coll_info->desc) : "";
      if (!constrained_phys && coll_info && (coll_info->destrFxId >= 0 || *destrFxTemplate) && ri_effect_cb)
        ri_effect_cb(coll_info->destrFxId, TMatrix::IDENT, tmNoScale, coll_info->desc.pool, false, nullptr, destrFxTemplate);
    }
    else
    {
      float hh = height * 0.5f;
      TMatrix cotm = TMatrix::IDENT;
      cotm.m[3][1] = hh;
      phys.riColObj = dacoll::add_dynamic_cylinder_collision(cotm, radius, height, /*uptr*/ nullptr, /*add_to_world*/ false);
      dacoll::obj_apply_impulse(phys.riColObj, impulse * mass, Point3(0.f, hh, 0.f));
    }

    if (create_destr && !create_phys && ri_effect_cb)
    {
      if (riDestrData.fxType != -1 || *riDestrData.fxTemplate != 0)
      {
        TMatrix fxTm = tmNoScale;
        fxTm *= riDestrData.fxScale;
        fxTm.setcol(3, tmNoScale.getcol(3));
        ri_effect_cb(riDestrData.fxType, TMatrix::IDENT, fxTm, desc.pool, false, nullptr, riDestrData.fxTemplate);
      }
    }
  }
  return offsetedDesc;
}

} // namespace rendinstdestr

rendinst::RendInstDesc rendinstdestr::create_tree_rend_inst_destr(const rendinst::RendInstDesc &desc, bool add_restorable,
  const Point3 &impactPos, const Point3 &_impulse, bool create_phys, bool constrained_phys, float wanted_omega, float at_time,
  const rendinst::CollisionInfo *coll_info, bool create_destr, bool from_damage)
{
  rendinst::RiDestrData riDestrData = rendinst::gather_ri_destr_data(desc, from_damage);
  if (!riDestrData.branchDestr)
    return desc;
  if (!get_destr_settings().createTreeDestr)
    create_destr = false;

  bool newPhysics = riDestrData.branchDestr->newPhysics;
#if RI_DESTR_DEBUG_ENABLED
  if (useDebugTreeInstData)
    newPhysics = from_damage ? debugTreeInstData.branchDestrFromDamage.newPhysics : debugTreeInstData.branchDestrOther.newPhysics;
#endif

  // at_time is not reliable, in some cases it's set to zero or very high values
  at_time = get_sync_time();

  if (newPhysics)
    return create_tree_rend_inst_destr_new(desc, add_restorable, impactPos, _impulse, create_phys, constrained_phys, wanted_omega,
      at_time, coll_info, create_destr, from_damage);
  else
    return create_tree_rend_inst_destr_old(desc, add_restorable, impactPos, _impulse, create_phys, constrained_phys, wanted_omega,
      at_time, coll_info, create_destr, from_damage);
}
