// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fault_report.h"

FaultReportDump::FaultReportDump() { sortedData.resize(TAG_MAX); }

FaultReportDump::RefId FaultReportDump::add(String txt)
{
  ++txtId;
  return addTagged(GlobalTag::TAG_PLAIN_TEXT, txtId, txt);
}

FaultReportDump::RefId FaultReportDump::addTagged(GlobalTag tag, uint64_t id, String txt)
{
  G_ASSERTF(tag < GlobalTag::TAG_MAX, "vulkan: wrong tag specified for fault report");

  RefId ret = data.size();
  data.push_back({tag, id, txt});

  sortedData[tag][id] = ret;

  return ret;
}

void FaultReportDump::addRef(RefId dst_id, GlobalTag src_tag, uint64_t src_id) { data[dst_id].refs.push_back({src_tag, src_id}); }

void FaultReportDump::dumpLog()
{
  debug("vulkan: fault dump start");
  // trick: as we do stream-like additions, dump data unsorted to make it somewhat readable
  size_t idx = 0;
  for (const DataElement &iter : data)
  {
    // small marker to conserve log space, but keep filterable
    // elevate error level for production significant crash dump analysis
    if (iter.tag == TAG_GPU_EXEC_MARKER || iter.tag == TAG_CPU_EXEC_MARKER || iter.tag == TAG_GPU_JOB_HASH)
      logwarn("VFD#%06lXv%02u-%016llX %s", idx, iter.tag, iter.id, iter.txt);
    else
      debug("VFD#%06lXv%02u-%016llX %s", idx, iter.tag, iter.id, iter.txt);
    for (const ExtRef &refIter : iter.refs)
    {
      auto findResult = sortedData[refIter.tag].find(refIter.id);
      if (findResult == sortedData[refIter.tag].end())
        debug("VFD#%06lXr%02u-%016llX %02u-%016llX #broken#", idx, iter.tag, iter.id, refIter.tag, refIter.id);
      else
        debug("VFD#%06lXr%02u-%016llX %02u-%016llX #%06lX", idx, iter.tag, iter.id, refIter.tag, refIter.id, findResult->second);
    }
    ++idx;
  }
  debug("vulkan: fault dump end");
}

bool FaultReportDump::hasEntry(GlobalTag tag, uint64_t id)
{
  auto findResult = sortedData[tag].find(id);
  return findResult != sortedData[tag].end();
}