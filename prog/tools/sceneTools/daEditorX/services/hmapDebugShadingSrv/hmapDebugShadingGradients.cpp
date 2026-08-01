// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_hmapDebugShadingGradients.h>

#include <libTools/util/strUtil.h>

#include <ioSys/dag_dataBlock.h>
#include <math/dag_color.h>
#include <math/dag_Point4.h>

#include <EASTL/sort.h>

void HeightmapColorGradient::load(const DataBlock &blk, int anchor_name_id)
{
  const DataBlock *gradientMappingBlk = blk.getBlockByName("gradient_mapping");
  if (!gradientMappingBlk)
    return;

  const int paramCount = gradientMappingBlk->paramCount();
  for (int i = 0; i < paramCount; ++i)
  {
    if (gradientMappingBlk->getParamNameId(i) != anchor_name_id || gradientMappingBlk->getParamType(i) != DataBlock::TYPE_POINT4)
      continue;

    const Point4 point4 = gradientMappingBlk->getPoint4(i);

    HeightmapColorGradient::Key &key = keys.push_back();
    key.color = e3dcolor(Color4(point4.x, point4.y, point4.z));
    key.height = point4.w;
  }

  sortByHeight();
}

void HeightmapColorGradient::save(DataBlock &blk) const
{
  DataBlock *gradientMappingBlk = blk.addBlock("gradient_mapping");
  for (const HeightmapColorGradient::Key &key : keys)
  {
    const Color4 color = color4(key.color);
    gradientMappingBlk->addPoint4("anchor", Point4(color.r, color.g, color.b, key.height));
  }
}

void HeightmapColorGradient::sortByHeight()
{
  eastl::stable_sort(keys.begin(), keys.end(),
    [](const HeightmapColorGradient::Key &a, const HeightmapColorGradient::Key &b) { return a.height < b.height; });
}

void HeightmapColorGradients::load(const DataBlock &blk)
{
  gradients.clear();

  const DataBlock *gradientsBlk = blk.getBlockByName("gradients");
  if (!gradientsBlk)
    return;

  const int anchorNameId = blk.getNameId("anchor");
  if (anchorNameId < 0)
    return;

  const int blockCount = gradientsBlk->blockCount();
  for (int i = 0; i < blockCount; ++i)
  {
    const DataBlock *gradientBlk = gradientsBlk->getBlock(i);

    const char *gradientName = gradientBlk->getBlockName();
    if (gradientName == nullptr || *gradientName == 0)
    {
      logerr("Heightmap color gradient has empty name. It will not be loaded.");
      continue;
    }

    HeightmapColorGradient &gradient = gradients.push_back();
    gradient.name = gradientName;
    gradient.load(*gradientBlk, anchorNameId);
  }

  sortByName();
}

void HeightmapColorGradients::save(DataBlock &blk) const
{
  DataBlock *gradientsBlk = blk.addBlock("gradients");

  for (const HeightmapColorGradient &gradient : gradients)
  {
    if (gradient.name.empty())
      logerr("Heightmap color gradient has empty name. It will not be saved.");
    else
      gradient.save(*gradientsBlk->addBlock(gradient.name));
  }
}

void HeightmapColorGradients::sortByName()
{
  eastl::stable_sort(gradients.begin(), gradients.end(),
    [](const HeightmapColorGradient &a, const HeightmapColorGradient &b) { return stricmp(a.name, b.name) < 0; });
}

int HeightmapColorGradients::getIndexByName(const char *name) const
{
  for (int i = 0; i < gradients.size(); ++i)
    if (gradients[i].name == name)
      return i;

  return -1;
}

HeightmapColorGradient *HeightmapColorGradients::getByName(const char *name)
{
  const int index = getIndexByName(name);
  return index >= 0 ? &gradients[index] : nullptr;
}

const HeightmapColorGradient *HeightmapColorGradients::getByName(const char *name) const
{
  const int index = getIndexByName(name);
  return index >= 0 ? &gradients[index] : nullptr;
}
