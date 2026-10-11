#include <daScript/ast/dyn_modules.h>

#include <daScript/misc/das_common.h>          // SimulateWithErrReport
#include <daScript/ast/ast.h>                  // ModuleGroup, CompileDaScript
#include <daScript/simulate/aot_builtin_fio.h> // dirent, DIR, readdir
#include <daScript/simulate/aot_builtin.h>     // das_get_cross_platform_name, das_is_dll_build
#include <daScript/misc/sysos.h>
#include <daScript/misc/string_writer.h>       // TextWriter, LOG (the env-gated scan trace)
#include <daScript/misc/anyhash.h>             // hash_block64
#include <daScript/misc/env_cfg.h>             // get_dasenv_trace_module_load
#include <daScript/misc/performance_time.h>
#include <cctype>                              // tolower (case-insensitive basename normalize)
#include <mutex>
#include <cstdio>                              // fprintf(stderr) for the shadow-shadows-global diagnostic

das::FileAccessPtr get_file_access( char * pak );

namespace das {

// Defined in module_builtin_fio.cpp — re-attempts modules whose dlopen was
// deferred during the folder scan (sibling-module dependency loaded out of order).
DAS_API void retry_pending_dynamic_modules();

static constexpr const char *MODULE_SUFFIX = ".das_module";
static constexpr const char *INIT_NAME = "initialize";

enum class Result {
    OK,
    // NoModule, // That's ok, we don't force every subfolder folder be module.
    CE,
    SimError,
    Exception,
};

static Result run_descriptor(smart_ptr<FileAccess> fa, const string & mod_filename, const string & path, TextWriter &tout,
                             das::vector<string> * deps = nullptr) {
    ModuleGroup dummyGroup;
    CodeOfPolicies policies;
    policies.no_init_check = true;
    policies.ignore_shared_modules = true;  // src/ast/ARCHITECTURE.md#module-scan-manifest
    auto program = compileDaScript(mod_filename, fa, tout, dummyGroup, policies);
    if ( program->failed() ) {
        for ( auto & err : program->errors ) {
            tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
        }
        return Result::CE;
    }
    if ( deps ) {   // every module with a file, the descriptor itself excluded - the manifest's key
        program->library.foreach([&](Module * mod) -> bool {
            if ( !mod->fileName.empty() && mod->fileName != mod_filename
                && das::find(deps->begin(), deps->end(), mod->fileName) == deps->end() ) {
                deps->push_back(mod->fileName);
            }
            return true;
        }, "*");
    }
    auto pctx = SimulateWithErrReport(program, tout);
    if ( !pctx ) {
        return Result::SimError;
    }
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
    }
    auto fnTest = fnMVec.back();
    pctx->restart();
    char * fname = pctx->allocateString(path.c_str(),uint32_t(path.length()),nullptr);
    vec4f args[1] = {
        cast<char *>::from(fname)
    };
    pctx->evalWithCatch(fnTest, args);
    if ( auto ex = pctx->getException() ) {
        tout << "EXCEPTION: " << ex << " at " << pctx->exceptionAt.describe() << "\n";
        return Result::Exception;
    }
    return Result::OK;
}

// src/ast/ARCHITECTURE.md#module-scan-manifest - one file per binary kind and per compile target,
// so a dynamic-module build and a static one, a native run and a --jit-target one, sharing a
// tree never rewrite each other's rows
static string manifest_file_name() {
    string name = ".das_module";
    if ( !das_is_dll_build() ) name += ".static";
    const string target = das_get_cross_platform_name();
    if ( !target.empty() ) name += "." + target;
    return name + ".manifest";
}
static constexpr const char *MANIFEST_HEADER = "das_module_manifest\t4";

static bool g_ignore_manifests = false;

void ignore_dynamic_module_manifests(bool ignore) {
    g_ignore_manifests = ignore;
}

static bool trace_scan() {
    static const bool on = []{
        const char * e = get_dasenv_trace_module_load();
        return e && e[0] && e[0] != '0';
    }();
    return on;
}

enum class ManifestVerdict { Missing, Stale, Damaged, OptOut, Replay };

struct DepStamp {
    string path;
    uint32_t size = 0;
    uint64_t hash = 0;
};

struct ManifestRead {
    ManifestVerdict verdict = ManifestVerdict::Missing;
    das::vector<DynModuleManifestRow> rows;
    das::vector<DepStamp> deps;     // the key's dependency stamps, verified against the files - a rewrite carries them
    string why;
};

