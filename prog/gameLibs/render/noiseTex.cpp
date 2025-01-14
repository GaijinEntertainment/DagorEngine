// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_resPtr.h>
#include <3d/dag_lockTexture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_Point3.h>
#include <dag_noise/dag_uint_noise.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_spinlock.h>
#include <generic/dag_carray.h>
#include <generic/dag_smallTab.h>
#include <drv/3d/dag_resetDevice.h>
#include <gameRes/dag_gameResources.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>
#include <math/dag_adjpow2.h>
#include <image/dag_dxtCompress.h>
#include <math/dag_imageFunctions.h>
#include <image/dag_texPixel.h>

#include "coherentNoise.h"

#include <EASTL/algorithm.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/array.h>
#include <initializer_list>

enum
{
  NOISE_TEX_ARGB,
  NOISE_TEX_L8,
  NOISE_TEX_HASH,
  NOISE_PERLIN,
  NOISE_BLUE_RG8,
  NOISE_TEX_COUNT
};
static carray<SharedTexHolder, NOISE_TEX_COUNT> noiseTex;
static carray<volatile int, NOISE_TEX_COUNT> noiseTexCounter;
static carray<os_spinlock_t, NOISE_TEX_COUNT> noiseTexLock;

enum
{
  NOT_INITIALIZED,
  INITIALIZING,
  INITIALIZED
};
static volatile int atomicForInit = NOT_INITIALIZED;
static constexpr int NOISE_W = 64;
static constexpr int HASH_NOISE_W = 128;
static constexpr int BLUE_NOISE_W = 128;

namespace blue_noise_data
{
extern const unsigned char texture_128x128_rg8[];
}

void init_tex_noises()
{
  for (auto &l : noiseTexLock)
    os_spinlock_init(&l);
}

struct NoiseTexLock
{
  os_spinlock_t *lockPtr;
  NoiseTexLock(os_spinlock_t *noise_lock_ptr) : lockPtr(noise_lock_ptr)
  {
    if (interlocked_acquire_load(atomicForInit) != INITIALIZED)
    {
      if (interlocked_compare_exchange(atomicForInit, INITIALIZING, NOT_INITIALIZED) == NOT_INITIALIZED)
      {
        init_tex_noises();
        interlocked_release_store(atomicForInit, INITIALIZED);
      }
      while (interlocked_acquire_load(atomicForInit) != INITIALIZED)
        ; // VOID
    }
    os_spinlock_lock(lockPtr);
  }
  ~NoiseTexLock() { os_spinlock_unlock(lockPtr); }
};

