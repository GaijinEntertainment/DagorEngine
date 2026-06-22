#pragma once

#include "daScript/misc/gc_node.h"
#include "daScript/misc/string_writer.h"

namespace das {

    // Group every gc_node on `root` by source file -> file:line, with per-type counts,
    // and print sorted by count (descending). Diagnostic. Reads each node's `at` via a
    // per-tag downcast — only the AST layer knows the concrete subclasses.
    DAS_API void gcReportHistogram ( const gc_root & root, const char * label, TextWriter & out, uint64_t minCount = 1 );

    // Per-stage leak delta. Snapshots the active gc root, diffs it against the previous
    // snapshot on this thread, prints what `stage` added/removed (grouped by file -> line),
    // then stores the new baseline. Resets the baseline when stage == "parse" (start of a
    // module). The compile unit is labelled by `moduleName`, falling back to `fileName` when
    // the module is nameless (descriptor parses). Gated by env DAS_GC_STAGE_REPORT — no-op when unset.
    DAS_API void gcStageReportDelta ( const char * moduleName, const char * fileName, const char * stage, TextWriter & out );

    // True if DAS_GC_STAGE_REPORT is set (cached). Lets call sites skip building strings.
    DAS_API bool gcStageReportEnabled ();

}
