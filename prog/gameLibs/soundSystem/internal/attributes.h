// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace FMOD::Studio
{
class EventDescription;
}

namespace sndsys
{
struct FMODGUID;
class EventAttributes
{
  union
  {
    int value = 0;
    struct
    {
      int16_t maxDistance;
      uint8_t flags;
      int8_t labelIdx;
    };
  };

  enum
  {
    FLAG_ONESHOT = (1 << 0),
    FLAG_HAS_SUSTAIN_POINT = (1 << 1),
    FLAG_3D = (1 << 2),
    FLAG_HAS_OCCLUSION = (1 << 3),
    FLAG_IS_DELAYABLE = (1 << 4),
    FLAG_VALID = (1 << 7), // Bit to distinguish valid EventAttributes from invalid one (can be omitted if e.g. maxDistance can't be 0)
  };

  inline void setFlag(int f, bool enable)
  {
    if (enable)
      flags |= f;
    else
      flags &= ~f;
  }

  inline void setMaxDistance(float max_distance) { maxDistance = int16_t(max_distance); }
  inline void setOneshot(bool is_oneshot) { setFlag(FLAG_ONESHOT, is_oneshot); }
  inline void set3d(bool is_3d) { setFlag(FLAG_3D, is_3d); }
  inline void setHasSustainPoint(bool has_cue) { setFlag(FLAG_HAS_SUSTAIN_POINT, has_cue); }
  inline void setHasOcclusion(bool has_occlusion) { setFlag(FLAG_HAS_OCCLUSION, has_occlusion); }
  inline void setIsDelayable(bool is_delayable) { setFlag(FLAG_IS_DELAYABLE, is_delayable); }

  inline void setVisualLabel(int idx)
  {
    using label_type_t = decltype(labelIdx);
    G_STATIC_ASSERT(eastl::numeric_limits<label_type_t>::min() <= -1);
    G_ASSERTF(idx >= -1 && idx <= eastl::numeric_limits<label_type_t>::max(), "Visual label idx %d is out of bounds [%d, %d]", idx, -1,
      eastl::numeric_limits<label_type_t>::max());
    labelIdx = idx;
  }

public:
  explicit operator bool() const { return value != 0; }
  inline float getMaxDistance() const { return float(maxDistance); }

  inline bool isOneshot() const { return (flags & FLAG_ONESHOT) != 0; }
  inline bool is3d() const { return (flags & FLAG_3D) != 0; }
  inline bool hasSustainPoint() const { return (flags & FLAG_HAS_SUSTAIN_POINT) != 0; }
  inline bool hasOcclusion() const { return (flags & FLAG_HAS_OCCLUSION) != 0; }
  inline bool isDelayable() const { return (flags & FLAG_IS_DELAYABLE) != 0; }
  inline int getVisualLabelIdx() const { return labelIdx; }

  EventAttributes() = default;
  inline EventAttributes(float max_distance, bool is_oneshot, bool is_3d, bool has_sustain_point, bool has_occlusion,
    bool is_delayable, int visual_label_idx)
  {
    flags = FLAG_VALID;
    setMaxDistance(max_distance);
    setOneshot(is_oneshot);
    set3d(is_3d);
    setHasSustainPoint(has_sustain_point);
    setHasOcclusion(has_occlusion);
    setIsDelayable(is_delayable);
    setVisualLabel(visual_label_idx);
  }

  inline bool operator==(const EventAttributes &other) const { return value == other.value; }
};

bool is_event_oneshot_impl(const FMOD::Studio::EventDescription &event_description);
bool is_event_3d_impl(const FMOD::Studio::EventDescription &event_description);
bool has_sustain_point_impl(const FMOD::Studio::EventDescription &event_description);
bool has_occlusion_impl(const FMOD::Studio::EventDescription &event_description);
bool is_delayable_impl(const FMOD::Studio::EventDescription &event_description);
float get_event_max_distance_impl(const FMOD::Studio::EventDescription &event_description);

EventAttributes find_event_attributes(const char *path, size_t path_len);
EventAttributes find_event_attributes(const FMODGUID &event_id);

void make_and_add_event_attributes(const char *path, size_t path_len, const FMOD::Studio::EventDescription &event_description,
  EventAttributes &event_attributes);
void make_and_add_event_attributes(const FMODGUID &event_id, const FMOD::Studio::EventDescription &event_description,
  EventAttributes &event_attributes);
EventAttributes make_event_attributes(const FMOD::Studio::EventDescription &event_description);

size_t get_num_cached_path_attributes();
size_t get_num_cached_guid_attributes();

void invalidate_attributes_cache();
} // namespace sndsys
