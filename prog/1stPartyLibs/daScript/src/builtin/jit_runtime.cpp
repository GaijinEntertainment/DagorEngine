#include "daScript/daScriptModule.h"
#include "daScript/misc/platform.h"

#include "daScript/misc/sysos.h"

#ifdef DAS_ENABLE_DYN_INCLUDES
#include "daScript/ast/dyn_modules.h"
#endif

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_handle.h"

#include "daScript/simulate/aot.h"
#include "daScript/simulate/aot_builtin_jit.h"
#include "daScript/simulate/aot_builtin.h"
#include "daScript/simulate/aot_builtin_rtti.h"
#include "daScript/simulate/aot_library.h"
#include "daScript/simulate/debug_info.h"
#include "daScript/simulate/debug_print.h"
#include "daScript/simulate/hash.h"
#include "daScript/simulate/heap.h"
#include "daScript/simulate/simulate.h"
#include "daScript/simulate/simulate_visit_op.h"

#include "daScript/misc/fpe.h"
#include "misc/include_fmt.h"
#include "jit_runtime.h"

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <cstdlib>
#include <cstring>

#if !DAS_NO_FILEIO
#include <filesystem>
#endif

namespace das {
    // forward declarations from module_builtin_fio.cpp
    void * register_dynamic_module(const char *, const char *, int, Context *, LineInfoArg *);
    void register_native_path(const char *, const char *, const char *, Context *, LineInfoArg *);
    bool builtin_fexist ( const char * path );
    DAS_API void retry_pending_dynamic_modules();
    DAS_API int report_pending_dynamic_modules();

    // JitFunction typedef lives in arraytype.h (the SimFunction::jitFunction mirror shares it)

    struct SimNode_Jit : SimNode {
        SimNode_Jit ( const LineInfo & at, JitFunction eval )
            : SimNode(at), func(eval) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
        virtual bool rtti_node_isJit() const override { return true; }
        virtual void * rtti_node_jitFunction() const override { return (void *) func; }
        JitFunction func = nullptr;
        // saved original node
        SimNode * saved_code = nullptr;
        bool saved_aot = false;
        void * saved_aot_function = nullptr;
    };

    static void runLlvmAotGlobInitOf ( Context & ctx, void * publ );

    SimNode * makeAotJitNode ( Context & ctx, void * publ ) {
        runLlvmAotGlobInitOf(ctx, publ);
        return ctx.code->makeNode<SimNode_Jit>(LineInfo(), (JitFunction)publ);
    }

    extern "C" void * das_aot_get_func_by_mnh ( uint64_t mnh, Context * ctx ) {
        return builtin_getFunctionByMnh(mnh, ctx).PTR;
    }

    static vector<pair<uint64_t,void*>> & llvmAotEntries() {
        static vector<pair<uint64_t,void*>> entries;
        return entries;
    }

    static vector<void(*)(Context*)> & llvmAotGlobInits() {
        static vector<void(*)(Context*)> inits;
        return inits;
    }

    // publ -> index into llvmAotGlobInits, so linking one function can init the object it came from.
    static das_hash_map<void *,uint32_t> & llvmAotGlobInitOfPubl() {
        static das_hash_map<void *,uint32_t> ofPubl;
        return ofPubl;
    }

    // An object registers its functions first, then its glob-init; the functions wait here until
    // the glob-init arrives and claims them.
    static vector<void *> & llvmAotPendingPubl() {
        static vector<void *> pending;
        return pending;
    }

    // Runs an object's glob-init once: the slot is cleared as it is consumed.
    static void runLlvmAotGlobInitOf ( Context & ctx, void * publ ) {
        auto it = llvmAotGlobInitOfPubl().find(publ);
        if ( it==llvmAotGlobInitOfPubl().end() ) return;
        auto & fn = llvmAotGlobInits()[it->second];
        if ( !fn ) return;
        auto todo = fn;
        fn = nullptr;
        todo(&ctx);
    }
    static void registerLlvmAotFunctions ( AotLibrary & lib ) {
        for ( auto & e : llvmAotEntries() ) {
            lib.emplace(e.first, AotFactory(e.second));
        }
    }
    static AotListBase g_llvmAotList(registerLlvmAotFunctions);

    extern "C" void das_aot_register ( uint64_t aotHash, void * publ ) {
        llvmAotEntries().emplace_back(aotHash, publ);
        llvmAotPendingPubl().push_back(publ);
    }

