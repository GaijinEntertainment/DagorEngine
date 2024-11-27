// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daStars.h"
#include <ioSys/dag_dataBlock.h>
#include <astronomy/astronomy.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_statDrv.h>
#include <daSkies2/daScattering.h>
#include <3d/dag_lockSbuffer.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>

static int starIntensityGlobVarId = -1;
static int starsScaleVarId = -1;

static int stars_moon_intensityVarId = -1, stars_moon_sizeVarId = -1, stars_moon_dirVarId = -1;

static int stars_lattitudeVarId = -1, stars_lstVarId = -1;

DaStars::~DaStars() { close(); }

void DaStars::close()
{
  starsTex.close();
  resetMoonResourceName();
  del_it(starsMaterial);
  del_it(moonMaterial);
  moonElem = NULL;
  starsVb.close();
  starsIb.close();
}

void DaStars::resetMoonResourceName() { moonTex.close(); }

bool DaStars::setMoonResourceName(const char *res_name)
{
  SharedTex moonTex2 = dag::get_tex_gameres(res_name);
  if (!moonTex2)
    return false;
  resetMoonResourceName();
  moonTex = SharedTexHolder(eastl::move(moonTex2), "stars_moon_tex");
  ShaderGlobal::set_sampler(get_shader_variable_id("stars_moon_tex_samplerstate", true),
    get_texture_separate_sampler(moonTex.getTexId()));
  return true;
}

void DaStars::init(const char *stars, const char *moon)
{
  stars_moon_dirVarId = get_shader_variable_id("stars_moon_dir");
  stars_moon_intensityVarId = get_shader_variable_id("stars_moon_intensity");
  stars_moon_sizeVarId = get_shader_variable_id("stars_moon_size");

  stars_lattitudeVarId = get_shader_variable_id("stars_lattitude");
  stars_lstVarId = get_shader_variable_id("stars_lst");
  starsTex = dag::get_tex_gameres(stars, "stars_tex");
  ShaderGlobal::set_sampler(get_shader_variable_id("stars_tex_samplerstate", true), d3d::request_sampler({}));
  G_ASSERTF(starsTex, "Texture '%s' for stars not found.", stars);

  moonSize = 0.02f;

  G_VERIFYF(setMoonResourceName(moon), "Texture '%s' for moon not found.", moon);

  starIntensityGlobVarId = ShaderGlobal::get_glob_var_id(::get_shader_variable_id("star_intensity"));
  starsScaleVarId = ::get_shader_variable_id("stars_scale");

  starsMaterial = new_shader_material_by_name("daStars");
  G_ASSERT(starsMaterial);
  if (starsMaterial)
  {
    static CompiledShaderChannelId starsChan[] = {
      {SCTYPE_SHORT2, SCUSAGE_POS, 0, 0},
      {SCTYPE_HALF2, SCUSAGE_TC, 0, 0},
    };
    G_ASSERT(starsMaterial->checkChannels(starsChan, sizeof(starsChan) / sizeof(starsChan[0])));

    starsRendElem.shElem = starsMaterial->make_elem();
    starsRendElem.vDecl = dynrender::addShaderVdecl(starsChan, sizeof(starsChan) / sizeof(starsChan[0]));
    G_ASSERT(starsRendElem.vDecl != BAD_VDECL);
    starsRendElem.stride = dynrender::getStride(starsChan, sizeof(starsChan) / sizeof(starsChan[0]));

    starsRendElem.minVert = 0;
    starsRendElem.numVert = star_catalog::g_star_count * 4;
    starsRendElem.startIndex = 0;
    starsRendElem.numPrim = star_catalog::g_star_count * 2;
    starsVb = dag::create_vb(starsRendElem.numVert * starsRendElem.stride, 0, "starsVb");
    G_ASSERT(starsVb);

    starsIb = dag::create_ib(starsRendElem.numPrim * 3 * sizeof(uint16_t), 0,
      "starsIb"); // To be filled on scene change.
    G_ASSERT(starsIb);

    generateStars();
  }

  moonMaterial = new_shader_material_by_name("daMoon");
  G_ASSERT(moonMaterial);
  if (moonMaterial)
    moonElem = moonMaterial->make_elem();
}

