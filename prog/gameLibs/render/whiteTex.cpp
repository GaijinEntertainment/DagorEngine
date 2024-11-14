// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/whiteTex.h>
#include <drv/3d/dag_driver.h>
#include <image/dag_texPixel.h>
#include <startup/dag_restart.h>
#include <generic/dag_initOnDemand.h>

class WhiteRestartProc : public SRestartProc
{
public:
  virtual const char *procname() { return "whiteTex"; }
  WhiteRestartProc() : SRestartProc(RESTART_VIDEO) {}

  void startup() {}
  void shutdown();
};

static InitOnDemand<WhiteRestartProc> white_base_rproc;
static UniqueTex white_tex;
static d3d::SamplerHandle white_sampler = d3d::INVALID_SAMPLER_HANDLE;

static void startup_white_tex()
{
  white_base_rproc.demandInit();
  add_restart_proc(white_base_rproc);
  white_base_rproc->startupf(RESTART_VIDEO);
  TexImage32 image[2];
  image[0].w = image[0].h = 1;
  *(E3DCOLOR *)(image + 1) = 0xFFFFFFFF;

  white_tex = UniqueTex(dag::create_tex(image, 1, 1, TEXCF_RGB | TEXCF_LOADONCE | TEXCF_SYSTEXCOPY, 1, "gui_whiteTex"), "whiteTex");
}

void WhiteRestartProc::shutdown() { white_tex.close(); }

const UniqueTex &get_white_on_demand()
{
  if (!white_tex)
  {
    startup_white_tex();
  }
  return white_tex;
}

BaseTexture *get_white_tex_on_demand() { return get_white_on_demand().getBaseTex(); }
d3d::SamplerHandle get_white_sampler_on_demand()
{
  if (white_sampler == d3d::INVALID_SAMPLER_HANDLE)
    white_sampler = d3d::request_sampler({});
  return white_sampler;
}