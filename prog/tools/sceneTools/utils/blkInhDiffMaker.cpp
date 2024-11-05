// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "blkInhDiffMaker.h"
#include "blkUtil.cpp"

#include <stdio.h>

#include <osApiWrappers/dag_direct.h>
#include <util/dag_string.h>
#include <util/dag_oaHashNameMap.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/blkUtil.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_fileIo.h>


bool DataBlockInhDiffMaker::makeDiff(const char *_parent_fn, const char *_source_fn, const char *_result_fn)
{
  // prepare files

  if (!dd_file_exist(_parent_fn))
  {
    printf("No parent file found \"%s\"\n", _parent_fn);
    return false;
  }

  if (!dd_file_exist(_source_fn))
  {
    printf("No source file found \"%s\"\n", _source_fn);
    return false;
  }

  String resultFn;
  if (_result_fn)
    resultFn = _result_fn;
  else
  {
    const char *ext = dd_get_fname_ext(_source_fn);
    if (ext)
    {
      resultFn = _source_fn;
      erase_items(resultFn, ext - _source_fn, strlen(ext));
      resultFn = resultFn + "_inh" + ext;
    }
    else
      resultFn = String(512, "%s_%s", _source_fn, "inh.blk");

    printf("Result file is \"%s\" \n", resultFn.str());
  }

  // making diff
  resBlk.reset();
  DataBlock parent(_parent_fn), source(_source_fn);
  if (!diff_datablocks(parent, source, resBlk))
    printf("Files is equal \n");

  // append include block and save

  char res_loc[260];
  ::dd_get_fname_location(res_loc, resultFn.str());
  String inc_fn = ::make_path_relative(_parent_fn, res_loc);
  String line(512, "include \"%s\"\n\n", inc_fn.str());
  DynamicMemGeneralSaveCB cwr(tmpmem, 0, 32 << 10);
  cwr.write(line.str(), line.size() - 1);
  resBlk.saveToTextStream(cwr);
  FullFileSaveCB fcwr(resultFn);
  fcwr.write(cwr.data(), cwr.size());
  fcwr.close();

  // test load and save
  /*
  DataBlock test_blk(resultFn);
  test_blk.saveToTextFile(resultFn + ".txt");
  //*/

  return true;
}


bool DataBlockInhDiffMaker::diff_datablocks(DataBlock &parent_blk, DataBlock &source_blk, DataBlock &result_blk)
{
  FastNameMapEx blk_names;
  Tab<int> start_after_named(tmpmem), block_counter(tmpmem);
  bool has_diff = false;

  for (int i = 0; i < source_blk.blockCount(); ++i)
  {
    DataBlock *s_block = source_blk.getBlock(i);
    const char *s_block_name = s_block->getBlockName();
    int p_block_id = parent_blk.getNameId(s_block_name);

    // parent block not exists

    if (p_block_id == -1)
    {
      result_blk.addNewBlock(s_block);
      has_diff = true;

      printf("new block \"%s\": \n", s_block_name); // DEBUG

      continue;
    }

    // parent block index found

    int old_blk_ind = -1;
    int found_blk_ind = -1;
    int sa_ind = blk_names.getNameId(s_block_name);
    if (sa_ind != -1)
      old_blk_ind = start_after_named[sa_ind];

    // search of blk

    for (int j = old_blk_ind + 1; j < parent_blk.blockCount(); ++j)
      if (parent_blk.getBlock(j)->getBlockNameId() == p_block_id)
      {
        found_blk_ind = j;
        break;
      }

    if (found_blk_ind == -1)
    {
      result_blk.addNewBlock(s_block);
      has_diff = true;

      printf("new copy of block \"%s\": \n", s_block_name); // DEBUG

      continue;
    }
    else
    {
      if (sa_ind != -1)
      {
        start_after_named[sa_ind] = found_blk_ind;
        ++block_counter[sa_ind];
      }
      else
      {
        blk_names.addNameId(s_block_name);
        start_after_named.push_back(found_blk_ind);
        block_counter.push_back(1);
      }

      // parent block found

      DataBlock *p_block = parent_blk.getBlock(found_blk_ind);
      DataBlock tmp_blk;
      bool diff_blks = diff_datablocks(*p_block, *s_block, tmp_blk);
      bool diff_pars = diff_params(*p_block, *s_block, tmp_blk);

      if (diff_blks || diff_pars)
      {
        String new_name = (sa_ind != -1) && (block_counter[sa_ind] > 1)
                            ? String(64, "@override:%s[%d]", s_block_name, block_counter[sa_ind])
                            : String(64, "@override:%s", s_block_name);
        result_blk.addNewBlock(&tmp_blk, new_name.str());
        has_diff = true;
      }
    }
  }

  // delete blocks

  for (int i = 0; i < parent_blk.blockCount(); ++i)
  {
    DataBlock *p_block = parent_blk.getBlock(i);
    const char *p_block_name = p_block->getBlockName();

    int old_block_ind = -1;
    int sa_ind = blk_names.getNameId(p_block_name);
    if (sa_ind != -1)
      old_block_ind = start_after_named[sa_ind];

    if (old_block_ind == -1)
    {
      printf("delete block \"%s\": \n", p_block_name); // DEBUG

      result_blk.addNewBlock(String(64, "@delete:%s", p_block_name).str());
      has_diff = true;
    }
    else if (i > old_block_ind)
    {
      printf("delete block copy \"%s\": \n", p_block_name); // DEBUG

      result_blk.addNewBlock(String(64, "@delete:%s[%d]", p_block_name, block_counter[sa_ind] + 1).str());
      has_diff = true;
    }
  }

  return has_diff;
}


