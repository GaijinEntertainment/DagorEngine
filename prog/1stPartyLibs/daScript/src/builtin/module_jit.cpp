#include "daScript/misc/platform.h"

#include "daScript/misc/performance_time.h"
#include "daScript/misc/sysos.h"
#include "daScript/simulate/aot.h"
#include "daScript/simulate/aot_builtin_jit.h"
#include "daScript/simulate/aot_builtin_ast.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/misc/fpe.h"
#include "daScript/misc/sysos.h"
#include "misc/include_fmt.h"
#include "module_builtin_rtti.h"
#include "module_builtin_ast.h"

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

namespace das {

    float4 das_invoke_code ( void * pfun, vec4f anything, void * cmres, Context * context ) {
        vec4f * arguments = cast<vec4f *>::to(anything);
        vec4f (*fun)(Context *, vec4f *, void *) = (vec4f(*)(Context *, vec4f *, void *)) pfun;
        vec4f res = fun ( context, arguments, cmres );
        return res;
    }

    bool das_is_jit_function ( const Func func ) {
        auto simfn = func.PTR;
        if ( !simfn ) return false;
        return simfn->code && simfn->code->rtti_node_isJit();
    }

    bool das_remove_jit ( const Func func ) {
        auto simfn = func.PTR;
        if ( !simfn ) return false;
        if ( simfn->code && simfn->code->rtti_node_isJit() ) {
            auto jitNode = static_cast<SimNode_Jit *>(simfn->code);
            simfn->code = jitNode->saved_code;
            simfn->aot = jitNode->saved_aot;
            simfn->aotFunction = jitNode->saved_aot_function;
            simfn->jit = false;
            return true;
        } else {
            return false;
        }
    }

    bool das_instrument_jit ( void * pfun, const Func func, Context & context ) {
        auto simfn = func.PTR;
        if ( !simfn ) return false;
        if ( simfn->code && simfn->code->rtti_node_isJit() ) {
            auto jitNode = static_cast<SimNode_Jit *>(simfn->code);
            jitNode->func = (JitFunction) pfun;
        } else {
            auto node = context.code->makeNode<SimNode_Jit>(LineInfo(), (JitFunction)pfun);
            node->saved_code = simfn->code;
            node->saved_aot = simfn->aot;
            node->saved_aot_function = simfn->aotFunction;
            simfn->code = node;
            simfn->aot = false;
            simfn->aotFunction = nullptr;
            simfn->jit = true;
        }
        return true;
    }

extern "C" {
    void jit_exception ( const char * text, Context * context, LineInfoArg * at ) {
        context->throw_error_at(at, "%s", text ? text : "");
    }

    vec4f jit_call_or_fastcall ( SimFunction * fn, vec4f * args, Context * context ) {
        return context->callOrFastcall(fn, args, nullptr);
    }

    vec4f jit_invoke_block ( const Block & blk, vec4f * args, Context * context ) {
        return context->invoke(blk, args, nullptr, nullptr);
    }

    vec4f jit_invoke_block_with_cmres ( const Block & blk, vec4f * args, void * cmres, Context * context ) {
        return context->invoke(blk, args, cmres, nullptr);
    }

    vec4f jit_call_with_cmres ( SimFunction * fn, vec4f * args, void * cmres, Context * context ) {
        return context->callWithCopyOnReturn(fn, args, cmres, nullptr);
    }

    char * jit_string_builder ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        StringBuilderWriter writer;
        DebugDataWalker<StringBuilderWriter> walker(writer, PrintFlags::string_builder);
        for ( int i=0, is=call->nArguments; i!=is; ++i ) {
            walker.walk(args[i], call->types[i]);
        }
        auto length = writer.tellp();
        if ( length ) {
            return context.allocateString(writer.c_str(), uint32_t(length),&call->debugInfo);
        } else {
            return nullptr;
        }
    }

