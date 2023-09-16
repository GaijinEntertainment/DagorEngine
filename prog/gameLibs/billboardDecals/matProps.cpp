#include <billboardDecals/matProps.h>
#include <ioSys/dag_dataBlock.h>
#include <scene/dag_physMat.h>
#if HAS_PROPS_REGISTRY
#include <propsRegistry/commonPropsRegistry.h>
#endif
#include <memory/dag_framemem.h>
#include <generic/dag_tabUtils.h>


void BillboardDecalsProps::load(const DataBlock *blk)
{
  const DataBlock *bhBlk = blk->getBlockByNameEx("billboardDecal");

#if HAS_PROPS_REGISTRY
  billboardDecalId = propsreg::get_props_id(bhBlk->getStr("type", "default_billboard_decal"), "billboard_decal");
#else
  billboardDecalId = -1;
#endif
  scale = bhBlk->getReal("scale", 1.f);
}

void BillboardDecalTexProps::load(const DataBlock *blk)
{
  diffuseTexName = blk->getStr("diffuseTex", NULL);
  normalTexName = blk->getStr("normalTex", NULL);
  shaderType = blk->getStr("type", NULL);
}

#if HAS_PROPS_REGISTRY
PROPS_REGISTRY_IMPL(BillboardDecalsProps, billboard_decals, "physmat");
PROPS_REGISTRY_IMPL(BillboardDecalTexProps, billboard_decal_tex, "billboard_decal");
#endif

static Tab<const BillboardDecalTexProps *> used_decals;
static Tab<int> decals_remap;

dag::ConstSpan<const BillboardDecalTexProps *> get_used_billboard_decals() { return used_decals; }

void compute_used_billboard_decals()
{
  clear_and_shrink(used_decals);
  clear_and_shrink(decals_remap);
  // TODO: make it from only used materials
  // which requires collecting all unique matIds from frt, ri, landmesh (at least)
  Tab<int> billboardDecalIds(framemem_ptr());
  decals_remap.resize(PhysMat::physMatCount());
  mem_set_ff(decals_remap);
  for (int i = 0; i < PhysMat::physMatCount(); ++i)
  {
    const BillboardDecalsProps *decalProps = BillboardDecalsProps::get_props(i);
    if (!decalProps)
      continue;
    int idx = tabutils::getIndex(billboardDecalIds, decalProps->billboardDecalId);
    if (idx < 0)
    {
      idx = billboardDecalIds.size();
      billboardDecalIds.push_back(decalProps->billboardDecalId);
    }

    decals_remap[i] = idx;
  }

  for (int i = 0; i < billboardDecalIds.size(); ++i)
  {
    const BillboardDecalTexProps *texProps = BillboardDecalTexProps::get_props(billboardDecalIds[i]);
    G_ASSERT(texProps);
    used_decals.push_back(texProps);
  }
}

int get_remaped_decal_tex(PhysMat::MatID mat_id) { return decals_remap[mat_id]; }

void load_billboard_decals(const DataBlock *blk)
{
#if HAS_PROPS_REGISTRY
  const int bdecalClassId = propsreg::get_prop_class("billboard_decal");
  for (int i = 0; i < blk->blockCount(); ++i)
  {
    const DataBlock *decalBlk = blk->getBlock(i);
    propsreg::register_props(decalBlk->getBlockName(), decalBlk, bdecalClassId);
  }
#else
  G_UNUSED(blk);
#endif
}
