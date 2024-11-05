// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <webui/httpserver.h>
#include <errno.h>

#include <webui/helpers.h>
#include <debug/dag_debug.h>
#include <sqplus.h>
#include <squirrelObject.h>
#include <squirrelVM.h>
#include <stdlib.h>

using namespace webui;


static void print_indent(YAMemSave &ms, int indent)
{
  for (int i = 0; i < indent; i++)
    ms.printf(" ");
}

static void dump_squirrel_table(YAMemSave &ms, SquirrelObject obj, int indent = 0)
{
  SquirrelObject key;
  SquirrelObject value;

  if (!obj.BeginIteration())
    return;
  print_indent(ms, indent);
  ms.printf("{\n");
  while (obj.Next(key, value))
  {
    switch (key.GetType())
    {
      case OT_STRING:
        if (!strncmp(key.ToString(), "_", 1))
          goto sq_skip;
        print_indent(ms, indent);
        ms.printf("%s", key.ToString());
        break;
      case OT_INTEGER:
        print_indent(ms, indent);
        ms.printf("%d", key.ToInteger());
        break;
      default:
        print_indent(ms, indent);
        ms.printf("???");
        break;
    }

    ms.printf(" = ");
    switch (value.GetType())
    {
      case OT_NULL: ms.printf("<null>"); break;
      case OT_INTEGER: ms.printf("%d", value.ToInteger()); break;
      case OT_FLOAT: ms.printf("%f", value.ToFloat()); break;
      case OT_BOOL: ms.printf("%s", value.ToBool() ? "true" : "false"); break;
      case OT_STRING: ms.printf("%s", value.ToString()); break;
      case OT_TABLE:
      case OT_ARRAY:
        ms.printf("\n");
        dump_squirrel_table(ms, value, indent + 2);
        break;
      case OT_USERDATA: ms.printf("<OT_USERDATA>"); break;
      case OT_CLOSURE: ms.printf("<OT_CLOSURE>"); break;
      case OT_NATIVECLOSURE: ms.printf("<OT_NATIVECLOSURE>"); break;
      case OT_GENERATOR: ms.printf("<OT_GENERATOR>"); break;
      case OT_USERPOINTER: ms.printf("<OT_USERPOINTER>"); break;
      case OT_THREAD: ms.printf("<OT_THREAD>"); break;
      case OT_FUNCPROTO: ms.printf("<OT_FUNCPROTO>"); break;
      case OT_CLASS:
        ms.printf("<OT_CLASS>\n");
        dump_squirrel_table(ms, value, indent + 2);
        break;
      case OT_INSTANCE:
        ms.printf("<OT_INSTANCE>\n");
        dump_squirrel_table(ms, value, indent + 2);
        break;
      case OT_WEAKREF: ms.printf("<OT_WEAKREF>"); break;
      default: ms.printf("<unknown (error)>"); break;
    }
    ms.printf("\n");
  sq_skip:;
  }
  print_indent(ms, indent);
  ms.printf("}\n");
  obj.EndIteration();
}

static void squirrel_root(RequestInfo *params)
{
  YAMemSave ms;

  SquirrelObject root = SquirrelVM::GetRootTable();
  dump_squirrel_table(ms, root);

  text_response(params->conn, ms.mem, (int)ms.offset);
}

webui::HttpPlugin webui::squirrel_http_plugins[] = {{"squirrel", "dump squirrel root table", NULL, squirrel_root}, {NULL}};
