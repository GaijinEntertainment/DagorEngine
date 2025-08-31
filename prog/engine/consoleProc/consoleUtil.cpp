// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
#include <util/dag_string.h>

void console::util::append_command_hints_to_string(const CommandStruct &cmd, String &out_cmd_text)
{
  if (!cmd.argsDescription.empty())
  {
    out_cmd_text += cmd.argsDescription;
  }
  else if (!cmd.varValue.empty())
  {
    out_cmd_text += "(=";
    out_cmd_text += cmd.varValue;
    out_cmd_text += ") ";
  }
  else
  {
    int args = 0;
    for (int k = 1; k < cmd.minArgs; k++)
    {
      out_cmd_text += "<x> ";
      args++;
    }

    for (int k = cmd.minArgs; k < cmd.maxArgs; k++)
    {
      out_cmd_text += "[x] ";
      args++;
      if (args > 12)
      {
        out_cmd_text += "...";
        break;
      }
    }
  }

  if (!cmd.description.empty())
  {
    out_cmd_text += " -- ";
    out_cmd_text += cmd.description;
  }
}

void console::util::print_command_with_hints_to_string(const CommandStruct &cmd, String &out_cmd_text)
{
  out_cmd_text.printf(256, "%s  ", cmd.command.str());
  append_command_hints_to_string(cmd, out_cmd_text);
}
