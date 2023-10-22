#include <render/gpuVisibilityTest.h>
#include <util/dag_string.h>
#include <3d/dag_drvDecl.h>
#include <perfMon/dag_statDrv.h>
#include <memory/dag_framemem.h>
#include <math/dag_frustum.h>
#include <3d/dag_quadIndexBuffer.h>

static constexpr int BBOX_VIS_RESULT_REG_ID = 1;
static constexpr int MAX_TESTS_IN_QUEUE = 128 << 10; // 8Mb

template <typename VectorTy>
static void safeCopyToEnd(VectorTy &arr, int firstPos, int lastPos)
{
  int prevSize = arr.size();
  G_ASSERT(0 <= firstPos && firstPos <= lastPos && lastPos <= prevSize);
  arr.resize(prevSize + lastPos - firstPos);
  eastl::copy(arr.begin() + firstPos, arr.begin() + lastPos, arr.begin() + prevSize);
}

GpuVisibilityTestManager::GpuVisibilityTestManager(shaders::OverrideState state)
{
  bboxesSBuf = dag::buffers::create_one_frame_sr_structured(sizeof(vec4f), MAX_OBJECTS_PER_FRAME * 2, "ri_bboxes_buf");

  // One bit (visible or not) per one instance.
  visResultRingBuffer.init(sizeof(uint32_t), MAX_OBJECTS_PER_FRAME / 32, 3, "bbox_vis_readback_buf", SBCF_UA_STRUCTURED_READBACK, 0,
    false);

  bboxRenderer.init("bbox_occlusion", nullptr, 0, "bbox_occlusion");

  bool hasConservativeRasterization = d3d::get_driver_desc().caps.hasConservativeRassterization;
  if (hasConservativeRasterization)
    state.set(shaders::OverrideState::CONSERVATIVE);
  visTestOverrideId = shaders::overrides::create(state);

  index_buffer::init_box();
}

GpuVisibilityTestManager::~GpuVisibilityTestManager()
{
  visResultRingBuffer.close();
  index_buffer::release_box();
}

void GpuVisibilityTestManager::addObjectGroupToTest(dag::ConstSpan<TestObjectData> objects, const ViewTransformData &view_transform)
{
  int count = getObjectsToTestCount(objects.size());
  if (objects.empty() || count <= 0)
    return;
  int startObjectId = objectsToTest.size();
  objectsToTest.insert(objectsToTest.end(), objects.cbegin(), min(objects.cend(), objects.cbegin() + count));
  objectGroups.push_back(GroupInfo{view_transform, startObjectId});
}

int GpuVisibilityTestManager::getAvailableSpace() const { return MAX_TESTS_IN_QUEUE - objectsToTest.size(); }

void GpuVisibilityTestManager::getVisibilityTestResults(
  const eastl::function<void(bool /*isVisible*/, const TestObjectData & /*testedObject*/)> &tested_fn)
{
  if (d3d::device_lost(nullptr))
    return;

  int curObjectToRead = 0;
  int testId = 0;
  int stride;
  uint32_t readbackFrame;
  while (uint32_t *data = (uint32_t *)(visResultRingBuffer.lock(stride, readbackFrame, false)))
  {
    if (testId < curTestsIssued.size() && curTestsIssued[testId].testFrame <= readbackFrame)
    {
      while (testId < curTestsIssued.size() && curTestsIssued[testId].testFrame < readbackFrame)
      {
        logwarn("Visibility test for %d bboxes failed at frame %d (current frame to read %d)", curTestsIssued[testId].objCount,
          curTestsIssued[testId].testFrame, readbackFrame);
        reAddObjectsToTest(curObjectToRead, curTestsIssued[testId].objCount);
        curObjectToRead += curTestsIssued[testId].objCount;
        ++testId;
      }
      if (testId < curTestsIssued.size())
      {
        TIME_PROFILE(visibility_test_results_read);
        for (int i = 0; i < curTestsIssued[testId].objCount; ++i)
          tested_fn(data[i / 32] & (1 << (i & 31)), objectsToTest[curObjectToRead + i]);

        curObjectToRead += curTestsIssued[testId].objCount;
        ++testId;
      }
    }
    visResultRingBuffer.unlock();
  }
  curTestsIssued.erase(curTestsIssued.begin(), curTestsIssued.begin() + testId);
  cleanTestedData(curObjectToRead);
}