#if !DAS_NO_FILEIO   // the reader's helpers: a build with no file io reads no manifest
static das::vector<string> split_tabs(const string & line) {
    das::vector<string> fields;
    size_t start = 0;
    for ( ;; ) {
        auto tab = line.find('\t', start);
        if ( tab == string::npos ) {
            fields.push_back(line.substr(start));
            return fields;
        }
        fields.push_back(line.substr(start, tab - start));
        start = tab + 1;
    }
}
#endif

static string hex64(uint64_t v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%016llx", (unsigned long long) v);
    return buf;
}

// one hash per file per FileAccess, however many descriptors share it
static bool dep_stamp(const smart_ptr<FileAccess> & fa, const string & path, uint32_t & size, uint64_t & hash) {
    struct Entry { const FileInfo * fi; uint32_t size; uint64_t hash; };
    static thread_local const FileAccess * owner = nullptr;
    static thread_local das::unordered_map<string, Entry> cache;
    if ( owner != fa.get() ) {
        owner = fa.get();
        cache.clear();
    }
    auto fi = fa->getFileInfo(path);
    if ( !fi ) return false;
    auto it = cache.find(path);
    if ( it != cache.end() && it->second.fi == fi ) {
        size = it->second.size;
        hash = it->second.hash;
        return true;
    }
    const char * src = nullptr;
    uint32_t len = 0;
    fi->getSourceAndLength(src, len);
    if ( !src ) return false;
    size = len;
    hash = hash_block64((const uint8_t *) src, len);
    cache[path] = Entry{fi, size, hash};
    return true;
}

#if !DAS_NO_FILEIO
static bool parse_on_error(const string & s, int & value) {   // RegisterOnError: Quiet, ErrorMsg, Fail
    if ( s.size() != 1 || s[0] < '0' || s[0] > '2' ) return false;
    value = s[0] - '0';
    return true;
}
#endif

// the inputs a descriptor's rows can depend on beyond its own bytes: its folder, the das root
// (`get_das_root()`), and the cross-compile target (`get_cross_platform_name()`, read from argv)
struct ManifestKey {
    string root, dasRoot, target;
};

static ManifestKey manifest_key(const string & path) {
    ManifestKey key;
    key.root = path;
    key.dasRoot = getDasRoot();
    key.target = das_get_cross_platform_name();
    return key;
}

