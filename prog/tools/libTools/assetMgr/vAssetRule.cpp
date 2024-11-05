// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "vAssetRule.h"
#include <assets/assetMsgPipe.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>

bool DagorVirtualAssetRule::load(const DataBlock &blk, IDagorAssetMsgPipe &msg_pipe)
{
  const char *pattern = blk.getStr("find", NULL);
  if (!pattern)
  {
    post_error(msg_pipe, "find pattern not specified");
    return false;
  }

  if (!reFind.compile(pattern, "i"))
  {
    post_error(msg_pipe, "invalid find pattern: %s", pattern);
    return false;
  }

  nameTemplate = blk.getStr("name", "$1");
  if (nameTemplate.empty())
  {
    post_error(msg_pipe, "bad name template", pattern);
    return false;
  }
  typeName = blk.getStr("className", "");

  int exclude_nid = blk.getNameId("exclude");
  for (int i = 0; i < blk.paramCount(); i++)
    if (blk.getParamNameId(i) == exclude_nid && blk.getParamType(i) == DataBlock::TYPE_STRING)
    {
      pattern = blk.getStr(i);
      RegExp *re = new RegExp;
      if (re->compile(pattern, "i"))
        reExclude.push_back(re);
      else
      {
        post_error(msg_pipe, "invalid exclude pattern: %s", pattern);
        delete re;
      }
    }

  contents.setFrom(blk.getBlockByNameEx("contents"));
  for (int i = 0; i < contents.blockCount(); i++)
    if (strncmp(contents.getBlock(i)->getBlockName(), "tag:", 4) == 0)
    {
      String tag(contents.getBlock(i)->getBlockName() + 4);
      tag.toLower();
      int id = tagNm.addNameId(tag);
      if (id == tagUpd.size())
      {
        tagUpd.push_back(new DataBlock(*contents.getBlock(i)));
        tagUpd.back()->compact();
      }
      else
        logerr("duplicate tag <%s> in virtual_res_blk in %s", contents.getBlock(i)->getBlockName() + 4, blk.resolveFilename());
      contents.removeBlock(i);
      i--;
    }
  contents.compact();

  addSrcFileAsBlk = blk.getBool("addSrcFileAsBlk", false);
  stopProcessing = blk.getBool("stopProcessing", true);
  replaceStrings = blk.getBool("replaceContentStrings", true);
  ignoreDupAsset = blk.getBool("ignoreDupAsset", false);
  overrideDupAsset = blk.getBool("overrideDupAsset", false);

  return true;
}

bool DagorVirtualAssetRule::testRule(const char *fname, String &out_asset_name)
{
  if (!reFind.test(fname))
    return false;
  for (int i = 0; i < reExclude.size(); i++)
    if (reExclude[i]->test(fname))
      return false;

  char *repl = reFind.replace2(nameTemplate);
  out_asset_name = repl;
  memfree(repl, strmem);
  return true;
}


static RegExp *curReFind = NULL;

static void replaceDataBlockStrings(DataBlock &blk)
{
  if (!blk.getBool("replaceContentStrings", true))
    return;

  int num = blk.paramCount();
  for (int i = 0; i < num; ++i)
  {
    const char *name = blk.getParamName(i);

    switch (blk.getParamType(i))
    {
      case DataBlock::TYPE_STRING:
        if (strchr(blk.getStr(i), '$'))
        {
          char *repl = curReFind->replace2(blk.getStr(i));
          blk.setStr(i, repl);
          memfree(repl, strmem);
        }
        break;
    }
  }

  num = blk.blockCount();
  for (int i = 0; i < num; ++i)
    replaceDataBlockStrings(*blk.getBlock(i));
}

void DagorVirtualAssetRule::applyRule(DataBlock &blk, const char *full_fn)
{
  curReFind = &reFind;
  DataBlock storBlk, &tmpBlk = !addSrcFileAsBlk ? blk : storBlk;

  tmpBlk.setFrom(&contents);
  if (tagNm.nameCount())
  {
    String fn(full_fn), substr;
    simplify_fname(fn);
    fn.toLower();
    for (int i = 0; i < tagNm.nameCount(); i++)
    {
      substr.printf(0, "/%s/", tagNm.getName(i));
      if (strstr(fn, substr))
        merge_data_block(tmpBlk, *tagUpd[i], true, true);
    }
  }

  if (replaceStrings)
    replaceDataBlockStrings(tmpBlk);
  if (&tmpBlk != &blk)
    merge_data_block(blk, tmpBlk, false, false, NULL);

  curReFind = NULL;
}
