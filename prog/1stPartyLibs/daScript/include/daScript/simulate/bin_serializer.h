#pragma once

#include "daScript/simulate/data_walker.h"
#include "daScript/simulate/simulate.h"

namespace das {
    // save ( obj, block<(bytesAt:string)> )
    vec4f _builtin_binary_save ( Context & context, SimNode_CallBase * call, vec4f * args );

    // load ( obj, bytesAt:uint32 )
    vec4f _builtin_binary_load ( Context & context, SimNode_CallBase * call, vec4f * args );
    void _builtin_binary_load ( Context & context, LineInfo * at, TypeInfo* info, const char *data, uint32_t len, char *to);
}
