// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/rendinstPhys.h>

#include <EASTL/vector.h>
#include <generic/dag_tab.h>
#include <math/dag_frustum.h>
#include <math/dag_mathUtils.h>
#include <math/dag_TMatrix.h>
#include <math/dag_vecMathCompatibility.h>
#include <vecmath/dag_vecMath.h>
#include <debug/dag_debug3d.h>
#include <memory/dag_framemem.h>
#include <perfMon/dag_statDrv.h>
#include <phys/dag_physDecl.h>
#include <phys/dag_physics.h>
#include <util/dag_convar.h>

#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/contactResultWrapper.h>
#include <rendInst/rendInstDebris.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/treeDestr.h>

#include "riDestrInternal.h"
#include "treeDestr.h"


static rendinst::RiPhysSettings riPhysSettings;

rendinst::RiPhysSettings &rendinst::get_mutable_ri_phys_settings() { return riPhysSettings; }
const rendinst::RiPhysSettings &rendinst::get_ri_phys_settings() { return riPhysSettings; }


RendInstPhys::RendInstPhys() :
  originalTm(TMatrix::IDENT),
  desc(-1, -1, -1, 0, 0),
  descForDestr(-1, -1, -1, 0, 0),
  ri(NULL),
  riColObj(),
  physModel(NULL),
  physBody(NULL),
  additionalBody(NULL),
  bendHelperBody(NULL),
  joints(midmem),
  centerOfMassTm(TMatrix::IDENT),
  scale(1.f, 1.f, 1.f),
  lastValidTm(TMatrix::IDENT),
  ttl(0.f),
  maxTtl(0.f),
  maxLifeDist(-1.f),
  collisionDistLimit(-1.f),
  collisionAngleLimitCos(-1.f)
{}

RendInstPhys::RendInstPhys(RendInstPhysType type, const rendinst::RendInstDesc &from_desc, const TMatrix &tm, float bodyMass,
  const Point3 &moment, const Point3 &scal, const Point3 &centerOfGravity, gamephys::DynamicPhysModel::PhysType phys_type,
  const TMatrix &original_tm, float max_life_time, float max_life_dist, rendinst::DestroyedRi *ri) :
  type(type),
  desc(from_desc),
  descForDestr(from_desc),
  ri(ri),
  riColObj(),
  physBody(NULL),
  additionalBody(NULL),
  bendHelperBody(NULL),
  joints(midmem),
  scale(scal),
  ttl(1.f),
  maxTtl(max_life_time),
  maxLifeDist(max_life_dist),
  collisionDistLimit(-1.f),
  collisionAngleLimitCos(-1.f),
  originalTm(original_tm),
  centerOfMassTm(TMatrix::IDENT),
  lastValidTm(TMatrix::IDENT)
{
  physModel = new gamephys::DynamicPhysModel(tm, bodyMass, moment, centerOfGravity, phys_type);
}

void RendInstPhys::cleanup()
{
  dacoll::destroy_dynamic_collision(riColObj);
  riColObj = CollisionObject();
  del_it(physModel);
  clear_all_ptr_items_and_shrink(joints);
  del_it(physBody);
  del_it(additionalBody);
  del_it(bendHelperBody);
  treeSound.destroy(lastValidTm != TMatrix::IDENT ? lastValidTm : originalTm);
}

void RendInstTreeSound::init(ri_tree_sound_cb tree_sound_cb, const TMatrix &tm, float tree_height, bool is_bush)
{
  G_ASSERT_RETURN(!inited(), );
  G_ASSERT_RETURN(tree_sound_cb, );
  soundCb = tree_sound_cb;
  wasFalled = false;
  desc.isBush = is_bush;
  desc.treeHeight = tree_height;
  isInited = soundCb(TREE_SOUND_CB_INIT, desc, soundCBData, tm);
}

void RendInstTreeSound::destroy(const TMatrix &tm)
{
  if (inited())
    soundCb(TREE_SOUND_CB_DESTROY, desc, soundCBData, tm);
}

void RendInstTreeSound::setFalled(const TMatrix &tm)
{
  if (inited() && !wasFalled)
  {
    wasFalled = true;
    soundCb(TREE_SOUND_CB_ON_FALLED, desc, soundCBData, tm);
  }
}

