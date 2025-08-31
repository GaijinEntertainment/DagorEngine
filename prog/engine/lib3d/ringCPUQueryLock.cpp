// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_resMgr.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_query.h>
#include <stdio.h>
#include <util/dag_baseDef.h>

void RingCPUBufferLock::close()
{
  for (int i = 0; i < buffers.size(); ++i)
  {
    release_managed_res_verified(buffers[i].id, buffers[i].gpu);
    buffers[i].gpu = 0;
    d3d::release_event_query(buffers[i].event);
    buffers[i].event = 0;
  }
  buffers.clear();
}

void RingCPUBufferLock::init(uint32_t element_size, uint32_t elements, int buffers_count, const char *name, uint32_t flags,
  uint32_t texfmt, bool is_texture)
{
  close();
  reset();
  buffers.resize(min<uint32_t>(buffers_count, buffers.capacity()));
  for (int i = 0; i < buffers.size(); ++i)
  {
    char cname[64];
    SNPRINTF(cname, countof(cname), "%s%d", name, i);
    if (is_texture)
      buffers[i].gpu = d3d::create_tex(NULL, element_size, elements, texfmt, 1, cname);
    else
      buffers[i].gpu = d3d::create_sbuffer(element_size, elements, flags, texfmt, name);
    buffers[i].event = d3d::create_event_query();
    buffers[i].id = register_managed_res(cname, buffers[i].gpu);
  }
  resourceName = name;
  state = NORMAL;
}

D3dResource *RingCPUBufferLock::getNewTargetAndId(uint32_t &frame, D3DRESID &id)
{
  if (!buffers.size())
    return nullptr;
  G_ASSERT_RETURN(state == NORMAL, nullptr);
  G_ASSERT_RETURN(buffers[0].gpu, nullptr);

  if (
    (currentBufferIssued + 1) % buffers.size() == (currentBufferToLock % buffers.size()) && currentBufferToLock < currentBufferIssued)
    return nullptr;

  uint32_t bufferIdx = currentBufferIssued % buffers.size();
  // after ring reset, some readback work may not yet be completed, breaking async readback conditions
  // make sure it is completed before giving target for writes
  if (!d3d::get_event_query_status(buffers[bufferIdx].event, false))
    return nullptr;

  state = NEWTARGET;
  frame = currentBufferIssued;
  currentBufferIssued++;
  id = buffers[bufferIdx].id;
  return buffers[bufferIdx].gpu;
}

void RingCPUBufferLock::startCPUCopy()
{
  G_ASSERT_RETURN(state == NEWTARGET, );
  if (!buffers.size())
    return;
  uint32_t bufferIdx = uint32_t(currentBufferIssued - 1) % buffers.size();

  int stride;
  if (lockData(bufferIdx, stride, nullptr, false))
    unlockData(bufferIdx);
  d3d::issue_event_query(buffers[bufferIdx].event);
  state = NORMAL;
}

void RingCPUBufferLock::unlockData(int buffer)
{
  if (buffers[buffer].gpu->getType() == D3DResourceType::SBUF)
    ((Sbuffer *)buffers[buffer].gpu)->unlock();
  else
    ((Texture *)buffers[buffer].gpu)->unlockimg();
}

bool RingCPUBufferLock::lockData(int buffer, int &stride, uint8_t **pdata, bool wait)
{
  if (buffers[buffer].gpu->getType() == D3DResourceType::SBUF)
    return ((Sbuffer *)buffers[buffer].gpu)->lock(0, 0, (void **)pdata, VBLOCK_READONLY | (wait ? 0 : VBLOCK_NOSYSLOCK));
  else
    return ((Texture *)buffers[buffer].gpu)
      ->lockimg((void **)pdata, stride, 0, TEXLOCK_READ | TEXLOCK_RAWDATA | (wait ? 0 : TEXLOCK_NOSYSLOCK));
}

uint8_t *RingCPUBufferLock::lock(int &stride, uint32_t &frame, bool get_to_topmost_event, int max_frames_to_flush)
{
  G_ASSERT_RETURN(state == NORMAL, nullptr);
  if (currentBufferToLock >= currentBufferIssued || !buffers.size())
    return nullptr;
  bool eventPassed = d3d::get_event_query_status(buffers[currentBufferToLock % buffers.size()].event, false);
  uint32_t regularCurrentBufferToLock = currentBufferToLock;
  // got to topmost event
  if (get_to_topmost_event)
  {
    while (eventPassed && currentBufferToLock < currentBufferIssued - 1 &&
           d3d::get_event_query_status(buffers[(currentBufferToLock + 1) % buffers.size()].event, false))
      currentBufferToLock++;
  }
  uint32_t bufferIdx = currentBufferToLock % buffers.size();
  // bool forcedFlush
  if (!eventPassed && (++bufferLockCounter) > max_frames_to_flush)
    eventPassed = d3d::get_event_query_status(buffers[bufferIdx].event, true); // trying for too long, flush

  if (eventPassed)
  {
    uint8_t *data = NULL;
    if (!lockData(bufferIdx, stride, &data, false))
    {
      if (regularCurrentBufferToLock < currentBufferToLock)
      {
        currentBufferToLock = regularCurrentBufferToLock;
        bufferIdx = currentBufferToLock % buffers.size();
        if (!lockData(bufferIdx, stride, &data, false))
        {
          bufferLockCounter = 0;
          if (currentBufferToLock < currentBufferIssued)
            currentBufferToLock++;
          int ll = d3d::is_in_device_reset_now() ? LOGLEVEL_WARN : LOGLEVEL_ERR;
          logmessage(ll, "Can't lock buffer <%d>, <%d> issued %d attempt. Resource name: %s", currentBufferToLock, currentBufferIssued,
            bufferLockCounter, resourceName.c_str());
          return nullptr;
        }
      }
      else
        return nullptr;
    }
    bufferLockCounter = 0;
    state = LOCKED;
    frame = currentBufferToLock;
    return data;
  }
  else if (bufferLockCounter > max_frames_to_flush * 2 + 1)
  {
    logwarn("Can't lock buffer <%d>, <%d> issued %d attempt", currentBufferToLock, currentBufferIssued, bufferLockCounter);
    bufferLockCounter = 0;
    if (currentBufferToLock < currentBufferIssued)
      currentBufferToLock++;
  }
  return nullptr;
}

void RingCPUBufferLock::unlock()
{
  G_ASSERT_RETURN(state == LOCKED, );
  if (!buffers.size())
    return;
  unlockData(currentBufferToLock % buffers.size());
  currentBufferToLock++;
  state = NORMAL;
}