struct Generates3DNoise
{
  PerlinNoise noise1, noise2, noise3;
  void fBm(float *out, float x, float y, float z, int period, int octs, float persistency, float global)
  {
    out[0] = out[1] = out[2] = 0;
    float scale = 1;
    float sum = 0;
    for (int i = 0; i < octs; ++i)
    {
      float rnd3 = noise3.perlin_noise3(x, y, z, period - 1);
      float rnd1 = noise1.perlin_noise3(x, y, z, period - 1);
      float rnd2 = noise2.perlin_noise3(x, y, z, period - 1);

      out[0] += scale * rnd1; // from 0 to 1;
      out[1] += scale * rnd2; // FRand()*2.f-1.f;
      out[2] += scale * rnd3; // FRand()*2.f-1.f;
      sum += scale;

      scale *= persistency;
      x *= 2;
      y *= 2;
      z *= 2;
      period += period;
    }
    // sumscale = 0.5/scale;
    scale = global / sum;
    out[0] = out[0] * scale;
    out[1] = out[1] * scale;
    out[2] = out[2] * scale;
  }
  static unsigned char gradient(float a, float b, float sz)
  {
    float grad = (b - a) * sz;
    grad = grad * 0.5f + 0.5f;
    return max(0.f, min(grad, 1.f)) * 255;
  }
  void generateTexture(int &noiseW, Point3 &maxR, Point3 &minR, SmallTab<uint8_t, TmpmemAlloc> &texData, int &mips, uint32_t fmt)
  {
    noiseW = 64;
    int period = 2;
    float origin[3];
    float perlinW = period;

    origin[2] = 0.5f * perlinW / noiseW;

    int numOctaves = get_log2i(noiseW);
    noise1.perlin_init(10);
    noise2.perlin_init(101);
    noise3.perlin_init(1011);
    SmallTab<float, TmpmemAlloc> slices;
    clear_and_resize(slices, noiseW * noiseW * noiseW * 4);
    float *result = slices.data();
    minR = Point3(1, 1, 1);
    maxR = -minR;
    ///*
    for (int d = 0; d < noiseW; ++d, origin[2] += perlinW / noiseW)
    {
      origin[1] = 0.5f * perlinW / noiseW;
      for (int y = 0; y < noiseW; ++y, origin[1] += perlinW / noiseW)
      {
        origin[0] = 0.5f * perlinW / noiseW;
        for (int x = 0; x < noiseW; ++x, origin[0] += perlinW / noiseW, result += 4)
        {
          fBm(result, origin[0], origin[1], origin[2], period, numOctaves, 0.71, 0.8);
          result[3] = 0;
          for (int i = 0; i < 3; ++i)
          {
            minR[i] = min(minR[i], result[i]);
            maxR[i] = max(maxR[i], result[i]);
          }
        }
      }
    }

    int row_pitch = noiseW * 2;
    int slice_pitch = noiseW * noiseW / 2;
    if (fmt == TEXFMT_DXT1)
    {
      slice_pitch = noiseW * noiseW / 2;
      row_pitch = noiseW * 2;
    }
    else if (fmt == TEXFMT_DXT5)
    {
      slice_pitch = noiseW * noiseW;
      row_pitch = noiseW * 4;
    }
    else
    {
      slice_pitch = noiseW * noiseW * 4;
      row_pitch = noiseW * 4;
    }
    clear_and_resize(texData, slice_pitch * noiseW);
    mips = 1;
    SmallTab<unsigned char, TmpmemAlloc> oneSlice;

    char *data = (char *)texData.data();
    for (int mip = 0, curW = noiseW; mip < mips; ++mip)
    {
      clear_and_resize(oneSlice, curW * curW * 4);
      result = slices.data();
      float scale[3] = {safediv(1.0f, maxR[0] - minR[0]), safediv(1.0f, maxR[1] - minR[1]), safediv(1.0f, maxR[2] - minR[2])};
      float mul[3] = {scale[0] * 255.f, scale[1] * 255.f, scale[2] * 255.f};
      float add[3] = {(-scale[0] * minR[0]) * 255.f, (-scale[1] * minR[1]) * 255.f, (-scale[2] * minR[2]) * 255.f};
      for (int d = 0; d < curW; ++d, data += slice_pitch)
      {
        unsigned char *slice = oneSlice.data();
        for (int y = 0; y < curW; ++y)
          for (int x = 0; x < curW; ++x, slice += 4, result += 4)
          {
            slice[0] = result[0] * mul[0] + add[0]; // red
            slice[1] = result[1] * mul[1] + add[1]; // green
            slice[2] = result[2] * mul[2] + add[2]; // blue
            slice[3] = slice[2];                    // blue
            if (fmt == TEXFMT_DXT5)
              slice[2] = slice[1]; // replace blue with green, to enhance encoding
          }
        if (fmt == TEXFMT_DXT1)
          ManualDXT(MODE_DXT1, (TexPixel32 *)(oneSlice.data()), curW, curW, row_pitch, data, DXT_ALGORITHM_EXCELLENT);
        else if (fmt == TEXFMT_DXT5)
          ManualDXT(MODE_DXT5, (TexPixel32 *)(oneSlice.data()), curW, curW, row_pitch, data, DXT_ALGORITHM_EXCELLENT);
        else
        {
          unsigned char *srcSlice = oneSlice.data();
          char *dstSlice = data;
          for (int y = 0; y < curW; ++y, dstSlice += row_pitch, srcSlice += curW * 4)
            memcpy(dstSlice, srcSlice, curW * 4);
        }
      }
      if (mip < mips - 1)
      {
        result = slices.data();
        for (int d = 0; d < curW; ++d, result += curW * curW * 4)
          imagefunctions::downsample4x_simda(result, result, curW >> 1, curW >> 1);

        result = slices.data();
        vec4f *dest = (vec4f *)result;
        for (int d = 0; d < curW / 2; ++d, result += curW * curW * 4 * 2)
        {
          vec4f *slice0 = (vec4f *)result;
          vec4f *slice1 = (vec4f *)(result + curW * curW * 4);
          vec4f h = V_C_HALF;
          for (int i = 0; i < curW * curW / 4; ++i, slice0++, slice1++, dest++)
            *dest = v_mul(v_add(*slice0, *slice1), h);
        }
        curW >>= 1;
      }
    }
  }
};

