#pragma once

#include "daScript/misc/uric.h"

namespace das {
    class Context;
    struct LineInfoArg;
    void delete_uri ( Uri & uri );
    void clone_uri ( Uri & uri, const Uri & uriS );
    char * uri_to_string ( const Uri & uri, Context * context );
    char * text_range_to_string ( const UriTextRangeA & trange, Context * context );
    char * to_unix_file_name ( const Uri & uri, Context * context );
    char * to_windows_file_name ( const Uri & uri, Context * context );
    char * to_file_name ( const Uri & uri, Context * context );
    Uri from_file_name ( const char * str );
    Uri from_windows_file_name ( const char * str );
    Uri from_unix_file_name ( const char * str );
    char * uri_to_unix_file_name ( char * uristr, Context * context );
    char * uri_to_windows_file_name ( char * uristr, Context * context );
    char * unix_file_name_to_uri ( char * uristr, Context * context );
    char * windows_file_name_to_uri ( char * uristr, Context * context );
    char * escape_uri ( char * uristr, bool spaceToPlus, bool normalizeBreaks, Context * context );
    char * unescape_uri ( char * uristr,Context * context );
    char * normalize_uri ( char * uristr,Context * context );
    char * makeNewGuid( Context * context, LineInfoArg * at );
    void uri_for_each_query_kv ( const Uri & uri, const TBlock<void,TTemporary<char *>,TTemporary<char*>> & blk, Context * context, LineInfoArg * at );
}
