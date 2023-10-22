#include <render/gpuBenchmark.h>
#include <math/dag_adjpow2.h>
#include <3d/dag_quadIndexBuffer.h>
#include <math/dag_math3d.h>
#include <math/random/dag_random.h>
#include <perfMon/dag_statDrv.h>

static const int NUM_CUBE = 2000;
static const int QUADS_CUBE = NUM_CUBE * 6;
static const int QUADS_HM_SIDE = 200;
static const int QUADS_HM = QUADS_HM_SIDE * QUADS_HM_SIDE;
static const int QUADS_COUNT = QUADS_CUBE + QUADS_HM;
static const int VERT_COUNT = QUADS_COUNT * 4;
static const int TRI_COUNT_CUBE = QUADS_CUBE * 2;
static const int TRI_COUNT_HM = QUADS_HM * 2;
static const int RANDOM_BUFFER_RESOLUTION = 1024;
static const int WARM_GPU_FRAMES = 10;


static inline float deg_to_fov(float deg) { return 1.f / tanf(DEG_TO_RAD * 0.5f * deg); }

Point3 gen_spherical_dir(float azimuth_min, float azimuth_max, float zenith_min, float zenith_max)
{
  float phiSin, phiCos;
  sincos((azimuth_min + (azimuth_max - azimuth_min) * gfrnd()) * 2 * PI - PI, phiSin, phiCos);
  float thetaSin, thetaCos;
  sincos((zenith_min + (zenith_max - zenith_min) * gfrnd()) * PI, thetaSin, thetaCos);
  return Point3(thetaSin * phiCos, thetaCos, thetaSin * phiSin);
}

static TMatrix get_random_inst_tm()
{
  Quat rot = quat_rotation_arc(Point3(1, 0, 0), gen_spherical_dir(0, 1, 0, 1));
  Point3 pos(gfrnd() - 0.5f, gfrnd() - 0.5f, gfrnd() - 0.5f);
  TMatrix tm = TMatrix((gfrnd() * 0.9f + 0.1f) * 3.0f);
  tm.setcol(3, pos * 30.0f);
  tm = tm * makeTM(rot);
  return tm;
}