static bool init_argb8_64_noise(const SharedTexHolder &t)
{
  if (!t)
    return true;
  const int seed1 = 0x11231233, seed2 = 0x31271213, seed3 = 0x51261235, seed4 = 0xA12314F;
  if (auto lockedTex = lock_texture<E3DCOLOR>(t.getTex2D(), 0, TEXLOCK_WRITE | TEXLOCK_DELSYSMEMCOPY))
  {
    for (int j = 0, pos = 0; j < NOISE_W; ++j)
    {
      for (int i = 0; i < NOISE_W; ++i, ++pos)
      {
        lockedTex.at(i, j) = E3DCOLOR(uint_noise1D(pos, seed1) & 255, uint_noise1D(pos, seed2) & 255, uint_noise1D(pos, seed3) & 255,
          uint_noise1D(pos, seed4) & 255);
      }
    }
    return true;
  }
  return false;
}

static bool init_l8_64_noise(const SharedTexHolder &t)
{
  if (!t)
    return true;
  int the_seed = 0x11231231;
  if (auto lockedTex = lock_texture<uint8_t>(t.getTex2D(), 0, TEXLOCK_WRITE | TEXLOCK_DELSYSMEMCOPY))
  {
    for (int j = 0, pos = 0; j < NOISE_W; ++j)
      for (int i = 0; i < NOISE_W; ++i, ++pos)
        lockedTex.at(i, j) = uint_noise1D(pos, the_seed) & 0xFF;
    return true;
  }
  return false; // we are in reset, so it will be called again anyway
}

static void generate_perlin_3d_texture(int &noiseW, Point3 &maxR, Point3 &minR, SmallTab<uint8_t, TmpmemAlloc> &texData, int &mips,
  uint32_t fmt)
{
  Generates3DNoise noise;
  noise.generateTexture(noiseW, maxR, minR, texData, mips, fmt);
}

