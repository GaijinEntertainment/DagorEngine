#pragma once

#include "daScript/simulate/fs_file_info.h"

namespace das {
    typedef smart_ptr<FileAccess> (* GetFileAccessFunc)(char * pak);
    void set_project_specific_fs_callbacks(GetFileAccessFunc getFileAccess);
    // project-specific file-access override, consumed by get_file_access (compiler lib)
    DAS_API GetFileAccessFunc get_project_specific_file_access();

    class Context;

    typedef Context * (* GetNewContextFunc)(int stackSize);
    typedef Context * (* GetCloneContextFunc)(Context * ctx, uint32_t category);

    void set_project_specific_ctx_callbacks(GetNewContextFunc getNewContext, GetCloneContextFunc getCloneContext);
}

DAS_API das::Context * get_context( int stackSize = 0 );
