// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/fx/lightning.h>
#include <perfMon/dag_statDrv.h>
#include <ioSys/dag_dataBlock.h>
#include <math/random/dag_random.h>
#include <math.h>

static inline Point4 point4(const Point3 &xyz, float w) { return Point4(xyz.x, xyz.y, xyz.z, w); }

Lightning::Lightning()
{
  renderer.init("screenspace_lightning_flash");
  lightning_aura_brightness_VarId = ::get_shader_variable_id("lightning_aura_brightness", false);
  lightning_aura_radius_VarId = ::get_shader_variable_id("lightning_aura_radius", false);
  lightning_brightnesss_VarId = ::get_shader_variable_id("lightning_brightness", false);
  lightning_width_VarId = ::get_shader_variable_id("lightning_width", false);
  lightning_seg_num_VarId = ::get_shader_variable_id("lightning_segment_num", false);
  lightning_seg_len_VarId = ::get_shader_variable_id("lightning_segment_len", false);
  lightning_pos_VarId = ::get_shader_variable_id("lightning_pos", false);
  lightning_tremble_freq_VarId = ::get_shader_variable_id("lightning_tremble_freq", false);
  lightning_color_VarId = ::get_shader_variable_id("lightning_color", false);
  lightning_deadzone_radius_VarId = ::get_shader_variable_id("lightning_deadzone_radius", false);
}

void Lightning::render()
{
  if (params.enabled)
  {
    float current_time = get_shader_global_time();
    if (strike_start_time_ + strike_time_ > current_time)
    {
      renderer.render();
      return;
    }
    if (strike_end_time_ < strike_start_time_)
    {
      strike_end_time_ = strike_start_time_ + strike_time_;
      sleep_time_ = rnd_float(params.sleep_min_time, params.sleep_max_time);
    }
    if (strike_end_time_ + sleep_time_ < current_time)
    {
      strike_start_time_ = current_time;
      strike_time_ = rnd_float(params.strike_min_time, params.strike_max_time);
      return;
    }
  }
}

void Lightning::setParams(LightningParams p)
{
  params = p;
  ShaderGlobal::set_real(lightning_aura_brightness_VarId, params.aura_brightness);
  ShaderGlobal::set_real(lightning_aura_radius_VarId, params.aura_radius);
  ShaderGlobal::set_real(lightning_brightnesss_VarId, params.brightness);
  ShaderGlobal::set_real(lightning_width_VarId, params.width);
  ShaderGlobal::set_int(lightning_seg_num_VarId, params.segment_num);
  ShaderGlobal::set_real(lightning_seg_len_VarId, params.segment_len);
  ShaderGlobal::set_real(lightning_pos_VarId, params.pos);
  ShaderGlobal::set_real(lightning_tremble_freq_VarId, params.tremble_freq);
  ShaderGlobal::set_color4(lightning_color_VarId, point4(params.color, 1.0));
  ShaderGlobal::set_real(lightning_deadzone_radius_VarId, params.deadzone_radius);
}

void LightningParams::write(DataBlock &blk) const
{
  blk.setBool("enabled", enabled);
  blk.setReal("width", width);
  blk.setReal("brightness", brightness);
  blk.setReal("deadzone_radius", deadzone_radius);
  blk.setReal("aura_brightness", aura_brightness);
  blk.setReal("aura_radius", aura_radius);
  blk.setInt("segment_num", segment_num);
  blk.setReal("segment_len", segment_len);
  blk.setReal("strike_max_time", strike_max_time);
  blk.setReal("strike_min_time", strike_min_time);
  blk.setReal("sleep_max_time", sleep_max_time);
  blk.setReal("sleep_min_time", sleep_min_time);
  blk.setReal("pos", pos);
  blk.setReal("tremble_freq", tremble_freq);
  blk.setPoint3("color", color);
}

LightningParams LightningParams::read(const DataBlock &blk)
{
  LightningParams result;
  if (!blk.isEmpty())
  {
    result.enabled = blk.getBool("enabled", result.enabled);
    result.width = blk.getReal("width", result.width);
    result.brightness = blk.getReal("brightness", result.brightness);
    result.deadzone_radius = blk.getReal("deadzone_radius", result.deadzone_radius);
    result.aura_brightness = blk.getReal("aura_brightness", result.aura_brightness);
    result.aura_radius = blk.getReal("aura_radius", result.aura_radius);
    result.segment_num = blk.getInt("segment_num", result.segment_num);
    result.segment_len = blk.getReal("segment_len", result.segment_len);
    result.tremble_freq = blk.getReal("tremble_freq", result.tremble_freq);
    result.strike_max_time = blk.getReal("strike_max_time", result.strike_max_time);
    result.strike_min_time = blk.getReal("strike_min_time", result.strike_min_time);
    result.sleep_max_time = blk.getReal("sleep_max_time", result.sleep_max_time);
    result.sleep_min_time = blk.getReal("sleep_min_time", result.sleep_min_time);
    result.pos = blk.getReal("pos", result.pos);
    result.color = blk.getPoint3("color", result.color);
  }
  return result;
}