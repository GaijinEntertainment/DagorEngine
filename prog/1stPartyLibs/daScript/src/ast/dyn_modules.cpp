#include <daScript/ast/dyn_modules.h>

#include <daScript/misc/das_common.h>          // SimulateWithErrReport
#include <daScript/ast/ast.h>                  // ModuleGroup, CompileDaScript
#include <daScript/simulate/aot_builtin_fio.h> // dirent, DIR, readdir
#include <daScript/misc/sysos.h>
#include <daScript/misc/string_writer.h>       // TextWriter
#include <cctype>                              // tolower (case-insensitive basename normalize)
#include <cstdio>                              // fprintf(stderr) for the shadow-shadows-global diagnostic

das::FileAccessPtr get_file_access( char * pak );

namespace das {

// Defined in module_builtin_fio.cpp — re-attempts modules whose dlopen was
// deferred during the folder scan (sibling-module dependency loaded out of order).
void retry_pending_dynamic_modules();

static constexpr const char *MODULE_SUFFIX = ".das_module";
static constexpr const char *INIT_NAME = "initialize";

enum class Result {
    OK,
    // NoModule, // That's ok, we don't force every subfolder folder be module.
    CE,
    SimError,
    Exception,
};

static Result init_dyn_modules(smart_ptr<FileAccess> fa, string path, TextWriter &tout, bool debug = false) {
    const auto mod_filename = path + "/" + MODULE_SUFFIX;
    if (debug) {
        tout << "try file: " << mod_filename << ".\n";
    }
    auto fi = fa->getFileInfo(mod_filename);
    if (fi) {
        ModuleGroup dummyGroup;
        if (debug) {
            tout << "file found: " << mod_filename << ".\n";
        }
        CodeOfPolicies policies;
        policies.no_init_check = true;
        auto program = compileDaScript(mod_filename, fa, tout, dummyGroup, policies);
        if ( program->failed() ) {
            for ( auto & err : program->errors ) {
                tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
            }
            return Result::CE;
        } else {
            auto pctx = SimulateWithErrReport(program, tout);
            if ( !pctx ) {
                return Result::SimError;
            } else {
                auto fnVec = pctx->findFunctions(INIT_NAME);
                das::vector<SimFunction *> fnMVec;
                for ( auto fnAS : fnVec ) {
                    if ( verifyCall<void, const char *>(fnAS->debugInfo, dummyGroup) ) {
                        fnMVec.push_back(fnAS);
                    }
                }
                if ( fnMVec.size()==0 ) {
                    tout << "function '"  << INIT_NAME << "' not found in '" << path << "'\n";
                    return Result::CE;
                } else if ( fnMVec.size()>1 ) {
                    tout << "too many options for '" << INIT_NAME << "'\ncandidates are:\n";
                    for ( auto fnAS : fnMVec ) {
                        tout << "    " << fnAS->mangledName << "\n";
                    }
                    return Result::CE;
                } else {
                    auto fnTest = fnMVec.back();
                    pctx->restart();
                    char * fname = pctx->allocateString(path.c_str(),uint32_t(path.length()),nullptr);
                    vec4f args[1] = {
                        cast<char *>::from(fname)
                    };
                    auto _res = pctx->evalWithCatch(fnTest, args);
                    if ( auto ex = pctx->getException() ) {
                        tout << "EXCEPTION: " << ex << " at " << pctx->exceptionAt.describe() << "\n";
                        return Result::Exception;
                    }
                    return Result::OK;
                }
            }
        }
    } else {
        if (debug) {
            tout << "file not found: " << mod_filename << ".\n";
        }
        return Result::OK;
    }
}

// Normalize a module-folder basename for case-insensitive shadow comparisons
// on filesystems that are case-insensitive at the OS layer (Windows, default
// macOS HFS+/APFS). Without this, a user-supplied -load_module D:/mods/dasimgui
// (lowercase) would NOT shadow modules/dasImgui because the on-disk basenames
// are byte-compared. Linux ext4 is case-sensitive, so leave names untouched.
static das::string normalize_module_name(const das::string &name) {
#if defined(_WIN32) || defined(__APPLE__)
    das::string out = name;
    for (auto &c : out) {
        c = (char)tolower((unsigned char)c);
    }
    return out;
#else
    return name;
#endif
}

// Collect directory names under path/modules/ without loading anything
static das_hash_set<das::string> collect_module_names(const das::string &path) {
    das_hash_set<das::string> result;
#if DAS_NO_FILEIO
    return result;
#else
    das::string modules_path = path + "/modules/";
#if defined(_MSC_VER)
    _finddata_t c_file;
    intptr_t hFile;
    das::string findPath = modules_path + "/*";
    if ((hFile = _findfirst(findPath.c_str(), &c_file)) != -1L) {
        do {
            if (c_file.name[0] == '.') {
                continue;
            }
            result.insert(normalize_module_name(das::string(c_file.name)));
        } while (_findnext(hFile, &c_file) == 0);
        _findclose(hFile);
    }
#else
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(modules_path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') {
                continue;
            }
            result.insert(normalize_module_name(das::string(ent->d_name)));
        }
        closedir(dir);
    }
