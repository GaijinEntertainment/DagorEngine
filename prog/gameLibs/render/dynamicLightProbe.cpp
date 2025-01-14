// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_statDrv.h>
#include <render/dynamicLightProbe.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <math/dag_mathUtils.h>
#include <3d/dag_resPtr.h>
#include <render/lightCube.h>
#include <util/dag_string.h>

#define NUM_CUBE_FACES 6
enum
{
  VALID0 = 1,
  VALID1 = 2,
  VALID2 = 4
};

#define GLOBAL_VARS_LIST      \
  VAR(dynamic_cube_tex_blend) \
  VAR(dynamic_cube_tex_level) \
  VAR(dynamic_cube_tex_1)     \
  VAR(dynamic_cube_tex_2)     \
  VAR(probetm)                \
  VAR(blend_face_no)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

/* 20 gives space for:
   - 0-5: downsampling mip 1 faces one by one
   - 6:   downsampling mip 2 faces 0-2
   - 7:   downsampling mip 2 faces 3-5
   - 8+:  downsampling all mip 3+ faces
*/
enum
{
  UPDATE_FACE = 0,
  PREPARE_MIPS = UPDATE_FACE + NUM_CUBE_FACES,
  BLENDING = PREPARE_MIPS + 19,
  READY
};

DynamicLightProbe::DynamicLightProbe()
{
  cubeIndex = 0;
  mode = Mode::Combined;

  blendCubesRenderer = NULL;
  currentBlendTime = -1;
  totalBlendTime = 1.0f;

  currentProbe = NULL;
  valid = 0;
  currentProbeId = 0;
  state = UPDATE_FACE;

  cockpitTm_inv[0] = cockpitTm_inv[1] = TMatrix4::IDENT;
}

void DynamicLightProbe::init(unsigned int size, const char *name, unsigned fmt, int cube_index, Mode _mode)
{
  close();

  cubeIndex = cube_index;
  mode = _mode;

  if ((mode == DynamicLightProbe::Mode::Separated) && (fmt == TEXFMT_R11G11B10F))
    fmt = TEXFMT_A2R10G10B10;

  int probeNum = (mode == Mode::Separated) ? 3 : 2;
  for (int i = 0; i < probeNum; i++)
  {
    probe[i] = light_probe::create(String(128, "%s_p%i", name, i), size, fmt);
    G_ASSERT(probe[i]);
    if (!probe[i])
      return;
  }

  blendCubesRenderer = create_postfx_renderer("blend_light_probes");

  if (blendCubesRenderer)
  {
    int mip_count = light_probe::getManagedTex(probe[0])->getCubeTex()->level_count();
    result = dag::create_cubetex(size, fmt | TEXCF_RTARGET, mip_count, String(128, "%s_result_specular_cube", name));
  }
  else
    logwarn("no blend_light_probes shader");

  currentProbe = NULL;
  state = 0;
  valid = 0;
#define VAR(a) a##VarId = get_shader_variable_id(#a, result.getCubeTex() ? false : true);
  GLOBAL_VARS_LIST
#undef VAR
}

void DynamicLightProbe::close()
{
  del_it(blendCubesRenderer);
  result.close();
  for (auto &p : probe)
  {
    light_probe::destroy(p);
    p = NULL;
  }
  currentProbeId = 0;
}


DynamicLightProbe::~DynamicLightProbe() { close(); }


bool DynamicLightProbe::refresh(float total_blend_time)
{
  if (state != READY)
    return false;

  state = UPDATE_FACE;

  totalBlendTime = currentBlendTime = total_blend_time;

  return true;
}


void DynamicLightProbe::invalidate() { valid = 0; }


