// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "query.h"
#include "backend/context.h"
#include "util/fault_report.h"
#include "wrapped_command_buffer.h"
#include "execution_sync.h"

using namespace drv3d_vulkan;

TSPEC void BEContext::execCmd(const CmdBeginSurvey &cmd)
{
  beginCustomStage("BeginSurvey");
  Backend::cb.wCmdResetQueryPool(cmd.pool, cmd.index, 1);
}

TSPEC void BEContext::execCmd(const CmdEndSurvey &cmd)
{
  beginCustomStage("EndSurvey");

  const uint32_t stride = sizeof(uint32_t);
  const uint32_t count = 1;
  uint32_t ofs = cmd.buffer->bufOffsetLoc(cmd.index * stride);
  uint32_t sz = stride * count;

  // do not copy if there was no writes to query, as copy will crash on some GPUs in this situation
  if (directDrawCountInSurvey == 0)
  {
    // fill zero instead
    fillBuffer(cmd.buffer, ofs, sz, 1);
    return;
  }

  verifyResident(cmd.buffer);
  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, cmd.buffer, {ofs, sz});
  Backend::sync.completeNeeded();

  VULKAN_LOG_CALL(
    Backend::cb.wCmdCopyQueryPoolResults(cmd.pool, cmd.index, count, cmd.buffer->getHandle(), ofs, sz, VK_QUERY_RESULT_WAIT_BIT));

  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_HOST_READ_BIT}, cmd.buffer, {ofs, sz});
  Backend::sync.completeNeeded();
}

TSPEC void BEContext::execCmd(const CmdInsertTimesampQuery &)
{
  QueryBlock &qb = *data->timestampQueryBlock;

  if (frameCoreQueue != DeviceQueueType::GRAPHICS)
  {
    qb.dataIndexes.push_back(qb.data.size() - 1);
    return;
  }

  if (actionIdx > lastTimestampActionIdx || qb.data.empty())
  {
    VULKAN_LOG_CALL(Backend::cb.wCmdWriteTimestamp(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, qb.pool, qb.data.size()));
    qb.data.push_back(0);
  }
  qb.dataIndexes.push_back(qb.data.size() - 1);
  lastTimestampActionIdx = actionIdx;
}

TSPEC void BEContext::execCmd(const CmdStartOcclusionQuery &cmd)
{
  QueryBlock &qb = *data->occlusionQueryBlock;

  G_UNUSED(cmd);
  G_ASSERTF(frameCoreQueue == DeviceQueueType::GRAPHICS, "vulkan: occlusion query begin on non graphics queue, from %s",
    getCurrentCmdCaller());
  G_ASSERTF(cmd.queryIndex == qb.data.size(), "vulkan: occlusion query index %u is invalid, some queries are skipped, from %s",
    getCurrentCmdCaller());

  beginCustomStage("BeginOcclusionQuery");
  VULKAN_LOG_CALL(Backend::cb.wCmdBeginQuery(qb.pool, qb.data.size(), 0));
  // dataIndexes are unused now, but possible work in this direction may done to support cross command buffer queries
  qb.dataIndexes.push_back(qb.data.size());
  qb.data.push_back(0);
}

TSPEC void BEContext::execCmd(const CmdEndOcclusionQuery &cmd)
{
  QueryBlock &qb = *data->occlusionQueryBlock;

  G_ASSERTF(frameCoreQueue == DeviceQueueType::GRAPHICS, "vulkan: occlusion query end on non graphics queue, from %s",
    getCurrentCmdCaller());

  beginCustomStage("EndOcclusionQuery");
  VULKAN_LOG_CALL(Backend::cb.wCmdEndQuery(qb.pool, cmd.queryIndex));
}
