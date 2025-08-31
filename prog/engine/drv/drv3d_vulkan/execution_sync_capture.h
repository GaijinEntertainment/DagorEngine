// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// class to capture sync and accesses that generated and tracked by execution sync tracker

#include "execution_sync.h"
#include <generic/dag_tab.h>

namespace drv3d_vulkan
{

struct ExecutionSyncCapture
{
  struct SyncStep
  {
    uint32_t visNode;
    uint32_t visBufPin;
    uint32_t visOpsPin;
    SyncOpCaller caller;
    uint32_t dstBuffer;
    uint32_t dstQueue;
    uint32_t idx;
    uint32_t visIndepSeq;
    uint32_t visIdepSeqPin;
    uint32_t visPrevDepPin;
    uint32_t visNextDepPin;
    uint32_t orderLevel;
    uint32_t orderPos;
    uint32_t invalidOrdering;
  };

  struct SyncOp
  {
    uint32_t visNode;
    uint32_t visStepPin;
    uint32_t visSrcPin;
    uint32_t visDstPin;
    uint32_t uid;
    uint32_t step;
    uint32_t localIdx;
    LogicAddress laddr;
    Resource *res;
    SyncOpCaller caller;
    String resName;
    uint32_t noLink;
  };

  struct SyncOpLink
  {
    uint32_t srcOpUid;
    uint32_t dstOpUid;
  };

  struct SyncBufferLink
  {
    uint32_t srcQueue;
    uint32_t srcBuffer;
    uint32_t dstQueue;
    uint32_t dstBuffer;
  };

  struct IndepSeq
  {
    uint32_t visIndepSeq;
    uint32_t visBufPin;
    uint32_t cat;
  };

  struct NodePos
  {
    uint32_t x;
    uint32_t y;
  };
#if EXECUTION_SYNC_DEBUG_CAPTURE > 0
  void addSyncStep();
  void addOp(SyncOpUid uid, LogicAddress laddr, Resource *res, SyncOpCaller caller);
  void addOpPrevStep(SyncOpUid uid, LogicAddress laddr, Resource *res, SyncOpCaller caller)
  {
    addOp(uid, laddr, res, caller);
    --ops.back().step;
  }
  void addLink(SyncOpUid src_op_uid, SyncOpUid dst_op_uid);
  void reset();

  Tab<SyncOp> ops;
  Tab<SyncStep> steps;
  Tab<SyncOpLink> links;
  Tab<NodePos> reformedPositions;
  Tab<IndepSeq> indepSeqs;
  Tab<SyncBufferLink> bufferLinks;
  uint32_t currentSyncStep = 0;
  uint32_t currentVisNode = 1;
  uint32_t currentLocalSyncStepOpIdx = 0;
  uint32_t currentVisPin = 1;
#else
  void addSyncStep() {}
  void addOp(SyncOpUid, LogicAddress, Resource *, SyncOpCaller) {}
  void addOpPrevStep(SyncOpUid, LogicAddress, Resource *, SyncOpCaller) {}
  void addLink(SyncOpUid, SyncOpUid) {}
  void reset() {}
#endif
};

} // namespace drv3d_vulkan
