#include "daScript/ast/ast.h"
#include "daScript/misc/platform.h"

#include "module_builtin.h"

#include "daScript/daScriptModule.h"

#include "daScript/simulate/aot_builtin_fio.h"
#include "daScript/simulate/simulate_nodes.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_policy_types.h"
#include "daScript/ast/ast_handle.h"

#include "daScript/misc/performance_time.h"
#include "daScript/misc/sysos.h"
#include "daScript/misc/string_writer.h"   // LOG / LogLevel — env-gated module-load trace
#include "daScript/misc/env_cfg.h"

#include <sstream>
#include <chrono>

#define DAS_POPEN_TIMEOUT 0x7FFFFF01

MAKE_TYPE_FACTORY(clock, das::Time)// use MAKE_TYPE_FACTORY out of namespace. Some compilers not happy otherwise

#if _WIN32
    /* macro definitions extracted from git/git-compat-util.h */
    #define PROT_READ  1
    #define PROT_WRITE 2
    #define MAP_FAILED ((void*)-1)
    /* macro definitions extracted from /usr/include/bits/mman.h */
    #define MAP_SHARED  0x01        /* Share changes.  */
    #define MAP_PRIVATE 0x02        /* Changes are private.  */
    void* mmap(void* start, size_t length, int prot, int flags, int fd, off_t offset);
    int munmap(void* start, size_t length);
    static int getchar_wrapper(void) { return getchar(); } // workaround for non-std callconv (fastcall, vectorcall...)

    #ifdef __clang__
        #define fileno _fileno
        #define getcwd _getcwd
        #define chdir _chdir
    #endif
#else
#define getchar_wrapper getchar
#endif

// The direct sequential writer (dwrite_*): strictly-ascending append for bulk producers that
// already know their total size — measured 2060 MB/s vs 275 MB/s for the same volume through
// buffered fwrite. Cache-bypassing where the platform offers it (FILE_FLAG_NO_BUFFERING on
// Windows, F_NOCACHE on macOS); the generic POSIX arm is buffered write + posix_fadvise
// DONTNEED — the page cache is still touched, just released, so the producer's READS keep it.
// Implemented per-platform at the bottom of this file (same split as the mmap shim above); the
// handle owns an aligned bounce buffer, so callers append any pointer at any size.
void * das_dwrite_open ( const char * path, uint64_t total_bytes, uint64_t band_bytes );
bool das_dwrite_append ( void * h, const void * data, uint64_t bytes );
void * das_dwrite_band ( void * h, uint64_t * avail );
bool das_dwrite_commit ( void * h, uint64_t bytes );
uint64_t das_dwrite_stat ( void * h, int which );
bool das_dwrite_close ( void * h );
bool das_prefetch_map ( void * base, uint64_t bytes );

namespace das {

    struct TimeAnnotation : ManagedValueAnnotation<Time> {
        TimeAnnotation(ModuleLibrary & mlib) : ManagedValueAnnotation<Time>(mlib, "clock","das::Time") {}
        virtual void walk ( DataWalker & walker, void * data ) override {
            if ( walker.reading ) {
                // there shuld be a way to read time from the stream here
            } else {
                Time * t = (Time *) data;
                char mbstr[100];
                if ( strftime(mbstr, sizeof(mbstr), "%c", localtime(&t->time)) ) {
                    char * str = mbstr;
                    walker.String(str);
                }
            }
        }
    };

    Time builtin_clock() {
        Time t;
        t.time = time(nullptr);
        return t;
    }

    // Current UTC wallclock as ISO 8601 with millisecond precision:
    //   "YYYY-MM-DDTHH:MM:SS.mmmZ"
    // Used by daslib/log for log-record timestamps; also general-purpose.
    char * iso8601_now ( Context * context, LineInfoArg * at ) {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
        if ( ms < 0 ) ms += 1000;
        struct tm utc;
#if defined(__linux__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
        gmtime_r(&t, &utc);          // desktop POSIX / wasm
#elif _WIN32
        gmtime_s(&utc, &t);          // MSVC: (tm*, time_t*)
#else
        gmtime_s(&t, &utc);          // consoles (PS4/PS5): POSIX-style gmtime_s(time_t*, tm*)
#endif
        char buf[32];
        int n = snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
            utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
            utc.tm_hour, utc.tm_min, utc.tm_sec, (int)ms);
        return context->allocateString(buf, uint32_t(n < 0 ? 0 : n), at);
    }

    Time builtin_mktime(int year, int month, int mday, int hour, int min, int sec) {
        struct tm timeinfo = {};
        timeinfo.tm_year = year - 1900;
        timeinfo.tm_mon = month - 1;
        timeinfo.tm_mday = mday;
        timeinfo.tm_hour = hour;
        timeinfo.tm_min = min;
        timeinfo.tm_sec = sec;

        Time t;
        t.time = mktime(&timeinfo);
        return t;
    }

    // Render a clock as local-time text via strftime. Empty/null `fmt` -> "%Y-%m-%d %H:%M".
    // Companion to the type's `%c` walk() serialization, but caller-formatted. Uses the
    // reentrant localtime_r/localtime_s into a stack tm (localtime() is not thread-safe and
    // can return null); a failed conversion or empty render yields an empty string.
    char * format_time ( Time t, const char * fmt, Context * context, LineInfoArg * at ) {
        const char * f = ( fmt && *fmt ) ? fmt : "%Y-%m-%d %H:%M";
        struct tm tmv;
        bool ok;
#if defined(__linux__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
        ok = localtime_r(&t.time, &tmv) != nullptr;     // desktop POSIX / wasm
#elif defined(_WIN32)
        ok = localtime_s(&tmv, &t.time) == 0;           // MSVC: (tm*, const time_t*) -> errno_t
#else
        ok = localtime_s(&t.time, &tmv) == 0;           // consoles: POSIX-style (time_t*, tm*)
#endif
        char buf[256];
        size_t n = ok ? strftime(buf, sizeof(buf), f, &tmv) : 0;
        return context->allocateString(n != 0 ? buf : "", uint32_t(n), at);
    }

    void Module_BuiltIn::addTime(ModuleLibrary & lib) {
        addAnnotation(new TimeAnnotation(lib));
        addExtern<DAS_BIND_FUN(builtin_clock)>(*this, lib, "get_clock", SideEffects::modifyExternal, "builtin_clock");
        addExtern<DAS_BIND_FUN(iso8601_now)>(*this, lib, "iso8601_now",
            SideEffects::accessExternal, "iso8601_now")->setTempStringResult();
        addExtern<DAS_BIND_FUN(builtin_mktime)>(*this, lib, "mktime", SideEffects::modifyExternal, "builtin_mktime")
            ->args({"year","month","mday","hour","min","sec"});
        // accessExternal (not none): reads process locale + timezone, so it must not be
        // CSE'd or constant-folded across environment changes — same as iso8601_now.
        addExtern<DAS_BIND_FUN(format_time)>(*this, lib, "format_time", SideEffects::accessExternal, "format_time")
            ->args({"time","format","context","at"})->setTempStringResult();
        // operations on time
        addExtern<DAS_BIND_FUN(time_equal)>(*this, lib, "==",
            SideEffects::none, "time_equal");
        addExtern<DAS_BIND_FUN(time_nequal)>(*this, lib, "!=",
            SideEffects::none, "time_nequal");
        addExtern<DAS_BIND_FUN(time_gtequal)>(*this, lib, ">=",
            SideEffects::none, "time_gtequal");
        addExtern<DAS_BIND_FUN(time_ltequal)>(*this, lib, "<=",
            SideEffects::none, "time_ltequal");
        addExtern<DAS_BIND_FUN(time_gt)>(*this, lib, ">",
            SideEffects::none, "time_gt");
        addExtern<DAS_BIND_FUN(time_lt)>(*this, lib, "<",
            SideEffects::none, "time_lt");
        addExtern<DAS_BIND_FUN(time_sub)>(*this, lib, "-",
            SideEffects::none, "time_sub");
        // TODO: move to upstream das
        addExtern<DAS_BIND_FUN(ref_time_ticks)>(*this, lib, "ref_time_ticks",
            SideEffects::accessExternal, "ref_time_ticks");
        addExtern<DAS_BIND_FUN(get_time_usec)>(*this, lib, "get_time_usec",
            SideEffects::accessExternal, "get_time_usec")->arg("ref");
        addExtern<DAS_BIND_FUN(cast_int64)>(*this, lib, "int64",
            SideEffects::none, "cast_int64")->arg("time");
        addExtern<DAS_BIND_FUN(cast_clock)>(*this, lib, "clock",
            SideEffects::none, "cast_clock")->arg("seconds");
        addExtern<DAS_BIND_FUN(get_time_nsec)>(*this, lib, "get_time_nsec",
            SideEffects::accessExternal, "get_time_nsec")->arg("ref");
    }
}


#if DAS_NO_FILEIO

