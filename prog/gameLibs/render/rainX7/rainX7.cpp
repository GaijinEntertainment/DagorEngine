// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_gameResources.h>
#include <ioSys/dag_dataBlock.h>
#include <3d/dag_materialData.h>
#include <3d/dag_render.h>
#include <math/dag_TMatrix4more.h>
#include <math/dag_Point4.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_buffers.h>
#include <math/random/dag_random.h>
#include <debug/dag_debug.h>

#include <stdlib.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <render/rainX7.h>
#include "perlin.h"
#include <startup/dag_globalSettings.h>

static inline bool feq(float a, float b) { return fabsf(a - b) < 1e-6f; }

static int rain_x7_translucency_factorVarId = -1;

#define MAX_PASSES 20

RainX7::RainX7(const DataBlock &blk) :
  passes(midmem), numPassesToRender(0), timePassed(1.f / 60.f), alphaFade(0.f), noiseX(0), noiseY(10000), noiseZ(20000)
{
  // Settings.

  for (size_t i = 0; i < MAX_VIEWS; i++)
  {
    currentCameraPos[i] = Point3(0.f, 0.f, 0.f);
    prevCameraPos[i] = Point3(0.f, 0.f, 0.f);
    currentGlobTm[i] = TMatrix4(TMatrix4::IDENT);
    prevGlobTm[i] = TMatrix4(TMatrix4::IDENT);
  }

  numParticles = blk.getInt("numParticles", 10000);
  particleBox = blk.getReal("particleBox", 30.f);
  numPassesMax = blk.getInt("numPassesMax", MAX_PASSES);
  cameraToGravity = blk.getReal("cameraToGravity", 0.f);

  numParticles *= ::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("rain_numParticles_multiplier", 1.0f);
  particleBox *= ::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("rain_particleBox_multiplier", 1.0f);

  diffuseTexId = ::get_tex_gameres(blk.getStr("diffuseTex", "rain_x7"));
  ShaderGlobal::set_sampler(get_shader_variable_id("rain_x7_diffuse_tex_samplerstate", true),
    get_texture_separate_sampler(diffuseTexId));
  G_ASSERT(diffuseTexId != BAD_TEXTUREID);

  /*
    parAlphaFadeSpeedBegin = blk.getReal("alphaFadeSpeedBegin", 10.f);  // Light rain defaults.
    parAlphaFadeSpeedEnd = blk.getReal("alphaFadeSpeedEnd", 40.f);
    parLength = blk.getReal("length", 0.15f);
    parSpeed = blk.getReal("speed", 4.5f);
    parWidth = blk.getReal("width", 0.02f);
    parDensity = blk.getReal("density", 3.f);
    parWind = blk.getReal("wind", 1.f);
    parAlpha = blk.getReal("alpha", 0.1f);
    initialParLighting = parLighting = color3(blk.getE3dcolor("lighting", 0xFFFFFFFF));
  */

  parAlphaFadeSpeedBegin = blk.getReal("alphaFadeSpeedBegin", 10.f); // Heavy snow defaults.
  parAlphaFadeSpeedEnd = blk.getReal("alphaFadeSpeedEnd", 40.f);
  parLength = blk.getReal("length", 0.04f);
  parSpeed = blk.getReal("speed", 1.5f);
  parWidth = blk.getReal("width", 0.04f);
  parDensity = blk.getReal("density", 15.f);
  parWind = blk.getReal("wind", 5.f);
  parAlpha = blk.getReal("alpha", 0.3f);
  initialParLighting = parLighting = Color3::xyz(blk.getPoint3("lighting", Point3(1, 1, 1)));
  perPassMul = blk.getReal("perPassMul", 1.f);


  // Shader vars.

  prevGlobTmLine0VarId = get_shader_variable_id("rain_x7_prev_globtm_line_0");
  prevGlobTmLine1VarId = get_shader_variable_id("rain_x7_prev_globtm_line_1");
  prevGlobTmLine2VarId = get_shader_variable_id("rain_x7_prev_globtm_line_2");
  prevGlobTmLine3VarId = get_shader_variable_id("rain_x7_prev_globtm_line_3");
  sizeScaleVarId = get_shader_variable_id("rain_x7_size_scale");
  forwardVarId = get_shader_variable_id("rain_x7_forward");
  posOffsetVarId = get_shader_variable_id("rain_x7_pos_offset");
  velocityVarId = get_shader_variable_id("rain_x7_velocity");
  particleBoxVarId = get_shader_variable_id("rain_x7_particle_box");
  lightingVarId = get_shader_variable_id("rain_x7_lighting");
  diffuseTexVarId = get_shader_variable_id("rain_x7_diffuse_tex");
  occluderPosVarId = get_shader_variable_id("rain_x7_occluder_pos");
  occluderHeightVarId = get_shader_variable_id("rain_x7_occluder_height");
  rain_x7_translucency_factorVarId = get_shader_variable_id("rain_x7_translucency_factor", true);

  ShaderGlobal::set_real(particleBoxVarId, particleBox);

  // Create material.

  Ptr<MaterialData> matNull = new MaterialData;

  static CompiledShaderChannelId chan[1] = {{SCTYPE_HALF4, SCUSAGE_TC, 0, 0}};

  matNull->className = "rain_x7";
  material = new_shader_material(*matNull);
  G_ASSERT(material);
  material->addRef();
  matNull = NULL;

  G_ASSERT(material->checkChannels(chan, sizeof(chan) / sizeof(chan[0])));

  rendElem.shElem = material->make_elem();
  rendElem.vDecl = dynrender::addShaderVdecl(chan, sizeof(chan) / sizeof(chan[0]));
  G_ASSERT(rendElem.vDecl != BAD_VDECL);
  rendElem.stride = dynrender::getStride(chan, sizeof(chan) / sizeof(chan[0]));
  rendElem.minVert = 0;
  rendElem.startIndex = 0;


  // VB and IB.
  rendElem.numVert = 4 * numParticles;
  vb = d3d::create_vb(rendElem.numVert * rendElem.stride, 0, "rainx7_vb");
  G_ASSERT(vb);

  rendElem.numPrim = 2 * numParticles;
  ib = d3d::create_ib(3 * rendElem.numPrim * sizeof(unsigned short int), 0, "rainx7_ib");

  G_ASSERT(ib);

  fillBuffers();

  // Passes.

  passes.resize(numPassesMax);
  for (unsigned int passNo = 0; passNo < numPassesMax; passNo++)
  {
    passes[passNo].speedScale = 0.75f + 0.5f * gfrnd();
    passes[passNo].offset = Point3(0.f, 0.f, 0.f);
    passes[passNo].wind = Point3(0.f, 0.f, 0.f);
    passes[passNo].random = Point3(particleBox * gfrnd(), particleBox * gfrnd(), particleBox * gfrnd());
  }
}

