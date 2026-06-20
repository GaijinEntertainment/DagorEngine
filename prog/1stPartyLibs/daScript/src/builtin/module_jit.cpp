#include "daScript/daScriptModule.h"
#include "daScript/misc/platform.h"

#include "daScript/misc/performance_time.h"
#include "daScript/misc/sysos.h"

#ifdef DAS_ENABLE_DYN_INCLUDES
#include "daScript/ast/dyn_modules.h"
#include "daScript/simulate/fs_file_info.h"
#endif

#include "daScript/ast/ast.h"                     // astTypeInfo
#include "daScript/ast/ast_handle.h"              // addConstant
#include "daScript/ast/ast_interop.h"             // addExtern

#include "daScript/simulate/aot.h"
#include "daScript/simulate/aot_builtin_jit.h"
#include "daScript/simulate/aot_builtin.h"
#include "daScript/simulate/debug_info.h"
#include "daScript/simulate/debug_print.h"
#include "daScript/simulate/hash.h"               // stringLength
#include "daScript/simulate/heap.h"
#include "daScript/simulate/simulate.h"
#include "daScript/simulate/simulate_visit_op.h"

#include "daScript/misc/fpe.h"
#include "daScript/misc/sysos.h"
#include "misc/include_fmt.h"

#include "module_builtin_rtti.h"
#include "module_builtin_ast.h"

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <cstdlib>
#if !DAS_NO_FILEIO
#include <filesystem>
#endif

// MSVC ships popen/pclose under _popen/_pclose; alias once for the whole TU.
#if defined(_WIN32) || defined(_WIN64)
    #define popen _popen
    #define pclose _pclose
#endif

namespace das {
    // forward declarations from module_builtin_fio.cpp
    void * register_dynamic_module(const char *, const char *, int, Context *, LineInfoArg *);
    void register_native_path(const char *, const char *, const char *, Context *, LineInfoArg *);
    bool builtin_fexist ( const char * path );

    typedef vec4f ( * JitFunction ) ( Context * , vec4f *, void * );

    struct SimNode_Jit : SimNode {
        SimNode_Jit ( const LineInfo & at, JitFunction eval )
            : SimNode(at), func(eval) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
        virtual bool rtti_node_isJit() const override { return true; }
        JitFunction func = nullptr;
        // saved original node
        SimNode * saved_code = nullptr;
        bool saved_aot = false;
        void * saved_aot_function = nullptr;
    };

    struct SimNode_JitBlock;

    struct JitBlock : Block {
        vec4f   node[10];
    };

    struct SimNode_JitBlock : SimNode_ClosureBlock {
        SimNode_JitBlock ( const LineInfo & at, JitBlockFunction eval, Block * bptr, uint64_t ad )
            : SimNode_ClosureBlock(at,false,false,ad), func(eval), blockPtr(bptr) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
        JitBlockFunction func = nullptr;
        Block * blockPtr = nullptr;
    };
    static_assert(sizeof(SimNode_JitBlock)<=sizeof(JitBlock().node),"jit block node must fit under node size");


    DAS_SUPPRESS_UB vec4f SimNode_Jit::eval ( Context & context ) {
        auto result = func(&context, context.abiArg, context.abiCMRES);
        context.result = result;
        return result;
    }

    SimNode * SimNode_JitBlock::visit ( SimVisitor & vis ) {
        uint64_t fptr = (uint64_t) func;
        V_BEGIN();
        V_OP(JitBlock);
        V_ARG(fptr);
        V_END();
    }

    DAS_SUPPRESS_UB vec4f SimNode_JitBlock::eval ( Context & context ) {
        auto ba = (BlockArguments *) ( context.stack.bottom() + blockPtr->argumentsOffset );
        return func(&context, ba->arguments, ba->copyOrMoveResult, blockPtr );
    }

    SimNode * SimNode_Jit::visit ( SimVisitor & vis ) {
        uint64_t fptr = (uint64_t) func;
        V_BEGIN();
        V_OP(Jit);
        V_ARG(fptr);
        V_END();
    }

