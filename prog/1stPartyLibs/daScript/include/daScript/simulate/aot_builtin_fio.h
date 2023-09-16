#pragma once

namespace das {
    struct FStat;
    class Context;
    struct Block;
    struct SimNode_CallBase;
    struct LineInfoArg;

    const FILE * builtin_fopen  ( const char * name, const char * mode );
    void builtin_fclose ( const FILE * f, Context * context, LineInfoArg * at );
    void builtin_fflush ( const FILE * f, Context * context, LineInfoArg * at );
    void builtin_fprint ( const FILE * f, const char * text, Context * context, LineInfoArg * at );
    char * builtin_fread ( const FILE * _f, Context * context, LineInfoArg * at );
    char* builtin_fgets(const FILE* _f, Context* context, LineInfoArg * at );
    void builtin_fwrite(const FILE * _f, char * str, Context * context, LineInfoArg * at );
    bool builtin_feof(const FILE* _f);
    int64_t builtin_ftell ( const FILE * f, Context * context, LineInfoArg * at );
    int64_t builtin_fseek ( const FILE * f, int64_t offset, int32_t mode, Context * context, LineInfoArg * at );
    vec4f builtin_read ( Context &, SimNode_CallBase * call, vec4f * args );
    vec4f builtin_write ( Context &, SimNode_CallBase * call, vec4f * args );
    vec4f builtin_load ( Context & context, SimNode_CallBase *, vec4f * args );
    void builtin_map_file ( const FILE* _f, const TBlock<void, TTemporary<TArray<uint8_t>>>& blk, Context*, LineInfoArg * at );
    char * builtin_dirname ( const char * name, Context * context, LineInfoArg * at );
    char * builtin_basename ( const char * name, Context * context, LineInfoArg * at );
    bool builtin_fstat ( const FILE * f, FStat & fs, Context * context, LineInfoArg * at );
    bool builtin_stat ( const char * filename, FStat & fs );
    void builtin_dir ( const char * path, const Block & fblk, Context * context, LineInfoArg * at );
    bool builtin_mkdir ( const char * path );
    const FILE * builtin_stdin();
    const FILE * builtin_stdout();
    const FILE * builtin_stderr();
    int builtin_popen ( const char * cmd, const TBlock<void,const FILE *> & blk, Context * context, LineInfoArg * at );
    int builtin_popen_binary ( const char * cmd, const TBlock<void,const FILE *> & blk, Context * context, LineInfoArg * at );
    char * get_full_file_name ( const char * path, Context * context, LineInfoArg * );
    bool builtin_remove_file ( const char * path );
    char * get_env_variable ( const char * var, Context * context );
    char * sanitize_command_line ( const char * cmd, Context * context, LineInfoArg * at );
}
