//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/array.h>
#include <animChar/dag_animCharacter2.h>
#include <math/dag_geomTree.h>
#include <math/dag_geomTreeMap.h>
#include <humanInput/dag_hiVrInput.h>
#include <daECS/core/componentType.h>

class AnimatedPhys;
class PhysVars;
class TexStreamingContext;

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
  };
  using HandsState = eastl::array<OneHandState, VrInput::Hands::Total>;

  void clear();

  // You have 2 possible interaction with this class: it may hold IAnimCharacter2 or you can store
  // animchar outside the class (in ECS)
  // If you want to store animchar in the class, use this methods:
  void init(const char *model_name_l, const char *model_name_r, const char *index_tip_l, const char *index_tip_r);
  // See Local Reference Space in OpenXR documentation
  // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#reference-spaces
  void update(float dt, const TMatrix &local_ref_space_tm, HandsState &&hands);
  void beforeRender(const Point3 &cam_pos);
  void render(TexStreamingContext texCtx);

  // If you want to store data in ecs, use this functions:
  void init(AnimV20::AnimcharBaseComponent &left_char, AnimV20::AnimcharBaseComponent &right_char);
  void updateHand(const TMatrix &local_ref_space_tm, VrInput::Hands side, AnimV20::AnimcharBaseComponent &animchar_component,
    TMatrix &tm);

  // We can fine tune squeeze/point axes in the game code
  void setState(HandsState &&state);

  TMatrix getPalmTm(VrInput::Hands side) const;
  TMatrix getIndexFingertipWtm(VrInput::Hands side) const;

private:
  void initHand(const char *model_name, VrInput::Hands side);

  void initAnimPhys(VrInput::Hands side, AnimV20::AnimcharBaseComponent &animchar_component);
  void updateAnimPhys(VrInput::Hands side, AnimV20::AnimcharBaseComponent &animchar_component);
  int addAnimPhysVar(AnimV20::AnimcharBaseComponent &animchar_component, VrInput::Hands side, const char *name, float def_val);
  void setAnimPhysVarVal(VrInput::Hands side, int var_id, float val);

private:
  AnimCharV20::IAnimCharacter2 *animChar[VrInput::Hands::Total] = {nullptr, nullptr};
  AnimatedPhys *animPhys[VrInput::Hands::Total] = {nullptr, nullptr};
  PhysVars *physVars[VrInput::Hands::Total] = {nullptr, nullptr};

  int indexFingerAnimVarId[VrInput::Hands::Total] = {-1, -1};
  int thumbAnimVarId[VrInput::Hands::Total] = {-1, -1};
  int fingersAnimVarId[VrInput::Hands::Total] = {-1, -1};

  HandsState handsState;
  GeomTreeIdxMap nodeMaps[VrInput::Hands::Total];
  GeomNodeTree::Index16 indexFingerTipsIndices[VrInput::Hands::Total];
};

} // namespace vr

ECS_DECLARE_RELOCATABLE_TYPE(vr::VrHands);
