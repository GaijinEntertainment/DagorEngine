#include "daScript/misc/platform.h"
#include "daScript/daScriptModule.h"
#include "daScript/simulate/fs_file_info.h"
#include "daScript/simulate/simulate.h"
#include "daScript/das_project_specific.h"

using namespace das;

#if !DAS_NO_FILEIO
static GetFileAccessFunc specificGetFileAccess = nullptr;
#endif

static GetNewContextFunc specificGetNewContext = nullptr;
static GetCloneContextFunc specificGetCloneContext = nullptr;

namespace das {

    void set_project_specific_fs_callbacks(GetFileAccessFunc getFileAccess) {
        DAS_ASSERT(getFileAccess);
        DAS_ASSERT(specificGetFileAccess == nullptr);
        specificGetFileAccess = getFileAccess;
    }

    void set_project_specific_ctx_callbacks(GetNewContextFunc getNewContext, GetCloneContextFunc getCloneContext) {
        DAS_ASSERT(getNewContext);
        DAS_ASSERT(getCloneContext);
        DAS_ASSERT(specificGetNewContext == nullptr);
        DAS_ASSERT(specificGetCloneContext == nullptr);

        specificGetNewContext = getNewContext;
        specificGetCloneContext = getCloneContext;
    }

} // namespace das

smart_ptr<das::FileAccess> get_file_access( char * pak ) {
    if (specificGetFileAccess)
        return specificGetFileAccess(pak);
#if !DAS_NO_FILEIO
    if ( pak ) {
        return make_smart<FsFileAccess>(pak, make_smart<FsFileAccess>());
    } else {
        return make_smart<FsFileAccess>();
    }
#else
    DAS_FATAL_ERROR(
        "daScript is configured with DAS_NO_FILEIO. However file access is not specified."
        "set_project_specific_fs_callbacks or link-time dependency in project_specific_file_info.cpp "
        "needs to be speicied."
    )
    return nullptr;
#endif
}

Context * get_context( int stackSize = 0 ) {
    if (specificGetNewContext)
        return specificGetNewContext(stackSize);
    return new Context(stackSize);
}


Context * get_clone_context( Context * ctx, uint32_t category ) {
    if (specificGetCloneContext)
        return specificGetCloneContext(ctx, category);
    return new Context(*ctx, category);
}
