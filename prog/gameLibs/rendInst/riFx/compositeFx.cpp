#include <rendInst/rendInstFx.h>
#include <rendInst/rendInstFxDetail.h>
#include <rendInst/riexHashMap.h>
#include <generic/dag_tab.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_TMatrix.h>
#include <math/random/dag_random.h>
#include <ska_hash_map/flat_hash_map2.hpp>

class FxNode
{
public:
  TMatrix tm = TMatrix::IDENT;
  struct FxCase
  {
    int type;
    float weight;
  };

  void pushCase(int fx_type, float weight);

  int getFxType(int seed) const;
  bool isValid() const { return fxCases.size() > 0; }

private:
  float sumWeight = 0.f;

  unsigned short int casesOffset = 0;
  unsigned short int casesCount = 0;
  static Tab<FxCase> fxCases;
};

struct FxTemplate
{
  unsigned short int nodeOffset;
  unsigned short int nodeCount;
};

Tab<FxNode::FxCase> FxNode::fxCases(midmem);
static Tab<FxNode> fxNodes;
static Tab<FxTemplate> fxTemplates;

int pushCompositeFxNode(const FxNode &fx_node, bool new_template)
{
  if (new_template)
  {
    FxTemplate fxTemplate{static_cast<unsigned short>(fxNodes.size()), static_cast<unsigned short>(1)};
    fxNodes.push_back(fx_node);
    fxTemplates.push_back(fxTemplate);
  }
  else
  {
    ++fxTemplates.back().nodeCount;
  }
  return fxTemplates.size() - 1;
}

dag::Span<FxNode> getCompositeFxNodes(int template_id)
{
  G_ASSERT(template_id >= 0 && template_id < fxTemplates.size());
  const FxTemplate &fxTemplate = fxTemplates[template_id];
  return make_span(fxNodes).subspan(fxTemplate.nodeOffset, fxTemplate.nodeCount);
}

void FxNode::pushCase(int template_id, float weight)
{
  fxCases.push_back(FxCase{template_id, weight});
  if (casesCount == 0)
  {
    casesOffset = fxCases.size() - 1;
  }
  ++casesCount;
  sumWeight += weight;
}

int FxNode::getFxType(int seed) const
{
  if (fxCases.size() == 1)
    return fxCases.front().type;

  const float arbitraryBigWeight = 8192.0f;
  float w = floorf(_frnd(seed) * sumWeight * arbitraryBigWeight + 0.5f) / arbitraryBigWeight;
  for (const FxCase fx : fxCases)
  {
    w -= fx.weight;
    if (w <= 0)
    {
      return fx.type;
    }
  }
  return fxCases.front().type;
}

int rifx::composite::loadCompositeEntityTemplate(const DataBlock *composite_fx_blk)
{
  int templateId = -1;
  int nodeId = composite_fx_blk->getNameId("fxNode");
  bool isNewFxType = true;
  for (int nIdx = 0; nIdx < composite_fx_blk->blockCount(); ++nIdx)
  {
    const DataBlock *fxNodeBlk = composite_fx_blk->getBlock(nIdx);
    if (fxNodeBlk->getBlockNameId() == nodeId)
    {
      FxNode fxNode;
      fxNode.tm = fxNodeBlk->getTm("tm", TMatrix::IDENT);

      int entIdx = fxNodeBlk->getNameId("ent");
      for (int eIdx = 0; eIdx < fxNodeBlk->blockCount(); ++eIdx)
      {
        const DataBlock *entBlk = fxNodeBlk->getBlock(eIdx);
        if (entBlk->getBlockNameId() == entIdx)
        {
          const char *fxName = entBlk->getStr("name", nullptr);
          int fxType = fxName ? rifx::composite::detail::getTypeByName(fxName) : -1;

          float weight = entBlk->getReal("weight", 1.f);
          G_ASSERTF(weight > 0, "bad weight for fx from rendinst dmg: %s", fxName);

          fxNode.pushCase(fxType, weight);
        }
      }
      if (fxNode.isValid())
      {
        templateId = pushCompositeFxNode(fxNode, isNewFxType);
        isNewFxType = false;
      }
    }
  }
  return templateId;
}

static RiexHashMap<Tab<AcesEffect *>> riHandleToFxDesc;

void rifx::composite::spawnEntitiy(rendinst::riex_handle_t ri_idx, unsigned short int template_id, const TMatrix &tm, bool restorable)
{
  for (const FxNode &fxNode : getCompositeFxNodes(template_id))
  {
    TMatrix fxTm = tm * fxNode.tm;
    int seed = grnd();
    int fxType = fxNode.getFxType(seed);

    AcesEffect *fxHandle = nullptr;
    if (fxType >= 0)
      rifx::composite::detail::startEffect(fxType, fxTm, restorable ? &fxHandle : nullptr);

    if (restorable)
    {
      riHandleToFxDesc[ri_idx].push_back(fxHandle);
    }
  }
}

void rifx::composite::setEntityApproved(rendinst::riex_handle_t ri_idx, bool keep_on_playing)
{
  auto fxDesc = riHandleToFxDesc.find(ri_idx);
  if (fxDesc == riHandleToFxDesc.end())
  {
    return;
  }

  if (!keep_on_playing)
  {
    for (AcesEffect *fx : fxDesc->second)
    {
      rifx::composite::detail::stopEffect(fx);
    }
  }
  riHandleToFxDesc.erase(fxDesc);
}
