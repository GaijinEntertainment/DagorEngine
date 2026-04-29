// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/sqEventBus/sqEventBus.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>

sqeventbus::ProcessingMode sqeventbus::bind_ex(SqModules *module_mgr, const char *vm_id, bool freeze_wevt_tables, bool immediate)
{
  const DataBlock *sqevtbusBlk = dgs_get_settings()->getBlockByNameEx("sqeventbus");
  if (const DataBlock *blk = sqevtbusBlk->getBlockByNameEx(vm_id); blk != &DataBlock::emptyBlock)
    sqevtbusBlk = blk;
  auto procMode = sqevtbusBlk->getBool("immediate", immediate) ? ProcessingMode::IMMEDIATE : ProcessingMode::MANUAL_PUMP;
  bool freezeWEvtTables = sqevtbusBlk->getBool("freezeWEvtTables", freeze_wevt_tables);
  bool freezeSqTables = sqevtbusBlk->getBool("freezeSqTables", false);
  bind(module_mgr, vm_id, procMode, freezeWEvtTables, freezeSqTables);
  return procMode;
}
