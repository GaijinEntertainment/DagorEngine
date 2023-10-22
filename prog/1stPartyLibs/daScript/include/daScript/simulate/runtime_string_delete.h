#pragma once

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/simulate_visit_op.h"

namespace das
{
    struct StringIterator : Iterator {
        StringIterator ( char * st ) : str(st) {}
        virtual bool first ( Context & context, char * value ) override;
        virtual bool next  ( Context & context, char * value ) override;
        virtual void close ( Context & context, char * value ) override;
        char * str;
    };

    struct SimNode_StringIterator : SimNode {
        SimNode_StringIterator ( const LineInfo & at, SimNode * s )
            : SimNode(at), source(s) { }
        virtual SimNode * visit ( SimVisitor & vis ) override;
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
        SimNode *   source;
    };
}

#include "daScript/simulate/simulate_visit_op_undef.h"

