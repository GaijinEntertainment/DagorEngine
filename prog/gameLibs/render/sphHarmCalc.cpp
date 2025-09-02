// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/sphHarmCalc.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_query.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_buffers.h>
#include <3d/dag_lockSbuffer.h>
#include <math/dag_TMatrix.h>
#include <math/dag_Point4.h>
#include <math/dag_adjpow2.h>
#include <shaders/dag_shaders.h>
#include <osApiWrappers/dag_miscApi.h>
#include <vecmath/dag_vecMath.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_gpuConfig.h>
#include <util/dag_hash.h>

#include <math/dag_hlsl_floatx.h>
#include "shaders/spherical_harmonics_consts.hlsli"

void getCubeFaceIvtm(int face, TMatrix &tm)
{
  tm.identity();
  switch (face)
  {
    case CUBEFACE_POSX: tm.setcol(2, 1, 0, 0); break;
    case CUBEFACE_NEGX: tm.setcol(2, -1, 0, 0); break;
    case CUBEFACE_POSY:
      tm.setcol(2, 0, 1, 0);
      tm.setcol(1, 0, 0, -1);
      break;
    case CUBEFACE_NEGY:
      tm.setcol(2, 0, -1, 0);
      tm.setcol(1, 0, 0, 1);
      break;
    case CUBEFACE_POSZ: tm.setcol(2, 0, 0, 1); break;
    case CUBEFACE_NEGZ: tm.setcol(2, 0, 0, -1); break;
  }
  tm.setcol(0, tm.getcol(1) % tm.getcol(2));
}

SphHarmCalc::SphHarmCalc(float hdr_scale) : hdrScale(hdr_scale) { reset(); }

void SphHarmCalc::processCubeFace(Texture *texture, int face, int width, float gamma)
{
  if (!valid)
    return;

  uint8_t *buf = NULL;
  int stride = 0;

  int res = texture->lockimg((void **)&buf, stride, 0, TEXLOCK_READ);
  if (res)
  {
    TextureInfo info;
    texture->getinfo(info, 0);
    if (buf)
      processFaceData(buf, face, width, stride, info.cflg & TEXFMT_MASK, gamma);

    texture->unlockimg();
  }

  if (!res || !buf)
  {
    logerr("SphHarmCalc::processCubeFace, can't lock texture");
    invalidate();
  }
}

//
// src: An Efficient Representation for Irradiance Environment Maps
// http://graphics.stanford.edu/papers/envmap/
//
inline void SphHarmCalc::calcValues(const Color4 &color, const Point3 &norm, float domega)
{
  float x = norm.x;
  float y = norm.y;
  float z = norm.z;

  values[SH3_SH00] += color * domega;

  /* L_{1m}. -1 <= m <= 1.  The linear terms */
  values[SH3_SH1M1] += color * y * domega; /* Y_{1-1} = 0.488603 y  */
  values[SH3_SH10] += color * z * domega;  /* Y_{10}  = 0.488603 z  */
  values[SH3_SH1P1] += color * x * domega; /* Y_{11}  = 0.488603 x  */

  /* The Quadratic terms, L_{2m} -2 <= m <= 2 */

  /* First, L_{2-2}, L_{2-1}, L_{21} corresponding to xy,yz,xz */
  values[SH3_SH2M2] += color * (x * y) * domega; /* Y_{2-2} = 1.092548 xy */
  values[SH3_SH2M1] += color * (y * z) * domega; /* Y_{2-1} = 1.092548 yz */
  values[SH3_SH2P1] += color * (x * z) * domega; /* Y_{21}  = 1.092548 xz */

  /* L_{20}.  Note that Y_{20} = 0.315392 (3z^2 - 1) */
  values[SH3_SH20] += color * (3 * z * z - 1) * domega;

  /* L_{22}.  Note that Y_{22} = 0.546274 (x^2 - y^2) */
  values[SH3_SH2P2] += color * (x * x - y * y) * domega;
}