    char * jit_string_builder_temp ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        StringBuilderWriter writer;
        DebugDataWalker<StringBuilderWriter> walker(writer, PrintFlags::string_builder);
        for ( int i=0, is=call->nArguments; i!=is; ++i ) {
            walker.walk(args[i], call->types[i]);
        }
        auto length = writer.tellp();
        if ( length ) {
            auto str = context.allocateString(writer.c_str(), uint32_t(length),&call->debugInfo);
            context.freeTempString(str,&call->debugInfo);
            return str;
        } else {
            return nullptr;
        }
    }


    void * jit_get_global_mnh ( uint64_t mnh, Context & context ) {
        return context.globals + context.globalOffsetByMangledName(mnh);
    }

    void * jit_get_shared_mnh ( uint64_t mnh, Context & context ) {
        return context.shared + context.globalOffsetByMangledName(mnh);
    }

    void * jit_alloc_heap ( uint32_t bytes, Context * context ) {
        auto ptr = context->allocate(bytes);
        if ( !ptr ) context->throw_out_of_memory(false, bytes);
        return ptr;
    }

    void * jit_alloc_persistent ( uint32_t bytes, Context * ) {
        return das_aligned_alloc16(bytes);
    }

    void jit_free_heap ( void * bytes, uint32_t size, Context * context ) {
        context->free((char *)bytes,size);
    }

    void jit_free_persistent ( void * bytes, Context * ) {
        das_aligned_free16(bytes);
    }

    void jit_array_lock ( const Array & arr, Context * context, LineInfoArg * at ) {
        builtin_array_lock(arr, context, at);
    }

    void jit_array_unlock ( const Array & arr, Context * context, LineInfoArg * at ) {
        builtin_array_unlock(arr, context, at);
    }

    int32_t jit_str_cmp ( char * a, char * b ) {
        return strcmp(a ? a : "",b ? b : "");
    }

    char * jit_str_cat ( const char * sA, const char * sB, Context * context, LineInfoArg * at ) {
        sA = sA ? sA : "";
        sB = sB ? sB : "";
        auto la = stringLength(*context, sA);
        auto lb = stringLength(*context, sB);
        uint32_t commonLength = la + lb;
        if ( !commonLength ) {
            return nullptr;
        } else if ( char * sAB = (char * ) context->allocateString(nullptr, commonLength, at) ) {
            memcpy ( sAB, sA, la );
            memcpy ( sAB+la, sB, lb+1 );
            context->stringHeap->recognize(sAB);
            return sAB;
        } else {
            context->throw_out_of_memory(true, commonLength);
            return nullptr;
        }
    }

    struct JitStackState {
        char * EP;
        char * SP;
    };

    void jit_prologue ( void * funcLineInfo, int32_t stackSize, JitStackState * stackState, Context * context, LineInfoArg * at ) {
        if (!context->stack.push(stackSize, stackState->EP, stackState->SP)) {
            context->throw_error_at(at, "stack overflow");
        }
#if DAS_ENABLE_STACK_WALK
        Prologue * pp = (Prologue *)context->stack.sp();
        pp->info = nullptr;
        pp->fileName = "`jit`";
        pp->functionLine = (LineInfo *) funcLineInfo;
        pp->stackSize = stackSize;
#endif
    }

    void jit_epilogue ( JitStackState * stackState, Context * context ) {
        context->stack.pop(stackState->EP, stackState->SP);
    }

    void jit_make_block ( Block * blk, int32_t argStackTop, uint64_t ad, void * bodyNode, void * jitImpl, void * funcInfo, Context * context ) {
        JitBlock * block = (JitBlock *) blk;
        block->stackOffset = context->stack.spi();
        block->argumentsOffset = argStackTop ? (context->stack.spi() + argStackTop) : 0;
        block->body = (SimNode *)(void*) block->node;
        block->aotFunction = nullptr;
        block->jitFunction = jitImpl;
        block->functionArguments = context->abiArguments();
        block->info = (FuncInfo *) funcInfo;
        new (block->node) SimNode_JitBlock(LineInfo(), (JitBlockFunction) bodyNode, blk, ad);
    }

    void jit_debug ( vec4f res, TypeInfo * typeInfo, char * message, Context * context, LineInfoArg * at ) {
        FPE_DISABLE;
        TextWriter ssw;
        if ( message ) ssw << message << " ";
        ssw << debug_type(typeInfo) << " = " << debug_value(res, typeInfo, PrintFlags::debugger) << "\n";
        context->to_out(at, ssw.str().c_str());
    }

    bool jit_iterator_iterate ( const das::Sequence &it, void *data, das::Context *context ) {
        return builtin_iterator_iterate(it, data, context);
    }

    void jit_iterator_delete ( const das::Sequence &it, das::Context *context ) {
        return builtin_iterator_delete(it, context);
    }

    void jit_iterator_close ( const das::Sequence &it, void *data, das::Context *context ) {
        return builtin_iterator_close(it, data, context);
    }

    bool jit_iterator_first ( const das::Sequence &it, void *data, das::Context *context, das::LineInfoArg *at ) {
        return builtin_iterator_first(it, data, context, at);
    }

    bool jit_iterator_next ( const das::Sequence &it, void *data, das::Context *context, das::LineInfoArg *at ) {
        return builtin_iterator_next(it, data, context, at);
    }

    void jit_debug_enter ( char * message, Context * context, LineInfoArg * at ) {
        TextWriter tw;
        tw << string(context->fnDepth, '\t'); context->fnDepth ++;
        tw << ">>";
        if ( !context->name.empty() ) tw << "(" << context->name << ")";
        tw << ": ";
        if ( message ) tw << message;
        if ( at && at->line ) tw << " at " << at->describe();
        tw << "\n";
        context->to_out(at, tw.str().c_str());
    }

    void jit_debug_exit ( char * message, Context * context, LineInfoArg * at ) {
        TextWriter tw;
        context->fnDepth --; tw << string(context->fnDepth, '\t');
        tw << " -";
        if ( !context->name.empty() ) tw << "(" << context->name << ")";
        tw << ": ";
        if ( message ) tw << message;
        if ( at && at->line ) tw << " at " << at->describe();
        tw << "\n";
        context->to_out(at, tw.str().c_str());
    }

    void jit_debug_line ( char * message, Context * context, LineInfoArg * at ) {
        TextWriter tw;
        tw << string(context->fnDepth + 1, '\t');
        tw << ">>";
        if ( !context->name.empty() ) tw << "(" << context->name << ")";
        tw << ": ";
        if ( message ) tw << message;
        if ( at && at->line ) tw << " at " << at->describe();
        tw << "\n";
        context->to_out(at, tw.str().c_str());
    }

    void jit_initialize_fileinfo ( void * dummy ) {
        new (dummy) FileInfo();
        auto fileInfoPtr = reinterpret_cast<FileInfo*>(dummy);
        fileInfoPtr->name = "jit_generated_fileinfo";
    }

    void * jit_ast_typedecl ( uint64_t hash, Context * context, LineInfoArg * at ) {
        if ( !context->thisProgram ) context->throw_error_at(at, "can't get ast_typeinfo, no program. is 'options rtti' missing?");
        auto ti = context->thisProgram->astTypeInfo.find(hash);
        if ( ti==context->thisProgram->astTypeInfo.end() ) {
            context->throw_error_at(at, "can't find ast_typeinfo for hash %" PRIx64, hash);
        }
        auto info = ti->second;
        info->addRef();
        return (void*) info;
    }
}

    void *das_get_jit_exception() { return (void *)&jit_exception; }
    void *das_get_jit_call_or_fastcall() { return (void *)&jit_call_or_fastcall; }
    void *das_get_jit_invoke_block() { return (void *)&jit_invoke_block; }
    void *das_get_jit_invoke_block_with_cmres() { return (void *)&jit_invoke_block_with_cmres; }
    void *das_get_jit_call_with_cmres() { return (void *)&jit_call_with_cmres; }
    void *das_get_jit_string_builder() { return (void *)&jit_string_builder; }
    void *das_get_jit_string_builder_temp() { return (void *)&jit_string_builder_temp; }
    void *das_get_jit_get_global_mnh() { return (void *)&jit_get_global_mnh; }
    void *das_get_jit_get_shared_mnh() { return (void *)&jit_get_shared_mnh; }
    void *das_get_jit_alloc_heap() { return (void *)&jit_alloc_heap; }
    void *das_get_jit_alloc_persistent() { return (void *)&jit_alloc_persistent; }
    void *das_get_jit_free_heap() { return (void *)&jit_free_heap; }
    void *das_get_jit_free_persistent() { return (void *)&jit_free_persistent; }
    void *das_get_jit_array_lock() { return (void *)&builtin_array_lock; }
    void *das_get_jit_array_unlock() { return (void *)&builtin_array_unlock; }
    void *das_get_jit_str_cmp() { return (void *)&jit_str_cmp; }
    void *das_get_jit_prologue() { return (void *)&jit_prologue; }
    void *das_get_jit_epilogue() { return (void *)&jit_epilogue; }
    void *das_get_jit_make_block() { return (void *)&jit_make_block; }
    void *das_get_jit_debug() { return (void *)&jit_debug; }
    void *das_get_jit_iterator_iterate() { return (void *)&builtin_iterator_iterate; }
    void *das_get_jit_iterator_delete() { return (void *)&builtin_iterator_delete; }
    void *das_get_jit_iterator_close() { return (void *)&builtin_iterator_close; }
    void *das_get_jit_iterator_first() { return (void *)&builtin_iterator_first; }
    void *das_get_jit_iterator_next() { return (void *)&builtin_iterator_next; }
    void *das_get_jit_str_cat() { return (void *)&jit_str_cat; }
    void *das_get_jit_debug_enter() { return (void *)&jit_debug_enter; }
    void *das_get_jit_debug_exit() { return (void *)&jit_debug_exit; }
    void *das_get_jit_debug_line() { return (void *)&jit_debug_line; }
    void *das_get_jit_initialize_fileinfo () { return (void*)&jit_initialize_fileinfo; }
    void *das_get_jit_ast_typedecl () { return (void*)&jit_ast_typedecl; }

    template <typename KeyType>
    int32_t jit_table_at ( Table * tab, KeyType key, int32_t valueTypeSize, Context * context, LineInfoArg * at ) {
        TableHash<KeyType> thh(context,valueTypeSize);
        auto hfn = hash_function(*context, key);
        return thh.reserve(*tab, key, hfn, at);
    }

    void * das_get_jit_table_at ( int32_t baseType, Context * context, LineInfoArg * at ) {
        JIT_TABLE_FUNCTION(jit_table_at);
    }

    template <typename KeyType>
    bool jit_table_erase ( Table * tab, KeyType key, int32_t valueTypeSize, Context * context, LineInfoArg * at ) {
        if ( tab->isLocked() ) context->throw_error_at(at, "can't erase from locked table");
        TableHash<KeyType> thh(context,valueTypeSize);
        auto hfn = hash_function(*context, key);
        return thh.erase(*tab, key, hfn) != -1;
    }

    void * das_get_jit_table_erase ( int32_t baseType, Context * context, LineInfoArg * at ) {
        JIT_TABLE_FUNCTION(jit_table_erase);
    }

    template <typename KeyType>
    int32_t jit_table_find ( Table * tab, KeyType key, int32_t valueTypeSize, Context * context ) {
        TableHash<KeyType> thh(context,valueTypeSize);
        auto hfn = hash_function(*context, key);
        return thh.find(*tab, key, hfn);
    }

    void * das_get_jit_table_find ( int32_t baseType, Context * context, LineInfoArg * at ) {
        JIT_TABLE_FUNCTION(jit_table_find);
    }

    void * das_get_jit_new ( TypeDeclPtr htype, Context * context, LineInfoArg * at ) {
        if ( !htype ) context->throw_error_at(at, "can't get `new`, type is null");
        if ( !htype->isHandle() ) context->throw_error_at(at, "can't get `new`, type is not a handle");
        return htype->annotation->jitGetNew();
    }

    void * das_get_jit_delete ( TypeDeclPtr htype, Context * context, LineInfoArg * at ) {
        if ( !htype ) context->throw_error_at(at, "can't get `delete`, type is null");
        if ( !htype->isHandle() ) context->throw_error_at(at, "can't get `delete`, type is not a handle");
        return htype->annotation->jitGetDelete();
    }

    void * das_instrument_line_info ( const LineInfo & info, Context * context, LineInfoArg * at ) {
        LineInfo * info_ptr = (LineInfo *) context->code->allocate(sizeof(LineInfo));
        if ( !info_ptr ) context->throw_error_at(at, "can't instrument line info, out of code heap");
        *info_ptr = info;
        return (void *) info_ptr;
    }

    void das_recreate_fileinfo_name ( FileInfo * info, const char * name, Context *, LineInfoArg *  ) {
        info->name = string{ name };
    }

    bool check_file_present ( const char * filename ) {
        if ( FILE * file = fopen(filename, "r"); file == NULL ) {
            return false;
        } else {
            fclose(file);
            return true;
        }
    }

