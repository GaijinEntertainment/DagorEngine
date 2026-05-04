// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sepClientInstance/blkConfigReader.h>
#include <sepClientInstance/sepClientInstanceTypes.h>

#include <debug/dag_assert.h>
#include <debug/dag_log.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <osApiWrappers/dag_miscApi.h>
#include <startup/dag_globalSettings.h>


constexpr char SEP_CONFIG_BLOCK_NAME[] = "sep";


namespace sepclientinstance::configreader
{

enum class ChildType
{
  Param,
  Block
};


// copy of function `get_platform_string` from `prog/daNetGame/main/settings.cpp`
static const char *get_platform_string()
{
  // if (is_true_net_server())
  //   return nullptr;

  switch (get_platform_id()) // -V785
  {
    case TP_WIN32:
    case TP_WIN64:
    case TP_LINUX64:
    case TP_MACOSX: return "pc";
    default: return get_platform_string_id();
  }
}


static bool blk_child_exists(const DataBlock &blk, const char *key, ChildType child_type)
{
  if (child_type == ChildType::Param)
  {
    const int nameId = blk.getNameId(key);
    if (nameId == -1)
    {
      return false;
    }
    const bool exists = blk.paramExists(nameId);
    return exists;
  }
  else
  {
    const bool exists = blk.blockExists(key);
    return exists;
  }
}


// based on function `get_platformed_value` from `prog/daNetGame/main/settings.cpp`
// NOTE! Here we fixed a bug when original `get_platformed_value`
// was using `getNameId` by mistake and could get false-positive key detection.
static eastl::string get_platformed_value_name(const DataBlock &blk, const char *key, ChildType child_type)
{
  eastl::string resultKey;

  const char *const platformString = get_platform_string();
  if (!platformString)
  {
    G_ASSERT_LOG(false, "[sep][sep_error] Failed detecting platform name (nullptr)");
    resultKey.assign(key);
    return resultKey;
  }

#if _TARGET_C4 && _TARGET_PC_WIN





#endif

  resultKey.sprintf("%s_%s", platformString, key);
  if (blk_child_exists(blk, resultKey.c_str(), child_type))
  {
    return resultKey;
  }

  resultKey.assign(key);
  return resultKey;
}


DAGOR_NOINLINE static int get_int_from_blk(const DataBlock &blk, const char *key, int default_value)
{
  const eastl::string platformedKey = get_platformed_value_name(blk, key, ChildType::Param);
  return blk.getInt(platformedKey.c_str(), default_value);
}


DAGOR_NOINLINE static bool get_bool_from_blk(const DataBlock &blk, const char *key, bool default_value)
{
  const eastl::string platformedKey = get_platformed_value_name(blk, key, ChildType::Param);
  return blk.getBool(platformedKey.c_str(), default_value);
}


DAGOR_NOINLINE static const DataBlock &get_block_from_blk(const DataBlock &blk, const char *key)
{
  const eastl::string platformedBlockName = get_platformed_value_name(blk, key, ChildType::Block);
  const DataBlock *childBlk = blk.getBlockByName(platformedBlockName.c_str());
  return childBlk ? *childBlk : DataBlock::emptyBlock;
}


DAGOR_NOINLINE static SepClientInstanceConfig from_circuit_blk_config(const DataBlock &sep_block)
{
  SepClientInstanceConfig config;

  config.sepUsagePermilleLimit = get_int_from_blk(sep_block, "sepUsagePermille", 0);
  config.supportCompression = get_bool_from_blk(sep_block, "supportCompression", true);
  config.verboseLogging = get_bool_from_blk(sep_block, "verboseLogging", false);

  const DataBlock &serversBlk = get_block_from_blk(sep_block, "servers");

  config.sepUrls.clear();
  config.sepUrls.reserve(serversBlk.paramCount());

  for (int i = 0; i < serversBlk.paramCount(); ++i)
  {
    if (serversBlk.getParamType(i) == DataBlock::TYPE_STRING)
    {
      config.sepUrls.emplace_back(serversBlk.getStr(i));
    }
    else
    {
      logwarn("[sep][sep_error] from_circuit_blk_config: invalid type of server url item at index %d, expected string, got type #%d",
        i, serversBlk.getParamType(i));
    }
  }

  return config;
}


static eastl::string to_string(const DataBlock &blk)
{
  DynamicMemGeneralSaveCB buffer(framemem_ptr(), 0, 8 << 10);
  blk.saveToTextStream(buffer);
  return {(const char *)buffer.data(), (size_t)buffer.size()};
}


SepClientInstanceConfig from_circuit_blk_config(get_config_cb_t get_circuit_config_cb)
{
  const DataBlock *circuitConfig = get_circuit_config_cb ? get_circuit_config_cb() : nullptr;
  if (!circuitConfig)
  {
    if (get_circuit_config_cb)
      logwarn("[sep][sep_error] from_circuit_blk_config: circuit config callback returned null");
    else
      logwarn("[sep][sep_error] from_circuit_blk_config: missing circuit config callback");

    circuitConfig = dgs_get_settings();
    G_ASSERT_RETURN(circuitConfig, {});
  }

  const DataBlock *overrideCircuit = dgs_get_settings()->getBlockByNameEx("debug");

  const DataBlock *mainSepBlock = circuitConfig->getBlockByNameEx(SEP_CONFIG_BLOCK_NAME);
  const DataBlock *overrideSepBlock = overrideCircuit->getBlockByNameEx(SEP_CONFIG_BLOCK_NAME);

  const eastl::string serializedMainSepBlock = to_string(*mainSepBlock);
  logdbg("[sep] from_circuit_blk_config: SEP circuit config block (before overriding):\n%s", serializedMainSepBlock.c_str());

  DataBlock mergedSepBlock(*mainSepBlock);

  if (overrideSepBlock->isEmpty())
  {
    logdbg("[sep] from_circuit_blk_config: no sep config overrides found in \"dgs_get_settings()/debug/sep\" block");
  }
  else
  {
    const eastl::string serializedOverrideSepBlock = to_string(*overrideSepBlock);
    logdbg("[sep] from_circuit_blk_config: applying config overrides from \"dgs_get_settings()/debug/sep\" block:\n%s",
      serializedOverrideSepBlock.c_str());

    overrideDataBlock(mergedSepBlock, *overrideSepBlock);
  }

  return from_circuit_blk_config(mergedSepBlock);
}


} // namespace sepclientinstance::configreader
