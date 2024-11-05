//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <math/dag_interpolator.h>
#include <debug/dag_debug.h>

template <class Value, class ReadFunc>
bool read_interpolate_tab_as_blocks(InterpolateTab<Value> &tab, const DataBlock &blk, ReadFunc read)
{
  bool done = true;
  float x = 0;
  Value y = ZERO<Value>();
  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock *rowBlk = blk.getBlock(i);
    if (!read(*rowBlk, x, y))
      continue;
    if (!tab.add(x, y))
    {
      done = false;
      debug("Interpolate tab '%s', value 'x' in row %d less than in previous row", blk.getBlockName(), i);
    }
  }

  return done;
}

template <class Value, class ReadFunc>
bool read_interpolate_tab_as_params(InterpolateTab<Value> &tab, const DataBlock &blk, ReadFunc read)
{
  bool done = true;
  float x = 0;
  Value y = ZERO<Value>();
  for (int i = 0; i < blk.paramCount(); ++i)
  {
    if (!read(blk, i, x, y))
      continue;
    if (!tab.add(x, y))
    {
      done = false;
      debug("Interpolate tab '%s', value 'x' in row %d less than in previous row", blk.getBlockName(), i);
    }
  }

  return done;
}

template <class Value, class ReadXFunc, class ReadYZFunc>
bool read_interpolate_2d_tab(Interpolate2DTab<Value> &tab, const DataBlock &blk, ReadXFunc read_x, ReadYZFunc read_yz,
  const char *x_param_name)
{
  int xParamNameId = blk.getNameId(x_param_name);

  bool done = true;
  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock *yzBlk = blk.getBlock(i);
    if (!yzBlk->paramExists(xParamNameId))
    {
      done = false;
      debug("Interpolate tab '%s', not found parameter %s for value 'x'", blk.getBlockName(), x_param_name);
      continue;
    }

    float x = 0;
    if (!read_x(yzBlk, xParamNameId, x))
      continue;

    InterpolateTab<Value> *y = tab.add(x, InterpolateTab<Value>());
    if (!y)
    {
      done = false;
      debug("Interpolate tab '%s', value 'x' in row %d less than in previous row", blk.getBlockName(), i);
      continue;
    }

    if (!read_interpolate_tab(yzBlk, *y, read_yz))
    {
      done = false;
      debug("Interpolate tab '%s', read error sub-tab %s", blk.getBlockName(), yzBlk->getBlockName());
    }
  }

  return done;
}

template <class Value, class ReadXFunc, class ReadYZFunc>
bool read_interpolate_2d_tab_p2(Interpolate2DTab<Value> &tab, const DataBlock &blk, ReadXFunc read_x, ReadYZFunc read_yz,
  const char *x_param_name)
{
  int xParamNameId = blk.getNameId(x_param_name);

  bool done = true;
  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock *yzBlk = blk.getBlock(i);
    if (!yzBlk->paramExists(xParamNameId))
    {
      done = false;
      debug("Interpolate tab '%s', not found parameter %s for value 'x'", blk.getBlockName(), x_param_name);
      continue;
    }

    float x = 0;
    if (!read_x(*yzBlk, xParamNameId, x))
      continue;

    InterpolateTab<Value> *y = tab.add(x);
    if (!y)
    {
      done = false;
      debug("Interpolate tab '%s', value 'x' in row %d less than in previous row", blk.getBlockName(), i);
      continue;
    }

    for (int j = 0; j < yzBlk->paramCount(); ++j)
    {
      if (yzBlk->getParamNameId(j) == xParamNameId)
        continue;

      float yx = 0;
      Value yz = ZERO<Value>();
      read_yz(blk, j, yx, yz);

      if (!y->add(yx, yz))
      {
        done = false;
        debug("Interpolate tab '%s', value 'y' in row %d less than in previous row", yzBlk->getBlockName(), i);
      }
    }
  }

  return done;
}

template <class Value, class ReadFunc>
bool read_interpolate_2d_tab_p3(Interpolate2DTab<Value> &tab, const DataBlock &blk, ReadFunc read)
{
  bool done = true;
  float x = 0;
  InterpolateTab<Value> *y = nullptr;
  for (int i = 0; i < blk.paramCount(); ++i)
  {
    float curX = 0;
    float curY = 0;
    Value curZ = ZERO<Value>();
    if (!read(blk, i, curX, curY, curZ))
      continue;

    if (i == 0 || curX != x)
    {
      y = tab.add(x);
      if (!y)
      {
        done = false;
        debug("Interpolate tab '%s', value 'x' in row %d less than in previous row", blk.getBlockName(), i);
        continue;
      }

      x = curX;
    }

    if (!y->add(curY, curX))
    {
      done = false;
      debug("Interpolate tab '%s', value 'y' in row %d less than in previous row", blk.getBlock(i)->getBlockName(), i);
    }
  }

  return done;
}


bool read_interpolate_tab_float_p2(InterpolateTabFloat &tab, const DataBlock &blk);

bool read_interpolate_2d_tab_float_p2(Interpolate2DTabFloat &tab, const DataBlock &blk, const char *x_param_name);
bool read_interpolate_2d_tab_float_p3(Interpolate2DTabFloat &tab, const DataBlock &blk);
