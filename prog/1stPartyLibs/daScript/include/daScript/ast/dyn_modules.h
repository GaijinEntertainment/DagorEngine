#include <daScript/misc/platform.h>             // DAS_API
#include <daScript/simulate/debug_info.h>       // FileAccess

namespace das {

class TextWriter;

// Initializes dynamic modules from:
// - das_root/modules
// - project_root/modules
// If project_root is empty it will be ignored.
// During initialization process it calls `initialize` in files `.das_module`.
DAS_API bool require_dynamic_modules(smart_ptr<FileAccess> file_access,
                                     const string &das_root,
                                     const string &project_root,
                                     TextWriter &tout);
}