#endif
    return result;
#endif // DAS_NO_FILEIO
}

static bool init_modules_for_folder(FileAccessPtr fa, const das::string &path, das::TextWriter &tout,
                                    const das_hash_set<das::string> *skip_set = nullptr) {
    // FileAccess do not support iteratinf over directory.
#if DAS_NO_FILEIO
    return false;
#else
    using namespace das;
    string modules_path = path + "/modules/";
    bool all_good = true;
#if defined(_MSC_VER)
    _finddata_t c_file;
    intptr_t hFile;
    string findPath = modules_path + "/*";
    if ((hFile = _findfirst(findPath.c_str(), &c_file)) != -1L) {
        do {
            if (strcmp(c_file.name, ".") == 0 || strcmp(c_file.name, "..") == 0) {
                continue;
            }
            if (skip_set && skip_set->count(normalize_module_name(das::string(c_file.name)))) {
                // stderr, not `tout`: this is a diagnostic, and `tout` is stdout — which for the MCP server /
                // dastest JSON / any stdout-parsing pipeline is a structured data channel a stray line corrupts.
                fprintf(stderr, "Warning: local '%s' shadows global — using local\n", c_file.name);
                continue;
            }
            all_good &= Result::OK == init_dyn_modules(fa, modules_path + c_file.name, tout, false);
        } while (_findnext(hFile, &c_file) == 0);
    }
    _findclose(hFile);
 #else
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (modules_path.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            if (skip_set && skip_set->count(normalize_module_name(das::string(ent->d_name)))) {
                // stderr, not `tout`: this is a diagnostic, and `tout` is stdout — which for the MCP server /
                // dastest JSON / any stdout-parsing pipeline is a structured data channel a stray line corrupts.
                fprintf(stderr, "Warning: local '%s' shadows global — using local\n", ent->d_name);
                continue;
            }
            all_good &= Result::OK == init_dyn_modules(fa, modules_path + ent->d_name, tout, false);
        }
        closedir (dir);
    }
 #endif
    return all_good;
#endif // DAS_NO_FILEIO
}

static das::string path_basename(const das::string &path) {
    // Trim trailing separators so "/mods/foo/" yields "foo" (otherwise
    // `substr(slash+1)` would be empty and shadow-skip would key on "").
    size_t end = path.size();
    while (end > 0 && (path[end - 1] == '/' || path[end - 1] == '\\')) {
        --end;
    }
    if (end == 0) {
        return das::string();
    }
    auto slash = path.find_last_of("\\/", end - 1);
    if (slash == das::string::npos) {
        return path.substr(0, end);
    }
    return path.substr(slash + 1, end - slash - 1);
}

bool require_dynamic_modules(FileAccessPtr file_access,
                             const das::string &das_root,
                             const das::string &project_root,
                             const das::vector<das::string> &load_modules,
                             das::TextWriter &tout) {
    bool has_project = !project_root.empty() &&
        normalizeFileName(das_root.c_str()) !=
        normalizeFileName(project_root.c_str());
    // Collect local module names so we can skip shadows in das_root:
    // both project_root scans and explicit -load_module paths take precedence
    // over das_root entries with matching basenames.
    das_hash_set<das::string> dasroot_skip;
    if (has_project) {
        dasroot_skip = collect_module_names(project_root);
    }
    // Basenames are normalized for case-insensitive filesystems (Windows, macOS)
    // so e.g. `-load_module D:/mods/dasimgui` shadows `modules/dasImgui`.
    das_hash_set<das::string> load_module_names;
    for (const auto &p : load_modules) {
        load_module_names.insert(normalize_module_name(path_basename(p)));
    }
    for (const auto &name : load_module_names) {
        dasroot_skip.insert(name);
    }
    bool all_good = das::init_modules_for_folder(file_access, das_root, tout,
        dasroot_skip.empty() ? nullptr : &dasroot_skip);
    if (has_project) {
        // Init for project_root, skipping anything an explicit -load_module
        // already covers (load_module wins over project_root/modules/<name>).
        all_good &= das::init_modules_for_folder(file_access, project_root, tout,
            load_module_names.empty() ? nullptr : &load_module_names);
    }
    // Finally, init each explicit -load_module path directly.
    for (const auto &p : load_modules) {
        all_good &= (Result::OK == init_dyn_modules(file_access, p, tout));
    }
    // Module .so's may carry DT_NEEDED on sibling-module .so's (e.g. node-editor ->
    // dasModuleImgui) that live in modules/<dep>/ and aren't on RUNPATH. Directory
    // enumeration order is unsorted on Linux (readdir), so a dependent can be visited
    // before its dependency — its register_dynamic_module dlopen then fails and is
    // deferred. Retry the deferred set in fixed-point passes so order stops mattering.
    retry_pending_dynamic_modules();
    return all_good;
}

}