Color3 SphHarmCalc::calcFunc(dag::ConstSpan<Color4> values, const Point3 &norm)
{
  Color3 intermediate0, intermediate1, intermediate2;

  // sph.a - constant frequency band 0, sph.rgb - linear frequency band 1

  Point4 norm4 = Point4(norm.x, norm.y, norm.z, 1.0f);
  intermediate0.r = norm4 * Point4::rgba(values[0]);
  intermediate0.g = norm4 * Point4::rgba(values[1]);
  intermediate0.b = norm4 * Point4::rgba(values[2]);

  // sph.rgba and sph6 - quadratic polynomials frequency band 2

  Point4 r1 = Point4(norm.x * norm.z, norm.y * norm.z, norm.x * norm.y, norm.z * norm.z);
  r1.w = 3.0 * r1.w - 1;
  intermediate1.r = r1 * Point4::rgba(values[3]);
  intermediate1.g = r1 * Point4::rgba(values[4]);
  intermediate1.b = r1 * Point4::rgba(values[5]);

  float r2 = norm.x * norm.x - norm.y * norm.y;
  intermediate2 = color3(values[6]) * r2;

  return intermediate0 + intermediate1 + intermediate2;
}

bool SphHarmCalc::processCubeData(CubeTexture *texture, const float gamma, const bool force_recalc)
{
  TextureInfo info;
  texture->getinfo(info, 0);
  if (spHarmReductor && spHarmFinalReductor)
    return processFaceDataGpu(texture, -1, info.w, texture->level_count(), gamma, force_recalc, EnviromentTextureType::CubeMap);
  else
  {
    for (int i = 0; i < 6; ++i)
    {
      uint8_t *bufData;
      int stride;
      if (!texture->lockimg((void **)&bufData, stride, i, 0, TEXLOCK_READ))
      {
        logerr("%s lockimg failed '%s'", __FUNCTION__, d3d::get_last_error());
        continue;
      }
      processFaceData(bufData, i, info.w, stride, info.cflg & TEXFMT_MASK, gamma);
      texture->unlockimg();
    }
    finalize();
    return true;
  }
}

bool SphHarmCalc::processPanoramaData(Texture *texture, const float gamma, const bool force_recalc)
{
  TextureInfo info;
  texture->getinfo(info, 0);
  if (spHarmReductor && spHarmFinalReductor)
    return processFaceDataGpu(texture, -1, info.h, texture->level_count(), gamma, force_recalc, EnviromentTextureType::Panorama);
  return false;
}

void SphHarmCalc::processFaceData(uint8_t *buf, int face, int width, int stride, uint32_t fmt, float gamma)
{
  // static const double total = PI;//( atan(1.f/sqrt(2.f+1.f))*4.f ) * 6.f = 4*PI;
  // since the lighting equation is integral over unit sphere of incomingLight*dot(normal, lightDirection)/PI*dw, then we need to
  // divide by PI, not by 4*PI (full solid angle)
  bool isLinearTarget = fabsf(gamma - 1) < 0.0001;

  TMatrix ivtm;
  getCubeFaceIvtm(face, ivtm);
  G_ASSERT(!fmt || fmt == TEXFMT_A32B32G32R32F || fmt == TEXFMT_A16B16G16R16F || fmt == TEXFMT_A8R8G8B8);

  float step_2 = 1.f / width;
  float step = step_2 * 2;
  for (int y = 0; y < width; ++y, buf += stride)
  {
    float y0 = float(y - width / 2) / (width / 2);
    float yc = y0 + step_2;
    float y1 = y0 + step;
    float y0sq = y0 * y0, y1sq = y1 * y1;
    float x1 = float(-width / 2) / (width / 2);

    float at11 = atanf(x1 * y1 / sqrtf(x1 * x1 + y1sq + 1.f));
    float at01 = atanf(x1 * y0 / sqrtf(x1 * x1 + y0sq + 1.f));

    for (int x = 0; x < width; ++x)
    {
      DECL_ALIGN16(Color4, col);
      if (fmt == TEXFMT_A32B32G32R32F)
        col = Color4((float *)(buf + x * 4 * 4));
      else if (fmt == TEXFMT_A16B16G16R16F)
        v_st(&col.r, v_half_to_float((uint16_t *)(buf + x * 4 * 2)));
      else
      {
        uint8_t *ptr = (uint8_t *)(buf + x * 4);
        col = Color4(ptr[0] / 255.0f, ptr[1] / 255.0f, ptr[2] / 255.0f, 1);
      }
      if (!isLinearTarget)
        col = Color4(powf(max(0.f, col.r), gamma), powf(max(0.f, col.g), gamma), powf(max(0.f, col.b), gamma), 1);

      // Harmonics are an approximation, be tolerant.
      if (col.r < -0.1f || col.g < -0.1f || col.b < -0.1f || col.r > 100.f || col.g > 100.f || col.b > 100.f || !check_finite(col.r) ||
          !check_finite(col.g) || !check_finite(col.b))
      {
        LOGERR_ONCE("Invalid SPH sample col=%@", col);
        col = Color4(0.1f, 0.2f, 0.4f); // Neutral day color.
      }
      col.clamp0();

      float at00 = at01;
      float at10 = at11;
      float x0 = x1;
      x1 = x0 + step;

      at11 = atanf(x1 * y1 / sqrtf(x1 * x1 + y1sq + 1.f));
      at01 = atanf(x1 * y0 / sqrtf(x1 * x1 + y0sq + 1.f));

      float k = (at00 - at01) + (at11 - at10);

      // k =atanf(x0*y0/sqrtf(x0*x0+y0sq+1.f));
      // k+=atanf(x1*y1/sqrtf(x1*x1+y1sq+1.f));
      // k-=atanf(x0*y1/sqrtf(x0*x0+y1sq+1.f));
      // k-=atanf(x1*y0/sqrtf(x1*x1+y0sq+1.f));

      float xc = x0 + step_2;

      Point3 norm = normalize(ivtm % Point3(xc, -yc, 1));

      calcValues(col, norm, k);
    }
  }
}

