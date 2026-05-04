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

#include <sstream>

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

    void Module_BuiltIn::addTime(ModuleLibrary & lib) {
        addAnnotation(make_smart<TimeAnnotation>(lib));
        addExtern<DAS_BIND_FUN(builtin_clock)>(*this, lib, "get_clock", SideEffects::modifyExternal, "builtin_clock");
        addExtern<DAS_BIND_FUN(builtin_mktime)>(*this, lib, "mktime", SideEffects::modifyExternal, "builtin_mktime")
            ->args({"year","month","mday","hour","min","sec"});
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
    int builtin_system ( const char * cmd, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * get_full_file_name ( const char * path, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    bool has_env_variable ( const char * var, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
    char * get_env_variable ( const char * var, Context * context, LineInfoArg * at ) GENERATE_IO_STUB
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
#undef GENERATE_IO_STUB

#define GENERATE_IO_STUB { DAS_FATAL_ERROR("%s is not implemented (because DAS_NO_FILEIO is enabled).", __FUNCTION__); }
#define GENERATE_IO_STUB_RET { GENERATE_IO_STUB; return {}; }
    void builtin_sleep ( uint32_t msec ) GENERATE_IO_STUB
    const FILE * builtin_stdin() GENERATE_IO_STUB_RET
    const FILE * builtin_stdout() GENERATE_IO_STUB_RET
    const FILE * builtin_stderr() GENERATE_IO_STUB_RET
    bool builtin_feof(const FILE* _f) GENERATE_IO_STUB_RET
    const FILE * builtin_fopen  ( const char * name, const char * mode ) GENERATE_IO_STUB_RET
    vec4f builtin_read ( Context & context, SimNode_CallBase * call, vec4f * args ) GENERATE_IO_STUB_RET
    vec4f builtin_write ( Context & context, SimNode_CallBase * call, vec4f * args ) GENERATE_IO_STUB_RET
    vec4f builtin_load ( Context & context, SimNode_CallBase * node, vec4f * args ) GENERATE_IO_STUB_RET
    bool builtin_stat ( const char * filename, FStat & fs ) GENERATE_IO_STUB_RET
    bool builtin_chdir ( const char * path ) GENERATE_IO_STUB_RET
    bool builtin_mkdir ( const char * path ) GENERATE_IO_STUB_RET
    void builtin_exit ( int32_t ec ) GENERATE_IO_STUB
    bool builtin_remove_file ( const char * path ) GENERATE_IO_STUB_RET
    bool builtin_rename_file ( const char * old_path, const char * new_path ) GENERATE_IO_STUB_RET
    bool builtin_rmdir ( const char * path ) GENERATE_IO_STUB_RET
    bool builtin_fexist ( const char * path ) GENERATE_IO_STUB_RET
    bool builtin_rmdir_rec ( const char * path ) GENERATE_IO_STUB_RET
    bool builtin_fs_is_absolute ( const char * path ) GENERATE_IO_STUB_RET
    int64_t builtin_fs_file_size ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB_RET
    bool builtin_fs_equivalent ( const char * a, const char * b, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB_RET
    bool builtin_fs_is_symlink ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB_RET
    bool builtin_fs_copy_file ( const char * src, const char * dst, bool overwrite, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB_RET
    bool builtin_fs_set_mtime ( const char * path, Time time, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB_RET
    bool builtin_fs_disk_space ( const char * path, DiskSpaceInfo & info, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB_RET
    bool builtin_remove_file_ec ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB_RET
    bool builtin_rename_file_ec ( const char * old_path, const char * new_path, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB_RET
    bool builtin_mkdir_ec ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB_RET
    bool builtin_rmdir_ec ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB_RET
    bool builtin_rmdir_rec_ec ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) GENERATE_IO_STUB_RET
#undef GENERATE_IO_STUB
#undef GENERATE_IO_STUB_RET


    class Module_FIO : public Module {
    public:
        Module_FIO() : Module("fio") {
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            return ModuleAotType::cpp;
        }
    };

}
#else // DAS_NO_FILEIO

#include <thread>
#include <atomic>
#include <chrono>
#if _WIN32
#include <fcntl.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <signal.h>
#include <sys/wait.h>
#endif

#include <filesystem>
#include <fstream>
#include <random>

MAKE_TYPE_FACTORY(FStat, das::FStat)
MAKE_TYPE_FACTORY(FILE,FILE)
MAKE_TYPE_FACTORY(DiskSpaceInfo, das::DiskSpaceInfo)

namespace das {
    void builtin_sleep ( uint32_t msec ) {
#if defined(_MSC_VER)
        _sleep(msec);
#else
        usleep(1000 * msec);
#endif
    }

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


    void builtin_fprint ( const FILE * f, const char * text, Context * context, LineInfoArg * at ) {
        if ( !f ) context->throw_error_at(at, "can't fprint NULL");
        if ( text ) fputs(text,(FILE *)f);
    }

    const FILE * builtin_fopen  ( const char * name, const char * mode ) {
        if ( name && mode ) {
            FILE * f = fopen(name, mode);
            if ( f ) setvbuf(f, NULL, _IOFBF, 65536);
            return f;
        } else {
            return nullptr;
        }
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

    bool builtin_feof(const FILE* _f) {
        FILE* f = (FILE*)_f;
        return feof(f);
    }

    void builtin_map_file(const FILE* f, const TBlock<void, TTemporary<TArray<uint8_t>>>& blk, Context* context, LineInfoArg * at) {
        if ( !f ) context->throw_error_at(at, "can't map NULL file");
        struct stat st;
        int fd = fileno((FILE *)f);
        fstat(fd, &st);
        void* data = mmap(nullptr, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
        Array arr;
        array_mark_locked(arr, data, uint32_t(st.st_size));
        vec4f args[1];
        args[0] = cast<Array *>::from(&arr);
        context->invoke(blk, args, nullptr, at);
        munmap(data, st.st_size);
    }

    int64_t builtin_ftell ( const FILE * f, Context * context, LineInfoArg * at ) {
        if ( !f ) context->throw_error_at(at, "can't ftell NULL");
        return ftell((FILE *)f);
    }

    int64_t builtin_fseek ( const FILE * f, int64_t offset, int32_t mode, Context * context, LineInfoArg * at ) {
        if ( !f ) context->throw_error_at(at, "can't fseek NULL");
        return fseek((FILE *)f, long(offset), mode);
    }

    char * builtin_fread ( const FILE * f, Context * context, LineInfoArg * at ) {
        if ( !f ) context->throw_error_at(at, "can't fread NULL");
        struct stat st;
        int fd = fileno((FILE*)f);
        fstat(fd, &st);
        char * res = context->allocateString(nullptr, st.st_size,at);
        fseek((FILE*)f, 0, SEEK_SET);
        auto bytes = fread(res, 1, st.st_size, (FILE *)f);
        if ( uint64_t(bytes) != uint64_t(st.st_size) ) {
            context->throw_error_at(at, "incorrect fread result, expected %lu, got %lu bytes. read requires binary file mode",
                (unsigned long)st.st_size, (unsigned long)bytes);
        }
        return res;
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
        int32_t res = (int32_t) fwrite(buf,1,len,fp);
        return cast<int32_t>::from(res);
    }

#ifdef _MSC_VER
#pragma warning(pop)
#endif

    // loads(file,block<data>)
    vec4f builtin_load ( Context & context, SimNode_CallBase * node, vec4f * args ) {
        auto fp = cast<FILE *>::to(args[0]);
        if ( !fp ) context.throw_error_at(node->debugInfo, "can't load NULL");
        int32_t len = cast<int32_t>::to(args[1]);
        if (len < 0) context.throw_error_at(node->debugInfo, "can't read negative number from binary save, %d", len);
        Block * block = cast<Block *>::to(args[2]);
        char * buf = (char *) malloc(len + 1);
        if (!buf) context.throw_error_at(node->debugInfo, "can't read. out of memory, %d", len);
        vec4f bargs[1];
        int32_t rlen = int32_t(fread(buf, 1, len, fp));
        if ( rlen != len ) {
            bargs[0] = v_zero();
            context.invoke(*block, bargs, nullptr, &node->debugInfo);
        }  else {
            buf[rlen] = 0;
            das::Array arr;
            array_mark_locked(arr, buf, uint32_t(rlen));
            bargs[0] = cast<das::Array*>::from(&arr);
            context.invoke(*block, bargs, nullptr, &node->debugInfo);
        }
        free(buf);
        return v_zero();
    }


    char * builtin_dirname ( const char * name, Context * context, LineInfoArg * at ) {
        if ( name ) {
#if defined(_MSC_VER)
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
#if defined(_MSC_VER)
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
        return fstat(fileno((FILE *)f), &fs.stats) == 0;
    }

    bool builtin_stat ( const char * filename, FStat & fs ) {
        if ( filename!=nullptr ) {
            return stat(filename, &fs.stats) == 0;
        } else {
            return false;
        }
    }

     void builtin_dir ( const char * path, const Block & fblk, Context * context, LineInfoArg * at ) {
#if defined(_MSC_VER)
        _finddata_t c_file;
        intptr_t hFile;
        string findPath = string(path ? path : "") + "/*";
        if ((hFile = _findfirst(findPath.c_str(), &c_file)) != -1L) {
            do {
                char * fname = context->allocateString(c_file.name, uint32_t(strlen(c_file.name)),at);
                vec4f args[1] = {
                    cast<char *>::from(fname)
                };
                context->invoke(fblk, args, nullptr, at);
            } while (_findnext(hFile, &c_file) == 0);
        }
        _findclose(hFile);
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
            return chdir(path) == 0;
        } else {
            return false;
        }
#endif
    }

    char * builtin_getcwd ( Context * context, LineInfoArg * at) {
#if defined(_EMSCRIPTEN_VER)
        return nullptr;
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
    }

    bool builtin_mkdir ( const char * path ) {
        if ( path ) {
#if defined(_MSC_VER)
            return _mkdir(path) == 0;
#elif defined(_EMSCRIPTEN_VER)
            return mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
#else
            return mkdir(path, ACCESSPERMS) == 0;
#endif
        } else {
            return false;
        }
    }

    void builtin_exit ( int32_t ec ) {
        exit(ec);
    }

    int builtin_popen_impl ( const char * cmd, bool bin, const TBlock<void,const FILE *> & blk, Context * context, LineInfoArg * at ) {
        if ( !cmd ) {
            context->throw_error_at(at, "popen of null");
            return -1;
        }
#ifdef _MSC_VER
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
#ifdef _MSC_VER
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
#ifdef _MSC_VER
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
        // Create job object to kill entire process tree on timeout
        HANDLE hJob = CreateJobObjectA(NULL, NULL);
        if ( hJob ) {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli;
            memset(&jeli, 0, sizeof(jeli));
            jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
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
        return remove(path) == 0;
    }

    bool builtin_rename_file ( const char * old_path, const char * new_path ) {
        if ( !old_path || !new_path ) return false;
        return rename(old_path, new_path) == 0;
    }

    bool builtin_rmdir ( const char * path ) {
        if ( !path ) return false;
#if defined(_MSC_VER)
        return _rmdir(path) == 0;
#else
        return rmdir(path) == 0;
#endif
    }

    bool builtin_fexist ( const char * path ) {
        if ( !path ) return false;
        struct stat st;
        return stat(path, &st) == 0;
    }

#if defined(_MSC_VER)
    static bool rmdir_rec_impl ( const string & path ) {
        _finddata_t c_file;
        intptr_t hFile;
        string findPath = path + "/*";
        if ((hFile = _findfirst(findPath.c_str(), &c_file)) != -1L) {
            do {
                if ( strcmp(c_file.name, ".") == 0 || strcmp(c_file.name, "..") == 0 ) continue;
                string child = path + "/" + c_file.name;
                if ( c_file.attrib & _A_SUBDIR ) {
                    if ( !rmdir_rec_impl(child) ) { _findclose(hFile); return false; }
                } else {
                    if ( c_file.attrib & _A_RDONLY ) _chmod(child.c_str(), _S_IREAD | _S_IWRITE);
                    if ( remove(child.c_str()) != 0 ) { _findclose(hFile); return false; }
                }
            } while (_findnext(hFile, &c_file) == 0);
            _findclose(hFile);
        }
        return _rmdir(path.c_str()) == 0;
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
            if ( stat(child.c_str(), &st) != 0 ) { closedir(dir); return false; }
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
        return rmdir_rec_impl(string(path));
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
        if ( remove(path) != 0 ) { error = errno_to_string(ctx, at); return false; }
        return true;
    }

    bool builtin_rename_file_ec ( const char * old_path, const char * new_path, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !old_path || !new_path ) { error = empty_path_error(ctx, at); return false; }
        if ( rename(old_path, new_path) != 0 ) { error = errno_to_string(ctx, at); return false; }
        return true;
    }

    bool builtin_mkdir_ec ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !path ) { error = empty_path_error(ctx, at); return false; }
#if defined(_MSC_VER)
        if ( _mkdir(path) != 0 ) { error = errno_to_string(ctx, at); return false; }
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
#if defined(_MSC_VER)
        if ( _rmdir(path) != 0 ) { error = errno_to_string(ctx, at); return false; }
#else
        if ( rmdir(path) != 0 ) { error = errno_to_string(ctx, at); return false; }
#endif
        return true;
    }

    bool has_env_variable ( const char * var, Context * , LineInfoArg * ) {
        if ( !var ) return false;
        auto res = getenv(var);
        return res != nullptr;
    }

    char * get_env_variable ( const char * var, Context * context, LineInfoArg * at ) {
        if ( !var ) return nullptr;
        auto res = getenv(var);
        if ( !res ) return nullptr;
        return context->allocateString(res, at);
    }

    enum class RegisterOnError {
        Quiet = 0,      // 'missing .shared_module' message is ignored
        ErrorMsg,       // error message is displayed
        Fail,           // error message is displayed and exception is thrown
    };

    // Returns DLL handle.
    void *register_dynamic_module(const char *path, const char *mod_name, int on_error, Context * context, LineInfoArg * at ) {
        auto lib = loadDynamicLibrary(path);
        if (!lib) {
            if (static_cast<RegisterOnError>(on_error) != RegisterOnError::Quiet) {
                auto err_msg = "dynamic module `" + string(mod_name) + "` — library not found: " + string(path) + "\n";
                context->to_err(at, err_msg.c_str());
                if (static_cast<RegisterOnError>(on_error) == RegisterOnError::Fail) {
                    context->throw_error(err_msg.c_str());
                }
            }
            return nullptr;
        }
        const auto regName = getDynModuleRegistratorName(mod_name);
        auto rawFn = getFunctionAddress(lib, regName.c_str());
        if (!rawFn) {
            auto err_msg = "dynamic module `" + string(mod_name) + "` — function `" + regName + "` not found in `" + path + "`\n";
            context->to_err(at, err_msg.c_str());
            if (static_cast<RegisterOnError>(on_error) == RegisterOnError::Fail) {
                context->throw_error(err_msg.c_str());
            }
            closeLibrary(lib);
            return nullptr;
        }
        auto fn = reinterpret_cast<Module*(*)(int)>(rawFn);
        auto mod = fn(DAS_BUILD_ID);
        if (!mod) {
            auto err_msg = "dynamic module `" + string(mod_name) + "` — build-id mismatch (host " + to_string(DAS_BUILD_ID) + "); rebuild the module for current configuration\n";
            context->to_err(at, err_msg.c_str());
            if (static_cast<RegisterOnError>(on_error) == RegisterOnError::Fail) {
                context->throw_error(err_msg.c_str());
            }
            closeLibrary(lib);
            return nullptr;
        }
        *ModuleKarma += unsigned(intptr_t(mod));
        return lib;
    }
    void *register_dynamic_module_silent(const char *path, const char *mod_name, Context * context, LineInfoArg * at ) {
        return register_dynamic_module(path, mod_name, static_cast<int>(RegisterOnError::Quiet), context, at);
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
    }

    char * sanitize_command_line ( const char * cmd, Context * context, LineInfoArg * at ) {
        if ( !cmd ) return nullptr;
        stringstream ss;
        for ( const char * ch=cmd; *ch; ) {
#if defined(_MSC_VER)
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

    // path manipulation

    char * builtin_fs_extension ( const char * path, Context * ctx, LineInfoArg * at ) {
        if ( !path ) return nullptr;
        auto ext = std::filesystem::path(path).extension().string();
        if ( ext.empty() ) return nullptr;
        return ctx->allocateString(ext.data(), uint32_t(ext.size()), at);
    }

    char * builtin_fs_stem ( const char * path, Context * ctx, LineInfoArg * at ) {
        if ( !path ) return nullptr;
        auto s = std::filesystem::path(path).stem().string();
        if ( s.empty() ) return nullptr;
        return ctx->allocateString(s.data(), uint32_t(s.size()), at);
    }

    char * builtin_fs_replace_extension ( const char * path, const char * new_ext, Context * ctx, LineInfoArg * at ) {
        if ( !path ) return nullptr;
        auto p = std::filesystem::path(path);
        p.replace_extension(new_ext ? new_ext : "");
        auto s = p.string();
        return ctx->allocateString(s.data(), uint32_t(s.size()), at);
    }

    char * builtin_fs_join ( const char * a, const char * b, Context * ctx, LineInfoArg * at ) {
        if ( !a && !b ) return nullptr;
        std::filesystem::path pa = a ? a : "";
        if ( b ) pa /= b;
        auto s = pa.string();
        return ctx->allocateString(s.data(), uint32_t(s.size()), at);
    }

    char * builtin_fs_normalize ( const char * path, Context * ctx, LineInfoArg * at ) {
        if ( !path ) return nullptr;
        auto s = std::filesystem::path(path).lexically_normal().string();
        if ( s.empty() ) return nullptr;
        return ctx->allocateString(s.data(), uint32_t(s.size()), at);
    }

    bool builtin_fs_is_absolute ( const char * path ) {
        if ( !path ) return false;
        return std::filesystem::path(path).is_absolute();
    }

    char * builtin_fs_relative ( const char * path, const char * base, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !path || !base ) { error = empty_path_error(ctx, at); return nullptr; }
        std::error_code ec;
        auto s = std::filesystem::relative(std::filesystem::path(path), std::filesystem::path(base), ec).string();
        if ( ec ) { error = ec_to_string(ec, ctx, at); return nullptr; }
        if ( s.empty() ) return nullptr;
        return ctx->allocateString(s.data(), uint32_t(s.size()), at);
    }

    char * builtin_fs_parent ( const char * path, Context * ctx, LineInfoArg * at ) {
        if ( !path ) return nullptr;
        auto s = std::filesystem::path(path).parent_path().string();
        if ( s.empty() ) return nullptr;
        return ctx->allocateString(s.data(), uint32_t(s.size()), at);
    }

    // file queries

    int64_t builtin_fs_file_size ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !path ) { error = empty_path_error(ctx, at); return -1; }
        std::error_code ec;
        auto sz = std::filesystem::file_size(path, ec);
        if ( ec ) { error = ec_to_string(ec, ctx, at); return -1; }
        return static_cast<int64_t>(sz);
    }

    bool builtin_fs_equivalent ( const char * a, const char * b, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !a || !b ) { error = empty_path_error(ctx, at); return false; }
        std::error_code ec;
        bool result = std::filesystem::equivalent(a, b, ec);
        if ( ec ) { error = ec_to_string(ec, ctx, at); return false; }
        return result;
    }

    bool builtin_fs_is_symlink ( const char * path, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !path ) { error = empty_path_error(ctx, at); return false; }
        std::error_code ec;
        bool result = std::filesystem::is_symlink(path, ec);
        if ( ec ) { error = ec_to_string(ec, ctx, at); return false; }
        return result;
    }

    // file operations

    bool builtin_fs_copy_file ( const char * src, const char * dst, bool overwrite, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !src || !dst ) { error = empty_path_error(ctx, at); return false; }
        std::error_code ec;
        auto opts = overwrite ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none;
        bool result = std::filesystem::copy_file(src, dst, opts, ec);
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
        std::filesystem::last_write_time(path, file_time, ec);
        if ( ec ) { error = ec_to_string(ec, ctx, at); return false; }
        return true;
    }

    // directory operations

    void builtin_fs_dir_rec ( const char * path, const TBlock<void, char *, bool> & blk, char * & error, Context * ctx, LineInfoArg * at ) {
        error = nullptr;
        if ( !path ) { error = empty_path_error(ctx, at); return; }
        std::filesystem::path root(path);
        std::error_code ec;
        auto it = std::filesystem::recursive_directory_iterator(root,
                std::filesystem::directory_options::skip_permission_denied, ec);
        if ( ec ) { error = ec_to_string(ec, ctx, at); return; }
        for ( auto & entry : it ) {
            if ( ec ) { ec.clear(); continue; }
            auto rel = std::filesystem::relative(entry.path(), root, ec).string();
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
        auto s = std::filesystem::temp_directory_path(ec).string();
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
            auto p = tmp / name.c_str();
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
        std::filesystem::remove_all(path, ec);
        if ( ec ) { error = ec_to_string(ec, ctx, at); return false; }
        return true;
    }

    class Module_FIO : public Module {
    public:
        Module_FIO() : Module("fio_core") {
            DAS_PROFILE_SECTION("Module_FIO");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            addBuiltinDependency(lib, Module::require("strings"));
            // type
            addAnnotation(make_smart<DummyTypeAnnotation>("FILE", "FILE", 16, 16));
            addAnnotation(make_smart<FStatAnnotation>(lib));
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
                    ->args({"name","mode"})->setNoDiscard();
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
                    ->args({"file","context","line"});
            addExtern<DAS_BIND_FUN(builtin_map_file)>(*this, lib, "fmap",
                SideEffects::modifyExternal, "builtin_map_file")
                    ->args({"file","block","context","line"});
            addExtern<DAS_BIND_FUN(builtin_fgets)>(*this, lib, "fgets",
                SideEffects::modifyExternal, "builtin_fgets")
                    ->args({"file","context","line"});
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
            addInterop<builtin_load,void,const FILE*,int32_t,const Block &>(*this, lib, "_builtin_load",
                das::SideEffects::modifyExternal, "builtin_load")
                    ->args({"file","length","block"});
            addExtern<DAS_BIND_FUN(builtin_dirname)>(*this, lib, "dir_name",
                SideEffects::none, "builtin_dirname")
                    ->args({"name","context","line"});
            addExtern<DAS_BIND_FUN(builtin_basename)>(*this, lib, "base_name",
                SideEffects::none, "builtin_basename")
                    ->args({"name","context","line"});
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
                    ->args({"context","at"});
            addExtern<DAS_BIND_FUN(builtin_stdin)>(*this, lib, "fstdin",
                SideEffects::modifyExternal, "builtin_stdin");
            addExtern<DAS_BIND_FUN(builtin_stdout)>(*this, lib, "fstdout",
                SideEffects::modifyExternal, "builtin_stdout");
            addExtern<DAS_BIND_FUN(builtin_stderr)>(*this, lib, "fstderr",
                SideEffects::modifyExternal, "builtin_stderr");
            addExtern<DAS_BIND_FUN(builtin_sleep)>(*this, lib, "sleep",
                SideEffects::modifyExternal, "builtin_sleep")
                    ->arg("msec");
            addExtern<DAS_BIND_FUN(getchar_wrapper)>(*this, lib, "getchar",
                SideEffects::modifyExternal, "getchar_wrapper");
            addExtern<DAS_BIND_FUN(builtin_exit)>(*this, lib, "exit",
                SideEffects::modifyExternal, "builtin_exit")
                    ->arg("exitCode")->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_popen)>(*this, lib, "popen",
                SideEffects::modifyExternal, "builtin_popen")
                    ->args({"command","scope","context","at"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_popen_binary)>(*this, lib, "popen_binary",
                SideEffects::modifyExternal, "builtin_popen_binary")
                    ->args({"command","scope","context","at"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_popen_timeout)>(*this, lib, "popen_timeout",
                SideEffects::modifyExternal, "builtin_popen_timeout")
                    ->args({"command","timeout","scope","context","at"})->unsafeOperation = true;
            addConstant<int32_t>(*this, "popen_timed_out", DAS_POPEN_TIMEOUT);
            addExtern<DAS_BIND_FUN(builtin_system)>(*this, lib, "system",
                SideEffects::modifyExternal, "builtin_system")
                    ->args({"command","context","at"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(get_full_file_name)>(*this, lib, "get_full_file_name",
                SideEffects::accessExternal, "get_full_file_name")
                    ->args({"path","context","at"});
            addExtern<DAS_BIND_FUN(get_env_variable)>(*this, lib, "get_env_variable",
                SideEffects::accessExternal, "get_env_variable")
                    ->args({"var","context","at"});
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
            addExtern<DAS_BIND_FUN(sanitize_command_line)>(*this, lib, "sanitize_command_line",
                SideEffects::none, "sanitize_command_line")
                    ->args({"var","context","at"});
            // filesystem operations (C++17 <filesystem>)
            addAnnotation(make_smart<DiskSpaceInfoAnnotation>(lib));
            // path manipulation
            addExtern<DAS_BIND_FUN(builtin_fs_extension)>(*this, lib, "extension",
                SideEffects::none, "builtin_fs_extension")
                    ->args({"path","context","at"});
            addExtern<DAS_BIND_FUN(builtin_fs_stem)>(*this, lib, "stem",
                SideEffects::none, "builtin_fs_stem")
                    ->args({"path","context","at"});
            addExtern<DAS_BIND_FUN(builtin_fs_replace_extension)>(*this, lib, "replace_extension",
                SideEffects::none, "builtin_fs_replace_extension")
                    ->args({"path","new_ext","context","at"});
            addExtern<DAS_BIND_FUN(builtin_fs_join)>(*this, lib, "path_join",
                SideEffects::none, "builtin_fs_join")
                    ->args({"a","b","context","at"});
            addExtern<DAS_BIND_FUN(builtin_fs_normalize)>(*this, lib, "normalize",
                SideEffects::none, "builtin_fs_normalize")
                    ->args({"path","context","at"});
            addExtern<DAS_BIND_FUN(builtin_fs_is_absolute)>(*this, lib, "is_absolute",
                SideEffects::none, "builtin_fs_is_absolute")
                    ->arg("path");
            addExtern<DAS_BIND_FUN(builtin_fs_relative)>(*this, lib, "relative",
                SideEffects::modifyArgumentAndExternal, "builtin_fs_relative")
                    ->args({"path","base","error","context","at"});
            addExtern<DAS_BIND_FUN(builtin_fs_parent)>(*this, lib, "parent",
                SideEffects::none, "builtin_fs_parent")
                    ->args({"path","context","at"});
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
                    ->args({"error","context","at"});
            addExtern<DAS_BIND_FUN(builtin_fs_create_temp_file)>(*this, lib, "create_temp_file",
                SideEffects::modifyArgumentAndExternal, "builtin_fs_create_temp_file")
                    ->args({"prefix","ext","error","context","at"});
            addExtern<DAS_BIND_FUN(builtin_fs_create_temp_directory)>(*this, lib, "create_temp_directory",
                SideEffects::modifyArgumentAndExternal, "builtin_fs_create_temp_directory")
                    ->args({"prefix","error","context","at"});
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

#if _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void * mmap (void* start, size_t length, int /*prot*/, int /*flags*/, int fd, off_t offset) {
    HANDLE hmap;
    void* temp;
    size_t len;
    struct stat st;
    uint64_t o = offset;
    uint32_t l = o & 0xFFFFFFFF;
    uint32_t h = (o >> 32) & 0xFFFFFFFF;
    fstat(fd, &st);
    len = (size_t)st.st_size;
    if ((length + offset) > len)
        length = len - offset;
    hmap = CreateFileMapping((HANDLE)_get_osfhandle(fd), 0, PAGE_WRITECOPY, 0, 0, 0);
    if (!hmap)
        return MAP_FAILED;
    temp = MapViewOfFileEx(hmap, FILE_MAP_COPY, h, l, length, start);
    if (!CloseHandle(hmap))
        fprintf(stderr, "unable to close file mapping handle\n");
    return temp ? temp : MAP_FAILED;
}

int munmap ( void* start, size_t ) {
    return !UnmapViewOfFile(start);
}

#endif

#endif // !DAS_NO_FILEIO
REGISTER_MODULE_IN_NAMESPACE(Module_FIO,das);
