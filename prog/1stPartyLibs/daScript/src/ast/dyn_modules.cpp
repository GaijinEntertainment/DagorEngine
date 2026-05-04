#include <daScript/ast/dyn_modules.h>

#include <daScript/das_common.h>               // SimulateWithErrReport
#include <daScript/ast/ast.h>                  // ModuleGroup, CompileDaScript
#include <daScript/simulate/aot_builtin_fio.h> // dirent, DIR, readdir
#include <daScript/misc/sysos.h>
#include <daScript/misc/string_writer.h>       // TextWriter

das::FileAccessPtr get_file_access( char * pak );

namespace das {

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
        auto program = compileDaScript(mod_filename, fa, tout, dummyGroup);
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
            result.insert(das::string(c_file.name));
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
            result.insert(das::string(ent->d_name));
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
            if (skip_set && skip_set->count(das::string(c_file.name))) {
                tout << "Warning: local '" << c_file.name << "' shadows global — using local\n";
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
            if (skip_set && skip_set->count(das::string(ent->d_name))) {
                tout << "Warning: local '" << ent->d_name << "' shadows global — using local\n";
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

bool require_dynamic_modules(FileAccessPtr file_access,
                             const das::string &das_root,
                             const das::string &project_root,
                             das::TextWriter &tout) {
    bool has_project = !project_root.empty() &&
        normalizeFileName(das_root.c_str()) !=
        normalizeFileName(project_root.c_str());
    // Collect local module names first so we can skip shadows in das_root
    das_hash_set<das::string> local_names;
    if (has_project) {
        local_names = collect_module_names(project_root);
    }
    // Always init for dasroot, skipping names that exist locally
    bool all_good = das::init_modules_for_folder(file_access, das_root, tout,
        local_names.empty() ? nullptr : &local_names);
    if (has_project) {
        // Init for project_root (no skipping)
        all_good &= das::init_modules_for_folder(file_access, project_root, tout);
    }
    return all_good;
}

}
