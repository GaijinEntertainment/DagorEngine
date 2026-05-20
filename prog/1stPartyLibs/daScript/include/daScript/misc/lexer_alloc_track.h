#pragma once

#include "daScript/misc/platform.h"

#if DAS_TRACK_ALLOC

#include <string>
#include <cstdio>

namespace das {

    // RelWithDebInfo-only side map: yytext per lexer NAME std::string*,
    // dumped at exit to attribute leaks (the C++ stack tracker groups them
    // all under one site).

    // Param is tokenText, not yytext — flex defines yytext as a macro.
    void lexer_track_alloc(std::string *p, const char *tokenText) noexcept;
    void lexer_track_free(std::string *p) noexcept;
    int  dump_lexer_string_leaks(FILE *out) noexcept;

}

#endif