void RendInstTreeSound::updateFalling(const TMatrix &tm)
{
  if (inited() && !wasFalled)
    soundCb(TREE_SOUND_CB_FALLING, desc, soundCBData, tm);
}

#if RI_DESTR_DEBUG_ENABLED
CONSOLE_FLOAT_VAL("ridestr", velocityThresholdOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", groundJointBreakVelocityCapOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", maxAngularVelocityOverride, -1.0f);
#endif

namespace rendinstdestr
{

Tab<RendInstPhys> riPhys(midmem);
static constexpr float max_ri_phys_rad = 5.f;
int currentCutTreeCount = 0;

static void cleanup_ri_phys(RendInstPhys &phys)
{
  if (phys.treeInstData.cutType == rendinst::TreeInstData::CutType::Top)
    currentCutTreeCount--;
  phys.cleanup();
}

bool restore_destroyed_tree(RendInstPhys &phys)
{
  const rendinst::RendInstDesc restoredDesc = phys.ri->savedData.desc;
  TMatrix restoredTm;
  v_mat_43cu_from_mat44(restoredTm.array, phys.ri->savedData.tm);
  if (!rendinst::returnRIGenExternalControl(phys.desc, phys.ri))
    return false;
  if (on_ri_restored_cb)
    on_ri_restored_cb(restoredDesc, restoredTm);
  cleanup_ri_phys(phys);
  return true;
}

} // namespace rendinstdestr

