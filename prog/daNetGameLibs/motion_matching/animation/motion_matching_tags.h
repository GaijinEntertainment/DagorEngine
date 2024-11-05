// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/bitset.h>

// 32 is enough for now. Can be increased in future
// need to use bitset for multiplatform pop_count
using Tags = eastl::bitset<32>;

struct AnimationFilterTags
{
  Tags selected;
  Tags selectedMask;
  bool wasChanged = false;

  void requireTag(uint32_t tag_idx)
  {
    selected.set(tag_idx, true);
    selectedMask.set(tag_idx, true);
    wasChanged = true;
  }
  void excludeTag(uint32_t tag_idx)
  {
    selected.set(tag_idx, false);
    selectedMask.set(tag_idx, true);
    wasChanged = true;
  }
  void removeTag(uint32_t tag_idx)
  {
    selected.set(tag_idx, false);
    selectedMask.set(tag_idx, false);
    wasChanged = true;
  }
  bool isTagRequired(uint32_t tag_idx) const { return selectedMask.test(tag_idx) && selected.test(tag_idx); }
  bool isTagExcluded(uint32_t tag_idx) const { return selectedMask.test(tag_idx) && !selected.test(tag_idx); }
  bool isMatch(const Tags &clip_tags) const { return (clip_tags & selectedMask) == selected; }
  AnimationFilterTags() = default;
  AnimationFilterTags(AnimationFilterTags &&) = default;
  AnimationFilterTags &operator=(const AnimationFilterTags &tags)
  {
    if (selectedMask == tags.selectedMask && selected == tags.selected)
      return *this;
    selectedMask = tags.selectedMask;
    selected = tags.selected;
    wasChanged = true;
    return *this;
  }
};