    float4 das_invoke_code ( void * pfun, vec4f anything, void * cmres, Context * context ) {
        vec4f * arguments = cast<vec4f *>::to(anything);
        vec4f (*fun)(Context *, vec4f *, void *) = (vec4f(*)(Context *, vec4f *, void *)) pfun;
        vec4f res = fun ( context, arguments, cmres );
        return res;
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

    bool das_instrument_jit ( void * pfun, const Func func, const LineInfo & lineInfo, Context & context ) {

        auto simfn = func.PTR;
        if ( !simfn ) return false;
        if ( simfn->code && simfn->code->rtti_node_isJit() ) {
            auto jitNode = static_cast<SimNode_Jit *>(simfn->code);
            jitNode->func = (JitFunction) pfun;
            jitNode->debugInfo = lineInfo;
        } else {
            auto node = context.code->makeNode<SimNode_Jit>(lineInfo, (JitFunction)pfun);
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
    DAS_API void jit_exception ( const char * text, Context * context, LineInfoArg * at ) {
        context->throw_error_at(at, "%s", text ? text : "");
    }

    DAS_API vec4f jit_call_or_fastcall ( SimFunction * fn, vec4f * args, Context * context ) {
        return context->callOrFastcall(fn, args, nullptr);
    }

    DAS_API vec4f jit_invoke_block ( const Block & blk, vec4f * args, Context * context ) {
        return context->invoke(blk, args, nullptr, nullptr);
    }

    DAS_API vec4f jit_invoke_block_with_cmres ( const Block & blk, vec4f * args, void * cmres, Context * context ) {
        return context->invoke(blk, args, cmres, nullptr);
    }

    DAS_API vec4f jit_call_with_cmres ( SimFunction * fn, vec4f * args, void * cmres, Context * context ) {
        return context->callWithCopyOnReturn(fn, args, cmres, nullptr);
    }

    DAS_API char * jit_string_builder ( Context & context, int32_t nArgs, TypeInfo ** types, LineInfoArg * at, vec4f * args ) {
        StringBuilderWriter writer;
        DebugDataWalker<StringBuilderWriter> walker(writer, PrintFlags::string_builder);
        for ( int i = 0; i < nArgs; ++i )
            walker.walk(args[i], types[i]);
        auto length = writer.tellp();
        if ( length ) {
            return context.allocateString(writer.c_str(), uint32_t(length), at);
        } else {
            return nullptr;
        }
    }

    DAS_API char * jit_string_builder_temp ( Context & context, int32_t nArgs, TypeInfo ** types, LineInfoArg * at, vec4f * args ) {
        StringBuilderWriter writer;
        DebugDataWalker<StringBuilderWriter> walker(writer, PrintFlags::string_builder);
        for ( int i = 0; i < nArgs; ++i )
            walker.walk(args[i], types[i]);
        auto length = writer.tellp();
        if ( length ) {
            auto str = context.allocateTempString(writer.c_str(), uint32_t(length), at);
            context.freeTempString(str, at);
            return str;
        } else {
            return nullptr;
        }
    }

    DAS_API void jit_simnode_interop(void *ptr, int argCount, TypeInfo **types) {
        auto res = new(ptr) SimNode_AotInteropBase();
        res->argumentValues = nullptr;
        res->nArguments = argCount;
        res->types = types;
    }

    DAS_API void jit_free_simnode_interop(SimNode_AotInteropBase *ptr) {
        ptr->~SimNode_AotInteropBase();
    }

    DAS_API void *das_get_jit_simnode_interop() {
        return (void *) &jit_simnode_interop;
    }

    DAS_API void *das_get_jit_free_simnode_interop() {
        return (void *) &jit_free_simnode_interop;
    }

    // We need it to bypass protected/private.
    class JitContext : public Context {
    public:
        ~JitContext() = default;
        JitContext(size_t totalVariables, size_t totalFunctions, size_t globalStringHeapSize,
                  size_t globSize, size_t shrSize, bool pinvoke, uint32_t stackSize = 16*1024)
            : Context(stackSize) {
            auto &context = *this;
            CodeOfPolicies policies;
            policies.debugger = false;
            context.setup(totalVariables, globalStringHeapSize, policies, {});
            context.globalsSize = globSize;
            context.sharedSize = shrSize;
            context.sharedOwner = true;
            for (int i = 0; i < totalVariables; i++) {
                globalVariables[i] = GlobalVariable{};
            }
            context.allocateGlobalsAndShared();
            // UBSAN forbid memset(null, 0, 0)
            if (context.globalsSize) memset(context.globals, 0, context.globalsSize);
            if (context.sharedSize)  memset(context.shared,  0, context.sharedSize);
            if ( pinvoke ) {
                context.contextMutex = new recursive_mutex;
            }
        }

        void allocFunctions ( uint64_t count ) {
            functions = (SimFunction *) code->allocate(count * sizeof(SimFunction));
            memset(functions, 0, count * sizeof(SimFunction));
            totalFunctions = (int) count;
            // Allocate stub debugInfo for all function slots so that
            // runShutdownScript can safely iterate them.
            auto stubInfo = (FuncInfo *) code->allocate(count * sizeof(FuncInfo));
            memset(stubInfo, 0, count * sizeof(FuncInfo));
            for ( uint64_t i = 0; i < count; i++ ) {
                stubInfo[i].name = (char *) "unimplemented";
                functions[i].name = (char *) "unimplemented";
                functions[i].debugInfo = &stubInfo[i];
            }
            tabMnLookup = make_shared<das_hash_map<uint64_t,SimFunction *>>();
            tabGMnLookup = make_shared<das_hash_map<uint64_t,uint32_t>>();
        }

        void *registerJitFunction ( uint64_t index, const char * name, const char * mangledName,
                                   uint64_t mnh, uint32_t stackSize, void * fnPtr,
                                   bool cmres, bool fastcall, bool pinvoke, uint32_t nArguments ) {
            DAS_ASSERT(index < (uint64_t) totalFunctions);
            auto & fn = functions[index];
            fn.name = code->allocateName(name);
            fn.mangledName = code->allocateName(mangledName);
            fn.mangledNameHash = mnh;
            fn.stackSize = stackSize;
            fn.flags = 0;
            fn.cmres = cmres;
            fn.fastcall = fastcall;
            fn.pinvoke = pinvoke;
            fn.jit = true;
            auto finfo = (FuncInfo *) code->allocate(sizeof(FuncInfo));
            memset(finfo, 0, sizeof(FuncInfo));
            finfo->name = fn.name;
            finfo->stackSize = stackSize;
            finfo->count = nArguments;
            fn.debugInfo = finfo;
            auto node = code->makeNode<SimNode_Jit>(LineInfo{}, (JitFunction) fnPtr);
            fn.code = node;
            (*tabMnLookup)[mnh] = &fn;
            return &fn;
        }

        void registerJitGlobalVariable(uint64_t mnh, size_t offset) {
            (*tabGMnLookup)[mnh] = offset;
        }

        void initFunctionAddr ( uint64_t index, void * globPtr ) {
            DAS_ASSERT(index < (uint64_t) totalFunctions);
            *((SimFunction **) globPtr) = &functions[index];
        }
    };

    // Note: this function called in runtime, before main.
    // When we built executable from das.
    DAS_API Context * jit_create_standalone_ctx ( uint64_t totalVariables,
                                                  uint64_t totalFunctions,
                                                  uint64_t globalStringHeapSize,
                                                  uint64_t globalsSize,
                                                  uint64_t sharedSize,
                                                  bool pinvoke,
                                                  uint64_t stackSize) {
        Context *context = new JitContext(totalVariables, totalFunctions, globalStringHeapSize,
                                         globalsSize, sharedSize, pinvoke,
                                         stackSize ? (uint32_t)stackSize : 16*1024);
        static_cast<JitContext *>(context)->allocFunctions(totalFunctions);
        return context;
    }

    DAS_API void *jit_register_standalone_function ( Context * ctx, uint64_t index,
                                             const char * name, const char * mangledName,
                                             uint64_t mnh, uint32_t stackSize,
                                             void * fnPtr,
                                             bool cmres, bool fastcall, bool pinvoke,
                                             uint32_t nArguments ) {
        return static_cast<JitContext *>(ctx)->registerJitFunction(index, name, mangledName, mnh, stackSize,
                                                            fnPtr, cmres, fastcall, pinvoke, nArguments);
    }

    DAS_API void jit_register_standalone_variable ( Context * ctx, uint64_t mangledNameHash, uint64_t offset ) {
        static_cast<JitContext *>(ctx)->registerJitGlobalVariable(mangledNameHash, offset);
    }

    DAS_API void jit_set_init_script ( Context * ctx, Context::JitInitScriptFn fn ) {
        ctx->jitInitScript = fn;
    }

    DAS_API void jit_init_function_addr ( Context * ctx, uint64_t index, void * globPtr ) {
        static_cast<JitContext *>(ctx)->initFunctionAddr(index, globPtr);
    }

    DAS_API void jit_init_extern_function ( const char * moduleName,
                                            const char * funcMangledName,
                                            void ** dllGlobal ) {
        bool found = false;
        Module::foreach([&](Module * module) -> bool {
            if ( module->name != moduleName ) return true;
            auto fn = module->findFunction(funcMangledName);
            if ( fn && fn->builtIn ) {
                *dllGlobal = static_cast<BuiltInFunction *>(fn)->getBuiltinAddress();
                found = *dllGlobal != nullptr;
                return !found;
            }
            return true;
        });
        if ( !found && strcmp(moduleName, "dasbind") == 0 && strncmp(funcMangledName, "@dasbind::__dasbind__", 21) == 0 ) {
            Module::foreach([&](Module * module) -> bool {
                if ( module->name != "dasbind" ) return true;
                auto resolverFn = module->findUniqueFunction("__dasbind_resolve");
                if ( resolverFn && resolverFn->builtIn ) {
                    auto resolver = (void * (*)(const char *))
                        static_cast<BuiltInFunction *>(resolverFn)->getBuiltinAddress();
                    if ( resolver ) {
                        *dllGlobal = resolver(funcMangledName);
                        found = *dllGlobal != nullptr;
                    }
                }
                return false;
            });
        }
        if (!found) {
            DAS_FATAL_ERROR("Failed to find %s in module %s.\n", funcMangledName, moduleName);
        }
    }

    DAS_API Annotation *jit_get_annotation ( const char * moduleName,
                                                 const char * annName ) {
        Annotation *result = nullptr;
        Module::foreach([&](Module * module) -> bool {
            if ( module->name != moduleName ) return true;
            result = module->findAnnotation(annName);
            return false;
        });
        if (!result) {
            DAS_FATAL_ERROR("Failed to find annotation %s in module %s.\n", annName, moduleName);
        }
        return result;
    }

    DAS_API void jit_trap() {
        DAS_FATAL_ERROR("FATAL: Unresolved dynamic function call in compiled code. This indicates a missing JIT symbol. Disable `strict` mode or remove this call.\n");
    }

    DAS_API void jit_set_command_line_arguments( int argc, char * argv[] ) {
        setCommandLineArguments(argc, argv);
    }

    DAS_API void *das_get_jit_init_extern_function() {
        return (void *) &jit_init_extern_function;
    }

    DAS_API void * jit_get_global_mnh ( uint64_t mnh, Context & context ) {
        return context.globals + context.globalOffsetByMangledName(mnh);
    }

    DAS_API void * jit_get_shared_mnh ( uint64_t mnh, Context & context ) {
        return context.shared + context.globalOffsetByMangledName(mnh);
    }

    // Return the raw globals / shared base pointers from the Context. Existed
    // before as JIT IR doing `GEP ctx+CONTEXT_OFFSET_OF_GLOBALS + load`, but
    // CONTEXT_OFFSET_OF_GLOBALS is a `static const uint32_t` baked at C++
    // compile time of THIS file using the host's `offsetof(Context, globals)`.
    // When cross-compiling JIT'd code to wasm32, the wasm32 Context struct
    // has 4-byte pointers and a different offsetof — emitting the host's
    // offset reads garbage. The runtime archive (libDaScript_runtime.a) is
    // built for the target with the right layout, so calling this helper
    // gives the right pointer.
    DAS_API void * jit_get_globals_base ( Context * context ) {
        return context->globals;
    }

    DAS_API void * jit_get_shared_base ( Context * context ) {
        return context->shared;
    }

    DAS_API void * jit_alloc_heap ( uint32_t bytes, Context * context ) {
        return context->allocate(bytes);
    }

    DAS_API void * jit_alloc_persistent ( uint32_t bytes, Context * context ) {
        if ( !bytes ) context->throw_out_of_memory(false, bytes);
        return das_aligned_alloc16(bytes);
    }

    DAS_API void jit_free_heap ( void * bytes, uint32_t size, Context * context ) {
        context->free((char *)bytes,size);
    }

    DAS_API void jit_free_persistent ( void * bytes, Context * ) {
        das_aligned_free16(bytes);
    }

    DAS_API void jit_array_lock ( const Array & arr, Context * context, LineInfoArg * at ) {
        builtin_array_lock_mutable(arr, context, at);
    }

    DAS_API void jit_array_unlock ( const Array & arr, Context * context, LineInfoArg * at ) {
        builtin_array_unlock_mutable(arr, context, at);
    }

    DAS_API int32_t jit_str_cmp ( char * a, char * b ) {
        return strcmp(a ? a : "",b ? b : "");
    }

    DAS_API char * jit_str_cat ( const char * sA, const char * sB, Context * context, LineInfoArg * at ) {
        sA = sA ? sA : "";
        sB = sB ? sB : "";
        auto la = stringLength(*context, sA);
        auto lb = stringLength(*context, sB);
        uint32_t commonLength = la + lb;
        if ( !commonLength ) {
            return nullptr;
        } else {
            char * sAB = (char * ) context->allocateString(nullptr, commonLength, at);
            memcpy ( sAB, sA, la );
            memcpy ( sAB+la, sB, lb+1 );
            context->stringHeap->recognize(sAB);
            return sAB;
        }
    }

    struct JitStackState {
        char * EP;
        char * SP;
    };

    DAS_API void jit_prologue ( const char *funcName, void * funcLineInfo,
            int32_t stackSize, JitStackState * stackState,
            Context * context, LineInfoArg * at ) {
        if (!context->stack.push(stackSize, stackState->EP, stackState->SP)) {
            context->throw_error_at(at, "stack overflow");
        }
#if DAS_ENABLE_STACK_WALK
        Prologue * pp = (Prologue *)context->stack.sp();
        pp->info = nullptr;
        pp->fileName = funcName;
        pp->functionLine = (LineInfo *) funcLineInfo;
        pp->stackSize = stackSize;
        pp->is_jit = true;
#endif
    }

    DAS_API void jit_epilogue ( JitStackState * stackState, Context * context ) {
        context->stack.pop(stackState->EP, stackState->SP);
    }

    DAS_API void jit_make_block ( Block * blk, int32_t argStackTop, uint64_t ad, void * bodyNode, void * jitImpl, void * funcInfo, void * lineInfo, Context * context ) {
        DAS_ASSERTF(lineInfo != nullptr, "Line info should not be null");

        JitBlock * block = (JitBlock *) blk;
        block->stackOffset = context->stack.spi();
        block->argumentsOffset = argStackTop ? (context->stack.spi() + argStackTop) : 0;
        block->body = (SimNode *)(void*) block->node;
        block->aotFunction = nullptr;
        block->jitFunction = jitImpl;
        block->functionArguments = context->abiArguments();
        block->info = (FuncInfo *) funcInfo;
        new (block->node) SimNode_JitBlock(*static_cast<LineInfo*>(lineInfo), (JitBlockFunction) bodyNode, blk, ad);
    }

    DAS_API void jit_debug ( vec4f res, TypeInfo * typeInfo, char * message, Context * context, LineInfoArg * at ) {
        FPE_DISABLE;
        TextWriter ssw;
        if ( message ) ssw << message << " ";
        ssw << debug_type(typeInfo) << " = " << debug_value(res, typeInfo, PrintFlags::debugger) << "\n";
        context->to_out(at, ssw.str().c_str());
    }

    DAS_API bool jit_iterator_iterate ( das::Sequence &it, void *data, das::Context *context ) {
        return builtin_iterator_iterate(it, data, context);
    }

    DAS_API void jit_iterator_delete ( das::Sequence &it, das::Context *context ) {
        return builtin_iterator_delete(it, context);
    }

    DAS_API void jit_iterator_close ( das::Sequence &it, void *data, das::Context *context ) {
        return builtin_iterator_close(it, data, context);
    }

    DAS_API bool jit_iterator_first ( das::Sequence &it, void *data, das::Context *context, das::LineInfoArg *at ) {
        return builtin_iterator_first(it, data, context, at);
    }

    DAS_API bool jit_iterator_next ( das::Sequence &it, void *data, das::Context *context, das::LineInfoArg *at ) {
        return builtin_iterator_next(it, data, context, at);
    }

    DAS_API void jit_debug_enter ( char * message, Context * context, LineInfoArg * at ) {
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

    DAS_API void jit_debug_exit ( char * message, Context * context, LineInfoArg * at ) {
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

    DAS_API void jit_debug_line ( char * message, Context * context, LineInfoArg * at ) {
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

    DAS_API void jit_initialize_fileinfo ( void * dummy, const char *filename ) {
        new (dummy) FileInfo();
        auto fileInfoPtr = reinterpret_cast<FileInfo*>(dummy);
        fileInfoPtr->name = filename;
    }

    DAS_API void jit_free_fileinfo ( void * dummy ) {
        reinterpret_cast<FileInfo*>(dummy)->~FileInfo();
    }

    struct JitAnnotationArgPod {
        const char * name;
        const char * sValue;
        int32_t      type;
        int32_t      iValue;  // covers bool/int/float (union bit-cast)
    };

    DAS_API void jit_initialize_varinfo_annotations ( void * varinfo_ptr, int32_t nArgs, JitAnnotationArgPod * args ) {
        auto vi = (VarInfo *) varinfo_ptr;
        auto aa = new AnnotationArguments();
        aa->reserve(nArgs);
        for ( int32_t i = 0; i < nArgs; i++ ) {
            AnnotationArgument arg;
            arg.type   = (Type) args[i].type;
            arg.name   = args[i].name   ? args[i].name   : "";
            arg.sValue = args[i].sValue ? args[i].sValue : "";
            arg.iValue = args[i].iValue;
            aa->push_back(std::move(arg));
        }
        vi->annotation_arguments = aa;
    }

    DAS_API void jit_free_varinfo_annotations ( void * varinfo_ptr ) {
        auto vi = (VarInfo *) varinfo_ptr;
        if ( vi->annotation_arguments ) {
            delete (AnnotationArguments *) vi->annotation_arguments;
            vi->annotation_arguments = nullptr;
        }
    }

    DAS_API void * jit_ast_typedecl ( uint64_t hash, Context * context, LineInfoArg * at ) {
        if ( !context->thisProgram ) context->throw_error_at(at, "can't get ast_typeinfo, no program. is 'options rtti' missing?");
        auto ti = context->thisProgram->astTypeInfo.find(hash);
        if ( ti==context->thisProgram->astTypeInfo.end() ) {
            context->throw_error_at(at, "can't find ast_typeinfo for hash %" PRIx64, hash);
        }
        auto info = ti->second;
        return (void*) new TypeDecl(*info);
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
    void *das_get_jit_get_globals_base() { return (void *)&jit_get_globals_base; }
    void *das_get_jit_get_shared_base() { return (void *)&jit_get_shared_base; }
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
    void *das_get_jit_free_fileinfo () { return (void*)&jit_free_fileinfo; }
    void *jit_get_initialize_varinfo_annotations () { return (void*)&jit_initialize_varinfo_annotations; }
    void *jit_get_free_varinfo_annotations () { return (void*)&jit_free_varinfo_annotations; }
    void *das_get_jit_ast_typedecl () { return (void*)&jit_ast_typedecl; }

    template <typename KeyType>
    int32_t jit_table_at ( Table * tab, KeyType key, int32_t valueTypeSize, Context * context, LineInfoArg * at ) {
        if ( tab->isLocked() ) context->throw_error_at(at, "can't insert to a locked table");
        TableHash<KeyType> thh(context,valueTypeSize);
        auto hfn = hash_function(*context, key);
        int64_t idx = thh.reserve(*tab, key, hfn, at);
        // TODO Phase 7: widen JIT helper return to int64_t. Until then, guard the narrowing
        // so JIT-emitted code never gets a wrapped-negative slot index for huge tables.
        if ( idx > int64_t(INT32_MAX) ) context->throw_error_at(at, "JIT table slot index %lld exceeds INT32_MAX; JIT does not yet support tables past INT_MAX slots", (long long)idx);
        return (int32_t) idx;
    }

    void * das_get_jit_table_at ( int32_t baseType, Context * context, LineInfoArg * at ) {
        JIT_TABLE_FUNCTION(&jit_table_at);
    }

    template <typename KeyType>
    bool jit_table_erase ( Table * tab, KeyType key, int32_t valueTypeSize, Context * context, LineInfoArg * at ) {
        if ( tab->isLocked() ) context->throw_error_at(at, "can't erase from locked table");
        TableHash<KeyType> thh(context,valueTypeSize);
        auto hfn = hash_function(*context, key);
        return thh.erase(*tab, key, hfn) != -1;
    }

    void * das_get_jit_table_erase ( int32_t baseType, Context * context, LineInfoArg * at ) {
        JIT_TABLE_FUNCTION(&jit_table_erase);
    }

    template <typename KeyType>
    int32_t jit_table_find ( Table * tab, KeyType key, int32_t valueTypeSize, Context * context ) {
        TableHash<KeyType> thh(context,valueTypeSize);
        auto hfn = hash_function(*context, key);
        int64_t idx = thh.find(*tab, key, hfn);
        // TODO Phase 7: widen JIT helper return to int64_t. Until then, guard the narrowing
        // so JIT-emitted code never gets a wrapped-negative slot index for huge tables.
        if ( idx > int64_t(INT32_MAX) ) context->throw_error("JIT table slot index exceeds INT32_MAX; JIT does not yet support tables past INT_MAX slots");
        return (int32_t) idx;
    }

    void * das_get_jit_table_find ( int32_t baseType, Context * context, LineInfoArg * at ) {
        JIT_TABLE_FUNCTION(&jit_table_find);
    }

    // String-key at that takes a precomputed hash, so the JIT emits the hash inline (foldable to an
    // immediate for a constant key) instead of recomputing it in C++. String-only — no per-baseType
    // matrix; the grow fallback for the inline large-string at (string find is fully inline).
    int32_t jit_string_table_at_with_hash ( Table * tab, char * key, uint64_t hfn, int32_t valueTypeSize, Context * context, LineInfoArg * at ) {
        if ( tab->isLocked() ) context->throw_error_at(at, "can't insert to a locked table");
        TableHash<char *> thh(context,valueTypeSize);
        int64_t idx = thh.reserve(*tab, key, hfn, at);
        if ( idx > int64_t(INT32_MAX) ) context->throw_error_at(at, "JIT table slot index %lld exceeds INT32_MAX; JIT does not yet support tables past INT_MAX slots", (long long)idx);
        return (int32_t) idx;
    }

    void * das_get_jit_string_table_at_with_hash ( ) {
        return (void*)&jit_string_table_at_with_hash;
    }

    // Insert after the JIT's inline packed find already proved the key absent from a packed
    // table — skips the C++ packed dedup. See TableHash::reserveAfterPackedMiss (which checks
    // the lock itself). String form takes the JIT-computed hash; the non-string template
    // computes the (cheap) integer hash here rather than threading it from the JIT.
    int32_t jit_string_table_at_after_packed_miss ( Table * tab, char * key, uint64_t hfn, int32_t valueTypeSize, Context * context, LineInfoArg * at ) {
        TableHash<char *> thh(context,valueTypeSize);
        int64_t idx = thh.reserveAfterPackedMiss(*tab, key, hfn, at);
        if ( idx > int64_t(INT32_MAX) ) context->throw_error_at(at, "JIT table slot index %lld exceeds INT32_MAX; JIT does not yet support tables past INT_MAX slots", (long long)idx);
        return (int32_t) idx;
    }

    void * das_get_jit_string_table_at_after_packed_miss ( ) {
        return (void*)&jit_string_table_at_after_packed_miss;
    }

    uint64_t das_get_global_variable_offset( const Context * ctx, int id ) {
        return ctx->getGlobalVariable(id).offset;
    }

    uint64_t das_get_global_variable_mnh( const Context * ctx, int id ) {
        return ctx->getGlobalVariable(id).mangledNameHash;
    }

    uint64_t das_get_context_globals_size( const Context * ctx ) {
        return ctx->getGlobalSize();
    }

    uint64_t das_get_context_shared_size( const Context * ctx ) {
        return ctx->getSharedSize();
    }

    extern "C" {
        DAS_API void * get_jit_table_find ( int32_t baseType, Context * context, LineInfoArg * at ) {
            return das_get_jit_table_find(baseType, context, at);
        }
        DAS_API void * get_jit_table_at ( int32_t baseType, Context * context, LineInfoArg * at ) {
            return das_get_jit_table_at(baseType, context, at);
        }
        DAS_API void * get_jit_table_erase ( int32_t baseType, Context * context, LineInfoArg * at ) {
            return das_get_jit_table_erase(baseType, context, at);
        }
        // String-key at routes through this precomputed-hash global (see llvm_jit.das
        // build_string_table_at_*); the standalone-exe glob baking needs it as a linkable symbol.
        // (String find is fully inline — no C++ helper.)
        DAS_API void * get_jit_string_table_at_with_hash ( ) {
            return das_get_jit_string_table_at_with_hash();
        }
        // Insert-after-packed-miss globals: the JIT's inline packed find calls these on a miss.
        // The standalone-exe baker (llvm_exe.das add_table_at_call) stores the in-exe address
        // here; without it the glob keeps its compile-process address and faults under ASLR.
        DAS_API void * get_jit_string_table_at_after_packed_miss ( ) {
            return das_get_jit_string_table_at_after_packed_miss();
        }
        DAS_API void * das_get_jit_new ( TypeAnnotation *annotation ) {
            return annotation->jitGetNew();
        }
        DAS_API void * das_get_jit_delete ( TypeAnnotation *annotation ) {
            return annotation->jitGetDelete();
        }
        DAS_API void * das_get_jit_clone ( TypeAnnotation *annotation ) {
            return annotation->jitGetClone();
        }
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

    struct LinkerPaths {
        string linker;
        string runtimeLibrary;  // always needed
        string compilerLibrary; // non-empty when linkWholeLib — exe also needs compiler lib
    };

    // Resolve a linker / tool path. Lookup order:
    //   1. `custom` — explicit override (non-null, non-empty)
    //   2. `<das_root>/bin/<bundledName>` if present on disk
    //   3. `fallback` — bare name picked up via PATH
    // Callers pass the platform-specific bundled name (e.g. `wasm-ld.exe` on
    // Windows, `wasm-ld` on POSIX, `emcc.bat` on Windows, etc.).
    static string find_linker(const char * custom, const string & bundledName, const string & fallback) {
        if ( custom != nullptr && custom[0] != '\0' ) return custom;
        string bundled = getDasRoot() + "/bin/" + bundledName;
        if ( check_file_present(bundled.c_str()) ) return bundled;
        return fallback;
    }

    // Host-toolchain JIT triple — when non-empty, daslib uses this instead of
    // LLVMGetDefaultTargetTriple() to drive JIT codegen.
    //
    // The prebuilt LLVM.dll shipped via dasLLVM's FetchContent is built with
    // MSVC clang-cl, so LLVMGetDefaultTargetTriple() always returns
    // x86_64-pc-windows-msvc regardless of what host compiler built
    // daslang.exe. On mingw daslang.exe, MSVC-ABI .o files reference
    // _fltused (msvcrt marker symbol) and other MSVC-CRT-isms that mingw's
    // ld.lld + libc CRT cannot satisfy. Forcing a mingw triple makes LLVM
    // emit a mingw-flavored .o with no MSVC marker symbols, links cleanly
    // against libDaScriptDyn_runtime.dll.a, and the resulting DLL calls
    // into the mingw daslang.exe via the same C ABI without CRT-mismatch
    // hazards.
    //
    // The arch suffix has to match the host — clang32 (i686), clang64
    // (x86_64), clangarm64 (aarch64), clangarm32 (armv7) all share the
    // mingw vendor/os/env suffix but differ on the arch component.
    // Detection uses the standard GCC/Clang arch predefined macros so
    // this works for both compilers on every supported mingw arch.
    const char * host_jit_triple ( ) {
    #if defined(_WIN32) && !defined(_MSC_VER)
        #if defined(__aarch64__) || defined(_M_ARM64)
            return "aarch64-w64-windows-gnu";
        #elif defined(__arm__) || defined(_M_ARM)
            return "armv7-w64-windows-gnu";
        #elif defined(__x86_64__) || defined(_M_X64)
            return "x86_64-w64-windows-gnu";
        #elif defined(__i386__) || defined(_M_IX86)
            return "i686-w64-windows-gnu";
        #else
            // Unknown mingw arch — fall back to LLVM's default and let
            // the user explicitly override via the customLinker path.
            return "";
        #endif
    #else
        return "";
    #endif
    }

    static LinkerPaths get_real_lib_linker_paths(const char * dasLib, const char * customLinker, bool isShared, bool linkWholeLib) {
        LinkerPaths result;
        result.linker = customLinker != nullptr ? customLinker : "";
        result.runtimeLibrary = dasLib != nullptr ? dasLib : "";

        if (result.linker.empty() || result.runtimeLibrary.empty()) {
            #if defined(_WIN32) || defined(_WIN64)
                // Two distinct Windows toolchains share the _WIN32 macro:
                //   MSVC (incl. clang-cl) → MSVC-style import libs (libFoo.lib),
                //     linked via clang-cl.exe with -DLL / -link / -OUT: syntax.
                //   mingw (clang-mingw / gcc-mingw) → mingw import libs
                //     (libFoo.dll.a), linked via clang.exe with -shared / -o
                //     syntax (Unix-like — clang.exe driver is identical to
                //     posix clang). The split keeps daslang.exe built with one
                //     toolchain talking to a JIT'd DLL built with the same one.
                #if defined(_MSC_VER)
                    if (result.linker.empty()) {
                        result.linker = find_linker(nullptr, "clang-cl.exe", "clang-cl");
                    }
                    if (result.runtimeLibrary.empty()) {
                        const auto path = get_prefix(getExecutableFileName());
                        const auto winCfg = path.substr(path.find_last_of("\\/") + 1);
                        const auto windowsConfig = (winCfg == "bin" ? "" : (winCfg + "/"));
                        result.runtimeLibrary = getDasRoot() + "/lib/" + windowsConfig + "libDaScriptDyn_runtime.lib";
                        if (linkWholeLib) {
                            result.compilerLibrary = getDasRoot() + "/lib/" + windowsConfig + "libDaScriptDyn.lib";
                        }
                    }
                #else
                    // mingw — note bundled clang-cl.exe (from dasLLVM prebuilt
                    // LLVM) is MSVC-flavored and cannot read .dll.a, so don't
                    // probe for it; default to "clang" on PATH (clang64/bin
                    // when daslang.exe was built with clang-mingw64).
                    if (result.linker.empty()) {
                        result.linker = "clang";
                    }
                    if (result.runtimeLibrary.empty()) {
                        const auto path = get_prefix(getExecutableFileName());
                        const auto winCfg = path.substr(path.find_last_of("\\/") + 1);
                        const auto windowsConfig = (winCfg == "bin" ? "" : (winCfg + "/"));
                        result.runtimeLibrary = getDasRoot() + "/lib/" + windowsConfig + "libDaScriptDyn_runtime.dll.a";
                        if (linkWholeLib) {
                            result.compilerLibrary = getDasRoot() + "/lib/" + windowsConfig + "libDaScriptDyn.dll.a";
                        }
                    }
                #endif
            #else
                if (result.linker.empty()) {
                    result.linker = "c++";
                }
                if (result.runtimeLibrary.empty()) {
                #if defined(__APPLE__)
                    result.runtimeLibrary = getDasRoot() + "/lib/liblibDaScriptDyn_runtime.dylib";
                    if (linkWholeLib) {
                        result.compilerLibrary = getDasRoot() + "/lib/liblibDaScriptDyn.dylib";
                    }
                #else
                    result.runtimeLibrary = getDasRoot() + "/lib/liblibDaScriptDyn_runtime.so";
                    if (linkWholeLib) {
                        result.compilerLibrary = getDasRoot() + "/lib/liblibDaScriptDyn.so";
                    }
                #endif
                }
            #endif
        }
        return result;
    }

#if (defined(_WIN32) || defined(__linux__) || defined(__APPLE__)) && !defined(_GAMING_XBOX) && !defined(_DURANGO)
    // Run a fully-formed linker command via popen, capture combined stdout/
    // stderr into a 16KB buffer, then log success or a failure diagnostic
    // (with the captured output) through the daslang Context.
    //   cmd          — complete shell command, already includes 2>&1.
    //   artifactPath — output file path (logged on success/failure).
    //   artifactKind — short label for messages ("Library", "Wasm", ...).
    static bool run_link_cmd(const char * cmd, const char * artifactPath,
                             const char * artifactKind, Context * context) {
        FILE * fp = popen(cmd, "r");
        if ( fp == NULL ) {
            LOG(LogLevel::error) << "Failed to run command '" << cmd << "'\n";
            return false;
        }
        static constexpr int MAX_OUTPUT_SIZE = 16 * 1024;
        char buffer[1024], output[MAX_OUTPUT_SIZE];
        output[0] = '\0';
        size_t output_length = 0;
        while ( fgets(buffer, sizeof(buffer), fp) != NULL ) {
            size_t buffer_length = strlen(buffer);
            if ( output_length + buffer_length < MAX_OUTPUT_SIZE ) {
                strcat(output, buffer);
                output_length += buffer_length;
            } else {
                strncat(output, buffer, MAX_OUTPUT_SIZE - output_length - 1);
                break;
            }
        }
        auto li = LineInfo();
        if ( int status = pclose(fp); status != 0 ) {
            string msg = string("Failed to link ") + artifactKind + " " + artifactPath + ", command '" + cmd + "'\n";
            context->to_out(&li, LogLevel::error, msg.c_str());
            string err = string("Output:\n") + output;
            context->to_out(&li, LogLevel::error, err.c_str());
            return false;
        }
        string msg = string(artifactKind) + " " + artifactPath + " linked - ok\n";
        context->to_out(&li, LogLevel::info, msg.c_str());
        return true;
    }

    bool create_shared_library ( const char * objFilePath, const char * libraryName, [[maybe_unused]] const char * dasLib, const char * customLinker, const char * extraLinkerArgs, bool isShared, bool linkWholeLib, Context *context ) {
        // cmd is built via fmt::format (heap-allocated std::string) rather
        // than a fixed stack buffer — long paths (deep build roots, spaces,
        // long extraLinkerArgs) can easily exceed a few hundred bytes, and
        // a fixed buffer would silently corrupt the stack on overflow.
        const char * extra = (extraLinkerArgs && extraLinkerArgs[0]) ? extraLinkerArgs : "";
        const auto paths = get_real_lib_linker_paths(dasLib, customLinker, isShared, linkWholeLib);
        const auto & linker = paths.linker;
        const auto & runtimeLibrary = paths.runtimeLibrary;
        const auto & compilerLibrary = paths.compilerLibrary;

        #if defined(_WIN32) || defined(_WIN64) || defined(__APPLE__)
        if (!check_file_present(runtimeLibrary.c_str())) {
            LOG(LogLevel::error) << "File '" << runtimeLibrary << "' , containing daslang runtime library, does not exist\n";
            return false;
        }
        #endif
        if (!check_file_present(objFilePath)) {
            LOG(LogLevel::error) << "File '" << objFilePath << "' , containing compiled definitions, does not exist\n";
            return false;
        }

        std::string cmd;
        #if defined(_WIN32) || defined(_WIN64)
            #if defined(_MSC_VER)
                // MSVC clang-cl: -DLL marks shared, -link separates link-only
                // flags, -OUT: names the output, msvcrt.lib pulls the import
                // CRT. Outer doubled quotes wrap the whole command for cmd.exe
                // because the linker path itself contains spaces on Program
                // Files installs.
                const auto linkerParam = isShared ? "-DLL" : "";
                cmd = compilerLibrary.empty()
                    ? fmt::format(FMT_STRING("\"\"{}\" \"{}\" \"{}\" msvcrt.lib {} -link {} -OUT:\"{}\" 2>&1\""), linker.c_str(), objFilePath, runtimeLibrary.c_str(), extra, linkerParam, libraryName)
                    : fmt::format(FMT_STRING("\"\"{}\" \"{}\" \"{}\" \"{}\" msvcrt.lib {} -link {} -OUT:\"{}\" 2>&1\""), linker.c_str(), objFilePath, runtimeLibrary.c_str(), compilerLibrary.c_str(), extra, linkerParam, libraryName);
            #else
                // mingw clang/gcc: Unix-flavored driver, -shared/-o syntax.
                // No rpath on Windows (DLLs resolve via PATH / LoadLibrary
                // search order), so the linux branch's rpath escape is
                // dropped. The linux branch's -Wl,--no-as-needed is also
                // dropped: on ELF, --no-as-needed forces a DT_NEEDED entry
                // for libraries that the linker would otherwise mark as
                // not-required (lazy bind). PE/COFF has no such concept —
                // listed libraries are always recorded in the import table,
                // so the flag has no effect. Worse, lld on newer mingw
                // sysroots rejects --no-as-needed as an unknown argument
                // (older clangs / GNU ld silently ignore it, which is why
                // this only surfaces on fresh CI installs).
                // Outer doubled quotes `\"...2>&1\"` wrap the whole command
                // — popen → cmd.exe strips one quote pair, leaving the
                // inner per-arg quotes intact. Without this wrap cmd.exe
                // misparses the very first quoted token as the command name.
                const auto linkerParam = isShared ? "-shared" : "";
                cmd = compilerLibrary.empty()
                    ? fmt::format(FMT_STRING("\"\"{}\" {} -o \"{}\" \"{}\" \"{}\" {} 2>&1\""),
                                            linker.c_str(), linkerParam, libraryName, objFilePath, runtimeLibrary.c_str(), extra)
                    : fmt::format(FMT_STRING("\"\"{}\" {} -o \"{}\" \"{}\" \"{}\" \"{}\" {} 2>&1\""),
                                            linker.c_str(), linkerParam, libraryName, objFilePath, runtimeLibrary.c_str(), compilerLibrary.c_str(), extra);
            #endif
        #elif defined(__APPLE__)
            const auto linkerParam = isShared ? "-shared " : "";
            // @executable_path first → relocated bundle finds dylibs next to the exe;
            // build-tree path second → dev workflow keeps working without copying dylibs.
            // The embedded `\" \"` splits the format-string's outer quotes so the linker
            // sees two distinct -Wl,-rpath flags, not one with an embedded space.
            const auto rpath = "-Wl,-rpath,@executable_path\" \"-Wl,-rpath," + get_prefix(runtimeLibrary);
            cmd = compilerLibrary.empty()
                ? fmt::format(FMT_STRING("\"{}\" {} \"{}\" -o \"{}\" \"{}\" \"{}\" {} 2>&1"), linker.c_str(), linkerParam, rpath.c_str(), libraryName, runtimeLibrary.c_str(), objFilePath, extra)
                : fmt::format(FMT_STRING("\"{}\" {} \"{}\" -o \"{}\" \"{}\" \"{}\" \"{}\" {} 2>&1"), linker.c_str(), linkerParam, rpath.c_str(), libraryName, runtimeLibrary.c_str(), compilerLibrary.c_str(), objFilePath, extra);
        #else
            const auto linkerParam = isShared ? "-shared" : "";
            // $ORIGIN first → relocated bundle finds .so next to the exe;
            // build-tree path second → dev workflow keeps working without copying.
            // \$ escapes the dollar so popen's shell passes $ORIGIN to ld unexpanded.
            // The embedded `\" \"` splits the format-string's outer quotes so the linker
            // sees two distinct -Wl,-rpath flags, not one with an embedded space.
            const auto rpath = "-Wl,-rpath,\\$ORIGIN\" \"-Wl,-rpath," + get_prefix(runtimeLibrary);
            cmd = compilerLibrary.empty()
                ? fmt::format(FMT_STRING("\"{}\" {} \"{}\" -o \"{}\" \"{}\" \"{}\" {} 2>&1"),
                                        linker.c_str(), linkerParam, rpath.c_str(), libraryName, objFilePath, runtimeLibrary.c_str(), extra)
                : fmt::format(FMT_STRING("\"{}\" {} \"{}\" -Wl,--no-as-needed -o \"{}\" \"{}\" \"{}\" \"{}\" {} 2>&1"),
                                        linker.c_str(), linkerParam, rpath.c_str(), libraryName, objFilePath, runtimeLibrary.c_str(), compilerLibrary.c_str(), extra);
        #endif
        return run_link_cmd(cmd.c_str(), libraryName, "Library", context);
    }
#else
    bool create_shared_library ( const char * objFilePath, const char * libraryName, [[maybe_unused]] const char * dasLib, const char * customLinker, const char * extraLinkerArgs, bool isShared, bool linkWholeLib, Context *context ) { return true; }
#endif

#if (defined(_WIN32) || defined(__linux__) || defined(__APPLE__)) && !defined(_GAMING_XBOX) && !defined(_DURANGO)
    // Cross-compile-link a wasm32 object into a standalone .wasm via emcc.
    //   runtimeLibPath — path to libDaScript_runtime.a (wasm32). Optional:
    //     present  → link runtime symbols (das_*, malloc, memcpy, …) in.
    //     missing  → link without; only safe for pure programs that touch
    //                no daslang runtime.
    //   customEmcc — explicit emcc override (nullptr/empty = resolve via
    //                bin/ then PATH).
    // emcc drives wasm-ld plus libc / wasi / wasm-exceptions glue, so output
    // is self-contained -sSTANDALONE_WASM (only wasi imports).
    bool link_wasm ( const char * objFilePath, const char * wasmPath,
                     const char * runtimeLibPath, const char * customEmcc,
                     Context * context ) {
        #if defined(_WIN32) || defined(_WIN64)
            const auto linker = find_linker(customEmcc, "emcc.bat", "emcc");
        #else
            const auto linker = find_linker(customEmcc, "emcc", "emcc");
        #endif
        if ( !check_file_present(objFilePath) ) {
            LOG(LogLevel::error) << "File '" << objFilePath << "' , containing wasm32 object, does not exist\n";
            return false;
        }
        const bool withRuntime = runtimeLibPath != nullptr && runtimeLibPath[0] != '\0'
                                 && check_file_present(runtimeLibPath);
        if ( runtimeLibPath != nullptr && runtimeLibPath[0] != '\0' && !withRuntime ) {
            LOG(LogLevel::warning) << "libDaScript_runtime archive '" << runtimeLibPath
                                   << "' not found - linking without; runtime refs will fail\n";
        }
        // -sSTANDALONE_WASM: emit self-contained .wasm with wasi imports only.
        // -fwasm-exceptions + -sWASM_LEGACY_EXCEPTIONS=0: match the runtime
        // archive's modern wasm EH, avoid emcc's JS invoke_* trampolines.
        // -sINITIAL_MEMORY=128MB: standalone wasm cannot grow memory under
        // wasmtime (-sALLOW_MEMORY_GROWTH adds an unsatisfiable
        // emscripten_notify_memory_growth import), so reserve up front. Cost
        // is virtual address space only — lazy paging keeps RSS proportional
        // to actual use. 128MB covers all current playground benchmarks; see
        // #2805.
        const std::string runtimeArg = withRuntime ? fmt::format("\"{}\" ", runtimeLibPath) : "";
        const std::string cmd = fmt::format(
            FMT_STRING("\"{}\" \"{}\" {}-o \"{}\" -sSTANDALONE_WASM -fwasm-exceptions -sWASM_LEGACY_EXCEPTIONS=0 -sINITIAL_MEMORY=128MB 2>&1"),
            linker.c_str(), objFilePath, runtimeArg, wasmPath);
        return run_link_cmd(cmd.c_str(), wasmPath, "Wasm", context);
    }
#else
    bool link_wasm ( const char *, const char *, const char *, const char *, Context * ) { return true; }
#endif

    void jit_set_jit_state(Context & context, void *shared_lib, void *llvm_ee, void *llvm_context) {
        context.deleteJITOnFinish.shared_lib = shared_lib;
        context.deleteJITOnFinish.llvm_ee = llvm_ee;
        context.deleteJITOnFinish.llvm_context = llvm_context;
    }

    void jit_get_jit_state(const TBlock<void,const void*, const void*, const void*> &block, Context * context, LineInfoArg * at) {
        vec4f args[3];
        args[0] = cast<void *>::from(context->deleteJITOnFinish.shared_lib);
        args[1] = cast<void *>::from(context->deleteJITOnFinish.llvm_ee);
        args[2] = cast<void *>::from(context->deleteJITOnFinish.llvm_context);
        context->invoke(block, args, nullptr, at);
    }

    class Module_Jit : public Module {
    public:
        Module_Jit() : Module("jit") {
            DAS_PROFILE_SECTION("Module_Jit");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            addBuiltinDependency(lib, Module::require("rtti_core"));
            addBuiltinDependency(lib, Module::require("ast_core"));
            addExtern<DAS_BIND_FUN(das_invoke_code)>(*this, lib, "invoke_code",
                SideEffects::worstDefault, "das_invoke_code")
                    ->args({"code","arguments","cmres","context"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(das_instrument_jit)>(*this, lib, "instrument_jit",
                SideEffects::worstDefault, "das_instrument_jit")
                    ->args({"code","function","at", "context"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(das_remove_jit)>(*this, lib, "remove_jit",
                SideEffects::worstDefault, "das_remove_jit")
                    ->args({"function"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(das_instrument_line_info)>(*this, lib, "instrument_line_info",
                SideEffects::worstDefault, "das_instrument_line_info")
                    ->args({"info","context","at"});
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
            addExtern<DAS_BIND_FUN(das_get_jit_simnode_interop)>(*this, lib, "get_jit_simnode_interop",
                SideEffects::none, "das_get_jit_simnode_interop");
            addExtern<DAS_BIND_FUN(das_get_jit_free_simnode_interop)>(*this, lib, "get_jit_free_simnode_interop",
                SideEffects::none, "das_get_jit_free_simnode_interop");
            addExtern<DAS_BIND_FUN(das_get_jit_init_extern_function)>(*this, lib, "get_jit_init_extern_function",
                SideEffects::none, "get_jit_init_extern_function");
            addExtern<DAS_BIND_FUN(das_get_jit_get_global_mnh)>(*this, lib, "get_jit_get_global_mnh",
                SideEffects::none, "das_get_jit_get_global_mnh");
            addExtern<DAS_BIND_FUN(das_get_jit_get_shared_mnh)>(*this, lib, "get_jit_get_shared_mnh",
                SideEffects::none, "das_get_jit_get_shared_mnh");
            addExtern<DAS_BIND_FUN(das_get_jit_get_globals_base)>(*this, lib, "get_jit_get_globals_base",
                SideEffects::none, "das_get_jit_get_globals_base");
            addExtern<DAS_BIND_FUN(das_get_jit_get_shared_base)>(*this, lib, "get_jit_get_shared_base",
                SideEffects::none, "das_get_jit_get_shared_base");
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
            addExtern<DAS_BIND_FUN(das_get_jit_string_table_at_with_hash)>(*this, lib, "get_jit_string_table_at_with_hash",
                SideEffects::none, "das_get_jit_string_table_at_with_hash");
            addExtern<DAS_BIND_FUN(das_get_jit_string_table_at_after_packed_miss)>(*this, lib, "get_jit_string_table_at_after_packed_miss",
                SideEffects::none, "das_get_jit_string_table_at_after_packed_miss");
            addExtern<DAS_BIND_FUN(das_get_global_variable_offset)>(*this, lib, "get_global_variable_offset",
                SideEffects::none, "das_get_global_variable_offset");
            addExtern<DAS_BIND_FUN(das_get_global_variable_mnh)>(*this, lib, "get_global_variable_mnh",
                SideEffects::none, "das_get_global_variable_mnh");
            addExtern<DAS_BIND_FUN(das_get_context_globals_size)>(*this, lib, "get_context_globals_size",
                SideEffects::none, "das_get_context_globals_size");
            addExtern<DAS_BIND_FUN(das_get_context_shared_size)>(*this, lib, "get_context_shared_size",
                SideEffects::none, "das_get_context_shared_size");
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
            addExtern<DAS_BIND_FUN(das_get_jit_new)>(*this, lib,  "get_jit_new",
                SideEffects::none, "das_get_jit_new")->args({"ann"});
            addExtern<DAS_BIND_FUN(das_get_jit_delete)>(*this, lib,  "get_jit_delete",
                SideEffects::none, "das_get_jit_delete")->args({"ann"});
            addExtern<DAS_BIND_FUN(das_get_jit_clone)>(*this, lib, "get_jit_clone",
                SideEffects::none, "das_get_jit_clone")->args({"ann"});
            addExtern<DAS_BIND_FUN(das_get_jit_debug_enter)>(*this, lib,  "get_jit_debug_enter",
                SideEffects::none, "das_get_jit_debug_enter");
            addExtern<DAS_BIND_FUN(das_get_jit_debug_exit)>(*this, lib,  "get_jit_debug_exit",
                SideEffects::none, "das_get_jit_debug_exit");
            addExtern<DAS_BIND_FUN(das_get_jit_debug_line)>(*this, lib,  "get_jit_debug_line",
                SideEffects::none, "das_get_jit_debug_line");
            addExtern<DAS_BIND_FUN(das_get_jit_initialize_fileinfo)>(*this, lib,  "get_jit_initialize_fileinfo",
                SideEffects::none, "das_get_jit_initialize_fileinfo");
            addExtern<DAS_BIND_FUN(das_get_jit_free_fileinfo)>(*this, lib,  "get_jit_free_fileinfo",
                SideEffects::none, "das_get_jit_free_fileinfo");
            addExtern<DAS_BIND_FUN(jit_get_initialize_varinfo_annotations)>(*this, lib,  "get_initialize_varinfo_annotations",
                SideEffects::none, "jit_get_initialize_varinfo_annotations");
            addExtern<DAS_BIND_FUN(jit_get_free_varinfo_annotations)>(*this, lib,  "get_free_varinfo_annotations",
                SideEffects::none, "jit_get_free_varinfo_annotations");
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
                    ->args({"objFilePath","libraryName","dasLib","customLinker","extraLinkerArgs","isShared","linkWholeLib","context"});
            addExtern<DAS_BIND_FUN(host_jit_triple)>(*this, lib, "host_jit_triple",
                SideEffects::none, "host_jit_triple");
            addExtern<DAS_BIND_FUN(link_wasm)>(*this, lib,  "link_wasm",
                SideEffects::worstDefault, "link_wasm")
                    ->args({"objFilePath","wasmPath","runtimeLibPath","customEmcc","context"});
            addExtern<DAS_BIND_FUN(jit_set_jit_state)>(*this, lib,  "set_jit_state",
                SideEffects::worstDefault, "jit_set_jit_state")
                    ->args({"context","shared_lib","llvm_ee","llvm_ctx"});
            addExtern<DAS_BIND_FUN(jit_get_jit_state)>(*this, lib,  "get_jit_state",
                SideEffects::worstDefault, "jit_get_jit_state")
                    ->args({"block","context","at"});
            addConstant<uint32_t>(*this, "SIZE_OF_PROLOGUE", uint32_t(sizeof(Prologue)));
            addConstant<uint32_t>(*this, "SIZE_OF_SIMNODE_INTEROP", uint32_t(sizeof(SimNode_AotInteropBase)));
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

// Test seam: unit tests override the exe-path source so resolution can be
// exercised with synthetic layouts. nullptr = use real getExecutableFileName.
// Returned char* must remain valid for the duration of one resolve call.
static const char * (*g_jit_exe_file_for_test)() = nullptr;

// Test predicate "does this path exist?" (default: real filesystem). Tests
// can swap in a mock predicate to exercise resolution without touching disk.
static bool (*g_jit_path_exists_for_test)(const char *) = nullptr;

#if !DAS_NO_FILEIO
static bool jit_path_exists ( const char * p ) {
    return g_jit_path_exists_for_test ? g_jit_path_exists_for_test(p) : das::builtin_fexist(p);
}

// Try a candidate dir / rel_path; if it (or its _debug variant in debug
// builds, gated on DAS_NO_ASSERTIONS to match register_dynamic_module's
// rewrite at module_builtin_fio.cpp:1099) exists, return the path to load.
// Empty string = miss.
//
// Returns the non-_debug name even when the _debug variant is what exists —
// register_dynamic_module applies the _debug rewrite itself in debug builds,
// so we mustn't double-rewrite. std::filesystem::path::operator/ handles
// trailing-separator and root cases correctly (POSIX `/` + `modules/X` →
// `/modules/X`, not `//modules/X`).
static das::string pick_at_dir ( const std::filesystem::path & dir, const char * rel_path ) {
    namespace fs = std::filesystem;
    if ( dir.empty() || !rel_path || !*rel_path ) return "";
    fs::path candidate = dir / rel_path;
    das::string candidate_str = candidate.string().c_str();
    if ( jit_path_exists(candidate_str.c_str()) ) return candidate_str;
#ifndef DAS_NO_ASSERTIONS
    if ( candidate.extension() == ".shared_module" ) {
        fs::path dbg = candidate.parent_path() / (candidate.stem().string() + "_debug" + candidate.extension().string());
        if ( jit_path_exists(dbg.string().c_str()) ) return candidate_str;
    }
#endif
    return "";
}

// Pure resolution: pick the path to load, in priority order:
//   1. <exe_dir>/<rel_path>   — daspkg release bundle layout (modules sit next to the exe)
//   2. <das_root>/<rel_path>  — SDK install layout AND local dev build
//   3. <fallback_abs_path>    — baked-at-codegen absolute path (legacy / paths without /modules/ segment)
//
// Tiers 1+2 are skipped when rel_path is empty/null. Returns the chosen path;
// tier 3 is unconditional, so the result is never empty when fallback is set.
static das::string resolve_dynamic_module_path ( const char * rel_path, const char * fallback_abs_path ) {
    namespace fs = std::filesystem;
    // tier 1 — exe_dir (parent_path correctly preserves filesystem root: "/myapp" → "/", "C:\myapp.exe" → "C:\")
    das::string exeFile;
    if ( g_jit_exe_file_for_test ) {
        const char * s = g_jit_exe_file_for_test();
        if ( s ) exeFile = s;
    } else {
        exeFile = das::getExecutableFileName();
    }
    if ( !exeFile.empty() ) {
        fs::path exeDir = fs::path(exeFile.c_str()).parent_path();
        if ( exeDir.empty() ) exeDir = ".";  // exe is a bare filename relative to cwd
        das::string p = pick_at_dir(exeDir, rel_path);
        if ( !p.empty() ) return p;
    }
    // tier 2 — das_root
    {
        das::string p = pick_at_dir(fs::path(das::getDasRoot().c_str()), rel_path);
        if ( !p.empty() ) return p;
    }
    // tier 3 — baked absolute (legacy fallback)
    return fallback_abs_path ? fallback_abs_path : "";
}
#else
// No file IO (DAS_NO_FILEIO): there is no filesystem to resolve a dynamic
// module against, and a build with no fio can't dlopen one anyway. Calling
// this is a logic error on such a platform, so abort loudly.
static das::string resolve_dynamic_module_path ( const char *, const char * ) {
    std::abort();
}
#endif

extern "C" {
DAS_API void das_ensure_environment () {
    das::daScriptEnvironment::ensure();
}

DAS_API void jit_initialize_modules () {
    // No need to initialize modules. JIT will generate required calls.
    das::daScriptEnvironment::ensure();
}

DAS_API void jit_initialize_modules_done () {
    das::Module::Initialize();
}

// Standalone-exe teardown. Emitted by inject_main right before main returns,
// so debug agents and modules drain while the runtime is alive. Without this,
// the static g_DebugAgents map dtor races ref_count_mutex during
// __cxa_finalize_ranges and terminate() fires (issue #2583).
DAS_API void jit_shutdown () {
    das::Module::ShutdownStandalone();
}

DAS_API void * jit_register_dynamic_module ( const char * path, const char * mod_name ) {
    return das::register_dynamic_module(path, mod_name, 0/*Quiet*/, nullptr, nullptr);
}

DAS_API void jit_set_exe_file_for_test_( const char * (*fn)() ) {
    g_jit_exe_file_for_test = fn;
}

DAS_API void jit_set_path_exists_for_test_( bool (*fn)(const char *) ) {
    g_jit_path_exists_for_test = fn;
}

// Test entry point: invoke pure resolution and return the chosen path via
// caller-owned buffer. Returns true on success (buf populated, NUL-terminated),
// false if buffer is too small. Unit tests use this to assert resolution order
// without dlopening anything.
DAS_API bool jit_resolve_dynamic_module_path_for_test_ ( const char * rel_path,
                                                         const char * fallback_abs_path,
                                                         char * out_buf,
                                                         size_t buf_size ) {
    das::string r = resolve_dynamic_module_path(rel_path, fallback_abs_path);
    if ( r.size() + 1 > buf_size ) return false;
    memcpy(out_buf, r.c_str(), r.size() + 1);
    return true;
}

// Resolve and load a dynamic module. Used by standalone exes (emitted by
// inject_main in llvm_exe.das).
DAS_API void * jit_register_dynamic_module_resolve ( const char * rel_path,
                                                     const char * fallback_abs_path,
                                                     const char * mod_name ) {
    das::string chosen = resolve_dynamic_module_path(rel_path, fallback_abs_path);
    return das::register_dynamic_module(chosen.c_str(), mod_name, 0/*Quiet*/, nullptr, nullptr);
}

DAS_API void jit_register_native_path ( const char * mod_name, const char * src_path, const char * dst_path ) {
    das::register_native_path(mod_name, src_path, dst_path, nullptr, nullptr);
}
}
