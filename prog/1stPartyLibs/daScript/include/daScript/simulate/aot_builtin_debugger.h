#pragma once

namespace das {

    DebugAgentPtr makeDebugAgent ( const void * pClass, const StructInfo * info, Context * context );
    void debuggerStackWalk ( Context & context, const LineInfo & lineInfo );
    void debuggerSetContextSingleStep ( Context & context, bool step );

    DataWalkerPtr makeDataWalker ( const void * pClass, const StructInfo * info, Context * context );
    void dapiWalkData ( DataWalkerPtr walker, void * data, const TypeInfo & info );
    void dapiWalkDataV ( DataWalkerPtr walker, float4 data, const TypeInfo & info );
    void dapiWalkDataS ( DataWalkerPtr walker, void * data, const StructInfo & info );

    StackWalkerPtr makeStackWalker ( const void * pClass, const StructInfo * info, Context * context );

    vec4f pinvoke_impl ( Context & context, SimNode_CallBase * call, vec4f * args );
    vec4f pinvoke_impl2 ( Context & context, SimNode_CallBase * call, vec4f * args );
    vec4f pinvoke_impl3 ( Context & context, SimNode_CallBase * call, vec4f * args );
    vec4f invokeInDebugAgent ( Context & context, SimNode_CallBase * call, vec4f * args );

    vec4f get_global_variable ( Context & context, SimNode_CallBase * call, vec4f * args );
    vec4f get_global_variable_by_index ( Context & context, SimNode_CallBase * node, vec4f * args );

    void instrument_context_allocations ( Context & ctx, bool isInstrumenting );
    void instrument_context_node ( Context & ctx, bool isInstrumenting, const TBlock<bool,LineInfo> & blk );
    void instrument_function ( Context & ctx, Func fn, bool isInstrumenting, uint64_t userData, Context * context, LineInfoArg * arg );
    void instrument_all_functions ( Context & ctx );
    void instrument_all_functions_thread_local ( Context & ctx );
    void instrument_all_functions_ex ( Context & ctx, const TBlock<uint64_t,Func,const SimFunction *> & blk, Context * context, LineInfoArg * arg );
    void instrument_all_functions_thread_local_ex ( Context & ctx, const TBlock<uint64_t,Func,const SimFunction *> & blk, Context * context, LineInfoArg * arg );
    void clear_instruments ( Context & ctx );

    bool has_function ( Context & ctx, const char * name );

    int32_t set_hw_breakpoint ( Context & ctx, void * address, int32_t size, bool writeOnly );
    bool clear_hw_breakpoint ( int32_t bpi );

    void break_on_free ( Context & ctx, void * ptr, uint32_t size );

    void track_insane_pointer ( void * ptr, Context * ctx );
}