void rendinstdestr::update_ri_phys(float dt, float current_time, const Frustum *frustum)
{
  PhysWorld *physWorld = dacoll::get_phys_world();
  physWorld->fetchSimRes(true);

  {
    TIME_PROFILE_DEV(riPhys);

    struct RendInstUpdateTm
    {
      TMatrix tm;
      int physIdx;
      bool moved;
      bool forceUpdateDestructionData;
    };

#if RI_DESTR_DEBUG_ENABLED
    const float velocityThreshold = velocityThresholdOverride < 0.0f ? 40.0f : velocityThresholdOverride;
    const float maxAngularVelocity =
      maxAngularVelocityOverride < 0.0f ? rendinstdestr::get_tree_destr().maxAngularVelocity : maxAngularVelocityOverride;
    const float groundJointBreakVelocityCap = groundJointBreakVelocityCapOverride < 0.0f ? 15.0f : groundJointBreakVelocityCapOverride;
#else
    const float velocityThreshold = 40.0f;
    const float groundJointBreakVelocityCap = 15.0f;
    const float maxAngularVelocity = rendinstdestr::get_tree_destr().maxAngularVelocity;
#endif
    const int maxDestroyedTreeCount = get_tree_destr().maxDestroyedTreeCount;

    eastl::vector<RendInstUpdateTm, framemem_allocator> updateList;
    updateList.reserve(riPhys.size());
    for (int i = 0; i < riPhys.size(); ++i)
    {
      RendInstPhys &phys = riPhys[i];

      bool isRiExtraValid = rendinst::isRiGenExtraValid(phys.ri->riHandle);

      bool physObject = phys.isPhysObject();
      const bool physObjectActive =
        physObject && (phys.physBody && phys.physBody->isActive() && phys.physBody->getInteractionLayer()) ||
        (phys.physBody && phys.additionalBody && phys.additionalBody->isActive() && phys.additionalBody->getInteractionLayer());

      bool actived = phys.ttl >= 0.f && isRiExtraValid;
      bool disableGroundJoint = false;
      bool forceUpdateDestructionData = false;

      const float currentVelocitySq = phys.physBody ? lengthSq(phys.physBody->getVelocity()) : 0.0f;
      const bool velocityThresholdExceeded = currentVelocitySq > sqr(velocityThreshold);
      const float groundJointVelocityThreshold = velocityThreshold * 0.5f;
      const bool groundJointVelocityThresholdExceeded = currentVelocitySq > sqr(groundJointVelocityThreshold);

      if (physObject)
      {
        if (physObjectActive)
        {
          if (phys.groundJointExists && groundJointVelocityThresholdExceeded)
            disableGroundJoint = true;

          if (velocityThresholdExceeded || phys.maxTtl < 0.0f)
          {
            phys.physBody->setTm(phys.lastValidTm * phys.centerOfMassTm);
            phys.physBody->setVelocity(Point3(0.f, 0.f, 0.f));
            phys.physBody->setAngularVelocity(Point3(0.f, 0.f, 0.f));
            phys.ttl = -1.f;
          }
          else
            phys.ttl = 5.f;
        }
        else
        {
          if (phys.physBody && !phys.physBody->getInteractionLayer())
            phys.physBody->activateBody(true);
          if (phys.additionalBody && !phys.additionalBody->getInteractionLayer())
            phys.additionalBody->activateBody(true);
          phys.ttl -= dt;
        }
      }
      else
      {
        if (phys.physModel->isActive())
          phys.ttl = 1.f;
        else
          phys.ttl -= dt;
      }
      if (actived && physObject && phys.maxLifeDist > 0.f && !physObjectActive)
      {
        const float distSq = lengthSq(phys.originalTm.getcol(3) - phys.lastValidTm.getcol(3));
        if (distSq > sqr(phys.maxLifeDist))
        {
          phys.maxLifeDist = -1.f;
          phys.maxTtl = -1.f;
          phys.ttl = -1.f;
        }
      }
      if (actived && physObject && phys.physBody && phys.physBody->getInteractionLayer())
      {
        Point3 angularVelocity = phys.physBody->getAngularVelocity();
        float angularSpeedSq = angularVelocity.lengthSq();
        if (angularSpeedSq > sqr(maxAngularVelocity))
        {
          angularVelocity = angularVelocity * (maxAngularVelocity / sqrt(angularSpeedSq));
          phys.physBody->setAngularVelocity(angularVelocity);
        }

        bool disableKinematicCollision = false;
        if (phys.collisionDistLimit >= 0.f)
        {
          const Point3 distanceFromOriginal = phys.originalTm.getcol(3) - phys.lastValidTm.getcol(3);
          const float distSq =
            phys.treeInstData.branchDestr.newPhysics ? lengthSq(Point2::xz(distanceFromOriginal)) : lengthSq(distanceFromOriginal);
          if (distSq > sqr(phys.collisionDistLimit))
            disableKinematicCollision = true;
        }
        if (phys.collisionAngleLimitCos >= 0.0f)
        { // originalTm contains scale, lastValidTm doesn't so only divide with it once
          const float angleCos = dot(phys.originalTm.getcol(1), phys.lastValidTm.getcol(1)) / phys.scale.y;
          if (angleCos < phys.collisionAngleLimitCos)
          {
            if (phys.groundJointExists && phys.treeInstData.cutType == rendinst::TreeInstData::CutType::None)
              disableGroundJoint = true;
            disableKinematicCollision = true;
          }
        }

        if (disableKinematicCollision)
        {
          phys.collisionDistLimit = -1.f;
          phys.collisionAngleLimitCos = -1.f;
          phys.physBody->setGroupAndLayerMask(dacoll::EPL_DEFAULT, dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC);
          if (phys.additionalBody)
            phys.additionalBody->setGroupAndLayerMask(dacoll::EPL_DEFAULT, dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC);

          if (phys.treeInstData.branchDestr.newPhysics)
          {
            phys.physBody->setDamping(0.3, 0.3);
            phys.physBody->activateBody(true);
          }
        }

        if (disableGroundJoint)
        { // Remove ground joint (it's the last joint in the list)
          // old physics don't set angle limit, so this won't happen in that case
          PhysJoint *groundJoint = phys.joints.back();
          delete groundJoint;
          phys.joints.pop_back();
          phys.groundJointExists = false;

          if (groundJointVelocityThresholdExceeded)
          { // clamp shape velocity below threshold
            const Point3 currentVelocity = phys.physBody->getVelocity();
            const Point3 clampedVelocity = currentVelocity * (groundJointBreakVelocityCap / sqrt(currentVelocitySq));
            phys.physBody->setVelocity(clampedVelocity);
            phys.physBody->setAngularVelocity(Point3(0.f, 0.f, 0.f));

            if (phys.additionalBody)
            {
              phys.additionalBody->setVelocity(Point3(0, 0, 0));
              phys.additionalBody->setAngularVelocity(Point3(0, 0, 0));
            }
          }
          phys.physBody->activateBody(true);
        }
      }
      phys.maxTtl -= dt;
      if (phys.ttl < 0.f || !isRiExtraValid)
      {
        if (!physObject)
        {
          if (restore_destroyed_tree(phys))
          {
            erase_items(riPhys, i, 1);
            i--;
            continue;
          }
        }
        else if (actived && phys.physBody && phys.physBody->getInteractionLayer())
        {
          phys.physBody->setGroupAndLayerMask(0, 0);
          phys.physBody->setGravity(Point3(0.f, -0.1f, 0.f));
          phys.physBody->setDamping(0, 0);
          phys.physBody->activateBody(true);
          if (phys.treeSound.inited())
            phys.treeSound.setFalled(phys.lastValidTm);
          clear_all_ptr_items_and_shrink(phys.joints);
          del_it(phys.additionalBody);
          phys.ttl = 15.f;
          phys.maxTtl = 15.f;
          phys.treeInstData.disappearStartTimeAt = current_time;
          forceUpdateDestructionData = true;
        }
        else if (phys.physBody || !isRiExtraValid)
        {
          if (rendinst::removeRIGenExternalControl(phys.desc, phys.ri))
          {
            cleanup_ri_phys(phys);
            erase_items(riPhys, i, 1);
            i--;
          }
          continue;
        }
      }

      phys.physModel->update(dt);

      DECL_ALIGN16(TMatrix, tm) = TMatrix::IDENT;
      if (physObject)
      {
        if (phys.physBody)
        {
          phys.physBody->getTm(tm);
          if (ri_debug_collision) [[unlikely]]
            dacoll::draw_phys_body(phys.physBody, phys.physBody->getInteractionLayer() & dacoll::EPL_KINEMATIC ? 0xFFFFFF : 0xFF44FF);
        }
        TMatrix additionalTm = TMatrix::IDENT;
        if (phys.additionalBody)
        {
          phys.additionalBody->getTm(additionalTm);
          if (ri_debug_collision) [[unlikely]]
            dacoll::draw_phys_body(phys.additionalBody,
              phys.additionalBody->getInteractionLayer() & dacoll::EPL_KINEMATIC ? 0xFFFFFF : 0xFF44FF);
        }
        if (check_nan(tm.getcol(3)) || lengthSq(tm.getcol(3)) > sqr(1e6f))
        {
          logerr("Removed invalid destr phys body at pos %@", tm.getcol(3));
          if (phys.physBody) // make static analyzer happy
          {
            phys.physBody->setTm(phys.lastValidTm * phys.centerOfMassTm);
            phys.physBody->setVelocity(Point3(0.f, 0.f, 0.f));
            phys.physBody->setAngularVelocity(Point3(0.f, 0.f, 0.f));
          }
          phys.ttl = -1.f;
          continue;
        }
        tm *= inverse(phys.centerOfMassTm);
        phys.lastValidTm = tm;

        if (phys.bendHelperBody)
        {
          if (ri_debug_collision) [[unlikely]]
            dacoll::draw_phys_body(phys.bendHelperBody, 0x0000FF);
          TMatrix bendTm = TMatrix::IDENT;
          phys.bendHelperBody->getTm(bendTm);
          Point3 bendPos = bendTm.getcol(3);
          Point3 bendPosLocal = inverse(tm) * bendPos;
          phys.treeInstData.bendAnglePrev = phys.treeInstData.bendAngle;
          phys.treeInstData.bendStrengthPrev = phys.treeInstData.bendStrength;
          phys.treeInstData.bendAngle = atan2f(bendPosLocal.x, bendPosLocal.z) + PI;
          float bendStrength = phys.treeInstData.branchDestr.bendStrength;
#if RI_DESTR_DEBUG_ENABLED
          if (useDebugTreeInstData)
            bendStrength = debugTreeInstData.branchDestrFromDamage.bendStrength;
#endif
          phys.treeInstData.bendStrength = sqrt(bendPosLocal.x * bendPosLocal.x + bendPosLocal.z * bendPosLocal.z) * bendStrength;
          if (phys.treeInstData.bendStrength > 0)
            forceUpdateDestructionData = true;
        }

        if (phys.additionalBody && phys.treeSound.inited() && !phys.treeSound.falled())
        {
          struct WrapperContactResultCallback
          {
            typedef ::CollisionObjectUserData obj_user_data_t;
            typedef ::gamephys::CollisionContactData contact_data_t;
            bool haveContact = false;
            void addSingleResult(const contact_data_t &, obj_user_data_t *, obj_user_data_t *) { haveContact = true; }
            static void visualDebugForSingleResult(...) {}
            bool needsCollision(obj_user_data_t *, bool b_is_static) { return b_is_static; }
          } contactCallback;

          physWorld->contactTest(phys.additionalBody, contactCallback);
          if (contactCallback.haveContact)
            phys.treeSound.setFalled(additionalTm);
          else
            phys.treeSound.updateFalling(tm);
        }

        if (
          phys.joints.size() && phys.groundJointLimitsEnabled && phys.treeInstData.cutType == rendinst::TreeInstData::CutType::Bottom)
        {
          // New physics only. Check if bottom half of a cut tree touches kinematic object
          // In this case, remove ground joint to let the tree fall. We don't support proper
          // physics simulation for the bottom half because it jerks around too much.
          struct WrapperContactResultCallback
          {
            typedef ::CollisionObjectUserData obj_user_data_t;
            typedef ::gamephys::CollisionContactData contact_data_t;
            void addSingleResult(const contact_data_t &, obj_user_data_t *, obj_user_data_t *)
            {
              Phys6DofJoint *dofJoint = Phys6DofJoint::cast(riPhys->joints[0]);
              if (dofJoint)
              {
                dofJoint->setLimit(3, Point2(-PI / 2, PI / 2));
                dofJoint->setLimit(5, Point2(-PI / 2, PI / 2));
                dofJoint->setAxisDamping(3, 0.1f);
                dofJoint->setAxisDamping(5, 0.1f);
                riPhys->physBody->activateBody(true);
              }
              riPhys->groundJointLimitsEnabled = false;
            }
            static void visualDebugForSingleResult(...) {}
            bool needsCollision(obj_user_data_t *, bool b_is_static) { return !b_is_static; }
            RendInstPhys *riPhys;
          } kinematicCallback;

          kinematicCallback.riPhys = &phys;
          physWorld->contactTest(phys.physBody, kinematicCallback, dacoll::EPL_DEFAULT, dacoll::EPL_KINEMATIC);
        }
      }
      else if (phys.riColObj.body)
      {
        phys.physModel->location.toTM(tm);
        phys.riColObj.body->setTmInstant(tm);
        if (ri_debug_collision) [[unlikely]]
          dacoll::draw_phys_body(phys.riColObj.body);
      }

      mat44f m44;
      v_mat44_make_from_43cu_unsafe(m44, tm.array);
      v_mat44_apply_scale33(m44, v_ldu(&phys.scale.x));

      bbox3f bbox = rendinst::riex_get_lbb(rendinst::handle_to_ri_type(phys.ri->riHandle));
      bbox3f fullBBox;
      v_bbox3_init(fullBBox, m44, bbox);
      if (frustum && !frustum->testBoxB(fullBBox.bmin, fullBBox.bmax))
      {
        if (phys.treeInstData.originalTmDataId != rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID)
        {
          rendinst::removeRiExtraPerInstanceRenderAdditionalData(phys.treeInstData.originalTmDataId,
            rendinst::RiExtraPerInstanceDataType::INITIAL_TM_3X3);
          phys.treeInstData.originalTmDataId = rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID;
        }
        if (phys.treeInstData.destructionDataId != rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID)
        {
          rendinst::removeRiExtraPerInstanceRenderAdditionalData(phys.treeInstData.destructionDataId,
            rendinst::RiExtraPerInstanceDataType::DESTR_RENDER_DATA);
          phys.treeInstData.destructionDataId = rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID;
        }
        continue;
      }
      RendInstUpdateTm &destrPos = updateList.push_back();
      destrPos.physIdx = i;
      v_mat_43cu_from_mat44(destrPos.tm.array, m44);
      destrPos.moved = physObject;
      destrPos.forceUpdateDestructionData = forceUpdateDestructionData;
    }
    if (!updateList.empty())
    {
      TIME_PROFILE(rendinstDestrTmUpdate);

      vec3f frustumPoints[8];
      frustum->generateAllPointFrustm(frustumPoints);
      Point3 nearPlaneCenter = (as_point3(&frustumPoints[2]) + as_point3(&frustumPoints[4])) * 0.5f;

      int currentDestroyedTreeCount = 0;
      rendinst::ScopedRIExtraWriteLock wr;
      for (int i = 0; i < updateList.size(); i++)
      {
        // Iterate backwards so that if no more items fit in the render data buffer,
        // latest items take precedence.
        RendInstUpdateTm &l = updateList[updateList.size() - i - 1];
        mat44f m44;
        mat43f m43;
        v_mat44_make_from_43cu_unsafe(m44, l.tm.array);
        v_mat44_transpose_to_mat43(m43, m44);

        RendInstPhys &phys = riPhys[l.physIdx];

        const mat43f &prevTm43 = rendinst::getRIGenExtra43(phys.ri->riHandle);
        mat44f prevTm44;
        v_mat43_transpose_to_mat44(prevTm44, prevTm43);
        TMatrix prevTm;
        v_mat_43cu_from_mat44(prevTm.array, prevTm44);

        // Avoid setting zero prev matrix when showing a hidden tree again
        const bool wasHidden = prevTm.getcol(0).lengthSq() <= 0.f;
        if (wasHidden)
          prevTm = l.tm;

        rendinst::RiExtraPerInstanceGpuItem prevTmRenderData[4];

        prevTmRenderData[0] = {bitwise_cast<uint32_t>(prevTm.m[0][0]), bitwise_cast<uint32_t>(prevTm.m[0][1]),
          bitwise_cast<uint32_t>(prevTm.m[0][2]), bitwise_cast<uint32_t>(0.0f)};
        prevTmRenderData[1] = {bitwise_cast<uint32_t>(prevTm.m[1][0]), bitwise_cast<uint32_t>(prevTm.m[1][1]),
          bitwise_cast<uint32_t>(prevTm.m[1][2]), bitwise_cast<uint32_t>(0.0f)};
        prevTmRenderData[2] = {bitwise_cast<uint32_t>(prevTm.m[2][0]), bitwise_cast<uint32_t>(prevTm.m[2][1]),
          bitwise_cast<uint32_t>(prevTm.m[2][2]), bitwise_cast<uint32_t>(0.0f)};
        prevTmRenderData[3] = {bitwise_cast<uint32_t>(prevTm.m[3][0]), bitwise_cast<uint32_t>(prevTm.m[3][1]),
          bitwise_cast<uint32_t>(prevTm.m[3][2]), bitwise_cast<uint32_t>(1.0f)};

        rendinst::setRiExtraPerInstanceRenderAdditionalData(phys.ri->riHandle, rendinst::RiExtraPerInstanceDataType::PREV_TM_DATA,
          rendinst::RiExtraPerInstanceDataPersistence::SINGLE_FRAME, prevTmRenderData, 4);

        if (phys.treeInstData.branchDestr.enableBranchDestruction &&
            phys.treeInstData.originalTmDataId == rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID)
        {
          TMatrix &tmOriginal = phys.originalTm;
          rendinst::RiExtraPerInstanceGpuItem originalTmRenderData[3];
          originalTmRenderData[0] = {bitwise_cast<uint32_t>(tmOriginal.m[0][0]), bitwise_cast<uint32_t>(tmOriginal.m[0][1]),
            bitwise_cast<uint32_t>(tmOriginal.m[0][2]), bitwise_cast<uint32_t>(0.0f)};
          originalTmRenderData[1] = {bitwise_cast<uint32_t>(tmOriginal.m[1][0]), bitwise_cast<uint32_t>(tmOriginal.m[1][1]),
            bitwise_cast<uint32_t>(tmOriginal.m[1][2]), bitwise_cast<uint32_t>(0.0f)};
          originalTmRenderData[2] = {bitwise_cast<uint32_t>(tmOriginal.m[2][0]), bitwise_cast<uint32_t>(tmOriginal.m[2][1]),
            bitwise_cast<uint32_t>(tmOriginal.m[2][2]), bitwise_cast<uint32_t>(0.0f)};
          phys.treeInstData.originalTmDataId = rendinst::setRiExtraPerInstanceRenderAdditionalData(phys.ri->riHandle,
            rendinst::RiExtraPerInstanceDataType::INITIAL_TM_3X3, rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT,
            originalTmRenderData, 3);
        }

        const bool passedDistanceCheck =
          (phys.originalTm.col[3] - nearPlaneCenter).lengthSq() < sqr(phys.treeInstData.branchDestr.maxVisibleDistance);
        const bool destructionAllowed = currentDestroyedTreeCount < maxDestroyedTreeCount && passedDistanceCheck;

        uint64_t destrDataId = phys.treeInstData.destructionDataId;
        if (!destructionAllowed)
          destrDataId = rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID;
        else if (phys.treeInstData.destructionDataId == rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID ||
                 useDebugTreeInstData || l.forceUpdateDestructionData)
        {
          destrDataId = rendinst::updateTreeDestrRenderData(phys.ri->riHandle, phys.treeInstData,
            useDebugTreeInstData ? &debugTreeInstData : nullptr);
        }

        if (destrDataId == rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID &&
            phys.treeInstData.destructionDataId != rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID)
        {
          rendinst::removeRiExtraPerInstanceRenderAdditionalData(phys.treeInstData.destructionDataId,
            rendinst::RiExtraPerInstanceDataType::DESTR_RENDER_DATA);
          phys.treeInstData.destructionDataId = rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID;
        }
        else
        {
          G_ASSERT(phys.treeInstData.destructionDataId == rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID ||
                   phys.treeInstData.destructionDataId == destrDataId);
          phys.treeInstData.destructionDataId = destrDataId;
        }

        bool hideInstance = false;

        if (phys.treeInstData.destructionDataId != rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID)
          currentDestroyedTreeCount++;
        else if (phys.treeInstData.cutType == rendinst::TreeInstData::CutType::Bottom)
          hideInstance = true;

        if (hideInstance)
        {
          m43.row0 = v_perm_xyzd(v_zero(), m43.row0);
          m43.row1 = v_perm_xyzd(v_zero(), m43.row1);
          m43.row2 = v_perm_xyzd(v_zero(), m43.row2);
          rendinst::moveRIGenExtra43(phys.ri->riHandle, m43, l.moved, true);
        }
        else
        {
          rendinst::moveRIGenExtra43(phys.ri->riHandle, m43, l.moved, true);
        }
      }
    }
  }
}