static ManifestRead read_manifest(const string & file, uint32_t descSize, uint64_t descHash, const ManifestKey & key,
                                  const smart_ptr<FileAccess> & fa) {
    ManifestRead res;
#if DAS_NO_FILEIO
    (void)file; (void)descSize; (void)descHash; (void)key; (void)fa;
    return res;
#else
    FILE * f = fopen(file.c_str(), "rb");
    if ( !f ) {
        return res;
    }
    string text;
    char buf[4096];
    for ( size_t n; (n = fread(buf, 1, sizeof(buf), f)) > 0; ) {
        text.append(buf, n);
    }
    fclose(f);
    das::vector<string> lines;
    for ( size_t start = 0; start < text.size(); ) {
        auto nl = text.find('\n', start);
        if ( nl == string::npos ) {
            lines.push_back(text.substr(start));
            break;
        }
        lines.push_back(text.substr(start, nl - start));
        start = nl + 1;
    }
    auto damaged = [&](const char * why) {
        res.verdict = ManifestVerdict::Damaged;
        res.rows.clear();
        res.why = why;
        return res;
    };
    auto stale = [&](const char * why) {
        res.verdict = ManifestVerdict::Stale;
        res.rows.clear();
        res.why = why;
        return res;
    };
    if ( lines.size() < 7 ) return damaged("fewer than 7 lines");
    if ( lines[0] != MANIFEST_HEADER ) return stale("format version");
    auto stamp = split_tabs(lines[1]);
    if ( stamp.size() != 3 || stamp[0] != "stamp" ) return damaged("stamp line");
    if ( stamp[1] != to_string(descSize) || stamp[2] != hex64(descHash) ) return stale("descriptor changed");
    auto dll = split_tabs(lines[2]);
    if ( dll.size() != 2 || dll[0] != "dll" ) return damaged("dll line");
    if ( dll[1] != (das_is_dll_build() ? "1" : "0") ) return stale("binary kind");
    auto rootLine = split_tabs(lines[3]);
    if ( rootLine.size() != 2 || rootLine[0] != "root" ) return damaged("root line");
    if ( rootLine[1] != key.root ) return stale("module folder moved");
    auto dasRootLine = split_tabs(lines[4]);
    if ( dasRootLine.size() != 2 || dasRootLine[0] != "dasroot" ) return damaged("dasroot line");
    if ( dasRootLine[1] != key.dasRoot ) return stale("das root moved");
    auto targetLine = split_tabs(lines[5]);
    if ( targetLine.size() != 2 || targetLine[0] != "target" ) return damaged("target line");
    if ( targetLine[1] != key.target ) return stale("compile target");
    bool optOut = false;
    size_t i = 6;
    for ( ; i < lines.size(); ++i ) {
        if ( lines[i].empty() ) return damaged("empty line");
        auto fields = split_tabs(lines[i]);
        const auto & kind = fields[0];
        if ( kind == "end" ) break;
        if ( kind == "dep" ) {
            if ( fields.size() != 4 ) return damaged("dep field count");
            uint32_t size = 0;
            uint64_t hash = 0;
            if ( !dep_stamp(fa, fields[1], size, hash) ) return stale("dependency missing");
            if ( fields[2] != to_string(size) || fields[3] != hex64(hash) ) return stale("dependency changed");
            res.deps.push_back(DepStamp{fields[1], size, hash});
        } else if ( kind == "np" ) {
            if ( fields.size() != 4 ) return damaged("np field count");
            DynModuleManifestRow row;
            row.a = fields[1]; row.b = fields[2]; row.c = fields[3];
            res.rows.push_back(das::move(row));
        } else if ( kind == "dm" ) {
            if ( fields.size() != 5 ) return damaged("dm field count");
            DynModuleManifestRow row;
            row.dynamic = true;
            if ( !parse_on_error(fields[3], row.on_error) ) return damaged("dm on_error field");
            row.a = fields[1]; row.b = fields[2]; row.c = fields[4];
            res.rows.push_back(das::move(row));
        } else if ( kind == "grp" ) {
            if ( fields.size() != 4 ) return damaged("grp field count");
            DynModuleManifestRow row;
            row.group = true;
            row.a = fields[1]; row.b = fields[2]; row.c = fields[3];
            res.rows.push_back(das::move(row));
        } else if ( kind == "no_manifest" ) {
            if ( fields.size() != 1 ) return damaged("no_manifest field count");
            optOut = true;
        } else {
            return damaged("unknown row kind");
        }
    }
    if ( i >= lines.size() ) return damaged("no end line");
    auto endLine = split_tabs(lines[i]);
    if ( endLine.size() != 2 || endLine[1] != to_string(res.rows.size()) ) return damaged("end row count");
    for ( ++i; i < lines.size(); ++i ) {
        if ( !lines[i].empty() ) return damaged("content after end");
    }
    if ( optOut && !res.rows.empty() ) return damaged("no_manifest with rows");
    res.verdict = optOut ? ManifestVerdict::OptOut : ManifestVerdict::Replay;
    return res;
#endif
}

static bool field_ok(const string & s) {
    return s.find('\t') == string::npos && s.find('\n') == string::npos && s.find('\r') == string::npos;
}

static bool write_manifest(const string & file, uint32_t descSize, uint64_t descHash, const ManifestKey & key,
                           const das::vector<DepStamp> & deps, const das::vector<DynModuleManifestRow> & rows,
                           bool optOut, string & why) {
    if ( !field_ok(key.root) || !field_ok(key.dasRoot) || !field_ok(key.target) ) { why = "a key path contains a tab or newline"; return false; }
    string text = string(MANIFEST_HEADER) + "\n";
    text += "stamp\t" + to_string(descSize) + "\t" + hex64(descHash) + "\n";
    text += string("dll\t") + (das_is_dll_build() ? "1" : "0") + "\n";
    text += "root\t" + key.root + "\n";
    text += "dasroot\t" + key.dasRoot + "\n";
    text += "target\t" + key.target + "\n";
    for ( auto & dep : deps ) {
        if ( !field_ok(dep.path) ) { why = "a dependency path contains a tab or newline"; return false; }
        text += "dep\t" + dep.path + "\t" + to_string(dep.size) + "\t" + hex64(dep.hash) + "\n";
    }
    if ( optOut ) {
        text += "no_manifest\n";
        text += "end\t0\n";
    } else {
        for ( auto & row : rows ) {
            if ( !field_ok(row.a) || !field_ok(row.b) || !field_ok(row.c) ) {
                why = "a recorded argument contains a tab or newline";
                return false;
            }
            if ( row.dynamic ) {
                text += "dm\t" + row.a + "\t" + row.b + "\t" + to_string(row.on_error) + "\t" + row.c + "\n";
            } else if ( row.group ) {
                text += "grp\t" + row.a + "\t" + row.b + "\t" + row.c + "\n";
            } else {
                text += "np\t" + row.a + "\t" + row.b + "\t" + row.c + "\n";
            }
        }
        text += "end\t" + to_string(rows.size()) + "\n";
    }
#if DAS_NO_FILEIO
    (void)file;
    why = "no file io";
    return false;
#else
    const string tmp = file + ".tmp";
    FILE * f = fopen(tmp.c_str(), "wb");
    if ( !f ) { why = "cannot create " + tmp; return false; }
    const bool wrote = fwrite(text.data(), 1, text.size(), f) == text.size();
    fclose(f);
    if ( !wrote ) { remove(tmp.c_str()); why = "short write to " + tmp; return false; }
#if defined(_WIN32)
    remove(file.c_str());   // rename does not replace on Windows
#endif
    if ( rename(tmp.c_str(), file.c_str()) != 0 ) { remove(tmp.c_str()); why = "cannot rename " + tmp; return false; }
    return true;
#endif
}

