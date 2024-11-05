// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <webui/httpserver.h>
#include <webui/helpers.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_e3dColor.h>
#include <stdlib.h>

#if TIME_PROFILER_ENABLED // jquery tree only needed in profiler yet
#include "jquerytablecss.inl"
#include "jquerytablejs.inl"
#endif

using namespace webui;

static int g_dump_line = 0;

static void dump_profiler_line(void *ctx, uintptr_t cNode, uintptr_t pNode, const char *name, const TimeIntervalInfo &ti)
{
  if (ti.skippedFrames > 2000)
    return;
  int line = g_dump_line++;
  // if ( pNode->mTotalTime )
  //  tabs
  YAMemSave *profiler_buf = (YAMemSave *)ctx;
  int color = max(255 - (int)ti.skippedFrames, 120);
  int col = (line & 1) ? 0xE0 : 0xFF;
  profiler_buf->printf("<tr data-tt-id=\"%p\" id=\"%p\"", cNode, cNode);
  if (pNode)
    profiler_buf->printf(" data-tt-parent-id=\"%p\" ", pNode);
  profiler_buf->printf(" color = \"#%X\" bgcolor=\"#%X\">", E3DCOLOR(color, color, color, color).u, E3DCOLOR(col, col, col, 0).u);

  profiler_buf->printf("<td>%s</td>", name);
  profiler_buf->printf("<td>%d</td>", ti.perFrameCalls);
  double avFrames = ((double)ti.lifetimeCalls) / max(1u, ti.lifetimeFrames);
  if (avFrames <= 0.99)
    profiler_buf->printf("<td>%.3f</td>", avFrames);
  else
    profiler_buf->printf("<td/>");

  profiler_buf->printf("<td>%.3f</td><td>%.3f</td><td>%.3f</td><td>%3.3f</td>", ti.tMin, ti.tMax, ti.tSpike, ti.unknownTimeMsec);

  if (ti.gpuValid)
  {
    profiler_buf->printf("<td>%.3f</td><td>%.3f</td><td>%.3f</td>", ti.tGpuMin, ti.tGpuMax, ti.tGpuSpike);
    profiler_buf->printf("<td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td>", ti.stat.val[DRAWSTAT_DP],
      (int)ti.stat.tri, ti.stat.val[DRAWSTAT_RT], ti.stat.val[DRAWSTAT_PS], ti.stat.val[DRAWSTAT_INS], ti.stat.val[DRAWSTAT_LOCKVIB]);
  }
  else
  {
    profiler_buf->printf("<td/><td/><td/>");
    profiler_buf->printf("<td/><td/><td/><td/><td/><td/>");
  }

  if (ti.skippedFrames)
    profiler_buf->printf("<td>-%d</td>", ti.skippedFrames);
  else
    profiler_buf->printf("<td/>");
  profiler_buf->printf("</tr>\n");
}

