// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gui/dag_visualLog.h>
#include <generic/dag_tab.h>
#include <gui/dag_stdGuiRender.h>
#include <drv/3d/dag_driver.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/dag_delayedAction.h>

namespace visuallog
{
static int max_items = DEFAULT_VISUALLOG_MAX_ITEMS;
#if _TARGET_PC
static int max_history_items = 512;
#else
static int max_history_items = 0;
#endif

static Tab<SimpleString> history(midmem);
static Tab<LogItem> tabItems(midmem);
static Point2 offset(20, 20);
static int fontId = 0;
static float time;
static bool has_any_error = false;
static bool drawDisabled = false;
static visuallog::OnLogItemAdded onLogItemAdded = nullptr;

static void clear_unwanted_items()
{
  while (tabItems.size() >= max_items)
  {
    int candidate = 0;
    for (int i = 0; i < tabItems.size(); i++)
    {
      if (!tabItems[i].cb)
      {
        candidate = i;
        break;
      }
    }

    if (tabItems.size() > 0)
    {
      LogItem &li = tabItems[candidate];
      if (li.cb)
        li.cb(EVT_DELETE, &li);
      erase_items(tabItems, candidate, 1);
    }
    else
      break;
  }

  tabItems.reserve(max_items);
}

void setMaxItems(int max_items_)
{
  max_items = max(max_items_, 0);
  clear_unwanted_items();
}

void reset(bool reset_border)
{
  G_ASSERT_AND_DO(is_main_thread(), return);
  time = 0;
  clear_and_shrink(tabItems);

  offset = Point2(20, 20);
  fontId = 0;
  if (reset_border)
    has_any_error = false;
}

void clear()
{
  G_ASSERT_AND_DO(is_main_thread(), return);
  time = 0;
  clear_and_shrink(tabItems);
}

struct LogItemDelayed : public DelayedAction, public LogItem
{
  bool precondition() override { return is_main_thread(); }
  void performAction() override { logmsg(text, cb, param, color, level); }
};

void logmsg(const char *text, LogItemCBProc cb, void *param, E3DCOLOR color, int level)
{
  if (!is_main_thread())
  {
    void *stor = tmpmem->tryAlloc(sizeof(LogItemDelayed));
    if (!stor)
      return;
    LogItemDelayed *p = new (stor, _NEW_INPLACE) LogItemDelayed;
    p->text = text;
    p->color = color;
    p->cb = cb;
    p->param = param;
    p->level = level;
    ::add_delayed_action(p);
    return;
  }

  static bool entered = false;
  if (entered)
    return;
  entered = true;

  if (level >= 1000)
    has_any_error = true;

  if (max_history_items > 0)
  {
    if (history.size() > max_history_items)
      erase_items(history, 0, clamp(max_history_items / 2, 1, 16));

    history.push_back() = text;
  }

  clear_unwanted_items();

  if (max_items > 0)
  {
    int n = append_items(tabItems, 1);
    LogItem &li = tabItems[n];

    int slen = (int)strlen(text);
    li.text.setStr(text, slen < 6000 ? slen : 6000);
    li.color = color;
    li.cb = cb;
    li.param = param;
    li.ialpha = /*1.f*/ 0x3f800000; // Avoid using floats to potentially avoid triggering FPE
    li.level = level;
    if (onLogItemAdded)
      onLogItemAdded(li);
  }

  entered = false;
}


void act(float dt)
{
  const float actRate = 0.02f;
  const float fade = 1.f / 6.5f; // full visibility time = 6.5 sec

  time += dt;
  if (time < actRate)
    return;

  G_ASSERT_AND_DO(is_main_thread(), return);
  for (int i = tabItems.size() - 1; i >= 0; i--)
  {
    LogItem &li = tabItems[i];

    bool reactivate = false;

    if (li.cb)
      reactivate = li.cb(EVT_REFRESH, &li);

    if (reactivate)
    {
      li.alpha = 1;
    }
    else
    {
      li.alpha -= fade * time;
      if (li.alpha < 0)
      {
        if (li.cb)
          li.cb(EVT_DELETE, &li);
        erase_items(tabItems, i, 1);
      }
    }
  }

  time = 0;
}

void draw(int min_level)
{
  G_ASSERT_AND_DO(is_main_thread(), return);

  if (drawDisabled)
    return;

  if (has_any_error && max_items == 0)
  {
    StdGuiRender::ScopeStarterOptional scopedStart;
    StdGuiRender::set_color(E3DCOLOR(255, 0, 0));
    StdGuiRender::render_frame(0, 0, StdGuiRender::screen_width(), StdGuiRender::screen_height(), 1);
  }

  if (!tabItems.size())
    return;

  TIME_D3D_PROFILE(visuallog)

  StdGuiRender::ScopeStarterOptional strt;
  StdGuiRender::set_font(fontId);

  real fontHeight = StdGuiRender::get_font_cell_size().y * 1.5f;

  for (int i = 0; i < tabItems.size(); i++)
  {
    LogItem &li = tabItems[i];
    if (min_level > li.level)
      continue;
    if (!li.text.length())
      continue;

    int x = (int)offset.x;
    int y = int(fontHeight * i + offset.y);

    E3DCOLOR bColor(0, 0, 0, 255);
    E3DCOLOR fColor(li.color);
    E3DCOLOR transparent(0, 0, 0, 0);

    real alpha = (li.alpha > 0.0999f) ? 1.0f : li.alpha * 10.0f;

    fColor = e3dcolor_lerp(transparent, fColor, alpha);
    bColor = e3dcolor_lerp(transparent, bColor, alpha);

    StdGuiRender::goto_xy(x + 1, y + 1);
    StdGuiRender::set_color(bColor);
    StdGuiRender::draw_str(li.text);

    StdGuiRender::goto_xy(x, y);
    StdGuiRender::set_color(fColor);
    StdGuiRender::draw_str(li.text);
  }
}

dag::ConstSpan<SimpleString> getHistory() { return history; }
} // namespace visuallog

void visuallog::setOffset(const Point2 &offs) { offset = offs; }

void visuallog::setFont(const char *name)
{
  fontId = StdGuiRender::get_font_id(name);
  if (fontId < 0)
    fontId = 0;
}

void visuallog::setDrawDisabled(bool disabled) { drawDisabled = disabled; }

void visuallog::setOnLogItemAdded(visuallog::OnLogItemAdded cb) { onLogItemAdded = cb; }
