// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "soundOcclusionGPU_internal.h"

#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <math/dag_color.h>
#include <stdio.h>

static ShaderVariableInfo snd_occ_sdf_listenerVarId_dbg("snd_occ_sdf_listener", true);

namespace snd_occlusion_gpu
{

void GpuSoundOcclusion::debug_set_enabled(bool enabled) { debugEnabled = enabled; }
bool GpuSoundOcclusion::debug_is_enabled() const { return debugEnabled; }


static E3DCOLOR occlusion_to_color(float occ)
{
  float c = (occ < 0.0f) ? 0.0f : (occ > 1.0f ? 1.0f : occ);
  int r = (int)(((c * 2.0f < 1.0f) ? c * 2.0f : 1.0f) * 255);
  float gf = c * 2.0f - 1.0f;
  gf = (gf < 0.0f) ? 0.0f : (gf > 1.0f ? 1.0f : gf);
  int g = (int)((1.0f - gf) * 255);
  return E3DCOLOR(r, g, 0, 200);
}


void GpuSoundOcclusion::debug_render_3d()
{
  if (!inited || !debugEnabled)
    return;

  set_cached_debug_lines_wtm(TMatrix::IDENT);
  begin_draw_cached_debug_lines(false, false);

  // Listener sphere (blue)
  if (lengthSq(listenerPos) > 0.001f)
    draw_cached_debug_sphere(listenerPos, 0.3f, E3DCOLOR(50, 50, 255, 200));

  // Source spheres and trace lines
  for (int si = 0; si < cfg.maxSources; si++)
  {
    if (!sources[si].active)
      continue;

    const Point3 &spos = sources[si].pos;
    float sradius = sources[si].radius;

    float occ = 0;
    if (results[si].valid)
      occ = results[si].values[0];

    E3DCOLOR color = occlusion_to_color(occ);
    draw_cached_debug_sphere(spos, sradius > 0.1f ? sradius : 0.3f, color);

    // Trace line from listener
    if (lengthSq(listenerPos) > 0.001f)
      draw_cached_debug_line(listenerPos, spos, color);

    // Text label
    char buf[64];
    if (sources[si].mode == MODE_DIRECT_REVERB)
    {
      float rev = results[si].valid ? results[si].values[1] : 0;
      snprintf(buf, sizeof(buf), "D:%.2f R:%.2f", occ, rev);
    }
    else
    {
      snprintf(buf, sizeof(buf), "occ: %.2f", occ);
    }
    add_debug_text_mark(spos, buf, -1, 0.5f);
  }

  end_draw_cached_debug_lines();
}


void GpuSoundOcclusion::debug_render_imgui()
{
  // ImGui rendering is driven by testGI's DECLARE_* panel system.
}


// --- Free functions delegating to global instance ---

void debug_render_3d() { gpu_sound_occlusion.debug_render_3d(); }
void debug_render_imgui() { gpu_sound_occlusion.debug_render_imgui(); }
void debug_set_enabled(bool enabled) { gpu_sound_occlusion.debug_set_enabled(enabled); }
bool debug_is_enabled() { return gpu_sound_occlusion.debug_is_enabled(); }

static eastl::unique_ptr<PostFxRenderer> worldSdfShadowRenderer;

void world_sdf_debug_render_shadow(const Point3 &listener)
{
  if (!worldSdfShadowRenderer || !worldSdfShadowRenderer->getElem())
  {
    worldSdfShadowRenderer.reset(new PostFxRenderer);
    worldSdfShadowRenderer->init("sound_occlusion_world_sdf_shadow");
    if (!worldSdfShadowRenderer->getElem())
    {
      worldSdfShadowRenderer.reset();
      return;
    }
  }

  snd_occ_sdf_listenerVarId_dbg.set_float4(listener.x, listener.y, listener.z, 0);
  worldSdfShadowRenderer->render();
}

void world_sdf_debug_close() { worldSdfShadowRenderer.reset(); }

} // namespace snd_occlusion_gpu