void SphHarmCalc::getValuesFromGpu(bool *should_retry)
{
  HarmCoefs *p, localValue;
  p = nullptr;
  if (harmonicsSum->lockEx(0, sizeof(*p), &p, VBLOCK_READONLY) && p)
  {
    memcpy(&localValue, p, sizeof(*p));
    harmonicsSum->unlock();
    for (int coefIdx = 0; coefIdx < HARM_COEFS_COUNT; ++coefIdx)
      values[coefIdx] =
        Color4(localValue.coeficients[coefIdx].x, localValue.coeficients[coefIdx].y, localValue.coeficients[coefIdx].z);
    inProgress = false;
    finalize();
  }
  else
  {
    attemptsToLock++;
    const int MAX_ATTEMPTS_TO_LOCK = 5;
    if (attemptsToLock < MAX_ATTEMPTS_TO_LOCK)
    {
      if (should_retry)
        *should_retry = true;
      return;
    }
    invalidate();
    if (!d3d::is_in_device_reset_now())
      logwarn("Can't lock spherical harmonics computation result.");
  }
  attemptsToLock = 0;
  if (should_retry)
    *should_retry = false;
}

bool SphHarmCalc::gpuFinalReduction(const int values_to_sum, const bool force_recalc)
{
  static int final_reduction_sizeVarId = ::get_shader_variable_id("sp_harm_final_reduction_size");

  ShaderGlobal::set_int_fast(final_reduction_sizeVarId, values_to_sum);

  {
    if (!inProgress)
    {
      TIME_D3D_PROFILE(dispatchArray)
      d3d::resource_barrier({harmonicsPreSum.get(), RB_RO_SRV | RB_STAGE_COMPUTE});
      d3d::set_rwbuffer(STAGE_CS, 0, harmonicsSum.get());
      d3d::set_buffer(STAGE_CS, 0, harmonicsPreSum.get());
      spHarmFinalReductor->dispatch(1, 1, 1);
      d3d::set_rwbuffer(STAGE_CS, 0, 0);
      d3d::set_buffer(STAGE_CS, 0, 0);
    }
    if (force_recalc)
    {
      bool shouldRetry = true;
      while (shouldRetry)
      {
        getValuesFromGpu(&shouldRetry);
        if (shouldRetry)
          d3d::driver_command(Drv3dCommand::D3D_FLUSH);
      }
      return valid;
    }
    else
    {
      if (!inProgress)
      {
        issue_readback_query(valueComputeFence.get(), harmonicsSum.get());
        inProgress = true;
      }
      return false;
    }
  }
}


