#include <ioSys/dag_genIo.h>
#include <math/dag_math3d.h>
#include <math/dag_mathUtils.h>
#include <generic/dag_qsort.h>
#include <util/dag_bitArray.h>
#include <util/dag_string.h>

#include <math/integer/dag_IBBox3.h>
#include <sceneRay/dag_sceneRay.h>
#include <ioSys/dag_zlibIo.h>
#include <osApiWrappers/dag_sharedMem.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>

static IMemAlloc *memPool = NULL;
static GlobalSharedMemStorage *sharedMem = NULL;
static inline IMemAlloc &frtMem() { return memPool ? *memPool : *midmem; }

static char *alloc_frt_dump(int sz, const char *dump_name)
{
  GlobalSharedMemStorage *sm = sharedMem;
  char *p = NULL;
  if (sm)
  {
    p = (char *)sm->allocPtr(dump_name, _MAKE4C('FRT'), sz);
    if (p)
      logmessage(_MAKE4C('SHMM'), "allocated FRT dump in shared mem: %p, %dK (mem %uK/%uK, rec=%d)", p, sz >> 10,
        unsigned(((uint64_t)sm->getMemUsed()) >> 10), unsigned(((uint64_t)sm->getMemSize()) >> 10), sm->getRecUsed());
    else
      logmessage(_MAKE4C('SHMM'),
        "failed to allocate FRT dump in shared mem: %p, %dK (mem %uK/%uK, rec=%d); "
        "falling back to conventional allocator",
        p, sz >> 10, unsigned(((uint64_t)sm->getMemUsed()) >> 10), unsigned(((uint64_t)sm->getMemSize()) >> 10), sm->getRecUsed());
  }
  return p ? p : (char *)frtMem().alloc(sz);
}

// лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл//
template <typename FI>
DeserializedStaticSceneRayTracerT<FI>::DeserializedStaticSceneRayTracerT() : StaticSceneRayTracerT<FI>(), loadedDump(NULL)
{}

template <typename FI>
DeserializedStaticSceneRayTracerT<FI>::~DeserializedStaticSceneRayTracerT()
{
  if (!sharedMem || !sharedMem->doesPtrBelong(loadedDump))
    frtMem().free(loadedDump);
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
  if (!sharedMem || !sharedMem->doesPtrBelong(loadedDump))
    frtMem().free(loadedDump);
  loadedDump = NULL;

  short ver = 0;
  int n = 0;
  char sign[6];

  if (sharedMem)
  {
    loadedDump = (char *)sharedMem->findPtr(cb.getTargetName(), _MAKE4C('FRT'));
    if (loadedDump)
    {
      n = (int)sharedMem->getPtrSize(loadedDump);
      logmessage(_MAKE4C('SHMM'), "reusing FRT dump from shared mem: : %p, %dK", loadedDump, n >> 10);

      dump = *(Dump *)loadedDump;
      goto done;
    }
  }
  cb.read(sign, 6);
  cb.read(&ver, 2);

  if (memcmp(sign, "RTdump", 6) != 0 || (ver != 0x155 && ver != 0x156 && ver != 0x157))
  {
    debug_ctx("bad signature or version mismatch (sign=%.6s ver=%X)", sign, ver);
    return false;
  }
  // curmem = midmem;
  cb.read(&n, 4);
  if (n >= 0 && (ver == 0x155 || ver == 0x157))
  {
    loadedDump = (char *)frtMem().alloc(n); // we do not support shared memory for old format
    cb.read(loadedDump, n);
  }
  else
  {
    if (ver == 0x155)
      n = -n;
    loadedDump = ver == 0x155 ? (char *)frtMem().alloc(n) : alloc_frt_dump(n, cb.getTargetName()); // we do not support shared memory
                                                                                                   // for old format
    ZlibLoadCB zlib_crd(cb, block_rest - 12);
    zlib_crd.read(loadedDump, n);
  }
  createInMemory(loadedDump);
  if (ver == 0x155)
  {
    Point3 *vertsLegacyPtr = (Point3 *)&verts(0);
    vertsVecLegacy.resize(getVertsCount());
    for (int i = 0; i < getVertsCount(); ++i)
    {
      vertsVecLegacy[i] = vertsLegacyPtr[i];
      vertsVecLegacy[i].resv = 0;
    }
    dump.vertsPtr = vertsVecLegacy.data();
  }

done:
  v_rtBBox = v_ldu_bbox3(getBox());

  if (sharedMem && sharedMem->doesPtrBelong(loadedDump))
  {
    mark_global_shared_mem_readonly(loadedDump, n, true);
    sharedMem->markPtrDataReady(loadedDump);
  }

  const int vsize = dump.vertsCount * (ver == 0x155 ? sizeof(Point3) : sizeof(Point3_vec4));
  const int fsize = dump.facesCount * sizeof(RTface);
  const int ffsize = ver == 0x155 ? dump.facesCount * sizeof(Point3) : 0;
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
