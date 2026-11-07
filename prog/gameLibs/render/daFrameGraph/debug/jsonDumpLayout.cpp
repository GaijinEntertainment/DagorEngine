// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "jsonDumpCommon.h"

#include <backend/resourceScheduling/heapLayout.h>
#include <backend/resourceScheduling/resourceScheduler.h>

namespace dafg::jsondump
{

extern Writer w;


static void print_gpu_description(const ResourceDescription &desc)
{
  w.Key("type");
  write_string(w, detail::get_enum_name(desc.type));

  w.Key("cFlags");
  w.Uint(desc.asBasicRes.cFlags);

  w.Key("activation");
  write_string(w, detail::get_enum_name(desc.asBasicRes.activation));

  w.Key("clearValue");
  {
    ARRAY_SCOPED
    for (int i = 0; i < 4; ++i)
      w.Uint(desc.asBasicRes.clearValue.asUint[i]);
  }

  if (desc.type == D3DResourceType::SBUF)
  {
    w.Key("elementSizeInBytes");
    w.Uint(desc.asBufferRes.elementSizeInBytes);
    w.Key("elementCount");
    w.Uint(desc.asBufferRes.elementCount);
    w.Key("viewFormat");
    w.Uint(desc.asBufferRes.viewFormat);
  }
  else
  {
    w.Key("mipLevels");
    w.Uint(desc.asBasicTexRes.mipLevels);

    if (desc.type == D3DResourceType::CUBETEX || desc.type == D3DResourceType::CUBEARRTEX)
    {
      w.Key("extent");
      w.Uint(desc.asCubeTexRes.extent);
      if (desc.type == D3DResourceType::CUBEARRTEX)
      {
        w.Key("cubes");
        w.Uint(desc.asArrayCubeTexRes.cubes);
      }
    }
    else
    {
      w.Key("width");
      w.Uint(desc.asTexRes.width);
      w.Key("height");
      w.Uint(desc.asTexRes.height);

      if (desc.type == D3DResourceType::VOLTEX)
      {
        w.Key("depth");
        w.Uint(desc.asVolTexRes.depth);
      }
      else if (desc.type == D3DResourceType::ARRTEX)
      {
        w.Key("arrayLayers");
        w.Uint(desc.asArrayTexRes.arrayLayers);
      }
    }
  }
}

static void print_rect(const LayoutRect &rect, const intermediate::Graph &graph, const Timeline &timeline)
{
  OBJECT_SCOPED

  w.Key("resource");
  w.String(get_name(graph, rect.what.resIdx));

  w.Key("frame");
  w.Uint(rect.what.frame);

  w.Key("offset");
  w.Uint64(rect.offset);

  w.Key("size");
  w.Uint64(rect.size);

  w.Key("align");
  w.Uint64(rect.align);

  w.Key("firstUse");
  {
    OBJECT_SCOPED
    w.Key("frame");
    w.Uint(rect.lifetime.firstUse.frame);
    w.Key("timepoint");
    w.Uint(rect.lifetime.firstUse.timepoint);
  }

  w.Key("release");
  {
    OBJECT_SCOPED
    w.Key("frame");
    w.Uint(rect.lifetime.release.frame);
    w.Key("timepoint");
    w.Uint(rect.lifetime.release.timepoint);
  }

  const uint32_t startMoment = timeline.moment(rect.lifetime.firstUse);
  const uint32_t endMoment = timeline.moment(rect.lifetime.release);

  w.Key("startMoment");
  w.Uint(startMoment);

  w.Key("endMoment");
  w.Uint(endMoment);

  w.Key("wraps");
  w.Bool(endMoment <= startMoment);

  w.Key("rect.hash");
  w.Uint64(rect.hash);

  if (!graph.resources.isMapped(rect.what.resIdx))
    return;
  const intermediate::Resource &res = graph.resources[rect.what.resIdx];
  if (!res.isScheduled())
    return;

  const intermediate::ScheduledResource &scheduled = res.asScheduled();
  w.Key("resourceType");
  write_string(w, detail::get_enum_name(scheduled.resourceType));

  if (scheduled.isGpuResource())
  {
    w.Key("description");
    OBJECT_SCOPED
    print_gpu_description(scheduled.getGpuDescription());
  }
  else
  {
    const intermediate::BlobDescription &blob = scheduled.getCpuDescription();
    w.Key("description");
    OBJECT_SCOPED
    w.Key("typeTag");
    w.Uint64(eastl::to_underlying(blob.typeTag));
    w.Key("size");
    w.Uint64(blob.size);
    w.Key("alignment");
    w.Uint64(blob.alignment);
  }
}

void dump_layout_to_json(const intermediate::Graph &graph, const Layout &layout)
{
  const Timeline timeline = make_timeline(graph);

  w.Key("layoutTimeline");
  {
    OBJECT_SCOPED

    w.Key("timepointsPerFrame");
    w.Uint(timeline.timepointsPerFrame);

    w.Key("length");
    w.Uint(timeline.length());
  }

  w.Key("heaps");
  ARRAY_SCOPED

  for (auto &&[heapIdx, heap] : layout.heaps.enumerate())
  {
    OBJECT_SCOPED

    w.Key("index");
    w.Uint(eastl::to_underlying(heapIdx));

    // The pointer only groups heaps inside one dump, it differs between runs
    w.Key("heapGroup");
    write_string(w, String(0, "%p", (void *)heap.heapClass.group));

    w.Key("cpuHeap");
    w.Bool(heap.heapClass.group == CPU_HEAP_GROUP);

    w.Key("untracked");
    w.Bool(heap.heapClass.untracked);

    w.Key("reservedSize");
    w.Uint64(heap.reservedSize);

    w.Key("usedSize");
    w.Uint64(measure_used_size(heap));

    w.Key("rects");
    ARRAY_SCOPED
    for (const auto &rect : heap.rects)
      print_rect(rect, graph, timeline);
  }
}

} // namespace dafg::jsondump
