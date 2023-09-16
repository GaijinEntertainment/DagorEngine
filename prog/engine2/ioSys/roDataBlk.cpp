#include <ioSys/dag_roDataBlock.h>
#include <ioSys/dag_genIo.h>

RoDataBlock RoDataBlock::emptyBlock;

RoDataBlock *RoDataBlock::load(IGenLoad &crd, int sz)
{
  if (sz == -1)
    crd.readInt(sz);

  void *mem = memalloc(sz, tmpmem);
  RoDataBlock *blk = new (mem, _NEW_INPLACE) RoDataBlock;

  crd.read(blk, sz);
  blk->patchData(mem);
  blk->patchNameMap(mem);
  return blk;
}

void RoDataBlock::patchData(void *base)
{
  nameMap.patch(base);
  blocks.patch(base);
  params.patch(base);
  for (int i = 0; i < blocks.size(); ++i)
    blocks[i].patchData(base);
}

int RoDataBlock::getNameId(const char *name) const { return nameMap ? nameMap->getNameId(name) : -1; }
const char *RoDataBlock::getName(int name_id) const { return nameMap ? nameMap->map[name_id].get() : NULL; }

RoDataBlock *RoDataBlock::getBlockByName(int nid, int after)
{
  for (int i = after + 1; i < blocks.size(); ++i)
    if (blocks[i].nameId == nid)
      return &blocks[i];
  return NULL;
}

int RoDataBlock::findParam(int nid, int after) const
{
  for (int i = after + 1; i < params.size(); ++i)
    if (params[i].nameId == nid)
      return i;
  return -1;
}
