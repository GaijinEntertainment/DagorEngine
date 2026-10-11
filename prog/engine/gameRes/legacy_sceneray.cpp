// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_legacySceneRay.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_zstdIo.h>
#include <scene/dag_physMat.h>
#include <util/dag_roNameMap.h>
#include <memory/dag_framemem.h>
#include <debug/dag_debug.h>

enum
{
  LEGACY_VERSION = 42,
  CURRENT_VERSION = 43,
};

enum
{
  ZlibCompression = 6,
  NoCompression = 7,
  ZstdCompression = 0,
};

template <typename FI>
StaticSceneRayTracerT<FI> *StaticSceneRayTracerT<FI>::load(IGenLoad &cb)
{
  unsigned block_rest = cb.readInt();

  uint16_t ver = 0;
  uint32_t n = 0;
  char sign[6];

  cb.read(sign, 6);
  cb.read(&ver, 2);
  const int version = ver >> 3;
  const int compression = ver & 7;
  if (memcmp(sign, "RTdump", 6) != 0 || (version != LEGACY_VERSION && version != CURRENT_VERSION) ||
      (compression != NoCompression && compression != ZlibCompression && compression != ZstdCompression))
  {
    DEBUG_CTX("bad signature or version mismatch (sign=%.6s version=%d compression %d ver=%0x)", sign, version, compression, ver);
    return nullptr;
  }
  cb.read(&n, 4);
  auto loadedDump = (char *)midmem->alloc(n);
  if (compression == NoCompression)
    cb.read(loadedDump, n);
  else if (compression == ZlibCompression)
  {
    ZlibLoadCB zlib_crd(cb, block_rest - 12);
    zlib_crd.read(loadedDump, n);
  }
  else
  {
    ZstdLoadCB zstd_crd(cb, block_rest - 12);
    zstd_crd.read(loadedDump, n);
  }

  // the two versions differ in the grid alone, which nothing reads
  auto ldump = (Dump *)loadedDump;
  ldump->vertsPtr.patch(ldump);
  ldump->facesPtr.patch(ldump);
  ldump->faceIndicesPtr.patch(ldump);
  return (StaticSceneRayTracerT *)loadedDump;
}

StaticSceneRayTracer *load_frt_block(IGenLoad &crd, Tab<uint8_t> &out_face_pmid)
{
  crd.readInt(); // the block's version word; a foreign one fails the tracer's own signature below
  const int start_pos = crd.tell();
  StaticSceneRayTracer *frt = StaticSceneRayTracer::load(crd);
  if (!frt)
    return nullptr;

  clear_and_resize(out_face_pmid, frt->getFacesCount());
  crd.readTabData(out_face_pmid);

  const int read = crd.tell() - start_pos;
  if (read & 3)
    crd.seekrel(4 - (read & 3));

  Tab<uint8_t> mem(framemem_ptr());
  crd.readTab(mem);
  RoNameMap &nm = *(RoNameMap *)mem.data();
  nm.patchData(mem.data());

  Tab<uint8_t> matid(framemem_ptr());
  matid.resize(nm.map.size());
  for (int i = 0; i < nm.map.size(); ++i)
  {
    const int id = PhysMat::getMaterialId(nm.map[i]);
    G_ASSERT(id >= 0 && id <= 255);
    matid[i] = id;
  }

  for (int i = 0; i < out_face_pmid.size(); ++i)
  {
    G_ASSERT(out_face_pmid[i] < nm.map.size());
    out_face_pmid[i] = matid[out_face_pmid[i]];
  }
  return frt;
}

template class StaticSceneRayTracerT<SceneRayI24F8>;
template class StaticSceneRayTracerT<uint16_t>;