    extern "C" void das_aot_register_globinit ( void (*fn)(Context*) ) {
        uint32_t index = uint32_t(llvmAotGlobInits().size());
        llvmAotGlobInits().push_back(fn);
        for ( auto publ : llvmAotPendingPubl() ) {
            llvmAotGlobInitOfPubl()[publ] = index;
        }
        llvmAotPendingPubl().clear();
    }

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
        if ( !blockPtr->argumentsOffset ) return context.result = func(&context, nullptr, nullptr, blockPtr );
        auto ba = (BlockArguments *) ( context.stack.bottom() + blockPtr->argumentsOffset );
        return context.result = func(&context, ba->arguments, ba->copyOrMoveResult, blockPtr );
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
            simfn->jitFunction = nullptr;   // pre-instrument mirror is always null (aot/jit exclusive)
            simfn->jit = false;
            return true;
        } else {
            return false;
        }
    }

    bool das_has_jit_fastpath ( const Func func ) {
        // test rail: is the invoke fastpath armed for this function (SimFunction::jitFunction mirror set)?
        auto simfn = func.PTR;
        return simfn && simfn->jitFunction;
    }

    bool das_instrument_jit ( void * pfun, const Func func, const LineInfo & lineInfo, Context & context ) {

        auto simfn = func.PTR;
        if ( !simfn ) return false;
        if ( simfn->code && simfn->code->rtti_node_isJit() ) {
            auto jitNode = static_cast<SimNode_Jit *>(simfn->code);
            jitNode->func = (JitFunction) pfun;
            jitNode->debugInfo = lineInfo;
            simfn->jitFunction = pfun;
        } else {
            auto node = context.code->makeNode<SimNode_Jit>(lineInfo, (JitFunction)pfun);
            node->saved_code = simfn->code;
            node->saved_aot = simfn->aot;
            node->saved_aot_function = simfn->aotFunction;
            simfn->code = node;
            simfn->aot = false;
            simfn->aotFunction = nullptr;
            simfn->jitFunction = pfun;      // the invoke-fastpath mirror of node->func
            simfn->jit = true;
        }
        return true;
    }

