//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <debug/dag_assert.h>
#include <debug/dag_log.h>
#include <util/dag_fixedBitArray.h>

class LayerHiddenMask
{
public:
  LayerHiddenMask() = default;
  LayerHiddenMask(int index, bool hidden) { setHidden(index, hidden); }

  bool isHidden(int index) const
  {
    if (index < 0 || index >= BIT_COUNT) [[unlikely]]
    {
      logerr("Invalid index in LayerHiddenMask::isHidden(%d).", index);
      // Return the reserved, always hidden layer. See IObjEntityFilter::setLayerHiddenMask() in srvEngine.cpp.
      return value.get(BIT_COUNT - 1);
    }

    return value.get(index);
  }

  void setVisible(int index)
  {
    if (index < 0 || index >= BIT_COUNT) [[unlikely]]
    {
      logerr("Invalid index in LayerHiddenMask::setVisible(%d).", index);
      return;
    }

    value.reset(index);
  }

  void setHidden(int index)
  {
    if (index < 0 || index >= BIT_COUNT) [[unlikely]]
    {
      logerr("Invalid index in LayerHiddenMask::setHidden(%d).", index);
      return;
    }

    value.set(index);
  }

  void setHidden(int index, bool hidden)
  {
    if (hidden)
      setHidden(index);
    else
      setVisible(index);
  }

  void setAllHidden() { memset(&value, 0xff, sizeof(value)); }

  bool operator==(const LayerHiddenMask &other) const { return memcmp(&value, &other.value, sizeof(value)) == 0; }
  bool operator!=(const LayerHiddenMask &other) const { return !(*this == other); }

  static constexpr int BIT_COUNT = 128;

private:
  FixedBitArray<BIT_COUNT> value;
  G_STATIC_ASSERT((sizeof(value) * 8) == BIT_COUNT);
};
