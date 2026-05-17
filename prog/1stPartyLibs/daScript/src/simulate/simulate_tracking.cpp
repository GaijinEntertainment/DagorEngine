#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/simulate/simulate.h"

namespace das
{
    string around ( const LineInfo & info ) {
        TextWriter tw;
        tw << ":" << info.line << ":" << info.column;
        return tw.str();
    }

    void ptr_ref_count::DumpTrackPtr() {
        lock_guard<mutex> guard(ref_count_mutex);
        TextPrinter tp;
        int total = 0;
        for ( auto p = ref_count_head; p; p = p->ref_count_next ) {
            tp << "0x" << HEX << intptr_t(p) << DEC
               << " (rc=" << p->use_count() << ", id=" << HEX << p->ref_count_id << DEC << ")";
            if ( p->ref_count_magic == TRACK_PTR_CONTEXT ) {
                auto ctx = static_cast<Context*>(p);
                tp << " Context " << ctx->name;
            } else if ( p->ref_count_magic == TRACK_PTR_PROGRAM ) {
                tp << " Program";
            } else if ( p->ref_count_magic == TRACK_PTR_FILE_ACCESS ) {
                tp << " FileAccess";
            }
            tp << "\n";
            total++;
        }
        if ( total ) tp << "total " << total << " tracked pointers\n";
    }
}