void GpuVisibilityTestManager::doTests(BaseTexture *depth_tex)
{
  if (d3d::device_lost(nullptr))
    return;

  int testFrame;
  Sbuffer *resultBuffer = (Sbuffer *)visResultRingBuffer.getNewTarget(testFrame);
  if (!resultBuffer)
    return;

  // Just process each frame the maximum possible amount of bboxes from the queue.
  // We can have some already tested bboxes on the input. It occurs when they wasn't read.
  // So here is curObjectToTestId var to handle it.
  int bboxesToTestCnt = min(int(objectsToTest.size() - curObjectToTestId), int(MAX_OBJECTS_PER_FRAME));
  if (bboxesToTestCnt)
  {
    TIME_D3D_PROFILE(bboxes_visibility_test);

    updateBboxesSbuf(curObjectToTestId, bboxesToTestCnt);

    int startObjectId = curObjectToTestId;
    int endObjectId = curObjectToTestId + bboxesToTestCnt;

    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;
    int x, y, w, h;
    float minz, maxz;
    d3d::getview(x, y, w, h, minz, maxz);

    d3d::set_render_target();
    d3d::set_render_target(0, (Texture *)NULL, 0);
    int levels = 0;
    if (depth_tex->restype() == RES3D_ARRTEX)
    {
      TextureInfo arrayInfo;
      depth_tex->getinfo(arrayInfo);
      levels = arrayInfo.a;
    }
    if (levels == 0)
      d3d::set_depth(depth_tex, DepthAccess::SampledRO);
    else
      d3d::set_depth(depth_tex, 0, DepthAccess::SampledRO);
    d3d::resource_barrier({depth_tex, RB_RO_CONSTANT_DEPTH_STENCIL_TARGET | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
    uint32_t zeroes[4] = {0, 0, 0, 0};
    d3d::clear_rwbufi(resultBuffer, zeroes);
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_PS, BBOX_VIS_RESULT_REG_ID, VALUE), resultBuffer);
    shaders::overrides::set(visTestOverrideId);

    d3d::resource_barrier({resultBuffer, RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});

    index_buffer::use_box();
    d3d::setvsrc(0, 0, 0);
    int currentLevel = 0;
    while (curObjectToTestId < endObjectId)
    {
      int nextStartObjectId =
        curGroupId + 1 >= objectGroups.size() ? objectsToTest.size() : objectGroups[curGroupId + 1].startObjectId;
      int groupEndObjectId = min(nextStartObjectId, endObjectId);

      GroupInfo &info = objectGroups[curGroupId];
      if (info.view.layer != currentLevel && levels > 0) // probably, we need to sort objectGroups by layer
      {
        G_ASSERTF_CONTINUE(uint32_t(info.view.layer) < levels, "target texture has %d slices, and requested to test %d slice", levels,
          info.view.layer);
        d3d::set_depth(depth_tex, currentLevel = min(info.view.layer, levels - 1), DepthAccess::SampledRO);
      }
      d3d::setview(info.view.x, info.view.y, info.view.w, info.view.h, info.view.minz, info.view.maxz);
      d3d::setglobtm(info.view.globtm);
      bboxRenderer.shader->setStates(0, true); // we only have to set it because we set matrices

      uint32_t instCnt = groupEndObjectId - curObjectToTestId;
      uint32_t startInst = curObjectToTestId - startObjectId;
      d3d::set_immediate_const(STAGE_VS, &startInst, 1);
      d3d::drawind_instanced(PRIM_TRILIST, 0, 12, 0, instCnt);

      if (nextStartObjectId <= endObjectId)
        ++curGroupId;
      curObjectToTestId = groupEndObjectId;
    }

    d3d::set_immediate_const(STAGE_VS, nullptr, 0);
    shaders::overrides::reset();
    d3d::setview(x, y, w, h, minz, maxz);
    curTestsIssued.push_back(TestInfo{bboxesToTestCnt, testFrame});
    d3d::resource_barrier({depth_tex, RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
  }

  TIME_PROFILE(visibility_tests_copy);
  visResultRingBuffer.startCPUCopy();
}

void GpuVisibilityTestManager::clearObjectsToTest()
{
  objectsToTest.clear();
  objectGroups.clear();
  resetIssuedTests();
}

void GpuVisibilityTestManager::resetIssuedTests()
{
  curObjectToTestId = curGroupId = 0;
  curTestsIssued.clear();
}

void GpuVisibilityTestManager::reAddObjectsToTest(int start_obj_id, int count)
{
  count = getObjectsToTestCount(count);
  if (count <= 0)
    return;
  int startGroupId = findGroupByObjId(start_obj_id);
  int endGroupId = findGroupByObjId(start_obj_id + count - 1);
  int prevGroupsSize = objectGroups.size();
  safeCopyToEnd(objectGroups, startGroupId, endGroupId + 1);
  for (int groupId = prevGroupsSize; groupId < objectGroups.size(); ++groupId)
    objectGroups[groupId].startObjectId += int(objectsToTest.size() - start_obj_id);
  objectGroups[prevGroupsSize].startObjectId = objectsToTest.size();
  safeCopyToEnd(objectsToTest, start_obj_id, start_obj_id + count);
}

void GpuVisibilityTestManager::updateBboxesSbuf(int start_obj_id, int count)
{
  TestObjectData *data;
  if (bboxesSBuf.getBuf()->lock(0, count * sizeof(TestObjectData), (void **)&data, VBLOCK_DISCARD | VBLOCK_WRITEONLY))
  {
    auto startObj = objectsToTest.begin() + start_obj_id;
    eastl::copy(startObj, startObj + count, data);
    bboxesSBuf.getBuf()->unlock();
  }
}

void GpuVisibilityTestManager::cleanTestedData(int until_id)
{
  if (objectGroups.size()) // If there aren't groups, then there aren't objectsToTest too.
  {
    objectsToTest.erase(objectsToTest.begin(), objectsToTest.begin() + until_id);
    curObjectToTestId -= until_id;

    for (auto &group : objectGroups)
      group.startObjectId -= until_id;
    int groupId = findGroupByObjId(0);
    objectGroups.erase(objectGroups.begin(), objectGroups.begin() + groupId);
    curGroupId -= groupId;
  }
}

int GpuVisibilityTestManager::findGroupByObjId(int obj_id)
{
  if (obj_id >= objectsToTest.size())
    return objectGroups.size();
  int groupId = -1;
  while (groupId + 1 < objectGroups.size() && objectGroups[groupId + 1].startObjectId <= obj_id)
    ++groupId;
  return groupId;
}

int GpuVisibilityTestManager::getObjectsToTestCount(int original_count)
{
  int count = min(original_count, MAX_TESTS_IN_QUEUE - int(objectsToTest.size()));
  if (count < original_count)
    logwarn("%d visibility tests can't be proceed due to the queue is full", original_count - count);
  return count;
}