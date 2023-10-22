#include <ioSys/dag_genIo.h>
#include <math/dag_math3d.h>
#include <math/dag_mathUtils.h>
#include <generic/dag_qsort.h>
#include <util/dag_bitArray.h>
#include <util/dag_string.h>

#include <math/integer/dag_IBBox3.h>
#include <sceneRay/dag_sceneRay.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_zstdIo.h>
#include "version.h"
#include <debug/dag_debug.h>
#include <debug/dag_log.h>

// лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл//
template <typename FI>
DeserializedStaticSceneRayTracerT<FI>::DeserializedStaticSceneRayTracerT() : StaticSceneRayTracerT<FI>(), loadedDump(NULL)
{}

template <typename FI>
DeserializedStaticSceneRayTracerT<FI>::~DeserializedStaticSceneRayTracerT()
{
  memfree_anywhere(loadedDump);
  loadedDump = NULL;
}


///===================deserialize====================
template <typename FI>
bool DeserializedStaticSceneRayTracerT<FI>::serializedLoad(IGenLoad &cb)
{
  unsigned block_rest = cb.readInt();
  bool ret = _serializedLoad(cb, block_rest);
  return ret;
}

template <typename FI>
void DeserializedStaticSceneRayTracerT<FI>::createInMemory(char *data)
{
  ((Dump *)data)->patch();
  dump = *(Dump *)data;
}


template <typename FI>
bool DeserializedStaticSceneRayTracerT<FI>::_serializedLoad(IGenLoad &cb, unsigned block_rest)
{
  memfree_anywhere(loadedDump);
  loadedDump = NULL;

  uint16_t ver = 0;
  uint32_t n = 0;
  char sign[6];

  cb.read(sign, 6);
  cb.read(&ver, 2);
  const int version = ver >> 3;
  FrtCompression compression = FrtCompression(ver & 7);
  if (memcmp(sign, "RTdump", 6) != 0 ||
      ((version != LEGACY_VERSION && version != CURRENT_VERSION) || !is_supported_compression(compression)))
  {
    debug_ctx("bad signature or version mismatch (sign=%.6s version=%d compression %d ver=%0x)", sign, version, (int)compression, ver);
    return false;
  }
  // curmem = midmem;
  cb.read(&n, 4);
  loadedDump = (char *)midmem->alloc(n); // we do not support shared memory for old format
  if (compression == NoCompression)
  {
    cb.read(loadedDump, n);
  }
  else
  {
    switch ((int)compression)
    {
      case ZlibCompression:
      {
        ZlibLoadCB zlib_crd(cb, block_rest - 12);
        zlib_crd.read(loadedDump, n);
        break;
      }
      case ZstdCompression:
      {
        ZstdLoadCB zstd_crd(cb, block_rest - 12);
        zstd_crd.read(loadedDump, n);
        break;
      }
      default: G_ASSERT(0);
    }
  }
  ((Dump *)loadedDump)->version = version;
  createInMemory(loadedDump);
  if (version == LEGACY_VERSION)
    StaticSceneRayTracerT<FI>::rearrangeLegacyDump(loadedDump, n);

  v_rtBBox = v_ldu_bbox3(getBox());

  const int vsize = dump.vertsCount * sizeof(Point3_vec4);
  const int fsize = dump.facesCount * sizeof(RTface);
  const int ffsize = 0;
  const int fisize = dump.faceIndicesCount * sizeof(FaceIndex);
  const int gsize = n - (vsize + fsize + ffsize + fisize + sizeof(Dump));
  const float ft = n ? (float)n : 1.f;
  debug("ray tracer data loaded, total size = %dK, vertex cnt = %d, size = %dK (%.1f%%),\n"
        "faces cnt = %d, size %dK (%.1f%%), faceBounds flags size %dK (%.1f%%),\n"
        "faceIndices cnt = %d, size %dK (%.1f%%), grid size = %dK (%.1f%%) ",
    n >> 10, dump.vertsCount, vsize >> 10, (float)(vsize / ft) * 100.f, dump.facesCount, fsize >> 10, (float)(fsize / ft) * 100.f,
    ffsize >> 10, (float)(ffsize / ft) * 100.f, dump.faceIndicesCount, fisize >> 10, (float)(fisize / ft) * 100.f, gsize >> 10,
    (float)(gsize / ft) * 100.f);

  /*for (int i = 0; i < 1; ++i)
  {
    int64_t reft = ref_time_ticks();
    BuildableStaticSceneRayTracer *brt = create_buildable_staticmeshscene_raytracer ( dump.leafSize, 8 );
    brt->addmesh((uint8_t*)&dump.vertsPtr[0], sizeof(dump.vertsPtr[0]), dump.vertsCount,
                 (unsigned*)&dump.facesPtr[0].v[0], sizeof(RTface), dump.facesCount, NULL, true);
    debug("cooking time %dusec", get_time_usec(reft));
    del_it(brt);
  }*/

  return true;
}

DeserializedStaticSceneRayTracer *create_staticmeshscene_raytracer(IGenLoad &cb)
{
  DeserializedStaticSceneRayTracer *rt = new DeserializedStaticSceneRayTracer();

  if (!rt->serializedLoad(cb))
  {
    delete rt;
    rt = NULL;
  }
  return rt;
}

template class DeserializedStaticSceneRayTracerT<SceneRayI24F8>;
template class DeserializedStaticSceneRayTracerT<uint16_t>;