static bool generate_perlin_noise_3d(SharedTexHolder &t, Point3 &min_r, Point3 &max_r)
{
  if (t)
    return true;
  SharedTex perlinNoiseTex3d;

  // try to read compressed noise tex from file
  if (d3d::get_texformat_usage(TEXFMT_BC7))
    perlinNoiseTex3d = dag::get_tex_gameres("perlin_voltex_bc7");
  if (!perlinNoiseTex3d)
  {
    perlinNoiseTex3d = dag::get_tex_gameres("perlin_voltex_dxt1");
  }
  if (perlinNoiseTex3d)
  {
    t = SharedTexHolder(eastl::move(perlinNoiseTex3d), "perlin_noise3d");
  }
  else // generate and compress it on runtime
  {
    SmallTab<uint8_t, TmpmemAlloc> texData;
    uint32_t fmt = TEXFMT_DXT1;
    if (!d3d::check_voltexformat(fmt))
      fmt = TEXFMT_DEFAULT;
    int mips, noiseW;
    generate_perlin_3d_texture(noiseW, max_r, min_r, texData, mips, fmt);

    TexPtr perlinNoise3D = dag::create_voltex(noiseW, noiseW, noiseW, fmt, mips, "perlin_noise3d");
    if (!perlinNoise3D)
      return false;
    perlinNoise3D->disableSampler();
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = d3d::AddressMode::Wrap;
      smpInfo.address_mode_v = d3d::AddressMode::Wrap;
      smpInfo.address_mode_w = d3d::AddressMode::Wrap;
      ShaderGlobal::set_sampler(::get_shader_variable_id("perlin_noise3d_samplerstate"), d3d::request_sampler(smpInfo));
    }
    char *data;
    int row_pitch, slice_pitch, src_row_pitch;
    SmallTab<unsigned char, TmpmemAlloc> oneSlice;
    uint8_t *srcData = texData.data();
    int curHt = noiseW / 4;
    if (fmt == TEXFMT_DXT1)
      src_row_pitch = noiseW * 2;
    else
    {
      src_row_pitch = noiseW * 4;
      curHt = noiseW;
    }

    for (int mip = 0, curW = noiseW; mip < mips; ++mip, curW >>= 1, curHt >>= 1)
    {
      if (!perlinNoise3D->lockbox((void **)&data, row_pitch, slice_pitch, mip, TEXLOCK_WRITE | TEXLOCK_DELSYSMEMCOPY))
      {
        return false;
      }
      for (int d = 0; d < curW; ++d, data += slice_pitch - row_pitch * curHt)
        for (int y = 0; y < curHt; ++y, srcData += src_row_pitch, data += row_pitch)
          memcpy(data, srcData, src_row_pitch);
      perlinNoise3D->unlockbox();
    }
    t = SharedTexHolder(eastl::move(perlinNoise3D), "perlin_noise3d");
  }
  return true;
}

static void stridedMemcpy(void *dst, const void *src, size_t size, size_t srcStride, size_t dstStride)
{
  G_ASSERT(dstStride >= srcStride);
  G_ASSERT(size % srcStride == 0);
  if (srcStride == dstStride)
  {
    memcpy(dst, src, size);
  }
  else
  {
    while (size > 0)
    {
      memcpy(dst, src, srcStride);
      dst = (char *)dst + dstStride;
      src = (char *)src + srcStride;
      size -= srcStride;
    }
  }
}

template <typename PixelType>
class NoiseTextureInitializer
{
public:
  template <typename NoiseFunc>
  static bool initialize(const SharedTexHolder &texture, NoiseFunc &&noiseFunc)
  {
    TextureInfo info;
    texture.getTex2D()->getinfo(info);
    return fillMips(texture, generateImageWithNoise(noiseFunc, info.w, info.h));
  }

private:
  template <typename T>
  using UniqueTexImagePtr = eastl::unique_ptr<TexImageAny<T>, void (*)(TexImageAny<T> *)>;

  template <typename T>
  static UniqueTexImagePtr<T> makeUniqueTexImage(int w, int h, std::initializer_list<T> initValues = {})
  {
    UniqueTexImagePtr<T> result(TexImageAny<T>::create(w, h, tmpmem), [](TexImageAny<T> *ptr) { memfree(ptr, tmpmem); });
    eastl::copy(initValues.begin(), initValues.end(), result->getPixels());
    return result;
  }

  template <typename NoiseFunc>
  static UniqueTexImagePtr<PixelType> generateImageWithNoise(NoiseFunc &&noiseFunc, size_t width, size_t height)
  {
    UniqueTexImagePtr<PixelType> result = makeUniqueTexImage<PixelType>(width, height);

    for (size_t y = 0; y < height; y++)
      for (size_t x = 0; x < width; x++)
        (*result)(x, y) = noiseFunc(x, y);

    return result;
  }

  template <typename T, size_t S>
  using Kernel = eastl::array<eastl::array<T, S>, S>;