void show_profiler(RequestInfo *params)
{
#if TIME_PROFILER_ENABLED
  if (params->params && *params->params)
  {
    for (int i = 0; params->params[i * 2]; ++i)
    {
      if (strcmp(params->params[i * 2], "gpu") == 0)
      {
        if (atoi(params->params[i * 2 + 1]))
          da_profiler::add_mode(da_profiler::GPU);
        else
          da_profiler::remove_mode(da_profiler::GPU);
        return;
      }
      if (strcmp(params->params[i * 2], "profiler") == 0)
      {
        if (atoi(params->params[i * 2 + 1]))
          da_profiler::add_mode(da_profiler::EVENTS);
        else
          da_profiler::remove_mode(da_profiler::EVENTS);
        return;
      }
    }
  }
  YAMemSave buf(64 << 10), *profiler_buf = &buf;
  g_dump_line = 0;
  {
    profiler_buf->printf("<html>");
    profiler_buf->printf("<head>");
    profiler_buf->write(jquerytablecss, sizeof(jquerytablecss) - 1);
    profiler_buf->printf("</head>");
    profiler_buf->printf("<body>");
    const char *ajax_op = // Note : in google chrome you can you "printf like debugging" with console.log()
      "\n<script type='text/javascript'>\n"
      "function enabler(name, what)\n"
      "{\n"
      "  var x = new XMLHttpRequest();\n"
      "  x.open('GET','profiler?'+what+'='+(document.getElementById(name).checked ? '1' : '0'), true);\n"
      "  x.send();\n"
      "}\n"
      "</script>\n";
    profiler_buf->printf(ajax_op);
    profiler_buf->printf("<table id=\"profiled-frame\" border=\"0\" cellspacing=\"0\" style=\"font-size:10px\">\n");
    profiler_buf->printf("<caption>\n");
    profiler_buf->printf(
      "<input id='chk_profiler' type='checkbox' %s onclick='enabler(\"chk_profiler\", \"profiler\")'>Profiler</input>\n",
      (da_profiler::get_current_mode() & da_profiler::EVENTS) ? "checked" : "");
    profiler_buf->printf("<a href=\"#\" onclick=\"jQuery('#profiled-frame').treetable('expandAll'); return false;\">Expand all</a>\n");
    profiler_buf->printf(
      "<a href=\"#\" onclick=\"jQuery('#profiled-frame').treetable('collapseAll'); return false;\">Collapse all</a>\n");
    profiler_buf->printf("<input id='chk_gpu' type='checkbox' %s onclick='enabler(\"chk_gpu\", \"gpu\")'>GPU</input>\n",
      (da_profiler::get_current_mode() & (da_profiler::GPU)) == (da_profiler::GPU) ? "checked" : "");
    profiler_buf->printf("</caption>\n");

    profiler_buf->printf("<thead style=\"font-size:8px\">\n");
    profiler_buf->printf(
      "<tr><th>Name</th><th>calls</th><th>avg.</th><th>CPUmin</th><th>CPUmax</th><th>CPU spike</th><th>self time</th>"
      "<th>GPUmin</th><th>GPUmax</th><th>GPU spike</th>");
    profiler_buf->printf("<th>dip</th><th>tri</th><th>rt</th><th>prog</th><th>INST</th>"
                         "<th>lock_vbib</th>");
    profiler_buf->printf("<th>skipped frames</th></tr>\n");
    profiler_buf->printf("</thead>\n");
    profiler_buf->printf("<tbody>\n");
  }
  da_profiler::dump_frames(dump_profiler_line, profiler_buf);
  profiler_buf->printf("</tbody>\n");
  profiler_buf->printf("</table>");

  {
    const double tToUs = 1000000. / profile_ticks_frequency();
    uint32_t i = 0;
    for (;; ++i)
    {
      uint32_t uniqueFrames = 0;
      const da_profiler::UniqueEventData *ued = da_profiler::get_unique_event(i, uniqueFrames);
      if (!ued)
        break;
      const uint32_t eventFrames = uniqueFrames >= ued->startFrame ? uniqueFrames - ued->startFrame + 1 : 0;
      if (i == 0)
      {
        profiler_buf->printf("<table id=\"profiled-frame\" border=\"0\" cellspacing=\"0\" style=\"font-size:10px\">\n");
        profiler_buf->printf("<thead style=\"font-size:8px\">\n");
        profiler_buf->printf("<tr><th>Name</th><th>calls</th><th>avg us</th><th>min us</th><th>max us</th><th>calls per "
                             "frame</th><th>us per frame</th><th>frames</th></tr>");
        profiler_buf->printf("<tbody>\n");
      }
      uint32_t colorF;
      const char *name = da_profiler::get_description(ued->desc, colorF);
      profiler_buf->printf("<tr><td>%s</td><td>%d</td><td>%.2f</td><td>%.2f</td><td>%.2f</td><td>%f</td><td>%.2f</td><td>%d</td></tr>",
        name ? name : "unknown", ued->totalOccurencies, (ued->totalTicks * tToUs) / max<uint64_t>(1, ued->totalOccurencies),
        ued->minTicks * tToUs, ued->maxTicks * tToUs, double(ued->totalOccurencies) / max(1u, eventFrames),
        (ued->totalTicks * tToUs) / max(1u, eventFrames), eventFrames);
    }
    if (i)
    {
      profiler_buf->printf("</tbody></table>\n");
    }
  }
  profiler_buf->printf("<script src=\"http://ajax.googleapis.com/ajax/libs/jquery/1.10.2/jquery.min.js\"></script>\n");
  profiler_buf->printf("<script src=\"http://ajax.googleapis.com/ajax/libs/jqueryui/1.10.3/jquery-ui.min.js\"></script>\n");
  profiler_buf->printf("<script>%s</script>", jquerytablejs);
  profiler_buf->printf("<script>$(\"#profiled-frame\").treetable({ expandable: true, clickableNodeNames: true });</script>");
  profiler_buf->printf("</body></html>");
  html_response_raw(params->conn, buf.mem);
#else
  (void)&dump_profiler_line;
  return html_response(params->conn, NULL, HTTP_BAD_REQUEST);
#endif
}

webui::HttpPlugin webui::profiler_http_plugin = {"profiler", "show current profiler", NULL, show_profiler};
