// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_textMarks.h>
#include <gui/dag_stdGuiRender.h>
#include <drv/3d/dag_driver.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_bounds2.h>
#include <memory/dag_regionMemAlloc.h>
#include <generic/dag_tab.h>

struct TextMarkRec
{
  float cx = 0.f, cy = 0.f, lofs = 0.f;
  char *str = nullptr;
  E3DCOLOR frameColor = E3DCOLOR_MAKE(0, 0, 0, 160);
};
static Tab<TextMarkRec> text_marks(midmem);
static RegionMemPool *mem = NULL;
static TMatrix4 view_gtm;
static int view_l, view_t, view_w, view_h;
static bool view_inited = false; // set/reset once per frame

void prepare_debug_text_marks(const TMatrix4 &glob_tm, float in_view_w, float in_view_h)
{
  view_inited = true;
  view_gtm = glob_tm;
  view_l = 0.0f;
  view_t = 0.0f;
  view_w = in_view_w;
  view_h = in_view_h;
}

bool cvt_debug_text_pos(const Point3 &wp, float &out_cx, float &out_cy)
{
  Point4 sp = Point4::xyz1(wp) * view_gtm;
  if (fabs(sp.w) < 1e-6)
    return false;
  sp /= sp.w;
  if (sp.z > 1 || fabs(sp.x) >= 1 || fabs(sp.y) >= 1 || sp.z < 0)
    return false;

  out_cx = view_l + (sp.x + 1.0f) * 0.5f * view_w;
  out_cy = view_t + (-sp.y + 1.0f) * 0.5f * view_h;
  return true;
}
void add_debug_text_mark(float scr_cx, float scr_cy, const char *str, int length, float line_ofs, E3DCOLOR frame_color)
{
  if (!str || !str[0])
    return;
  int len = length > -1 ? length + 1 : (int)strlen(str) + 1;
  if (len > 4096)
    return;

  if (!mem)
    mem = RegionMemPool::createPool(32 << 10);

  TextMarkRec &r = text_marks.push_back();
  r.cx = scr_cx;
  r.cy = scr_cy;
  r.lofs = line_ofs;
  r.frameColor = frame_color;
  r.str = (char *)RegionMemPool::alloc(mem, len, 1, 32 << 10);
  memcpy(r.str, str, len); // -V575
  r.str[len - 1] = '\0';
}

void add_debug_text_mark(const Point3 &wp, const char *str, int length, float line_ofs, E3DCOLOR frame_color)
{
  float cx, cy;
  if (cvt_debug_text_pos(wp, cx, cy))
    add_debug_text_mark(cx, cy, str, length, line_ofs, frame_color);
}

static void reset_debug_text_marks()
{
  view_inited = false;
  text_marks.clear();
  RegionMemPool::clear(mem);
}

void render_debug_text_marks()
{
  if (!text_marks.size())
  {
    reset_debug_text_marks();
    return;
  }
  G_ASSERTF(view_inited, "render_debug_text_marks() is used without first initializing it using prepare_debug_text_marks()");

  StdGuiRender::start_render();
  StdGuiRender::reset_textures();
  StdGuiRender::set_font(0);

  float fh = StdGuiRender::get_font_cell_size().y;
  E3DCOLOR bgColor = E3DCOLOR_MAKE(0, 0, 0, 160);
  for (int i = 0; i < text_marks.size(); i++)
  {
    const TextMarkRec &r = text_marks[i];
    if (r.frameColor == bgColor)
      continue;
    Point2 htsz = StdGuiRender::get_str_bbox(r.str).width() * 0.5f;
    StdGuiRender::set_color(r.frameColor);
    StdGuiRender::render_box(r.cx - htsz.x - 4, r.cy - htsz.y - 4 + (r.lofs - 0.8) * fh, r.cx + htsz.x + 4,
      r.cy + htsz.y + 4 + (r.lofs - 0.8) * fh);
  }
  StdGuiRender::set_color(bgColor);
  for (int i = 0; i < text_marks.size(); i++)
  {
    const TextMarkRec &r = text_marks[i];
    Point2 htsz = StdGuiRender::get_str_bbox(r.str).width() * 0.5f;
    StdGuiRender::render_box(r.cx - htsz.x - 2, r.cy - htsz.y - 2 + (r.lofs - 0.8) * fh, r.cx + htsz.x + 2,
      r.cy + htsz.y + 2 + (r.lofs - 0.8) * fh);
  }
  StdGuiRender::set_color(E3DCOLOR(255, 255, 255, 255));
  for (int i = 0; i < text_marks.size(); i++)
  {
    const TextMarkRec &r = text_marks[i];
    Point2 htsz = StdGuiRender::get_str_bbox(r.str).width() * 0.5f;
    StdGuiRender::goto_xy(floorf(r.cx - htsz.x), floorf(r.cy - htsz.y + r.lofs * fh));
    StdGuiRender::draw_str(r.str);
  }

  StdGuiRender::end_render();
  reset_debug_text_marks();
}

void shutdown_debug_text_marks()
{
  clear_and_shrink(text_marks);
  RegionMemPool::deletePool(mem);
  mem = NULL;
}