// src/ast/ARCHITECTURE.md#module-scan-manifest - a replayed manifest with a nameless dm row, kept until the scan ends
struct NamelessRowsManifest {
    string descriptor, file;
    uint32_t descSize = 0;
    uint64_t descHash = 0;
    ManifestKey key;
    das::vector<DepStamp> deps;
    das::vector<DynModuleManifestRow> rows;
};
static das::vector<NamelessRowsManifest> g_nameless_manifests;

static void name_replayed_rows() {
    for ( auto & m : g_nameless_manifests ) {
        size_t named = 0;
        for ( auto & row : m.rows ) {
            if ( !row.dynamic || !row.c.empty() ) continue;
            auto name = registered_dynamic_module_name(row.a.c_str(), row.b.c_str());
            if ( name.empty() ) continue;
            row.c = name;
            named ++;
        }
        if ( named == 0 ) continue;
        string why;
        const bool written = write_manifest(m.file, m.descSize, m.descHash, m.key, m.deps, m.rows, false, why);
        if ( trace_scan() ) {
            LOG(LogLevel::info) << "[module] descriptor " << m.descriptor
                << (written ? ": manifest rewritten, " + to_string(named) + " row(s) named" : ": manifest not rewritten: " + why) << "\n";
        }
    }
    g_nameless_manifests.clear();
}

