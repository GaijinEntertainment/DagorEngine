#include <3d/dag_drv3d.h>
#include <3d/dag_textureIDHolder.h>
#include <gui/dag_visualLog.h>
#include <math/dag_adjpow2.h>
#include <perfMon/dag_statDrv.h>
#include <render/bcCompressor.h>
#include <render/lightProbe.h>
#include <shaders/dag_shaders.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_convar.h>

#include <render/lightProbeSpecularCubesContainer.h>

LightProbeSpecularCubesContainer::LightProbeSpecularCubesContainer() = default;
LightProbeSpecularCubesContainer::~LightProbeSpecularCubesContainer() = default;

static int bc6h_compression_modeVarId = -1;
static int bc6h_src_mipVarId = -1;
static int bc6h_src_faceVarId = -1;
static int bc6h_src_texVarId = -1;

CONSOLE_BOOL_VAL("render", debug_probes_output, false);

int LightProbeSpecularCubesContainer::allocate()
{
  G_ASSERTF(!usedIndices.all(), "All specular cubes are used.");
  for (int i = 0; i < CAPACITY; ++i)
    if (!usedIndices.test(i))
    {
      usedIndices.set(i);
      return i;
    }
  return -1;
}

void LightProbeSpecularCubesContainer::deallocate(int index)
{
  G_ASSERTF(usedIndices.test(index), "Cube with index %d wasn't allocated.", index);
  usedIndices.reset(index);
  for (int i = updateQueueBegin; i != updateQueueEnd; i = (i + 1) % updateQueue.size())
    if (updateQueue[i]->getProbeId() == index)
    {
      for (int j = i; (j + 1) % updateQueue.size() != updateQueueEnd; j = (j + 1) % updateQueue.size())
        eastl::swap(updateQueue[j], updateQueue[(j + 1) % updateQueue.size()]);
      updateQueueEnd = (updateQueueEnd + updateQueue.size() - 1) % updateQueue.size();
      break;
    }
}

void LightProbeSpecularCubesContainer::init(const int cube_size, uint32_t texture_format)
{
  if (cubeSize)
    return;

  cubeSize = cube_size;
  specularMips = get_log2w(cubeSize);
  texture_format |= TEXCF_RTARGET;

  compressionAvailable = BcCompressor::isAvailable(BcCompressor::COMPRESSION_BC6H);
  if (compressionAvailable)
  {
    sizeInCompressedBlocks = cubeSize / 4;
    compressedMips = get_log2w(sizeInCompressedBlocks) + 1;

    compressor =
      eastl::make_unique<BcCompressor>(BcCompressor::COMPRESSION_BC6H, compressedMips, cubeSize, cubeSize, 1, "bc6h_compressor");
    if (compressor->getCompressionType() == BcCompressor::COMPRESSION_ERR)
    {
      logwarn("Can't create a compressor for light probes specular");
      compressionAvailable = false;
      compressor.reset();
    }
  }

  uint32_t cube_format = compressionAvailable ? TEXFMT_BC6H | TEXCF_UPDATE_DESTINATION : texture_format;

  const char *texName = "light_probe_specular_cubes";
  cubesArray = dag::create_cube_array_tex(cubeSize, CAPACITY, cube_format | TEXCF_CLEAR_ON_CREATE, specularMips, texName);

  rtCube.reset(light_probe::create("light_probe_rt_cube", cubeSize, texture_format));

  if (!compressionAvailable)
    return;

  bc6h_compression_modeVarId = ::get_shader_variable_id("bc6h_compression_mode", true);
  bc6h_src_mipVarId = ::get_shader_variable_id("bc6h_cs_src_mip", true);
  bc6h_src_faceVarId = ::get_shader_variable_id("bc6h_cs_src_face", true);
  bc6h_src_texVarId = ::get_shader_variable_id("bc6h_cs_src_tex", true);
  usedIndices.reset();

  const char *lastMipsFaceTexName = "bc6h_high_quality_compression_target";
  bc6hHighQualityCompressor = eastl::unique_ptr<ComputeShaderElement>(new_compute_shader("bc6h_high_quality_cs", true));
  lastMipsBc6HTarget = dag::create_array_tex(sizeInCompressedBlocks, sizeInCompressedBlocks, 6,
    TEXFMT_A32B32G32R32UI | TEXCF_UNORDERED, compressedMips, lastMipsFaceTexName);

  ShaderGlobal::set_int_fast(bc6h_compression_modeVarId, 0);
}

void LightProbeSpecularCubesContainer::addProbeToUpdate(LightProbe *probe)
{
  for (int i = updateQueueBegin; i != updateQueueEnd; i = (i + 1) % updateQueue.size())
    if (updateQueue[i] == probe)
      return;
  updateQueue[updateQueueEnd] = probe;
  updateQueueEnd = (updateQueueEnd + 1) % updateQueue.size();
}

