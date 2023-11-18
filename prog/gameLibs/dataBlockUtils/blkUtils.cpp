#include <dataBlockUtils/blkUtils.h>

#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>

bool check_param_exist(const DataBlock &blk, std::initializer_list<const char *> param_names)
{
  G_ASSERT(param_names.size() > 0);

  for (auto param_name : param_names)
    if (blk.paramExists(param_name))
      return true;

  return false;
}

bool check_all_params_exist(const DataBlock &blk, const char *prop_name, std::initializer_list<const char *> param_names)
{
  G_ASSERT(param_names.size() > 0);

  bool existSome = false;
  bool existAll = true;

  for (auto param_name : param_names)
  {
    if (blk.paramExists(param_name))
      existSome = true;
    else
      existAll = false;
  }

  if (existSome && !existAll)
  {
    for (auto param_name : param_names)
      if (!blk.paramExists(param_name))
        debug("parameter %s in %s not defined!", param_name, prop_name);
  }

  return existAll;
}

bool check_all_params_exist_in_subblocks(const DataBlock &blk, const char *subblock_name, const char *prop_name,
  std::initializer_list<const char *> param_names)
{
  G_ASSERT(param_names.size() > 0);

  int subblockNameId = blk.getNameId(subblock_name);
  if (subblockNameId < 0 || !blk.getBlockByName(subblockNameId))
    return false;

  bool res = true;

  for (int i = 0; i < blk.blockCount(); ++i)
    if (blk.getBlock(i)->getBlockNameId() == subblockNameId)
      res &= check_all_params_exist(*blk.getBlock(i), prop_name, param_names);

  return res;
}