#pragma pack(push)
#pragma pack(1)
struct Short2
{
  int16_t x, y;
  Short2() {} //-V730
  Short2(float xf, float yf)
  {
    x = clamp(xf * 32767.f, -32768.f, 32767.f);
    y = clamp(yf * 32767.f, -32768.f, 32767.f);
  }
};
struct Half2
{
  uint16_t x, y;
  Half2() {} //-V730
  Half2(float xf, float yf)
  {
    x = float_to_half(xf);
    y = float_to_half(yf);
  }
};
struct StarVertex
{
  Short2 pos;
  Half2 brightness_size;
};
#pragma pack(pop)


void DaStars::generateStars()
{
  // Fill VB, IB.

  if (auto starsVertices = lock_sbuffer<StarVertex>(starsVb.getBuf(), 0, 0, VBLOCK_WRITEONLY))
  {
    if (auto starsIndices = lock_sbuffer<unsigned short int>(starsIb.getBuf(), 0, 0, VBLOCK_WRITEONLY))
    {
      static bool enhanceAsterisms = true;
      if (enhanceAsterisms) // enhance most well known asterisms/constellations: including two Ursus's (their main asterics)
      {
        enhanceAsterisms = false;
        for (int i = 0; i < countof(star_catalog::asterisms); ++i)
        {
          for (int j = 0; j < star_catalog::asterisms[i].numberOfStars; ++j)
          {
            int star = star_catalog::asterisms[i].stars[j];
            if (star < star_catalog::g_star_count)
              star_catalog::g_star_data[star].visualMagnitude -= 200 / (2 + star_catalog::asterisms[i].importance);
          }
        }
      }

      for (unsigned int starNo = 0; starNo < star_catalog::g_star_count; starNo++)
      {
        star_catalog::StarDesc s = star_catalog::g_star_data[starNo];
        // Point3 pos;

        // float altitude, azimuth;
        // radec_2_altaz(s.rightAscension, s.declination, lst, latitude, altitude, azimuth);
        // altaz_2_xyz(altitude, azimuth, pos.z, pos.y, pos.x);
        // radec_2_xyz(s.rightAscension, s.declination, lst, latitude, pos.z, pos.y, pos.x);

        float visualMagnitude = s.visualMagnitude / 100.0f;
        const float referenceMagnitude = 352 / 100.0f;
        float magnitudeToReference = powf(2.512, (referenceMagnitude - visualMagnitude));
        // float magnitude = 700.f / (s.visualMagnitude - star_catalog::g_max_bright + 700.f);
        // float size = starSize * magnitude;
        float size = sqrt(1.0 + 0.75 * max(0.0f, logf(1 + magnitudeToReference)));
        float magnitude = logf(magnitudeToReference + 1);

        starsVertices[starNo * 4 + 0].pos = starsVertices[starNo * 4 + 1].pos = starsVertices[starNo * 4 + 2].pos =
          starsVertices[starNo * 4 + 3].pos = Short2(s.rightAscension / 24.0, (s.declination + 90.0) / 180.0);

        // unsigned int texNo = grnd() * numTextures / 32768;
        starsVertices[starNo * 4 + 0].brightness_size = Half2(-size, -magnitude);
        starsVertices[starNo * 4 + 1].brightness_size = Half2(-size, +magnitude);
        starsVertices[starNo * 4 + 2].brightness_size = Half2(+size, +magnitude);
        starsVertices[starNo * 4 + 3].brightness_size = Half2(+size, -magnitude);

        starsIndices[starNo * 6] = starNo * 4;
        starsIndices[starNo * 6 + 1] = starNo * 4 + 1;
        starsIndices[starNo * 6 + 2] = starNo * 4 + 2;
        starsIndices[starNo * 6 + 3] = starNo * 4 + 0;
        starsIndices[starNo * 6 + 4] = starNo * 4 + 2;
        starsIndices[starNo * 6 + 5] = starNo * 4 + 3;
      }
    }
  }
}