GpuBenchmark::GpuBenchmark()
{
  benchShader.init("gpu_benchmark", NULL, 0, "gpu_benchmark");

  GPUWatchMs::init_freq();
  index_buffer::init_quads_32bit(QUADS_COUNT);

  int vbSize = sizeof(Point4) * 2 * VERT_COUNT;
  vb.set(d3d::create_vb(vbSize, SBCF_MAYBELOST | SBCF_CPU_ACCESS_WRITE, "gpu_benchmark_vb"), "gpu_benchmark");
  G_ASSERT(vb.getBuf());

  struct VertexData
  {
    Point4 pos;
    Point4 tc;
  };

  VertexData *vbData = NULL;
  bool vbLockSuccess = vb.getBuf()->lock(0, vbSize, (void **)&vbData, VBLOCK_WRITEONLY);
  G_ASSERT(vbLockSuccess && vbData != NULL);
  G_UNUSED(vbLockSuccess);
  if (vbData)
  {
    for (int cubeNo = 0; cubeNo < NUM_CUBE; ++cubeNo)
    {
      TMatrix tm = get_random_inst_tm();
      VertexData *cube = vbData + cubeNo * 24;
      Point2 uv = Point2(rnd_float(0.0f, 0.9f), rnd_float(0.0f, 0.9f));
      float uvs = 0.1f;
      float alpha = rnd_float(0.1f, 1.0f);
      //
      cube[0] = {Point4::xyz0(Point3(-0.5f, -0.5f, -0.5f) * tm), Point4(uv.x, uv.y, 0, alpha)};
      cube[1] = {Point4::xyz0(Point3(0.5f, -0.5f, -0.5f) * tm), Point4(uv.x + uvs, uv.y, 0, alpha)};
      cube[2] = {Point4::xyz0(Point3(0.5f, -0.5f, 0.5f) * tm), Point4(uv.x + uvs, uv.y + uvs, 0, alpha)};
      cube[3] = {Point4::xyz0(Point3(-0.5f, -0.5f, 0.5f) * tm), Point4(uv.x, uv.y + uvs, 0, alpha)};
      //
      cube[4] = {Point4::xyz0(Point3(-0.5f, 0.5f, -0.5f) * tm), Point4(uv.x, uv.y, 0, alpha)};
      cube[5] = {Point4::xyz0(Point3(-0.5f, 0.5f, 0.5f) * tm), Point4(uv.x + uvs, uv.y, 0, alpha)};
      cube[6] = {Point4::xyz0(Point3(0.5f, 0.5f, 0.5f) * tm), Point4(uv.x + uvs, uv.y + uvs, 0, alpha)};
      cube[7] = {Point4::xyz0(Point3(0.5f, 0.5f, -0.5f) * tm), Point4(uv.x, uv.y + uvs, 0, alpha)};
      //
      cube[8] = {Point4::xyz0(Point3(-0.5f, -0.5f, 0.5f) * tm), Point4(uv.x, uv.y, 0, alpha)};
      cube[9] = {Point4::xyz0(Point3(0.5f, -0.5f, 0.5f) * tm), Point4(uv.x + uvs, uv.y, 0, alpha)};
      cube[10] = {Point4::xyz0(Point3(0.5f, 0.5f, 0.5f) * tm), Point4(uv.x + uvs, uv.y + uvs, 0, alpha)};
      cube[11] = {Point4::xyz0(Point3(-0.5f, 0.5f, 0.5f) * tm), Point4(uv.x, uv.y + uvs, 0, alpha)};
      //
      cube[12] = {Point4::xyz0(Point3(-0.5f, -0.5f, -0.5f) * tm), Point4(uv.x, uv.y, 0, alpha)};
      cube[13] = {Point4::xyz0(Point3(-0.5f, 0.5f, -0.5f) * tm), Point4(uv.x + uvs, uv.y, 0, alpha)};
      cube[14] = {Point4::xyz0(Point3(0.5f, 0.5f, -0.5f) * tm), Point4(uv.x + uvs, uv.y + uvs, 0, alpha)};
      cube[15] = {Point4::xyz0(Point3(0.5f, -0.5f, -0.5f) * tm), Point4(uv.x, uv.y + uvs, 0, alpha)};
      //
      cube[16] = {Point4::xyz0(Point3(-0.5f, -0.5f, -0.5f) * tm), Point4(uv.x, uv.y, 0, alpha)};
      cube[17] = {Point4::xyz0(Point3(-0.5f, -0.5f, 0.5f) * tm), Point4(uv.x + uvs, uv.y, 0, alpha)};
      cube[18] = {Point4::xyz0(Point3(-0.5f, 0.5f, 0.5f) * tm), Point4(uv.x + uvs, uv.y + uvs, 0, alpha)};
      cube[19] = {Point4::xyz0(Point3(-0.5f, 0.5f, -0.5f) * tm), Point4(uv.x, uv.y + uvs, 0, alpha)};
      //
      cube[20] = {Point4::xyz0(Point3(0.5f, -0.5f, -0.5f) * tm), Point4(uv.x, uv.y, 0, alpha)};
      cube[21] = {Point4::xyz0(Point3(0.5f, 0.5f, -0.5f) * tm), Point4(uv.x + uvs, uv.y, 0, alpha)};
      cube[22] = {Point4::xyz0(Point3(0.5f, 0.5f, 0.5f) * tm), Point4(uv.x + uvs, uv.y + uvs, 0, alpha)};
      cube[23] = {Point4::xyz0(Point3(0.5f, -0.5f, 0.5f) * tm), Point4(uv.x, uv.y + uvs, 0, alpha)};
    }
    const float HM_SIZE = 50.0f;
    for (int quadX = 0; quadX < QUADS_HM_SIDE; ++quadX)
      for (int quadY = 0; quadY < QUADS_HM_SIDE; ++quadY)
      {
        VertexData *quad = vbData + NUM_CUBE * 24 + (quadX * QUADS_HM_SIDE + quadY) * 4;
        float step = 1.0f / QUADS_HM_SIDE;
        quad[0] = {Point4(HM_SIZE * (-0.5f + (quadX + 0.0f) * step), -15.0f, HM_SIZE * (-0.5f + (quadY + 0.0f) * step), 15.0f),
          Point4((quadX + 0.0f) * step, (quadY + 0.0f) * step, 0, 1)};
        quad[1] = {Point4(HM_SIZE * (-0.5f + (quadX + 0.0f) * step), -15.0f, HM_SIZE * (-0.5f + (quadY + 1.0f) * step), 15.0f),
          Point4((quadX + 1.0f) * step, (quadY + 0.0f) * step, 0, 1)};
        quad[2] = {Point4(HM_SIZE * (-0.5f + (quadX + 1.0f) * step), -15.0f, HM_SIZE * (-0.5f + (quadY + 1.0f) * step), 15.0f),
          Point4((quadX + 1.0f) * step, (quadY + 1.0f) * step, 0, 1)};
        quad[3] = {Point4(HM_SIZE * (-0.5f + (quadX + 1.0f) * step), -15.0f, HM_SIZE * (-0.5f + (quadY + 0.0f) * step), 15.0f),
          Point4((quadX + 0.0f) * step, (quadY + 1.0f) * step, 0, 1)};
      }
  }
  vb.getBuf()->unlock();

  static CompiledShaderChannelId channels[] = {{SCTYPE_FLOAT4, SCUSAGE_POS, 0, 0}, {SCTYPE_FLOAT4, SCUSAGE_TC, 0, 0}};
  G_ASSERT(benchShader.material->checkChannels(channels, countof(channels)));
  G_UNUSED(channels);

  int level_count = get_log2i(RANDOM_BUFFER_RESOLUTION);
  randomTex =
    dag::create_tex(NULL, RANDOM_BUFFER_RESOLUTION, RANDOM_BUFFER_RESOLUTION, TEXFMT_A8R8G8B8, level_count, "gpu_benchmark_tex");
  randomTex.setVarId(::get_shader_glob_var_id("gpu_benchmark_tex"));
  for (int lev = 0; lev < level_count; ++lev)
  {
    uint8_t *randomBufferData = NULL;
    int randomBufferStride = 0;
    if (!randomTex->lockimg((void **)&randomBufferData, randomBufferStride, lev, TEXLOCK_WRITE) || !randomBufferData)
    {
      randomTex->unlockimg();
      continue;
    }
    for (int y = 0; y < RANDOM_BUFFER_RESOLUTION >> lev; ++y, randomBufferData += randomBufferStride)
      for (int x = 0; x < RANDOM_BUFFER_RESOLUTION >> lev; ++x)
        for (int c = 0; c < 4; ++c)
          randomBufferData[x * 4 + c] = clamp(gauss_rnd() * 0.5f + 0.5f, 0.0f, 1.0f) * 255;
    randomTex->unlockimg();
  }
  randomTex->texaddr(TEXADDR_WRAP);
  randomTex->texfilter(TEXFILTER_SMOOTH);

  gpu_benchmark_hmapVarId = get_shader_variable_id("gpu_benchmark_hmap");
}