void rendinstdestr::debug_draw_ri_phys()
{
  for (auto &phys : riPhys)
  {
    TMatrix tm = phys.lastValidTm;
    BBox3 bbox = rendinst::getRIGenFullBBox(phys.descForDestr);
    mat44f mat;
    v_mat44_make_from_43cu_unsafe(mat, tm.m[0]);

    bbox3f bbox_transformed;
    v_bbox3_init(bbox_transformed, mat, v_ldu_bbox3(bbox));

    BBox3 resBox;
    resBox += as_point3(&bbox_transformed.bmin);
    resBox += as_point3(&bbox_transformed.bmax);
    draw_cached_debug_box(resBox, E3DCOLOR(0, 0, 255));

    end_draw_cached_debug_lines();
    set_cached_debug_lines_wtm(TMatrix::IDENT);
  }
}

bbox3f rendinstdestr::get_ri_phys_containing_bbox(const bbox3f &intersect_bbox, const Frustum &intersect_frustum, bool only_trees)
{
  bbox3f resultBbox;
  v_bbox3_init_empty(resultBbox);
  for (auto &phys : riPhys)
  {
    if (only_trees && phys.type != RendInstPhysType::TREE)
      continue;

    TMatrix tm = phys.lastValidTm;
    BBox3 bbox = rendinst::getRIGenFullBBox(phys.descForDestr);
    mat44f mat;
    v_mat44_make_from_43cu_unsafe(mat, tm.m[0]);

    bbox3f bbox_transformed;
    v_bbox3_init(bbox_transformed, mat, v_ldu_bbox3(bbox));

    if (!v_bbox3_test_box_intersect(bbox_transformed, intersect_bbox))
      continue;
    if (!intersect_frustum.testBoxB(bbox_transformed.bmin, bbox_transformed.bmax))
      continue;

    v_bbox3_add_pt(resultBbox, bbox_transformed.bmin);
    v_bbox3_add_pt(resultBbox, bbox_transformed.bmax);
  }
  return resultBbox;
}