bool DataBlockInhDiffMaker::diff_params(DataBlock &parent_blk, DataBlock &source_blk, DataBlock &result_blk)
{
  bool has_diff = false;
  FastNameMapEx param_names;
  Tab<int> start_after_named(tmpmem), param_counter(tmpmem);

  for (int i = 0; i < source_blk.paramCount(); ++i)
  {
    const char *s_param_name = source_blk.getParamName(i);
    int p_param_id = parent_blk.getNameId(s_param_name);

    // parent param not exists

    if (p_param_id == -1)
    {
      cpyBlkParam(source_blk, i, result_blk, s_param_name);
      has_diff = true;
      printf("new param \"%s\": \n", s_param_name); // DEBUG
      continue;
    }

    // parent param index found

    int old_param_ind = -1;
    int found_param_ind = -1;
    int sa_ind = param_names.getNameId(s_param_name);
    if (sa_ind != -1)
      old_param_ind = start_after_named[sa_ind];

    // search of param

    found_param_ind = parent_blk.findParam(p_param_id, old_param_ind);

    if (found_param_ind == -1)
    {
      cpyBlkParam(source_blk, i, result_blk, s_param_name);
      has_diff = true;

      printf("new copy of param \"%s\": \n", s_param_name); // DEBUG

      continue;
    }
    else
    {
      if (sa_ind != -1)
      {
        start_after_named[sa_ind] = found_param_ind;
        ++param_counter[sa_ind];
      }
      else
      {
        param_names.addNameId(s_param_name);
        start_after_named.push_back(found_param_ind);
        param_counter.push_back(1);
      }

      // parent param found

      if (!cmpBlkParam(parent_blk, found_param_ind, source_blk, i))
      {
        printf("new param value \"%s\": \n", s_param_name); // DEBUG

        String new_name = (sa_ind != -1) && (param_counter[sa_ind] > 1)
                            ? String(64, "@override:%s[%d]", s_param_name, param_counter[sa_ind])
                            : String(64, "@override:%s", s_param_name);
        cpyBlkParam(source_blk, i, result_blk, new_name.str());
        has_diff = true;
      }
    }
  }

  // delete params

  for (int i = 0; i < parent_blk.paramCount(); ++i)
  {
    const char *p_param_name = parent_blk.getParamName(i);
    int old_param_ind = -1;
    int sa_ind = param_names.getNameId(p_param_name);
    if (sa_ind != -1)
      old_param_ind = start_after_named[sa_ind];

    if (old_param_ind == -1)
    {
      printf("delete param \"%s\": \n", p_param_name); // DEBUG

      cpyBlkParam(parent_blk, i, result_blk, String(64, "@delete:%s", p_param_name).str());
      has_diff = true;
    }
    else if (i > old_param_ind)
    {
      printf("delete copy param \"%s\": \n", p_param_name); // DEBUG

      cpyBlkParam(parent_blk, i, result_blk, String(64, "@delete:%s[%d]", p_param_name, param_counter[sa_ind] + 1).str());
      has_diff = true;
    }
  }

  return has_diff;
}