void LightProbeSpecularCubesContainer::update(const CubeUpdater &cube_updater, const Point3 &origin)
{
  if (updateQueueBegin == updateQueueEnd)
    return;
  enum
  {
    SPECULAR_FACES_COUNT = FACES_COUNT + 6,
    FACES_TO_COMPRESS_COUNT = FACES_COUNT * 3 + 4,
  };
  SCOPE_RENDER_TARGET;
  G_ASSERT(renderedCubeFaces <= FACES_COUNT);
  G_ASSERT(processedSpecularFaces <= SPECULAR_FACES_COUNT);
  G_ASSERT(compressedFaces <= FACES_TO_COMPRESS_COUNT);
  if (renderedCubeFaces == 0)
  {
    float nearestDist = (origin - updateQueue[updateQueueBegin]->getPosition()).lengthSq();
    int nearestProbeIdx = updateQueueBegin;
    for (int i = updateQueueBegin; i != updateQueueEnd; i = (i + 1) % updateQueue.size())
    {
      const float currentDist = (origin - updateQueue[i]->getPosition()).lengthSq();
      if (currentDist < nearestDist)
      {
        nearestDist = currentDist;
        nearestProbeIdx = i;
      }
    }
    eastl::swap(updateQueue[updateQueueBegin], updateQueue[nearestProbeIdx]);
  }

  LightProbe *probe = updateQueue[updateQueueBegin];

  if (renderedCubeFaces == 0)
  {
    const int probesInQueue = (updateQueueEnd + updateQueue.size() - updateQueueBegin) % updateQueue.size();
    if (debug_probes_output.get())
      visuallog::logmsg(String(0, "Probe %d is updating. FrameNo: %d. Active probes: %d. In queue: %d.", probe->getCubeIndex(),
                          dagor_frame_no(), usedIndices.size() - probesInQueue, probesInQueue),
        E3DCOLOR(0, 255, 0), LOGLEVEL_DEBUG);
  }
  if (renderedCubeFaces < FACES_COUNT)
  {
    TIME_D3D_PROFILE(lightProbesCubeUpdate);
    cube_updater(light_probe::getManagedTex(rtCube.get()), probe->getPosition(), renderedCubeFaces);
    renderedCubeFaces++;
    return;
  }
  if (processedSpecularFaces < SPECULAR_FACES_COUNT)
  {
    TIME_D3D_PROFILE(lightProbeSpecular)
    if (processedSpecularFaces < FACES_COUNT)
      updateMips(processedSpecularFaces, 1, 1, 1); // 1st mip, 1 face per frame
    else if (processedSpecularFaces == FACES_COUNT)
      updateMips(0, 3, 2, 1); // 2nd mip, first 3 faces
    else if (processedSpecularFaces == FACES_COUNT + 1)
      updateMips(3, 3, 2, 1); // 2nd mip, last 3 faces
    else if (processedSpecularFaces == FACES_COUNT + 2)
      updateMips(0, 6, 3, 1); // 3rd mip, all 6 faces
    else if (processedSpecularFaces == FACES_COUNT + 3)
      updateMips(0, 6, 4, 1); // 4th mip, all 6 faces
    else if (processedSpecularFaces == FACES_COUNT + 4)
      updateMips(0, 6, 5, 1); // 5th mip, all 6 faces
    else
      updateMips(0, 6, 6, 10); // everything else
    processedSpecularFaces++;
    return;
  }
  if (compressedFaces < FACES_TO_COMPRESS_COUNT)
  {
    TIME_D3D_PROFILE(compressCubeFace)
    if (compressedFaces < FACES_COUNT)
    {
      TIME_D3D_PROFILE(mip0)
      compressMips(probe->getCubeIndex(), compressedFaces % FACES_COUNT, 1, 0, 1);
    }
    else if (compressedFaces < 2 * FACES_COUNT)
    {
      TIME_D3D_PROFILE(mip1)
      compressMips(probe->getCubeIndex(), compressedFaces % FACES_COUNT, 1, 1, 1);
    }
    else if (compressedFaces < 3 * FACES_COUNT)
    {
      TIME_D3D_PROFILE(mip2)
      compressMips(probe->getCubeIndex(), compressedFaces % FACES_COUNT, 1, 2, 1);
    }
    else if (compressedFaces < 3 * FACES_COUNT + 1)
    {
      TIME_D3D_PROFILE(mip3)
      compressMips(probe->getCubeIndex(), 0, 6, 3, 1);
    }
    else if (compressedFaces < 3 * FACES_COUNT + 2)
    {
      TIME_D3D_PROFILE(mip4)
      compressMips(probe->getCubeIndex(), 0, 6, 4, 1);
    }
    else if (compressedFaces < 3 * FACES_COUNT + 3)
    {
      TIME_D3D_PROFILE(mip5)
      compressMips(probe->getCubeIndex(), 0, 6, 5, 1);
    }
    else
    {
      {
        TIME_D3D_PROFILE(mip6)
        compressMips(probe->getCubeIndex(), 0, 6, 6, 1);
      }
      if (!probe->shouldCalcDiffuse())
      {
        onProbeDone(probe);
        return;
      }
    }
    compressedFaces++;
    return;
  }

  G_ASSERT(probe->shouldCalcDiffuse());
  TIME_D3D_PROFILE(lightProbeDiffuse)
  bool valid = calcDiffuse(1, 1, false);
  if (valid)
  {
    probe->setSphHarm(light_probe::getSphHarm(rtCube.get()));
    onProbeDone(probe);
  }
}

