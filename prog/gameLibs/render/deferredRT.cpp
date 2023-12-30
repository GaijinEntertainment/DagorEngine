#include <render/deferredRenderer.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix4.h>
#include <math/integer/dag_IPoint2.h>
#include <util/dag_string.h>
#include <perfMon/dag_statDrv.h>

#define GLOBAL_VARS_LIST      \
  VAR(screen_pos_to_texcoord) \
  VAR(screen_size)            \
  VAR(gbuffer_view_size)      \
  VAR(albedo_gbuf)            \
  VAR(normal_gbuf)            \
  VAR(material_gbuf)          \
  VAR(motion_gbuf)            \
  VAR(depth_gbuf)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

void DeferredRT::close()
{
  for (int i = 0; i < numRt; ++i)
  {
    mrts[i].close();
  }
  depth.close();
  numRt = 0;
}

void DeferredRT::setRt()
{
  d3d::set_render_target();
  for (int i = 0; i < numRt; ++i)
    d3d::set_render_target(i, mrts[i].getTex2D(), 0);
  if (depth.getTex2D())
    d3d::set_depth(depth.getTex2D(), DepthAccess::RW);
}

void DeferredRT::setVar()
{
  ShaderGlobal::set_texture(albedo_gbufVarId, mrts[0]);
  ShaderGlobal::set_texture(normal_gbufVarId, mrts[1]);
  ShaderGlobal::set_texture(material_gbufVarId, mrts[2]);

  if (numRt > 3)
    ShaderGlobal::set_texture(motion_gbufVarId, mrts[3]);

  ShaderGlobal::set_texture(depth_gbufVarId, depth);
  ShaderGlobal::set_color4(screen_pos_to_texcoordVarId, 1.f / width, 1.f / height, HALF_TEXEL_OFSF / width, HALF_TEXEL_OFSF / height);
  ShaderGlobal::set_color4(screen_sizeVarId, width, height, 1.0 / width, 1.0 / height);
  ShaderGlobal::set_int4(gbuffer_view_sizeVarId, width, height, 0, 0);
}

uint32_t DeferredRT::recreateDepthInternal(uint32_t targetFmt)
{
  if (!(d3d::get_texformat_usage(targetFmt) & d3d::USAGE_DEPTH))
  {
    debug("not supported depth format 0x%08x, fallback to TEXFMT_DEPTH24", targetFmt);
    targetFmt = (targetFmt & (~TEXFMT_MASK)) | TEXFMT_DEPTH24;
  }
  uint32_t currentFmt = 0;
  if (depth.getTex2D())
  {
    TextureInfo tinfo;
    depth.getTex2D()->getinfo(tinfo, 0);
    currentFmt = tinfo.cflg & (TEXFMT_MASK | TEXCF_SAMPLECOUNT_MASK | TEXCF_TC_COMPATIBLE);
    targetFmt |= currentFmt & (~TEXFMT_MASK);
  }
  if (currentFmt == targetFmt)
    return currentFmt;

  depth.close();

  auto cs = calcCreationSize();

  const uint32_t flags = TEXCF_RTARGET;
  String depthName(128, "%s_intzDepthTex", name);
  TexPtr depthTex = dag::create_tex(NULL, cs.x, cs.y, targetFmt | flags, 1, depthName);

  if (!depthTex && (targetFmt & TEXFMT_MASK) != TEXFMT_DEPTH24)
  {
    debug("can't create depth format 0x%08x, fallback to TEXFMT_DEPTH24", targetFmt);
    targetFmt = (targetFmt & (~TEXFMT_MASK)) | TEXFMT_DEPTH24;
    depthTex = dag::create_tex(NULL, cs.x, cs.y, targetFmt | flags, 1, depthName);
  }

  if (!depthTex)
  {
    DAG_FATAL("can't create intzDepthTex (INTZ, DF24, RAWZ) due to err '%s'", d3d::get_last_error());
  }

  depth = eastl::move(depthTex);

  depth.getTex2D()->texfilter(TEXFILTER_POINT);
  depth.getTex2D()->texaddr(TEXADDR_CLAMP);

  return targetFmt;
}

IPoint2 DeferredRT::calcCreationSize() const
{
  switch (stereoMode)
  {
    case StereoMode::MonoOrMultipass: return IPoint2(width, height);
    case StereoMode::SideBySideHorizontal: return IPoint2(width * 2, height);
    case StereoMode::SideBySideVertical: return IPoint2(width, height * 2);
  }

  // Thanks Linux!
  return IPoint2(1, 1);
}

uint32_t DeferredRT::recreateDepth(uint32_t targetFmt) { return recreateDepthInternal(targetFmt); }

DeferredRT::DeferredRT(const char *name_, int w, int h, StereoMode stereo_mode, unsigned msaaFlag, int num_rt, const unsigned *texFmt,
  uint32_t depth_fmt) :
  stereoMode(stereo_mode)
{
  if (screen_sizeVarId < 0)
  {
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
    GLOBAL_VARS_LIST
#undef VAR
  }

  strncpy(name, name_, sizeof(name));
  uint32_t currentFmt = depth_fmt;
  close();
  width = w;
  height = h;

  if (depth_fmt)
    recreateDepthInternal(currentFmt | msaaFlag);

  auto cs = calcCreationSize();

  numRt = num_rt;
  for (int i = numRt - 1; i >= 0; --i)
  {
    String mrtName(128, "%s_mrt_%d", name, i);
    unsigned mrtFmt = texFmt ? texFmt[i] : TEXFMT_A8R8G8B8;
    auto mrtTex = dag::create_tex(NULL, cs.x, cs.y, mrtFmt | TEXCF_RTARGET | msaaFlag, 1, mrtName.str());
    d3d_err(!!mrtTex);
    mrtTex->texaddr(TEXADDR_CLAMP);
    mrtTex->texfilter(TEXFILTER_POINT);
    mrts[i] = ResizableTex(std::move(mrtTex), mrtName);
  }
}

void DeferredRT::changeResolution(const int w, const int h)
{
  width = w;
  height = h;

  auto cs = calcCreationSize();

  for (int i = 0; i < numRt; ++i)
  {
    mrts[i].resize(cs.x, cs.y);
  }
  depth.resize(cs.x, cs.y);
}
