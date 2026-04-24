#pragma once

#include "daScript/simulate/cast.h"
#include "daScript/ast/compilation_errors.h"

namespace das
{
    class Context;

    DAS_API string unescapeString ( const string & input, bool * error, bool das_escape = true );
    DAS_API string escapeString ( const string & input, bool das_escape = true );
    DAS_API string to_cpp_double ( double val );
    DAS_API string to_cpp_float ( float val );
    DAS_API string reportError ( const struct LineInfo & li, const string & message, const string & extra,
        const string & fixme, CompilationError erc = CompilationError::unspecified );
    DAS_API string reportError ( const char * st, uint32_t stlen, const char * fileName, int row, int col, int lrow, int lcol, int tabSize, const string & message,
        const string & extra, const string & fixme, CompilationError erc = CompilationError::unspecified );
    DAS_API int32_t levenshtein_distance ( const char * s1, const char * s2 );
}

