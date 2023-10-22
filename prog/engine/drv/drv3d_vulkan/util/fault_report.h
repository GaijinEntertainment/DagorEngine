#pragma once
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include <ska_hash_map/flat_hash_map2.hpp>

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

public:
  FaultReportDump();
  ~FaultReportDump() {}

  RefId add(String txt);
  RefId addTagged(GlobalTag tag, uint64_t id, String txt);
  void addRef(RefId dst_id, GlobalTag src_tag, uint64_t src_id);
  void dumpLog();
  bool hasEntry(GlobalTag tag, uint64_t id);
};