static Result init_dyn_modules(smart_ptr<FileAccess> fa, string path, TextWriter &tout, bool debug = false) {
    const auto mod_filename = path + "/" + MODULE_SUFFIX;
    if (debug) {
        tout << "try file: " << mod_filename << ".\n";
    }
    auto fi = fa->getFileInfo(mod_filename);
    if (!fi) {
        if (debug) {
            tout << "file not found: " << mod_filename << ".\n";
        }
        return Result::OK;
    }
    if (debug) {
        tout << "file found: " << mod_filename << ".\n";
    }
    const char * src = nullptr;
    uint32_t len = 0;
    fi->getSourceAndLength(src, len);
    const uint64_t stamp = src ? hash_block64((const uint8_t *) src, len) : 0;
    const string manifest = path + "/" + manifest_file_name();
    const ManifestKey key = manifest_key(path);
    auto time0 = ref_time_ticks();
#if DAS_NO_FILEIO
    const bool useManifest = false;
    const char * noManifestWhy = "no file io";
#else
    const bool useManifest = src && !g_ignore_manifests;
    const char * noManifestWhy = !src ? "no source" : g_ignore_manifests ? "manifests ignored" : "no_manifest";
#endif
    auto mr = useManifest ? read_manifest(manifest, len, stamp, key, fa) : ManifestRead();
    if ( mr.verdict == ManifestVerdict::Replay ) {
        int64_t dllUsec = 0;
        size_t deferred = 0;
        bool nameless = false;
        for ( auto & row : mr.rows ) {
            nameless |= row.dynamic && row.c.empty();
        }
        if ( nameless ) {   // src/ast/ARCHITECTURE.md#module-scan-manifest - named at the end of the scan once the module registered
            g_nameless_manifests.push_back({mod_filename, manifest, len, stamp, key, mr.deps, mr.rows});
        }
        for ( auto & row : mr.rows ) {
            if ( row.group ) {
                replay_module_group(row.a.c_str(), row.b.c_str(), row.c.c_str());
            } else if ( !row.dynamic ) {
                replay_native_path(row.a.c_str(), row.b.c_str(), row.c.c_str());
            } else if ( !row.c.empty() ) {
                defer_dynamic_module(row.a.c_str(), row.b.c_str(), row.on_error, row.c.c_str());
                deferred ++;
            } else {
                auto dll0 = ref_time_ticks();
                replay_dynamic_module(row.a.c_str(), row.b.c_str(), row.on_error);
                dllUsec += get_time_usec(dll0);
            }
        }
        if ( trace_scan() ) {
            LOG(LogLevel::info) << "[module] descriptor " << mod_filename << ": replayed " << mr.rows.size() << " row(s) in "
                << (get_time_usec(time0) / 1000000.) << " (shared module load " << (dllUsec / 1000000.)
                << ", deferred " << deferred << ")\n";
        }
        return Result::OK;
    }
    const bool record = useManifest && mr.verdict != ManifestVerdict::OptOut;
    if ( record ) begin_dynamic_module_recording();
    das::vector<string> depFiles;
    auto res = run_descriptor(fa, mod_filename, path, tout, record ? &depFiles : nullptr);
    if ( !record ) {
        if ( trace_scan() ) {
            LOG(LogLevel::info) << "[module] descriptor " << mod_filename << ": compiled (" << noManifestWhy << ")\n";
        }
        return res;
    }
    das::vector<DynModuleManifestRow> rows;
    bool optOut = false;
    end_dynamic_module_recording(rows, optOut);
    if ( res != Result::OK ) {
        if ( trace_scan() ) {
            LOG(LogLevel::info) << "[module] descriptor " << mod_filename << ": compiled with errors, no manifest\n";
        }
        return res;
    }
    string why;
    das::vector<DepStamp> deps;
    for ( auto & dep : depFiles ) {
        DepStamp d;
        d.path = dep;
        if ( !dep_stamp(fa, dep, d.size, d.hash) ) { why = "cannot stamp " + dep; break; }
        deps.push_back(das::move(d));
    }
    const bool written = why.empty() && write_manifest(manifest, len, stamp, key, deps, rows, optOut, why);
    if ( trace_scan() ) {
        LOG(LogLevel::info) << "[module] descriptor " << mod_filename << ": compiled (" << (mr.why.empty() ? "no manifest" : mr.why) << ")"
            << (written ? (optOut ? ", no_manifest recorded" : ", manifest written (" + to_string(rows.size()) + " row(s))")
                        : ", manifest not written: " + why) << "\n";
    }
    return res;
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

// Always lowercase, on every platform — unlike normalize_module_name (which is
// identity on case-sensitive Linux). The explicit `--disable-module` list is
// user intent, not a filesystem-shadow concern, so `--disable-module dashv`
// must match the on-disk folder `dasHV` everywhere, including CI's Linux.
static das::string force_lower(const das::string &name) {
    das::string out = name;
    for (auto &c : out) {
        c = (char)tolower((unsigned char)c);
    }
    return out;
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
                                    const das_hash_set<das::string> *skip_set = nullptr,
                                    const das_hash_set<das::string> *disabled_set = nullptr) {
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
                fprintf(stderr, "Warning: local '%s' shadows global - using local\n", c_file.name);
                continue;
            }
            if (disabled_set && disabled_set->count(force_lower(das::string(c_file.name)))) {
                continue;   // explicitly disabled (--disable-module) — never load/register
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
                fprintf(stderr, "Warning: local '%s' shadows global - using local\n", ent->d_name);
                continue;
            }
            if (disabled_set && disabled_set->count(force_lower(das::string(ent->d_name)))) {
                continue;   // explicitly disabled (--disable-module) — never load/register
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

static void move_all_nodes(gc_root & from, gc_root & to) {
    while ( from.gc_first ) {
        auto node = from.gc_first;
        from.gc_unlink(node);
        to.gc_link(node);
    }
}

// src/ast/ARCHITECTURE.md#module-scan-manifest
static bool load_deferred_module_for_require(const string & name) {
    static recursive_mutex loadMutex;
    lock_guard<recursive_mutex> guard(loadMutex);
    // a collect walks one root: the thread root's own nodes park while the load's leftovers gather on loadRoot
    auto & threadRoot = gc_root::gc_get_thread_root();
    gc_root parked, loadRoot;
    move_all_nodes(threadRoot, parked);
    auto boundProgram = daScriptEnvironment::getBound()->g_Program;    // a module's builtin das part parses under its own program; the caller may be mid-parse
    bool loaded = false;
    bool grown = false;     // the module set changed - the fixed point and the collect are owed whether or not `name` came in
    {
        gc_active_scope scope(&loadRoot);
        bool wasDeferred = is_dynamic_module_deferred(name.c_str());
        loaded = load_deferred_dynamic_module(name.c_str());
        grown = loaded;
        if ( !loaded && wasDeferred ) {
            // its dlopen failed - a sibling it links may be deferred too; bring every row in (retries the pending ones)
            load_all_deferred_dynamic_modules();
            grown = true;
            loaded = Module::require(name) != nullptr;
        }
        if ( grown ) {
            string notInitialized;
            if ( !Module::InitializeDependencies(notInitialized) ) {
                load_all_deferred_dynamic_modules();
                if ( !Module::InitializeDependencies(notInitialized) ) {
                    DAS_FATAL_ERROR("Unable to initialize some modules:%s\n", notInitialized.c_str());
                }
            }
        }
    }
    move_all_nodes(threadRoot, loadRoot);
    if ( grown ) {
        // every module: a constructor registers into existing ones too (a vector type's functions)
        Module::foreach([&](Module * m) {
            m->gc_collect(&loadRoot);
            return true;
        });
    }
    loadRoot.gc_sweep();
    move_all_nodes(parked, threadRoot);
    daScriptEnvironment::getBound()->g_Program = boundProgram;
    return loaded;
}

bool require_dynamic_modules(FileAccessPtr file_access,
                             const das::string &das_root,
                             const das::string &project_root,
                             const das::vector<das::string> &load_modules,
                             const das::vector<das::string> &disabled_modules,
                             das::TextWriter &tout) {
    clear_deferred_dynamic_modules();
    g_nameless_manifests.clear();
    setDeferredModuleLoader(&load_deferred_module_for_require);
    // Explicitly-disabled modules (case-insensitive on every platform) are never
    // loaded/registered — keeps a native-only module out of a wasm cross-compile.
    das_hash_set<das::string> disabled_set;
    for (const auto &m : disabled_modules) {
        disabled_set.insert(force_lower(m));
    }
    const das_hash_set<das::string> *disabled_ptr = disabled_set.empty() ? nullptr : &disabled_set;
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
        dasroot_skip.empty() ? nullptr : &dasroot_skip, disabled_ptr);
    if (has_project) {
        // Init for project_root, skipping anything an explicit -load_module
        // already covers (load_module wins over project_root/modules/<name>).
        all_good &= das::init_modules_for_folder(file_access, project_root, tout,
            load_module_names.empty() ? nullptr : &load_module_names, disabled_ptr);
    }
    // Finally, init each explicit -load_module path directly (unless disabled).
    for (const auto &p : load_modules) {
        if (disabled_ptr && disabled_ptr->count(force_lower(path_basename(p)))) {
            continue;
        }
        all_good &= (Result::OK == init_dyn_modules(file_access, p, tout));
    }
    // Module .so's may carry DT_NEEDED on sibling-module .so's (e.g. node-editor ->
    // dasModuleImgui) that live in modules/<dep>/ and aren't on RUNPATH. Directory
    // enumeration order is unsorted on Linux (readdir), so a dependent can be visited
    // before its dependency — its register_dynamic_module dlopen then fails and is
    // deferred. Retry the deferred set in fixed-point passes so order stops mattering.
    retry_pending_dynamic_modules();
    // src/ast/ARCHITECTURE.md#module-scan-manifest - still pending with its artifact on disk: an import a deferred row holds
    if ( pending_dynamic_module_artifact_present() ) {
        if ( trace_scan() ) {
            LOG(LogLevel::info) << "[module] a pending module's artifact exists - loading every deferred module\n";
        }
        load_all_deferred_dynamic_modules();
    }
    name_replayed_rows();
    return all_good;
}

// Back-compat overload: the original 5-arg form, no disabled modules.
bool require_dynamic_modules(FileAccessPtr file_access,
                             const das::string &das_root,
                             const das::string &project_root,
                             const das::vector<das::string> &load_modules,
                             das::TextWriter &tout) {
    return require_dynamic_modules(file_access, das_root, project_root, load_modules,
                                   das::vector<das::string>(), tout);
}

}

extern "C" DAS_CC_API void das_require_dynamic_modules () {
    das::TextPrinter tout;
    das::require_dynamic_modules(get_file_access(nullptr), das::getDasRoot(), "", das::vector<das::string>(), tout);
}
