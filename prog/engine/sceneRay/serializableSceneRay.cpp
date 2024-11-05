// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_genIo.h>
#include <math/dag_math3d.h>
#include <generic/dag_qsort.h>
#include <util/dag_bitArray.h>
#include <util/dag_string.h>

#include <math/integer/dag_IBBox3.h>
#include <sceneRay/dag_sceneRay.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_ioUtils.h>
#include "version.h"
#include <debug/dag_log.h>

template <typename FI>
void StaticSceneRayTracerT<FI>::Node::serialize(IGenSave &cb, const void *dump_base) const
{
  G_STATIC_ASSERT(sizeof(Node) == sizeof(bsc) + sizeof(bsr2) + 16);
  G_ASSERTF((cb.tell() & 0xF) == 8, "cb.tell()=0x%x", cb.tell());
  cb.write(&bsc, sizeof(bsc));
  cb.write(&bsr2, sizeof(bsr2));
  if (isNode())
  {
    int sub1Tell = cb.tell();
    int end = cb.tell() + 8 * 2;
    cb.writeInt(0);
    cb.writeInt(0); // sub0, right
    cb.writeInt(end);
    cb.writeInt(0); // sub1, left
    getRight()->serialize(cb, dump_base);

    int sub1Addr = cb.tell();
    cb.seekto(sub1Tell);
    cb.writeInt(sub1Addr);
    cb.seektoend();
    getLeft()->serialize(cb, dump_base);
  }
  else
  {
    cb.writeInt(int(((char *)sub0.get()) - (char *)dump_base));
    cb.writeInt(0);
    cb.writeInt(int(((char *)sub1.get()) - (char *)dump_base));
    cb.writeInt(0);
  }
}

///============Serialization=================================

template <typename FI>
bool StaticSceneRayTracerT<FI>::serialize(IGenSave &cb, bool be_target, int *out_dump_sz, bool zcompr)
{
  G_ASSERT(!be_target);
  const uint16_t ver = get_full_version(zcompr ? ZstdCompression : NoCompression);
  const uint32_t pos = cb.tell();
  cb.writeInt(0);
  cb.write((void *)"RTdump", 6);
  cb.write(&ver, 2);

  dump.vertsPtr.clearUpperBits();
  dump.facesPtr.clearUpperBits();
  // dump.faceboundsPtr.clearUpperBits();
  dump.faceIndicesPtr.clearUpperBits();
  dump.grid.clearUpperBits();

  // store numbers
  size_t dumpLen = sizeof(Dump);
  Dump serializeDump = dump;
  serializeDump.vertsCount = getVertsCount();
  serializeDump.facesCount = getFacesCount();

  serializeDump.vertsPtr = (Point3_vec4 *)(dumpLen);
  dumpLen += getVertsCount() * sizeof(Point3_vec4);

  serializeDump.facesPtr = (RTface *)(dumpLen);
  dumpLen += getFacesCount() * sizeof(RTface);

  // serializeDump.obsolete_faceboundsPtr = (RTfaceBoundFlags*)(dumpLen);
  // dumpLen += getFacesCount()*sizeof(RTfaceBoundFlags);
  serializeDump.obsolete_faceboundsPtr = NULL;

  const void *dump_base = ((char *)dump.faceIndicesPtr.get()) - dumpLen;
  serializeDump.faceIndicesPtr = (FaceIndex *)(dumpLen);
  dumpLen += getFaceIndicesCount() * sizeof(FaceIndex);

  MemorySaveCB gridSave(16 << 20);
  gridSave.setMcdMinMax(2 << 20, 8 << 20);

  size_t pre_grid_padding = 0;
  while ((dumpLen & 15) != 8) // align grid on sizeof(vec4f)+8 to force grid::Node alignment to sizeof(vec4f)
    dumpLen++, pre_grid_padding++;
  serializeDump.grid = (RTHierGrid3<Leaf, 0> *)(dumpLen);
  dump.grid->serialize(gridSave, dump_base);

  dumpLen += gridSave.getSize();

  IGenSave *data_cwr = &cb;
  ZstdSaveCB *zlib_cwr = NULL;

  G_ASSERT(dumpLen < 0x7FFFFFFF);
  int i_dumpLen = (int)dumpLen;
  if (out_dump_sz)
    *out_dump_sz = i_dumpLen;

  cb.writeInt(i_dumpLen);
  if (zcompr)
    data_cwr = zlib_cwr = new ZstdSaveCB(cb, ZSTD_DEFAULT_COMPRESSION_LEVEL);
  data_cwr->write(&serializeDump, sizeof(serializeDump));
  data_cwr->write(&verts(0), getVertsCount() * sizeof(Point3_vec4));
  data_cwr->write(&faces(0), getFacesCount() * sizeof(RTface));
  data_cwr->write(&faceIndices(0), getFaceIndicesCount() * sizeof(FaceIndex));

  write_zeros(*data_cwr, pre_grid_padding);
  gridSave.copyDataTo(*data_cwr);
  G_STATIC_ASSERT((sizeof(Dump) & 15) == 0);

  if (zlib_cwr)
  {
    zlib_cwr->finish();
    delete zlib_cwr;
  }

  // store end mark
  int new_pos = cb.tell();
  int sz = new_pos - pos - 4;
  cb.seekto(pos);
  cb.writeInt(sz);
  cb.seekto(new_pos);

  return true;
}