  template <typename KernelType, typename DivisorType, size_t KernelSize>
  static UniqueTexImagePtr<PixelType> downsampleFiltered(const TexImageAny<PixelType> &image,
    const Kernel<KernelType, KernelSize> &kernel, DivisorType divisor)
  {
    UniqueTexImagePtr<PixelType> result = makeUniqueTexImage<PixelType>(image.w / 2, image.h / 2);

    for (size_t y = 0; y < result->h; y++)
      for (size_t x = 0; x < result->w; x++)
      {
        using AccumulatorType = decltype(eastl::declval<PixelType>() * eastl::declval<KernelType>());
        AccumulatorType accumulator{};

        for (size_t ky = 0; ky < KernelSize; ky++)
          for (size_t kx = 0; kx < KernelSize; kx++)
            accumulator =
              accumulator + image((x * 2 - KernelSize / 2 + kx) % image.w, (y * 2 - KernelSize / 2 + ky) % image.h) * kernel[ky][kx];

        (*result)(x, y) = accumulator / divisor;
      }

    return result;
  }

  static bool fillMips(const SharedTexHolder &texture, UniqueTexImagePtr<PixelType> image)
  {
    for (int level = 0, e = texture.getTex2D()->level_count(); level < e; level++)
    {
      if (auto lockedTex = lock_texture(texture.getTex2D(), level, TEXLOCK_WRITE | (level == e - 1 ? TEXLOCK_DELSYSMEMCOPY : 0)))
        stridedMemcpy(lockedTex.get(), image->getPixels(), image->w * image->h * sizeof(PixelType), image->w * sizeof(PixelType),
          lockedTex.getByteStride());
      else
        return false; // we are in reset, so it will be called again anyway

      static const Kernel<uint8_t, 3> gaussianKernel{{{1, 2, 1}, {2, 4, 2}, {1, 2, 1}}};
      static const int gaussianDivisor = 16;

      image = downsampleFiltered(*image, gaussianKernel, gaussianDivisor);
    }

    return true;
  }
};

static bool init_hash_noise(const SharedTexHolder &t)
{
  if (!t)
    return true;

  TextureInfo info;
  t.getTex2D()->getinfo(info);

  // noise LUT texture by @Dave_Hoskins, www.shadertoy.com/view/4sfGzS
  auto hashNoiseFunc = [width = info.w, height = info.h](size_t x, size_t y) {
    const int seed = 0x11231232;

    return TexPixelRg<uint8_t>{
      uint8_t(uint_noise1D(y * width + x, seed)), uint8_t(uint_noise1D((y - 17) % height * width + (x - 37) % width, seed))};
  };

  return NoiseTextureInitializer<TexPixelRg<uint8_t>>::initialize(t, hashNoiseFunc);
}

static bool init_blue_noise(const SharedTexHolder &t)
{
  if (!t)
    return true;

  TextureInfo info;
  t.getTex2D()->getinfo(info);

  if (auto lockedTex = lock_texture(t.getTex2D(), 0, TEXLOCK_WRITE))
    stridedMemcpy(lockedTex.get(), blue_noise_data::texture_128x128_rg8, BLUE_NOISE_W * BLUE_NOISE_W * 2, BLUE_NOISE_W * 2,
      lockedTex.getByteStride());
  else
    return false; // we are in reset, so it will be called again anyway

  return true;
}

template <typename CB>
static inline const SharedTexHolder &init_and_get_noise(int index, CB cb)
{
  NoiseTexLock lock(&noiseTexLock[index]);
  if (noiseTex[index].getBaseTex())
  {
    noiseTexCounter[index]++;
    return noiseTex[index];
  }
  cb(noiseTex[index]);
  noiseTex[index].setVar();
  G_ASSERT(noiseTexCounter[index] == 0);
  noiseTexCounter[index] = 1;
  return noiseTex[index];
}

const SharedTexHolder &init_and_get_argb8_64_noise()
{
  return init_and_get_noise(NOISE_TEX_ARGB, [](SharedTexHolder &t) {
    if (!VariableMap::isVariablePresent(get_shader_variable_id("noise_64_tex", true)))
      return false;
    t = dag::create_tex(NULL, NOISE_W, NOISE_W, TEXCF_MAYBELOST, 1, "noise_64_tex");
    t->disableSampler();
    ShaderGlobal::set_sampler(get_shader_variable_id("noise_64_tex_samplerstate"), d3d::request_sampler({}));
    return init_argb8_64_noise(t);
  });
}