void RainX7::fillBuffers()
{
  struct Vertex
  {
    uint16_t pos_texcoord[4];
  } *vertices;

  vb->lock(0, rendElem.numVert * rendElem.stride, (void **)&vertices, VBLOCK_WRITEONLY);

  unsigned short int *indices;
  ib->lock(0, rendElem.numPrim * 3 * sizeof(unsigned short int), &indices, VBLOCK_WRITEONLY);

  if (vertices && indices)
  {
    Point3 defaultOffset(particleBox, particleBox, particleBox);
    for (unsigned int particleNo = 0; particleNo < numParticles; particleNo++)
    {
      Point3 pos(gfrnd() * particleBox, gfrnd() * particleBox, gfrnd() * particleBox);
      pos += defaultOffset;

      vertices[0].pos_texcoord[0] = vertices[1].pos_texcoord[0] = vertices[2].pos_texcoord[0] = vertices[3].pos_texcoord[0] =
        float_to_half(pos.x);
      vertices[0].pos_texcoord[1] = vertices[1].pos_texcoord[1] = vertices[2].pos_texcoord[1] = vertices[3].pos_texcoord[1] =
        float_to_half(pos.y);
      vertices[0].pos_texcoord[2] = vertices[1].pos_texcoord[2] = vertices[2].pos_texcoord[2] = vertices[3].pos_texcoord[2] =
        float_to_half(pos.z);
      vertices[0].pos_texcoord[3] = float_to_half(0.1f);
      vertices[1].pos_texcoord[3] = 0; // float_to_half(0.0f) is not written to the buffer for some unknown reason
      vertices[2].pos_texcoord[3] = float_to_half(1.0f);
      vertices[3].pos_texcoord[3] = float_to_half(1.1f);

      indices[0] = 4 * particleNo;
      indices[1] = 4 * particleNo + 1;
      indices[2] = 4 * particleNo + 2;
      indices[3] = 4 * particleNo + 2;
      indices[4] = 4 * particleNo + 3;
      indices[5] = 4 * particleNo;

      vertices += 4;
      indices += 6;
    }
  }

  vb->unlock();
  ib->unlock();
}


