// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/hdrDecoder.h>

#include <shaders/dag_shaders.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_statDrv.h>
#include <render/hdrRender.h>
#include <drv/3d/dag_renderTarget.h>

#define GLOBAL_VARS_LIST VAR(paper_white_nits)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

// Same as in render_options.nut
constexpr int MIN_PAPER_WHITE_NITS = 100;
constexpr int MAX_PAPER_WHITE_NITS = 1000;
constexpr int DEF_PAPER_WHITE_NITS = 200;

static bool int10_hdr_buffer() { return d3d::driver_command(Drv3dCommand::INT10_HDR_BUFFER); }

static void update_paper_white_nits(uint32_t value) { ShaderGlobal::set_real(paper_white_nitsVarId, value); }

HDRDecoder::HDRDecoder(const DataBlock &videoCfg)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR

  uint32_t paper_white_nits = videoCfg.getInt("paperWhiteNits", DEF_PAPER_WHITE_NITS);
  if (paper_white_nits < MIN_PAPER_WHITE_NITS || paper_white_nits > MAX_PAPER_WHITE_NITS)
    paper_white_nits = DEF_PAPER_WHITE_NITS;
  update_paper_white_nits(paper_white_nits);

  decodeHDRRenderer = eastl::make_unique<PostFxRenderer>();
  decodeHDRRenderer->init("decode_hdr_to_sdr");

  int width, height;
  d3d::get_render_target_size(width, height, 0, 0);
  int fpFormat = int10_hdr_buffer() ? TEXFMT_A2B10G10R10 : TEXFMT_A16B16G16R16F;
  copyTex = dag::create_tex(nullptr, width, height, fpFormat | TEXCF_RTARGET, 1, "hdr_tex");
  sdrTex = dag::create_tex(nullptr, width, height, TEXFMT_A8R8G8B8 | TEXCF_RTARGET, 1, "sdr_tex");
}

char *HDRDecoder::LockResult(int &stride, int &w, int &h)
{
  char *p = nullptr;
  d3d::resource_barrier({sdrTex.getTex2D(), RB_RO_GENERIC_READ_TEXTURE, 0, 0});
  TextureInfo info;
  sdrTex.getTex2D()->getinfo(info);
  w = info.w;
  h = info.h;
  sdrTex.getTex2D()->lockimg((void **)&p, stride, 0, TEXLOCK_READ);
  locked = true;
  return p;
}

bool HDRDecoder::IsActive() { return hdrrender::is_active(); }

void HDRDecoder::Decode()
{
  G_ASSERT_RETURN(decodeHDRRenderer, );

  TIME_D3D_PROFILE(hdr_decode);
  if (locked)
  {
    sdrTex->unlockimg();
    locked = false;
  }
  d3d::copy_from_current_render_target(copyTex.getTex2D());
  SCOPE_RENDER_TARGET;
  copyTex.setVar();
  d3d::resource_barrier({copyTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  d3d::resource_barrier({sdrTex.getTex2D(), RB_RW_RENDER_TARGET, 0, 0});
  d3d::set_render_target(sdrTex.getTex2D(), 0);
  decodeHDRRenderer->render();
}

HDRDecoder::~HDRDecoder()
{
  if (locked)
    sdrTex->unlockimg();
  sdrTex.close();
  copyTex.close();
  decodeHDRRenderer.reset();
}
