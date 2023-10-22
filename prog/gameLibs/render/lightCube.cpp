#include <3d/dag_drv3d.h>
#include <render/dynamicCube.h>
#include <3d/dag_resPtr.h>
#include <math/dag_TMatrix4.h>
#include <shaders/dag_shaders.h>
#include <math/dag_adjpow2.h>
#include <stdio.h>
#include <render/sphHarmCalc.h>
#include <shaders/dag_postFxRenderer.h>
#include <render/lightCube.h>

namespace light_probe
{
#define GLOBAL_VARS_LIST \
  VAR(num_probe_mips)    \
  VAR(integrate_face)    \
  VAR(relight_mip)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

class Cube
{
  UniqueTex dynamic_cube_specular;
  PostFxRenderer specular;
  int specularMips = -1;
  SphHarmCalc::valuesOpt_t sphHarmOpt;
  SphHarmCalc harmonicsComp;
  // SphHarmCalc::values_t sphHarm;
public:
  Cube() {}
  ~Cube() { close(); }
  void close()
  {
    specular.clear();
    dynamic_cube_specular.close();
  }
  bool init(const char *name, int specular_size, unsigned texFmt)
  {

    specular_size = get_closest_pow2(specular_size);
    char buf[256];

    if (name)
      _snprintf(buf, sizeof(buf), "%s_probe_specular", name);
    else
      _snprintf(buf, sizeof(buf), "%p_probe_specular", this);
    specularMips = get_log2w(specular_size);
    specularMips = min(specularMips, (num_probe_mipsVarId < 0) ? 7 : ShaderGlobal::get_int(num_probe_mipsVarId));
    specular.init("specular_cube");
    if ((d3d::get_texformat_usage(texFmt) & (d3d::USAGE_RTARGET | d3d::USAGE_FILTER)) != (d3d::USAGE_RTARGET | d3d::USAGE_FILTER))
    {
      logwarn("light probe tex fmt can not filter, degrade to argb8_srgb !");
      texFmt = TEXCF_SRGBREAD | TEXCF_SRGBWRITE; //
    }
    if (!specular.getMat())
    {
      texFmt |= TEXCF_GENERATEMIPS;
      logwarn("no pregen specular filter, use genmips");
    }
    dynamic_cube_specular = dag::create_cubetex(specular_size, TEXCF_RTARGET | TEXCF_CLEAR_ON_CREATE | texFmt, specularMips, buf);
    return true;
  }
  static void renderCubeTex(CubeTexture *cubTex, int face_start, int face_count, int from_mip, int mips_cnt, ManagedTex &tex,
    PostFxRenderer &shader)
  {
    d3d::set_render_target();
    bool integrate_one_face = false;

#if _TARGET_IOS || _TARGET_TVOS || _TARGET_ANDROID
    integrate_one_face = true;
#endif

    if (face_count != 6)
      integrate_one_face = true;
    else
    {
      G_ASSERT(face_start == 0);
      face_start = 0;
    }
    if (face_count != 6)
      G_ASSERTF(mips_cnt == 1, "we can't process more than 1 mip if not processing all faces");
    int stop_mip = min(from_mip + mips_cnt, tex->level_count());
    for (int mip = from_mip; mip < stop_mip; ++mip)
    {
      for (int faceNo = 0; faceNo < 6 && mip <= cubTex->level_count() && mip > 0; ++faceNo)
        d3d::resource_barrier({cubTex, RB_RO_SRV | RB_STAGE_PIXEL, (unsigned)(mip - 1 + cubTex->level_count() * faceNo), 1});
      ShaderGlobal::set_int(relight_mipVarId, mip);
      for (int faceNo = 0; faceNo < (integrate_one_face ? face_count : 1); ++faceNo)
      {
        ShaderGlobal::set_int(integrate_faceVarId, integrate_one_face ? face_start + faceNo : 6);
        for (int i = 0; i < (integrate_one_face ? 1 : 6); ++i)
          d3d::set_render_target(i, tex.getCubeTex(), face_start + faceNo + i, mip);
        if (cubTex == tex.getCubeTex())
          cubTex->texmiplevel(0, 0);
        d3d::settex(7, cubTex);
        shader.render();
        for (int i = 0; i < (integrate_one_face ? 1 : 6); ++i)
          d3d::set_render_target(i, nullptr, 0);
      }
    }
    if (cubTex == tex.getCubeTex())
      cubTex->texmiplevel(-1, -1);
  }
  bool update(CubeTexture *cubTex, int face_start, int face_count, int from_mip, int mips_count)
  {
    if (!specular.getMat())
    {
      if (cubTex && face_start == 0)
      {
        dynamic_cube_specular.getCubeTex()->update(cubTex);
        dynamic_cube_specular.getCubeTex()->generateMips();
        d3d::resource_barrier({dynamic_cube_specular.getCubeTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
      }
      return true;
    }

    if (!cubTex)
      cubTex = dynamic_cube_specular.getCubeTex();
    if (!cubTex)
      return false;
    int fromMip = clamp(from_mip, (cubTex == dynamic_cube_specular.getCubeTex()) ? 1 : 0, specularMips - 1);
    int mipCnt = min(specularMips - fromMip, mips_count);
    renderCubeTex(cubTex, face_start, face_count, fromMip, mipCnt, dynamic_cube_specular, specular);
    d3d::resource_barrier({dynamic_cube_specular.getCubeTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
    return true;
  }
  bool calcDiffuse(CubeTexture *cubTex, float gamma, float brightness, bool force_recalc)
  {
    if (!harmonicsComp.isValid())
      harmonicsComp.reset();
    if (!harmonicsComp.isValid())
      return false;

    if (!cubTex)
      cubTex = dynamic_cube_specular.getCubeTex();
    harmonicsComp.setHdrScale(brightness);
    const bool processResult = harmonicsComp.processCubeData(cubTex, gamma, force_recalc);
    if (processResult)
    {
      mem_copy_from(sphHarmOpt, harmonicsComp.getValuesOpt().data());
      hasNans = harmonicsComp.hasNans;
    }
    // mem_copy_from(sphHarm, calcDiffuse.getValues().data());

    return processResult;
  }
  const ManagedTex &getManagedTex() { return dynamic_cube_specular; }
  const Color4 *getSphHarm() { return sphHarmOpt.data(); }
  bool hasNans = false;
};

Cube *create() { return new Cube; }
void destroy(Cube *cube) { del_it(cube); }

bool init(Cube *h, const char *name, int spec_size, unsigned fmt)
{
  if (h)
    return h->init(name, spec_size, fmt);
  return false;
}

bool update(Cube *h, CubeTexture *cubTex, int face_start, int face_count, int from_mip, int mips_count)
{
  if (h)
    return h->update(cubTex, face_start, face_count, from_mip, mips_count);
  return false;
}

bool calcDiffuse(Cube *h, CubeTexture *cubTex, float gamma, float scale, bool force_recalc)
{
  if (h)
    return h->calcDiffuse(cubTex, gamma, scale, force_recalc);
  return false;
}

const ManagedTex *getManagedTex(Cube *h)
{
  if (!h)
    return NULL;
  return &h->getManagedTex();
}

const Color4 *getSphHarm(Cube *h)
{
  if (h)
    return h->getSphHarm();
  return NULL;
}

bool hasNans(Cube *h)
{
  if (h)
    return h->hasNans;
  return false;
}

}; // namespace light_probe