extern "C" {
    DAS_API void jit_exception ( const char * text, Context * context, LineInfoArg * at ) {
        context->throw_error_at(at, "%s", text ? text : "");
    }

    DAS_API vec4f jit_call_or_fastcall ( SimFunction * fn, vec4f * args, Context * context ) {
        if ( !fn ) context->throw_error("jit_call_or_fastcall: null function (unresolved LLVM-AOT call target)");
        return context->callOrFastcall(fn, args, nullptr);
    }

    DAS_API vec4f jit_invoke_block ( const Block & blk, vec4f * args, Context * context ) {
        return context->invoke(blk, args, nullptr, nullptr);
    }

    DAS_API vec4f jit_invoke_block_with_cmres ( const Block & blk, vec4f * args, void * cmres, Context * context ) {
        return context->invoke(blk, args, cmres, nullptr);
    }

    DAS_API vec4f jit_call_with_cmres ( SimFunction * fn, vec4f * args, void * cmres, Context * context ) {
        if ( !fn ) context->throw_error("jit_call_with_cmres: null function (unresolved LLVM-AOT call target)");
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
                  size_t globSize, size_t shrSize, bool pinvoke, uint32_t stackSize = 16*1024,
                  bool persistentHeap = false, bool gcEnabled = false)
            : Context(stackSize) {
            auto &context = *this;
            CodeOfPolicies policies;
            policies.debugger = false;
            // standalone exe skips Program::simulate (which would read `options
            // persistent_heap` / `options gc`), so set the heap mode here. Without a
            // persistent heap + gcEnabled, heap_collect() throws at runtime.
            policies.persistent_heap = persistentHeap;
            context.setup(totalVariables, globalStringHeapSize, policies, {});
            context.gcEnabled = gcEnabled;
            context.globalsSize = globSize;
            context.sharedSize = shrSize;
            context.sharedOwner = true;
            for (size_t i = 0; i < totalVariables; i++) {
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
            functionLookup = make_shared<NameLookup>();
            variableLookup = make_shared<NameLookup>();
        }

        void *registerJitFunction ( uint64_t index, const char * funcName, const char * mangledName,
                                   uint64_t mnh, uint32_t stackSize, void * fnPtr,
                                   bool cmres, bool fastcall, bool pinvoke, uint32_t nArguments ) {
            DAS_ASSERT(index < (uint64_t) totalFunctions);
            auto & fn = functions[index];
            fn.name = code->allocateName(funcName);
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
            fn.jitFunction = fnPtr;         // the invoke-fastpath mirror
            return &fn;
        }

        void registerJitGlobalVariable(uint64_t index, const char * name, uint64_t mnh, size_t offset, bool shared) {
            DAS_ASSERT(index < (uint64_t) totalVariables);
            auto & gv = globalVariables[index];
            gv.name = code->allocateName(name);
            gv.mangledNameHash = mnh;
            gv.offset = (uint32_t) offset;
            gv.flags = shared ? 1u : 0u;
        }

        // the exe carries both lookups as constant data the emitter sealed; nothing is built or owned here
        void adoptLookups(const NameLookup::StaticTable * functions, const NameLookup::StaticTable * variables) {
            functionLookup->adopt(*functions);
            variableLookup->adopt(*variables);
        }

        // registerJitGlobalVariable fills name, hash, offset and the shared flag; the
        // debugInfo is the exe-resident TypeInfo emitted by create_type_info_global, which
        // only the JIT'd init function can wire. collectHeap walks globalVariables[i] via
        // .offset/.debugInfo/.shared, so a NULL debugInfo is a GC crash.
        void setStandaloneGlobalInfo(uint64_t index, uint64_t offset, void* debugInfo, int shared) {
            DAS_ASSERT(index < (uint64_t) totalVariables);
            auto & gv = globalVariables[index];
            gv.offset = (uint32_t) offset;
            gv.debugInfo = (VarInfo *) debugInfo;
            gv.flags = shared ? 1u : 0u;
        }

        void setStandaloneFunctionArgInfo ( uint64_t index, uint32_t arg, void * debugInfo ) {
            DAS_ASSERT(index < (uint64_t) totalFunctions);
            auto finfo = functions[index].debugInfo;
            if ( !finfo || arg >= finfo->count ) return;
            if ( !finfo->fields ) {
                finfo->fields = (VarInfo **) code->allocate(sizeof(VarInfo *) * finfo->count);
                memset(finfo->fields, 0, sizeof(VarInfo *) * finfo->count);
            }
            // the emitter hands a TypeInfo; readers of fields[] expect a VarInfo (name, offset past it)
            auto vi = (VarInfo *) code->allocate(sizeof(VarInfo));
            memset(vi, 0, sizeof(VarInfo));
            memcpy((TypeInfo *) vi, debugInfo, sizeof(TypeInfo));
            vi->name = "";
            finfo->fields[arg] = vi;
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
                                         stackSize ? (uint32_t)stackSize : 16*1024,
                                         /*persistentHeap*/ true, /*gcEnabled*/ true);
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

    DAS_API void jit_register_standalone_variable ( Context * ctx, uint64_t index, const char * name, uint64_t mangledNameHash, uint64_t offset, int shared ) {
        static_cast<JitContext *>(ctx)->registerJitGlobalVariable(index, name, mangledNameHash, offset, shared != 0);
    }

    DAS_API void jit_adopt_standalone_lookups ( Context * ctx, const void * functions, const void * variables ) {
        static_cast<JitContext *>(ctx)->adoptLookups((const NameLookup::StaticTable *) functions, (const NameLookup::StaticTable *) variables);
    }

    // Populate globalVariables[index] so the GC can trace standalone-exe globals.
    // Emitted by generate_globals_initialization_fn into the init function (debugInfo
    // is the exe-resident TypeInfo, so it can only be wired at codegen time).
    DAS_API void jit_set_global_var ( Context * ctx, uint64_t index, uint64_t offset, void* debugInfo, int shared ) {
        static_cast<JitContext *>(ctx)->setStandaloneGlobalInfo(index, offset, debugInfo, shared);
    }

    DAS_API void jit_set_standalone_function_arg ( Context * ctx, uint64_t index, uint32_t arg, void * debugInfo ) {
        static_cast<JitContext *>(ctx)->setStandaloneFunctionArgInfo(index, arg, debugInfo);
    }

    DAS_API void jit_set_init_script ( Context * ctx, Context::JitInitScriptFn fn ) {
        ctx->jitInitScript = fn;
    }

    // A missing MODULE and a missing FUNCTION die on the same lookup — distinguish them,
    // because the former is a load failure (dlopen), not a signature mismatch.
    static bool jit_module_is_registered ( const char * moduleName ) {
        bool exists = false;
        Module::foreach([&](Module * module) -> bool {
            if ( module->name != moduleName ) return true;
            exists = true;
            return false;
        });
        return exists;
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
            if ( !jit_module_is_registered(moduleName) ) {
                DAS_FATAL_ERROR("Failed to find %s: module %s is not registered (its .shared_module may have failed to load - see errors above).\n", funcMangledName, moduleName);
            }
            das::string pending = describe_pending_dynamic_modules();
            DAS_FATAL_ERROR("Failed to find %s in module %s.%s%s\n", funcMangledName, moduleName,
                pending.empty() ? "" : " Dynamic modules still pending: ", pending.c_str());
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
            if ( !jit_module_is_registered(moduleName) ) {
                DAS_FATAL_ERROR("Failed to find annotation %s: module %s is not registered (its .shared_module may have failed to load - see errors above).\n", annName, moduleName);
            }
            DAS_FATAL_ERROR("Failed to find annotation %s in module %s.\n", annName, moduleName);
        }
        return result;
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

    // Resolve a handled-type (C++) field offset at runtime. The JIT bakes field
    // offsets via the HOST annotation's offsetof, which is wrong when cross-compiling
    // to a target whose C++ ABI lays the struct out differently — even at equal pointer
    // width (e.g. an MSVC host vs a clang/wasm64 target: vptr + multiple-base ordering
    // diverge for polymorphic classes like Context). The runtime archive (built for the
    // target) registers each handled type's annotation with the right offsetof, so
    // resolve by (module, type, field) here. Called once per offset-global at init,
    // after initialize_modules() has registered the modules.
    DAS_API uint32_t jit_get_handled_field_offset ( const char * moduleName,
                                                    const char * typeName,
                                                    const char * fieldName ) {
        uint32_t offset = (uint32_t)-1;
        Module::foreach([&](Module * module) -> bool {
            if ( module->name != moduleName ) return true;
            auto ann = module->findAnnotation(typeName);
            // handled-type annotations only — StructureAnnotation isn't a TypeAnnotation (see note in
            // jit_find_handled_annotation); the JIT only resolves offsets for handled-type fields.
            if ( ann && ann->rtti_isHandledTypeAnnotation() ) {
                offset = ((TypeAnnotation *)ann)->getFieldOffset(fieldName);
                return false; // stop iterating
            }
            return true;
        });
        // Compiler only requests offsets for fields it resolved on the host annotation; a
        // miss = target runtime annotation diverges (ABI/module mismatch). Fail loud, not -1.
        if ( offset == (uint32_t)-1 ) {
            DAS_FATAL_ERROR("jit: unresolved handled-type field offset %s::%s.%s (target runtime annotation diverges from the host).\n",
                moduleName, typeName, fieldName);
        }
        return offset;
    }

    // ---- ABI safe-check (opt-in via --jit-check-abi) ------------------------
    // Cross-compiling from an MSVC host to a wasm/clang target can bake a wrong
    // handled-type (C++ struct) SIZE or field OFFSET when the two C++ ABIs lay the
    // struct out differently (vptr/base ordering, or #ifdef WIN32/EMSCRIPTEN-
    // conditional members). The codegen emits, at startup, one check call per used
    // handled type (size) and per field (offset) carrying the HOST-baked value;
    // each compares against the TARGET runtime annotation and records every
    // divergence. jit_handled_abi_check_report() warns on stderr with them all at once
    // (nothing when clean) and the program runs on - a drift rarely breaks the app.
    static string g_abi_check_report;
    static int    g_abi_check_count = 0;        // mismatches
    static int    g_abi_types_checked = 0;      // type-size checks where the target annotation was found
    static int    g_abi_types_skipped = 0;      // ... not registered on target (module not linked here)
    static int    g_abi_fields_checked = 0;
    static int    g_abi_fields_skipped = 0;

    static TypeAnnotation * jit_find_handled_annotation ( const char * moduleName, const char * typeName ) {
        TypeAnnotation * found = nullptr;
        Module::foreach([&](Module * module) -> bool {
            if ( module->name != moduleName ) return true;
            auto ann = module->findAnnotation(typeName);
            // handled-type annotations only: StructureAnnotation derives from Annotation (not
            // TypeAnnotation), so casting one to TypeAnnotation* would be UB. The ABI sweep only
            // emits checks for handled (BasicStructureAnnotation) types, so this is also exact.
            if ( ann && ann->rtti_isHandledTypeAnnotation() ) {
                found = (TypeAnnotation *) ann;
                return false; // stop iterating
            }
            return true;
        });
        return found;
    }

    DAS_API void jit_check_handled_type_size ( const char * moduleName, const char * typeName, uint32_t hostSize ) {
        auto ann = jit_find_handled_annotation(moduleName, typeName);
        if ( !ann ) { g_abi_types_skipped ++; return; } // not registered on target -> module not linked here
        if ( !ann->isLocal() && !ann->canCopy() && !ann->canMove() ) { g_abi_types_skipped ++; return; }
        g_abi_types_checked ++;
        uint32_t targetSize = uint32_t(ann->getSizeOf());
        if ( targetSize != hostSize ) {
            char buf[256];
            snprintf(buf, sizeof(buf), "  size   %s::%s  host=%u target=%u\n",
                moduleName, typeName, hostSize, targetSize);
            g_abi_check_report += buf;
            g_abi_check_count ++;
        }
    }

    DAS_API void jit_check_handled_field_offset ( const char * moduleName, const char * typeName,
                                                  const char * fieldName, uint32_t hostOffset ) {
        auto ann = jit_find_handled_annotation(moduleName, typeName);
        if ( !ann ) { g_abi_fields_skipped ++; return; }
        g_abi_fields_checked ++;
        uint32_t targetOffset = ann->getFieldOffset(fieldName);
        if ( targetOffset != hostOffset ) {
            char buf[256];
            if ( targetOffset == (uint32_t)-1 ) // field absent on the target annotation
                snprintf(buf, sizeof(buf), "  offset %s::%s.%s  host=%u target=MISSING\n",
                    moduleName, typeName, fieldName, hostOffset);
            else
                snprintf(buf, sizeof(buf), "  offset %s::%s.%s  host=%u target=%u\n",
                    moduleName, typeName, fieldName, hostOffset, targetOffset);
            g_abi_check_report += buf;
            g_abi_check_count ++;
        }
    }

    DAS_API void jit_handled_abi_check_report () {
        if ( g_abi_check_count ) {
            fprintf(stderr, "JIT ABI CHECK: %d handled-type layout mismatch(es) (host-baked vs target runtime; "
                "types %d checked / %d skipped, fields %d checked / %d skipped):\n%s",
                g_abi_check_count, g_abi_types_checked, g_abi_types_skipped, g_abi_fields_checked, g_abi_fields_skipped,
                g_abi_check_report.c_str());
            fflush(stderr);
        }
        // reset so a subsequent sweep in the same process reports only its own results
        g_abi_types_checked = g_abi_types_skipped = g_abi_fields_checked = g_abi_fields_skipped = 0;
        g_abi_check_count = 0;
        g_abi_check_report.clear();
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

    DAS_API void jit_array_resize ( Array & arr, int newSize, int stride, Context * context, LineInfoArg * at ) {
        builtin_array_resize(arr, newSize, stride, context, at);
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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4611)  // setjmp + C++ object destruction
#endif

    DAS_API void WIN_EH_NO_ASAN jit_try_recover ( Block * try_block, Block * catch_block, void * lineInfo, Context * context ) {
        auto at = (LineInfoArg *) lineInfo;
        auto aa = context->abiArg; auto acm = context->abiCMRES; auto fcd = context->fastCallDepth;
        char * EP, * SP;
        context->stack.watermark(EP,SP);
        #if DAS_ENABLE_EXCEPTIONS
            try {
                context->invoke(*try_block, nullptr, nullptr, at);
            } catch ( const dasException & ) {
                context->abiArg = aa;
                context->abiCMRES = acm;
                context->fastCallDepth = fcd;
                context->stack.pop(EP,SP);
                context->stopFlags = 0;
                context->last_exception = context->exception;
                context->exception = nullptr;
                context->invoke(*catch_block, nullptr, nullptr, at);
            }
        #else
            jmp_buf ev;
            jmp_buf * JB = context->throwBuf;
            context->throwBuf = &ev;
            if ( !setjmp(ev) ) {
                context->invoke(*try_block, nullptr, nullptr, at);
            } else {
                context->throwBuf = JB;
                context->abiArg = aa;
                context->abiCMRES = acm;
                context->fastCallDepth = fcd; //-V1048
                context->stack.pop(EP,SP);
                context->stopFlags = 0;
                context->last_exception = context->exception;
                context->exception = nullptr;
                context->invoke(*catch_block, nullptr, nullptr, at);
            }
            context->throwBuf = JB;
        #endif
    }

#ifdef _MSC_VER
#pragma warning(pop)
#endif

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

    DAS_API uint64_t jit_ad_by_sid ( uint64_t sid, Context * context ) {
        if ( !context || !context->tabAdLookup ) return 0;
        auto it = context->tabAdLookup->find(sid);
        return it != context->tabAdLookup->end() ? it->second : 0;
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
    void *das_get_jit_get_handled_field_offset() { return (void *)&jit_get_handled_field_offset; }
    void *das_get_jit_check_handled_type_size() { return (void *)&jit_check_handled_type_size; }
    void *das_get_jit_check_handled_field_offset() { return (void *)&jit_check_handled_field_offset; }
    void *das_get_jit_handled_abi_check_report() { return (void *)&jit_handled_abi_check_report; }
    void *das_get_jit_alloc_heap() { return (void *)&jit_alloc_heap; }
    void *das_get_jit_alloc_persistent() { return (void *)&jit_alloc_persistent; }
    void *das_get_jit_free_heap() { return (void *)&jit_free_heap; }
    void *das_get_jit_free_persistent() { return (void *)&jit_free_persistent; }
    void *das_get_jit_array_resize() { return (void *)&builtin_array_resize; }
    void *das_get_jit_make_block() { return (void *)&jit_make_block; }
    void *das_get_jit_try_recover() { return (void *)&jit_try_recover; }
    void *das_get_jit_ad_by_sid() { return (void *)&jit_ad_by_sid; }
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
        DAS_API void * das_get_jit_each ( TypeAnnotation *annotation ) {
            return annotation->jitGetEach();
        }
        DAS_API void * das_get_jit_at ( TypeAnnotation *annotation, int32_t indexType ) {
            return annotation->jitGetAt(Type(indexType));
        }
    }
}

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

static das::string jit_exe_file () {
    if ( g_jit_exe_file_for_test ) {
        const char * s = g_jit_exe_file_for_test();
        return s ? das::string(s) : das::string();
    }
    return das::getExecutableFileName();
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
    das::string exeFile = jit_exe_file();
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

// Same three tiers for a native-path template's destination. It may be a PATTERN
// ("modules/dasLLVM/daslib/{path}.das"), so the probe is its directory prefix and the
// winner is that base with the whole pattern re-rooted onto it. Empty rel_pattern (no
// /modules/ segment at codegen) → tier 3.
static das::string resolve_native_path_dst ( const char * rel_pattern, const char * fallback_abs ) {
    namespace fs = std::filesystem;
    if ( rel_pattern && *rel_pattern ) {
        std::string rel = rel_pattern;
        size_t firstBrace = rel.find('{');       // npos → rfind searches the whole string
        size_t dirEnd = rel.rfind('/', firstBrace);
        std::string dirRel = dirEnd == std::string::npos ? "" : rel.substr(0, dirEnd);
        das::vector<fs::path> bases;
        das::string exeFile = jit_exe_file();
        if ( !exeFile.empty() ) {
            fs::path exeDir = fs::path(exeFile.c_str()).parent_path();
            bases.push_back(exeDir.empty() ? fs::path(".") : exeDir);
        }
        das::string dasRoot = das::getDasRoot();
        if ( !dasRoot.empty() ) bases.push_back(fs::path(dasRoot.c_str()));
        for ( auto & base : bases ) {
            fs::path dir = dirRel.empty() ? base : base / dirRel;
            if ( jit_path_exists(dir.string().c_str()) ) {
                return das::string((base / rel).string().c_str());
            }
        }
    }
    return fallback_abs ? fallback_abs : "";
}
#else
// No file IO (DAS_NO_FILEIO): there is no filesystem to resolve a dynamic
// module against, and a build with no fio can't dlopen one anyway. Calling
// this is a logic error on such a platform, so abort loudly.
static das::string resolve_dynamic_module_path ( const char *, const char * ) {
    std::abort();
}
// Unlike a dynamic module, a native-path entry is an inert string-table registration
// (register_native_path is a stubbed no-op under DAS_NO_FILEIO), so a standalone exe
// that registered one must keep starting - hand back the baked path, never abort.
static das::string resolve_native_path_dst ( const char *, const char * fallback_abs ) {
    return fallback_abs ? fallback_abs : "";
}
#endif

// Standalone-exe browser lifecycle (matches the interpreter's WebLoop in
// utils/daslang/main.cpp). A cross-compiled wasm graphics app exports
// init/update/shutdown, but its `main` is the blocking desktop driver. On the web
// the generated entry (llvm_exe.das) runs init() then calls jit_run_web_lifecycle
// instead of main: it installs an rAF loop on update() and runs shutdown() when
// update() returns false. updateFn/shutdownFn are jitted `RetT(Context*)` pointers;
// updateReturnsValue selects the void vs bool/int call signature (bool/int both
// return wasm i32, so they share one signature).
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
namespace {
    struct JitWebLifecycle {
        das::Context *  ctx;
        void *          updateFn;
        bool            updateReturnsValue;
        void *          shutdownFn;             // nullable
    };
    void jit_web_lifecycle_tick ( void * arg ) {
        auto * lc = (JitWebLifecycle *) arg;
        bool keepGoing = true;
        if ( lc->updateReturnsValue ) {
            keepGoing = ((int32_t(*)(das::Context*))lc->updateFn)(lc->ctx) != 0;
        } else {
            ((void(*)(das::Context*))lc->updateFn)(lc->ctx);
        }
        if ( keepGoing ) lc->ctx->collectHeapIfMostlyFree();
        if ( !keepGoing ) {
            emscripten_cancel_main_loop();
            if ( lc->shutdownFn ) ((void(*)(das::Context*))lc->shutdownFn)(lc->ctx);
        }
    }
}
#endif

extern "C" {
// See JitWebLifecycle note above. Defined for every target so the symbol always
// links; only the emscripten build installs the rAF loop (others block, but the
// generated entry only emits this call on the wasm target).
DAS_API void jit_run_web_lifecycle ( das::Context * ctx, void * updateFn,
                                     int32_t updateReturnsValue, void * shutdownFn ) {
#ifdef __EMSCRIPTEN__
    // arg leaks by design (lives the whole program). 0 = browser rAF cadence;
    // true = simulate_infinite_loop, so this never returns and the entry's
    // jit_shutdown() stays unreachable — the runtime persists for the rAF callbacks.
    auto * lc = new JitWebLifecycle{ ctx, updateFn, updateReturnsValue != 0, shutdownFn };
    emscripten_set_main_loop_arg(jit_web_lifecycle_tick, lc, 0, true);
#else
    bool keepGoing = true;
    while ( keepGoing ) {
        if ( updateReturnsValue ) keepGoing = ((int32_t(*)(das::Context*))updateFn)(ctx) != 0;
        else ((void(*)(das::Context*))updateFn)(ctx);
    }
    if ( shutdownFn ) ((void(*)(das::Context*))shutdownFn)(ctx);
#endif
}

// Standalone-exe main guard: the generated entry (llvm_exe.das) routes das main through this
// so a runtime exception prints and exits nonzero instead of unwinding out of the entry with
// no message (hosted runs get this boundary from their runWithCatch call sites; a bare exe
// had none — fix for the silent-exit-127 class). resultKind: 0 = void main, 1 = int main
// (value = exit code), 2 = bool main (true -> 0, false -> 1).
DAS_API int32_t jit_run_main_guarded ( das::Context * ctx, void * mainFn, int32_t resultKind ) {
    int32_t rc = 0;
    bool ok = ctx->runWithCatch([&]() {
        if ( resultKind == 1 ) {
            rc = ((int32_t(*)(das::Context*))mainFn)(ctx);
        } else if ( resultKind == 2 ) {
            rc = ((bool(*)(das::Context*))mainFn)(ctx) ? 0 : 1;
        } else {
            ((void(*)(das::Context*))mainFn)(ctx);
        }
    });
    if ( !ok ) {
        das::TextPrinter tp;
        tp << "EXCEPTION: " << (ctx->getException() ? ctx->getException() : "unknown") << "\n";
        tp.output();    // TextWriter's dtor only frees its buffer — output() is what prints
        return 1;
    }
    return rc;
}

DAS_API void das_ensure_environment () {
    das::daScriptEnvironment::ensure();
}

DAS_API void * jit_register_module_once ( const char * dasName, das::Module * (*reg)() ) {
    das::daScriptEnvironment::ensure();
    if ( das::Module * have = das::Module::require(dasName ? dasName : "") ) return have;
    return reg();
}

DAS_API int32_t jit_lib_run_once ( int32_t * guard, void ** env, void (*fn)() ) {
    static das::mutex once_mutex;
    das::lock_guard<das::mutex> lock(once_mutex);
    if ( *guard ) {
        if ( *env && das::daScriptEnvironment::getBound()!=*env ) {
            das::daScriptEnvironment::setBound((das::daScriptEnvironment *)*env);
        }
        return 1;
    }
    das::daScriptEnvironment::ensure();
    *env = das::daScriptEnvironment::getBound();
    *guard = das::daScriptEnvironment::getBound()->modules ? 2 : 1;
    {
        das::gc_guard registration_gc_scope;
        fn();
        das::Module::foreach([&](das::Module * m) {
            m->gc_collect(&registration_gc_scope.guard_root);
            return true;
        });
    }
    return 1;
}

DAS_API int32_t jit_lib_invoke_guarded ( das::Context * ctx, void (*tramp)(das::Context *, void *), void * frame ) {
    if ( !ctx ) return 0;
    ctx->clearException();
    if ( ctx->contextMutex ) {
        das::lock_guard<das::recursive_mutex> guard(*ctx->contextMutex);
        return ctx->runWithCatch([&]() { tramp(ctx, frame); }) ? 1 : 0;
    }
    return ctx->runWithCatch([&]() { tramp(ctx, frame); }) ? 1 : 0;
}

DAS_API das::Context * jit_lib_create_finish ( das::Context * ctx, int32_t ok, char ** err ) {
    if ( ok ) return ctx;
    const char * why = ctx ? ( ctx->getException() ? ctx->getException() : "unknown exception" )
                           : "out of memory";
    if ( err ) {
        free(*err);
        const size_t len = strlen(why) + 1;
        *err = (char *) malloc(len);
        if ( *err ) memcpy(*err, why, len);
    }
    delete ctx;
    return nullptr;
}

DAS_API const char * jit_lib_last_error ( das::Context * ctx ) {
    return ctx ? ctx->getException() : nullptr;
}

DAS_API void jit_destroy_standalone_ctx ( das::Context * ctx ) {
    delete ctx;
}

DAS_API void jit_initialize_modules_done ( int32_t guard ) {
    if ( guard==2 ) {
        das::string notInitialized;
        if ( !das::Module::InitializeDependencies(notInitialized) ) {
            das::LOG(das::LogLevel::error) << "LLVM LIB: unable to initialize modules:" << notInitialized << "\n";
        }
        auto & threadRoot = das::gc_root::gc_get_thread_root();
        das::Module::foreach([&](das::Module * m) {
            m->gc_collect(&threadRoot);
            return true;
        });
        return;
    }
    das::Module::Initialize();
}

// Standalone-exe teardown. Emitted by inject_main right before main returns,
// so debug agents and modules drain while the runtime is alive. Without this,
// the static g_DebugAgents map dtor races ref_count_mutex during
// __cxa_finalize_ranges and terminate() fires (issue #2583).
DAS_API void jit_shutdown () {
    das::Module::ShutdownStandalone();
}

DAS_API void jit_lib_shutdown ( int32_t guard ) {
    if ( guard!=1 ) return;
    das::Module::ShutdownStandalone();
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

DAS_API bool jit_resolve_native_path_dst_for_test_ ( const char * rel_pattern,
                                                    const char * fallback_abs,
                                                    char * out_buf,
                                                    size_t buf_size ) {
    das::string r = resolve_native_path_dst(rel_pattern, fallback_abs);
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

// Emitted by inject_main after the per-module jit_register_dynamic_module_resolve calls.
// Those load Quiet (sibling DT_NEEDED ordering makes a first-attempt failure normal), so
// run the fixed-point retry, then treat anything still unloadable as fatal: every module
// the exe registers is required by its program, and continuing only defers the death to
// the first extern lookup, whose message no longer names the real cause.
DAS_API void jit_finalize_dynamic_modules () {
    das::retry_pending_dynamic_modules();
    if ( int failed = das::report_pending_dynamic_modules() ) {
        DAS_FATAL_ERROR("%d dynamic module(s) failed to load (see above).\n", failed);
    }
}

DAS_API void jit_lib_finalize_dynamic_modules () {
    das::retry_pending_dynamic_modules();
    das::report_pending_dynamic_modules();
}

// Emitted by inject_main (llvm_exe.das) for every native path the program compiled
// against: the destination is re-rooted onto <exe_dir> / <das_root> at run time, so a
// bundle built elsewhere finds its own modules/ instead of the build machine's.
DAS_API void jit_register_native_path_resolve ( const char * mod_name, const char * src_path,
                                                const char * rel_dst, const char * fallback_abs_dst ) {
    das::string chosen = resolve_native_path_dst(rel_dst, fallback_abs_dst);
    das::register_native_path(mod_name, src_path, chosen.c_str(), nullptr, nullptr);
}
}
