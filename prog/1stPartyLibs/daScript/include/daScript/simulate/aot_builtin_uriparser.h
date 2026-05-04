#pragma once

#include "daScript/misc/uric.h"

namespace das {
    class Context;
    struct LineInfoArg;
    DAS_API void delete_uri ( Uri & uri );
    DAS_API void clone_uri ( Uri & uri, const Uri & uriS );
    DAS_API char * uri_to_string ( const Uri & uri, Context * context, LineInfoArg * at );
    DAS_API char * text_range_to_string ( const UriTextRangeA & trange, Context * context, LineInfoArg * at );
    DAS_API char * to_unix_file_name ( const Uri & uri, Context * context, LineInfoArg * at );
    DAS_API char * to_windows_file_name ( const Uri & uri, Context * context, LineInfoArg * at );
    DAS_API char * to_file_name ( const Uri & uri, Context * context, LineInfoArg * at );
    DAS_API Uri from_file_name ( const char * str );
    DAS_API Uri from_windows_file_name ( const char * str );
    DAS_API Uri from_unix_file_name ( const char * str );
    DAS_API char * uri_to_unix_file_name ( char * uristr, Context * context, LineInfoArg * at );
    DAS_API char * uri_to_windows_file_name ( char * uristr, Context * context, LineInfoArg * at );
    DAS_API char * unix_file_name_to_uri ( char * uristr, Context * context, LineInfoArg * at );
    DAS_API char * windows_file_name_to_uri ( char * uristr, Context * context, LineInfoArg * at );
    DAS_API char * escape_uri ( char * uristr, bool spaceToPlus, bool normalizeBreaks, Context * context, LineInfoArg * at );
    DAS_API char * unescape_uri ( char * uristr,Context * context, LineInfoArg * at );
    DAS_API char * normalize_uri ( char * uristr,Context * context, LineInfoArg * at );
    DAS_API char * makeNewGuid( Context * context, LineInfoArg * at);
    DAS_API void uri_for_each_query_kv ( const Uri & uri, const TBlock<void,TTemporary<char *>,TTemporary<char*>> & blk, Context * context, LineInfoArg * at );
}
