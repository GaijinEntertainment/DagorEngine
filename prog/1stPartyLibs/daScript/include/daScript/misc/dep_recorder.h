#pragma once

#include "daScript/misc/platform.h"

namespace das {

    DAS_API void das_dep_arm ( const char * depFilePath, const char * target );

    DAS_API void das_dep_note_read ( const char * path );

    DAS_API void das_dep_flush ();

    DAS_API int das_dep_flag_argc ( const char * arg );

    DAS_API bool das_dep_is_env_entry ( const char * entry );
}
