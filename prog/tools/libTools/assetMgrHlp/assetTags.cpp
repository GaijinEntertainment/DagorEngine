// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/assetHlp.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_localConv.h>
#include <util/dag_simpleString.h>
#include <util/dag_fastIntList.h>
#include <ska_hash_map/flat_hash_map2.hpp>

constexpr const char *AssetsTagsFilename = "assets-tags.blk";

static Tab<SimpleString> tagNames;
static Tab<int> tagRefs;
static ska::flat_hash_map<int, FastIntList> assetTags;
static int tagsVersion = 0;

bool assettags::save(const DagorAssetMgr &mgr)
{
  const char *path = assetlocalprops::makePath(AssetsTagsFilename);

  DataBlock blk;
  int index = 0;
  for (int i = 0; i < tagNames.size(); ++i)
  {
    if (getTagRefCount(i) <= 0)
      continue;

    DataBlock *tag = blk.addNewBlock("tag");
    if (tag)
    {
      tag->setStr("name", tagNames[i]);
      const int assetNameId = tag->addNameId("asset");
      for (auto &taggedAsset : assetTags)
      {
        if (taggedAsset.second.hasInt(i))
        {
          const char *assetName = mgr.getAssetName(taggedAsset.first);
          tag->addNewStrByNameId(assetNameId, assetName);
        }
      }
    }
    index++;
  }
  blk.saveToTextFile(path);
  return true;
}

bool assettags::load(const DagorAssetMgr &mgr)
{
  const char *path = assetlocalprops::makePath(AssetsTagsFilename);

  FullFileLoadCB crd(path, DF_READ | DF_IGNORE_MISSING);
  if (crd.fileHandle == nullptr)
    return true;

  DataBlock blk;
  if (!blk.loadFromStream(crd, path, crd.getTargetDataSize()))
    return false;

  const int blockCount = blk.blockCount();
  for (int i = 0; i < blockCount; ++i)
  {
    DataBlock *tag = blk.getBlock(i);
    if (tag)
    {
      const char *blockName = tag->getBlockName();
      if (dd_stricmp(blockName, "tag") != 0)
        continue;

      const char *tagName = tag->getStr("name");
      int tagId = addTagName(tagName);
      const int assetParamId = tag->getNameId("asset");
      for (int j = 0; j < tag->paramCount(); j++)
      {
        if (tag->getParamNameId(j) == assetParamId && tag->getParamType(j) == DataBlock::TYPE_STRING)
        {
          const char *assetName = tag->getStr(j);
          const int assetId = mgr.getAssetNameId(assetName);
          if (assetId > -1)
          {
            FastIntList &tagIds = assetTags[assetId];
            if (tagIds.addInt(tagId))
              tagRefs[tagId]++;
          }
        }
      }
    }
  }
  return true;
}

int assettags::getVersion() { return tagsVersion; }

int assettags::getCount() { return tagNames.size(); }

int assettags::addTagName(const char *name)
{
  int count = tagNames.size();
  for (int i = 0; i < count; ++i)
  {
    if (dd_stricmp(tagNames[i], name) == 0)
      return i;
  }
  tagNames.push_back(SimpleString(name));
  tagRefs.push_back(0);
  tagsVersion++;
  return count;
}

int assettags::getTagNameId(const char *name)
{
  for (int i = 0; i < tagNames.size(); ++i)
  {
    if (dd_stricmp(tagNames[i], name) == 0)
      return i;
  }
  return -1;
}

const char *assettags::getTagName(int tag_id) { return (tag_id >= 0 && tag_id < tagNames.size()) ? tagNames[tag_id].str() : nullptr; }

int assettags::getTagRefCount(int tag_id) { return (tag_id >= 0 && tag_id < tagRefs.size()) ? tagRefs[tag_id] : -1; }

int assettags::getReferencedTagCount()
{
  int count = 0;
  for (int i = 0; i < tagRefs.size(); ++i)
  {
    if (tagRefs[i] > 0)
      count++;
  }
  return count;
}

const FastIntList &assettags::getTagIds(const DagorAsset &asset)
{
  static FastIntList tempList;
  tempList.reset();
  auto it = assetTags.find(asset.getNameId());
  if (it != assetTags.end())
    return it->second;
  return tempList;
}

int assettags::addTagIds(const DagorAsset &asset, const FastIntList &tag_ids)
{
  FastIntList &tagIds = assetTags[asset.getNameId()];
  int added = 0;
  for (int i = 0; i < tag_ids.size(); i++)
  {
    const int tagId = tag_ids[i];
    if (getTagRefCount(tagId) > -1)
    {
      if (tagIds.addInt(tagId))
      {
        tagRefs[tagId]++;
        added++;
        tagsVersion++;
      }
    }
  }
  return added;
}

int assettags::delTagIds(const DagorAsset &asset, const FastIntList &tag_ids)
{
  FastIntList &tagIds = assetTags[asset.getNameId()];
  int removed = 0;
  for (int i = 0; i < tag_ids.size(); i++)
  {
    const int tagId = tag_ids[i];
    if (tagIds.delInt(tagId))
    {
      if (getTagRefCount(tagId) > 0)
        tagRefs[tagId]--;
      removed++;
      tagsVersion++;
    }
  }
  return removed;
}
