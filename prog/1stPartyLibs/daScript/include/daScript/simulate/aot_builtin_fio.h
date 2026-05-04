#pragma once

#include <daScript/simulate/aot_builtin_time.h>

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
#endif

namespace das {
    struct FStat;
    class Context;
    struct Block;
    struct SimNode_CallBase;
    struct LineInfoArg;

#if !DAS_NO_FILEIO

    struct FStat {
        struct stat stats;
        bool        is_valid;
        uint64_t size() const   { return stats.st_size; }
        Time     atime() const  { return { stats.st_atime }; }
        Time     ctime() const  { return { stats.st_ctime }; }
        Time     mtime() const  { return { stats.st_mtime }; }
#if defined(_MSC_VER)
        bool     is_reg() const { return stats.st_mode & _S_IFREG; }
        bool     is_dir() const { return stats.st_mode & _S_IFDIR; }
#else
        bool     is_reg() const { return S_ISREG(stats.st_mode); }
        bool     is_dir() const { return S_ISDIR(stats.st_mode); }
#endif
    };
#endif

    DAS_API const FILE * builtin_fopen  ( const char * name, const char * mode );
    DAS_API void builtin_fclose ( const FILE * f, Context * context, LineInfoArg * at );
    DAS_API void builtin_fflush ( const FILE * f, Context * context, LineInfoArg * at );
    DAS_API void builtin_fprint ( const FILE * f, const char * text, Context * context, LineInfoArg * at );
    DAS_API char * builtin_fread ( const FILE * _f, Context * context, LineInfoArg * at );
    DAS_API char* builtin_fgets(const FILE* _f, Context* context, LineInfoArg * at );
    DAS_API void builtin_fwrite(const FILE * _f, char * str, Context * context, LineInfoArg * at );
    DAS_API bool builtin_feof(const FILE* _f);
    DAS_API int64_t builtin_ftell ( const FILE * f, Context * context, LineInfoArg * at );
    DAS_API int64_t builtin_fseek ( const FILE * f, int64_t offset, int32_t mode, Context * context, LineInfoArg * at );
    DAS_API vec4f builtin_read ( Context &, SimNode_CallBase * call, vec4f * args );
    DAS_API vec4f builtin_write ( Context &, SimNode_CallBase * call, vec4f * args );
    DAS_API vec4f builtin_load ( Context & context, SimNode_CallBase *, vec4f * args );
    DAS_API void builtin_map_file ( const FILE* _f, const TBlock<void, TTemporary<TArray<uint8_t>>>& blk, Context*, LineInfoArg * at );
    DAS_API char * builtin_dirname ( const char * name, Context * context, LineInfoArg * at );
    DAS_API char * builtin_basename ( const char * name, Context * context, LineInfoArg * at );
    DAS_API bool builtin_fstat ( const FILE * f, FStat & fs, Context * context, LineInfoArg * at );
    DAS_API bool builtin_stat ( const char * filename, FStat & fs );
    DAS_API void builtin_dir ( const char * path, const Block & fblk, Context * context, LineInfoArg * at );
    DAS_API bool builtin_mkdir ( const char * path );
    DAS_API bool builtin_chdir ( const char * path );
    DAS_API char * builtin_getcwd ( Context * context, LineInfoArg * at );
    DAS_API const FILE * builtin_stdin();
    DAS_API const FILE * builtin_stdout();
    DAS_API const FILE * builtin_stderr();
    DAS_API int builtin_popen ( const char * cmd, const TBlock<void,const FILE *> & blk, Context * context, LineInfoArg * at );
    DAS_API int builtin_popen_binary ( const char * cmd, const TBlock<void,const FILE *> & blk, Context * context, LineInfoArg * at );
    DAS_API int builtin_popen_timeout ( const char * cmd, float timeout_sec, const TBlock<void,const FILE *> & blk, Context * context, LineInfoArg * at );
    DAS_API char * get_full_file_name ( const char * path, Context * context, LineInfoArg * );
    DAS_API bool builtin_remove_file ( const char * path );
    DAS_API bool builtin_rename_file ( const char * old_path, const char * new_path );
    DAS_API bool builtin_rmdir ( const char * path );
    DAS_API bool builtin_fexist ( const char * path );
    DAS_API bool builtin_rmdir_rec ( const char * path );
    DAS_API bool has_env_variable ( const char * var, Context * context, LineInfoArg * at );
    DAS_API char * get_env_variable ( const char * var, Context * context, LineInfoArg * at );
    DAS_API char * sanitize_command_line ( const char * cmd, Context * context, LineInfoArg * at );
    // filesystem operations (C++17 <filesystem>)
    struct DiskSpaceInfo {
        uint64_t capacity = 0;
        uint64_t free = 0;
        uint64_t available = 0;
    };
    // path manipulation
    DAS_API char * builtin_fs_extension ( const char * path, Context * context, LineInfoArg * at );
    DAS_API char * builtin_fs_stem ( const char * path, Context * context, LineInfoArg * at );
    DAS_API char * builtin_fs_replace_extension ( const char * path, const char * new_ext, Context * context, LineInfoArg * at );
    DAS_API char * builtin_fs_join ( const char * a, const char * b, Context * context, LineInfoArg * at );
    DAS_API char * builtin_fs_normalize ( const char * path, Context * context, LineInfoArg * at );
    DAS_API bool   builtin_fs_is_absolute ( const char * path );
    DAS_API char * builtin_fs_relative ( const char * path, const char * base, char * & error, Context * context, LineInfoArg * at );
    DAS_API char * builtin_fs_parent ( const char * path, Context * context, LineInfoArg * at );
    // file queries
    DAS_API int64_t builtin_fs_file_size ( const char * path, char * & error, Context * context, LineInfoArg * at );
    DAS_API bool    builtin_fs_equivalent ( const char * a, const char * b, char * & error, Context * context, LineInfoArg * at );
    DAS_API bool    builtin_fs_is_symlink ( const char * path, char * & error, Context * context, LineInfoArg * at );
    // file operations
    DAS_API bool    builtin_fs_copy_file ( const char * src, const char * dst, bool overwrite, char * & error, Context * context, LineInfoArg * at );
    DAS_API bool    builtin_fs_set_mtime ( const char * path, Time time, char * & error, Context * context, LineInfoArg * at );
    // directory operations
    DAS_API void    builtin_fs_dir_rec ( const char * path, const TBlock<void, char *, bool> & blk, char * & error, Context * context, LineInfoArg * at );
    // system queries
    DAS_API char *  builtin_fs_temp_directory ( char * & error, Context * context, LineInfoArg * at );
    DAS_API char *  builtin_fs_create_temp_file ( const char * prefix, const char * ext, char * & error, Context * context, LineInfoArg * at );
    DAS_API char *  builtin_fs_create_temp_directory ( const char * prefix, char * & error, Context * context, LineInfoArg * at );
    DAS_API bool    builtin_fs_disk_space ( const char * path, DiskSpaceInfo & info, char * & error, Context * context, LineInfoArg * at );
    // POSIX operations with error reporting
    DAS_API bool    builtin_remove_file_ec ( const char * path, char * & error, Context * context, LineInfoArg * at );
    DAS_API bool    builtin_rename_file_ec ( const char * old_path, const char * new_path, char * & error, Context * context, LineInfoArg * at );
    DAS_API bool    builtin_mkdir_ec ( const char * path, char * & error, Context * context, LineInfoArg * at );
    DAS_API bool    builtin_rmdir_ec ( const char * path, char * & error, Context * context, LineInfoArg * at );
    DAS_API bool    builtin_rmdir_rec_ec ( const char * path, char * & error, Context * context, LineInfoArg * at );
}