void DynamicLightProbe::update(float dt, IRenderLightProbeFace *cb, bool cockpit, const TMatrix4 &cockpitTm, bool force_flush)
{
  if (!probe[0])
    return;

  Driver3dRenderTarget prevRt;

  unsigned check = (((mode == Mode::Combined) || cockpit) ? (VALID0 | VALID1) : 0) | ((mode == Mode::Separated) ? VALID2 : 0);
  if ((valid & check) != check)
  {
    d3d::get_render_target(prevRt);
    if (mode == Mode::Separated)
    {
      if (((valid & (VALID0 | VALID1)) != (VALID0 | VALID1)) && ((check & (VALID0 | VALID1)) == (VALID0 | VALID1)))
      {
        const ManagedTex *source = light_probe::getManagedTex(probe[2]);
        ShaderGlobal::set_texture(dynamic_cube_tex_1VarId, *source);
        TMatrix4 probeTm = (cockpitTm * cockpitTm_inv[0]).transpose();
        ShaderGlobal::set_float4x4(probetmVarId, probeTm);

        cockpitTm_inv[1] = inverse44(cockpitTm);
        CubeTexture *target = light_probe::getManagedTex(probe[1 - currentProbeId])->getCubeTex();
        for (int f = 0; f < NUM_CUBE_FACES; ++f)
        {
          ShaderGlobal::set_int(blend_face_noVarId, f);
          cb->renderLightProbeFace(target, 2 + f, f,
            IRenderLightProbeFace::Element::Cockpit | IRenderLightProbeFace::Element::Resolve | IRenderLightProbeFace::Element::Blend,
            TMatrix4::IDENT);
        }
        light_probe::update(probe[1 - currentProbeId], target);
        light_probe::update(probe[currentProbeId], target);
        currentProbeId = 1 - currentProbeId;
        currentProbe = light_probe::getManagedTex(probe[currentProbeId]);
        valid |= VALID0 | VALID1;
        state = READY;
      }
      if (((valid & VALID2) != VALID2) && ((check & VALID2) == VALID2))
      {
        cockpitTm_inv[0] = inverse44(cockpitTm);
        CubeTexture *target = light_probe::getManagedTex(probe[2])->getCubeTex();
        for (int f = 0; f < NUM_CUBE_FACES; ++f)
          cb->renderLightProbeFace(target, cubeIndex, f, IRenderLightProbeFace::Element::Envi | IRenderLightProbeFace::Element::Copy,
            TMatrix::IDENT);
        light_probe::update(probe[2], target);
        valid |= VALID2;
        state = READY;
      }
    }
    else if (mode == Mode::Combined)
    {
      CubeTexture *target = light_probe::getManagedTex(probe[1 - currentProbeId])->getCubeTex();
      for (int f = 0; f < NUM_CUBE_FACES; ++f)
        cb->renderLightProbeFace(target, cubeIndex, f, IRenderLightProbeFace::Element::All, TMatrix4::IDENT);
      light_probe::update(probe[1 - currentProbeId], target);
      light_probe::update(probe[currentProbeId], target);
      currentProbeId = 1 - currentProbeId;
      currentProbe = light_probe::getManagedTex(probe[currentProbeId]);
      valid |= VALID0 | VALID1;
      state = READY;
    }
    d3d::set_render_target(prevRt);
  }

  if ((mode == Mode::Separated) && !cockpit)
  {
    currentProbe = light_probe::getManagedTex(probe[2]);
    state = READY;
  }

  if (state == READY)
    return;

  d3d::get_render_target(prevRt);
  if (state >= BLENDING)
  {
    TIME_D3D_PROFILE(blend);
    G_ASSERTF(valid & (VALID0 | VALID1), "make light probe blending");
    currentBlendTime -= dt;
    float blend = totalBlendTime ? clamp(1.0f - currentBlendTime / totalBlendTime, 0.0f, 1.0f) : 1;
    ShaderGlobal::set_real(dynamic_cube_tex_blendVarId, blend);
    const ManagedTex *probe1 = light_probe::getManagedTex(probe[currentProbeId]);
    const ManagedTex *probe2 = light_probe::getManagedTex(probe[1 - currentProbeId]);
    const ManagedTex *blendTo = &result;
    ShaderGlobal::set_texture(dynamic_cube_tex_1VarId, *probe1);
    ShaderGlobal::set_texture(dynamic_cube_tex_2VarId, *probe2);

    for (int level = 0; level < (*blendTo)->level_count(); ++level)
    {
      ShaderGlobal::set_real(dynamic_cube_tex_levelVarId, level);
      G_ASSERT(d3d::get_driver_desc().maxSimRT >= 6);
      for (int f = 0; f < 6; ++f)
        d3d::set_render_target(f, blendTo->getCubeTex(), f, level);
      blendCubesRenderer->render();
    }
    d3d::resource_barrier({blendTo->getCubeTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

    currentProbe = blendTo;
    if (blend >= 1.0f)
    {
      currentProbeId = 1 - currentProbeId;
      state = READY;
    }
  }
  else if (state >= PREPARE_MIPS && state < BLENDING)
  {
    int innerState = state - PREPARE_MIPS;

    char mipName[] = "prepare_mips_00";
    mipName[13] += innerState / 10;
    mipName[14] += innerState % 10;
    TIME_D3D_PROFILE_NAME(prepare_mips, mipName);

    int probeId = 1 - currentProbeId;
    light_probe::Cube *source = probe[probeId];
    CubeTexture *target = light_probe::getManagedTex(source)->getCubeTex();
    bool update_all_faces = !result.getCubeTex() || currentBlendTime <= 0 || !(valid & (1 << (probeId)));

    if (update_all_faces)
      light_probe::update(source, target, 0, 6);
    else
    {
      switch (innerState)
      {
        case 0: // face 0, mip 1
        case 1: // face 1, mip 1
        case 2: // face 2, mip 1
        case 3: // face 3, mip 1
        case 4: // face 4, mip 1
        case 5: // face 5, mip 1
          light_probe::update(source, target, state - PREPARE_MIPS, 1, 1, 1);
          break;
        case 6: // face 0, mip 2
          light_probe::update(source, target, 0, 1, 2, 1);
          break;
        case 7: // face 1, mip 2
          light_probe::update(source, target, 1, 1, 2, 1);
          break;
        case 8: // face 2, mip 2
          light_probe::update(source, target, 2, 1, 2, 1);
          break;
        case 9: // face 3, mip 2
          light_probe::update(source, target, 3, 1, 2, 1);
          break;
        case 10: // face 4, mip 2
          light_probe::update(source, target, 4, 1, 2, 1);
          break;
        case 11: // face 5, mip 2
          light_probe::update(source, target, 5, 1, 2, 1);
          if (target->level_count() < 3)
            state = BLENDING - 1;
          break;
        case 12: // face 0-1, mip 3
          light_probe::update(source, target, 0, 2, 3, 1);
          break;
        case 13: // face 2-3, mip 3
          light_probe::update(source, target, 2, 2, 3, 1);
          break;
        case 14: // face 4-5, mip 3
          light_probe::update(source, target, 4, 2, 3, 1);
          if (target->level_count() < 4)
            state = BLENDING - 1;
          break;
        case 15: // face 0-2, mip 4
          light_probe::update(source, target, 0, 3, 4, 1);
          break;
        case 16: // face 3-5, mip 4
          light_probe::update(source, target, 3, 3, 4, 1);
          if (target->level_count() < 5)
            state = BLENDING - 1;
          break;
        case 17: // face 0-3, mip 5
          light_probe::update(source, target, 0, 4, 5, 1);
          break;
        case 18: // face 4-5, mip 5 and face 0-1, mip 6
          light_probe::update(source, target, 4, 2, 5, 1);
          if (target->level_count() < 6)
          {
            state = BLENDING - 1;
            break;
          }
          light_probe::update(source, target, 0, 2, 6, 1);
          break;
        case 19: // face 2-5, mip 6
          light_probe::update(source, target, 2, 4, 6, 1);
          break;
      }
    }

    state++;
    if (update_all_faces)
    {
      currentProbeId = 1 - currentProbeId;
      currentProbe = light_probe::getManagedTex(probe[currentProbeId]);
      valid |= (1 << currentProbeId);
      state = (currentBlendTime > 0) && ((valid & (VALID0 | VALID1)) == (VALID0 | VALID1)) ? BLENDING : READY;
    }
  }
  else
  {
    TIME_D3D_PROFILE(update_face);
    if (force_flush)
      d3d::driver_command(Drv3dCommand::FLUSH_STATES);
    if (mode == Mode::Separated)
    {
      if (cockpit)
      {
        const ManagedTex *source = light_probe::getManagedTex(probe[2]);
        ShaderGlobal::set_texture(dynamic_cube_tex_1VarId, *source);
        TMatrix4 probeTm = (cockpitTm * cockpitTm_inv[0]).transpose();
        ShaderGlobal::set_float4x4(probetmVarId, probeTm);

        CubeTexture *target = light_probe::getManagedTex(probe[1 - currentProbeId])->getCubeTex();
        TMatrix4 gbufferTm = (cockpitTm_inv[1] * cockpitTm).transpose();
        ShaderGlobal::set_int(blend_face_noVarId, state);
        cb->renderLightProbeFace(target, 2 + state, state,
          IRenderLightProbeFace::Element::Resolve | IRenderLightProbeFace::Element::Blend, gbufferTm);
      }
    }
    else if (mode == Mode::Combined)
    {
      CubeTexture *target = light_probe::getManagedTex(probe[1 - currentProbeId])->getCubeTex();
      cb->renderLightProbeFace(target, cubeIndex, state, IRenderLightProbeFace::Element::All, TMatrix4::IDENT);
    }
    if (force_flush)
      d3d::driver_command(Drv3dCommand::FLUSH_STATES);
    state++;
  }
  d3d::set_render_target(prevRt);
}
