// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "net/netStat.h"
#include <gui/dag_stdGuiRender.h>
#include <stdio.h>
#include "net/net.h"
#include "net/plosscalc.h"
#include "phys/physUtils.h"
#include <drv/3d/dag_renderTarget.h>

extern FixedPlossCalc &phys_get_snapshots_ploss_calc();

namespace netstat
{
static bool render_enabled = false;
void toggle_render() { render_enabled = !render_enabled; }

//
// I sincerely hope that this implementation only temp one and soon will be replaced to something daRg-based
//
void render()
{
  dag::ConstSpan<Sample> ns = get_aggregations();
  if (ns.empty() || !render_enabled)
    return;

  float physSnapPloss = phys_get_snapshots_ploss_calc().calcPacketLoss() * 100.f;

  StdGuiRender::ScopeStarterOptional strt;

  static const char stubText[] = "XXXXXXXXXXX XXXX XXX.X XXX.X XXX.X";
  int w = 0, h = 0;
  d3d::get_screen_size(w, h);
  StdGuiRender::set_font(0); // hmmm
  Point2 bbw = StdGuiRender::get_str_bbox(stubText, sizeof(stubText) - 1).width();
  Point2 cursor(w * 0.03, 32); // h-bbw.y*16

  static const char numbers[] = "0123456789";
  Point2 numw = StdGuiRender::get_str_bbox(numbers, sizeof(numbers) - 1).width();
  StdGuiRender::set_font(0, 0, ceilf(numw.x / 10)); // emulating monospaced font

  auto draw_str = [&cursor, &bbw](uint8_t r, uint8_t g, uint8_t b, const char *str, int len) {
    StdGuiRender::goto_xy(cursor);
    StdGuiRender::set_color(r, g, b);
    StdGuiRender::draw_str(str, len);
    cursor.y += bbw.y * 1.5f;
  };

  int slen;
  char tmp[256];

#define SCR_PRINTF(r, g, b, ...)                     \
  do                                                 \
  {                                                  \
    slen = _snprintf(tmp, sizeof(tmp), __VA_ARGS__); \
    G_FAST_ASSERT((unsigned)slen < sizeof(tmp));     \
    draw_str(r, g, b, tmp, slen);                    \
  } while (0)

#define KBIT(x) ((x)*8) / 1024.f
#define F(x)    float(x)
  static const char fmt[] = "%-15s %6s %6.1f %6.1f %6.1f";
  // Colors are from https://en.wikipedia.org/wiki/ANSI_escape_code#Colors (Terminal.app)
  SCR_PRINTF(252, 57, 31, "%-15s %6s %6s %6s %6s", "name", "dim", "avg", "min", "max"); // header
  SCR_PRINTF(49, 231, 34, fmt, "RX", "Kbit/s", KBIT(ns[AG_AVG].counters[CT_RX_BYTES]), KBIT(ns[AG_MIN].counters[CT_RX_BYTES]),
    KBIT(ns[AG_MAX].counters[CT_RX_BYTES]));
  SCR_PRINTF(234, 236, 35, fmt, "TX", "Kbit/s", KBIT(ns[AG_AVG].counters[CT_TX_BYTES]), KBIT(ns[AG_MIN].counters[CT_TX_BYTES]),
    KBIT(ns[AG_MAX].counters[CT_TX_BYTES]));
  SCR_PRINTF(88, 51, 255, fmt, "RX PPS", "", F(ns[AG_AVG].counters[CT_RX_PACKETS]), F(ns[AG_MIN].counters[CT_RX_PACKETS]),
    F(ns[AG_MAX].counters[CT_RX_PACKETS]));
  SCR_PRINTF(249, 53, 248, fmt, "TX PPS", "", F(ns[AG_AVG].counters[CT_TX_PACKETS]), F(ns[AG_MIN].counters[CT_TX_PACKETS]),
    F(ns[AG_MAX].counters[CT_TX_PACKETS]));
  SCR_PRINTF(20, 240, 240, fmt, "Ping", "ms", F(ns[AG_AVG].counters[CT_RTT]), F(ns[AG_MIN].counters[CT_RTT]),
    F(ns[AG_MAX].counters[CT_RTT]));
  SCR_PRINTF(233, 235, 235, fmt, "RX Phys Ploss", "%", physSnapPloss, physSnapPloss, physSnapPloss); // TODO: min/max
  SCR_PRINTF(173, 173, 39, fmt, "TX Ctrl Ploss", "%", F(ns[AG_AVG].counters[CT_CTRL_PLOSS]) / 10.f,
    F(ns[AG_MIN].counters[CT_CTRL_PLOSS]) / 10.f, F(ns[AG_MAX].counters[CT_CTRL_PLOSS]) / 10.f);
  float deciTick = phys_get_tickrate() / 10.f;
  auto tickPct = [deciTick](cnt_t v) { return v * deciTick; };
  if (ns[AG_MAX].counters[CT_DEV_SERVER_TICK])
    SCR_PRINTF(203, 204, 205, fmt, "Dev Server Load", "%", tickPct(ns[AG_AVG].counters[CT_DEV_SERVER_TICK]),
      tickPct(ns[AG_MIN].counters[CT_DEV_SERVER_TICK]), tickPct(ns[AG_MAX].counters[CT_DEV_SERVER_TICK]));
  else
    SCR_PRINTF(203, 204, 205, fmt, "Server Load", "%", tickPct(ns[AG_AVG].counters[CT_SERVER_TICK]),
      tickPct(ns[AG_MIN].counters[CT_SERVER_TICK]), tickPct(ns[AG_MAX].counters[CT_SERVER_TICK]));
  if ((net::get_server_flags() & net::ServerFlags::DynTick) != net::ServerFlags::None || phys_get_tickrate() != PHYS_DEFAULT_TICKRATE)
    SCR_PRINTF(203, 204, 205, "%-23s %3u", "Server Tickrate", phys_get_tickrate());
  SCR_PRINTF(51, 187, 200, fmt, "TX Enet Ploss", "%", F(ns[AG_AVG].counters[CT_ENET_PLOSS]) / 10.f,
    F(ns[AG_MIN].counters[CT_ENET_PLOSS]) / 10.f, F(ns[AG_MAX].counters[CT_ENET_PLOSS]) / 10.f);

  SCR_PRINTF(252, 57, 31, "%-16s %7s %7s", "name", "AAS", "Partial"); // header
  SCR_PRINTF(49, 231, 34, "%-16s %4d/%-2d %4d/%-2d", "Desyncs/AAS", ns[AG_SUM].counters[CT_AAS_DESYNCS],
    ns[AG_SUM].counters[CT_AAS_PROCESSED], ns[AG_SUM].counters[CT_PAAS_DESYNCS], ns[AG_SUM].counters[CT_PAAS_PROCESSED]);
  SCR_PRINTF(249, 53, 248, "%-15s %7.2f %7.2f", "Max pos diff (m)", F(ns[AG_MAX].counters[CT_AAS_DESYNC_POS_DIFF]) / 100.f,
    F(ns[AG_MAX].counters[CT_PAAS_DESYNC_POS_DIFF]) / 100.f);
}

} // namespace netstat
