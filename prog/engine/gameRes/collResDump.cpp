// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_collResDump.h>
#include <gameRes/collResStream.h>
#include <gameRes/collisionResourceBuilder.h>
#include <ioSys/dag_baseIo.h>
#include <memory/dag_mem.h>
#include <debug/dag_log.h>
#include "collisionGameResInternal.h"
#include <string.h>

bool CollisionResourceBuilder::readRaw(CollisionResource &res, IGenLoad &crd)
{
  using Data = CollisionResource::Data;
  CollResRawHead h;
  crd.read(&h, sizeof(h));
  if (h.blockBytes < sizeof(Data) || h.blockBytes > MAX_COLLRES_BLOCK_BYTES)
    return false;
  Data *d = (Data *)midmem->allocAligned(h.blockBytes, Data::BLOCK_ALIGN);
  crd.read(d, h.blockBytes);
  d->refCnt = 0;
  res.data = d;
  if (d->blockBytes() != h.blockBytes)
  {
    res.data = Data::build(nullptr);
    return false;
  }
  res.vFullBBox.bmin = v_perm_xyzd(v_ldu(&h.boundingBox[0].x), v_zero());
  res.vFullBBox.bmax = v_perm_xyzd(v_ldu(&h.boundingBox[1].x), v_zero());
  res.vBoundingSphere = h.boundingSphere;
  res.vBindTraceSphere = h.bindTraceSphere;
  res.boundingBox = h.boundingBox;
  res.setBoundingSphereRad(h.boundingSphereRad);
  res.collisionFlags = h.collisionFlags;
  res.bsphereCenterNode = h.bsphereCenterNode;
  CollisionResourceInstance &inst = res.defaultInstance;
  inst.poseMeta.resize(d->allNodesList().size());
  inst.nodeTm.resize(d->allNodesList().size());
  crd.read(inst.poseMeta.data(), data_size(inst.poseMeta));
  crd.read(inst.nodeTm.data(), data_size(inst.nodeTm));
  inst.recomputeRootBBox();
  inst.posedSinceBind = h.posedSinceBind;
  if (inst.posedSinceBind && res.hasAllNodesTLAS()) // as deepCopy does
    inst.refitAllTlasLeaves();
  return true;
}

namespace
{
// read() past the end gives zeros and sets overran instead of throwing: an interrupted ri.dump_collres ends inside
// a record, and a build without exceptions would fatal on it.
struct BoundedLoad final : IBaseLoad
{
  IGenLoad &in;
  const int64_t total;
  bool overran = false;
  explicit BoundedLoad(IGenLoad &cb) : in(cb), total(cb.getTargetDataSize()) {}
  void read(void *ptr, int size) override
  {
    if (overran || (total >= 0 && size > total - in.tell()))
    {
      overran = true;
      memset(ptr, 0, size);
    }
    else
      in.read(ptr, size);
  }
  int tryRead(void *ptr, int size) override { return in.tryRead(ptr, size); }
  int tell() override { return in.tell(); }
  void seekto(int ofs) override { in.seekto(ofs); }
  void seekrel(int ofs) override { in.seekrel(ofs); }
  const char *getTargetName() override { return in.getTargetName(); }
  int64_t getTargetDataSize() override { return total; }
};
} // namespace

bool load_collres_dump(IGenLoad &in, CollResDump &out)
{
  BoundedLoad cb(in);
  out = {};
  bool ok = (uint32_t)cb.readInt() == COLLRES_DUMP_MAGIC && cb.readInt() == COLLRES_DUMP_VERSION;
  out.materials.resize(ok ? cb.readInt() : 0);
  for (CollResDump::Material &m : out.materials)
  {
    cb.readString(m.name);
    cb.readString(m.soundMaterial);
    cb.readString(m.bfClass);
    cb.read(&m.imp_absorb_k, collres_mat_pod_bytes(m));
    m.id = int(&m - out.materials.data());
    m.bfid = -1;
  }
  while (ok && !cb.overran && cb.tell() < cb.getTargetDataSize())
  {
    CollResDump::Pool &p = out.pools.push_back();
    cb.readString(p.name);
    cb.readTab(p.instances);
    p.collRes = new CollisionResource;
    ok = CollisionResourceBuilder::readRaw(*p.collRes, cb);
  }
  ok = ok && !cb.overran;
  if (!ok)
  {
    logerr("collres dump: a torn dump, or not one of this engine build");
    out = {};
  }
  return ok;
}
