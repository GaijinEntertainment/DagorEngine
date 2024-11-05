#include "daScript/misc/platform.h"

#include "module_builtin.h"

#include "daScript/simulate/aot_builtin_fio.h"

#include "daScript/simulate/simulate_nodes.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_policy_types.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/simulate/aot_builtin_time.h"

#include "daScript/misc/performance_time.h"
#include "daScript/misc/sysos.h"

#include <sstream>

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



#if !DAS_NO_FILEIO

#include <sys/stat.h>

#if defined(_MSC_VER)

#include <io.h>
#include <direct.h>

#else
#include <libgen.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#endif

namespace das {
    struct FStat {
        struct stat stats;
        bool        is_valid;
        uint64_t size() const   { return stats.st_size; }
#if defined(_MSC_VER)
        Time     atime() const  { return { stats.st_atime }; }
        Time     ctime() const  { return { stats.st_ctime }; }
        Time     mtime() const  { return { stats.st_mtime }; }
        bool     is_reg() const { return stats.st_mode & _S_IFREG; }
        bool     is_dir() const { return stats.st_mode & _S_IFDIR; }

#else
        Time     atime() const  { return { stats.st_atime }; }
        Time     ctime() const  { return { stats.st_ctime }; }
        Time     mtime() const  { return { stats.st_mtime }; }
        bool     is_reg() const { return S_ISREG(stats.st_mode); }
        bool     is_dir() const { return S_ISDIR(stats.st_mode); }

#endif
    };
}


MAKE_TYPE_FACTORY(FStat, das::FStat)
MAKE_TYPE_FACTORY(FILE,FILE)

namespace das {
    void builtin_sleep ( uint32_t msec ) {
#if defined(_MSC_VER)
        _sleep(msec);
#else
        usleep(1000 * msec);
#endif
    }

    #include "fio.das.inc"

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
        arr.data = (char *) data;
        arr.capacity = arr.size = uint32_t(st.st_size);
        arr.lock = 1;
        arr.flags = 0;
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
            arr.data = buf;
            arr.size = rlen;
            arr.capacity = rlen;
            arr.lock = 1;
            arr.flags = 0;
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

    char * get_env_variable ( const char * var, Context * context, LineInfoArg * at ) {
        if ( !var ) return nullptr;
        auto res = getenv(var);
        if ( !res ) return nullptr;
        return context->allocateString(res, at);
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

    class Module_FIO : public Module {
    public:
        Module_FIO() : Module("fio") {
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
            addExtern<DAS_BIND_FUN(builtin_rename_file)>(*this, lib, "rename",
                SideEffects::modifyExternal, "builtin_rename_file")
                    ->args({"old_name","new_name"});
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
                SideEffects::modifyExternal, "builtin_fstat")
                    ->args({"file","stat","context","line"});
            addExtern<DAS_BIND_FUN(builtin_stat)>(*this, lib, "stat",
                SideEffects::modifyExternal, "builtin_stat")
                    ->args({"file","stat"});
            addExtern<DAS_BIND_FUN(builtin_dir)>(*this, lib, "builtin_dir",
                SideEffects::modifyExternal, "builtin_dir")
                    ->args({"path","block","context","line"});
            addExtern<DAS_BIND_FUN(builtin_mkdir)>(*this, lib, "mkdir",
                SideEffects::modifyExternal, "builtin_mkdir")
                    ->arg("path");
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
                SideEffects::modifyExternal, "getchar");
            addExtern<DAS_BIND_FUN(builtin_exit)>(*this, lib, "exit",
                SideEffects::modifyExternal, "builtin_exit")
                    ->arg("exitCode")->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_popen)>(*this, lib, "popen",
                SideEffects::modifyExternal, "builtin_popen")
                    ->args({"command","scope","context","at"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_popen_binary)>(*this, lib, "popen_binary",
                SideEffects::modifyExternal, "builtin_popen_binary")
                    ->args({"command","scope","context","at"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(get_full_file_name)>(*this, lib, "get_full_file_name",
                SideEffects::accessExternal, "get_full_file_name")
                    ->args({"path","context","at"});
            addExtern<DAS_BIND_FUN(get_env_variable)>(*this, lib, "get_env_variable",
                SideEffects::accessExternal, "get_env_variable")
                    ->args({"var","context","at"});
            addExtern<DAS_BIND_FUN(sanitize_command_line)>(*this, lib, "sanitize_command_line",
                SideEffects::none, "sanitize_command_line")
                    ->args({"var","context","at"});
            // add builtin module
            compileBuiltinModule("fio.das",fio_das, sizeof(fio_das));
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

REGISTER_MODULE_IN_NAMESPACE(Module_FIO,das);

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

#endif