RainX7::~RainX7()
{
  material->delRef();
  del_d3dres(vb);
  del_d3dres(ib);
  release_managed_tex(diffuseTexId);
}


void RainX7::update(float dt, const Point3 &camera_speed)
{
  timePassed = dt;

  for (size_t i = 0; i < MAX_VIEWS; i++)
  {
    prevGlobTm[i] = currentGlobTm[i];
    prevCameraPos[i] = currentCameraPos[i];
  }

  numPassesToRender = (int)(parDensity + 0.999f);
  numPassesToRender = min(numPassesToRender, numPassesMax);

  // make each pass use the wind a varying amount

  int wrap = numPassesMax / 5;
  float wind = parWind / (float)wrap;


  // increment the noise lookup values for each axis

  noiseX += (int)(timePassed * 5000.0f);
  noiseY += (int)(timePassed * 5000.0f);
  noiseZ += (int)(timePassed * 5000.0f);


  // get a noise vector

  PerlinNoiseDefault perlinNoise;
  Point3 noise(perlinNoise.SoftNoise1D(noiseX), 0.5f * perlinNoise.SoftNoise1D(noiseY), perlinNoise.SoftNoise1D(noiseZ));


  for (unsigned int passNo = 0; passNo < numPassesMax; passNo++)
  {
    // calculate and add the per-pass gravity and wind

    float gravity = parSpeed * timePassed * passes[passNo].speedScale;
    passes[passNo].wind = noise * wind * (float)((passNo + 1) % (int)wrap);


    // add the movement from wind

    passes[passNo].offset += passes[passNo].wind * timePassed;


    // add the movement due to gravity

    float cameraOffset = camera_speed.y < 0.f ? cameraToGravity * camera_speed.y * dt : 0.f;
    passes[passNo].offset.y += gravity - cameraOffset;


    // wrap the offset to be within 0.0 - m_fParticleBox in each dimension

    if (passes[passNo].offset.x > particleBox)
      passes[passNo].offset.x = 0.f;
    else if (passes[passNo].offset.x < 0.f)
      passes[passNo].offset.x = particleBox;

    if (passes[passNo].offset.y > particleBox)
      passes[passNo].offset.y = 0.f;
    else if (passes[passNo].offset.y < 0.f)
      passes[passNo].offset.y = particleBox;

    if (passes[passNo].offset.z > particleBox)
      passes[passNo].offset.z = 0.f;
    else if (passes[passNo].offset.z < 0.f)
      passes[passNo].offset.z = particleBox;
  }
}

void RainX7::render(const Point3 &view_pos, const TMatrix &view_tm, const TMatrix4 &proj_tm, const TMatrix4 &globtm, uint32_t view_id)
{
  if (numPassesToRender == 0)
    return;

  if (parAlpha < 0.001f)
    return;

  setConstants(view_id, view_pos, view_tm, proj_tm, globtm);

  if (alphaFade <= 0.f)
    return;

  ShaderGlobal::set_texture(diffuseTexVarId, diffuseTexId);

  d3d::setvdecl(rendElem.vDecl);
  d3d::setvsrc(0, vb, rendElem.stride);
  d3d::setind(ib);

  float alpha = parDensity;
  for (unsigned int passNo = 0; passNo < numPassesToRender; passNo++)
  {
    setPassConstants(passNo, alpha, view_pos, view_tm);

    rendElem.shElem->render(0, rendElem.numVert, 0, rendElem.numPrim);
    alpha -= 1.f;
  }
}

