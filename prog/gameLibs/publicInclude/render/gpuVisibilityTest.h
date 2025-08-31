//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_bounds3.h>
#include <math/dag_Point4.h>
#include <shaders/dag_overrideStates.h>
#include <3d/dag_resPtr.h>
#include <EASTL/deque.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <3d/dag_ringCPUQueryLock.h>

#include "viewTransformData.h"

// Implements functionality for testing bboxes visibility
// by rasterizing them into provided depth texture. (without depth writing)
// When we pass z-test (at least at one pixel) for a bbox instance,
// we set related bit to 1. Then readback this result to cpu
// (which takes >= 1 frame time).
// Since there is a limit for tests per one frame, we create a queue
// and each frame take from it maximum amount of bboxes to test.
class GpuVisibilityTestManager
{
  struct GroupInfo
  {
    ViewTransformData view;
    int startObjectId; // Start index in objects queue.
  };

  struct TestInfo
  {
    int objCount;
    int testFrame;
  };

public:
  static constexpr int MAX_OBJECTS_PER_FRAME = 8192;
  using TestObjectData = bbox3f; // w components of bmin and bmax aren't touched,
                                 // so they can be used for storing user data.

  GpuVisibilityTestManager(shaders::OverrideState state);
  ~GpuVisibilityTestManager();

  void addObjectGroupToTest(dag::ConstSpan<TestObjectData> objects, const ViewTransformData &view_transform);
  int getAvailableSpace() const;
  void reserveSpaceForObjectsGroup(int expected_objects_count)
  {
    G_ASSERT(reservedSpaceForTestObjects == 0 && getAvailableSpace() >= expected_objects_count);
    reservedSpaceForTestObjects = expected_objects_count;
  }
  void clearReservedSpace() { reservedSpaceForTestObjects = 0; }

  // The following methods should be called every frame.
  // If there aren't any objects in the queue, just nothing will happen.
  void getVisibilityTestResults(const eastl::function<void(bool /*isVisible*/, const TestObjectData & /*testedObject*/)> &tested_fn);
  // Caller is responsible for saving,restoring Render Target, Viewport, Globtm states
  void doTestsOnGpu(Texture *tex);
  // Remove groups with objects that can't be tested by current depth tex
  void removeOutdatedObjectGroups(
    const eastl::fixed_function<sizeof(void *) * 2, bool(const ViewTransformData & /*group_view*/)> &is_group_outdated);

  // Clears all objects added for testing, including already issued.
  void clearObjectsToTest();

  inline void afterDeviceReset() { resetIssuedTests(); }

protected:
  void resetIssuedTests();

  // Just copies existing objects to the end of the queue
  // and makes related object groups (needed in case of test failure).
  void reAddObjectsToTest(int start_obj_id, int count);
  void updateBboxesSbuf(int start_obj_id, int count);

  void cleanTestedData(int until_id);
  int findGroupByObjId(int obj_id);
  int getObjectsToTestCount(int original_count);

  UniqueBufHolder bboxesSBuf;
  RingCPUBufferLock visResultRingBuffer;
  DynamicShaderHelper bboxRenderer;
  shaders::UniqueOverrideStateId visTestOverrideId;

  // Set subarray size to MAX_OBJECTS_PER_FRAME is a good tradeoff between
  // amount of memory consumption and amount of deallocations on erasing processed elements.
  // In most cases there will be exactly one deallocation as almost always number of
  // obejects to remove will be <= MAX_OBJECTS_PER_FRAME (and mostly ==).
  // (with default size the time for erasing is ~10 times slower in most cases)
  eastl::deque<TestObjectData, EASTLAllocatorType, MAX_OBJECTS_PER_FRAME> objectsToTest;
  eastl::deque<GroupInfo> objectGroups;
  int curObjectToTestId = 0;
  int curGroupId = 0;
  int reservedSpaceForTestObjects = 0;
  eastl::deque<TestInfo> curTestsIssued;

  bool deviceIsResetting = false;
};