#if (defined(_MSC_VER) || defined(__linux__) || defined(__APPLE__)) && !defined(_GAMING_XBOX) && !defined(_DURANGO)
    void create_shared_library ( const char * objFilePath, const char * libraryName, const char * jitModuleObj ) {
        char cmd[1024];

        if (!check_file_present(jitModuleObj)) {
            das_to_stderr("Error: File '%s', containing daScript library, does not exist\n", jitModuleObj);
            return;
        }
        if (!check_file_present(objFilePath)) {
            das_to_stderr("Error: File '%s', containing compiled definitions, does not exist\n", objFilePath);
            return;
        }

        #if defined(_WIN32) || defined(_WIN64)
            auto result = fmt::format_to(cmd, "clang-cl {} {} -link -DLL -OUT:{} 2>&1", objFilePath, jitModuleObj, libraryName);
        #elif defined(__APPLE__)
            auto result = fmt::format_to(cmd, "clang -shared -o {} {} {} 2>&1", libraryName, objFilePath, jitModuleObj);
        #else
            auto result = fmt::format_to(cmd, "gcc -shared -o {} {} {} 2>&1", libraryName, objFilePath, jitModuleObj);
        #endif
            *result = '\0';

#if defined(_WIN32) || defined(_WIN64)
    #define popen _popen
    #define pclose _pclose
#endif

        FILE * fp = popen(cmd, "r");
        if ( fp == NULL ) {
            das_to_stderr("Failed to run command '%s'\n", cmd);
            return;
        }

        static constexpr int MAX_OUTPUT_SIZE = 16 * 1024;

        char buffer[1024], output[MAX_OUTPUT_SIZE];
        output[0] = '\0';

        size_t output_length = 0;

        // Read the output a line at a time and accumulate it
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            size_t buffer_length = strlen(buffer);
            if (output_length + buffer_length < MAX_OUTPUT_SIZE) {
                strcat(output, buffer);
                output_length += buffer_length;
            } else {
                strncat(output, buffer, MAX_OUTPUT_SIZE - output_length - 1);
                break;
            }
        }

        if ( int status = pclose(fp); status != 0 ) {
            das_to_stderr("Failed to make shared library %s, command '%s'\n", libraryName, cmd);
            printf("Output:\n%s", output);
        } else {
            das_to_stdout("Library %s made - ok\n", libraryName);
        }
    }
