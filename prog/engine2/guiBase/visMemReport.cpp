#include <debug/dag_memReport.h>
#include <memory/dag_memStat.h>
#include <gui/dag_stdGuiRender.h>
#include <3d/tql.h>
#include <util/memReport.h>

void memreport::on_screen_memory_usage_report(int x0, int y0, bool sysmem, bool gpumem)
{
  String text[4];
  E3DCOLOR text_col[4] = {
    E3DCOLOR(255, 255, 255, 255),
    E3DCOLOR(255, 0, 0, 255),
    E3DCOLOR(255, 0, 255, 255),
    E3DCOLOR(0, 128, 255, 255),
  };
  float wd = 0, ht = 0, ofs[4] = {0, 0, 0, 0};
  const float H_MARGIN = StdGuiRender::get_font_cell_size().x;
  const float V_INDENT = StdGuiRender::get_font_line_spacing() * 0.75;
  BBox2 text_bb;

  size_t total_mem = dagor_memory_stat::get_memory_allocated(true);
#if MEMREPORT_CRT_INCLUDES_GPUMEM
  DECL_AND_GET_GPU_METRICS(gpumem || sysmem);
  memreport::update_max_used(total_mem - (size_t(mem_used_discardable_kb + mem_used_persistent_kb) << 10));
#else
  DECL_AND_GET_GPU_METRICS(gpumem);
  memreport::update_max_used(total_mem);
#endif

  if (sysmem)
  {
    size_t dagor_mem = dagor_memory_stat::get_memory_allocated(false);
    size_t total_ptr = dagor_memory_stat::get_memchunk_count(true);
    size_t dagor_ptr = dagor_memory_stat::get_memchunk_count(false);
#if MEMREPORT_CRT_INCLUDES_GPUMEM
    total_mem -= size_t(mem_used_discardable_kb + mem_used_persistent_kb) << 10;
#endif
    text[0].printf(128, "SYSmem: %dK in %dk ptrs (+CRT %dM in %d ptrs) Max=%dM", dagor_mem >> 10, dagor_ptr >> 10,
      (total_mem - dagor_mem) >> 20, total_ptr - dagor_ptr, interlocked_acquire_load(sysmem_used_max_kb) >> 10);
    text_bb = StdGuiRender::get_str_bbox(text[0]);
    inplace_max(wd, text_bb.width().x);
    ofs[0] = ofs[1] = ht - text_bb[0].y;
    ht += text_bb.width().y + V_INDENT;

    if (sysmem_used_ref)
    {
      text[1].printf(128, "DIFF: %dK in %d ptrs [+base %dM]", ptrdiff_t(total_mem - (sysmem_used_ref - sharedmem_used_ref)) / 1024,
        total_ptr - sysmem_ptrs_ref, (sysmem_used_ref - sharedmem_used_ref) >> 20);
      text_bb = StdGuiRender::get_str_bbox(text[1]);
      inplace_max(wd, text_bb.width().x);
      ofs[1] = ht - text_bb[0].y;
      ht += text_bb.width().y + V_INDENT;
    }
  }

  if (gpumem && tql::streaming_enabled)
  {
    text[2].printf(128, "GPUmem: %dK  [%dK (%d tex) +%dK pers]", mem_used_discardable_kb + mem_used_persistent_kb,
      mem_used_discardable_kb, tex_used_discardable_cnt, mem_used_persistent_kb);
    text_bb = StdGuiRender::get_str_bbox(text[2]);
    inplace_max(wd, text_bb.width().x);
    ofs[2] = ht - text_bb[0].y;
    ht += text_bb.width().y + V_INDENT;

    text[3].printf(128, "GPUmem: Quota=%dM  Max=%dM [%dM + %dM]", tql::mem_quota_kb >> 10, mem_used_sum_kb_max >> 10,
      mem_used_discardable_kb_max >> 10, mem_used_persistent_kb_max >> 10);
    text_bb = StdGuiRender::get_str_bbox(text[3]);
    inplace_max(wd, text_bb.width().x);
    ofs[3] = ht - text_bb[0].y;
    ht += text_bb.width().y + V_INDENT;
  }

  wd += H_MARGIN * 2, ht += V_INDENT;

  StdGuiRender::set_ablend(true);
  StdGuiRender::set_color(0, 0, 0, 192);
  if (x0 < 0)
    x0 += StdGuiRender::screen_width() - wd;
  if (y0 < 0)
    y0 += StdGuiRender::screen_height() - ht;

  StdGuiRender::set_texture(BAD_TEXTUREID);
  StdGuiRender::render_rect(Point2(x0, y0), Point2(x0 + wd, y0 + ht));

  for (int ln = 0; ln < countof(text); ln++)
  {
    StdGuiRender::set_color(text_col[ln]);
    StdGuiRender::goto_xy(x0 + H_MARGIN, y0 + ofs[ln] + V_INDENT);
    StdGuiRender::draw_str(text[ln]);
  }
}
