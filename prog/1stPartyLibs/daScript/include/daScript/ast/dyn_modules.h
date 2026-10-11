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

// src/ast/ARCHITECTURE.md#module-scan-manifest
struct DynModuleManifestRow {
    bool    dynamic = false;    // false: native path (a=mod_name, b=src, c=dst); true: dynamic module (a=path, b=cpp class, c=das name)
    bool    group = false;      // group member (a=group, b=member, c=guard or empty)
    string  a, b, c;
    int     on_error = 0;       // dynamic only: the RegisterOnError the descriptor used
};
DAS_API void begin_dynamic_module_recording();
DAS_API void end_dynamic_module_recording(vector<DynModuleManifestRow> & rows, bool & optOut);
DAS_API void replay_native_path(const char * mod_name, const char * src, const char * dst);
DAS_API void replay_module_group(const char * group, const char * member, const char * guard);
DAS_API void replay_dynamic_module(const char * path, const char * cpp_class, int on_error);
DAS_API void defer_dynamic_module(const char * path, const char * cpp_class, int on_error, const char * das_name);
DAS_API bool load_deferred_dynamic_module(const char * das_name);
DAS_API size_t load_all_deferred_dynamic_modules();                 // the count it attempted
DAS_API string describe_pending_dynamic_modules();                  // "" when nothing is pending
DAS_API bool is_dynamic_module_deferred(const char * das_name);
DAS_API bool pending_dynamic_module_artifact_present();             // a dlopen-failed row whose file exists: an import the deferred set holds
DAS_API string registered_dynamic_module_name(const char * path, const char * cpp_class);   // empty while that row's module is not registered
DAS_API void clear_deferred_dynamic_modules();                      // the rows are the scan's: cleared at scan start and at shutdown
DAS_CC_API void ignore_dynamic_module_manifests(bool ignore);
}
