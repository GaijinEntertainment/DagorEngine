#include <daScript/misc/platform.h>

// For some reason Win requires this function to be marked
// with DAS_API even though we link it directly to exe.
//
// Nothing by default.
DAS_API void require_project_specific_modules() {}