void DaStars::renderStars(const Driver3dPerspective &persp, float sun_brightness, float latitude, float longtitude, double julian_day,
  float initial_azimuth_angle, float stars_intensity_mul)
{
  if (!starsVb || sun_brightness > 1.0)
    return;

  TMatrix worldTm;
  worldTm.rotyTM(-initial_azimuth_angle * DEG_TO_RAD);
  d3d::settm(TM_WORLD, worldTm);

  TIME_D3D_PROFILE(render_stars);
  float stars_lst = calc_lst_hours(julian_day, longtitude);
  ShaderGlobal::set_real(stars_lattitudeVarId, latitude);
  ShaderGlobal::set_real(stars_lstVarId, stars_lst);

  float starIntensity = (1 - sun_brightness) * stars_intensity_mul;
  ShaderGlobal::set_real_fast(starIntensityGlobVarId, starIntensity);

  ShaderGlobal::set_real(starsScaleVarId, 0.001f / (persp.wk > 0 ? persp.wk : 1.3f));

  d3d::setvsrc(0, starsVb.getBuf(), starsRendElem.stride);
  d3d::setind(starsIb.getBuf());
  starsTex.setVar();

  starsRendElem.shElem->render(0, starsRendElem.numVert, 0, starsRendElem.numPrim);

  d3d::settm(TM_WORLD, TMatrix::IDENT);
}

float DaStars::getMoonIntenisty(const Point3 &origin, const Point3 &moonDir, float moonAgeDay, float sun_brightness, float &moonAge)
{
  moonAge = 0.f;
  float moonIntensity = 2 - sun_brightness;
  if (moonIntensity <= 0)
    return 0;
  moonAge = moonAgeDay / MOON_PERIOD;
  if (moonAge < 0.01 || moonAge >= 0.99)
    return 0;

  if (moonDir.y + moonSize < -0.5) // fixme: check ground intersection and approximate moon size
    return 0;
  if (moonDir.y < 0)
  {
    float RG = DaScattering::getEarthRadius();
    float r = max(4.f, origin.y) * 0.001f + RG;
    float mu = moonDir.y + moonSize * 2;
    if (mu * fabsf(mu) < -(1.0 - (RG / r) * (RG / r)))
      return 0;
  }
  return moonIntensity;
}

void DaStars::setMoonSize(float size)
{
  moonSize = size;
  ShaderGlobal::set_real(stars_moon_sizeVarId, moonSize);
}

bool DaStars::setMoonVars(const Point3 &origin, const Point3 &moonDir, float moonAgeDay, float sun_brightness)
{
  float moonAgeNorm;
  float moonIntensity = getMoonIntenisty(origin, moonDir, moonAgeDay, sun_brightness, moonAgeNorm);
  if (moonIntensity <= 0.00001f)
  {
    ShaderGlobal::set_texture(moonTex.getVarId(), BAD_TEXTUREID);
    return false;
  }
  moonTex.setVar();
  ShaderGlobal::set_real_fast(stars_moon_intensityVarId, moonIntensity * 0.5);
  ShaderGlobal::set_color4(stars_moon_dirVarId, moonDir.x, moonDir.y, moonDir.z, moonAgeNorm);
  ShaderGlobal::set_real(stars_moon_sizeVarId, moonSize);
  return true;
}

void DaStars::renderMoon(const Point3 &origin, const Point3 &moonDir, float moonAge, float sun_brightness)
{
  if (!setMoonVars(origin, moonDir, moonAge, sun_brightness))
    return;

  TIME_D3D_PROFILE(render_moon);
  moonElem->setStates(0, true);
  d3d::setvsrc(0, starsVb.getBuf(), starsRendElem.stride);
  d3d::setind(starsIb.getBuf());
  d3d::drawind(PRIM_TRILIST, 0, 2, 0);
}

void DaStars::renderCelestialObject(TEXTUREID tid, const Point3 &dir, float phase, float intensity, float size)
{
  TIME_D3D_PROFILE(render_celestial);
  ShaderGlobal::set_texture(moonTex.getVarId(), tid);
  ShaderGlobal::set_real_fast(stars_moon_intensityVarId, intensity);
  ShaderGlobal::set_color4(stars_moon_dirVarId, dir.x, dir.y, dir.z, phase);
  ShaderGlobal::set_real(stars_moon_sizeVarId, size);
  moonElem->setStates(0, true);
  d3d::setvsrc(0, starsVb.getBuf(), starsRendElem.stride);
  d3d::setind(starsIb.getBuf());
  d3d::drawind(PRIM_TRILIST, 0, 2, 0);
}