void LightProbeSpecularCubesContainer::invalidateProbe(LightProbe *probe)
{
  if (updateQueueBegin == updateQueueEnd)
    return;
  if (probe == updateQueue[updateQueueBegin])
  {
    renderedCubeFaces = 0;
    processedSpecularFaces = 0;
    compressedFaces = 0;
  }
  for (int i = updateQueueBegin; i != updateQueueEnd; i = (i + 1) % updateQueue.size())
    if (updateQueue[i] == probe)
    {
      const uint32_t lastProbeIdx = (updateQueueEnd + updateQueue.size() - 1) % updateQueue.size();
      eastl::swap(updateQueue[i], updateQueue[lastProbeIdx]);
      updateQueueEnd = lastProbeIdx;
      return;
    }
}

void LightProbeSpecularCubesContainer::compressMips(int cube_index, int face_start, int face_count, int from_mip, int mips_count)
{
  ArrayTexture *arrayTex = cubesArray.getArrayTex();
  for (int mip = from_mip; mip < from_mip + mips_count; ++mip)
  {
    if (!compressionAvailable)
    {
      const int cubeMipSide = cubeSize >> min(mip, specularMips - 1);
      for (int faceNumber = face_start; faceNumber < face_start + face_count; ++faceNumber)
      {
        const int destCubeFace = mip + specularMips * (faceNumber + cube_index * 6);
        arrayTex->updateSubRegion(light_probe::getManagedTex(rtCube.get())->getCubeTex(),
          BaseTexture::calcSubResIdx(mip, faceNumber, specularMips), 0, 0, 0, max(1, cubeMipSide), max(1, cubeMipSide), 1,
          destCubeFace, 0, 0, 0);
      }
      continue;
    }

    const int cubeMipSide = sizeInCompressedBlocks >> min(mip, compressedMips - 1);
    if (mip > 2)
    {
      ShaderGlobal::set_real_fast(bc6h_src_mipVarId, mip);
      ShaderGlobal::set_texture(bc6h_src_texVarId, *light_probe::getManagedTex(rtCube.get()));
      d3d::set_rwtex(STAGE_CS, 0, lastMipsBc6HTarget.getArrayTex(), 0, min(mip, compressedMips - 1));
      bc6hHighQualityCompressor->dispatch(cubeMipSide, cubeMipSide, 6);
      d3d::set_rwtex(STAGE_CS, 0, 0, 0, min(mip, compressedMips - 1));
    }
    for (int faceNumber = face_start; faceNumber < face_start + face_count; ++faceNumber)
    {
      const int destCubeFace = mip + specularMips * (faceNumber + cube_index * 6);
      if (mip <= 2)
      {
        compressor->updateFromFaceMip(light_probe::getManagedTex(rtCube.get())->getTexId(), faceNumber, mip,
          min(mip, compressedMips - 1));
        compressor->copyToMip(arrayTex, destCubeFace, 0, 0, min(mip, compressedMips - 1));
      }
      else
      {
        d3d::resource_barrier({lastMipsBc6HTarget.getArrayTex(), RB_RO_COPY_SOURCE,
          (unsigned)(compressedMips * faceNumber + min(mip, compressedMips - 1)), 1});
        arrayTex->updateSubRegion(lastMipsBc6HTarget.getArrayTex(),
          BaseTexture::calcSubResIdx(min(mip, compressedMips - 1), faceNumber, compressedMips), 0, 0, 0, max(1, cubeMipSide),
          max(1, cubeMipSide), 1, destCubeFace, 0, 0, 0);
      }
    }
  }
}

void LightProbeSpecularCubesContainer::onProbeDone(LightProbe *probe)
{
  G_ASSERT(updateQueueBegin != updateQueueEnd);
  probe->markValid();
  updateQueueBegin = (updateQueueBegin + 1) % updateQueue.size();
  renderedCubeFaces = 0;
  processedSpecularFaces = 0;
  compressedFaces = 0;
  const int probesInQueue = (updateQueueEnd + updateQueue.size() - updateQueueBegin) % updateQueue.size();
  if (debug_probes_output.get())
    visuallog::logmsg(String(0, "Probe %d updated. FrameNo: %d. Active probes: %d. In queue: %d.", probe->getCubeIndex(),
                        ::dagor_frame_no(), usedIndices.size() - probesInQueue, probesInQueue),
      E3DCOLOR(0, 180, 180), LOGLEVEL_DEBUG);
}