const SharedTexHolder &init_and_get_l8_64_noise()
{
  return init_and_get_noise(NOISE_TEX_L8, [](SharedTexHolder &t) {
    if (!VariableMap::isVariablePresent(get_shader_variable_id("noise_64_tex_l8", true)))
      return false;
    t = dag::create_tex(NULL, NOISE_W, NOISE_W, TEXCF_MAYBELOST | TEXFMT_L8, 1, "noise_64_tex_l8");
    t->disableSampler();
    ShaderGlobal::set_sampler(get_shader_variable_id("noise_64_tex_l8_samplerstate"), d3d::request_sampler({}));
    return init_l8_64_noise(t);
  });
}

const SharedTexHolder &init_and_get_perlin_noise_3d(Point3 &min_r, Point3 &max_r)
{
  return init_and_get_noise(NOISE_PERLIN, [&](SharedTexHolder &t) {
    if (!VariableMap::isVariablePresent(get_shader_variable_id("perlin_noise3d", true)))
      return false;
    return generate_perlin_noise_3d(t, min_r, max_r);
  });
}

const SharedTexHolder &init_and_get_hash_128_noise()
{
  return init_and_get_noise(NOISE_TEX_HASH, [](SharedTexHolder &t) {
    if (!VariableMap::isVariablePresent(get_shader_variable_id("noise_128_tex_hash", true)))
      return false;
    const size_t mipLevels = 6;
    t = dag::create_tex(NULL, HASH_NOISE_W, HASH_NOISE_W, TEXCF_MAYBELOST | TEXFMT_R8G8, mipLevels, "noise_128_tex_hash");
    ShaderGlobal::set_sampler(get_shader_variable_id("noise_128_tex_hash_samplerstate"), d3d::request_sampler({}));
    t->disableSampler();
    return init_hash_noise(t);
  });
}

const SharedTexHolder &init_and_get_blue_noise()
{
  return init_and_get_noise(NOISE_BLUE_RG8, [](SharedTexHolder &t) {
    if (!VariableMap::isVariablePresent(get_shader_variable_id("blue_noise_tex", true)))
      return false;
    t = dag::create_tex(NULL, BLUE_NOISE_W, BLUE_NOISE_W, TEXCF_MAYBELOST | TEXFMT_R8G8, 1, "blue_noise_tex");
    t->disableSampler();

    return init_blue_noise(t);
  });
}

static void release_noise(int index)
{
  NoiseTexLock lock(&noiseTexLock[index]);
  noiseTexCounter[index]--;
  G_ASSERT(noiseTexCounter[index] >= 0);
  if (noiseTexCounter[index] == 0)
    noiseTex[index].close();
}


void release_hash_128_noise() { release_noise(NOISE_TEX_HASH); }
void release_l8_64_noise() { release_noise(NOISE_TEX_L8); }
void release_64_noise() { release_noise(NOISE_TEX_ARGB); }
void release_perline_noise_3d() { release_noise(NOISE_PERLIN); }
void release_blue_noise() { release_noise(NOISE_BLUE_RG8); }

void term_tex_noises()
{
  interlocked_release_store(atomicForInit, NOT_INITIALIZED);
  for (auto &l : noiseTexLock)
    os_spinlock_destroy(&l);
  for (auto &t : noiseTex)
    t.close();
  for (auto &c : noiseTexCounter)
    c = 0;
}

static void noisetex_after_reset(bool)
{
  init_hash_noise(noiseTex[NOISE_TEX_HASH]);
  init_argb8_64_noise(noiseTex[NOISE_TEX_ARGB]);
  init_l8_64_noise(noiseTex[NOISE_TEX_L8]);

  if (noiseTex[NOISE_PERLIN])
  {
    // we need close this texture because we have check if (t) return true; inside generate_perlin_noise_3d
    noiseTex[NOISE_PERLIN].close();

    Point3 pmin, pmax;
    generate_perlin_noise_3d(noiseTex[NOISE_PERLIN], pmin, pmax);
  }
}

REGISTER_D3D_AFTER_RESET_FUNC(noisetex_after_reset);
