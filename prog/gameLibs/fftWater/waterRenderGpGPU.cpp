#include <3d/dag_drv3d.h>
#include <perfMon/dag_statDrv.h>
#include <FFT_CPU_Simulation.h>
#include <shaders/dag_shaders.h>
#include <math/dag_adjpow2.h>
#include <math/dag_Point4.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include "waterGPGPUData.h"

// fft
static int current_water_timeVarId = -1;
static int choppy_scale0123VarId = -1;
static int choppy_scale4567VarId = -1;
static int fft_sizeVarId = -1;
static int k_texVarId = -1;
static int omega_texVarId = -1;
static int fft_source_texture_no = -1;
static int butterfly_texVarId = -1;
// update h0
static int gauss_texVarId = -1;
static int small_waves_fractionVarId = -1;

struct Ht0Vertex
{
  Point2 pos;
  Point2 tc;
  Point4 fft_tc;
  Point4 gaussTc__norm;
  Point4 windows;
  Point4 ampSpeed;
  Point4 windProp;
};

static void bit_reverse(float *aIndices, int N, int n)
{
  N *= 2;
  unsigned int mask = 0x1;
  for (int j = 0; j < N; j++)
  {
    unsigned int val = 0x0;
    int temp = int(aIndices[j]);
    for (int i = 0; i < n; i++)
    {
      unsigned int t = (mask & temp);
      val = (val << 1) | t;
      temp = temp >> 1;
    }
    aIndices[j] = float(val);
  }
}

static void calc_weights(float **aaIndices, float **aaWeights, const int N)
{
  unsigned int i, i1, j, i2, l, l1, l2;
  float c1, c2, u1, u2, z;

  // Compute the FFT
  c1 = -1.0f;
  c2 = 0.0f;
  l2 = 1;
  const int m = get_log2w(N);
  for (l = 0; l < m; l++)
  {
    l1 = l2;
    l2 <<= 1;
    u1 = 1.0f;
    u2 = 0.0f;
    for (j = 0; j < l1; j++)
    {
      for (i = j; i < N; i += l2)
      {
        i1 = i + l1;
        aaWeights[l][i * 2] = u1;
        aaWeights[l][i * 2 + 1] = u2;
        aaWeights[l][i1 * 2] = -u1;
        aaWeights[l][i1 * 2 + 1] = -u2;

        aaIndices[l][i * 2 + 0] = i;
        aaIndices[l][i * 2 + 1] = i1;
        aaIndices[l][i1 * 2 + 0] = i;
        aaIndices[l][i1 * 2 + 1] = i1;
      }
      z = u1 * c1 - u2 * c2;
      u2 = u1 * c2 + u2 * c1;
      u1 = z;
    }
    c2 = sqrtf((1.0f - c1) / 2.0f); // c2 == 1, sqrtf(1/2), sqrtf((1-sqrt(0.5))/2)
    c1 = sqrtf((1.0f + c1) / 2.0f); // c1 == 0, sqrtf(1/2), sqrtf(1+sqrt(0.5)/2)
  }
  bit_reverse(aaIndices[0], N, m);
}

void GPGPUData::reset()
{
  h0Mat = NULL;
  h0Element = NULL;
  ht0Vbuf = NULL;
  ht0Ibuf = NULL;
  fftVbuf = NULL;
  fftVMat = NULL;
  fftVElement = NULL;
  fftHMat = NULL;
  fftHElement = NULL;
  h0GPUUpdateRequired = true;
  omegaGPUUpdateRequired = true;
}

