// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "circuit.h"
#include "main.h"
#include "vromfs.h"
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>
#include <EASTL/string.h>
#include <EASTL/fixed_string.h>
#include <EASTL/unique_ptr.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_direct.h>
#include <compressionUtils/memSilentInPlaceLoadCB.h>
#include <digitalSignature/digitalSignatureCheck.h>
#include <publicConfigs/publicConfigs.h>
#include "settings.h"
#include "net/telemetry.h"
#include "gameProjConfig.h"
#include <util/dag_string.h>
#include <ioSys/dag_dataBlockUtils.h>

#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_C4

#elif _TARGET_C3

#endif

namespace circuit
{

#define NETWORK_BLK_NAME         "network.blk"
#define NETWORK_BLK_OFFLINE_PATH "config/network.blk"

static DataBlock circuit_conf;
static eastl::fixed_string<char, 16> circuit_name;

#if _TARGET_ANDROID | _TARGET_IOS
// env is tied to updater config right now
static DataBlock mobile_updater_conf;
static bool mobile_updater_conf_loaded = false;

void load_mobile_updater_config()
{
  if (!mobile_updater_conf_loaded)
  {

#if _TARGET_ANDROID
    if (!dblk::load(mobile_updater_conf, "asset://updater.blk", dblk::ReadFlag::ROBUST))
#else
    if (!dblk::load(mobile_updater_conf, "updater.blk", dblk::ReadFlag::ROBUST))
#endif
    {
      DAG_FATAL("updater.blk was not found in assets");
    }
    mobile_updater_conf_loaded = true;
  }
}
#endif

bool is_staging_env()
{
#if _TARGET_C3

#elif _TARGET_ANDROID | _TARGET_IOS
  load_mobile_updater_config();
  return mobile_updater_conf.getBool("staging", false);
#else
  return false;
#endif
}

bool is_submission_env()
{
#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_C4

#elif _TARGET_C3

#elif _TARGET_ANDROID | _TARGET_IOS
  load_mobile_updater_config();
  return mobile_updater_conf.getBool("submission", false);
#endif
  return false;
}

const char *init_name_early()
{
  if (!circuit_name.empty())
    return circuit_name.c_str();
  //-config:circuit:t=sun
  static const char CIRCUIT_ARG[] = "circuit:t=";
  int iterator = 0;
  while (const char *config = ::dgs_get_argv("config", iterator))
    if (strncmp(config, CIRCUIT_ARG, sizeof(CIRCUIT_ARG) - 1) == 0)
    {
      circuit_name = config + sizeof(CIRCUIT_ARG) - 1;
      if (!circuit_name.empty())
        return circuit_name.c_str();
      else
        return nullptr;
    }

  if (!dgs_get_settings() || dgs_get_settings()->isEmpty())
  {
    eastl::string configBlkFilename(eastl::string::CtorSprintf{}, "%s.blk", gameproj::config_name_prefix());
    DataBlock local_config_blk;
    if (dd_file_exist(configBlkFilename.c_str()))
      if (dblk::load(local_config_blk, configBlkFilename.c_str(), dblk::ReadFlag::ROBUST))
      {
        const char *name = nullptr;
        if (is_submission_env())
          name = get_platformed_value(local_config_blk, "submission_circuit", nullptr);
        else if (is_staging_env())
          name = get_platformed_value(local_config_blk, "staging_circuit", nullptr);
        if (!name) //-V547
          name = get_platformed_value(local_config_blk, "circuit", nullptr);
        if (name)
        {
          circuit_name = name;
          return circuit_name.c_str();
        }
      }
  }
  return nullptr;
}

static void network_blk_cb(dag::ConstSpan<char> data, void *arg)
{
  if (data.empty())
  {
    logerr("network.blk download failed");
    return;
  }

  G_ASSERT(arg);
  eastl::unique_ptr<DataBlock> &dst = *((eastl::unique_ptr<DataBlock> *)arg);

  enum
  {
    SHA256_SIG_SZ = 256
  };
  const void *buf[] = {data.data(), NULL};
  unsigned buf_sz[] = {unsigned(data.size()) - SHA256_SIG_SZ, 0};

  if (data.size() < SHA256_SIG_SZ || !verify_digital_signature_with_key(buf, buf_sz, buf_sz[0] + (unsigned char *)buf[0],
                                       SHA256_SIG_SZ, gameproj::public_key_DER(), gameproj::public_key_DER_len()))
  {
    logerr("network.blk signature verification failed (%d bytes)", data.size());
    return;
  }

  if (data.data() && data_size(data))
  {
    MemSilentInPlaceLoadCB crd(data.data(), data_size(data));
    dst.reset(new DataBlock);
    if (!dblk::load_from_stream(*dst, crd, dblk::ReadFlag::ROBUST))
    {
      debug("failed to load cloud network.blk due to broken blk file");
      dst.reset();
    }
  }
}

static bool download_network_blk(eastl::unique_ptr<DataBlock> &dst, const char *circuitName)
{
  char blkPath[DAGOR_MAX_PATH];
  SNPRINTF(blkPath, sizeof(blkPath), "%s/network.blk", circuitName);

  return pubcfg::get(blkPath, network_blk_cb, &dst);
}

static void load_operator_overrides(DataBlock *circuit_blk)
{
  DataBlock partnerConfig(framemem_ptr());
  if (!dblk::load(partnerConfig, "yupartner.blk", dblk::ReadFlag::ROBUST))
    return;

  const char *operatorStr = partnerConfig.getStr("partner", nullptr);
  if (!operatorStr || strcmp(operatorStr, "gaijin") == 0)
    return;

  DataBlock overrideConfig(framemem_ptr());
  String override_config_path(0, "%s/override_circuit.blk", operatorStr);
  if (!dblk::load(overrideConfig, override_config_path, dblk::ReadFlag::ROBUST))
  {
    logerr("Operator config not found ( %s )", override_config_path.c_str());
    return;
  }

  debug("Using operator: %s", operatorStr);

  DataBlock overrideBlk(framemem_ptr());
  if (const DataBlock *overrideCommon = overrideConfig.getBlockByName("_common"))
    overrideBlk.setFrom(overrideCommon);
  if (const DataBlock *overrideCircuit = overrideConfig.getBlockByName(circuit_blk->getBlockName()))
    merge_data_block(overrideBlk, *overrideCircuit);

  merge_data_block(*circuit_blk, overrideBlk);
}

void init()
{
  const DataBlock &settings = *dgs_get_settings();

  eastl::unique_ptr<DataBlock> networkBlk;
  if (settings.getBlockByName("debug")->getBool("forceLocalNetworkBlk", false))
    debug_print_datablock("forced local network.blk", &settings);
  else
    download_network_blk(networkBlk, gameproj::game_codename());

  if (!networkBlk) // When failed to get online network.blk -> load it from vrom
    networkBlk.reset(new DataBlock(NETWORK_BLK_OFFLINE_PATH));

  const char *curCircuit = nullptr;

  if (is_submission_env())
  {
    curCircuit = get_platformed_value(settings, "submission_circuit", nullptr);
    if (curCircuit)
      debug("Use circuit for submission mode");
    else
      debug("Failed to get submission circuit. fallback to common one");
  }
  else if (is_staging_env())
  {
    curCircuit = get_platformed_value(settings, "staging_circuit", nullptr);
    if (curCircuit)
      debug("Use circuit for staging mode");
    else
      debug("Failed to get staging circuit. fallback to common one");
  }
  if (!curCircuit)
    curCircuit = get_platformed_value(settings, "circuit", nullptr);
  curCircuit = settings.getStr("circuit", curCircuit);

  if (curCircuit)
  {
    circuit_name = curCircuit;
    debug("Use network circuit '%s'", curCircuit);
    DataBlock const *circuitBlk = networkBlk->getBlockByName(curCircuit);
#if DAGOR_DBGLEVEL > 0
    if (!circuitBlk)
      circuitBlk = settings.getBlockByNameEx("network")->getBlockByName(curCircuit);
#endif

    if (circuitBlk)
      circuit_conf = *circuitBlk;
    else
      logerr("Circuit config for %s not found, using empty", curCircuit);
  }
  else
    debug("Don't use network circuit (not set in *.settings.blk, *.config.blk or commandline)");
  load_operator_overrides(&circuit_conf);
}

const DataBlock *get_conf() { return &circuit_conf; }

eastl::string_view get_name() { return circuit_name; }

} // namespace circuit
