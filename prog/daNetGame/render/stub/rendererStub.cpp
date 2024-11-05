// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/renderer.h"
#include <render/viewVecs.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include "render/skies.h"
#include <util/dag_string.h>
#include <gui/dag_visConsole.h>
#include <render/rendererFeatures.h>
#include <3d/dag_textureIDHolder.h>

struct Color3;
size_t framework_render_pulls = 0;

static class DebugConsoleDriver final : public console::IVisualConsoleDriver
{
public:
  void init(const char *) override {}
  void shutdown() override {}

  void render() override {}
  void update() override {}
  bool processKey(int, int, bool) override { return false; }

  void puts(const char *str, console::LineType /*type*/) override { debug("console: %s", str); }

  void show() override {}
  void hide() override {}
  bool is_visible() override { return false; }
  void reset_important_lines() {}

  real get_cur_height() override { return 1.0f; }
  // setup current console height (0.0f < h <= 1.0f)
  void set_cur_height(real) override {}
  void set_progress_indicator(const char *, const char *) override{};

  void save_text(const char *) override {}

  void setFontId(int) override {}

  virtual const char *getEditTextBeforeModify() const override { return nullptr; }
} debug_console_vis_driver;


void init_world_renderer() { console::set_visual_driver(&debug_console_vis_driver, NULL); }
void pull_render_das() {}
void init_renderer_per_game() {}
void term_renderer_per_game() {}

IRenderWorld *create_world_renderer() { return NULL; }
void destroy_world_renderer() {}

void close_world_renderer() { console::set_visual_driver(NULL, NULL); }

IRenderWorld *get_world_renderer() { return NULL; }

webui::HttpPlugin *get_renderer_http_plugins()
{
  debug("get_renderer_http_plugins() stub");
  return nullptr;
}

// console commands
void reload_cube() {}

void reinit_cube(int) {}

void prepare_last_clip() {}

void invalidate_clipmap(bool) {}

void dump_clipmap() {}
void prerun_fx() {}

bool get_sun_sph0_color(float, Color3 &, Color3 &) { return false; }


void init_tex_streaming() { const_cast<DataBlock *>(dgs_get_settings())->addBlock("texStreaming")->setBool("disableTexScan", true); }
bool have_renderer() { return false; }

void before_draw_scene(int, float, float, ecs::EntityId, TMatrix &) {}
void draw_scene(const TMatrix &) {}

void push_debug_sphere(const Point3 &) {}
void set_debug_group_name(const char *) {}
void init_device_reset() {}

void animated_splash_screen_start(bool) {}
void animated_splash_screen_stop() {}
void animated_splash_screen_draw(Texture *) {}
void debug_animated_splash_screen() {}
bool is_animated_splash_screen_started() { return false; }
bool is_animated_splash_screen_encoding() { return false; }

void start_animated_splash_screen_in_thread() {}
void stop_animated_splash_screen_in_thread() {}
bool is_animated_splash_screen_in_thread() { return false; }
void animated_splash_screen_allow_watchdog_kick(bool) {}

bool should_draw_debug_collision() { return false; }
bool should_hide_debug() { return false; }

const TextureIDHolder &init_and_get_perlin_noise_3d(Point3 &, Point3 &)
{
  static TextureIDHolder dummy;
  return dummy;
}
void release_perline_noise_3d() {}

void apply_united_vdata_settings(const DataBlock *) {}
void prepare_ri_united_vdata_setup(const DataBlock *) {}
void prepare_dynm_united_vdata_setup(const DataBlock *) {}

const char *node_based_shader_current_platform_suffix() { return ""; }
DngSkies *get_daskies() { return nullptr; }
void save_daskies(DataBlock &) {}
void load_daskies(const DataBlock &, float, const char *, int, int, int, float, float, int) {}
void select_weather_preset_delayed(const char *) {}
float get_daskies_time() { return 0.0f; }
void set_daskies_time(float /*time_of_day*/) {}
DPoint2 get_strata_clouds_origin() { return DPoint2(); }
DPoint2 get_clouds_origin() { return DPoint2(); }
void set_strata_clouds_origin(DPoint2) {}
void set_clouds_origin(DPoint2) {}

void send_gpu_net_event(const char *, int, const uint32_t *) {}

void dump_periodic_gpu_info() {}

void erase_grass(const Point3 &, float) {}
void invalidate_after_heightmap_change(const BBox3 &) {}

void remove_puddles_in_crater(const Point3 &, float) {}

void rendering_path::set_force_default(bool) {}
bool rendering_path::get_force_default() { return false; }
String get_corrected_rendering_path() { return String{}; }
