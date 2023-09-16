#include <render/motionBlur.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_render.h>
#include <perfMon/dag_statDrv.h>
#include <debug/dag_debug.h>
#include <render/viewVecs.h>
#include <3d/dag_lockSbuffer.h>

#define MOTION_BLUR_VARS      \
  VAR(forward_blur)           \
  VAR(velocity_mul)           \
  VAR(max_velocity)           \
  VAR(alpha_mul_on_apply)     \
  VAR(overscan_texcoord)      \
  VAR(prev_globtm)            \
  VAR(source_color_tex)       \
  VAR(accumulation_tex)       \
  VAR(blur_depth_tex)         \
  VAR(blur_source_tex)        \
  VAR(motion_blur_depth_type) \
  VAR(blur_motion_vector_tex) \
  VAR(accumulation_size)


#define VAR(a) static int a##VarId = -1;
MOTION_BLUR_VARS
#undef VAR

MotionBlur::MotionBlur()
{
#define VAR(a) a##VarId = get_shader_variable_id(#a);
  MOTION_BLUR_VARS
#undef VAR

  G_ASSERT_RETURN(isSupported(), );

  useSettings({});

  // Create accumulation material.
  motionBlurMaterial = new_shader_material_by_name("accumulate_motion_blur");
  G_ASSERT(motionBlurMaterial);
  motionBlurShElem = motionBlurMaterial->make_elem();
  motionBlurMvMaterial = new_shader_material_by_name("accumulate_motion_blur_mv");
  G_ASSERT(motionBlurMvMaterial);
  motionBlurMvShElem = motionBlurMvMaterial->make_elem();

  // Apply.
  applyMotionBlurRenderer.init("apply_motion_blur");
}


void MotionBlur::update(float dt, const Point3 camera_pos, float jump_dist)
{
  if (lengthSq(prevCamPos - camera_pos) > jump_dist * jump_dist)
    onCameraLeap();
  prevCamPos = camera_pos;

  if (numValidFrames == numHistoryFrames)
  {
    ShaderGlobal::set_real(velocity_mulVarId, scale * velocityMultiplier / (dt + 0.0001f));
  }
  else
  {
    ShaderGlobal::set_real(velocity_mulVarId, 0.f);
  }

  if (skipFrames == 0)
  {
    numValidFrames = min(numValidFrames + 1, numHistoryFrames);
  }
  else
  {
    skipFrames--;
  }
}


void MotionBlur::onCameraLeap()
{
  skipFrames = 2;
  numValidFrames = 0;
}

void MotionBlur::accumulateDepthVersion(ManagedTexView source_color_tex, ManagedTexView blur_depth_tex, DepthType depth_type,
  TMatrix4 currentGlobTm, TMatrix4 prevGlobTm, ManagedTexView accumulationTex)
{
  SCOPE_VIEW_PROJ_MATRIX;
  // Set globtm matrices.
  ShaderGlobal::set_float4x4(prev_globtmVarId, prevGlobTm.transpose());

  d3d::setglobtm(currentGlobTm);
  set_viewvecs_to_shader();

  ShaderGlobal::set_texture(blur_depth_texVarId, blur_depth_tex);
  ShaderGlobal::set_int(motion_blur_depth_typeVarId, static_cast<int>(depth_type));
  accumulateInternal(source_color_tex, motionBlurShElem.get(), accumulationTex);
}

void MotionBlur::accumulateMotionVectorVersion(ManagedTexView source_color_tex, ManagedTexView motion_vector_tex,
  ManagedTexView accumulationTex)
{
  ShaderGlobal::set_texture(blur_motion_vector_texVarId, motion_vector_tex);
  accumulateInternal(source_color_tex, motionBlurMvShElem.get(), accumulationTex);
}

void MotionBlur::accumulateInternal(ManagedTexView source_color_tex, ShaderElement *elem, ManagedTexView accumulationTex)
{
  if (scale < SCALE_EPS)
    return;

  TIME_D3D_PROFILE(MotionBlur);
  SCOPE_RENDER_TARGET;

  ShaderGlobal::set_texture(blur_source_texVarId, source_color_tex);

  // build accum
  d3d::set_render_target(accumulationTex.getTex2D(), 0);
  d3d::clearview(CLEAR_TARGET, 0, 0.f, 0);

  // Set shader vars.
  ShaderGlobal::set_real(max_velocityVarId, maxVelocity);
  ShaderGlobal::set_real(alpha_mul_on_applyVarId, alphaMulOnApply * scale);
  ShaderGlobal::set_color4(overscan_texcoordVarId, 0.5f - 0.5f * overscan * overscanMul * scale,
    -0.5f + 0.5f * overscan * overscanMul * scale, 0.5f, 0.5f);

  TextureInfo info;
  accumulationTex->getinfo(info);
  const int accumulationWidth = info.w;
  const int accumulationHeight = info.h;
  ShaderGlobal::set_color4(accumulation_sizeVarId, accumulationWidth, accumulationHeight);

  // Render to accumulation texture.
  d3d::setvdecl(BAD_VDECL);
  d3d::setvsrc(0, nullptr, 0);
  if (elem->setStates(0, true))
    d3d::draw(PRIM_LINELIST, 0, accumulationWidth * accumulationHeight);
}

void MotionBlur::combine(ManagedTexView target_tex, ManagedTexView accumulationTex)
{
  if (scale < SCALE_EPS)
    return;

  d3d::set_render_target(target_tex.getTex2D(), 0);
  ShaderGlobal::set_texture(accumulation_texVarId, accumulationTex);
  applyMotionBlurRenderer.render();
}

void MotionBlur::useSettings(const Settings &settings)
{
  velocityMultiplier = settings.velocityMultiplier;
  maxVelocity = settings.maxVelocity;
  alphaMulOnApply = settings.alphaMulOnApply;
  overscanMul = settings.overscanMul;
  ShaderGlobal::set_real(forward_blurVarId, settings.forwardBlur);
  scale = settings.scale;
}

bool MotionBlur::isSupported()
{
  if (!(d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & d3d::USAGE_VERTEXTEXTURE))
  {
    debug("no suitable motion blur accum texture format:\n"
          "TEXFMT_A16B16G16R16F not supported as vertex texture, motion blur disabled");
    return false;
  }
  return true;
}
