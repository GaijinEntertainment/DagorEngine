// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#define VK_CMD_DUMP_PARAM(x)     addCmdParam(v.x, #x, (uintptr_t) & v.x - (uintptr_t) & v)
#define VK_CMD_DUMP_PARAM_PTR(x) addCmdParamPtr(v.x, #x, (uintptr_t) & v.x - (uintptr_t) & v)

class FaultReportDump
{
public:
  enum GlobalTag
  {
    TAG_PLAIN_TEXT,
    TAG_TRACKED_STATE,
    TAG_OBJECT,
    TAG_VK_HANDLE,
    TAG_MARKER,
    TAG_CMD,
    TAG_CMD_DATA,
    TAG_CMD_PARAM,
    TAG_WORK_ITEM,
    TAG_EXT_FAULT,
    TAG_CALLER_HASH,
    TAG_PIPE,
    TAG_GPU_ADDR,
    TAG_OBJ_MEM_SIZE,
    TAG_GPU_JOB_ITEM,
    TAG_GPU_EXEC_MARKER,
    TAG_CPU_EXEC_MARKER,
    TAG_GPU_JOB_HASH,
    TAG_MAX
  };

  typedef size_t RefId;

private:
  struct ExtRef
  {
    GlobalTag tag;
    uint64_t id;
  };

  struct DataElement
  {
    GlobalTag tag;
    uint64_t id;
    String txt;
    dag::Vector<ExtRef> refs;
  };

  dag::Vector<DataElement> data;
  typedef ska::flat_hash_map<uint64_t, uint64_t> RefMap;
  dag::Vector<RefMap> sortedData;
  uint64_t txtId = 0;
  size_t workItemId = 0;
  const uint64_t *callerHashes = nullptr;
  RefId currentCommand = 0;
  const void *cmdPtr = nullptr;

  // significants that define dump context, so it may be referenced from dumping logic
  // in order to give quick references for local dumped reference
  // i.e. if we dump dispatch and reference active CS right away, we give way to know needed connected data without search
  // and that is most valuable in current dump, making it human readable
  struct Significants
  {
    RefId programCS;
    RefId programGR;
  } significants{};

public:
  FaultReportDump();
  ~FaultReportDump() {}

  RefId add(String txt);
  RefId addTagged(GlobalTag tag, uint64_t id, String txt);
  void addRef(RefId dst_id, GlobalTag src_tag, uint64_t src_id);
  void dumpLog();
  bool hasEntry(GlobalTag tag, uint64_t id);

  void setCommandStreamParameters(size_t work_id, const uint64_t *caller_hashes)
  {
    workItemId = work_id;
    callerHashes = caller_hashes;
  }
  void beginCmd(const void *cmd_ptr) { cmdPtr = cmd_ptr; };
  void endCmd() {};

  template <typename T>
  void execCmd(const T &cmd);

  template <typename T>
  String paramToString(const T &v);

  template <typename T>
  String paramToStringPtr(const T *v);

  template <typename T>
  void addCmdParamPtr(const T *v, const char *name, uintptr_t offset)
  {
    String paramValue = paramToStringPtr(v);
    FaultReportDump::RefId rid =
      addTagged(FaultReportDump::GlobalTag::TAG_CMD_PARAM, (uint64_t)cmdPtr + offset, String(64, "%s = %s", name, paramValue));
    addRef(rid, FaultReportDump::GlobalTag::TAG_CMD, currentCommand);
  }

  template <typename T>
  void addCmdParam(const T &v, const char *name, uintptr_t offset)
  {
    String paramValue = paramToString(v);
    FaultReportDump::RefId rid =
      addTagged(FaultReportDump::GlobalTag::TAG_CMD_PARAM, (uint64_t)cmdPtr + offset, String(64, "%s = %s", name, paramValue));
    addRef(rid, FaultReportDump::GlobalTag::TAG_CMD, currentCommand);
  }

  template <typename T>
  void dumpCmd(const T &);
};
