#pragma once

#include "daScript/simulate/cast.h"
#include "daScript/ast/compilation_errors.h"

namespace das
{
    class Context;

    string unescapeString ( const string & input, bool * error, bool das_escape = true );
    string escapeString ( const string & input, bool das_escape = true );
    string to_string_ex ( double dnum );
    string to_string_ex ( float dnum );
    string reportError ( const struct LineInfo & li, const string & message, const string & extra,
        const string & fixme, CompilationError erc = CompilationError::unspecified );
    string reportError ( const char * st, uint32_t stlen, const char * fileName, int row, int col, int lrow, int lcol, int tabSize, const string & message,
        const string & extra, const string & fixme, CompilationError erc = CompilationError::unspecified );
}

