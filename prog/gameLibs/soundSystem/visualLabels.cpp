// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/vector.h>
#include <EASTL/array.h>
#include <EASTL/fixed_string.h>
#include <memory/dag_framemem.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_critSec.h>
#include <fmod_studio.hpp>
#include <soundSystem/soundSystem.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/debug.h>
#include <soundSystem/visualLabels.h>
#include "internal/fmodCompatibility.h"
#include "internal/attributes.h"
#include "internal/banks.h"
#include "internal/visualLabels.h"
#include "internal/debug.h"
#include <osApiWrappers/dag_atomic_types.h>

static WinCritSec g_labels_cs;
#define SNDSYS_LABELS_BLOCK WinAutoLock labelsLock(g_labels_cs);

namespace sndsys
{
using namespace fmodapi;
using namespace banks;

struct VisualLabelType
{
  eastl::fixed_string<char, 16> name;
  VisualLabelType() = delete;
  VisualLabelType(const char *name) : name(name) {}
};

struct VisualLabelDesc
{
  const FMOD::Studio::EventDescription *desc;
  int typeIdx;
  float maxDistance;
  VisualLabelDesc() = delete;
  VisualLabelDesc(const FMOD::Studio::EventDescription *desc, int type_idx, float max_distance) :
    desc(desc), typeIdx(type_idx), maxDistance(max_distance)
  {}
};

static eastl::vector<VisualLabelType> visual_label_types;
static eastl::vector<VisualLabelDesc> visual_label_descs;

// should call query_visual_labels() time to time to enable desc gathering
static dag::AtomicInteger<bool> visual_labels_enabled(false);
static const float disable_visual_labels_query_interval = 5.f;
static float visual_labels_timer = 0.f;

int get_visual_label_idx(const char *name)
{
  SNDSYS_LABELS_BLOCK;
  if (name && *name)
  {
    auto pred = [&](const VisualLabelType &it) { return it.name == name; };
    auto it = eastl::find_if(visual_label_types.begin(), visual_label_types.end(), pred);
    if (it != visual_label_types.end())
      return it - visual_label_types.begin();
    visual_label_types.emplace_back(name);
    return int(visual_label_types.size()) - 1;
  }
  return -1;
}

void append_visual_label(const FMOD::Studio::EventDescription &event_description, const EventAttributes &attributes)
{
  if (!visual_labels_enabled.load())
    return;
  const int idx = attributes.getVisualLabelIdx();
  if (idx >= 0)
  {
    SNDSYS_LABELS_BLOCK;
    auto pred = [&](const VisualLabelDesc &it) { return it.desc == &event_description; };
    if (visual_label_descs.end() == eastl::find_if(visual_label_descs.begin(), visual_label_descs.end(), pred))
      visual_label_descs.emplace_back(&event_description, idx, attributes.getMaxDistance());
  }
}

void update_visual_labels(float dt)
{
  if (visual_labels_enabled.load())
  {
    SNDSYS_LABELS_BLOCK;
    visual_labels_timer -= dt;
    if (visual_labels_timer < 0.f)
    {
      visual_labels_timer = 0.f;
      visual_labels_enabled.store(false);
      invalidate_visual_labels();
    }
  }
}

void invalidate_visual_labels()
{
  SNDSYS_LABELS_BLOCK;
  visual_label_descs.clear();
}

VisualLabels query_visual_labels()
{
  SNDSYS_LABELS_BLOCK;
  visual_labels_enabled.store(true);
  visual_labels_timer = disable_visual_labels_query_interval;

  VisualLabels labels;
  Attributes3D attributes3d;
  eastl::array<FMOD::Studio::EventInstance *, 32> instances;
  size_t total = visual_label_descs.size();
  for (intptr_t idx = 0; idx < total;)
  {
    VisualLabelDesc &it = visual_label_descs[idx];
    if (it.desc->isValid())
    {
      int numInstances = 0;
      const FMOD_RESULT res = it.desc->getInstanceList(instances.begin(), instances.size(), &numInstances);
      SOUND_VERIFY(res);
      if (FMOD_OK == res && numInstances > 0)
      {
        for (const FMOD::Studio::EventInstance *instance : make_span(instances.begin(), numInstances))
        {
          if (FMOD_OK == instance->get3DAttributes(&attributes3d))
            labels.emplace_back(visual_label_types[it.typeIdx].name.c_str(), as_point3(attributes3d.position), it.maxDistance);
        }
        ++idx;
        continue;
      }
    }
    it = eastl::move(visual_label_descs[--total]);
  }

  visual_label_descs.erase(visual_label_descs.begin() + total, visual_label_descs.end());

  return labels;
}
} // namespace sndsys
