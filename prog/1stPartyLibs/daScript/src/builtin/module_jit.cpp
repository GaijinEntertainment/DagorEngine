#include "daScript/misc/platform.h"

#include "daScript/misc/performance_time.h"
#include "daScript/simulate/aot.h"
#include "daScript/simulate/aot_builtin_jit.h"
#include "daScript/simulate/aot_builtin_ast.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/misc/fpe.h"
#include "module_builtin_rtti.h"
#include "module_builtin_ast.h"

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

    void jit_exception ( const char * text, Context * context, LineInfoArg * at ) {
        context->throw_error_at(at, "%s", text ? text : "");
    }

    void * das_get_jit_exception ( ) {
        return (void *) &jit_exception;
    }

    vec4f jit_call_or_fastcall ( SimFunction * fn, vec4f * args, Context * context ) {
        return context->callOrFastcall(fn, args, nullptr);
    }

    void * das_get_jit_call_or_fastcall ( ) {
        return (void *) &jit_call_or_fastcall;
    }

    vec4f jit_invoke_block ( const Block & blk, vec4f * args, Context * context ) {
        return context->invoke(blk, args, nullptr, nullptr);
    }

    void * das_get_jit_invoke_block ( ) {
        return (void *) &jit_invoke_block;
    }

    vec4f jit_invoke_block_with_cmres ( const Block & blk, vec4f * args, void * cmres, Context * context ) {
        return context->invoke(blk, args, cmres, nullptr);
    }

    void * das_get_jit_invoke_block_with_cmres ( ) {
        return (void *) &jit_invoke_block_with_cmres;
    }

    vec4f jit_call_with_cmres ( SimFunction * fn, vec4f * args, void * cmres, Context * context ) {
        return context->callWithCopyOnReturn(fn, args, cmres, nullptr);
    }

    void * das_get_jit_call_with_cmres ( ) {
        return (void *) &jit_call_with_cmres;
    }

    char * jit_string_builder ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        StringBuilderWriter writer;
        DebugDataWalker<StringBuilderWriter> walker(writer, PrintFlags::string_builder);
        for ( int i=0, is=call->nArguments; i!=is; ++i ) {
            walker.walk(args[i], call->types[i]);
        }
        auto length = writer.tellp();
        if ( length ) {
            return context.stringHeap->allocateString(writer.c_str(), length);
        } else {
            return nullptr;
        }
    }

    void * das_get_jit_string_builder ( ) {
        return (void *) &jit_string_builder;
    }

    void * jit_get_global_mnh ( uint64_t mnh, Context & context ) {
        return context.globals + context.globalOffsetByMangledName(mnh);
    }

    void * das_get_jit_get_global_mnh () {
        return (void *) &jit_get_global_mnh;
    }

    void * jit_alloc_heap ( uint32_t bytes, Context * context ) {
        return context->heap->allocate(bytes);
    }

    void * das_get_jit_alloc_heap () {
        return (void *) &jit_alloc_heap;
    }

    void * jit_alloc_persistent ( uint32_t bytes, Context * ) {
        return das_aligned_alloc16(bytes);
    }

    void * das_get_jit_alloc_persistent () {
        return (void *) &jit_alloc_persistent;
    }

    void jit_free_heap ( void * bytes, uint32_t size, Context * context ) {
        context->heap->free((char *)bytes,size);
    }

    void * das_get_jit_free_heap () {
        return (void *) &jit_free_heap;
    }

    void jit_free_persistent ( void * bytes, Context * ) {
        das_aligned_free16(bytes);
    }

    void * das_get_jit_free_persistent () {
        return (void *) &jit_free_persistent;
    }

    void * das_get_jit_array_lock () {
        return (void *) &builtin_array_lock;
    }

    void * das_get_jit_array_unlock () {
        return (void *) &builtin_array_unlock;
    }

    template <typename KeyType>
    int32_t jit_table_at ( Table * tab, KeyType key, int32_t valueTypeSize, Context * context ) {
        TableHash<KeyType> thh(context,valueTypeSize);
        auto hfn = hash_function(*context, key);
        return thh.reserve(*tab, key, hfn);
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

    int32_t jit_str_cmp ( char * a, char * b ) {
        return strcmp(a ? a : "",b ? b : "");
    }

    void * das_get_jit_str_cmp () {
        return (void *) &jit_str_cmp;
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

    void * das_get_jit_prologue () {
        return (void *) &jit_prologue;
    }

    void jit_epilogue ( JitStackState * stackState, Context * context ) {
        context->stack.pop(stackState->EP, stackState->SP);
    }

    void * das_get_jit_epilogue () {
        return (void *) &jit_epilogue;
    }

    void jit_make_block ( Block * blk, int32_t argStackTop, void * bodyNode, void * jitImpl, void * funcInfo, Context * context ) {
        JitBlock * block = (JitBlock *) blk;
        block->stackOffset = context->stack.spi();
        block->argumentsOffset = argStackTop ? (context->stack.spi() + argStackTop) : 0;
        block->body = (SimNode *)(void*) block->node;
        block->aotFunction = nullptr;
        block->jitFunction = jitImpl;
        block->functionArguments = context->abiArguments();
        block->info = (FuncInfo *) funcInfo;
        new (block->node) SimNode_JitBlock(LineInfo(), (JitBlockFunction) bodyNode);
    }

    void * das_get_jit_make_block () {
        return (void *) &jit_make_block;
    }

    void jit_debug ( vec4f res, TypeInfo * typeInfo, char * message, Context * context, LineInfoArg * at ) {
        FPE_DISABLE;
        TextWriter ssw;
        if ( message ) ssw << message << " ";
        ssw << debug_type(typeInfo) << " = " << debug_value(res, typeInfo, PrintFlags::debugger) << "\n";
        context->to_out(at, ssw.str().c_str());
    }

    void * das_get_jit_debug () {
        return (void *) &jit_debug;
    }

    void * das_get_jit_iterator_iterate() {
        return (void *) &builtin_iterator_iterate;
    }

    void * das_get_jit_iterator_delete() {
        return (void *) &builtin_iterator_delete;
    }

    void * das_get_jit_iterator_close() {
        return (void *) &builtin_iterator_close;
    }

    char * jit_str_cat ( char * sA, char * sB, Context * context ) {
        auto la = stringLength(*context, sA);
        auto lb = stringLength(*context, sB);
        uint32_t commonLength = la + lb;
        if ( !commonLength ) {
            return nullptr;
        } else if ( char * sAB = (char * ) context->stringHeap->allocateString(nullptr, commonLength) ) {
            memcpy ( sAB, sA, la );
            memcpy ( sAB+la, sB, lb+1 );
            context->stringHeap->recognize(sAB);
            return sAB;
        } else {
            context->throw_error("can't add two strings, out of heap"); // this is so unlikely
            return nullptr;
        }
    }

    void * das_get_jit_str_cat () {
        return (void *) &jit_str_cat;
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
        if ( !info_ptr ) context->throw_error_at(at, "can't instrument line info, out of heap");
        *info_ptr = info;
        return (void *) info_ptr;
    }

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
            addExtern<DAS_BIND_FUN(das_get_jit_get_global_mnh)>(*this, lib, "get_jit_get_global_mnh",
                SideEffects::none, "das_get_jit_get_global_mnh");
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
            addExtern<DAS_BIND_FUN(das_get_jit_iterator_close)>(*this, lib, "get_jit_iterator_close",
                SideEffects::none, "das_get_jit_iterator_close");
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
            addConstant<uint32_t>(*this, "SIZE_OF_PROLOGUE", uint32_t(sizeof(Prologue)));
            addConstant<uint32_t>(*this, "CONTEXT_OFFSET_OF_EVAL_TOP", uint32_t(uint32_t(offsetof(Context, stack) + offsetof(StackAllocator, evalTop))));
            addConstant<uint32_t>(*this, "CONTEXT_OFFSET_OF_GLOBALS", uint32_t(uint32_t(offsetof(Context, globals))));
            // lets make sure its all aot ready
            verifyAotReady();
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            tw << "#include \"daScript/simulate/aot_builtin_jit.h\"\n";
            return ModuleAotType::cpp;
        }
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_Jit,das);