namespace das {
    #define GENERATE_IO_STUB { context->throw_error_at(at, "%s is not implemented (because DAS_NO_FILEIO is enabled)", __FUNCTION__); }
    void builtin_fprint(const FILE *f, const char *text, Context *context, LineInfoArg *at) GENERATE_IO_STUB
    void builtin_fclose ( const FILE * f, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    void builtin_fflush ( const FILE * f, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    void builtin_map_file(const FILE* f, const TBlock<void, TTemporary<TArray<uint8_t>>>& blk, Context* context, LineInfoArg * at) GENERATE_IO_STUB
    void * builtin_fmap_open ( const char * name, uint64_t * size, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    void * builtin_fmap_open_rw ( const char * name, uint64_t * size, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    void builtin_fmap_close ( void * data, uint64_t size, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    void * builtin_dwrite_open ( const char * name, uint64_t total_bytes, uint64_t band_bytes, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    bool builtin_dwrite_append ( void * h, void * data, uint64_t bytes, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    void * builtin_dwrite_band ( void * h, uint64_t * avail, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    bool builtin_dwrite_commit ( void * h, uint64_t bytes, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    uint64_t builtin_dwrite_stat ( void * h, int32_t which, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    bool builtin_dwrite_close ( void * h, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    bool builtin_prefetch_map ( void * base, uint64_t bytes, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    int64_t builtin_ftell ( const FILE * f, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    int64_t builtin_fseek ( const FILE * f, int64_t offset, int32_t mode, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * builtin_fread ( const FILE * f, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * builtin_fgets(const FILE* f, Context* context, LineInfoArg * at ) GENERATE_IO_STUB
    void builtin_fwrite ( const FILE * f, char * str, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * builtin_dirname ( const char * name, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * builtin_basename ( const char * name, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    bool builtin_fstat ( const FILE * f, FStat & fs, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    void builtin_dir ( const char * path, const Block & fblk, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * builtin_getcwd ( Context * context, LineInfoArg * at) GENERATE_IO_STUB
    int builtin_popen_impl ( const char * cmd, bool bin, const TBlock<void,const FILE *> & blk, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    int builtin_popen_binary ( const char * cmd, const TBlock<void,const FILE *> & blk, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    int builtin_popen ( const char * cmd, const TBlock<void,const FILE *> & blk, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    int builtin_popen_timeout ( const char * cmd, float timeout_sec, const TBlock<void,const FILE *> & blk, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    bool builtin_spawn_argv ( const Array & args_arr, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    int builtin_system ( const char * cmd, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * get_full_file_name ( const char * path, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    bool has_env_variable ( const char * var, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * get_env_variable ( const char * var, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    void set_env_variable ( const char * var, const char * value, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * sanitize_command_line ( const char * cmd, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    // filesystem stubs
    char * builtin_fs_extension ( const char * path, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * builtin_fs_stem ( const char * path, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * builtin_fs_replace_extension ( const char * path, const char * new_ext, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * builtin_fs_join ( const char * a, const char * b, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * builtin_fs_normalize ( const char * path, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * builtin_fs_relative ( const char * path, const char * base, char * & error, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * builtin_fs_parent ( const char * path, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    void builtin_fs_dir_rec ( const char * path, const TBlock<void, char *, bool> & blk, char * & error, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * builtin_fs_temp_directory ( char * & error, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * builtin_fs_create_temp_file ( const char * prefix, const char * ext, char * & error, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * builtin_fs_create_temp_directory ( const char * prefix, char * & error, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    int builtin_popen_argv ( const Array & args_arr, float timeout_sec, const TBlock<void,const FILE *> & blk, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    int builtin_popen_argv_pipe ( const Array & args_arr, const TBlock<void,const FILE *,const FILE *> & blk, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    void * register_dynamic_module_silent ( const char * path, const char * mod_name, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    void for_each_registered_native_path ( const TBlock<void,const char *,const char *,const char *> & block, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    void for_each_registered_dynamic_module ( const TBlock<void,const char *,const char *,const char *> & block, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
#undef GENERATE_IO_STUB

    // entry points that report via `ctx` instead of `context`
#define GENERATE_IO_STUB { ctx->throw_error_at(at, "%s is not implemented (because DAS_NO_FILEIO is enabled)", __FUNCTION__); }
    int64_t builtin_fs_file_size ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB
    bool builtin_fs_equivalent ( const char * a, const char * b, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB
    bool builtin_fs_is_symlink ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB
    bool builtin_fs_copy_file ( const char * src, const char * dst, bool overwrite, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB
    bool builtin_fs_set_mtime ( const char * path, Time time, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB
    bool builtin_fs_disk_space ( const char * path, DiskSpaceInfo & info, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB
    bool builtin_remove_file_ec ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB
    bool builtin_rename_file_ec ( const char * old_path, const char * new_path, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB
    bool builtin_mkdir_ec ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB
    bool builtin_rmdir_ec ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB
    bool builtin_rmdir_rec_ec ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB
#undef GENERATE_IO_STUB

    // no usable context to report on: return an empty result / no-op (never crash)
#define GENERATE_IO_STUB {}
#define GENERATE_IO_STUB_RET { return {}; }
    // vec4f is __m128 on SIMD targets, which has no brace-init - it needs v_zero()
#define GENERATE_IO_STUB_VEC { return v_zero(); }
    void builtin_sleep ( uint32_t ) GENERATE_IO_STUB
    const FILE * builtin_stdin() GENERATE_IO_STUB_RET
    const FILE * builtin_stdout() GENERATE_IO_STUB_RET
    const FILE * builtin_stderr() GENERATE_IO_STUB_RET
    bool builtin_is_terminal ( int32_t ) GENERATE_IO_STUB_RET
    int32_t builtin_terminal_width () GENERATE_IO_STUB_RET
    bool builtin_feof(const FILE*) GENERATE_IO_STUB_RET
    const FILE * builtin_fopen  ( const char *, const char *, Context *, LineInfoArg * ) GENERATE_IO_STUB_RET
    vec4f builtin_read ( Context &, SimNode_CallBase *, vec4f * ) GENERATE_IO_STUB_VEC
    vec4f builtin_write ( Context &, SimNode_CallBase *, vec4f * ) GENERATE_IO_STUB_VEC
    vec4f builtin_read64 ( Context &, SimNode_CallBase *, vec4f * ) GENERATE_IO_STUB_VEC
    vec4f builtin_write64 ( Context &, SimNode_CallBase *, vec4f * ) GENERATE_IO_STUB_VEC
    vec4f builtin_load ( Context &, SimNode_CallBase *, vec4f * ) GENERATE_IO_STUB_VEC
    bool builtin_stat ( const char *, FStat & ) GENERATE_IO_STUB_RET
    bool builtin_chdir ( const char * ) GENERATE_IO_STUB_RET
    bool builtin_mkdir ( const char * ) GENERATE_IO_STUB_RET
    void builtin_exit ( int32_t, Context *, LineInfoArg * ) GENERATE_IO_STUB
    char * builtin_resolve_this_module_dir ( const char *, bool, Context * ) GENERATE_IO_STUB_RET
    bool builtin_remove_file ( const char * ) GENERATE_IO_STUB_RET
    bool builtin_rename_file ( const char *, const char * ) GENERATE_IO_STUB_RET
    bool builtin_rmdir ( const char * ) GENERATE_IO_STUB_RET
    bool builtin_fexist ( const char * ) GENERATE_IO_STUB_RET
    bool builtin_rmdir_rec ( const char * ) GENERATE_IO_STUB_RET
    bool builtin_fs_is_absolute ( const char * ) GENERATE_IO_STUB_RET
    void * register_dynamic_module ( const char *, const char *, int, Context *, LineInfoArg * ) GENERATE_IO_STUB_RET
    void register_native_path ( const char *, const char *, const char *, Context *, LineInfoArg * ) GENERATE_IO_STUB
    DAS_API void retry_pending_dynamic_modules () GENERATE_IO_STUB
    DAS_API string describe_pending_dynamic_modules () GENERATE_IO_STUB_RET
    DAS_API int report_pending_dynamic_modules () GENERATE_IO_STUB_RET

#undef GENERATE_IO_STUB
#undef GENERATE_IO_STUB_RET
#undef GENERATE_IO_STUB_VEC

}
#else // DAS_NO_FILEIO

#include <thread>
#include <atomic>
#include <chrono>
#if _WIN32
#include <fcntl.h>
#include <io.h>             // _isatty
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>         // isatty, STDOUT_FILENO
#include <sys/ioctl.h>      // TIOCGWINSZ
#endif

#include <filesystem>
#include <fstream>
#include <random>

namespace das {
    void builtin_sleep ( uint32_t msec ) {
#if defined(_WIN32)
        _sleep(msec);
#else
        usleep(1000 * msec);
#endif
    }

    void builtin_fprint ( const FILE * f, const char * text, Context * context, LineInfoArg * at ) {
        if ( !f ) context->throw_error_at(at, "can't fprint NULL");
        if ( text ) fputs(text,(FILE *)f);
    }

    static bool is_valid_fopen_mode(const char *mode) {
        return mode && strchr("rwa", mode[0]) && mode[1 + strspn(mode + 1, "+btx")] == '\0';
    }

#if defined(_WIN32)
    static std::wstring utf8_file_path_to_wide ( const char * path ) {
        if ( !path || !*path ) return std::wstring();
        const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            path, -1, nullptr, 0);
        if ( count <= 0 ) return std::wstring();
        std::wstring result(size_t(count), L'\0');
        if ( MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                path, -1, result.data(), count) != count ) {
            return std::wstring();
        }
        result.resize(size_t(count - 1));
        return result;
    }

    static std::string wide_file_path_to_utf8 ( const wchar_t * path ) {
        if ( !path || !*path ) return std::string();
        const int count = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
        if ( count <= 0 ) return std::string();
        std::string result(size_t(count), '\0');
        if ( WideCharToMultiByte(CP_UTF8, 0, path, -1, result.data(), count, nullptr, nullptr) != count ) {
            return std::string();
        }
        result.resize(size_t(count - 1));
        return result;
    }

    static FILE * das_fopen_utf8 ( const char * name, const char * mode ) {
        auto wideName = utf8_file_path_to_wide(name);
        if ( wideName.empty() ) return nullptr;
        wchar_t wideMode[8] = {};
        size_t index = 0;
        while ( mode[index] && index + 1 < sizeof(wideMode) / sizeof(wideMode[0]) ) {
            wideMode[index] = wchar_t(uint8_t(mode[index]));
            ++index;
        }
        return _wfopen(wideName.c_str(), wideMode);
    }
#else
    static FILE * das_fopen_utf8 ( const char * name, const char * mode ) {
        return fopen(name, mode);
    }
#endif

    // das strings are UTF-8 by convention. On Windows both fs::path(char*) and
    // path::string() convert through the ANSI codepage - misreading UTF-8 names on the
    // way in, and throwing system_error on the way out for names the codepage cannot
    // represent. Every std::filesystem boundary in this module goes through this pair.
    // Invalid UTF-8 input yields an empty path (downstream calls fail with a clean ec);
    // un-pairable UTF-16 in on-disk names degrades to U+FFFD instead of throwing.
    static std::filesystem::path das_to_path ( const char * path ) {
#if defined(_WIN32)
        return std::filesystem::path(utf8_file_path_to_wide(path));
#else
        return std::filesystem::path(path ? path : "");
#endif
    }

    static std::string path_to_das ( const std::filesystem::path & p ) {
#if defined(_WIN32)
        return wide_file_path_to_utf8(p.c_str());
#else
        return p.string();
#endif
    }

    const FILE * builtin_fopen  ( const char * name, const char * mode, Context * context, LineInfoArg * at ) {
        if ( !name ) context->throw_error_at(at, "can't fopen NULL name");
        if ( !is_valid_fopen_mode(mode) ) context->throw_error_at(at, "invalid fopen mode '%s'", mode ? mode : "<null>");
        FILE * f = das_fopen_utf8(name, mode);
        if ( f ) setvbuf(f, NULL, _IOFBF, 65536);
        return f;
    }

    void builtin_fclose ( const FILE * f, Context * context, LineInfoArg * at ) {
        if ( !f ) context->throw_error_at(at, "can't fclose NULL");
        fclose((FILE *)f);
    }

    void builtin_fflush ( const FILE * f, Context * context, LineInfoArg * at ) {
        if ( !f ) context->throw_error_at(at, "can't fflush NULL");
        fflush((FILE *)f);
    }

    const FILE * builtin_stdin() {
        return stdin;
    }

    const FILE * builtin_stdout() {
        return stdout;
    }

    const FILE * builtin_stderr() {
        return stderr;
    }

    // fd is 0/1/2 (stdin/stdout/stderr). Whether the stream is a real terminal is the only
    // reliable way to know a `\r`-redrawn progress display is safe — TERM is still set when
    // stdout is a pipe or a log file.
    bool builtin_is_terminal ( int32_t fd ) {
        if ( fd<0 || fd>2 ) return false;
#if defined(_WIN32)
        return _isatty(fd) != 0;
#else
        return isatty(fd) != 0;
#endif
    }

    // Terminal columns, or 0 when unknown (not a terminal, or the query failed). COLUMNS wins
    // when the OS query has nothing, which is what a user overriding it expects.
    int32_t builtin_terminal_width () {
#if defined(_WIN32)
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if ( GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi) ) {
            int32_t w = int32_t(csbi.srWindow.Right) - int32_t(csbi.srWindow.Left) + 1;
            if ( w>0 ) return w;
        }
#else
        struct winsize ws;
        if ( ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)==0 && ws.ws_col>0 ) {
            return int32_t(ws.ws_col);
        }
#endif
        if ( const char * cols = get_columns() ) {
            int32_t w = atoi(cols);
            if ( w>0 ) return w;
        }
        return 0;
    }

    bool builtin_feof(const FILE* _f) {
        FILE* f = (FILE*)_f;
        return feof(f);
    }

    // 64-bit-size stat/fstat (see das_filestat in aot_builtin_fio.h): on Windows the plain
    // stat/fstat truncate st_size to 32 bits and FAIL outright past 2GB.
    static int das_stat64 ( const char * path, das_filestat & st ) {
#if defined(_WIN32)
        auto widePath = utf8_file_path_to_wide(path);
        return widePath.empty() ? -1 : _wstat64(widePath.c_str(), &st);
#else
        return stat(path, &st);
#endif
    }

    static int das_fstat64 ( int fd, das_filestat & st ) {
#if defined(_WIN32)
        return _fstat64(fd, &st);
#else
        return fstat(fd, &st);
#endif
    }

    void builtin_map_file(const FILE* f, const TBlock<void, TTemporary<TArray<uint8_t>>>& blk, Context* context, LineInfoArg * at) {
        if ( !f ) context->throw_error_at(at, "can't map NULL file");
        das_filestat st;
        int fd = fileno((FILE *)f);
        if ( das_fstat64(fd, st) != 0 ) context->throw_error_at(at, "fmap: can't stat file");
        if ( st.st_size == 0 ) {    // empty file: mmap(0) fails (EINVAL) — invoke with an empty array instead
            Array arr;
            array_mark_locked(arr, nullptr, 0);
            vec4f args[1];
            args[0] = cast<Array *>::from(&arr);
            context->invoke(blk, args, nullptr, at);
            return;
        }
        void* data = mmap(nullptr, size_t(st.st_size), PROT_READ, MAP_SHARED, fd, 0);
        if ( data == MAP_FAILED ) context->throw_error_at(at, "fmap: can't map file (%llu bytes)", (unsigned long long)st.st_size);
        Array arr;
        // st.st_size is 64-bit (off_t / __int64); array_mark_locked takes a uint64 size and
        // Array::size is uint64 — a uint32 cast here truncated maps of files >4GB.
        array_mark_locked(arr, data, uint64_t(st.st_size));
        vec4f args[1];
        args[0] = cast<Array *>::from(&arr);
        context->invoke(blk, args, nullptr, at);
        munmap(data, size_t(st.st_size));
    }

    // split fmap (the prepared-model-image loader): the mapping OUTLIVES the call — the caller
    // owns it and unmaps via fmap_close. POSIX maps PROT_READ/MAP_SHARED and the win32 shim
    // below maps PAGE_READONLY/FILE_MAP_READ: OS-enforced read-only on both, clean droppable
    // pages shared across processes through the page cache, and no commit charge. The FILE
    // closes right after mapping — both POSIX and the win32 shim keep the view alive through
    // the mapping's own file reference. Failure returns null (size stays 0) rather than
    // throwing: this is a cache probe — the caller declines and regenerates from the source.
    void * builtin_fmap_open ( const char * name, uint64_t * size, Context * context, LineInfoArg * at ) {
        if ( !size ) context->throw_error_at(at, "fmap_open: null size out-param");
        *size = 0;
        if ( !name ) context->throw_error_at(at, "fmap_open: null path");
        FILE * f = das_fopen_utf8(name, "rb");
        if ( !f ) return nullptr;
        das_filestat st;
        int fd = fileno(f);
        if ( das_fstat64(fd, st) != 0 ) {
            fclose(f);
            return nullptr;
        }
        // empty files can't map (mmap(0) is EINVAL); a size that doesn't survive the size_t
        // round-trip would truncate the mapping on 32-bit targets
        if ( st.st_size == 0 || uint64_t(st.st_size) != uint64_t(size_t(st.st_size)) ) {
            fclose(f);
            return nullptr;
        }
        void * data = mmap(nullptr, size_t(st.st_size), PROT_READ, MAP_SHARED, fd, 0);
        fclose(f);
        if ( data == MAP_FAILED ) return nullptr;
        *size = uint64_t(st.st_size);
        return data;
    }

    // the writable twin: a PAGE_READWRITE / PROT_WRITE SHARED mapping — writes go back to the
    // file through the page cache; still no commit charge (not WRITECOPY). The consumer that
    // wanted it: device APIs that refuse to import read-only pages (VK_EXT_external_memory_host
    // on NVIDIA/Windows). Same ownership contract as fmap_open; fmap_close unmaps either kind.
    void * builtin_fmap_open_rw ( const char * name, uint64_t * size, Context * context, LineInfoArg * at ) {
        if ( !size ) context->throw_error_at(at, "fmap_open_rw: null size out-param");
        *size = 0;
        if ( !name ) context->throw_error_at(at, "fmap_open_rw: null path");
        FILE * f = das_fopen_utf8(name, "r+b");   // the writable section needs write on the handle
        if ( !f ) return nullptr;
        das_filestat st;
        int fd = fileno(f);
        if ( das_fstat64(fd, st) != 0 ) {
            fclose(f);
            return nullptr;
        }
        if ( st.st_size == 0 || uint64_t(st.st_size) != uint64_t(size_t(st.st_size)) ) {
            fclose(f);
            return nullptr;
        }
        void * data = mmap(nullptr, size_t(st.st_size), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        fclose(f);
        if ( data == MAP_FAILED ) return nullptr;
        *size = uint64_t(st.st_size);
        return data;
    }

    void builtin_fmap_close ( void * data, uint64_t size, Context * context, LineInfoArg * at ) {
        if ( !data ) context->throw_error_at(at, "fmap_close: null mapping");
        munmap(data, size_t(size));
    }

    // dwrite: open a file for cache-bypassing sequential append. `total_bytes` preallocates (the
    // file is truncated back to what was actually appended at close), `band_bytes` sizes the
    // internal aligned bounce buffer. Null on failure — the caller falls back or reports.
    void * builtin_dwrite_open ( const char * name, uint64_t total_bytes, uint64_t band_bytes,
                                 Context * context, LineInfoArg * at ) {
        if ( !name ) context->throw_error_at(at, "dwrite_open: null path");
        return das_dwrite_open(name, total_bytes, band_bytes);
    }

    // append `bytes` at the current end. Any pointer, any size — the handle re-blocks into
    // aligned bands internally. false = the write failed (ENOSPC in practice); a failed writer
    // stays failed, so callers can check once at close instead of per append.
    bool builtin_dwrite_append ( void * h, void * data, uint64_t bytes, Context * context, LineInfoArg * at ) {
        if ( !h ) context->throw_error_at(at, "dwrite_append: null writer");
        if ( !data && bytes ) context->throw_error_at(at, "dwrite_append: null data");
        return das_dwrite_append(h, data, bytes);
    }

    // the writer's own staging buffer, for producers that would otherwise build a band and then
    // copy it in. `avail` reports what is left in the current band; fill up to that and commit.
    void * builtin_dwrite_band ( void * h, uint64_t * avail, Context * context, LineInfoArg * at ) {
        if ( !h ) context->throw_error_at(at, "dwrite_band: null writer");
        if ( !avail ) context->throw_error_at(at, "dwrite_band: null avail out-param");
        return das_dwrite_band(h, avail);
    }

    bool builtin_dwrite_commit ( void * h, uint64_t bytes, Context * context, LineInfoArg * at ) {
        if ( !h ) context->throw_error_at(at, "dwrite_commit: null writer");
        return das_dwrite_commit(h, bytes);
    }

    // where the writer spent its time: 0 = staging ns, 1 = syscall ns, 2 = direct bytes,
    // 3 = bounced bytes. Call before dwrite_close — the handle is gone after.
    uint64_t builtin_dwrite_stat ( void * h, int32_t which, Context * context, LineInfoArg * at ) {
        if ( !h ) context->throw_error_at(at, "dwrite_stat: null writer");
        return das_dwrite_stat(h, int(which));
    }

    // flush the tail, truncate to the appended size, close. false if anything along the way
    // failed — the ONLY completion check a caller needs.
    bool builtin_dwrite_close ( void * h, Context * context, LineInfoArg * at ) {
        if ( !h ) context->throw_error_at(at, "dwrite_close: null writer");
        return das_dwrite_close(h);
    }

    // advisory readahead over a mapped range (PrefetchVirtualMemory / madvise WILLNEED) — the
    // cold-conversion fix. false = the OS declined; reads still work, just cold.
    bool builtin_prefetch_map ( void * base, uint64_t bytes, Context * context, LineInfoArg * at ) {
        if ( !base && bytes ) context->throw_error_at(at, "prefetch_map: null base");
        return das_prefetch_map(base, bytes);
    }

    // plain ftell/fseek take `long`, which is 32-bit on Windows (LLP64) — use the explicit
    // 64-bit forms so offsets past 2GB survive on every platform
    int64_t builtin_ftell ( const FILE * f, Context * context, LineInfoArg * at ) {
        if ( !f ) context->throw_error_at(at, "can't ftell NULL");
#ifdef _MSC_VER
        return _ftelli64((FILE *)f);
#else
        return ftello((FILE *)f);
#endif
    }

    int64_t builtin_fseek ( const FILE * f, int64_t offset, int32_t mode, Context * context, LineInfoArg * at ) {
        if ( !f ) context->throw_error_at(at, "can't fseek NULL");
#ifdef _MSC_VER
        return _fseeki64((FILE *)f, offset, mode);
#else
        // off_t can be 32-bit (no _FILE_OFFSET_BITS=64) — a truncated offset would silently
        // seek to the wrong position; fail loudly instead
        if ( int64_t(off_t(offset)) != offset ) {
            context->throw_error_at(at, "fseek: offset %lli does not fit off_t", (long long int)offset);
        }
        return fseeko((FILE *)f, off_t(offset), mode);
#endif
    }

    char * builtin_fread ( const FILE * f, Context * context, LineInfoArg * at ) {
        if ( !f ) context->throw_error_at(at, "can't fread NULL");
        das_filestat st;
        int fd = fileno((FILE*)f);
        if ( das_fstat64(fd, st) != 0 ) context->throw_error_at(at, "fread: can't stat file");
        if ( (st.st_mode & S_IFMT) == S_IFREG && st.st_size > 0 ) {
            // regular file with a known size: one exact-size read
            char * res = context->allocateString(nullptr, st.st_size,at);
            fseek((FILE*)f, 0, SEEK_SET);
            auto bytes = fread(res, 1, st.st_size, (FILE *)f);
            if ( uint64_t(bytes) != uint64_t(st.st_size) ) {
                context->throw_error_at(at, "incorrect fread result, expected %lu, got %lu bytes. read requires binary file mode",
                    (unsigned long)st.st_size, (unsigned long)bytes);
            }
            return res;
        }
        // pipes/sockets/consoles (and /proc-style size-0 regular files): st_size is not a
        // length, so read to EOF with a growing buffer - the stat is already in hand, the
        // detection is one bit-test. 64 KB chunk matches the default kernel pipe capacity,
        // so one syscall typically drains a full pipe buffer.
        string buf;
        constexpr size_t CHUNK = 64 * 1024;
        vector<char> chunk(CHUNK);
        size_t n;
        while ( (n = fread(chunk.data(), 1, CHUNK, (FILE *)f)) > 0 ) {
            buf.append(chunk.data(), n);
        }
        return context->allocateString(buf, at);
    }

    char * builtin_fgets(const FILE* f, Context* context, LineInfoArg * at ) {
        if ( !f ) context->throw_error_at(at, "can't fgets NULL");
        vector<char> buffer(16384);
        if (char* buf = fgets(buffer.data(), int(buffer.size()), (FILE *)f)) {
            return context->allocateString(buf, uint32_t(strlen(buf)),at);
        } else {
            return nullptr;
        }
    }

    void builtin_fwrite ( const FILE * f, char * str, Context * context, LineInfoArg * at ) {
        if ( !f ) context->throw_error_at(at, "can't fprint NULL");
        if (!str) return;
        uint32_t len = stringLength(*context, str);
        if (len) fwrite(str, 1, len, (FILE*)f);
    }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100)
#endif

    vec4f builtin_read ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        DAS_ASSERT ( call->types[1]->isRef() || call->types[1]->isRefType()
            || call->types[1]->type==Type::tString || call->types[1]->type==Type::tPointer);
        auto fp = cast<FILE *>::to(args[0]);
        if ( !fp ) context.throw_error_at(call->debugInfo, "can't read NULL");
        auto buf = cast<void *>::to(args[1]);
        auto len = cast<int32_t>::to(args[2]);
        // a negative count is an int32 wrap upstream; fread would sign-extend it to a huge
        // size_t and stream out of bounds — refuse loudly instead
        if ( len < 0 ) context.throw_error_at(call->debugInfo, "read of negative byte count %i (int32 wrap?) - use the 64-bit rail (long_fread)", len);
        int32_t res = (int32_t) fread(buf,1,len,fp);
        return cast<int32_t>::from(res);
    }

    vec4f builtin_write ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        DAS_ASSERT ( call->types[1]->isRef() || call->types[1]->isRefType()
            || call->types[1]->type==Type::tString || call->types[1]->type==Type::tPointer);
        auto fp = cast<FILE *>::to(args[0]);
        if ( !fp ) context.throw_error_at(call->debugInfo, "can't write NULL");
        auto buf = cast<void *>::to(args[1]);
        auto len = cast<int32_t>::to(args[2]);
        if ( len < 0 ) context.throw_error_at(call->debugInfo, "write of negative byte count %i (int32 wrap?) - use the 64-bit rail (long_fwrite)", len);
        int32_t res = (int32_t) fwrite(buf,1,len,fp);
        return cast<int32_t>::from(res);
    }

    vec4f builtin_read64 ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        DAS_ASSERT ( call->types[1]->isRef() || call->types[1]->isRefType()
            || call->types[1]->type==Type::tString || call->types[1]->type==Type::tPointer);
        auto fp = cast<FILE *>::to(args[0]);
        if ( !fp ) context.throw_error_at(call->debugInfo, "can't read NULL");
        auto buf = cast<void *>::to(args[1]);
        auto len = cast<int64_t>::to(args[2]);
        if ( len < 0 ) context.throw_error_at(call->debugInfo, "read of negative byte count %lli", (long long)len);
        if ( sizeof(size_t) < 8 && uint64_t(len) > uint64_t(SIZE_MAX) ) context.throw_error_at(call->debugInfo, "read of %lli bytes exceeds this platform's address space", (long long)len);
        int64_t res = (int64_t) fread(buf,1,size_t(len),fp);
        return cast<int64_t>::from(res);
    }

    vec4f builtin_write64 ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        DAS_ASSERT ( call->types[1]->isRef() || call->types[1]->isRefType()
            || call->types[1]->type==Type::tString || call->types[1]->type==Type::tPointer);
        auto fp = cast<FILE *>::to(args[0]);
        if ( !fp ) context.throw_error_at(call->debugInfo, "can't write NULL");
        auto buf = cast<void *>::to(args[1]);
        auto len = cast<int64_t>::to(args[2]);
        if ( len < 0 ) context.throw_error_at(call->debugInfo, "write of negative byte count %lli", (long long)len);
        if ( sizeof(size_t) < 8 && uint64_t(len) > uint64_t(SIZE_MAX) ) context.throw_error_at(call->debugInfo, "write of %lli bytes exceeds this platform's address space", (long long)len);
        int64_t res = (int64_t) fwrite(buf,1,size_t(len),fp);
        return cast<int64_t>::from(res);
    }

#ifdef _MSC_VER
#pragma warning(pop)
#endif

    // loads(file,block<data>)
    vec4f builtin_load ( Context & context, SimNode_CallBase * node, vec4f * args ) {
        auto fp = cast<FILE *>::to(args[0]);
        if ( !fp ) context.throw_error_at(node->debugInfo, "can't load NULL");
        int64_t len = cast<int64_t>::to(args[1]);
        if (len < 0) context.throw_error_at(node->debugInfo, "can't read negative number from binary save, %lld", (long long)len);
        // On 32-bit (size_t < int64), len near SIZE_MAX truncates and `size_t(len)+1` can wrap to 0,
        // giving a tiny malloc + a huge fread (OOB write). Reject loads that don't fit size_t. No-op on 64-bit.
        if ( uint64_t(len) >= uint64_t(SIZE_MAX) ) context.throw_error_at(node->debugInfo, "can't read. file too large to load on this platform, %lld", (long long)len);
        Block * block = cast<Block *>::to(args[2]);
        char * buf = (char *) malloc(size_t(len) + 1);
        if (!buf) context.throw_error_at(node->debugInfo, "can't read. out of memory, %lld", (long long)len);
        vec4f bargs[1];
        int64_t rlen = int64_t(fread(buf, 1, size_t(len), fp));
        if ( rlen != len ) {
            bargs[0] = v_zero();
            context.invoke(*block, bargs, nullptr, &node->debugInfo);
        }  else {
            buf[rlen] = 0;
            das::Array arr;
            array_mark_locked(arr, buf, uint64_t(rlen));
            bargs[0] = cast<das::Array*>::from(&arr);
            context.invoke(*block, bargs, nullptr, &node->debugInfo);
        }
        free(buf);
        return v_zero();
    }


    char * builtin_dirname ( const char * name, Context * context, LineInfoArg * at ) {
        if ( name ) {
#if defined(_WIN32)
            char full_path[ _MAX_PATH ];
            char dir[ _MAX_DIR ];
            char fname[ _MAX_FNAME ];
            char ext[ _MAX_EXT ];
            _splitpath(name, full_path, dir, fname, ext);
            strcat(full_path, dir);
            uint32_t len = uint32_t(strlen(full_path));
            if (len) {
                if (full_path[len - 1] == '/' || full_path[len - 1] == '\\') {
                    full_path[--len] = 0;
                }
            }
            return context->allocateString(full_path, len, at);
#else
            char * tempName = strdup(name);
            char * dirName = dirname(tempName);
            char * result = context->allocateString(dirName, strlen(dirName), at);
            free(tempName);
            return result;
#endif
        } else {
            return nullptr;
        }
    }

    char * builtin_basename ( const char * name, Context * context, LineInfoArg * at ) {
        if ( name ) {
#if defined(_WIN32)
            char drive[ _MAX_DRIVE ];
            char full_path[ _MAX_PATH ];
            char dir[ _MAX_DIR ];
            char ext[ _MAX_EXT ];
            _splitpath(name, drive, dir, full_path, ext);
            strcat(full_path, ext);
            return context->allocateString(full_path, uint32_t(strlen(full_path)), at);
#else
            char * tempName = strdup(name);
            char * dirName = basename(tempName);
            char * result = context->allocateString(dirName, strlen(dirName), at);
            free(tempName);
            return result;
#endif
        } else {
            return nullptr;
        }
    }

    bool builtin_fstat ( const FILE * f, FStat & fs, Context * context, LineInfoArg * at ) {
        if ( !f ) context->throw_error_at(at, "fstat of null");
        return das_fstat64(fileno((FILE *)f), fs.stats) == 0;
    }

    bool builtin_stat ( const char * filename, FStat & fs ) {
        if ( filename!=nullptr ) {
            return das_stat64(filename, fs.stats) == 0;
        } else {
            return false;
        }
    }

     void builtin_dir ( const char * path, const Block & fblk, Context * context, LineInfoArg * at ) {
#if defined(_WIN32)
        _wfinddata_t c_file;
        intptr_t hFile;
        auto findPath = utf8_file_path_to_wide((string(path ? path : "") + "/*").c_str());
        if (!findPath.empty() && (hFile = _wfindfirst(findPath.c_str(), &c_file)) != -1L) {
            do {
                auto name8 = wide_file_path_to_utf8(c_file.name);
                char * fname = context->allocateString(name8.data(), uint32_t(name8.size()),at);
                vec4f args[1] = {
                    cast<char *>::from(fname)
                };
                context->invoke(fblk, args, nullptr, at);
            } while (_wfindnext(hFile, &c_file) == 0);
            _findclose(hFile);
        }
 #else
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir (path ? path : "")) != NULL) {
            while ((ent = readdir (dir)) != NULL) {
                char * fname = context->allocateString(ent->d_name,uint32_t(strlen(ent->d_name)),at);
                vec4f args[1] = {
                    cast<char *>::from(fname)
                };
                context->invoke(fblk, args, nullptr, at);
            }
            closedir (dir);
        }
 #endif
    }

    bool builtin_chdir ( const char * path ) {
#if defined(_EMSCRIPTEN_VER)
        return false;
#else
        if ( path ) {
#if defined(_WIN32)
            auto widePath = utf8_file_path_to_wide(path);
            return !widePath.empty() && _wchdir(widePath.c_str()) == 0;
#else
            return chdir(path) == 0;
#endif
        } else {
            return false;
        }
#endif
    }

    char * builtin_getcwd ( Context * context, LineInfoArg * at) {
#if defined(_EMSCRIPTEN_VER)
        return nullptr;
#else
#if defined(_WIN32)
        wchar_t * buf = _wgetcwd(nullptr, 0);
        if ( buf ) {
            auto cwd8 = wide_file_path_to_utf8(buf);
            free(buf);
            return context->allocateString(cwd8.data(), uint32_t(cwd8.size()), at);
        } else {
            return nullptr;
        }
#else
        char * buf = getcwd(nullptr, 0);
        if ( buf ) {
            char * res = context->allocateString(buf, uint32_t(strlen(buf)), at);
            free(buf);
            return res;
        } else {
            return nullptr;
        }
#endif
#endif
    }

    bool builtin_mkdir ( const char * path ) {
        if ( path ) {
#if defined(_WIN32)
            auto widePath = utf8_file_path_to_wide(path);
            return !widePath.empty() && _wmkdir(widePath.c_str()) == 0;
#elif defined(_EMSCRIPTEN_VER)
            return mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
#else
            return mkdir(path, ACCESSPERMS) == 0;
#endif
        } else {
            return false;
        }
    }

    void builtin_exit ( int32_t ec, Context * context, LineInfoArg * at ) {
        // A script calling exit(1) produced a bare non-zero exit with no message, no stack and no
        // EXCEPTION -- indistinguishable from every other silent failure. A non-zero exit is a
        // failure worth naming; exit(0) is a normal shutdown and stays quiet.
        if ( ec != 0 ) {
            TextPrinter tp;
            tp << "exit(" << ec << ") called from " << (at ? at->describe().c_str() : "<unknown>") << "\n";
            tp.output();
            if ( context ) context->stackWalk(at, false, false);
        }
        exit(ec);
    }

    int builtin_popen_impl ( const char * cmd, bool bin, const TBlock<void,const FILE *> & blk, Context * context, LineInfoArg * at ) {
        if ( !cmd ) {
            context->throw_error_at(at, "popen of null");
            return -1;
        }
#ifdef _WIN32
        FILE * f = _popen(cmd, bin ? "rb" : "rt");
#elif defined(__linux__)
        FILE * f = popen(cmd, "r");
#elif defined(__APPLE__)
        FILE * f = nullptr;
        static mutex mtx;
        {
            // `popen` sometimes returns 127 on OSX when executed in parallel.
            // Related: https://github.com/microsoft/vcpkg-tool/pull/695#discussion_r973364608
            lock_guard<mutex> lock(mtx);
            f = popen(cmd, "r+");
        }
#else
        FILE * f = popen(cmd, "r+");
#endif
        vec4f args[1];
        args[0] = cast<FILE *>::from(f);
        context->invoke(blk, args, nullptr, at);
#ifdef _WIN32
        return _pclose( f );
#elif defined(__APPLE__)
        {
            lock_guard<mutex> lock(mtx);
            auto t = pclose(f);
            return WIFEXITED(t) ? WEXITSTATUS(t) : WIFSIGNALED(t) ? WTERMSIG(t) : t;
        }
#else
        auto t = pclose(f);
        return WIFEXITED(t) ? WEXITSTATUS(t) : WIFSIGNALED(t) ? WTERMSIG(t) : t;
#endif
    }

    int builtin_popen_binary ( const char * cmd, const TBlock<void,const FILE *> & blk, Context * context, LineInfoArg * at ) {
        return builtin_popen_impl(cmd, true, blk, context, at);
    }

    int builtin_popen ( const char * cmd, const TBlock<void,const FILE *> & blk, Context * context, LineInfoArg * at ) {
        return builtin_popen_impl(cmd, false, blk, context, at);
    }

    int builtin_popen_timeout ( const char * cmd, float timeout_sec, const TBlock<void,const FILE *> & blk, Context * context, LineInfoArg * at ) {
        if ( !cmd ) {
            context->throw_error_at(at, "popen of null");
            return -1;
        }
        if ( timeout_sec <= 0.0f ) {
            return builtin_popen_impl(cmd, false, blk, context, at);
        }
#ifdef _WIN32
        // Windows: CreateProcess with stdout pipe + job object for process tree kill
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;
        HANDLE hReadPipe = NULL, hWritePipe = NULL;
        if ( !CreatePipe(&hReadPipe, &hWritePipe, &sa, 0) ) {
            vec4f args[1]; args[0] = cast<FILE *>::from(nullptr);
            context->invoke(blk, args, nullptr, at);
            return -1;
        }
        SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);
        // Create job object to kill entire process tree on timeout.
        // BREAKAWAY_OK lets a descendant that explicitly requests
        // CREATE_BREAKAWAY_FROM_JOB (spawn_detached) opt OUT of the subtree;
        // everything that doesn't ask still dies with the job.
        HANDLE hJob = CreateJobObjectA(NULL, NULL);
        if ( hJob ) {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli;
            memset(&jeli, 0, sizeof(jeli));
            jeli.BasicLimitInformation.LimitFlags =
                JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_BREAKAWAY_OK;
            SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
        }
        STARTUPINFOA si;
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        si.hStdOutput = hWritePipe;
        si.hStdError = hWritePipe;
        si.dwFlags = STARTF_USESTDHANDLES;
        PROCESS_INFORMATION pi;
        memset(&pi, 0, sizeof(pi));
        string cmdLine = string("cmd.exe /c ") + cmd;
        BOOL created = CreateProcessA(NULL, (LPSTR)cmdLine.c_str(), NULL, NULL, TRUE,
            CREATE_NO_WINDOW | CREATE_SUSPENDED, NULL, NULL, &si, &pi);
        CloseHandle(hWritePipe);
        if ( !created ) {
            CloseHandle(hReadPipe);
            if ( hJob ) CloseHandle(hJob);
            vec4f args[1]; args[0] = cast<FILE *>::from(nullptr);
            context->invoke(blk, args, nullptr, at);
            return -1;
        }
        if ( hJob ) AssignProcessToJobObject(hJob, pi.hProcess);
        ResumeThread(pi.hThread);
        CloseHandle(pi.hThread);
        int fd = _open_osfhandle((intptr_t)hReadPipe, _O_RDONLY | _O_TEXT);
        FILE * f = _fdopen(fd, "rt");
        // Watchdog thread: wait for timeout, then kill entire process tree via job object
        HANDLE hProcess = pi.hProcess;
        DWORD timeout_ms = (DWORD)(timeout_sec * 1000.0f);
        atomic<bool> timedOut{false};
        thread watchdog([hProcess, hJob, timeout_ms, &timedOut]() {
            if ( WaitForSingleObject(hProcess, timeout_ms) == WAIT_TIMEOUT ) {
                timedOut = true;
                if ( hJob ) {
                    TerminateJobObject(hJob, 1);
                } else {
                    TerminateProcess(hProcess, 1);
                }
            }
        });
        vec4f args[1];
        args[0] = cast<FILE *>::from(f);
        context->invoke(blk, args, nullptr, at);
        WaitForSingleObject(hProcess, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeProcess(hProcess, &exitCode);
        fclose(f);
        watchdog.join();
        CloseHandle(hProcess);
        if ( hJob ) CloseHandle(hJob);
        return timedOut ? DAS_POPEN_TIMEOUT : (int)exitCode;
#else
        // Unix: fork/exec with pipe + watchdog thread
        int pipefd[2];
        if ( pipe(pipefd) == -1 ) {
            vec4f args[1]; args[0] = cast<FILE *>::from(nullptr);
            context->invoke(blk, args, nullptr, at);
            return -1;
        }
        pid_t pid = fork();
        if ( pid == -1 ) {
            close(pipefd[0]);
            close(pipefd[1]);
            vec4f args[1]; args[0] = cast<FILE *>::from(nullptr);
            context->invoke(blk, args, nullptr, at);
            return -1;
        }
        if ( pid == 0 ) {
            // Child process
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);
            execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
            _exit(127);
        }
        // Parent process
        close(pipefd[1]);
        FILE * f = fdopen(pipefd[0], "r");
        atomic<bool> timedOut{false};
        atomic<bool> processDone{false};
        thread watchdog([pid, timeout_sec, &timedOut, &processDone]() {
            auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds((int)(timeout_sec * 1000.0f));
            while ( std::chrono::steady_clock::now() < deadline ) {
                if ( processDone.load() ) return;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            if ( !processDone.load() ) {
                timedOut = true;
                kill(pid, SIGKILL);
            }
        });
        vec4f args[1];
        args[0] = cast<FILE *>::from(f);
        context->invoke(blk, args, nullptr, at);
        int status = 0;
        waitpid(pid, &status, 0);
        processDone = true;
        fclose(f);
        watchdog.join();
        if ( timedOut ) return DAS_POPEN_TIMEOUT;
        return WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? WTERMSIG(status) : status;
#endif
    }

#ifdef _WIN32
    // Quote a single argv element for Windows CommandLineToArgvW parsing.
    // Wraps in double quotes if needed, doubles backslashes that precede a
    // quote (or end-of-string inside a quoted token), and escapes embedded
    // quotes with backslash. Matches the documented MSVCRT / CommandLineToArgvW
    // algorithm (https://learn.microsoft.com/en-us/cpp/cpp/main-function-command-line-args).
    static string winArgvEscape ( const char * arg ) {
        if ( !arg ) return "\"\"";
        string a = arg;
        if ( !a.empty() && a.find_first_of(" \t\n\v\"") == string::npos ) {
            return a;
        }
        string out = "\"";
        for ( size_t i = 0; i < a.size(); ) {
            size_t backslashes = 0;
            while ( i < a.size() && a[i] == '\\' ) { ++backslashes; ++i; }
            if ( i == a.size() ) {
                out.append(backslashes * 2, '\\');
            } else if ( a[i] == '"' ) {
                out.append(backslashes * 2 + 1, '\\');
                out += '"';
                ++i;
            } else {
                out.append(backslashes, '\\');
                out += a[i];
                ++i;
            }
        }
        out += '"';
        return out;
    }

    static string winBuildCommandLine ( char ** argv, uint64_t argc ) {
        string s;
        for ( uint64_t i = 0; i < argc; ++i ) {
            if ( i ) s += ' ';
            s += winArgvEscape(argv[i]);
        }
        return s;
    }
#endif

    // Launch an argv-based subprocess and return immediately. The child
    // inherits cwd, environment, and stdout/stderr; stdin is detached so it
    // cannot consume the parent's input stream. No shell is involved.
    bool builtin_spawn_argv ( const Array & args_arr, Context * context, LineInfoArg * at ) {
        if ( args_arr.size == 0 ) {
            context->throw_error_at(at, "spawn_argv with empty args");
            return false;
        }
        char ** argv = (char **) args_arr.data;
        if ( !argv[0] ) {
            context->throw_error_at(at, "spawn_argv with null exe");
            return false;
        }
#ifdef _WIN32
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;
        HANDLE hNullInput = CreateFileA("NUL", GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        auto duplicateOutput = [&sa](DWORD stdHandle) -> HANDLE {
            HANDLE source = GetStdHandle(stdHandle);
            HANDLE result = INVALID_HANDLE_VALUE;
            if ( source && source != INVALID_HANDLE_VALUE
                    && DuplicateHandle(GetCurrentProcess(), source,
                        GetCurrentProcess(), &result, 0, TRUE,
                        DUPLICATE_SAME_ACCESS) ) {
                return result;
            }
            return CreateFileA("NUL", GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        };
        HANDLE hOutput = duplicateOutput(STD_OUTPUT_HANDLE);
        HANDLE hError = duplicateOutput(STD_ERROR_HANDLE);
        if ( hNullInput == INVALID_HANDLE_VALUE
                || hOutput == INVALID_HANDLE_VALUE || hError == INVALID_HANDLE_VALUE ) {
            if ( hNullInput != INVALID_HANDLE_VALUE ) CloseHandle(hNullInput);
            if ( hOutput != INVALID_HANDLE_VALUE ) CloseHandle(hOutput);
            if ( hError != INVALID_HANDLE_VALUE ) CloseHandle(hError);
            return false;
        }
        STARTUPINFOEXA si;
        memset(&si, 0, sizeof(si));
        si.StartupInfo.cb = sizeof(si);
        si.StartupInfo.hStdInput = hNullInput;
        si.StartupInfo.hStdOutput = hOutput;
        si.StartupInfo.hStdError = hError;
        si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
        SIZE_T attributeBytes = 0;
        InitializeProcThreadAttributeList(NULL, 1, 0, &attributeBytes);
        vector<uint8_t> attributeStorage(attributeBytes);
        si.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
            attributeStorage.data());
        HANDLE inheritedHandles[] = { hNullInput, hOutput, hError };
        bool attributeListInitialized = InitializeProcThreadAttributeList(
            si.lpAttributeList, 1, 0, &attributeBytes);
        bool attributesReady = attributeListInitialized
            && UpdateProcThreadAttribute(si.lpAttributeList, 0,
                PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inheritedHandles,
                sizeof(inheritedHandles), NULL, NULL);
        PROCESS_INFORMATION pi;
        memset(&pi, 0, sizeof(pi));
        string cmdLine = winBuildCommandLine(argv, args_arr.size);
        BOOL created = attributesReady && CreateProcessA(NULL,
            (LPSTR)cmdLine.c_str(), NULL, NULL, TRUE,
            CREATE_NEW_PROCESS_GROUP | EXTENDED_STARTUPINFO_PRESENT,
            NULL, NULL, &si.StartupInfo, &pi);
        if ( attributeListInitialized ) {
            DeleteProcThreadAttributeList(si.lpAttributeList);
        }
        CloseHandle(hNullInput);
        CloseHandle(hOutput);
        CloseHandle(hError);
        if ( !created ) return false;
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
#else
        vector<char *> cargv;
        cargv.reserve(args_arr.size + 1);
        for ( uint64_t i = 0; i < args_arr.size; ++i ) {
            cargv.push_back(argv[i] ? argv[i] : (char *)"");
        }
        cargv.push_back(nullptr);
        int execError[2];
        if ( pipe(execError) == -1 ) return false;
        int descriptorFlags = fcntl(execError[1], F_GETFD);
        if ( descriptorFlags == -1
                || fcntl(execError[1], F_SETFD, descriptorFlags | FD_CLOEXEC) == -1 ) {
            close(execError[0]);
            close(execError[1]);
            return false;
        }
        pid_t launcher = fork();
        if ( launcher == -1 ) {
            close(execError[0]);
            close(execError[1]);
            return false;
        }
        if ( launcher == 0 ) {
            close(execError[0]);
            if ( setsid() == -1 ) {
                int error = errno;
                const ssize_t written = write(execError[1], &error, sizeof(error));
                (void)written;
                _exit(127);
            }
            pid_t child = fork();
            if ( child == -1 ) {
                int error = errno;
                const ssize_t written = write(execError[1], &error, sizeof(error));
                (void)written;
                _exit(127);
            }
            if ( child > 0 ) {
                close(execError[1]);
                _exit(0);
            }
            int devnull = open("/dev/null", O_RDONLY);
            if ( devnull >= 0 ) {
                dup2(devnull, STDIN_FILENO);
                close(devnull);
            }
            execvp(cargv[0], cargv.data());
            int error = errno;
            const ssize_t written = write(execError[1], &error, sizeof(error));
            (void)written;
            _exit(127);
        }
        close(execError[1]);
        // Reap only the short-lived launcher. Its grandchild is adopted by
        // init and remains independent of the caller without becoming a
        // zombie in this process.
        int status = 0;
        while ( waitpid(launcher, &status, 0) == -1 ) {
            if ( errno != EINTR ) {
                close(execError[0]);
                return false;
            }
        }
        int childError = 0;
        ssize_t errorBytes;
        do {
            errorBytes = read(execError[0], &childError, sizeof(childError));
        } while ( errorBytes == -1 && errno == EINTR );
        close(execError[0]);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0 && errorBytes == 0;
#endif
    }

    // popen_argv: argv-based subprocess. Bypasses the shell entirely -- on
    // Windows, CreateProcess invokes the .exe directly (no cmd.exe, no
    // first-quote-stripping); on Unix, fork+execvp (no /bin/sh, no $() /
    // backtick expansion). timeout_sec <= 0 means no timeout.
    int builtin_popen_argv ( const Array & args_arr, float timeout_sec,
                             const TBlock<void,const FILE *> & blk,
                             Context * context, LineInfoArg * at ) {
        if ( args_arr.size == 0 ) {
            context->throw_error_at(at, "popen_argv with empty args");
            return -1;
        }
        char ** argv = (char **) args_arr.data;
        if ( !argv[0] ) {
            context->throw_error_at(at, "popen_argv with null exe");
            return -1;
        }
#ifdef _WIN32
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;
        HANDLE hReadPipe = NULL, hWritePipe = NULL;
        if ( !CreatePipe(&hReadPipe, &hWritePipe, &sa, 0) ) {
            vec4f cargs[1]; cargs[0] = cast<FILE *>::from(nullptr);
            context->invoke(blk, cargs, nullptr, at);
            return -1;
        }
        SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);
        HANDLE hJob = NULL;
        if ( timeout_sec > 0.0f ) {
            hJob = CreateJobObjectA(NULL, NULL);
            if ( hJob ) {
                JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli;
                memset(&jeli, 0, sizeof(jeli));
                // BREAKAWAY_OK: a descendant that explicitly requests
                // CREATE_BREAKAWAY_FROM_JOB (spawn_detached) may leave the
                // kill-on-close subtree; everything else still dies with it.
                jeli.BasicLimitInformation.LimitFlags =
                    JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_BREAKAWAY_OK;
                SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
            }
        }
        // Isolate child stdin from the parent's. Without this, the child
        // inherits the parent's stdin handle (via bInheritHandles=TRUE), and
        // any accidental read in the child silently steals bytes intended
        // for the parent — fatal for MCP / language-server style stdio
        // transports. NUL reads return EOF immediately.
        HANDLE hNullInput = CreateFileA("NUL", GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        STARTUPINFOA si;
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        si.hStdInput = (hNullInput == INVALID_HANDLE_VALUE) ? NULL : hNullInput;
        si.hStdOutput = hWritePipe;
        si.hStdError = hWritePipe;
        si.dwFlags = STARTF_USESTDHANDLES;
        PROCESS_INFORMATION pi;
        memset(&pi, 0, sizeof(pi));
        // CreateProcess wants a writable buffer for lpCommandLine.
        string cmdLine = winBuildCommandLine(argv, args_arr.size);
        DWORD createFlags = CREATE_NO_WINDOW;
        if ( timeout_sec > 0.0f ) createFlags |= CREATE_SUSPENDED;
        BOOL created = CreateProcessA(NULL, (LPSTR)cmdLine.c_str(), NULL, NULL, TRUE,
            createFlags, NULL, NULL, &si, &pi);
        CloseHandle(hWritePipe);
        if ( hNullInput != INVALID_HANDLE_VALUE ) CloseHandle(hNullInput);
        if ( !created ) {
            CloseHandle(hReadPipe);
            if ( hJob ) CloseHandle(hJob);
            vec4f cargs[1]; cargs[0] = cast<FILE *>::from(nullptr);
            context->invoke(blk, cargs, nullptr, at);
            return -1;
        }
        if ( timeout_sec > 0.0f ) {
            if ( hJob ) AssignProcessToJobObject(hJob, pi.hProcess);
            ResumeThread(pi.hThread);
        }
        CloseHandle(pi.hThread);
        int fd = _open_osfhandle((intptr_t)hReadPipe, _O_RDONLY | _O_TEXT);
        FILE * f = _fdopen(fd, "rt");
        HANDLE hProcess = pi.hProcess;
        atomic<bool> timedOut{false};
        thread watchdog;
        if ( timeout_sec > 0.0f ) {
            DWORD timeout_ms = (DWORD)(timeout_sec * 1000.0f);
            watchdog = thread([hProcess, hJob, timeout_ms, &timedOut]() {
                if ( WaitForSingleObject(hProcess, timeout_ms) == WAIT_TIMEOUT ) {
                    timedOut = true;
                    if ( hJob ) {
                        TerminateJobObject(hJob, 1);
                    } else {
                        TerminateProcess(hProcess, 1);
                    }
                }
            });
        }
        vec4f cargs[1];
        cargs[0] = cast<FILE *>::from(f);
        context->invoke(blk, cargs, nullptr, at);
        WaitForSingleObject(hProcess, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeProcess(hProcess, &exitCode);
        fclose(f);
        if ( watchdog.joinable() ) watchdog.join();
        CloseHandle(hProcess);
        if ( hJob ) CloseHandle(hJob);
        return timedOut ? DAS_POPEN_TIMEOUT : (int)exitCode;
#else
        int pipefd[2];
        if ( pipe(pipefd) == -1 ) {
            vec4f cargs[1]; cargs[0] = cast<FILE *>::from(nullptr);
            context->invoke(blk, cargs, nullptr, at);
            return -1;
        }
        // Build a NULL-terminated argv copy for execvp. We can't pass
        // args_arr.data directly because daslang doesn't guarantee a
        // trailing NULL element.
        vector<char *> cargv;
        cargv.reserve(args_arr.size + 1);
        for ( uint64_t i = 0; i < args_arr.size; ++i ) {
            cargv.push_back(argv[i] ? argv[i] : (char *)"");
        }
        cargv.push_back(nullptr);
        pid_t pid = fork();
        if ( pid == -1 ) {
            close(pipefd[0]);
            close(pipefd[1]);
            vec4f cargs[1]; cargs[0] = cast<FILE *>::from(nullptr);
            context->invoke(blk, cargs, nullptr, at);
            return -1;
        }
        if ( pid == 0 ) {
            close(pipefd[0]);
            // Isolate child stdin from the parent's: redirect to /dev/null so
            // any accidental read in the child returns EOF instead of stealing
            // bytes intended for the parent (fatal for MCP / language-server
            // style stdio transports).
            int devnull = open("/dev/null", O_RDONLY);
            if ( devnull >= 0 ) {
                dup2(devnull, STDIN_FILENO);
                close(devnull);
            }
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);
            execvp(cargv[0], cargv.data());
            _exit(127);
        }
        close(pipefd[1]);
        FILE * f = fdopen(pipefd[0], "r");
        atomic<bool> timedOut{false};
        atomic<bool> processDone{false};
        thread watchdog;
        if ( timeout_sec > 0.0f ) {
            watchdog = thread([pid, timeout_sec, &timedOut, &processDone]() {
                auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds((int)(timeout_sec * 1000.0f));
                while ( std::chrono::steady_clock::now() < deadline ) {
                    if ( processDone.load() ) return;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                if ( !processDone.load() ) {
                    timedOut = true;
                    kill(pid, SIGKILL);
                }
            });
        }
        vec4f cargs[1];
        cargs[0] = cast<FILE *>::from(f);
        context->invoke(blk, cargs, nullptr, at);
        int status = 0;
        waitpid(pid, &status, 0);
        processDone = true;
        fclose(f);
        if ( watchdog.joinable() ) watchdog.join();
        if ( timedOut ) return DAS_POPEN_TIMEOUT;
        return WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? WTERMSIG(status) : status;
#endif
    }

    // popen_argv_pipe: argv-based subprocess with bidirectional pipes.
    // Block receives two FILE*: writable stdin (parent → child) and readable
    // stdout (child → parent, with stderr merged). After the block returns,
    // parent closes stdin (signaling EOF), waits for the child to exit,
    // closes stdout, and returns the exit code. Same shell-bypass semantics
    // as popen_argv (CreateProcess / fork+execvp).
    int builtin_popen_argv_pipe ( const Array & args_arr,
                                  const TBlock<void,const FILE *,const FILE *> & blk,
                                  Context * context, LineInfoArg * at ) {
        if ( args_arr.size == 0 ) {
            context->throw_error_at(at, "popen_argv_pipe with empty args");
            return -1;
        }
        char ** argv = (char **) args_arr.data;
        if ( !argv[0] ) {
            context->throw_error_at(at, "popen_argv_pipe with null exe");
            return -1;
        }
#ifdef _WIN32
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;
        HANDLE hStdinR = NULL, hStdinW = NULL;
        HANDLE hStdoutR = NULL, hStdoutW = NULL;
        if ( !CreatePipe(&hStdinR, &hStdinW, &sa, 0) ) {
            vec4f cargs[2];
            cargs[0] = cast<FILE *>::from(nullptr);
            cargs[1] = cast<FILE *>::from(nullptr);
            context->invoke(blk, cargs, nullptr, at);
            return -1;
        }
        if ( !CreatePipe(&hStdoutR, &hStdoutW, &sa, 0) ) {
            CloseHandle(hStdinR); CloseHandle(hStdinW);
            vec4f cargs[2];
            cargs[0] = cast<FILE *>::from(nullptr);
            cargs[1] = cast<FILE *>::from(nullptr);
            context->invoke(blk, cargs, nullptr, at);
            return -1;
        }
        // Parent ends must not be inherited by the child.
        SetHandleInformation(hStdinW, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(hStdoutR, HANDLE_FLAG_INHERIT, 0);
        STARTUPINFOA si;
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        si.hStdInput = hStdinR;
        si.hStdOutput = hStdoutW;
        si.hStdError = hStdoutW;
        si.dwFlags = STARTF_USESTDHANDLES;
        PROCESS_INFORMATION pi;
        memset(&pi, 0, sizeof(pi));
        string cmdLine = winBuildCommandLine(argv, args_arr.size);
        BOOL created = CreateProcessA(NULL, (LPSTR)cmdLine.c_str(), NULL, NULL, TRUE,
            CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
        // Close child-side pipe ends in parent.
        CloseHandle(hStdinR);
        CloseHandle(hStdoutW);
        if ( !created ) {
            CloseHandle(hStdinW);
            CloseHandle(hStdoutR);
            vec4f cargs[2];
            cargs[0] = cast<FILE *>::from(nullptr);
            cargs[1] = cast<FILE *>::from(nullptr);
            context->invoke(blk, cargs, nullptr, at);
            return -1;
        }
        CloseHandle(pi.hThread);
        // Wrap each pipe end in a FILE*. Check both fd and FILE* — if either
        // conversion fails, close any raw handles/fds we still own to avoid
        // leaking the child's stdin write end (would deadlock waitpid since
        // child never sees EOF).
        int stdinFd  = _open_osfhandle((intptr_t)hStdinW,  _O_WRONLY | _O_BINARY);
        if ( stdinFd == -1 ) CloseHandle(hStdinW);
        int stdoutFd = _open_osfhandle((intptr_t)hStdoutR, _O_RDONLY | _O_BINARY);
        if ( stdoutFd == -1 ) CloseHandle(hStdoutR);
        FILE * fStdin  = stdinFd  != -1 ? _fdopen(stdinFd,  "wb") : nullptr;
        if ( !fStdin  && stdinFd  != -1 ) _close(stdinFd);
        FILE * fStdout = stdoutFd != -1 ? _fdopen(stdoutFd, "rb") : nullptr;
        if ( !fStdout && stdoutFd != -1 ) _close(stdoutFd);
        vec4f cargs[2];
        cargs[0] = cast<FILE *>::from(fStdin);
        cargs[1] = cast<FILE *>::from(fStdout);
        context->invoke(blk, cargs, nullptr, at);
        // Block done: close stdin (EOF → child). Drain stdout on a thread
        // while waiting — a child that writes more than the pipe buffer
        // (~4-64KB) after EOF-on-stdin would otherwise block in write() and
        // deadlock our WaitForSingleObject. Discard the drained bytes; the
        // contract is "read what you care about inside the block".
        if ( fStdin ) fclose(fStdin);
        thread drain;
        if ( fStdout ) {
            FILE * f = fStdout;
            drain = thread([f]() {
                char buf[4096];
                while ( fread(buf, 1, sizeof(buf), f) > 0 ) { /* discard */ }
            });
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        if ( drain.joinable() ) drain.join();
        if ( fStdout ) fclose(fStdout);
        return (int)exitCode;
#else
        int inPipe[2], outPipe[2];
        if ( pipe(inPipe) == -1 ) {
            vec4f cargs[2];
            cargs[0] = cast<FILE *>::from(nullptr);
            cargs[1] = cast<FILE *>::from(nullptr);
            context->invoke(blk, cargs, nullptr, at);
            return -1;
        }
        if ( pipe(outPipe) == -1 ) {
            close(inPipe[0]); close(inPipe[1]);
            vec4f cargs[2];
            cargs[0] = cast<FILE *>::from(nullptr);
            cargs[1] = cast<FILE *>::from(nullptr);
            context->invoke(blk, cargs, nullptr, at);
            return -1;
        }
        vector<char *> cargv;
        cargv.reserve(args_arr.size + 1);
        for ( uint64_t i = 0; i < args_arr.size; ++i ) {
            cargv.push_back(argv[i] ? argv[i] : (char *)"");
        }
        cargv.push_back(nullptr);
        pid_t pid = fork();
        if ( pid == -1 ) {
            close(inPipe[0]); close(inPipe[1]);
            close(outPipe[0]); close(outPipe[1]);
            vec4f cargs[2];
            cargs[0] = cast<FILE *>::from(nullptr);
            cargs[1] = cast<FILE *>::from(nullptr);
            context->invoke(blk, cargs, nullptr, at);
            return -1;
        }
        if ( pid == 0 ) {
            close(inPipe[1]);
            close(outPipe[0]);
            dup2(inPipe[0], STDIN_FILENO);
            dup2(outPipe[1], STDOUT_FILENO);
            dup2(outPipe[1], STDERR_FILENO);
            close(inPipe[0]);
            close(outPipe[1]);
            execvp(cargv[0], cargv.data());
            _exit(127);
        }
        close(inPipe[0]);
        close(outPipe[1]);
        // Wrap each pipe end in a FILE*. If fdopen fails, close the raw fd
        // we still own — otherwise the child's stdin write end leaks and
        // waitpid would deadlock (child never sees EOF).
        FILE * fStdin  = fdopen(inPipe[1],  "wb");
        if ( !fStdin  ) close(inPipe[1]);
        FILE * fStdout = fdopen(outPipe[0], "rb");
        if ( !fStdout ) close(outPipe[0]);
        vec4f cargs[2];
        cargs[0] = cast<FILE *>::from(fStdin);
        cargs[1] = cast<FILE *>::from(fStdout);
        context->invoke(blk, cargs, nullptr, at);
        // Block done: close stdin (EOF → child). Drain stdout on a thread
        // while waiting — a child that writes more than the pipe buffer
        // (~64KB on Linux) after EOF-on-stdin would otherwise block in
        // write() and deadlock our waitpid. Discard the drained bytes;
        // the contract is "read what you care about inside the block".
        if ( fStdin ) fclose(fStdin);
        thread drain;
        if ( fStdout ) {
            FILE * f = fStdout;
            drain = thread([f]() {
                char buf[4096];
                while ( fread(buf, 1, sizeof(buf), f) > 0 ) { /* discard */ }
            });
        }
        int status = 0;
        waitpid(pid, &status, 0);
        if ( drain.joinable() ) drain.join();
        if ( fStdout ) fclose(fStdout);
        return WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? WTERMSIG(status) : status;
#endif
    }

    int builtin_system ( const char * cmd, Context * context, LineInfoArg * at ) {
        if ( !cmd ) {
            context->throw_error_at(at, "system of null");
            return -1;
        }
        return system(cmd);
    }

    char * get_full_file_name ( const char * path, Context * context, LineInfoArg * at ) {
        if ( !path ) return nullptr;
        auto res = normalizeFileName(path);
        if ( res.length()==0 ) return nullptr;
        return context->allocateString(res,at);
    }

    bool builtin_remove_file ( const char * path ) {
        if ( !path ) return false;
#if defined(_WIN32)
        auto widePath = utf8_file_path_to_wide(path);
        return !widePath.empty() && _wremove(widePath.c_str()) == 0;
#else
        return remove(path) == 0;
#endif
    }

    bool builtin_rename_file ( const char * old_path, const char * new_path ) {
        if ( !old_path || !new_path ) return false;
#if defined(_WIN32)
        auto wideOldPath = utf8_file_path_to_wide(old_path);
        auto wideNewPath = utf8_file_path_to_wide(new_path);
        return !wideOldPath.empty() && !wideNewPath.empty()
            && _wrename(wideOldPath.c_str(), wideNewPath.c_str()) == 0;
#else
        return rename(old_path, new_path) == 0;
#endif
    }

    bool builtin_rmdir ( const char * path ) {
        if ( !path ) return false;
#if defined(_WIN32)
        auto widePath = utf8_file_path_to_wide(path);
        return !widePath.empty() && _wrmdir(widePath.c_str()) == 0;
#else
        return rmdir(path) == 0;
#endif
    }

    bool builtin_fexist ( const char * path ) {
        if ( !path ) return false;
        das_filestat st;
        return das_stat64(path, st) == 0;
    }

#if defined(_WIN32)
    static bool rmdir_rec_impl ( const std::wstring & path ) {
        _wfinddata_t c_file;
        intptr_t hFile;
        std::wstring findPath = path + L"/*";
        if ((hFile = _wfindfirst(findPath.c_str(), &c_file)) != -1L) {
            do {
                if ( wcscmp(c_file.name, L".") == 0 || wcscmp(c_file.name, L"..") == 0 ) continue;
                std::wstring child = path + L"/" + c_file.name;
                // A junction or symlink to a directory reports _A_SUBDIR, so
                // descending would delete the contents of whatever it points
                // at — outside the tree being removed. Remove the link itself,
                // which is `rm -rf`'s rule and matters wherever the tree was
                // written by something untrusted.
                DWORD childAttr = GetFileAttributesW(child.c_str());
                if ( childAttr != INVALID_FILE_ATTRIBUTES && (childAttr & FILE_ATTRIBUTE_REPARSE_POINT) ) {
                    bool unlinked = (childAttr & FILE_ATTRIBUTE_DIRECTORY)
                        ? RemoveDirectoryW(child.c_str()) != 0
                        : _wremove(child.c_str()) == 0;
                    if ( !unlinked ) { _findclose(hFile); return false; }
                    continue;
                }
                if ( c_file.attrib & _A_SUBDIR ) {
                    if ( !rmdir_rec_impl(child) ) { _findclose(hFile); return false; }
                } else {
                    if ( c_file.attrib & _A_RDONLY ) _wchmod(child.c_str(), _S_IREAD | _S_IWRITE);
                    if ( _wremove(child.c_str()) != 0 ) { _findclose(hFile); return false; }
                }
            } while (_wfindnext(hFile, &c_file) == 0);
            _findclose(hFile);
        }
        return _wrmdir(path.c_str()) == 0;
    }
#else
    static bool rmdir_rec_impl ( const string & path ) {
        DIR * dir = opendir(path.c_str());
        if ( !dir ) return false;
        struct dirent * ent;
        while ((ent = readdir(dir)) != nullptr) {
            if ( strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0 ) continue;
            string child = path + "/" + ent->d_name;
            struct stat st;
            // lstat, not stat: a symlink must be unlinked, never descended into.
            // stat() reports a link-to-directory as a directory, so following it
            // would delete the contents of whatever it points at — outside the
            // tree being removed. That is `rm -rf`'s rule, and it matters
            // wherever the tree being deleted was written by something
            // untrusted (a build output directory, an unpacked archive).
            if ( lstat(child.c_str(), &st) != 0 ) { closedir(dir); return false; }
            if ( S_ISDIR(st.st_mode) ) {
                if ( !rmdir_rec_impl(child) ) { closedir(dir); return false; }
            } else {
                if ( remove(child.c_str()) != 0 ) { closedir(dir); return false; }
            }
        }
        closedir(dir);
        return rmdir(path.c_str()) == 0;
    }
#endif

    bool builtin_rmdir_rec ( const char * path ) {
        if ( !path ) return false;
#if defined(_WIN32)
        auto widePath = utf8_file_path_to_wide(path);
        return !widePath.empty() && rmdir_rec_impl(widePath);
#else
        return rmdir_rec_impl(string(path));
#endif
    }

    static char * errno_to_string ( Context * ctx, LineInfoArg * at ) {
        auto msg = strerror(errno);
        return ctx->allocateString(msg, uint32_t(strlen(msg)), at);
    }

    static char * empty_path_error ( Context * ctx, LineInfoArg * at ) {
        return ctx->allocateString("empty path", 10, at);
    }

    bool builtin_remove_file_ec ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !path ) { error = empty_path_error(ctx, at); return false; }
#if defined(_WIN32)
        auto widePath = utf8_file_path_to_wide(path);
        if ( widePath.empty() || _wremove(widePath.c_str()) != 0 ) { error = errno_to_string(ctx, at); return false; }
#else
        if ( remove(path) != 0 ) { error = errno_to_string(ctx, at); return false; }
#endif
        return true;
    }

    bool builtin_rename_file_ec ( const char * old_path, const char * new_path, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !old_path || !new_path ) { error = empty_path_error(ctx, at); return false; }
#if defined(_WIN32)
        auto wideOld = utf8_file_path_to_wide(old_path);
        auto wideNew = utf8_file_path_to_wide(new_path);
        if ( wideOld.empty() || wideNew.empty()
            || _wrename(wideOld.c_str(), wideNew.c_str()) != 0 ) { error = errno_to_string(ctx, at); return false; }
#else
        if ( rename(old_path, new_path) != 0 ) { error = errno_to_string(ctx, at); return false; }
#endif
        return true;
    }

    bool builtin_mkdir_ec ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !path ) { error = empty_path_error(ctx, at); return false; }
#if defined(_WIN32)
        auto widePath = utf8_file_path_to_wide(path);
        if ( widePath.empty() || _wmkdir(widePath.c_str()) != 0 ) { error = errno_to_string(ctx, at); return false; }
#elif defined(_EMSCRIPTEN_VER)
        if ( mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 ) { error = errno_to_string(ctx, at); return false; }
#else
        if ( mkdir(path, ACCESSPERMS) != 0 ) { error = errno_to_string(ctx, at); return false; }
#endif
        return true;
    }

    bool builtin_rmdir_ec ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !path ) { error = empty_path_error(ctx, at); return false; }
#if defined(_WIN32)
        auto widePath = utf8_file_path_to_wide(path);
        if ( widePath.empty() || _wrmdir(widePath.c_str()) != 0 ) { error = errno_to_string(ctx, at); return false; }
#else
        if ( rmdir(path) != 0 ) { error = errno_to_string(ctx, at); return false; }
#endif
        return true;
    }

    bool has_env_variable ( const char * var, Context * , LineInfoArg * ) {
        if ( !var ) return false;
        auto res = das_getenv(var);
        return res != nullptr;
    }

    char * get_env_variable ( const char * var, Context * context, LineInfoArg * at ) {
        if ( !var ) return nullptr;
        auto res = das_getenv(var);
        if ( !res ) return nullptr;
        return context->allocateString(res, at);
    }

    void set_env_variable ( const char * var, const char * value, Context *, LineInfoArg * ) {
        // process-wide; children spawned via popen/popen_argv inherit it
        if ( !var || !*var ) return;
        das_setenv(var, value);
    }

    enum class RegisterOnError {
        Quiet = 0,      // missing/unloadable .shared_module is silent (deferred for retry); broken artifacts still report
        ErrorMsg,       // error message is displayed
        Fail,           // error message is displayed and exception is thrown
    };

    static vector<tuple<string,string,string>> g_registered_dynamic_modules; // path, cpp_class_name, daslang_name
    static vector<tuple<string,string,string>> g_registered_native_paths;
    // Modules whose dlopen failed in Quiet mode — typically a sibling-module
    // DT_NEEDED dependency (e.g. node-editor -> dasModuleImgui) not yet loaded.
    // retry_pending_dynamic_modules() re-attempts these in fixed-point passes
    // after the folder scan, so module enumeration order stops mattering.
    static vector<tuple<string,string,string>> g_pending_dynamic_modules; // path, cpp_class_name, last dlopen error

    // Env-gated per-attempt module-load trace (default off). Set
    // DAS_TRACE_MODULE_LOAD=1 to surface each dlopen — turns a swallowed
    // 'missing prerequisite' into a visible "FAILED — <dlerror>" line.
    static bool trace_module_load() {
        static const bool on = []{
            const char * e = get_dasenv_trace_module_load();
            return e && e[0] && e[0] != '0';
        }();
        return on;
    }

    // Returns DLL handle, nullptr on failure. on_error governs LOAD failures only:
    // Quiet defers a failed dlopen to retry_pending_dynamic_modules() (sibling-dep
    // ordering). A loadable-but-broken artifact (registrator missing, build-id
    // mismatch) always reports — to the context if any, else LOG(error) (#2580).
    void *register_dynamic_module(const char *path, const char *mod_name, int on_error, Context * context, LineInfoArg * at ) {
        string actualPath(path);
#ifndef NDEBUG
        // Debug builds produce _debug.shared_module; rewrite the path so that
        // .das_module files don't need per-config conditional logic.
        auto pos = actualPath.rfind(".shared_module");
        if ( pos != string::npos && pos + 14 == actualPath.size() ) {
            actualPath.insert(pos, "_debug");
        }
#endif
        auto lib = loadDynamicLibrary(actualPath.c_str());
        // Capture the dlopen error once, before anything else can clear it.
        string dlErr = lib ? string() : getDynamicLibraryError();
        if ( trace_module_load() ) {
            LOG(LogLevel::info) << "[module] " << (mod_name ? mod_name : "(null)") << " <- " << actualPath
                << " : " << (lib ? "loaded" : ("FAILED - " + dlErr)) << "\n";
        }
        if (!lib) {
            if (static_cast<RegisterOnError>(on_error) != RegisterOnError::Quiet) {
                auto err_msg = "dynamic module `" + string(mod_name) + "` - failed to load: " + actualPath
                    + (dlErr.empty() ? string("\n") : (" (" + dlErr + ")\n"));
                if (context) context->to_err(at, err_msg.c_str());
                else LOG(LogLevel::error) << err_msg;
                if (context && static_cast<RegisterOnError>(on_error) == RegisterOnError::Fail) {
                    context->throw_error(err_msg.c_str());
                }
            } else {
                // Quiet: a sibling-module DT_NEEDED dependency may not be loaded yet
                // (module .so's live in modules/<dep>/, not on RUNPATH, so a dependent
                // enumerated before its dependency fails dlopen). Defer; the post-scan
                // retry_pending_dynamic_modules() re-attempts once deps are loaded.
                g_pending_dynamic_modules.emplace_back(string(path), string(mod_name), dlErr);
            }
            return nullptr;
        }
        const auto regName = getDynModuleRegistratorName(mod_name);
        auto rawFn = getFunctionAddress(lib, regName.c_str());
        if (!rawFn) {
            auto err_msg = "dynamic module `" + string(mod_name) + "` - function `" + regName + "` not found in `" + actualPath + "`\n";
            if (context) context->to_err(at, err_msg.c_str());
            else LOG(LogLevel::error) << err_msg;
            if (context && static_cast<RegisterOnError>(on_error) == RegisterOnError::Fail) {
                context->throw_error(err_msg.c_str());
            }
            closeLibrary(lib);
            return nullptr;
        }
        auto fn = reinterpret_cast<Module*(*)(int)>(rawFn);
        auto mod = fn(DAS_BUILD_ID);
        if (!mod) {
            auto err_msg = "dynamic module `" + string(mod_name) + "` - build-id mismatch (host " + to_string(DAS_BUILD_ID) + "); rebuild the module for current configuration\n";
            if (context) context->to_err(at, err_msg.c_str());
            else LOG(LogLevel::error) << err_msg;
            if (context && static_cast<RegisterOnError>(on_error) == RegisterOnError::Fail) {
                context->throw_error(err_msg.c_str());
            }
            closeLibrary(lib);
            return nullptr;
        }
        *ModuleKarma += unsigned(intptr_t(mod));
        g_registered_dynamic_modules.emplace_back(path, mod_name, mod->name);
        return lib;
    }
    void *register_dynamic_module_silent(const char *path, const char *mod_name, Context * context, LineInfoArg * at ) {
        return register_dynamic_module(path, mod_name, static_cast<int>(RegisterOnError::Quiet), context, at);
    }

    // Re-attempt modules whose dlopen was deferred (Quiet failure during the
    // module-folder scan — usually a sibling-module DT_NEEDED dep not yet loaded
    // because directory enumeration visited the dependent before its dependency).
    // Fixed-point: each pass retries every deferred module; one whose deps loaded
    // in a prior pass now succeeds and may unblock others. Keep iterating while a
    // pass loads at least one; stop when a pass makes no progress (remaining
    // entries have genuinely-missing .so files — left silent, matching Quiet).
    DAS_API void retry_pending_dynamic_modules() {
        bool progress = true;
        while (progress && !g_pending_dynamic_modules.empty()) {
            progress = false;
            vector<tuple<string,string,string>> pending;
            pending.swap(g_pending_dynamic_modules); // drain; failures re-push into the now-empty global
            for (auto & pr : pending) {
                if (register_dynamic_module(get<0>(pr).c_str(), get<1>(pr).c_str(),
                        static_cast<int>(RegisterOnError::Quiet), nullptr, nullptr) != nullptr) {
                    progress = true;
                }
            }
        }
    }

    // One line per still-pending (dlopen-failed) module: "  Module_Glfw <- <path> (<dlerror>)"
    // — the registrator class name, which is what was looked up, not the dylib's file name.
    // Empty string when nothing is pending. Compile-error paths append this to the misleading
    // "missing prerequisite ...; file not found" so a load failure stops reading as a path typo.
    DAS_API string describe_pending_dynamic_modules() {
        TextWriter tw;
        for (auto & pr : g_pending_dynamic_modules) {
            tw << "  " << get<1>(pr) << " <- " << get<0>(pr);
            if (!get<2>(pr).empty()) tw << " (" << get<2>(pr) << ")";
            tw << "\n";
        }
        return tw.str();
    }

    // Standalone-exe epilogue to the per-module Quiet registrations: everything the exe
    // registers is REQUIRED by its program, so a module still pending after the retry pass
    // is a hard failure — report it loudly instead of dying later on the first extern
    // lookup with an unrelated-looking "Failed to find <fn> in module <mod>" fatal.
    DAS_API int report_pending_dynamic_modules() {
        for (auto & pr : g_pending_dynamic_modules) {
            LOG(LogLevel::error) << "dynamic module `" << get<1>(pr) << "` failed to load: " << get<0>(pr)
                << (get<2>(pr).empty() ? string() : (" (" + get<2>(pr) + ")")) << "\n";
        }
        return int(g_pending_dynamic_modules.size());
    }

    void register_native_path(const char *mod_name, const char *src_path, const char *dst_path, Context * /*context*/, LineInfoArg * /*at*/ ) {
        // daScriptEnvironment::ensure() will help, but let's keep assertion.
        DAS_ASSERTF(daScriptEnvironment::getBound(), "When register_native_path called we expect that environment is already set.");
        auto &mod_resolve = daScriptEnvironment::getBound()->g_dyn_modules_resolve;
        if (mod_resolve == nullptr) {
            mod_resolve = new vector<DynamicModuleInfo>();
        }
        auto cur_mod = find_if(mod_resolve->begin(), mod_resolve->end(), [&mod_name](DynamicModuleInfo &mod) {
            return mod.name == mod_name;
        });
        if (cur_mod == mod_resolve->end()) {
            // Add new module
            mod_resolve->emplace_back(DynamicModuleInfo{mod_name,{}});
            cur_mod = prev(mod_resolve->end());
        }
        cur_mod->paths.emplace_back(src_path, dst_path);
        g_registered_native_paths.emplace_back(mod_name, src_path, dst_path);
    }

    void for_each_registered_native_path ( const TBlock<void,const char *,const char *,const char *> & block, Context * context, LineInfoArg * at ) {
        for ( const auto & [mod_name, src_path, dst_path] : g_registered_native_paths ) {
            das_invoke<void>::invoke<const char *,const char *,const char *>(context, at, block, mod_name.c_str(), src_path.c_str(), dst_path.c_str());
        }
    }

    void for_each_registered_dynamic_module ( const TBlock<void,const char *,const char *,const char *> & block, Context * context, LineInfoArg * at ) {
        for ( const auto & [path, mod_name, das_name] : g_registered_dynamic_modules ) {
            das_invoke<void>::invoke<const char *,const char *,const char *>(context, at, block, path.c_str(), mod_name.c_str(), das_name.c_str());
        }
    }

    char * sanitize_command_line ( const char * cmd, Context * context, LineInfoArg * at ) {
        if ( !cmd ) return nullptr;
        stringstream ss;
        for ( const char * ch=cmd; *ch; ) {
#if defined(_WIN32)
            if ( *ch=='^' || *ch=='|' || *ch=='<' || *ch=='>' || *ch=='&' ||
                    *ch=='%' || *ch=='$' || *ch=='`' || *ch=='\'' || *ch=='@' ) {
                ss.put('^');
                ss.put(*ch++);
#else
            if ( *ch=='$' || *ch=='`' ) {
                ss.put('\\');
                ss.put(*ch++);
#endif
            } else {
                ss.put(*ch++);
            }
        }
        if ( ss.str().size() > UINT_MAX ) context->throw_error_at(at, "string too long");
        return context->allocateString(ss.str().data(), uint32_t(ss.str().size()), at);
    }

    // ---- filesystem operations (C++17 <filesystem>) ----

    static char * ec_to_string ( const std::error_code & ec, Context * ctx, LineInfoArg * at ) {
        auto msg = ec.message();
        return ctx->allocateString(msg.data(), uint32_t(msg.size()), at);
    }

    // path manipulation

    char * builtin_fs_extension ( const char * path, Context * ctx, LineInfoArg * at ) {
        if ( !path ) return nullptr;
        auto ext = path_to_das(das_to_path(path).extension());
        if ( ext.empty() ) return nullptr;
        return ctx->allocateString(ext.data(), uint32_t(ext.size()), at);
    }

    char * builtin_fs_stem ( const char * path, Context * ctx, LineInfoArg * at ) {
        if ( !path ) return nullptr;
        auto s = path_to_das(das_to_path(path).stem());
        if ( s.empty() ) return nullptr;
        return ctx->allocateString(s.data(), uint32_t(s.size()), at);
    }

    char * builtin_fs_replace_extension ( const char * path, const char * new_ext, Context * ctx, LineInfoArg * at ) {
        if ( !path ) return nullptr;
        auto p = das_to_path(path);
        p.replace_extension(das_to_path(new_ext));
        auto s = path_to_das(p);
        return ctx->allocateString(s.data(), uint32_t(s.size()), at);
    }

    char * builtin_fs_join ( const char * a, const char * b, Context * ctx, LineInfoArg * at ) {
        if ( !a && !b ) return nullptr;
        auto pa = das_to_path(a);
        if ( b ) pa /= das_to_path(b);
        auto s = path_to_das(pa);
        return ctx->allocateString(s.data(), uint32_t(s.size()), at);
    }

    char * builtin_fs_normalize ( const char * path, Context * ctx, LineInfoArg * at ) {
        if ( !path ) return nullptr;
        auto s = path_to_das(das_to_path(path).lexically_normal());
        if ( s.empty() ) return nullptr;
        return ctx->allocateString(s.data(), uint32_t(s.size()), at);
    }

    bool builtin_fs_is_absolute ( const char * path ) {
        if ( !path ) return false;
        return das_to_path(path).is_absolute();
    }

    char * builtin_fs_relative ( const char * path, const char * base, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !path || !base ) { error = empty_path_error(ctx, at); return nullptr; }
        std::error_code ec;
        auto s = path_to_das(std::filesystem::relative(das_to_path(path), das_to_path(base), ec));
        if ( ec ) { error = ec_to_string(ec, ctx, at); return nullptr; }
        if ( s.empty() ) return nullptr;
        return ctx->allocateString(s.data(), uint32_t(s.size()), at);
    }

    char * builtin_fs_parent ( const char * path, Context * ctx, LineInfoArg * at ) {
        if ( !path ) return nullptr;
        auto s = path_to_das(das_to_path(path).parent_path());
        if ( s.empty() ) return nullptr;
        return ctx->allocateString(s.data(), uint32_t(s.size()), at);
    }

    // file queries

    int64_t builtin_fs_file_size ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !path ) { error = empty_path_error(ctx, at); return -1; }
        std::error_code ec;
        auto sz = std::filesystem::file_size(das_to_path(path), ec);
        if ( ec ) { error = ec_to_string(ec, ctx, at); return -1; }
        return static_cast<int64_t>(sz);
    }

    bool builtin_fs_equivalent ( const char * a, const char * b, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !a || !b ) { error = empty_path_error(ctx, at); return false; }
        std::error_code ec;
        bool result = std::filesystem::equivalent(das_to_path(a), das_to_path(b), ec);
        if ( ec ) { error = ec_to_string(ec, ctx, at); return false; }
        return result;
    }

    bool builtin_fs_is_symlink ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !path || !path[0] ) { error = empty_path_error(ctx, at); return false; }
        std::error_code ec;
        bool result = std::filesystem::is_symlink(das_to_path(path), ec);
        if ( ec ) { error = ec_to_string(ec, ctx, at); return false; }
        return result;
    }

    // file operations

    bool builtin_fs_copy_file ( const char * src, const char * dst, bool overwrite, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !src || !dst ) { error = empty_path_error(ctx, at); return false; }
        std::error_code ec;
        auto opts = overwrite ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none;
        bool result = std::filesystem::copy_file(das_to_path(src), das_to_path(dst), opts, ec);
        if ( ec ) { error = ec_to_string(ec, ctx, at); return false; }
        return result;
    }

    bool builtin_fs_set_mtime ( const char * path, Time time, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !path ) { error = empty_path_error(ctx, at); return false; }
        std::error_code ec;
        auto sys_time = std::chrono::system_clock::from_time_t(time.time);
        auto file_time = std::filesystem::file_time_type::clock::now() +
            (sys_time - std::chrono::system_clock::now());
        std::filesystem::last_write_time(das_to_path(path), file_time, ec);
        if ( ec ) { error = ec_to_string(ec, ctx, at); return false; }
        return true;
    }

    // directory operations

    void builtin_fs_dir_rec ( const char * path, const TBlock<void, char *, bool> & blk, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !path ) { error = empty_path_error(ctx, at); return; }
        std::filesystem::path root = das_to_path(path);
        std::error_code ec;
        auto it = std::filesystem::recursive_directory_iterator(root,
                std::filesystem::directory_options::skip_permission_denied, ec);
        if ( ec ) { error = ec_to_string(ec, ctx, at); return; }
        for ( auto & entry : it ) {
            if ( ec ) { ec.clear(); continue; }
            std::string rel = path_to_das(std::filesystem::relative(entry.path(), root, ec));
            if ( ec ) { ec.clear(); continue; }
            bool is_dir = entry.is_directory(ec);
            if ( ec ) { ec.clear(); }
            char * fname = ctx->allocateString(rel.data(), uint32_t(rel.size()), at);
            vec4f args[2] = {
                cast<char*>::from(fname),
                cast<bool>::from(is_dir)
            };
            ctx->invoke(blk, args, nullptr, at);
        }
    }

    // system queries

    char * builtin_fs_temp_directory ( char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        std::error_code ec;
        auto s = path_to_das(std::filesystem::temp_directory_path(ec));
        if ( ec ) { error = ec_to_string(ec, ctx, at); return nullptr; }
        return ctx->allocateString(s.data(), uint32_t(s.size()), at);
    }

    static std::filesystem::path generate_unique_temp_path ( const char * prefix, const char * ext, std::error_code & ec ) {
        auto tmp = std::filesystem::temp_directory_path(ec);
        if ( ec ) return {};
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis;
        for ( int i = 0; i < 100; ++i ) {
            auto name = string(prefix ? prefix : "tmp") + "_" + das::to_string(dis(gen))
                + (ext ? ext : "");
            // In order to support EASTL do not mix strings.
            auto p = tmp / das_to_path(name.c_str());
            if ( !std::filesystem::exists(p, ec) && !ec ) return p;
        }
        return {};
    }

    char * builtin_fs_create_temp_file ( const char * prefix, const char * ext, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        std::error_code ec;
        auto p = generate_unique_temp_path(prefix, ext, ec);
        if ( ec ) { error = ec_to_string(ec, ctx, at); return nullptr; }
        if ( p.empty() ) return nullptr;
        std::ofstream ofs(p, std::ios::binary);
        if ( !ofs ) return nullptr;
        ofs.close();
        auto s = p.string();
        return ctx->allocateString(s.data(), uint32_t(s.size()), at);
    }

    char * builtin_fs_create_temp_directory ( const char * prefix, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        std::error_code ec;
        auto p = generate_unique_temp_path(prefix, "", ec);
        if ( ec ) { error = ec_to_string(ec, ctx, at); return nullptr; }
        if ( p.empty() ) return nullptr;
        if ( !std::filesystem::create_directory(p, ec) || ec ) {
            if ( ec ) error = ec_to_string(ec, ctx, at);
            return nullptr;
        }
        auto s = p.string();
        return ctx->allocateString(s.data(), uint32_t(s.size()), at);
    }

    bool builtin_fs_disk_space ( const char * path, DiskSpaceInfo & info, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !path ) { error = empty_path_error(ctx, at); return false; }
        std::error_code ec;
        auto si = std::filesystem::space(path, ec);
        if ( ec ) { error = ec_to_string(ec, ctx, at); return false; }
        info.capacity = si.capacity;
        info.free = si.free;
        info.available = si.available;
        return true;
    }

    bool builtin_rmdir_rec_ec ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !path ) { error = empty_path_error(ctx, at); return false; }
        std::error_code ec;
        std::filesystem::remove_all(das_to_path(path), ec);
        if ( ec ) { error = ec_to_string(ec, ctx, at); return false; }
        return true;
    }

    // 3-tier directory resolver mirroring the shared-module resolution policy
    // from PR #2579 (module_jit.cpp::resolve_dynamic_module_path), but for
    // source-side asset directories.  Given a baked source-file path captured
    // at macro expansion (e.g. ".../modules/das-cards/cards/card_mesh.das"),
    // returns the directory where that module's runtime assets currently live:
    //   1. <exe_dir>/<rel>            — daspkg-release standalone bundle
    //   2. <das_root>/<rel>           — SDK / cmake install layout
    //   3. dir_name(baked_path)       — dev (interpreted from source tree)
    // <rel> is the substring of dir_name(baked) starting at the last
    // "/modules/" segment.  Tiers 1+2 are skipped when baked has no /modules/
    // segment (e.g. project-local code outside the package layout).
    //
    // `standalone` is is_standalone_exe() folded at the call site (true only in
    // a daspkg-release -exe). A standalone exe must NEVER resolve to the baked
    // dev source tree: the compile-machine path is meaningless on a target box,
    // and on the AUTHOR's own machine it still exists — so tier 3 would wrongly
    // prefer it over the assets shipped next to the exe (the dictation-bot
    // release-config bug). When standalone, the tier-3 fallback is <exe_dir>.
    // For project-local code that fallback also fires when the baked dir has
    // gone missing on the target machine (relocated bundle), even interpreted.
    char * builtin_resolve_this_module_dir ( const char * baked_path, bool standalone, Context * context ) {
        namespace fs = std::filesystem;
        if ( !baked_path || !*baked_path ) return context->allocateString("", nullptr);
        // generic_string() (here and at every other path-to-string conversion
        // in this function) emits forward-slash separators on every platform,
        // matching getDasRoot() and the rest of daslang's path conventions.
        // Without it Windows would return native backslashes that break string
        // compares against `/`-formed paths from script-side daslang code.
        fs::path baked(baked_path);
        fs::path baked_dir = baked.parent_path();
        std::string baked_dir_str = baked_dir.generic_string();
        // Find the last "/modules/" boundary; everything from that segment
        // onward is the bundle/SDK-relative suffix (e.g.
        // "modules/das-cards/cards"). rfind, not find — for nested layouts
        // like "<root>/modules/A/modules/B/x.das" the right answer is "B"'s
        // package, not "A". Mirrors compute_modules_relative_suffix in
        // modules/dasLLVM/daslib/llvm_exe.das.
        const std::string sep = "/modules/";
        std::string canon = baked_dir_str;
        for ( char & c : canon ) if ( c == '\\' ) c = '/';
        std::string rel;
        size_t pos = canon.rfind(sep);
        if ( pos != std::string::npos ) {
            rel = canon.substr(pos + 1);  // drop leading '/' to keep it relative
        }
        // <exe_dir> — computed once, reused by the tier-1 probe and the fallback.
        fs::path exeDir;
        {
            das::string exeFile = das::getExecutableFileName();
            if ( !exeFile.empty() ) {
                exeDir = fs::path(exeFile.c_str()).parent_path();
                if ( exeDir.empty() ) exeDir = ".";
            }
        }
        if ( !rel.empty() ) {
            // Tier 1 — <exe_dir>/<rel>
            if ( !exeDir.empty() ) {
                fs::path candidate = exeDir / rel;
                std::error_code ec;
                if ( fs::is_directory(candidate, ec) ) {
                    return context->allocateString(candidate.generic_string().c_str(), nullptr);
                }
            }
            // Tier 2 — <das_root>/<rel>
            fs::path candidate = fs::path(das::getDasRoot().c_str()) / rel;
            std::error_code ec;
            if ( fs::is_directory(candidate, ec) ) {
                return context->allocateString(candidate.generic_string().c_str(), nullptr);
            }
        }
        // Tier 3 — fallback. Prefer <exe_dir> over the baked dev dir when this
        // is a standalone exe (never trust the compile-machine source path), or
        // when project-local code's baked dir has gone missing (relocated
        // bundle). Otherwise (dev, running from source) the baked dir is right.
        if ( !exeDir.empty() ) {
            std::error_code ec;
            bool baked_dir_exists = fs::is_directory(baked_dir, ec);
            if ( standalone || ( rel.empty() && !baked_dir_exists ) ) {
                return context->allocateString(exeDir.generic_string().c_str(), nullptr);
            }
        }
        return context->allocateString(baked_dir_str.c_str(), nullptr);
    }
}
#endif // DAS_NO_FILEIO

MAKE_TYPE_FACTORY(FStat, das::FStat)
MAKE_TYPE_FACTORY(FILE,FILE)
MAKE_TYPE_FACTORY(DiskSpaceInfo, das::DiskSpaceInfo)

namespace das {

    struct FStatAnnotation : ManagedStructureAnnotation <FStat,true> {
        FStatAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("FStat", ml) {
            validationNeverFails = true;
            addField<DAS_BIND_MANAGED_FIELD(is_valid)>("is_valid");
            addProperty<DAS_BIND_MANAGED_PROP(size)>("size");
            addProperty<DAS_BIND_MANAGED_PROP(atime)>("atime");
            addProperty<DAS_BIND_MANAGED_PROP(ctime)>("ctime");
            addProperty<DAS_BIND_MANAGED_PROP(mtime)>("mtime");
            addProperty<DAS_BIND_MANAGED_PROP(is_reg)>("is_reg");
            addProperty<DAS_BIND_MANAGED_PROP(is_dir)>("is_dir");
        }
        virtual bool canMove() const override { return true; }
        virtual bool canCopy() const override { return true; }
        virtual bool isLocal() const override { return true; }
    };

    struct DiskSpaceInfoAnnotation : ManagedStructureAnnotation<DiskSpaceInfo, true> {
        DiskSpaceInfoAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation("DiskSpaceInfo", ml) {
            validationNeverFails = true;
            addField<DAS_BIND_MANAGED_FIELD(capacity)>("capacity");
            addField<DAS_BIND_MANAGED_FIELD(free)>("free");
            addField<DAS_BIND_MANAGED_FIELD(available)>("available");
        }
        virtual bool canMove() const override { return true; }
        virtual bool canCopy() const override { return true; }
        virtual bool isLocal() const override { return true; }
    };

    class Module_FIO : public Module {
    public:
        Module_FIO() : Module("fio_core") {
            DAS_PROFILE_SECTION("Module_FIO");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            addBuiltinDependency(lib, Module::require("strings"));
            // type
            addAnnotation(new DummyTypeAnnotation("FILE", "FILE", 16, 16));
            addAnnotation(new FStatAnnotation(lib));
            // seek constants
            addConstant<int32_t>(*this, "seek_set", SEEK_SET);
            addConstant<int32_t>(*this, "seek_cur", SEEK_CUR);
            addConstant<int32_t>(*this, "seek_end", SEEK_END);
            // file io
            addExtern<DAS_BIND_FUN(builtin_remove_file)>(*this, lib, "remove",
                SideEffects::modifyExternal, "builtin_remove_file")
                    ->args({"name"});
            addExtern<DAS_BIND_FUN(builtin_remove_file_ec)>(*this, lib, "remove",
                SideEffects::modifyArgumentAndExternal, "builtin_remove_file_ec")
                    ->args({"name","error","context","at"});
            addExtern<DAS_BIND_FUN(builtin_rename_file)>(*this, lib, "rename",
                SideEffects::modifyExternal, "builtin_rename_file")
                    ->args({"old_name","new_name"});
            addExtern<DAS_BIND_FUN(builtin_rename_file_ec)>(*this, lib, "rename",
                SideEffects::modifyArgumentAndExternal, "builtin_rename_file_ec")
                    ->args({"old_name","new_name","error","context","at"});
            addExtern<DAS_BIND_FUN(builtin_fexist)>(*this, lib, "fexist",
                SideEffects::modifyExternal, "builtin_fexist")
                    ->arg("path");
            addExtern<DAS_BIND_FUN(builtin_rmdir_rec)>(*this, lib, "rmdir_rec",
                SideEffects::modifyExternal, "builtin_rmdir_rec")
                    ->arg("path");
            addExtern<DAS_BIND_FUN(builtin_rmdir_rec_ec)>(*this, lib, "rmdir_rec",
                SideEffects::modifyArgumentAndExternal, "builtin_rmdir_rec_ec")
                    ->args({"path","error","context","at"});
            addExtern<DAS_BIND_FUN(builtin_fopen)>(*this, lib, "fopen",
                SideEffects::modifyExternal, "builtin_fopen")
                    ->args({"name","mode","context","line"})->setNoDiscard();
            addExtern<DAS_BIND_FUN(builtin_fclose)>(*this, lib, "fclose",
                SideEffects::modifyExternal, "builtin_fclose")
                    ->args({"file","context","line"});
            addExtern<DAS_BIND_FUN(builtin_fflush)>(*this, lib, "fflush",
                SideEffects::modifyExternal, "builtin_fflush")
                    ->args({"file","context","line"});
            addExtern<DAS_BIND_FUN(builtin_fprint)>(*this, lib, "fprint",
                SideEffects::modifyExternal, "builtin_fprint")
                    ->args({"file","text","context","line"});
            addExtern<DAS_BIND_FUN(builtin_fread)>(*this, lib, "fread",
                SideEffects::modifyExternal, "builtin_fread")
                    ->args({"file","context","line"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(builtin_map_file)>(*this, lib, "fmap",
                SideEffects::modifyExternal, "builtin_map_file")
                    ->args({"file","block","context","line"});
            addExtern<DAS_BIND_FUN(builtin_fmap_open)>(*this, lib, "fmap_open",
                SideEffects::modifyExternal, "builtin_fmap_open")
                    ->args({"path","size","context","line"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_fmap_open_rw)>(*this, lib, "fmap_open_rw",
                SideEffects::modifyExternal, "builtin_fmap_open_rw")
                    ->args({"path","size","context","line"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_fmap_close)>(*this, lib, "fmap_close",
                SideEffects::modifyExternal, "builtin_fmap_close")
                    ->args({"data","size","context","line"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_dwrite_open)>(*this, lib, "dwrite_open",
                SideEffects::modifyExternal, "builtin_dwrite_open")
                    ->args({"path","total_bytes","band_bytes","context","line"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_dwrite_append)>(*this, lib, "dwrite_append",
                SideEffects::modifyExternal, "builtin_dwrite_append")
                    ->args({"writer","data","bytes","context","line"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_dwrite_band)>(*this, lib, "dwrite_band",
                SideEffects::modifyExternal, "builtin_dwrite_band")
                    ->args({"writer","avail","context","line"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_dwrite_commit)>(*this, lib, "dwrite_commit",
                SideEffects::modifyExternal, "builtin_dwrite_commit")
                    ->args({"writer","bytes","context","line"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_dwrite_stat)>(*this, lib, "dwrite_stat",
                SideEffects::modifyExternal, "builtin_dwrite_stat")
                    ->args({"writer","which","context","line"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_dwrite_close)>(*this, lib, "dwrite_close",
                SideEffects::modifyExternal, "builtin_dwrite_close")
                    ->args({"writer","context","line"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_prefetch_map)>(*this, lib, "prefetch_map",
                SideEffects::modifyExternal, "builtin_prefetch_map")
                    ->args({"base","bytes","context","line"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_fgets)>(*this, lib, "fgets",
                SideEffects::modifyExternal, "builtin_fgets")
                    ->args({"file","context","line"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(builtin_fwrite)>(*this, lib, "fwrite",
                SideEffects::modifyExternal, "builtin_fwrite")
                    ->args({"file","text","context","line"});
            addExtern<DAS_BIND_FUN(builtin_feof)>(*this, lib, "feof",
                SideEffects::modifyExternal, "builtin_feof")
                    ->arg("file");
            addExtern<DAS_BIND_FUN(builtin_fseek)>(*this, lib, "fseek",
                SideEffects::modifyExternal, "builtin_fseek")
                    ->args({"file","offset","mode","context","line"});
            addExtern<DAS_BIND_FUN(builtin_ftell)>(*this, lib, "ftell",
                SideEffects::modifyExternal, "builtin_ftell")
                    ->args({"file","context","line"});
            // builtin file functions
            addInterop<builtin_read,int,const FILE*,vec4f,int32_t>(*this, lib, "_builtin_read",
                SideEffects::modifyExternal, "builtin_read")
                    ->args({"file","buffer","length"});
            addInterop<builtin_write,int,const FILE*,vec4f,int32_t>(*this, lib, "_builtin_write",
                SideEffects::modifyExternal, "builtin_write")
                    ->args({"file","buffer","length"});
            addInterop<builtin_read64,int64_t,const FILE*,vec4f,int64_t>(*this, lib, "_builtin_read64",
                SideEffects::modifyExternal, "builtin_read64")
                    ->args({"file","buffer","length"});
            addInterop<builtin_write64,int64_t,const FILE*,vec4f,int64_t>(*this, lib, "_builtin_write64",
                SideEffects::modifyExternal, "builtin_write64")
                    ->args({"file","buffer","length"});
            addInterop<builtin_load,void,const FILE*,int64_t,const Block &>(*this, lib, "_builtin_load",
                das::SideEffects::modifyExternal, "builtin_load")
                    ->args({"file","length","block"});
            addExtern<DAS_BIND_FUN(builtin_dirname)>(*this, lib, "dir_name",
                SideEffects::none, "builtin_dirname")
                    ->args({"name","context","line"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(builtin_basename)>(*this, lib, "base_name",
                SideEffects::none, "builtin_basename")
                    ->args({"name","context","line"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(builtin_fstat)>(*this, lib, "fstat",
                SideEffects::modifyArgumentAndExternal, "builtin_fstat")
                    ->args({"file","stat","context","line"});
            addExtern<DAS_BIND_FUN(builtin_stat)>(*this, lib, "stat",
                SideEffects::modifyArgumentAndExternal, "builtin_stat")
                    ->args({"file","stat"});
            addExtern<DAS_BIND_FUN(builtin_dir)>(*this, lib, "builtin_dir",
                SideEffects::modifyExternal, "builtin_dir")
                    ->args({"path","block","context","line"});
            addExtern<DAS_BIND_FUN(builtin_mkdir)>(*this, lib, "mkdir",
                SideEffects::modifyExternal, "builtin_mkdir")
                    ->arg("path");
            addExtern<DAS_BIND_FUN(builtin_mkdir_ec)>(*this, lib, "mkdir",
                SideEffects::modifyArgumentAndExternal, "builtin_mkdir_ec")
                    ->args({"path","error","context","at"});
            addExtern<DAS_BIND_FUN(builtin_rmdir)>(*this, lib, "rmdir",
                SideEffects::modifyExternal, "builtin_rmdir")
                    ->arg("path");
            addExtern<DAS_BIND_FUN(builtin_rmdir_ec)>(*this, lib, "rmdir",
                SideEffects::modifyArgumentAndExternal, "builtin_rmdir_ec")
                    ->args({"path","error","context","at"});
            addExtern<DAS_BIND_FUN(builtin_chdir)>(*this, lib, "chdir",
                SideEffects::modifyExternal, "builtin_chdir")
                    ->arg("path");
            addExtern<DAS_BIND_FUN(builtin_getcwd)>(*this, lib, "getcwd",
                SideEffects::modifyExternal, "builtin_getcwd")
                    ->args({"context","at"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(builtin_stdin)>(*this, lib, "fstdin",
                SideEffects::modifyExternal, "builtin_stdin");
            addExtern<DAS_BIND_FUN(builtin_stdout)>(*this, lib, "fstdout",
                SideEffects::modifyExternal, "builtin_stdout");
            addExtern<DAS_BIND_FUN(builtin_stderr)>(*this, lib, "fstderr",
                SideEffects::modifyExternal, "builtin_stderr");
            addExtern<DAS_BIND_FUN(builtin_is_terminal)>(*this, lib, "is_terminal",
                SideEffects::accessExternal, "builtin_is_terminal")
                    ->arg("fd");
            addExtern<DAS_BIND_FUN(builtin_terminal_width)>(*this, lib, "terminal_width",
                SideEffects::accessExternal, "builtin_terminal_width");
            addExtern<DAS_BIND_FUN(builtin_sleep)>(*this, lib, "sleep",
                SideEffects::modifyExternal, "builtin_sleep")
                    ->arg("msec");
            addExtern<DAS_BIND_FUN(getchar_wrapper)>(*this, lib, "getchar",
                SideEffects::modifyExternal, "getchar_wrapper");
            addExtern<DAS_BIND_FUN(builtin_exit)>(*this, lib, "exit",
                SideEffects::modifyExternal, "builtin_exit")
                    ->args({"exitCode","context","line"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_popen)>(*this, lib, "popen",
                SideEffects::modifyExternal, "builtin_popen")
                    ->args({"command","scope","context","at"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_popen_binary)>(*this, lib, "popen_binary",
                SideEffects::modifyExternal, "builtin_popen_binary")
                    ->args({"command","scope","context","at"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_popen_timeout)>(*this, lib, "popen_timeout",
                SideEffects::modifyExternal, "builtin_popen_timeout")
                    ->args({"command","timeout","scope","context","at"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_spawn_argv)>(*this, lib, "spawn_argv",
                SideEffects::modifyExternal, "builtin_spawn_argv")
                    ->args({"args","context","at"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_popen_argv)>(*this, lib, "popen_argv",
                SideEffects::modifyExternal, "builtin_popen_argv")
                    ->args({"args","timeout","scope","context","at"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_popen_argv_pipe)>(*this, lib, "popen_argv_pipe",
                SideEffects::modifyExternal, "builtin_popen_argv_pipe")
                    ->args({"args","scope","context","at"})->unsafeOperation = true;
            addConstant<int32_t>(*this, "popen_timed_out", DAS_POPEN_TIMEOUT);
            addExtern<DAS_BIND_FUN(builtin_system)>(*this, lib, "system",
                SideEffects::modifyExternal, "builtin_system")
                    ->args({"command","context","at"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(get_full_file_name)>(*this, lib, "get_full_file_name",
                SideEffects::accessExternal, "get_full_file_name")
                    ->args({"path","context","at"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(builtin_resolve_this_module_dir)>(*this, lib, "__builtin_resolve_this_module_dir",
                SideEffects::accessExternal, "builtin_resolve_this_module_dir")
                    ->args({"baked_path","standalone","context"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(get_env_variable)>(*this, lib, "get_env_variable",
                SideEffects::accessExternal, "get_env_variable")
                    ->args({"var","context","at"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(set_env_variable)>(*this, lib, "set_env_variable",
                SideEffects::modifyExternal, "set_env_variable")
                    ->args({"var","value","context","at"});
            addExtern<DAS_BIND_FUN(has_env_variable)>(*this, lib, "has_env_variable",
                SideEffects::accessExternal, "has_env_variable")
                    ->args({"var","context","at"});
            addExtern<DAS_BIND_FUN(register_dynamic_module)>(*this, lib, "register_dynamic_module",
                SideEffects::worstDefault, "register_dynamic_module")
                    ->args({"path", "name", "on_error", "context","at"});
            addExtern<DAS_BIND_FUN(register_dynamic_module_silent)>(*this, lib, "register_dynamic_module",
                SideEffects::worstDefault, "register_dynamic_module")
                    ->args({"path", "name", "context","at"});
            addExtern<DAS_BIND_FUN(register_native_path)>(*this, lib, "register_native_path",
                SideEffects::worstDefault, "register_native_path")
                    ->args({"mod_name", "src", "dst", "context","at"});
            addExtern<DAS_BIND_FUN(for_each_registered_dynamic_module)>(*this, lib, "for_each_registered_dynamic_module",
                SideEffects::accessExternal, "for_each_registered_dynamic_module")
                    ->args({"block", "context","at"});
            addExtern<DAS_BIND_FUN(for_each_registered_native_path)>(*this, lib, "for_each_registered_native_path",
                SideEffects::accessExternal, "for_each_registered_native_path")
                    ->args({"block", "context","at"});
            addExtern<DAS_BIND_FUN(sanitize_command_line)>(*this, lib, "sanitize_command_line",
                SideEffects::none, "sanitize_command_line")
                    ->args({"var","context","at"})->setTempStringResult();
            // filesystem operations (C++17 <filesystem>)
            addAnnotation(new DiskSpaceInfoAnnotation(lib));
            // path manipulation
            addExtern<DAS_BIND_FUN(builtin_fs_extension)>(*this, lib, "extension",
                SideEffects::none, "builtin_fs_extension")
                    ->args({"path","context","at"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(builtin_fs_stem)>(*this, lib, "stem",
                SideEffects::none, "builtin_fs_stem")
                    ->args({"path","context","at"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(builtin_fs_replace_extension)>(*this, lib, "replace_extension",
                SideEffects::none, "builtin_fs_replace_extension")
                    ->args({"path","new_ext","context","at"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(builtin_fs_join)>(*this, lib, "path_join",
                SideEffects::none, "builtin_fs_join")
                    ->args({"a","b","context","at"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(builtin_fs_normalize)>(*this, lib, "normalize",
                SideEffects::none, "builtin_fs_normalize")
                    ->args({"path","context","at"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(builtin_fs_is_absolute)>(*this, lib, "is_absolute",
                SideEffects::none, "builtin_fs_is_absolute")
                    ->arg("path");
            addExtern<DAS_BIND_FUN(builtin_fs_relative)>(*this, lib, "relative",
                SideEffects::modifyArgumentAndExternal, "builtin_fs_relative")
                    ->args({"path","base","error","context","at"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(builtin_fs_parent)>(*this, lib, "parent",
                SideEffects::none, "builtin_fs_parent")
                    ->args({"path","context","at"})->setTempStringResult();
            // file queries
            addExtern<DAS_BIND_FUN(builtin_fs_file_size)>(*this, lib, "file_size",
                SideEffects::modifyArgumentAndExternal, "builtin_fs_file_size")
                    ->args({"path","error","context","at"});
            addExtern<DAS_BIND_FUN(builtin_fs_equivalent)>(*this, lib, "equivalent",
                SideEffects::modifyArgumentAndExternal, "builtin_fs_equivalent")
                    ->args({"a","b","error","context","at"});
            addExtern<DAS_BIND_FUN(builtin_fs_is_symlink)>(*this, lib, "is_symlink",
                SideEffects::modifyArgumentAndExternal, "builtin_fs_is_symlink")
                    ->args({"path","error","context","at"});
            // file operations
            addExtern<DAS_BIND_FUN(builtin_fs_copy_file)>(*this, lib, "copy_file",
                SideEffects::modifyArgumentAndExternal, "builtin_fs_copy_file")
                    ->args({"src","dst","overwrite","error","context","at"});
            addExtern<DAS_BIND_FUN(builtin_fs_set_mtime)>(*this, lib, "set_mtime",
                SideEffects::modifyArgumentAndExternal, "builtin_fs_set_mtime")
                    ->args({"path","time","error","context","at"});
            // directory operations
            addExtern<DAS_BIND_FUN(builtin_fs_dir_rec)>(*this, lib, "builtin_dir_rec",
                SideEffects::modifyArgumentAndExternal, "builtin_fs_dir_rec")
                    ->args({"path","block","error","context","at"});
            // system queries
            addExtern<DAS_BIND_FUN(builtin_fs_temp_directory)>(*this, lib, "temp_directory",
                SideEffects::modifyArgumentAndExternal, "builtin_fs_temp_directory")
                    ->args({"error","context","at"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(builtin_fs_create_temp_file)>(*this, lib, "create_temp_file",
                SideEffects::modifyArgumentAndExternal, "builtin_fs_create_temp_file")
                    ->args({"prefix","ext","error","context","at"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(builtin_fs_create_temp_directory)>(*this, lib, "create_temp_directory",
                SideEffects::modifyArgumentAndExternal, "builtin_fs_create_temp_directory")
                    ->args({"prefix","error","context","at"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(builtin_fs_disk_space)>(*this, lib, "builtin_disk_space",
                SideEffects::modifyArgumentAndExternal, "builtin_fs_disk_space")
                    ->args({"path","info","error","context","at"});
            // lets verify all names
            uint32_t verifyFlags = uint32_t(VerifyBuiltinFlags::verifyAll);
            verifyFlags &= ~VerifyBuiltinFlags::verifyHandleTypes;  // we skip annotatins due to FILE and FStat
            verifyBuiltinNames(verifyFlags);
            // and now its aot ready
            verifyAotReady();
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            tw << "#include \"daScript/misc/performance_time.h\"\n";
            tw << "#include \"daScript/simulate/aot_builtin_fio.h\"\n";
            return ModuleAotType::cpp;
        }
    };
}

#if _WIN32 && !DAS_NO_FILEIO

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void * mmap (void* start, size_t length, int prot, int /*flags*/, int fd, off_t offset) {
    HANDLE hmap;
    void* temp;
    size_t len;
    struct _stat64 st;   // plain fstat truncates st_size to 32 bits and fails past 2GB
    uint64_t o = offset;
    uint32_t l = o & 0xFFFFFFFF;
    uint32_t h = (o >> 32) & 0xFFFFFFFF;
    if (_fstat64(fd, &st) != 0)
        return MAP_FAILED;
    len = (size_t)st.st_size;
    if ((length + offset) > len)
        length = len - offset;
    // PAGE_READONLY, not PAGE_WRITECOPY: a copy-on-write section makes Windows reserve backing
    // store for every page that COULD be privatized, so it charges commit for the whole view —
    // a 73 GB gguf mapping showed up as 73 GB of private bytes on top of the model itself
    // (measured 2026-07-27: 149.9 GB private for a 78 GB image). Read-only maps charge nothing,
    // match what POSIX already does here (PROT_READ), and fault loudly on an accidental write
    // instead of silently privatizing the page. PROT_WRITE = a real shared writable view
    // (PAGE_READWRITE, writes go back to the file) — the fmap_open_rw path; still no commit charge.
    DWORD page = (prot & PROT_WRITE) ? PAGE_READWRITE : PAGE_READONLY;
    DWORD access = (prot & PROT_WRITE) ? (FILE_MAP_READ | FILE_MAP_WRITE) : FILE_MAP_READ;
    hmap = CreateFileMapping((HANDLE)_get_osfhandle(fd), 0, page, 0, 0, 0);
    if (!hmap)
        return MAP_FAILED;
    temp = MapViewOfFileEx(hmap, access, h, l, length, start);
    if (!CloseHandle(hmap))
        fprintf(stderr, "unable to close file mapping handle\n");
    return temp ? temp : MAP_FAILED;
}

int munmap ( void* start, size_t ) {
    return !UnmapViewOfFile(start);
}

#endif

// ===== prefetch: advisory readahead over a mapped range =====

#if DAS_NO_FILEIO

bool das_prefetch_map ( void *, uint64_t ) { return false; }

#else

// Ask the OS to fault a mapped range in AHEAD of use — the cold-read fix (on-demand page
// faults inside a parallel transcode loop serialize the lanes; measured 9x on Q4_K source).
// Advisory: a false return means the OS declined — reads still work, just cold.
bool das_prefetch_map ( void * base, uint64_t bytes ) {
    if ( !base || bytes==0 ) return false;
#if defined(_WIN32)
    // _WIN32, not _MSC_VER - clang-mingw is Windows too and has no madvise
    // the condition memoryapi.h declares WIN32_MEMORY_RANGE_ENTRY and PrefetchVirtualMemory under.
    // A host targeting Windows 7 (_WIN32_WINNT=0x0601) gets neither, and prefetch is advisory, so
    // that target reads cold instead of failing to build.
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) \
        && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)
    WIN32_MEMORY_RANGE_ENTRY range;
    range.VirtualAddress = base;
    range.NumberOfBytes = (SIZE_T) bytes;
    return PrefetchVirtualMemory(GetCurrentProcess(), 1, &range, 0) != 0;
#else
    (void)base; (void)bytes;
    return false;
#endif
#else
    return madvise(base, (size_t)bytes, MADV_WILLNEED)==0;
#endif
}

#endif

// ===== dwrite: the direct sequential writer =====

#if DAS_NO_FILEIO

void * das_dwrite_open ( const char *, uint64_t, uint64_t ) { return nullptr; }
bool das_dwrite_append ( void *, const void *, uint64_t ) { return false; }
void * das_dwrite_band ( void *, uint64_t * avail ) { if ( avail ) *avail = 0; return nullptr; }
bool das_dwrite_commit ( void *, uint64_t ) { return false; }
uint64_t das_dwrite_stat ( void *, int ) { return 0; }
bool das_dwrite_close ( void * ) { return false; }

#else

#include <thread>
#include <mutex>
#include <condition_variable>

// The bounce buffer is what makes the API take any pointer at any size: FILE_FLAG_NO_BUFFERING
// demands sector-aligned buffer, offset AND count, and O_DIRECT-class paths elsewhere want the
// same. Re-blocking costs one memcpy at memory bandwidth against a ~2 GB/s device — noise.
// Two bands double-buffer: a full band flushes on a writer thread while the caller stages the
// other, so transcode and device time overlap. One job in flight, handed off in order — the
// strictly-ascending cursor the NTFS valid-data-length watermark demands is preserved.
struct DasDirectWriter {
    uint8_t *   band = nullptr;
    uint8_t *   band2 = nullptr;   // the async double-buffer; null = synchronous fallback
    uint64_t    band_bytes = 0;
    uint64_t    fill = 0;          // bytes currently staged in `band`
    uint64_t    written = 0;       // bytes handed to the OS so far
    uint64_t    align = 1;         // sector size on win32, 1 elsewhere
    bool        ok = true;
    // where the time actually goes — staging the caller's bytes vs waiting on the device. Read
    // back with dwrite_stat; the split is what separates "our memory reads are slow" from
    // "the device is slow", which no external probe can distinguish from outside.
    uint64_t    copy_ns = 0;
    uint64_t    write_ns = 0;
    uint64_t    direct_bytes = 0;  // bypassed the band (already aligned)
    uint64_t    bounce_bytes = 0;  // staged through the band
    std::thread writer;            // spawned lazily on the first full band
    std::mutex  mtx;
    std::condition_variable cv;
    const uint8_t * job_src = nullptr;   // the one in-flight async write (null = idle)
    uint64_t    job_bytes = 0;
    bool        quit = false;
    bool        async_fail = false;
#if _WIN32
    HANDLE      h = INVALID_HANDLE_VALUE;
    wchar_t *   wpath = nullptr;   // kept for the close-time truncate (NO_BUFFERING can't set EOF)
#else
    int         fd = -1;
#endif
};

static bool das_dwrite_wait_async ( DasDirectWriter * w );
static void das_dwrite_join_async ( DasDirectWriter * w );

static const uint64_t DAS_DWRITE_DEFAULT_BAND = 16u << 20;

static inline uint64_t das_dwrite_now_ns () {
    return (uint64_t) std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

#if _WIN32

void * das_dwrite_open ( const char * path, uint64_t total_bytes, uint64_t band_bytes ) {
    if ( !path ) return nullptr;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
    if ( wlen <= 0 ) return nullptr;
    wchar_t * wpath = (wchar_t *) malloc(sizeof(wchar_t) * wlen);
    if ( !wpath ) return nullptr;
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, wlen);
    // sector size decides the alignment NO_BUFFERING enforces; fall back to 4096 if the volume
    // won't say (a path with no drive letter, a UNC share)
    DWORD sector = 4096;
    if ( wlen > 2 && wpath[1] == L':' ) {
        wchar_t root[4] = { wpath[0], L':', L'\\', 0 };
        DWORD spc = 0, bps = 0, freec = 0, totalc = 0;
        if ( GetDiskFreeSpaceW(root, &spc, &bps, &freec, &totalc) && bps ) sector = bps;
    }
    HANDLE h = CreateFileW(wpath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if ( h == INVALID_HANDLE_VALUE ) {
        free(wpath);
        return nullptr;
    }
    // preallocate: NTFS lays the run down contiguously, and because every write below is strictly
    // ascending the valid-data-length watermark advances with them — no zero-fill is ever charged
    if ( total_bytes ) {
        LARGE_INTEGER li;
        li.QuadPart = (LONGLONG) total_bytes;
        if ( SetFilePointerEx(h, li, nullptr, FILE_BEGIN) ) SetEndOfFile(h);
        li.QuadPart = 0;
        SetFilePointerEx(h, li, nullptr, FILE_BEGIN);
    }
    DasDirectWriter * w = new DasDirectWriter();
    w->align = sector;
    w->band_bytes = band_bytes ? band_bytes : DAS_DWRITE_DEFAULT_BAND;
    w->band_bytes = ((w->band_bytes + sector - 1) / sector) * sector;
    w->band = (uint8_t *) VirtualAlloc(nullptr, (SIZE_T) w->band_bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    w->h = h;
    w->wpath = wpath;
    if ( !w->band ) {
        CloseHandle(h);
        free(wpath);
        delete w;
        return nullptr;
    }
    // the async double-buffer; when this fails the writer just stays synchronous
    w->band2 = (uint8_t *) VirtualAlloc(nullptr, (SIZE_T) w->band_bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    return w;
}

// push exactly `bytes` (already a multiple of the sector size) from a sector-aligned pointer
static bool das_dwrite_raw ( DasDirectWriter * w, const uint8_t * src, uint64_t bytes ) {
    uint64_t at = 0;
    while ( at < bytes ) {
        uint64_t left = bytes - at;
        DWORD chunk = (DWORD) (left < uint64_t(1u << 30) ? left : uint64_t(1u << 30));
        DWORD wrote = 0;
        if ( !WriteFile(w->h, src + at, chunk, &wrote, nullptr) || wrote != chunk ) return false;
        at += wrote;
    }
    w->written += bytes;
    return true;
}

bool das_dwrite_close ( void * hh ) {
    DasDirectWriter * w = (DasDirectWriter *) hh;
    if ( !w ) return false;
    das_dwrite_join_async(w);   // the in-flight band lands (or fails into w->ok) before the tail
    uint64_t final_bytes = w->written + w->fill;
    if ( w->ok && w->fill ) {
        // the tail: pad to a sector so NO_BUFFERING accepts it, then truncate below
        uint64_t padded = ((w->fill + w->align - 1) / w->align) * w->align;
        memset(w->band + w->fill, 0, (size_t)(padded - w->fill));
        w->ok = das_dwrite_raw(w, w->band, padded);
        w->fill = 0;
    }
    bool ok = w->ok;
    CloseHandle(w->h);
    // NO_BUFFERING can't set a non-sector EOF, and the preallocation left the file at its full
    // reserved size — reopen normally to cut it back to what was actually appended
    if ( ok ) {
        HANDLE t = CreateFileW(w->wpath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        if ( t != INVALID_HANDLE_VALUE ) {
            LARGE_INTEGER li;
            li.QuadPart = (LONGLONG) final_bytes;
            if ( !SetFilePointerEx(t, li, nullptr, FILE_BEGIN) || !SetEndOfFile(t) ) ok = false;
            CloseHandle(t);
        } else {
            ok = false;
        }
    }
    VirtualFree(w->band, 0, MEM_RELEASE);
    if ( w->band2 ) VirtualFree(w->band2, 0, MEM_RELEASE);
    free(w->wpath);
    delete w;
    return ok;
}

#else

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

void * das_dwrite_open ( const char * path, uint64_t total_bytes, uint64_t band_bytes ) {
    if ( !path ) return nullptr;
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if ( fd < 0 ) return nullptr;
    DasDirectWriter * w = new DasDirectWriter();
    w->fd = fd;
    w->band_bytes = band_bytes ? band_bytes : DAS_DWRITE_DEFAULT_BAND;
    w->band = (uint8_t *) malloc((size_t) w->band_bytes);
    if ( !w->band ) {
        close(fd);
        delete w;
        return nullptr;
    }
    // the async double-buffer; when this fails the writer just stays synchronous
    w->band2 = (uint8_t *) malloc((size_t) w->band_bytes);
#if defined(__APPLE__)
    fcntl(fd, F_NOCACHE, 1);            // the APFS/HFS equivalent of NO_BUFFERING
    if ( total_bytes ) {
        fstore_t st = { F_ALLOCATECONTIG | F_ALLOCATEALL, F_PEOFPOSMODE, 0, (off_t) total_bytes, 0 };
        if ( fcntl(fd, F_PREALLOCATE, &st) < 0 ) {
            st.fst_flags = F_ALLOCATEALL;   // contiguous is a preference, not a requirement
            fcntl(fd, F_PREALLOCATE, &st);
        }
    }
#else
    if ( total_bytes ) {
        if ( posix_fallocate(fd, 0, (off_t) total_bytes) != 0 ) {
            // filesystems without fallocate (or a full disk) — the appends below still work,
            // they just extend the file the ordinary way
        }
    }
#endif
    return w;
}

static bool das_dwrite_raw ( DasDirectWriter * w, const uint8_t * src, uint64_t bytes ) {
    uint64_t at = 0;
    while ( at < bytes ) {
        // chunk like the Windows path: macOS write(2) REJECTS nbyte > INT_MAX with EINVAL
        // (Linux merely truncates, which is why the unchunked loop survived there) — an 11 GB
        // plane append otherwise dies instantly and reads as "disk full"
        uint64_t left = bytes - at;
        size_t chunk = (size_t)(left < (uint64_t)(1u << 30) ? left : (uint64_t)(1u << 30));
        ssize_t wrote = write(w->fd, src + at, chunk);
        if ( wrote <= 0 ) {
            if ( errno == EINTR ) continue;
            return false;
        }
        at += (uint64_t) wrote;
    }
#if !defined(__APPLE__)
    // hand the just-written range back to the OS: the producer is streaming a much larger source
    // through the same page cache, and these pages will never be read again
    off_t from = (off_t) w->written;
    posix_fadvise(w->fd, from, (off_t) bytes, POSIX_FADV_DONTNEED);
#endif
    w->written += bytes;
    return true;
}

bool das_dwrite_close ( void * hh ) {
    DasDirectWriter * w = (DasDirectWriter *) hh;
    if ( !w ) return false;
    das_dwrite_join_async(w);   // the in-flight band lands (or fails into w->ok) before the tail
    uint64_t final_bytes = w->written + w->fill;
    if ( w->ok && w->fill ) {
        w->ok = das_dwrite_raw(w, w->band, w->fill);
        w->fill = 0;
    }
    bool ok = w->ok;
    if ( ok && ftruncate(w->fd, (off_t) final_bytes) != 0 ) ok = false;
    close(w->fd);
    free(w->band);
    free(w->band2);
    delete w;
    return ok;
}

#endif

// the writer thread: one job in flight at a time, handed off in order — the async half of the
// double-buffer. Owns w->written and write_ns while a job runs (the caller waits before touching).
static void das_dwrite_thread ( DasDirectWriter * w ) {
    std::unique_lock<std::mutex> lk(w->mtx);
    for ( ;; ) {
        w->cv.wait(lk, [&]{ return w->job_src != nullptr || w->quit; });
        if ( w->job_src ) {
            const uint8_t * src = w->job_src;
            uint64_t bytes = w->job_bytes;
            lk.unlock();
            uint64_t t0 = das_dwrite_now_ns();
            bool wok = das_dwrite_raw(w, src, bytes);
            uint64_t dt = das_dwrite_now_ns() - t0;
            lk.lock();
            w->write_ns += dt;
            if ( !wok ) w->async_fail = true;
            w->job_src = nullptr;
            w->cv.notify_all();
            continue;
        }
        if ( w->quit ) break;
    }
}

// drain the in-flight band; false = the async write failed (the writer is broken)
static bool das_dwrite_wait_async ( DasDirectWriter * w ) {
    if ( !w->writer.joinable() ) return !w->async_fail;
    std::unique_lock<std::mutex> lk(w->mtx);
    w->cv.wait(lk, [&]{ return w->job_src == nullptr; });
    return !w->async_fail;
}

// close-time retirement: drain, stop, join; an async failure lands in w->ok
static void das_dwrite_join_async ( DasDirectWriter * w ) {
    if ( !das_dwrite_wait_async(w) ) w->ok = false;
    if ( w->writer.joinable() ) {
        {
            std::lock_guard<std::mutex> g(w->mtx);
            w->quit = true;
        }
        w->cv.notify_all();
        w->writer.join();
    }
}

// a full band: hand it to the writer thread and keep staging into the other; without the second
// band (alloc failed) flush synchronously exactly as before
static bool das_dwrite_flush_full_band ( DasDirectWriter * w ) {
    if ( w->band2 ) {
        if ( !das_dwrite_wait_async(w) ) return false;
        if ( !w->writer.joinable() ) w->writer = std::thread(das_dwrite_thread, w);
        {
            std::lock_guard<std::mutex> g(w->mtx);
            w->job_src = w->band;
            w->job_bytes = w->band_bytes;
        }
        w->cv.notify_all();
        std::swap(w->band, w->band2);
        w->fill = 0;
        return true;
    }
    uint64_t t0 = das_dwrite_now_ns();
    bool wok = das_dwrite_raw(w, w->band, w->band_bytes);
    w->write_ns += das_dwrite_now_ns() - t0;
    if ( !wok ) return false;
    w->fill = 0;
    return true;
}

// platform-independent half: stage into the band, push whenever it fills. A writer that has
// already failed stays failed and stops touching the disk.
bool das_dwrite_append ( void * hh, const void * data, uint64_t bytes ) {
    DasDirectWriter * w = (DasDirectWriter *) hh;
    if ( !w || !w->ok ) return false;
    const uint8_t * src = (const uint8_t *) data;
    while ( bytes ) {
        // bulk fast path: on a band boundary, a band-sized-or-larger run from an already-aligned
        // source goes straight to the device, skipping a full memcpy of the payload (~10% of wall
        // time at 2 GB/s). Checked per iteration, not once on entry — a plane rarely starts on a
        // band boundary, but it reaches one after the first partial band and the rest is bulk.
        // Synchronous by contract (the caller may retire `data` the moment we return), so the
        // in-flight band must land first to keep the cursor strictly ascending.
        if ( w->fill == 0 && bytes >= w->band_bytes && (uintptr_t(src) % w->align) == 0 ) {
            if ( !das_dwrite_wait_async(w) ) {
                w->ok = false;
                return false;
            }
            uint64_t direct = (bytes / w->align) * w->align;
            uint64_t t0 = das_dwrite_now_ns();
            bool wok = das_dwrite_raw(w, src, direct);
            w->write_ns += das_dwrite_now_ns() - t0;
            w->direct_bytes += direct;
            if ( !wok ) {
                w->ok = false;
                return false;
            }
            src += direct;
            bytes -= direct;
            continue;   // whatever is left is a sub-sector tail for the band below
        }
        uint64_t room = w->band_bytes - w->fill;
        uint64_t take = bytes < room ? bytes : room;
        uint64_t tc = das_dwrite_now_ns();
        memcpy(w->band + w->fill, src, (size_t) take);
        w->copy_ns += das_dwrite_now_ns() - tc;
        w->bounce_bytes += take;
        w->fill += take;
        src += take;
        bytes -= take;
        if ( w->fill == w->band_bytes ) {
            if ( !das_dwrite_flush_full_band(w) ) {
                w->ok = false;
                return false;
            }
        }
    }
    return true;
}

// band/commit: hand the producer the writer's own aligned staging buffer so it can build bytes
// straight into it — no intermediate allocation, no bounce copy. `avail` is what's left in the
// current band; commit what you filled and the band flushes itself when it tops out.
// 0 = ns spent staging caller bytes into the band, 1 = ns spent inside write syscalls,
// 2 = bytes written straight from the caller, 3 = bytes staged through the band.
uint64_t das_dwrite_stat ( void * hh, int which ) {
    DasDirectWriter * w = (DasDirectWriter *) hh;
    if ( !w ) return 0;
    std::lock_guard<std::mutex> g(w->mtx);   // write_ns updates on the writer thread
    switch ( which ) {
        case 0: return w->copy_ns;
        case 1: return w->write_ns;
        case 2: return w->direct_bytes;
        case 3: return w->bounce_bytes;
        default: return 0;
    }
}

void * das_dwrite_band ( void * hh, uint64_t * avail ) {
    DasDirectWriter * w = (DasDirectWriter *) hh;
    if ( !w || !w->ok ) {
        if ( avail ) *avail = 0;
        return nullptr;
    }
    if ( avail ) *avail = w->band_bytes - w->fill;
    return w->band + w->fill;
}

bool das_dwrite_commit ( void * hh, uint64_t bytes ) {
    DasDirectWriter * w = (DasDirectWriter *) hh;
    if ( !w || !w->ok ) return false;
    if ( w->fill + bytes > w->band_bytes ) return false;   // the producer overran its own band
    w->fill += bytes;
    if ( w->fill == w->band_bytes ) {
        if ( !das_dwrite_flush_full_band(w) ) {
            w->ok = false;
            return false;
        }
    }
    return true;
}

#endif

REGISTER_MODULE_IN_NAMESPACE(Module_FIO,das);
