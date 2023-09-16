#pragma once

#include "daScript/simulate/fs_file_info.h"

namespace das {
    typedef smart_ptr<FileAccess> (* GetFileAccessFunc)(char * pak);
    void set_project_specific_fs_callbacks(GetFileAccessFunc getFileAccess);

    class Context;

    typedef Context * (* GetNewContextFunc)(int stackSize);
    typedef Context * (* GetCloneContextFunc)(Context * ctx, uint32_t category);

    void set_project_specific_ctx_callbacks(GetNewContextFunc getNewContext, GetCloneContextFunc getCloneContext);
}