bool SphHarmCalc::processFaceDataGpu(Texture *texture, const int face, int width, int level_count, const float gamma,
  const bool force_recalc, EnviromentTextureType envi_type)
{
  static const int cube_face_widthVarId = ::get_shader_variable_id("sp_harm_cube_face_width");
  static const int face_numberVarId = ::get_shader_variable_id("sp_harm_face_number");
  static const int mip_numberVarId = ::get_shader_variable_id("sp_harm_mip_number");
  static const int gammaVarId = ::get_shader_variable_id("sp_harm_gamma");


  if (envi_type == EnviromentTextureType::CubeMap && texture->getType() != D3DResourceType::CUBETEX)
  {
    logerr("passing not cubemap to harmonics, texname %s, type %d", texture->getName(), eastl::to_underlying(texture->getType()));
    return false;
  }
  if (envi_type == EnviromentTextureType::Panorama && texture->getType() != D3DResourceType::TEX)
  {
    logerr("passing not texture2d to harmonics, texname %s, type %d", texture->getName(), eastl::to_underlying(texture->getType()));
    return false;
  }

  // auto correction
  int sampling_resolution = min(width, MAX_TEXTURE_SAMPLING_RESOLUTION);

  int desiredMip = min((int)get_log2i(width / MAX_TEXTURE_SAMPLING_RESOLUTION), level_count - 1);
  const uint32_t facesCount = face == -1 ? 6 : 1;
  const uint32_t xJobs = max(sampling_resolution / THREADS_PER_GROUP_AXIS, 2);
  const uint32_t yJobs = xJobs / 2;
  const uint32_t groupsToRun = xJobs * yJobs * facesCount;

  if (inProgress && d3d::get_event_query_status(valueComputeFence.get(), false) && groupsToRun == previousRunSize && !force_recalc)
  {
    getValuesFromGpu();
    return valid;
  }

  if (groupsToRun != previousRunSize)
  {
    harmonicsPreSum.reset();
    harmonicsPreSum.reset(d3d::buffers::create_ua_sr_structured(sizeof(HarmCoefs), groupsToRun, "harmonicsValues"));
    previousRunSize = groupsToRun;

    if (!harmonicsSum)
      harmonicsSum.reset(d3d::buffers::create_ua_structured_readback(sizeof(HarmCoefs), 1, "harmonicsValuesFinal"));
  }
  d3d::set_rwbuffer(STAGE_CS, 0, harmonicsPreSum.get());

  d3d::set_tex(STAGE_CS, 1, texture);
  d3d::set_sampler(STAGE_CS, 1, d3d::request_sampler({}));

  ShaderGlobal::set_int_fast(cube_face_widthVarId, sampling_resolution);
  ShaderGlobal::set_int_fast(face_numberVarId, face);
  ShaderGlobal::set_int_fast(mip_numberVarId, desiredMip);
  ShaderGlobal::set_real_fast(gammaVarId, gamma);

  ShaderGlobal::set_variant(envi_type == EnviromentTextureType::CubeMap ? ShaderGlobal::variant<"sp_harm_type"_h, "from_sky_box"_h>
                                                                        : ShaderGlobal::variant<"sp_harm_type"_h, "from_panorama"_h>);

  {
    TIME_D3D_PROFILE(dispatchCube)
    spHarmReductor->dispatch(xJobs, yJobs, facesCount);
  }

  bool ret = gpuFinalReduction(groupsToRun, force_recalc);
  d3d::set_rwbuffer(STAGE_CS, 0, 0);
  d3d::set_tex(STAGE_CS, 1, nullptr);
  return ret;
}

template <class T>
static bool remove_color_nan(T &a)
{
  bool hasnan = false;
  if (check_nan(a.r))
  {
    a.r = 0.0f;
    hasnan = true;
  }
  if (check_nan(a.g))
  {
    a.g = 0.0f;
    hasnan = true;
  }
  if (check_nan(a.b))
  {
    a.b = 0.0f;
    hasnan = true;
  }
  return hasnan;
}

