// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "jsonDump.h"
#include "jsonDumpCommon.h"

#include <util/dag_console.h>


namespace dafg
{

namespace jsondump
{

Buffer sb;
Writer w(sb);

} // namespace jsondump

namespace state_dump
{

void dump(const DumpSources &dump_src, const char *custom_filename)
{
  jsondump::sb.Clear();
  jsondump::w.Reset(jsondump::sb);
  jsondump::w.SetIndent(' ', 2);

  {
    jsondump::w.StartObject();

    jsondump::dump_registry_to_json(dump_src.registry);
    jsondump::dump_ir_graph_to_json(dump_src.registry, dump_src.ir_graph);
    jsondump::dump_state_deltas_to_json(dump_src.ir_graph, dump_src.state_deltas);
    jsondump::dump_events_to_json(dump_src.ir_graph, dump_src.events);
    jsondump::dump_layout_to_json(dump_src.ir_graph, dump_src.layout);

    jsondump::w.EndObject();
  }

  if (!jsondump::w.IsComplete())
  {
    console::error("daFG: failed to dump, json is broken!");
    return;
  }


  String filename(custom_filename);
  if (filename.empty())
  {
    DagorDateTime dt;
    get_local_time(&dt);
    filename.printf(0, "dafgStateDump_%02d-%02d-%02d", dt.hour, dt.minute, dt.second);
  }

  jsondump::write_to_file(filename, jsondump::sb);
}

} // namespace state_dump

} // namespace dafg
