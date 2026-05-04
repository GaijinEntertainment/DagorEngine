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

namespace das {
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
                  size_t globSize, size_t shrSize, bool pinvoke) {
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
            memset(context.globals, 0, context.globalsSize);
            memset(context.shared, 0, context.sharedSize);

            // Instead of copying everything like in standalone contexts
            // Let's add only things we really need.
            // And apparently now we need nothing.
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
                                   bool cmres, bool fastcall, bool pinvoke ) {
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
                                                  bool pinvoke) {
        Context *context = new JitContext(totalVariables, totalFunctions, globalStringHeapSize,
                                         globalsSize, sharedSize, pinvoke);
        static_cast<JitContext *>(context)->allocFunctions(totalFunctions);
        return context;
    }

    DAS_API void *jit_register_standalone_function ( Context * ctx, uint64_t index,
                                             const char * name, const char * mangledName,
                                             uint64_t mnh, uint32_t stackSize,
                                             void * fnPtr,
                                             bool cmres, bool fastcall, bool pinvoke ) {
        return static_cast<JitContext *>(ctx)->registerJitFunction(index, name, mangledName, mnh, stackSize,
                                                            fnPtr, cmres, fastcall, pinvoke);
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
                *dllGlobal = static_cast<BuiltInFunction *>(fn.get())->getBuiltinAddress();
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
                        static_cast<BuiltInFunction *>(resolverFn.get())->getBuiltinAddress();
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
            result = module->findAnnotation(annName).get();
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
    void *das_get_jit_free_fileinfo () { return (void*)&jit_free_fileinfo; }
    void *jit_get_initialize_varinfo_annotations () { return (void*)&jit_initialize_varinfo_annotations; }
    void *jit_get_free_varinfo_annotations () { return (void*)&jit_free_varinfo_annotations; }
    void *das_get_jit_ast_typedecl () { return (void*)&jit_ast_typedecl; }

    template <typename KeyType>
    int32_t jit_table_at ( Table * tab, KeyType key, int32_t valueTypeSize, Context * context, LineInfoArg * at ) {
        if ( tab->isLocked() ) context->throw_error_at(at, "can't insert to a locked table");
        TableHash<KeyType> thh(context,valueTypeSize);
        auto hfn = hash_function(*context, key);
        return thh.reserve(*tab, key, hfn, at);
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
        return thh.find(*tab, key, hfn);
    }

    void * das_get_jit_table_find ( int32_t baseType, Context * context, LineInfoArg * at ) {
        JIT_TABLE_FUNCTION(&jit_table_find);
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

    static pair<string, string> get_real_lib_linker_paths(const char * dasLib, const char * customLinker) {
        string linker = customLinker != nullptr ? customLinker : "";
        string dasLibrary = dasLib != nullptr ? dasLib : "";
        if (linker.empty() || dasLibrary.empty()) {
            #if defined(_WIN32) || defined(_WIN64)
                if (linker.empty()) {
                    linker = getDasRoot() + "/bin/clang-cl.exe";
                }
                if (dasLibrary.empty()) {
                    const auto path = get_prefix(getExecutableFileName());
                    const auto winCfg = path.substr(path.find_last_of("\\/") + 1);
                    const auto windowsConfig = (winCfg == "bin" ? "" : (winCfg + "/"));
                    dasLibrary = getDasRoot() + "/lib/" + windowsConfig + "libDaScriptDyn.lib";
                }
            #else
                if (linker.empty()) {
                    linker = "c++";
                }
                if (dasLibrary.empty()) {
                #if defined(__APPLE__)
                    dasLibrary = getDasRoot() + "/lib/liblibDaScriptDyn.dylib";
                #else
                    dasLibrary = getDasRoot() + "/lib/liblibDaScriptDyn.so";
                #endif
                }
            #endif
        }
        return {linker, dasLibrary};
    }

#if (defined(_MSC_VER) || defined(__linux__) || defined(__APPLE__)) && !defined(_GAMING_XBOX) && !defined(_DURANGO)
    void create_shared_library ( const char * objFilePath, const char * libraryName, [[maybe_unused]] const char * dasLib, const char * customLinker, bool isShared ) {
        char cmd[1024];
        const auto [linker, dasLibrary] = get_real_lib_linker_paths(dasLib, customLinker);

        #if defined(_WIN32) || defined(_WIN64) || defined(__APPLE__)
        if (!check_file_present(dasLibrary.c_str())) {
            LOG(LogLevel::error) << "File '" << dasLibrary << "' , containing daslang library, does not exist\n";
            return;
        }
        #endif
        if (!check_file_present(objFilePath)) {
            LOG(LogLevel::error) << "File '" << objFilePath << "' , containing compiled definitions, does not exist\n";
            return;
        }


        #if defined(_WIN32) || defined(_WIN64)
            const auto linkerParam = isShared ? "-DLL" : "";
            auto result = fmt::format_to(cmd, FMT_STRING("\"\"{}\" \"{}\" \"{}\" msvcrt.lib -link {} -OUT:\"{}\" 2>&1\""), linker, objFilePath, dasLibrary, linkerParam, libraryName);
        #elif defined(__APPLE__)
            const auto linkerParam = isShared ? "-shared " : "";
            const auto rpath = "-Wl,-rpath," + get_prefix(dasLibrary);
            auto result = fmt::format_to(cmd, FMT_STRING("\"{}\" {} \"{}\" -o \"{}\" \"{}\" \"{}\" 2>&1"), linker, linkerParam, rpath, libraryName, dasLibrary, objFilePath);
        #else
            const auto linkerParam = isShared ? "-shared" : "";
            const auto rpath = "-Wl,-rpath," + get_prefix(dasLibrary);
            auto result = fmt::format_to(cmd, FMT_STRING("\"{}\" {} \"{}\" -o \"{}\" \"{}\" \"{}\" 2>&1"),
                                        linker, linkerParam, rpath, libraryName, objFilePath, dasLibrary);
        #endif
            *result = '\0';

#if defined(_WIN32) || defined(_WIN64)
    #define popen _popen
    #define pclose _pclose
#endif

        FILE * fp = popen(cmd, "r");
        if ( fp == NULL ) {
            LOG(LogLevel::error) << "Failed to run command '" << cmd << "'\n";
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
            LOG(LogLevel::error) << "Failed to make shared library " << libraryName << ", command '" << cmd << "'\n";
            printf("Output:\n%s", output);
        } else {
            LOG(LogLevel::debug) << "Library " << libraryName << " made - ok\n";
        }
    }
#else
    void create_shared_library ( const char * objFilePath, const char * libraryName, [[maybe_unused]] const char * dasLib, const char * customLinker, bool isShared ) { }
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
            addExtern<DAS_BIND_FUN(das_get_builtin_function_address)>(*this, lib,  "get_builtin_function_address",
                SideEffects::none, "das_get_builtin_function_address")
                    ->args({"fn","context","at"});
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
                    ->args({"objFilePath","libraryName","dasLib","customLinker", "isShared"});
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

static void init() {
    NEED_ALL_DEFAULT_MODULES;
    NEED_MODULE(Module_UriParser);
    NEED_MODULE(Module_JobQue);
}

extern "C" {
DAS_API void jit_initialize_modules () {
    init();
#ifdef DAS_ENABLE_DYN_INCLUDES
    das::daScriptEnvironment::ensure();
    auto access = das::make_smart<das::FsFileAccess>();
    das::TextPrinter tout;
    das::require_dynamic_modules(access, das::getDasRoot(), "", tout);
#endif
    das::Module::Initialize();
}
}