#else
    void create_shared_library ( const char * , const char * , const char *  ) { }
#endif

    class Module_Jit : public Module {
    public:
        Module_Jit() : Module("jit") {
            DAS_PROFILE_SECTION("Module_Jit");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            addBuiltinDependency(lib, Module::require("rtti"));
            addBuiltinDependency(lib, Module::require("ast"));
            addExtern<DAS_BIND_FUN(das_invoke_code)>(*this, lib, "invoke_code",
                SideEffects::worstDefault, "das_invoke_code")
                    ->args({"code","arguments","cmres","context"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(das_instrument_jit)>(*this, lib, "instrument_jit",
                SideEffects::worstDefault, "das_instrument_jit")
                    ->args({"code","function","context"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(das_remove_jit)>(*this, lib, "remove_jit",
                SideEffects::worstDefault, "das_remove_jit")
                    ->args({"function"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(das_instrument_line_info)>(*this, lib, "instrument_line_info",
                SideEffects::worstDefault, "das_instrument_line_info")
                    ->args({"info","context","at"});
            addExtern<DAS_BIND_FUN(das_is_jit_function)>(*this, lib, "is_jit_function",
                SideEffects::worstDefault, "das_is_jit_function")
                    ->args({"function"});
            addExtern<DAS_BIND_FUN(das_get_jit_exception)>(*this, lib, "get_jit_exception",
                SideEffects::none, "das_get_jit_exception");
            addExtern<DAS_BIND_FUN(das_get_jit_call_or_fastcall)>(*this, lib, "get_jit_call_or_fastcall",
                SideEffects::none, "das_get_jit_call_or_fastcall");
            addExtern<DAS_BIND_FUN(das_get_jit_call_with_cmres)>(*this, lib, "get_jit_call_with_cmres",
                SideEffects::none, "das_get_jit_call_with_cmres");
            addExtern<DAS_BIND_FUN(das_get_jit_invoke_block)>(*this, lib, "get_jit_invoke_block",
                SideEffects::none, "das_get_jit_invoke_block");
            addExtern<DAS_BIND_FUN(das_get_jit_invoke_block_with_cmres)>(*this, lib, "get_jit_invoke_block_with_cmres",
                SideEffects::none, "das_get_jit_invoke_block_with_cmres");
            addExtern<DAS_BIND_FUN(das_get_jit_string_builder)>(*this, lib, "get_jit_string_builder",
                SideEffects::none, "das_get_jit_string_builder");
            addExtern<DAS_BIND_FUN(das_get_jit_string_builder_temp)>(*this, lib, "get_jit_string_builder_temp",
                SideEffects::none, "das_get_jit_string_builder_temp");
            addExtern<DAS_BIND_FUN(das_get_jit_get_global_mnh)>(*this, lib, "get_jit_get_global_mnh",
                SideEffects::none, "das_get_jit_get_global_mnh");
            addExtern<DAS_BIND_FUN(das_get_jit_get_shared_mnh)>(*this, lib, "get_jit_get_shared_mnh",
                SideEffects::none, "das_get_jit_get_shared_mnh");
            addExtern<DAS_BIND_FUN(das_get_jit_alloc_heap)>(*this, lib, "get_jit_alloc_heap",
                SideEffects::none, "das_get_jit_alloc_heap");
            addExtern<DAS_BIND_FUN(das_get_jit_alloc_persistent)>(*this, lib, "get_jit_alloc_persistent",
                SideEffects::none, "das_get_jit_alloc_persistent");
            addExtern<DAS_BIND_FUN(das_get_jit_free_heap)>(*this, lib, "get_jit_free_heap",
                SideEffects::none, "das_get_jit_free_heap");
            addExtern<DAS_BIND_FUN(das_get_jit_free_persistent)>(*this, lib, "get_jit_free_persistent",
                SideEffects::none, "das_get_jit_free_persistent");
            addExtern<DAS_BIND_FUN(das_get_jit_array_lock)>(*this, lib, "get_jit_array_lock",
                SideEffects::none, "das_get_jit_array_lock");
            addExtern<DAS_BIND_FUN(das_get_jit_array_unlock)>(*this, lib, "get_jit_array_unlock",
                SideEffects::none, "das_get_jit_array_unlock");
            addExtern<DAS_BIND_FUN(das_get_jit_table_at)>(*this, lib, "get_jit_table_at",
                SideEffects::none, "das_get_jit_table_at");
            addExtern<DAS_BIND_FUN(das_get_jit_table_erase)>(*this, lib, "get_jit_table_erase",
                SideEffects::none, "das_get_jit_table_erase");
            addExtern<DAS_BIND_FUN(das_get_jit_table_find)>(*this, lib, "get_jit_table_find",
                SideEffects::none, "das_get_jit_table_find");
            addExtern<DAS_BIND_FUN(das_get_jit_str_cmp)>(*this, lib, "get_jit_str_cmp",
                SideEffects::none, "das_get_jit_str_cmp");
            addExtern<DAS_BIND_FUN(das_get_jit_str_cat)>(*this, lib, "get_jit_str_cat",
                SideEffects::none, "das_get_jit_str_cat");
            addExtern<DAS_BIND_FUN(das_get_jit_prologue)>(*this, lib, "get_jit_prologue",
                SideEffects::none, "das_get_jit_prologue");
            addExtern<DAS_BIND_FUN(das_get_jit_epilogue)>(*this, lib, "get_jit_epilogue",
                SideEffects::none, "das_get_jit_epilogue");
            addExtern<DAS_BIND_FUN(das_get_jit_make_block)>(*this, lib, "get_jit_make_block",
                SideEffects::none, "das_get_jit_make_block");
            addExtern<DAS_BIND_FUN(das_get_jit_debug)>(*this, lib, "get_jit_debug",
                SideEffects::none, "das_get_jit_debug");
            addExtern<DAS_BIND_FUN(das_get_jit_iterator_iterate)>(*this, lib, "get_jit_iterator_iterate",
                SideEffects::none, "das_get_jit_iterator_iterate");
            addExtern<DAS_BIND_FUN(das_get_jit_iterator_delete)>(*this, lib, "get_jit_iterator_delete",
                SideEffects::none, "das_get_jit_iterator_delete");
            addExtern<DAS_BIND_FUN(das_get_jit_iterator_first)>(*this, lib, "get_jit_iterator_first",
                SideEffects::none, "das_get_jit_iterator_first");
            addExtern<DAS_BIND_FUN(das_get_jit_iterator_next)>(*this, lib, "get_jit_iterator_next",
                SideEffects::none, "das_get_jit_iterator_next");
            addExtern<DAS_BIND_FUN(das_get_jit_iterator_close)>(*this, lib, "get_jit_iterator_close",
                SideEffects::none, "das_get_jit_iterator_close");
            addExtern<DAS_BIND_FUN(das_get_jit_ast_typedecl)>(*this, lib, "get_jit_ast_typedecl",
                SideEffects::none, "das_get_jit_ast_typedecl");
            addExtern<DAS_BIND_FUN(das_get_builtin_function_address)>(*this, lib,  "get_builtin_function_address",
                SideEffects::none, "das_get_builtin_function_address")
                    ->args({"fn","context","at"});
            addExtern<DAS_BIND_FUN(das_make_interop_node)>(*this, lib,  "make_interop_node",
                SideEffects::none, "das_make_interop_node")
                    ->args({"ctx","call","context","at"});
            addExtern<DAS_BIND_FUN(das_sb_make_interop_node)>(*this, lib,  "make_interop_node",
                SideEffects::none, "das_sb_make_interop_node")
                    ->args({"ctx","builder","context","at"});
            addExtern<DAS_BIND_FUN(das_get_jit_new)>(*this, lib,  "get_jit_new",
                SideEffects::none, "das_get_jit_new")
                    ->args({"type","context","at"});
            addExtern<DAS_BIND_FUN(das_get_jit_delete)>(*this, lib,  "get_jit_delete",
                SideEffects::none, "das_get_jit_delete")
                    ->args({"type","context","at"});
            addExtern<DAS_BIND_FUN(das_get_jit_debug_enter)>(*this, lib,  "get_jit_debug_enter",
                SideEffects::none, "das_get_jit_debug_enter");
            addExtern<DAS_BIND_FUN(das_get_jit_debug_exit)>(*this, lib,  "get_jit_debug_exit",
                SideEffects::none, "das_get_jit_debug_exit");
            addExtern<DAS_BIND_FUN(das_get_jit_debug_line)>(*this, lib,  "get_jit_debug_line",
                SideEffects::none, "das_get_jit_debug_line");
            addExtern<DAS_BIND_FUN(das_get_jit_initialize_fileinfo)>(*this, lib,  "get_jit_initialize_fileinfo",
                SideEffects::none, "das_get_jit_initialize_fileinfo");
            addExtern<DAS_BIND_FUN(das_recreate_fileinfo_name)>(*this, lib,  "recreate_fileinfo_name",
                SideEffects::worstDefault, "das_recreate_fileinfo_name");
            addExtern<DAS_BIND_FUN(loadDynamicLibrary)>(*this, lib,  "load_dynamic_library",
                SideEffects::worstDefault, "loadDynamicLibrary")
                    ->args({"filename"});
            addExtern<DAS_BIND_FUN(getFunctionAddress)>(*this, lib,  "get_function_address",
                SideEffects::worstDefault, "getFunctionAddress")
                    ->args({"library","name"});
            addExtern<DAS_BIND_FUN(closeLibrary)>(*this, lib,  "close_dynamic_library",
                SideEffects::worstDefault, "closeLibrary")
                    ->args({"library"});
            addExtern<DAS_BIND_FUN(create_shared_library)>(*this, lib,  "create_shared_library",
                SideEffects::worstDefault, "create_shared_library")
                    ->args({"objFilePath","libraryName","jitModuleObj"});
            addExtern<DAS_BIND_FUN(jit_initialize_fileinfo)>(*this, lib,  "jit_initialize_fileinfo",
                SideEffects::worstDefault, "jit_initialize_fileinfo");
            addConstant<uint32_t>(*this, "SIZE_OF_PROLOGUE", uint32_t(sizeof(Prologue)));
            addConstant<uint32_t>(*this, "CONTEXT_OFFSET_OF_EVAL_TOP", uint32_t(uint32_t(offsetof(Context, stack) + offsetof(StackAllocator, evalTop))));
            addConstant<uint32_t>(*this, "CONTEXT_OFFSET_OF_GLOBALS", uint32_t(uint32_t(offsetof(Context, globals))));
            addConstant<uint32_t>(*this, "CONTEXT_OFFSET_OF_SHARED", uint32_t(uint32_t(offsetof(Context, shared))));
            // lets make sure its all aot ready
            verifyAotReady();
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            tw << "#include \"daScript/simulate/aot_builtin_jit.h\"\n";
            tw << "#include \"daScript/misc/sysos.h\"\n";
            return ModuleAotType::cpp;
        }
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_Jit,das);