void SphHarmCalc::finalize()
{
  hasNans = false;
  for (int i = 0; i < values.size(); ++i)
  {
    Color4 &v = values[i];
    if (remove_color_nan(v))
      hasNans = true;
    v.a = v.r * 0.299f + v.g * 0.587f + v.b * 0.114f;
    v *= hdrScale;
  }

  if (hasNans)
  {
    logerr("NaNs in sph harmonics");
  }


  // Values at this point are integrals of colors multiplied by simplified spherical harmonic functions (without coefficients), apply
  // the coefficients. Coefficients are constant parts of Y_lm in equations 3 and 10 from the paper.

  const float Y_00_coef = 0.282095f;
  const float Y_1m_coef = 0.488603f;
  const float Y_2m_coef = 1.092548f;
  const float Y_20_coef = 0.315392f;
  const float Y_22_coef = 0.546274f;

  values[SH3_SH00] *= Y_00_coef;
  values[SH3_SH1P1] *= Y_1m_coef;
  values[SH3_SH10] *= Y_1m_coef;
  values[SH3_SH1M1] *= Y_1m_coef;
  values[SH3_SH2P1] *= Y_2m_coef;
  values[SH3_SH2M1] *= Y_2m_coef;
  values[SH3_SH2M2] *= Y_2m_coef;
  values[SH3_SH20] *= Y_20_coef;
  values[SH3_SH2P2] *= Y_22_coef;


  // Now values are the lighting coefficients L_lm as in Figure 2 from the paper.

  const float c1 = 0.429043; // A^ * Y from https://cseweb.ucsd.edu/~ravir/papers/envmap/envmap.pdf
  const float c2 = 0.511664;
  // c3 = 3 * c5, factor them out.
  const float c4 = 0.886227;
  const float c5 = 0.247708;

  // Equation 13 from the paper.
  // col = c1*L22*(x2-y2) + c5*L20*(3*z2-1) + c4*L00
  //  + 2*c1*(L2_2*xy + L21*xz + L2_1*yz)
  //  + 2*c2*(L11*x+L1_1*y+L10*z) ;

  values[SH3_SH00] *= c4;

  values[SH3_SH1M1] *= 2.f * c2;
  values[SH3_SH10] *= 2.f * c2;
  values[SH3_SH1P1] *= 2.f * c2;

  values[SH3_SH2M2] *= 2.f * c1;
  values[SH3_SH2M1] *= 2.f * c1;
  values[SH3_SH2P1] *= 2.f * c1;

  values[SH3_SH20] *= c5;
  values[SH3_SH2P2] *= c1;


  // From radiant exitance to Lambertian reflected radiance.

  for (int i = 0; i < values.size(); ++i)
    values[i] /= PI;


  // Pack for efficient lighting calculation.

  // normal.x
  valuesOpt[0][0] = values[SH3_SH1P1][0];
  valuesOpt[1][0] = values[SH3_SH1P1][1];
  valuesOpt[2][0] = values[SH3_SH1P1][2];

  // normal.y
  valuesOpt[0][1] = values[SH3_SH1M1][0];
  valuesOpt[1][1] = values[SH3_SH1M1][1];
  valuesOpt[2][1] = values[SH3_SH1M1][2];

  // normal.z
  valuesOpt[0][2] = values[SH3_SH10][0];
  valuesOpt[1][2] = values[SH3_SH10][1];
  valuesOpt[2][2] = values[SH3_SH10][2];

  // normal.w = 1
  valuesOpt[0][3] = values[SH3_SH00][0];
  valuesOpt[1][3] = values[SH3_SH00][1];
  valuesOpt[2][3] = values[SH3_SH00][2];

  // xz, yz, xy
  valuesOpt[3][0] = values[SH3_SH2P1][0];
  valuesOpt[4][0] = values[SH3_SH2P1][1];
  valuesOpt[5][0] = values[SH3_SH2P1][2];

  valuesOpt[3][1] = values[SH3_SH2M1][0];
  valuesOpt[4][1] = values[SH3_SH2M1][1];
  valuesOpt[5][1] = values[SH3_SH2M1][2];

  valuesOpt[3][2] = values[SH3_SH2M2][0];
  valuesOpt[4][2] = values[SH3_SH2M2][1];
  valuesOpt[5][2] = values[SH3_SH2M2][2];

  // w = 3 * z^2 - 1
  valuesOpt[3][3] = values[SH3_SH20][0];
  valuesOpt[4][3] = values[SH3_SH20][1];
  valuesOpt[5][3] = values[SH3_SH20][2];

  // x^2 - y^2
  valuesOpt[6] = values[SH3_SH2P2];
}

void SphHarmCalc::reset()
{
  mem_set_0(values);
  mem_set_0(valuesOpt);
  valid = true;

  auto driverDesc = d3d::get_driver_desc();
  bool init_compute = d3d::get_driver_code()
                        .map(d3d::iOS, false)                   // compute version hangs mobile gpu
                        .map(d3d::macOSX && d3d::vulkan, false) // Mac+Vulkan has problems with compute version
                        .map(d3d::macOSX, d3d_get_gpu_cfg().primaryVendor != GpuVendor::INTEL) // on some intel drivers it fails to
                                                                                               // create pipeline state on mac
                        .map(d3d::any, driverDesc.shaderModel >= 5.0_sm && !driverDesc.issues.hasComputeTimeLimited);

  if (init_compute)
  {
    spHarmReductor.reset(new_compute_shader("spherical_harmonics_reduction_cs", true));
    spHarmFinalReductor.reset(new_compute_shader("final_harmonics_reduction_cs", true));
    valueComputeFence.reset(d3d::create_event_query());
  }
  else
  {
    spHarmReductor.reset();
    spHarmFinalReductor.reset();
    valueComputeFence.reset();
  }
}

void SphHarmCalc::invalidate()
{
  reset();
  valid = false;
}

bool SphHarmCalc::isValid() { return valid; }