void RainX7::setConstants(uint32_t view_id, const Point3 &view_pos, const TMatrix &view_tm, const TMatrix4 &proj_tm,
  const TMatrix4 &globtm)
{
  if (feq(timePassed, 0.f))
    return;

  currentCameraPos[view_id] = view_pos;
  Point3 cameraMovement = prevCameraPos[view_id] - currentCameraPos[view_id];

  // calculate a fade value based on the camera speed

  float cameraSpeed = length(cameraMovement) / timePassed;
  cameraSpeed = (cameraSpeed - parAlphaFadeSpeedBegin) / parAlphaFadeSpeedEnd;
  if (cameraSpeed < 0.f)
    cameraSpeed = 0.f;
  else if (cameraSpeed > 1.f)
    cameraSpeed = 1.f;

  alphaFade = 1.f - cameraSpeed;

  //  if (alphaFade > 0.f)
  // Artyom: we always must initialize constants.

  {
    // set current and previous world view proj matrices

    currentGlobTm[view_id] = globtm;

    TMatrix4 prevGtm = prevGlobTm[view_id];
    process_tm_for_drv_consts(prevGtm);

    ShaderGlobal::set_color4(prevGlobTmLine0VarId, prevGtm[0][0], prevGtm[0][1], prevGtm[0][2], prevGtm[0][3]);
    ShaderGlobal::set_color4(prevGlobTmLine1VarId, prevGtm[1][0], prevGtm[1][1], prevGtm[1][2], prevGtm[1][3]);
    ShaderGlobal::set_color4(prevGlobTmLine2VarId, prevGtm[2][0], prevGtm[2][1], prevGtm[2][2], prevGtm[2][3]);
    ShaderGlobal::set_color4(prevGlobTmLine3VarId, prevGtm[3][0], prevGtm[3][1], prevGtm[3][2], prevGtm[3][3]);


    // set vector to control width and height of particles

    float lengthScale = parLength / timePassed;
    if (parSpeed > 0.f)
      lengthScale /= parSpeed;

    const float normalAspectRatio = 16.0f / 9.0f;           // snowflake was made with 16:9 in mind
    float verticalZoom = proj_tm[1][1] / normalAspectRatio; // account for fov changes
    float width = parWidth * verticalZoom;

    ShaderGlobal::set_color4(sizeScaleVarId, width, lengthScale, parSpeed > 0.f ? 1.f : 0.f,
      abs(d3d::get_screen_aspect_ratio() - normalAspectRatio)); // take diff to rescale width of snowflake

    // set a forward shift vector - this gets a greater portion of the box inside the view frustum
    float boxOffset = particleBox * 0.5f;
    Color4 forward(view_tm[0][2] * boxOffset, view_tm[1][2] * boxOffset, view_tm[2][2] * boxOffset, view_tm[3][2]);
    ShaderGlobal::set_color4(forwardVarId, forward);
  }

  ShaderGlobal::set_real(rain_x7_translucency_factorVarId, translucencyFactor);
}


void RainX7::setPassConstants(unsigned int pass_no, float alpha, const Point3 &view_pos, const TMatrix &view_tm)
{
  // set the position offset for this pass

  float boxOffset = particleBox * 0.5f;
  Point3 forward(view_tm[0][2] * boxOffset, view_tm[1][2] * boxOffset, view_tm[2][2] * boxOffset);

  // combine all offsets together

  Point3 offset = view_pos + passes[pass_no].random + passes[pass_no].offset + forward;
  Color4 posOffset(fmodf(offset.x, particleBox), fmodf(offset.y, particleBox), fmodf(offset.z, particleBox), 0.f);

  // set the velocity vector for this pass

  Color4 velocity(passes[pass_no].wind.x * timePassed,
    passes[pass_no].wind.y * timePassed + passes[pass_no].speedScale * parSpeed * timePassed, passes[pass_no].wind.z * timePassed,
    0.f);

  if (feq(parWind, 0.f))
    velocity = Color4(timePassed, timePassed, timePassed, 0.f);


  // set color for this pass
  if (alpha > 1.0f)
    alpha = 1.0f;

  alpha *= parAlpha * alphaFade;

  ShaderGlobal::set_color4(posOffsetVarId, posOffset);
  ShaderGlobal::set_color4(velocityVarId, velocity);
  float passMul = (pass_no % 2) ? perPassMul : 1.f;
  ShaderGlobal::set_color4(lightingVarId, parLighting.r * passMul, parLighting.g * passMul, parLighting.b * passMul, alpha);
}

void RainX7::setOccluder(float x1, float z1, float x2, float z2, float y)
{
  if (feq(x1, x2) || feq(z1, z2))
  {
    ShaderGlobal::set_real(occluderHeightVarId, -1e10f);
    return;
  }

  ShaderGlobal::set_color4(occluderPosVarId, safediv(1.f, (x2 - x1)), safediv(1.f, (z2 - z1)), safediv(-x1, (x2 - x1)),
    safediv(-z1, (z2 - z1)));

  ShaderGlobal::set_real(occluderHeightVarId, y);
}