void GPGPUData::close()
{
  for (int i = 0; i < 3; ++i)
  {
    htPing[i].close();
    htPong[i].close();
  }
  ShaderGlobal::set_texture(k_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(omega_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(butterfly_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(gauss_texVarId, BAD_TEXTUREID);

  ht0.close();
  omega.close();
  gauss.close();
  butterfly.close();

  h0Element = NULL; // deleted by refcount within h0Mat
  del_it(h0Mat);
  fftHElement = NULL;
  fftVElement = NULL;
  del_it(fftHMat);
  del_it(fftVMat);
  del_d3dres(ht0Vbuf);
  del_d3dres(ht0Ibuf);
  del_d3dres(fftVbuf);

  for (int cascadeNo = 0; cascadeNo < dispGPU.size(); ++cascadeNo)
    dispGPU[cascadeNo].close();

  dispArray.close();
}

bool GPGPUData::fillOmega(const NVWaveWorks_FFT_CPU_Simulation *fft, const int numCascades)
{
  if (!omegaGPUUpdateRequired)
    return true;
  omegaGPUUpdateRequired = false;
  G_ASSERT(omega.getTex2D());
  const int N = 1 << fft[0].getParams().fft_resolution_bits;
  Texture *omegaTex = omega.getTex2D();
  uint8_t *data;
  int stride;
  if (!omegaTex->lockimg((void **)&data, stride, 0, TEXLOCK_WRITE))
    return false;
  for (int y = 0; y < N; ++y, data += stride)
  {
    Point2 *omegaData = (Point2 *)data;
    for (int i = 0; i < numCascades; i += 2, omegaData += N)
    {
      for (int x = 0; x < N; ++x)
      {
        Point2 cOmega(0, 0);
        cOmega[0] = fft[i].getOmegaData(y)[x];
        cOmega[1] = i + 1 < numCascades ? fft[i + 1].getOmegaData(y)[x] : cOmega[0];
        omegaData[x] = cOmega;
      }
    }
  }
  omegaTex->unlockimg();
  return true;
}

bool GPGPUData::init(const NVWaveWorks_FFT_CPU_Simulation *fft, int numCascades)
{
  G_ASSERT(numCascades <= MAX_NUM_CASCADES);

  const int N = 1 << fft[0].getParams().fft_resolution_bits;

  const int tileW = (numCascades + 1) / 2;

  // better be replaced with TEXFMT_A16B16G16R16, but with scale
  unsigned fmt = TEXFMT_A16B16G16R16F; // TEXFMT_A32B32G32R32F;//

  // better be replaced with TEXFMT_A16B16G16R16, but with scale
  ht0 = dag::create_tex(NULL, (N + 1) * tileW, N + 1, TEXCF_RTARGET | fmt, 1, "fftHt0Tex"); // TEXFMT_A16B16G16R16F
  // 129*2 * 129*4*4 - bytes
  ht0.getTex2D()->texfilter(TEXFILTER_POINT);
  ht0.getTex2D()->texaddr(TEXADDR_CLAMP);

  k_texVarId = get_shader_variable_id("k_tex");

  int gauss_resolution, gauss_stride;
  const cpu_types::float2 *in_gauss = get_global_gauss_data(gauss_resolution, gauss_stride);
  gauss = dag::create_tex(NULL, gauss_resolution + 1, gauss_resolution + 1, TEXFMT_G32R32F, 1, "gaussTex"); // can be replaced with
                                                                                                            // G16R16, with scale?
  gauss.getTex2D()->texfilter(TEXFILTER_POINT);
  gauss.getTex2D()->texaddr(TEXADDR_CLAMP);

  gauss_texVarId = get_shader_variable_id("gauss_tex");

  uint8_t *data;
  int stride;
  if (!gauss.getTex2D()->lockimg((void **)&data, stride, 0, TEXLOCK_WRITE))
    return false;
  for (int y = 0; y <= gauss_resolution; ++y, data += stride, in_gauss += gauss_stride)
    memcpy(data, in_gauss, (gauss_resolution + 1) * sizeof(Point2));
  gauss.getTex2D()->unlockimg();
  ShaderGlobal::set_texture(gauss_texVarId, gauss);


  for (int i = 0; i < 3; ++i)
  {
    String texName;
    texName.printf(128, "htPingTex%d", i);
    htPing[i] = dag::create_tex(NULL, N * tileW, N, TEXCF_RTARGET | fmt, 1, texName.str());
    htPing[i].getTex2D()->texfilter(TEXFILTER_POINT);
    htPing[i].getTex2D()->texaddr(TEXADDR_CLAMP);

    texName.printf(128, "htPongTex%d", i);
    htPong[i] = dag::create_tex(NULL, N * tileW, N, TEXCF_RTARGET | fmt, 1, texName.str());
    htPong[i].getTex2D()->texfilter(TEXFILTER_POINT);
    htPong[i].getTex2D()->texaddr(TEXADDR_CLAMP);
  }
  htRenderer.init("update_ht");
  current_water_timeVarId = get_shader_variable_id("current_water_time");
  fft_sizeVarId = get_shader_variable_id("fft_size");
  omega_texVarId = get_shader_variable_id("omega_tex");

  // fixme:omega can be replaced with G16R16 but with scale(s)
  omega = dag::create_tex(NULL, N * tileW, N, TEXFMT_G32R32F, 1, "omegaTex"); // 128*2 * 128*2*4 - bytes
  omega.getTex2D()->texfilter(TEXFILTER_POINT);
  omega.getTex2D()->texaddr(TEXADDR_CLAMP);
  fft_source_texture_no = ShaderGlobal::get_int_fast(get_shader_variable_id("fft_source_texture_no"));
  butterfly_texVarId = get_shader_variable_id("butterfly_tex");
  choppy_scale0123VarId = get_shader_variable_id("choppy_scale0123", true);
  choppy_scale4567VarId = get_shader_variable_id("choppy_scale4567", true);
  const int miNumButterflies = get_log2w(N);

  // can be replaced with argb8 texture (for up to 256x256)! coordinates. weight can be indexed, there are only 2*log2(N) of them.
  // Should be faster, than floats!
  butterfly = dag::create_tex(NULL, N, miNumButterflies, TEXFMT_A32B32G32R32F, 1, "butterflyIndex");
  butterfly.getTex2D()->texfilter(TEXFILTER_POINT);
  butterfly.getTex2D()->texaddr(TEXADDR_CLAMP);
  if (!butterfly.getTex2D()->lockimg((void **)&data, stride, 0, TEXLOCK_WRITE))
    return false;

  float **aaIndices = new float *[miNumButterflies];
  float **aaWeights = new float *[miNumButterflies];
  for (int i = 0; i < miNumButterflies; i++)
  {
    aaIndices[i] = new float[2 * N];
    aaWeights[i] = new float[2 * N];
  }

  calc_weights(aaIndices, aaWeights, N);

  // float* pButterFlyData = new float[miDimention * miNumButterflies * 4];

  for (int y = 0; y < miNumButterflies; ++y, data += stride)
  {
    Point4 *indexWeight = (Point4 *)data;
    for (int x = 0; x < N; ++x, indexWeight++)
    {
      indexWeight->x = (aaIndices[y][2 * x] + 0.5f) / float(N);
      indexWeight->y = (aaIndices[y][2 * x + 1] + 0.5f) / float(N);
      indexWeight->z = aaWeights[y][2 * x];
      indexWeight->w = aaWeights[y][2 * x + 1];
    }
  }
  butterfly.getTex2D()->unlockimg();
  for (int i = 0; i < miNumButterflies; i++)
  {
    delete aaIndices[i];
    delete aaWeights[i];
  }
  delete[] aaIndices;
  delete[] aaWeights;

  ShaderGlobal::set_texture(butterfly_texVarId, butterfly);

  displacementsRenderer.init("fft_displacements");

  h0Mat = new_shader_material_by_name("update_ht0");
  G_ASSERT(h0Mat);
  h0Element = h0Mat->make_elem();
  int num_quads = (numCascades + 1) / 2;
  ht0Ibuf = d3d::create_ib(sizeof(uint16_t) * 6 * num_quads, 0);
  d3d_err(ht0Ibuf);
  uint16_t *indices;
  d3d_err(ht0Ibuf->lock(0, 0, &indices, VBLOCK_WRITEONLY));
  for (int i = 0; i < num_quads; ++i, indices += 6)
  {
    int base = i * 4;
    indices[0] = base;
    indices[1] = base + 1;
    indices[2] = base + 2;
    indices[3] = base + 2;
    indices[4] = base + 1;
    indices[5] = base + 3;
  }
  ht0Ibuf->unlock();

  // create vbuffer for fft/butterflies
  fftVbuf = d3d::create_vb(sizeof(Point3) * 3 * miNumButterflies, 0, "fftVbuf");
  d3d_err(fftVbuf);
  Point3 *fftVertices;
  d3d_err(fftVbuf->lock(0, 0, (void **)&fftVertices, VBLOCK_WRITEONLY));
  for (int i = 0; i < miNumButterflies; ++i, fftVertices += 3)
  {
    float index = (i + 0.5f) / miNumButterflies;
    fftVertices[0] = Point3(-1, +1, index);
    fftVertices[1] = Point3(-1, -3, index);
    fftVertices[2] = Point3(+3, +1, index);
  }
  fftVbuf->unlock();
  fftVMat = new_shader_material_by_name("fftV");
  G_ASSERT(fftVMat);
  fftVElement = fftVMat->make_elem();
  fftHMat = new_shader_material_by_name("fftH");
  G_ASSERT(fftHMat);
  fftHElement = fftHMat->make_elem();

  small_waves_fractionVarId = get_shader_variable_id("small_waves_fraction");
  updateHt0WindowsVB(fft, numCascades);

  dispArray = dag::create_array_tex(N, N, numCascades, TEXCF_RTARGET | TEXFMT_A16B16G16R16F, 1, "water3d_disp_gpu");
  if (!dispArray)
  {
    for (int cascadeNo = 0; cascadeNo < numCascades; ++cascadeNo)
    {
      String texName;
      texName.printf(128, "water3d_disp_gpu%d", cascadeNo);
      dispGPU[cascadeNo] = dag::create_tex(NULL, N, N, TEXCF_RTARGET | TEXFMT_A16B16G16R16F, 1, texName.str());
    }
  }
  return true;
}

void GPGPUData::updateHt0WindowsVB(const NVWaveWorks_FFT_CPU_Simulation *fft, int numCascades)
{
  ShaderGlobal::set_real(small_waves_fractionVarId, fft[0].getParams().small_wave_fraction);

  int gauss_resolution, gauss_stride;
  get_global_gauss_data(gauss_resolution, gauss_stride);

  int num_quads = (numCascades + 1) / 2;
  Ht0Vertex *vertices;
  del_d3dres(ht0Vbuf);
  ht0Vbuf = d3d::create_vb(sizeof(Ht0Vertex) * 4 * num_quads, 0, "ht0Vbuf");
  d3d_err(ht0Vbuf);
  G_ASSERT(ht0Vbuf);
  d3d_err(ht0Vbuf->lock(0, 0, (void **)&vertices, VBLOCK_WRITEONLY));
  const int N = 1 << fft[0].getParams().fft_resolution_bits;
  G_ASSERT(N <= gauss_resolution);
  float gauss_corner0 = (gauss_resolution - N) * 0.5f / (gauss_resolution + 1.0f);
  float halfTexelOffsetX = 0.5f;
  float halfTexelOffsetY = 0.5f;
  float gauss_corner1 = 1.0 - gauss_corner0;
  gauss_corner0 += HALF_TEXEL_OFSF / (gauss_resolution + 1.0f);
  gauss_corner1 += HALF_TEXEL_OFSF / (gauss_resolution + 1.0f);
  float quad_width = 2.0f / num_quads;
  for (int i = 0; i < num_quads; ++i, vertices += 4)
  {
    int c0 = i * 2;
    int c1 = c0 + 1 < numCascades ? c0 + 1 : c0;
    float fft_period0 = fft[c0].getParams().fft_period;
    float fft_period1 = fft[c1].getParams().fft_period;

    // kStep = 2PI/L; norm = kStep * sqrt(1/2)
    float norm0 = 2 * PI * 0.7071068 / (fft_period0), norm1 = 2 * PI * 0.7071068 / (fft_period1);
    vertices[0].pos = Point2(-1 + i * quad_width, +1);
    vertices[1].pos = Point2(vertices[0].pos.x + quad_width, +1);
    vertices[2].pos = Point2(vertices[0].pos.x, -1);
    vertices[3].pos = Point2(vertices[0].pos.x + quad_width, -1);

    vertices[0].tc = Point2(-N / 2 - halfTexelOffsetX + HALF_TEXEL_OFSF, -N / 2 - halfTexelOffsetY + HALF_TEXEL_OFSF);
    vertices[1].tc = Point2(+N / 2 + halfTexelOffsetX + HALF_TEXEL_OFSF, -N / 2 - halfTexelOffsetY + HALF_TEXEL_OFSF);
    vertices[2].tc = Point2(-N / 2 - halfTexelOffsetX + HALF_TEXEL_OFSF, +N / 2 + halfTexelOffsetY + HALF_TEXEL_OFSF);
    vertices[3].tc = Point2(+N / 2 + halfTexelOffsetX + HALF_TEXEL_OFSF, +N / 2 + halfTexelOffsetY + HALF_TEXEL_OFSF);
    for (int j = 0; j < 4; ++j)
    {
      vertices[j].fft_tc = Point4(vertices[j].tc.x * TWOPI / fft_period0, vertices[j].tc.y * TWOPI / fft_period0,
        vertices[j].tc.x * TWOPI / fft_period1, vertices[j].tc.y * TWOPI / fft_period1);
    }

    vertices[0].gaussTc__norm = Point4(gauss_corner0, gauss_corner0, norm0, norm1);
    vertices[1].gaussTc__norm = Point4(gauss_corner1, gauss_corner0, norm0, norm1);
    vertices[2].gaussTc__norm = Point4(gauss_corner0, gauss_corner1, norm0, norm1);
    vertices[3].gaussTc__norm = Point4(gauss_corner1, gauss_corner1, norm0, norm1);

    Point2 window0 = Point2(fft[c0].getParams().window_in, fft[c0].getParams().window_out);
    Point2 window1 = Point2(fft[c1].getParams().window_in, fft[c1].getParams().window_out);
    Point4 windows(window0.x, window0.y, window1.x, window1.y);
    vertices[0].windows = windows;
    vertices[1].windows = windows;
    vertices[2].windows = windows;
    vertices[3].windows = windows;

    Point4 windProp = Point4(fft[c0].getParams().wind_dependency, fft[c1].getParams().wind_dependency,
      fft[c0].getParams().wind_alignment, fft[c1].getParams().wind_alignment);
    vertices[0].windProp = windProp;
    vertices[1].windProp = windProp;
    vertices[2].windProp = windProp;
    vertices[3].windProp = windProp;

    Point2 ampSpeed0 = Point2(fft[c0].getParams().wave_amplitude, fft[c0].getParams().wind_speed);
    Point2 ampSpeed1 = Point2(fft[c1].getParams().wave_amplitude, fft[c1].getParams().wind_speed);
    // Use square of amplitude, because Phillips is an *energy* spectrum
    ampSpeed0.x *= ampSpeed0.x;
    ampSpeed1.x *= ampSpeed1.x;
    // Encoded vector for passing into shaders
    Point4 ampSpeed = Point4(ampSpeed0.x, ampSpeed0.y, ampSpeed1.x, ampSpeed1.y);

    vertices[0].ampSpeed = ampSpeed;
    vertices[1].ampSpeed = ampSpeed;
    vertices[2].ampSpeed = ampSpeed;
    vertices[3].ampSpeed = ampSpeed;
  }
  ht0Vbuf->unlock();
  h0GPUUpdateRequired = true;
  omegaGPUUpdateRequired = true;
}

void GPGPUData::updateH0(int numCascades)
{
  if (!h0GPUUpdateRequired)
    return;
  if (!h0Element)
    return;
  ShaderGlobal::set_texture(k_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(gauss_texVarId, gauss);
  d3d::set_render_target(ht0.getTex2D(), 0);
  h0Element->setStates(0, true);
  d3d::setind(ht0Ibuf);
  d3d::setvsrc(0, ht0Vbuf, sizeof(Ht0Vertex));
  int num_quads = (numCascades + 1) / 2;
  d3d::drawind(PRIM_TRILIST, 0, 2 * num_quads, 0);

  ShaderGlobal::set_texture(k_texVarId, ht0);

  /*const int N = fft[0].getParams().fft_resolution;
  for (int i = 0; i < numCascades; ++i)
  {
    if (fft[i].IsH0UpdateRequired())
    {
      for (int j = 0; j <= fft[i].getParams().fft_resolution; ++j)
        fft[i].UpdateH0(j);
      fft[i].SetH0UpdateNotRequired();
    }
  }
  SmallTab<Point4, TmpmemAlloc> gpuHt0;
  gpuHt0.resize((N+1)*2*(N+1));
  uint8_t * data; int stride;
  d3d_err(ht0Tex->lockimg((void**)&data, stride, 0, TEXLOCK_READ));
  for (int y=0; y <= N; ++y, data+=stride)
    memcpy(gpuHt0.data()+y*(N+1)*2, data, (N+1)*2*sizeof(Point4));
  ht0Tex->unlockimg();
  for (int i = 0; i<numCascades; ++i)//numCascades
  {
    Point4 *gpuData = (Point4*)((Point2*)(gpuHt0.data() + (i/2)*(N+1)) + (i&1));
    String cpustr, gpustr;
    Point2 maxDiff(0,0), avgDiff(0,0);
    IPoint2 diffX(0,0), diffY(0,0);
    for (int y = 0; y<=N; ++y)
    {
      const cpu_types::float2 *h0 = fft[i].getH0Data(y);
      Point4 *gpuRow = gpuData+(N+1)*2*y;
      cpustr.aprintf(128, "%d:",y);
      gpustr.aprintf(128, "%d:",y);
      for (int x = 0; x <= N; ++x)
      {
        Point2 diff(fabsf(h0[x].x-gpuRow[x].x), fabsf(h0[x].y-gpuRow[x].y));
        avgDiff+=diff/(N+1.f);
        if (maxDiff.x < diff.x)
        {
          maxDiff.x = diff.x;
          diffX = IPoint2(x, y);
        }
        if (maxDiff.y < diff.y)
        {
          maxDiff.y = diff.y;
          diffY = IPoint2(x, y);
        }
        cpustr.aprintf(128, "(%g %g) ", h0[x].x, h0[x].y);
        gpustr.aprintf(128, "(%g %g) ", gpuRow[x].x, gpuRow[x].y);
      }
      cpustr.aprintf(128, "\n");
      gpustr.aprintf(128, "\n");
    }
    debug("cascade %d diff %@, max %@: at %@ %@", i, avgDiff/(N+1), maxDiff, diffX, diffY);
    debug("at %@ cpu (%g %g) gpu (%g %g)", diffX, fft[i].getH0Data(diffX.y)[diffX.x].x, fft[i].getH0Data(diffX.y)[diffX.x].y,
      gpuData[diffX.y*(N+1)*2+diffX.x].x, gpuData[diffX.y*(N+1)*2+diffX.x].y);
    debug("at %@ cpu (%g %g) gpu (%g %g)", diffY, fft[i].getH0Data(diffY.y)[diffY.x].x, fft[i].getH0Data(diffY.y)[diffY.x].y,
      gpuData[diffY.y*(N+1)*2+diffY.x].x, gpuData[diffY.y*(N+1)*2+diffY.x].y);
    if (i==1)
    {
      debug("CPU %d:\n\n%s\n\n", i, cpustr.str());
      debug("GPU %d:\n\n%s\n\n", i, gpustr.str());
    }
  }*/
  h0GPUUpdateRequired = false;
}

void GPGPUData::perform(const NVWaveWorks_FFT_CPU_Simulation *fft, int numCascades, double time)
{
  if (!h0Element)
    return;
  TIME_D3D_PROFILE(gpGPUFFT);
  const int N = 1 << fft[0].getParams().fft_resolution_bits;
  fillOmega(fft, numCascades);
  Driver3dRenderTarget prevRt;
  d3d::get_render_target(prevRt);
  updateH0(numCascades);
  d3d::set_render_target();

#if _TARGET_TVOS
  for (int i = 0; i < 2; ++i)
#else
  for (int i = 0; i < 3; ++i)
#endif
    d3d::set_render_target(i, htPing[i].getTex2D(), 0);
  ShaderGlobal::set_texture(omega_texVarId, omega);
  ShaderGlobal::set_int(fft_sizeVarId, N);
  ShaderGlobal::set_real(current_water_timeVarId, time);
  htRenderer.render();

#if _TARGET_TVOS
  d3d::set_render_target(0, htPing[2].getTex2D(), 0);
  htRenderer.render();
#endif

  // check!
  /*static SmallTab<Point2, TmpmemAlloc> htGpu[MAX_NUM_CASCADES];
  if (!htGpu[0].size())
  {
    for (int i = 0; i < numCascades; ++i)
      htGpu[i].resize(N*N*3);
  }
  if (water_debug)
  {
    for (int i = 0; i < 3; ++i)
    {
      uint8_t *data; int stride;
      if (htPingTex[i]->lockimg((void**)&data, stride,0, TEXLOCK_READ))
      {
        Point2 maxD(0,0), avgD(0,0);
        IPoint2 maxX(0,0), maxY(0,0);
        for (int y = 0; y < N; ++y, data+=stride)
        {
          Point4 *ht = (Point4*) data;
          if (!i)
          {
            int ofs = y*N;
            for (int cascade = 0; cascade < numCascades; cascade+=2, ht+=N)
            {
              for (int x = 0; x < N; ++x)
              {
                htGpu[cascade][x+ofs] = Point2::xy(ht[x]);
                htGpu[cascade+1][x+ofs] = Point2(ht[x].z, ht[x].w);
              }
            }
          } else
          {
            int ofs = y*N + N*N;
            int cascadeOfs = (i==1) ? 0 : 1;
            for (int cascade = 0; cascade < numCascades; cascade+=2, ht+=N)
            {
              for (int x = 0; x < N; ++x)
              {
                htGpu[cascade+cascadeOfs][x+ofs] = Point2::xy(ht[x]);
                htGpu[cascade+cascadeOfs][x+ofs+N*N] = Point2(ht[x].z, ht[x].w);
              }
            }
          }
        }
        htPingTex[i]->unlockimg();
      }
    }

    for (int index = 0; index < 3; ++index)
    {
      debug("index = %d",index);
      int ofsIndex = index*N*N;
      for (int i = 0; i < numCascades; ++i)
      {
        String rowcpu, rowgpu;
        Point2 maxD(0,0), avgD(0,0);
        IPoint2 maxX(0,0), maxY(0,0);
        for (int j = 0; j < N; ++j)
        {
          rowgpu.aprintf(128, "\n");
          rowcpu.aprintf(128, "\n");
          for (int x = 0; x < N; ++x)
          {
            rowgpu.aprintf(128, "(%g %g) ", htGpu[i][j*N+x+ofsIndex].x, htGpu[i][j*N+x+ofsIndex].y);
            rowcpu.aprintf(128, "(%g %g) ", savedHt[i][j*N+x+ofsIndex].x, savedHt[i][j*N+x+ofsIndex].y);
            Point2 diff = Point2(fabsf(htGpu[i][j*N+x+ofsIndex].x-savedHt[i][j*N+x+ofsIndex].x),
                                 fabsf(htGpu[i][j*N+x+ofsIndex].y-savedHt[i][j*N+x+ofsIndex].y));
            avgD += diff/N;
            if (maxD.x<diff.x)
            {
              maxD.x = diff.x;
              maxX = IPoint2(x, j);
            }
            if (maxD.y < diff.y)
            {
              maxD.y = diff.y;
              maxY = IPoint2(x, j);
            }
          }
        }
        //debug("cascade%d CPU\n%s\n",i, rowcpu.str());
        //debug("cascade%d GPU\n%s\n",i, rowgpu.str());
        avgD/=N;
        debug("diff(%d) %@, max %@ (at %@ %@)",i, avgD,maxD, maxX, maxY);

        debug("diff(%d) vals gpu = %@ cpu=%@ gpu=%@ cpu=%@", i,
          htGpu[i][maxX.x+ maxX.y*N+ofsIndex],
          savedHt[i][maxX.x+ maxX.y*N+ofsIndex],
          htGpu[i][maxY.x+ maxY.y*N+ofsIndex],
          savedHt[i][maxY.x+ maxY.y*N+ofsIndex]
        );
      }
    }
  }*/

  ShaderGlobal::set_texture(butterfly_texVarId, butterfly);
  int log2N = get_log2w(N);
  bool isPing = false;
  d3d::setvsrc_ex(0, fftVbuf, 0, sizeof(Point3));
  fftHElement->setStates(0, true);
  // d3d::setvsrc_ex(0, fftVbuf, 0, sizeof(Point3));
  for (int butterflyI = 0; butterflyI < log2N; butterflyI++)
  {
#if _TARGET_TVOS
    for (int i = 0; i < 2; ++i)
#else
    for (int i = 0; i < 3; ++i)
#endif
    {
      d3d::set_render_target(i, isPing ? htPing[i].getTex2D() : htPong[i].getTex2D(), 0);
      d3d::settex(fft_source_texture_no + i, isPing ? htPong[i].getTex2D() : htPing[i].getTex2D());
    }
    // ShaderGlobal::set_texture(fft_source_textureVarId, isPing ? htPongTexId[i] : htPingTexId[i]);
    // ShaderGlobal::set_real(butterfly_indexVarId, float(butterfly+0.5f)/(log2N));//fixme: replace with per vertex data
    d3d::draw(PRIM_TRILIST, butterflyI * 3, 1);

#if _TARGET_TVOS
    d3d::set_render_target(0, isPing ? htPing[2].getTex2D() : htPong[2].getTex2D(), 0);
    d3d::settex(fft_source_texture_no + 0, isPing ? htPong[2].getTex2D() : htPing[2].getTex2D());
    d3d::draw(PRIM_TRILIST, butterflyI * 3, 1);
#endif

    isPing = !isPing;
  }

  fftVElement->setStates(0, true);
  // d3d::setvsrc_ex(0, fftVbuf, 0, sizeof(Point3));
  for (int butterflyI = 0; butterflyI < log2N; butterflyI++)
  {
#if _TARGET_TVOS
    for (int i = 0; i < 2; ++i)
#else
    for (int i = 0; i < 3; ++i)
#endif
    {
      d3d::set_render_target(i, isPing ? htPing[i].getTex2D() : htPong[i].getTex2D(), 0);
      d3d::settex(fft_source_texture_no + i, isPing ? htPong[i].getTex2D() : htPing[i].getTex2D());
    }
    // ShaderGlobal::set_real(butterfly_indexVarId, float(butterfly+0.5f)/(log2N));//fixme: replace with per vertex data
    // fftVPass.render();
    d3d::draw(PRIM_TRILIST, butterflyI * 3, 1);

#if _TARGET_TVOS
    d3d::set_render_target(0, isPing ? htPing[2].getTex2D() : htPong[2].getTex2D(), 0);
    d3d::settex(fft_source_texture_no + 0, isPing ? htPong[2].getTex2D() : htPing[2].getTex2D());
    d3d::draw(PRIM_TRILIST, butterflyI * 3, 1);
#endif

    isPing = !isPing;
  }

  /*if (water_debug)
  {
    debug("FFT=========================================");
    for (int i = 0; i < numCascades; ++i)
      memcpy(savedHt[i].data(), fft[i].getHtData(), data_size(savedHt[i]));

    for (int i = 0; i < 3; ++i)
    {
      Texture * src = isPing ? htPongTex[i] : htPingTex[i];
      uint8_t *data; int stride;
      if (src->lockimg((void**)&data, stride,0, TEXLOCK_READ))
      {
        Point2 maxD(0,0), avgD(0,0);
        IPoint2 maxX(0,0), maxY(0,0);
        for (int y = 0; y < N; ++y, data+=stride)
        {
          Point4 *ht = (Point4*) data;
          if (!i)
          {
            int ofs = y*N;
            for (int cascade = 0; cascade < numCascades; cascade+=2, ht+=N)
            {
              for (int x = 0; x < N; ++x)
              {
                htGpu[cascade][x+ofs] = Point2::xy(ht[x]);
                htGpu[cascade+1][x+ofs] = Point2(ht[x].z, ht[x].w);
              }
            }
          } else
          {
            int ofs = y*N + N*N;
            int cascadeOfs = (i==1) ? 0 : 1;
            for (int cascade = 0; cascade < numCascades; cascade+=2, ht+=N)
            {
              for (int x = 0; x < N; ++x)
              {
                htGpu[cascade+cascadeOfs][x+ofs] = Point2::xy(ht[x]);
                htGpu[cascade+cascadeOfs][x+ofs+N*N] = Point2(ht[x].z, ht[x].w);
              }
            }
          }
        }
        src->unlockimg();
      }
    }

    for (int index = 0; index < 3; ++index)
    {
      debug("index = %d",index);
      int ofsIndex = index*N*N;
      for (int i = 0; i < numCascades; ++i)
      {
        String rowcpu, rowgpu;
        Point2 maxD(0,0), avgD(0,0);
        IPoint2 maxX(0,0), maxY(0,0);
        for (int j = 0; j < N; ++j)
        {
          rowgpu.aprintf(128, "\n");
          rowcpu.aprintf(128, "\n");
          for (int x = 0; x < N; ++x)
          {
            rowgpu.aprintf(128, "(%g %g) ", htGpu[i][j*N+x+ofsIndex].x, htGpu[i][j*N+x+ofsIndex].y);
            rowcpu.aprintf(128, "(%g %g) ", savedHt[i][j*N+x+ofsIndex].x, savedHt[i][j*N+x+ofsIndex].y);
            Point2 diff = Point2(fabsf(htGpu[i][j*N+x+ofsIndex].x-savedHt[i][j*N+x+ofsIndex].x),
                                 fabsf(htGpu[i][j*N+x+ofsIndex].y-savedHt[i][j*N+x+ofsIndex].y));
            avgD += diff/N;
            if (maxD.x<diff.x)
            {
              maxD.x = diff.x;
              maxX = IPoint2(x, j);
            }
            if (maxD.y < diff.y)
            {
              maxD.y = diff.y;
              maxY = IPoint2(x, j);
            }
          }
        }
        //debug("cascade%d CPU\n%s\n",i, rowcpu.str());
        //debug("cascade%d GPU\n%s\n",i, rowgpu.str());
        avgD/=N;
        debug("diff(%d) %@, max %@ (at %@ %@)",i, avgD,maxD, maxX, maxY);

        debug("diff(%d) vals gpu = %@ cpu=%@ gpu=%@ cpu=%@", i,
          htGpu[i][maxX.x+ maxX.y*N+ofsIndex],
          savedHt[i][maxX.x+ maxX.y*N+ofsIndex],
          htGpu[i][maxY.x+ maxY.y*N+ofsIndex],
          savedHt[i][maxY.x+ maxY.y*N+ofsIndex]
        );
      }
    }
    debug("FFT----------------");
  }*/

  for (int i = 0; i < 3; ++i)
  {
    d3d::settex(fft_source_texture_no + i, isPing ? htPong[i].getTex2D() : htPing[i].getTex2D());
  }
  for (int i = 0; i < numCascades; ++i)
  {
    if (dispArray.getArrayTex())
      d3d::set_render_target(i, dispArray.getArrayTex(), i, 0);
    else
      d3d::set_render_target(i, dispGPU[i].getTex2D(), 0);
  }
  G_ASSERT(numCascades <= 5);
  if (VariableMap::isGlobVariablePresent(choppy_scale0123VarId) && ShaderGlobal::get_var_type(choppy_scale0123VarId) == SHVT_COLOR4 &&
      VariableMap::isGlobVariablePresent(choppy_scale4567VarId) && ShaderGlobal::get_var_type(choppy_scale4567VarId) == SHVT_COLOR4)
  {
    ShaderGlobal::set_color4(choppy_scale0123VarId,
      Color4(numCascades > 0 ? fft[0].getParams().choppy_scale : 1.0f, numCascades > 1 ? fft[1].getParams().choppy_scale : 1.0f,
        numCascades > 2 ? fft[2].getParams().choppy_scale : 1.0f, numCascades > 3 ? fft[3].getParams().choppy_scale : 1.0f));
    ShaderGlobal::set_color4(choppy_scale4567VarId,
      Color4(numCascades > 4 ? fft[4].getParams().choppy_scale : 1.0f, 1.0f, 1.0f, 1.0f));
  }
  displacementsRenderer.render();

  d3d::set_render_target(prevRt);
}