void rendinstdestr::remove_tree_rendinst_destr(const rendinst::RendInstDesc &desc)
{
  rendinst::delRIGenExtraFromCell(desc);

  for (int i = 0; i < riPhys.size(); ++i)
  {
    if (riPhys[i].descForDestr != desc)
      continue;

    rendinst::removeRIGenExternalControl(riPhys[i].desc, riPhys[i].ri);
    cleanup_ri_phys(riPhys[i]);
    erase_items(riPhys, i, 1);
    return;
  }
}

void rendinstdestr::clear_phys_objs()
{
  const bool clearExternal = rendinst::should_clear_external_controls();
  for (int i = 0; i < riPhys.size(); ++i)
  {
    if (clearExternal && riPhys[i].physBody)
      rendinst::removeRIGenExternalControl(riPhys[i].desc, riPhys[i].ri);
    cleanup_ri_phys(riPhys[i]);
  }
  clear_and_shrink(riPhys);
}

int rendinstdestr::test_dynobj_to_ri_phys_collision(const CollisionObject &coA, const TMatrix &tmA, float max_rad)
{
  PhysWorld *physWorld = dacoll::get_phys_world();
  int countCollision = 0;
  coA.body->setTmInstant(tmA);
  physWorld->fetchSimRes(true);
  for (int i = 0; i < riPhys.size(); ++i)
  {
    RendInstPhys &phys = riPhys[i];

    bool physObject = phys.isPhysObject();
    if (physObject)
      continue;

    TMatrix physTm = TMatrix::IDENT;
    dacoll::get_coll_obj_tm(phys.riColObj, physTm);
    if (lengthSq(physTm.getcol(3) - tmA.getcol(3)) > sqr(max_ri_phys_rad + max_rad))
      continue;

    int prevCountContacts = phys.physModel->contacts.size();
    WrapperContactResultCB physCollCb(phys.physModel->contacts);
    physWorld->contactTestPair(phys.riColObj.body, coA.body, physCollCb);
    countCollision += phys.physModel->contacts.size() - prevCountContacts;
  }
  return countCollision;
}

int rendinstdestr::test_dynobj_to_ri_phys_collision(const CollisionObject &coA, float max_rad)
{
  TMatrix tmA = TMatrix::IDENT;

  PhysWorld *physWorld = dacoll::get_phys_world();
  physWorld->fetchSimRes(true);
  coA.body->getTmInstant(tmA);

  return test_dynobj_to_ri_phys_collision(coA, tmA, max_rad);
}

void rendinstdestr::gather_ri_phys_for_dynobj_test(Tab<Point3> &out_pos)
{
  for (const RendInstPhys &phys : riPhys)
  {
    if (phys.isPhysObject())
      continue;
    TMatrix physTm = TMatrix::IDENT;
    dacoll::get_coll_obj_tm(phys.riColObj, physTm);
    out_pos.push_back(physTm.getcol(3));
  }
}

bool rendinstdestr::is_dynobj_near_ri_phys(dag::ConstSpan<Point3> ri_phys_pos, const Point3 &pos, float max_rad)
{
  const float maxDistSq = sqr(max_ri_phys_rad + max_rad);
  for (const Point3 &riPos : ri_phys_pos)
    if (lengthSq(riPos - pos) <= maxDistSq)
      return true;
  return false;
}
