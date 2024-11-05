// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiJoyData.h>
#include <generic/dag_tab.h>
#include <generic/dag_sort.h>
#include <startup/dag_inpDevClsDrv.h>
#include <ioSys/dag_dataBlock.h>


class MovieVibroPlayer
{
public:
  MovieVibroPlayer() : items(midmem) {}

  void init(const char *filename)
  {
    lastPos = 0;
    lastItem = -1;
    curMode = MODE_IMM;

    clear_and_shrink(items);
    DataBlock blk(filename);

    int itemId = blk.getNameId("line");
    for (int i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == itemId)
      {
        DataBlock *cb = blk.getBlock(i);
        SubItem item;
        item.start = cb->getInt("at", 0);
        if (cb->getBool("fade", false))
          item.mode = MODE_FADE;
        else if (cb->getBool("timed", false))
          item.mode = MODE_TIMED;
        else
          item.mode = MODE_IMM;
        item.low = cb->getReal("low", 0);
        item.high = cb->getReal("high", 0);
        item.len = cb->getInt("len", 0);
        items.push_back(item);
      }

    sort(items, &cmpSubItem);
  }

  void update(unsigned ms)
  {
    int nextItem = lastItem + 1;
    if (nextItem < items.size())
      if ((ms >= items[nextItem].start) && (lastPos < items[nextItem].start))
      {
        lastItem = nextItem;
        startPlayback(ms);
      }
    updatePlayback(ms);
    lastPos = ms;
  }

private:
  int lastPos;
  int lastItem;

  void startPlayback(unsigned ms)
  {
    SubItem &item = items[lastItem];
    if (curMode != MODE_FADE)
      ::global_cls_drv_joy->getDefaultJoystick()->doRumble(item.low, item.high);
    curMode = item.mode;
    curStart = ms;
  }
  void updatePlayback(unsigned ms)
  {
    if (lastItem < 0)
      return;
    if (curMode == MODE_IMM)
      return;

    SubItem &item = items[lastItem];
    if (ms - curStart >= item.len)
    {
      curMode = MODE_IMM;
      ::global_cls_drv_joy->getDefaultJoystick()->doRumble(0.0, 0.0);
    }
    else if (curMode == MODE_FADE)
    {
      real amp = 1.0 - float(ms - curStart) / float(item.len);
      ::global_cls_drv_joy->getDefaultJoystick()->doRumble(item.low * amp, item.high * amp);
    }
  }

  enum RumbleMode
  {
    MODE_IMM,
    MODE_FADE,
    MODE_TIMED
  };
  struct SubItem
  {
    unsigned start;
    RumbleMode mode;
    real low;
    real high;
    unsigned len;
  };
  static int cmpSubItem(const SubItem *p1, const SubItem *p2) { return p1->start - p2->start; }

  Tab<SubItem> items;

  RumbleMode curMode;
  unsigned curStart;
};
