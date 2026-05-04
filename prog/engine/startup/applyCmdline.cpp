// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_loadSettings.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <debug/dag_debug.h>
#include <memory/dag_framemem.h>
#include <EASTL/fixed_string.h>

static bool apply_blk_from_cli_arg(DataBlock *target, const char *value)
{
  eastl::fixed_string<char, 256, true, framemem_allocator> tmpOpt = value;

  char *assign_sep = strchr(tmpOpt.data(), '=');
  if (assign_sep)
    *assign_sep = '\0';

  // lookup block to apply parameter & parameter itself
  const char *curTok = NULL, *prevTok = NULL;
  do
  {
    prevTok = curTok;
    curTok = strtok(curTok ? NULL : tmpOpt.data(), "/");
    if (!curTok)
      break;
    else if (prevTok)
      target = target->addBlock(prevTok);
  } while (1);

  if (assign_sep)
    *assign_sep = '=';

  DataBlock dummyBlock(framemem_ptr());
  if (prevTok && dblk::load_text(dummyBlock, make_span_const(prevTok, strlen(prevTok)), dblk::ReadFlag::ROBUST))
  {
    merge_data_block(*target, dummyBlock);
    return true;
  }

  return false;
}

const char *dgs_cli_arg_value_for_log(const char *arg, char *buf, int buf_size)
{
  const char *eq = strchr(arg, '=');
  if (eq)
    for (const char *pat : {"login:", "password:"})
      if (const char *p = strstr(arg, pat); p && (p == arg || p[-1] == '/' || p[-1] == '-'))
      {
        snprintf(buf, buf_size, "%.*s=<hidden>", (int)(eq - arg), arg);
        return buf;
      }
  return arg;
}

void dgs_apply_command_line_arg_to_blk(DataBlock *target, const char *value, const char *target_desc)
{
  char buf[256];
  if (apply_blk_from_cli_arg(target, value))
    debug("applied '%s' to '%s'", dgs_cli_arg_value_for_log(value, buf, sizeof(buf)), target_desc);
  else
    logwarn("can't apply '%s' to '%s'", dgs_cli_arg_value_for_log(value, buf, sizeof(buf)), target_desc);
}

OverrideFilter gen_default_override_filter(const SettingsHashMap *changed_settings)
{
  static SettingsHashMap accumulatedChangedSettings;
  if (changed_settings)
  {
    for (int i = 0; i < changed_settings->nameCount(); i++)
      accumulatedChangedSettings.addNameId(changed_settings->getName(i));
  }
  const SettingsHashMap *accumulatedChangedSettingsPtr = &accumulatedChangedSettings;
  return [accumulatedChangedSettingsPtr](const char *setting) {
    if (accumulatedChangedSettingsPtr->nameCount() == 0)
      return true;
    const char *nameEnd = strchr(setting, ':');
    size_t nameLen = nameEnd ? (nameEnd - setting) : strlen(setting);
    return accumulatedChangedSettingsPtr->getNameId(setting, nameLen) < 0;
  };
}

void dgs_apply_command_line_to_config(DataBlock *target, const OverrideFilter *override_filter)
{
  int iterator = 0;
  while (const char *blk = ::dgs_get_argv("config", iterator))
    if (strchr(blk, '=') && (!override_filter || (*override_filter)(blk))) // historically config may contain a path to config file
      dgs_apply_command_line_arg_to_blk(target, blk, "config");
}
