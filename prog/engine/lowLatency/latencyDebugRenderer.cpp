// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_lowLatency.h>
#include <gui/dag_stdGuiRender.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderVar.h>
#include <util/dag_convar.h>

CONSOLE_BOOL_VAL("latency", debug_latency, false);
CONSOLE_INT_VAL("latency", display_refresh_time, 500, 1, 10000);
CONSOLE_INT_VAL("latency", display_max_frame_count, 32, 1, 64);

static lowlatency::LatencyData latency_data;
static uint32_t latency_display_time = 0;

static void new_line(Point2 &pos, const E3DCOLOR &color)
{
  StdGuiRender::goto_xy(pos);
  StdGuiRender::set_color(color);
  pos.y += 12;
};

static void display_value(Point2 &pos, const E3DCOLOR &color, const char *str, const lowlatency::Statistics &value)
{
  new_line(pos, color);
  StdGuiRender::draw_str(String(0, "%s: min=%3.3f max=%3.3f avg=%3.3f", str, value.min, value.max, value.avg));
};

// a,b: normalized values
static void display_interval(Point2 &pos, const E3DCOLOR &color, const char *s, float a, float b)
{
  constexpr uint32_t len = 50;
  char str[len + 1];
  str[len] = 0;
  str[0] = ':';
  for (uint32_t i = 1; i < len; ++i)
    str[i] = ' ';
  Point2 p = pos;
  new_line(pos, color);
  int aPos = static_cast<int>((len - 2) * a) + 1;
  if (aPos >= 1 && aPos <= len)
    str[aPos] = '(';
  int bPos = static_cast<int>((len - 2) * b) + 1;
  if (bPos >= 1 && bPos <= len)
    str[bPos] = aPos == bPos ? '|' : ')';
  StdGuiRender::draw_str(s);
  StdGuiRender::goto_xy(Point2(p.x + 100, p.y));
  StdGuiRender::draw_str(str);
};

static float discard_invalid_value(float val) { return val > 10 * 16.667f ? 0 : val; };

void lowlatency::render_debug_low_latency()
{
  if (debug_latency.get())
  {
    const uint32_t currentTime = get_time_msec();
    if (currentTime - latency_display_time > display_refresh_time.get() || latency_data.frameCount == 0)
    {
      latency_display_time = currentTime;
      latency_data = lowlatency::get_statistics_since(0, display_max_frame_count.get());
    }
    if (latency_data.frameCount > 0)
    {
      IPoint2 current_block;
      current_block[0] = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
      current_block[1] = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE);
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
      {
        StdGuiRender::ScopeStarter strt;
        Point2 pos(StdGuiRender::screen_size().x - 500, 100);

        StdGuiRender::set_color(E3DCOLOR(0, 0, 0, 180));
        if (lowlatency::get_supported_latency_modes() > 0)
          StdGuiRender::render_rect(pos.x - 10, pos.y - 20, pos.x + 380, pos.y + 140);
        else
          StdGuiRender::render_rect(pos.x - 10, pos.y - 20, pos.x + 380, pos.y + 20);
        if (lowlatency::get_supported_latency_modes() > 0 && latency_data.frameCount > 0)
        {
          display_value(pos, 0xFFFFFFFF, "Game to render latency", latency_data.gameToRenderLatency);
          display_value(pos, 0xFFFFFFFF, "Render latency", latency_data.renderLatency);
          display_value(pos, 0xFFFFFFFF, "Game latency", latency_data.gameLatency);

          const float intervalLength = eastl::max({discard_invalid_value(latency_data.simEndTime.avg),
            discard_invalid_value(latency_data.inputSampleTime.avg), discard_invalid_value(latency_data.renderSubmitEndTime.avg),
            discard_invalid_value(latency_data.presentEndTime.avg), discard_invalid_value(latency_data.driverEndTime.avg),
            discard_invalid_value(latency_data.osRenderQueueEndTime.avg), discard_invalid_value(latency_data.gpuRenderEndTime.avg)});

          float invLength = 1.0f / intervalLength;

          new_line(pos, 0xFFFFFFFF);
          StdGuiRender::draw_str(String(0, "Displayed interval length=%3.3f", intervalLength));

          display_interval(pos, 0xFF8888FF, "Simulation", 0, latency_data.simEndTime.avg * invLength);
          display_interval(pos, 0xFFFF88FF, "Input", latency_data.inputSampleTime.avg * invLength,
            latency_data.inputSampleTime.avg * invLength);
          display_interval(pos, 0x88FF88FF, "Render submission", latency_data.renderSubmitStartTime.avg * invLength,
            latency_data.renderSubmitEndTime.avg * invLength);
          display_interval(pos, 0x88FFFFFF, "Present", latency_data.presentStartTime.avg * invLength,
            latency_data.presentEndTime.avg * invLength);
          display_interval(pos, 0x8888FFFF, "Driver", latency_data.driverStartTime.avg * invLength,
            latency_data.driverEndTime.avg * invLength);
          display_interval(pos, 0xFF88FFFF, "Os render queue", latency_data.osRenderQueueStartTime.avg * invLength,
            latency_data.osRenderQueueEndTime.avg * invLength);
          display_interval(pos, 0x888888FF, "Gpu render", latency_data.gpuRenderStartTime.avg * invLength,
            latency_data.gpuRenderEndTime.avg * invLength);
        }
      }
      ShaderGlobal::setBlock(current_block[0], ShaderGlobal::LAYER_FRAME);
      ShaderGlobal::setBlock(current_block[1], ShaderGlobal::LAYER_SCENE);
    }
  }
}

// I needed to move this definition here, because
// the console vars are not visible otherwise in War Thunder
lowlatency::ScopedLatencyMarker::ScopedLatencyMarker(uint32_t frame_id, LatencyMarkerType start_marker, LatencyMarkerType end_marker) :
  frameId(frame_id), endMarker(end_marker)
{
  TIME_PROFILE(ScopedLatencyMarker);

  set_marker(frameId, start_marker);
}