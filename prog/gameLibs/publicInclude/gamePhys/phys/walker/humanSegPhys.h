//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point3.h>

enum SegPhysCheckType
{
  SEGPHYS_ON_INVALID,
  SEGPHYS_ON_ALWAYS,            // no check, must always succeed
  SEGPHYS_ON_IS_CLIMB_THRU,     // check if climbing isClimbThrough == 'true'
  SEGPHYS_ON_CLIMB_CHECK_FLOOR, // trace floor down <= transition.len
};
struct SegPhysTransition
{
  bool inv = false;
  SegPhysCheckType on = SEGPHYS_ON_INVALID;
  float len = 0.f;
  int to = -1;

  // displacement (optional origin correction after transition happens)
  Point3 disp = Point3::ZERO;
};

struct SegPhysTrajectoryPoint
{
  float t = 0.f; // time of trajectory, should start with 0
  float h = 1.f; // human height, -1 = prone, 0 = crouch, 1 = stand
  Point3 p;
};

enum SegPhysSegmentType
{
  SEGPHYS_INVALID,
  SEGPHYS_END,  // forceably finish segmented physics
  SEGPHYS_PASS, // execute instantly i.e. resolve transition at same tick (avoid loops!)
  SEGPHYS_WAIT, // wait for currTime >= segment.maxTime before resolving transition

  // threshold = enough distance
  // maxTime = apply velocity max time
  // endTime = just move to pos time if max time out
  SEGPHYS_CLIMB_MOVE_TO_PULL_UP_POS,

  // speedCoef = time speed along timed trajectory
  SEGPHYS_CLIMB_MOVE_BY_TRAJECTORY,
  SEGPHYS_CLIMB_END,
};
struct SegPhysSegment
{
  eastl::string name;
  SegPhysSegmentType type = SEGPHYS_INVALID;

  // Use animID to index into anims[] array in SegmentedHumanPhysics
  // also use it to remap animID to custom indices for AnimTree/etc.
  // if -1 no special animation assigned to this segment
  int animID = -1;

  // These parameters could be used by any segment type if needed
  // default values correspond to "no special effects" variant
  float speedCoef = 1.f;
  float threshold = 0.f;
  float maxTime = 0.f;
  float endTime = 0.f;
  Point3 offset = Point3::ZERO;

  // Trajectory of timed point should only be used for specific
  // segment types i.e. SEGPHYS_CLIMB_MOVE_BY_TRAJECTORY and alike
  eastl::vector<SegPhysTrajectoryPoint> trajectory;

  // GUIDELINE: When out of transitions in current segment we should
  // instead finish execution of segmented physics (make currSeg -1)
  eastl::vector<SegPhysTransition> transitions;
};

struct SegmentedHumanPhysics
{
  bool isLoaded = false;
  int reloadVersion = 0;

  eastl::vector<SegPhysSegment> segs;
  eastl::vector<eastl::string> anims;

  int initSeg_performClimb = -1;

  bool LoadFromTemplate(const DataBlock &blk);
};
struct SegmentedHumanPhysicsState
{
  int prevSeg = -1;
  int currSeg = -1;
  float prevTime = 0.f;
  float currTime = 0.f;
  float prevDuration = 0.f;
  float currDuration = 0.f;
  Point3 currFromPos = Point3::ZERO;
};
