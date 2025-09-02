// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_color.h>


namespace dafg::multiplexing
{
struct Index;
}

struct CameraParams;
namespace camera_in_camera
{
void setup(const bool has_feature);
bool activate_view();
void update_transforms(const CameraParams &main_view, const CameraParams &prev_main_view, const CameraParams &lens_view);
bool is_lens_render_active();
bool is_lens_only_zoom_enabled();
bool is_main_view(const dafg::multiplexing::Index &);

class RenderMainViewOnly
{
public:
  explicit RenderMainViewOnly(const dafg::multiplexing::Index &index);
  ~RenderMainViewOnly();

private:
  bool isMainView = true;
  bool isCamCamRenderActive = false;
};

class ApplyMasterState
{
public:
  explicit ApplyMasterState(const dafg::multiplexing::Index &index);
  ApplyMasterState(const dafg::multiplexing::Index &index, const CameraParams &view);
  ~ApplyMasterState();

private:
  bool isMainView = true;
  bool hasStencilTest = false;
  bool hasSavedState = false;

  Color4 savedViewVecLT;
  Color4 savedViewVecRT;
  Color4 savedViewVecLB;
  Color4 savedViewVecRB;
};

enum
{
  USE_STENCIL = 1
};

class ApplyPostfxState
{
public:
  ApplyPostfxState(const dafg::multiplexing::Index &, const CameraParams &, bool use_stencil = false);
  ApplyPostfxState(
    const dafg::multiplexing::Index &, const CameraParams &cur_view, const CameraParams &prev_view, bool use_stencil = false);
  ~ApplyPostfxState();

private:
  bool lensRenderActive = false;
  bool isMainView = true;
  bool hasStencilTest = false;
  bool hasPrevState = false;

  Color4 savedProjtmPsf0;
  Color4 savedProjtmPsf1;
  Color4 savedProjtmPsf2;
  Color4 savedProjtmPsf3;

  Color4 savedPrevGlobtmPsf0;
  Color4 savedPrevGlobtmPsf1;
  Color4 savedPrevGlobtmPsf2;
  Color4 savedPrevGlobtmPsf3;

  Color4 savedPrevGlobtmNoOfsPsf0;
  Color4 savedPrevGlobtmNoOfsPsf1;
  Color4 savedPrevGlobtmNoOfsPsf2;
  Color4 savedPrevGlobtmNoOfsPsf3;

  Color4 savedGlobtmPsf0;
  Color4 savedGlobtmPsf1;
  Color4 savedGlobtmPsf2;
  Color4 savedGlobtmPsf3;

  Color4 savedGlobtmNoOfsPsf0;
  Color4 savedGlobtmNoOfsPsf1;
  Color4 savedGlobtmNoOfsPsf2;
  Color4 savedGlobtmNoOfsPsf3;

  Color4 savedViewVecLT;
  Color4 savedViewVecRT;
  Color4 savedViewVecLB;
  Color4 savedViewVecRB;
};
} // namespace camera_in_camera