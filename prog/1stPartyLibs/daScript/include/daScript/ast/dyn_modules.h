#include <daScript/misc/platform.h>             // DAS_API
#include <daScript/simulate/debug_info.h>       // FileAccess

namespace das {

class TextWriter;

// Initializes dynamic modules from:
// - das_root/modules
// - project_root/modules (if project_root is non-empty and differs from das_root)
// - each path in load_modules — treated as a single module folder (the one
//   containing `.das_module`), bypassing the modules/<name> scan. Path basenames
//   shadow same-named entries in das_root and project_root.
// During initialization process it calls `initialize` in files `.das_module`.
DAS_CC_API bool require_dynamic_modules(smart_ptr<FileAccess> file_access,
                                     const string &das_root,
                                     const string &project_root,
                                     const vector<string> &load_modules,
                                     TextWriter &tout);

// As above, plus `disabled_modules`: dynamic modules whose folder basename
// matches (case-insensitive, every platform) are never loaded/registered.
// Used to keep a native-only module (e.g. dashv/libhv) out of a wasm
// cross-compile, so a guarded `require ?mod` resolves as absent instead of
// dragging in a module whose target archive can't exist.
DAS_CC_API bool require_dynamic_modules(smart_ptr<FileAccess> file_access,
                                     const string &das_root,
                                     const string &project_root,
                                     const vector<string> &load_modules,
                                     const vector<string> &disabled_modules,
                                     TextWriter &tout);
}
