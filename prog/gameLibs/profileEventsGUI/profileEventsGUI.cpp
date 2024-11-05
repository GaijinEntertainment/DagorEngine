// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <profileEventsGUI/profileEventsGUI.h>
#include <perfMon/dag_statDrv.h>
#include <hash/wyhash.h>
#include <util/dag_console.h>
#include <gui/dag_stdGuiRender.h>
#include <dag/dag_vectorSet.h>
#include <EASTL/string.h>
#include <memory/dag_framemem.h>

#if DA_PROFILER_ENABLED
void ProfileUniqueEventsGUI::updateAll()
{
  // add all new events
  auto ued = da_profiler::get_unique_event(uniqueDescs.size(), uniqueFrames);
  for (uint32_t i = uniqueDescs.size(); ued; ued = da_profiler::get_unique_event(++i, uniqueFrames))
  {
    uint32_t colorF = 0;
    const char *name = da_profiler::get_description(ued->desc, colorF);
    colorF &= (1 << 25) - 1;
    if (colorF == 0)
      colorF = (wyhash64_const(uint64_t(ued->desc), _wyp[0])) & ((1 << 25) - 1);
    uniqueDescs.emplace_back(UniqueProfileDesc{ued, name, E3DCOLOR(colorF | (0xFF << 24))});
  }
}

void ProfileUniqueEventsGUI::drawProfiled()
{
  updateAll();
  bool do_start = !StdGuiRender::is_render_started();
  if (do_start)
    StdGuiRender::start_render();
  StdGuiRender::set_font(0);
  StdGuiRender::set_ablend(true);
  eastl::string s;
  int base_line = StdGuiRender::get_font_ascent() + 5, font_ht = (StdGuiRender::get_def_font_ht(0) * 3 + 3) / 4;
  const int prevHt = StdGuiRender::set_font_ht(font_ht);

  const double tToUs = 1000000. / profile_ticks_frequency();
  int at = 0;
  auto drawOne = [&](UniqueProfileDesc &uedI, uint32_t flushAt) {
    if (at >= 1080)
      return;
    const uint32_t eventFrames = uniqueFrames >= uedI.ued->startFrame ? uniqueFrames - uedI.ued->startFrame + 1 : 0;
    if (!uedI.ued->totalOccurencies || !eventFrames)
      return;
    s.sprintf("min %04.1f|avg %05.1f|max %06.1f|%04d fr calls|%05.1fus frame|%03d frames: %s", uedI.ued->minTicks * tToUs,
      (uedI.ued->totalTicks * tToUs) / uedI.ued->totalOccurencies, uedI.ued->maxTicks * tToUs,
      uint32_t(floor(double(uedI.ued->totalOccurencies) / eventFrames + 0.5)), (uedI.ued->totalTicks * tToUs) / eventFrames,
      eventFrames, uedI.name);
    StdGuiRender::set_color(uedI.color);
    StdGuiRender::goto_xy(30, base_line + font_ht * 2 + (font_ht * 3 / 2) * at);
    StdGuiRender::set_draw_str_attr(FFT_SHADOW, 1, 1, E3DCOLOR((0xFF << 24)), 16);
    StdGuiRender::draw_str(s.c_str());
    if (flushAt && (eventFrames % flushAt) == 0)
    {
      auto ue = const_cast<da_profiler::UniqueEventData *>(uedI.ued);
      ue->maxTicks = ue->totalTicks = ue->totalOccurencies = 0;
      ue->minTicks = ~0ULL;
      ue->startFrame = uniqueFrames;
    }
    ++at;
  };
  if (profileAll)
  {
    for (auto &uedI : uniqueDescs)
      drawOne(uedI, allFlushAt);
  }
  else
  {
    for (auto i : profiledEvents)
      drawOne(uniqueDescs[i], uniqueDescs[i].flushAtFrame);
  }
  StdGuiRender::reset_draw_str_attr();
  StdGuiRender::flush_data();
  StdGuiRender::set_font_ht(prevHt);
  if (do_start)
    StdGuiRender::end_render();
}

void ProfileUniqueEventsGUI::addProfile(const char *name, uint32_t flushAt)
{
  if (strcmp(name, "-") == 0)
  {
    profiledEvents.clear();
    profileAll = false;
    return;
  }
  else if (strcmp(name, "*") == 0)
  {
    profiledEvents.clear();
    allFlushAt = flushAt;
    profileAll = true;
    return;
  }
  profileAll = false;
  updateAll();
  dag::VectorSet<uint16_t, eastl::less<uint16_t>, framemem_allocator> alreadyProfiled;
  for (auto i : profiledEvents)
    if (strstr(name, uniqueDescs[i].name))
      alreadyProfiled.emplace(i);

  for (uint32_t i = 0, e = uniqueDescs.size(); i != e; ++i)
    if (strstr(name, uniqueDescs[i].name) && alreadyProfiled.find(i) == alreadyProfiled.end())
    {
      profiledEvents.push_back(i);
      uniqueDescs[i].flushAtFrame = flushAt;
    }
}

void ProfileUniqueEventsGUI::flush(const char *name)
{
  updateAll();
  for (auto &ued : uniqueDescs)
    if (!name || strstr(name, ued.name))
    {
      auto ue = const_cast<da_profiler::UniqueEventData *>(ued.ued);
      ue->maxTicks = ue->totalTicks = ue->totalOccurencies = 0;
      ue->minTicks = ~0ULL;
      ue->startFrame = uniqueFrames;
    }
}
#else
void ProfileUniqueEventsGUI::flush(const char *) {}
void ProfileUniqueEventsGUI::addProfile(const char *, uint32_t) {}
void ProfileUniqueEventsGUI::drawProfiled() {}
void ProfileUniqueEventsGUI::updateAll() {}
#endif
