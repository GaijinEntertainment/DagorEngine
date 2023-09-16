#include <bindQuirrelEx/bindQuirrelEx.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <sqModules/sqModules.h>


namespace bindquirrel
{

void apply_compiler_options_from_game_settings(SqModules *mng)
{
  HSQUIRRELVM vm = mng->getVM();
  const DataBlock *blk = ::dgs_get_settings()->getBlockByNameEx("quirrel");
  mng->compilationOptions.useAST = blk->getBool("ast", true);
  sq_setcompilationoption(vm, CO_CLOSURE_HOISTING_OPT, blk->getBool("closureHoisting", true));
}

} // namespace bindquirrel