GpuBenchmark::~GpuBenchmark() { index_buffer::release_quads_32bit(); }

void GpuBenchmark::update(float dt)
{
  rotAngle += dt;

  if (lastGpuTime > 0 && numWarmGpuFrames > WARM_GPU_FRAMES)
  {
    totalGpuTime += lastGpuTime;
    ++numGpuFrames;
  }
  ++numWarmGpuFrames;
}

void GpuBenchmark::render(Texture *target_tex, Texture *depth_tex)
{
  TIME_D3D_PROFILE(gpu_benchmark);
  uint64_t gpuTimeMs;
  timingIdx = (timingIdx + 1) % timings.size();
  if (timings[timingIdx].read(gpuTimeMs, 1000000))
    lastGpuTime = gpuTimeMs;
  else
    lastGpuTime = 0;
  timings[timingIdx].start();

  SCOPE_VIEW_PROJ_MATRIX;
  SCOPE_RENDER_TARGET;

  d3d::set_render_target(target_tex, 0);
  if (depth_tex)
    d3d::set_depth(depth_tex, DepthAccess::RW);
  else
    d3d::set_backbuf_depth();
  d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, E3DCOLOR(159, 159, 255, 255), 0.0f, 0);

  d3d::settm(TM_WORLD, TMatrix::IDENT);

  Quat rot(Point3(0, 1, 0), rotAngle);
  TMatrix4 viewTm = matrix_look_at_lh(rot * Point3(20, 15, 20), Point3(0, 0, 0), Point3(0, 1, 0));
  d3d::settm(TM_VIEW, &viewTm);

  int w, h;
  d3d::get_target_size(w, h);
  float wk = deg_to_fov(90.0f);
  float hk = wk * w / h;
  TMatrix4 projTm = matrix_perspective(wk, hk, 0.1f, 1000.0f);
  d3d::settm(TM_PROJ, &projTm);

  index_buffer::Quads32BitUsageLock quads32Lock;
  d3d::setvsrc(0, vb.getBuf(), sizeof(Point4) * 2);

  randomTex.setVar();

  ShaderGlobal::set_int(gpu_benchmark_hmapVarId, 1);
  if (!benchShader.shader->setStates())
    return;
  d3d_err(d3d::drawind(PRIM_TRILIST, 0, TRI_COUNT_HM, NUM_CUBE * 24));

  ShaderGlobal::set_int(gpu_benchmark_hmapVarId, 0);
  if (!benchShader.shader->setStates())
    return;
  d3d_err(d3d::drawind(PRIM_TRILIST, 0, TRI_COUNT_CUBE, 0));

  timings[timingIdx].stop();
}

void GpuBenchmark::resetTimings()
{
  lastGpuTime = 0;
  totalGpuTime = 0;
  numGpuFrames = 0;
  numWarmGpuFrames = 0;
}