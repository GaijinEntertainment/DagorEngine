//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/array.h>
#include <EASTL/optional.h>
#include <animChar/dag_animCharacter2.h>
#include <math/dag_geomTree.h>
#include <math/dag_geomTreeMap.h>
#include <drv/hid/dag_hiVrInput.h>
#include <daECS/core/componentType.h>
#include <gamePhys/collision/collisionObject.h>

class AnimatedPhys;
class PhysVars;
class TexStreamingContext;
class CollisionResource;

namespace vr
{

using VrInput = HumanInput::VrInput;

class VrHands
{
public:
  VrHands() = default;
  ~VrHands();

  struct OneHandState
  {
    TMatrix grip = TMatrix::IDENT;
    TMatrix aim = TMatrix::IDENT;
    float squeeze = 0.0f;
    float indexFinger = 0.0f;
    float thumb = 0.0f;
    bool isActive = false;
    eastl::vector<TMatrix> joints{};
    eastl::optional<Point3> indexFingerAttachmentPoint{};
    struct Attachment
    {
      bool isAttached = false;
      Point3 a{};
      Point3 b{};
      float r = 0.0f;
      Point3 pos{};
      enum Shape
      {
        SPHERE,
        CYLINDER,
        WHEEL
      } shape = CYLINDER;
    } attachment{};
  };

  using HandsState = eastl::array<OneHandState, VrInput::Hands::Total>;
  using Joints = HumanInput::VrInput::Joints;
  using JointNameMap = eastl::tuple_vector<Joints, const char *>;
  using GeomNodeTreeJointIndices = eastl::array<GeomNodeTree::Index16, Joints::VRIN_HAND_JOINTS_TOTAL>;

  struct FingerStates
  {
    enum Finger
    {
      THUMB = 0,
      INDEX,
      MIDDLE,
      RING,
      LITTLE,

      TOTAL
    };

    enum State
    {
      FOLDED,
      RELAXED,
      POINTING
    } states[Finger::TOTAL];

    enum class Gesture
    {
      GRAB,
      POINTING
    };

    State thumb() const { return states[THUMB]; }
    State index() const { return states[INDEX]; }
    State middle() const { return states[MIDDLE]; }
    State ring() const { return states[RING]; }
    State little() const { return states[LITTLE]; }

    bool isFolded(const State &state) const { return state == FOLDED; }
    bool isPointing(const State &state) const { return state == POINTING; }

    bool isGestureActive(Gesture gesture) const
    {
      switch (gesture)
      {
        case Gesture::POINTING:
        {
          return isFolded(middle()) && isFolded(ring()) && isFolded(little()) && isPointing(index());
        }
        case Gesture::GRAB:
        {
          return isFolded(middle()) && isFolded(ring()) && isFolded(little());
        }
        default:
        {
          return false;
        }
      }
    }
  };

  void clear();

  struct HandsInitInfo
  {
    struct SingleHandInitInfo
    {
      const char *handModelName;
      JointNameMap jointNodesMap;
    };
    float grabRadius;
    eastl::array<SingleHandInitInfo, VrInput::Hands::Total> hands;
  };

  // You have 2 possible interaction with this class: it may hold IAnimCharacter2 or you can store
  // animchar outside the class (in ECS)
  // If you want to store animchar in the class, use this methods:
  void init(const HandsInitInfo &);
  // See Local Reference Space in OpenXR documentation
  // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#reference-spaces
  void update(float dt, const TMatrix &local_ref_space_tm, HandsState &&hands);
  void applyOffset(VrInput::Hands hand, const Point3 &offset);
  void updateCollision(const CollisionResource &collres, const GeomNodeTree *coll_node_tree,
    dag::ConstSpan<CollisionObject> collision_objects, uint64_t active_collision_bit_mask, const TMatrix &tm,
    const TMatrix &ref_space);
  void beforeRender(const Point3 &playspace_to_cam_vec, const Point3 &cam_pos_in_world);
  void render(TexStreamingContext texCtx);

  // If you want to store data in ecs, use this functions:
  void init(AnimV20::AnimcharBaseComponent &left_char, AnimV20::AnimcharBaseComponent &right_char);
  void updateHand(const TMatrix &local_ref_space_tm, VrInput::Hands side, AnimV20::AnimcharBaseComponent &animchar_component,
    TMatrix &tm);
  void updateJoints(const TMatrix &local_ref_space_tm, VrInput::Hands side);

  // We can fine tune squeeze/point axes in the game code
  void setState(HandsState &&state);

  VrHands::FingerStates getFingerStates(VrInput::Hands side) const { return fingerStates[side]; }
  float getGrabRadius() const { return grabRadius; }

  // These xforms are in local_ref_space_tm, as that is what we currently need most of the time
  TMatrix getGrabTm(VrInput::Hands side) const { return grabTms[side]; }
  TMatrix getPalmTm(VrInput::Hands side) const { return palmTms[side]; }
  TMatrix getVisualIndexFingertipTm(VrInput::Hands side) const { return visualIndexFingertipTms[side]; }
  TMatrix getRealIndexFingertipTm(VrInput::Hands side) const { return realIndexFingertipTms[side]; }

  void drawDebugCollision() const;

private:
  void initHand(const char *model_name, VrInput::Hands side);

  void initAnimPhys(VrInput::Hands side, AnimV20::AnimcharBaseComponent &animchar_component);
  void updateAnimPhys(VrInput::Hands side, AnimV20::AnimcharBaseComponent &animchar_component);
  int addAnimPhysVar(AnimV20::AnimcharBaseComponent &animchar_component, VrInput::Hands side, const char *name, float def_val);
  void setAnimPhysVarVal(VrInput::Hands side, int var_id, float val);
  void updateFingerStates(VrInput::Hands side);
  void updateAttachment(VrInput::Hands side, const TMatrix &ref_rot_tm, const bool is_fold_forced);
  void convertDistalToFingertipTm(TMatrix &in_out_tm);

private:
  float grabRadius = 0.f;
  float collisionRadius = 0.04f;
  float fingerCollisionRadius = 0.01f;
  float fingertipLength = 0.017f;
  AnimCharV20::IAnimCharacter2 *animChar[VrInput::Hands::Total] = {nullptr, nullptr};
  AnimatedPhys *animPhys[VrInput::Hands::Total] = {nullptr, nullptr};
  PhysVars *physVars[VrInput::Hands::Total] = {nullptr, nullptr};

  int indexFingerAnimVarId[VrInput::Hands::Total] = {-1, -1};
  int thumbAnimVarId[VrInput::Hands::Total] = {-1, -1};
  int fingersAnimVarId[VrInput::Hands::Total] = {-1, -1};

  HandsState handsState;
  FingerStates fingerStates[VrInput::Hands::Total]{};
  GeomTreeIdxMap nodeMaps[VrInput::Hands::Total];
  GeomNodeTreeJointIndices jointIndices[VrInput::Hands::Total]{};

  TMatrix palmTms[VrInput::Hands::Total]{};
  TMatrix grabTms[VrInput::Hands::Total]{};
  TMatrix visualIndexFingertipTms[VrInput::Hands::Total]{};
  TMatrix realIndexFingertipTms[VrInput::Hands::Total]{};
  Point3 handPhysPos[VrInput::Hands::Total]{};
};

} // namespace vr

ECS_DECLARE_RELOCATABLE_TYPE(vr::VrHands);
