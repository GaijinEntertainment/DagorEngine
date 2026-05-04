#pragma once

namespace das {

    DAS_API DebugAgentPtr makeDebugAgent ( const void * pClass, const StructInfo * info, Context * context );
    DAS_API void debuggerStackWalk ( Context & context, const LineInfo & lineInfo );
    DAS_API void debuggerSetContextSingleStep ( Context & context, bool step );

    DAS_API DataWalkerPtr makeDataWalker ( const void * pClass, const StructInfo * info, Context * context );
    DAS_API void dapiWalkData ( DataWalkerPtr walker, void * data, const TypeInfo & info );
    DAS_API void dapiWalkDataV ( DataWalkerPtr walker, float4 data, const TypeInfo & info );
    DAS_API void dapiWalkDataS ( DataWalkerPtr walker, void * data, const StructInfo & info );

    DAS_API StackWalkerPtr makeStackWalker ( const void * pClass, const StructInfo * info, Context * context );

    DAS_API vec4f pinvoke_impl ( Context & context, SimNode_CallBase * call, vec4f * args );
    DAS_API vec4f pinvoke_impl2 ( Context & context, SimNode_CallBase * call, vec4f * args );
    DAS_API vec4f pinvoke_impl3 ( Context & context, SimNode_CallBase * call, vec4f * args );
    DAS_API vec4f invokeInDebugAgent ( Context & context, SimNode_CallBase * call, vec4f * args );

    DAS_API vec4f get_global_variable ( Context & context, SimNode_CallBase * call, vec4f * args );
    DAS_API vec4f get_global_variable_by_index ( Context & context, SimNode_CallBase * node, vec4f * args );

    DAS_API void instrument_context ( Context & ctx, bool isInstrumenting, const TBlock<bool,LineInfo> & blk, Context * context, LineInfoArg * line );
    DAS_API void instrument_context_allocations ( Context & ctx, bool isInstrumenting );
    DAS_API void instrument_context_node ( Context & ctx, bool isInstrumenting, const TBlock<bool,LineInfo> & blk );
    DAS_API void instrument_function ( Context & ctx, Func fn, bool isInstrumenting, uint64_t userData, Context * context, LineInfoArg * arg );
    DAS_API void instrument_all_functions ( Context & ctx );
    DAS_API void instrument_all_functions_thread_local ( Context & ctx );
    DAS_API void instrument_all_functions_ex ( Context & ctx, const TBlock<uint64_t,Func,const SimFunction *> & blk, Context * context, LineInfoArg * arg );
    DAS_API void instrument_all_functions_thread_local_ex ( Context & ctx, const TBlock<uint64_t,Func,const SimFunction *> & blk, Context * context, LineInfoArg * arg );
    DAS_API void clear_instruments ( Context & ctx );

    DAS_API bool has_function ( Context & ctx, const char * name );

    DAS_API int32_t set_hw_breakpoint ( Context & ctx, void * address, int32_t size, bool writeOnly );
    DAS_API bool clear_hw_breakpoint ( int32_t bpi );

    DAS_API void break_on_free ( Context & ctx, void * ptr, uint32_t size );

    DAS_API void track_insane_pointer ( void * ptr, Context * ctx );

    DAS_API void free_temp_string ( Context & context, LineInfoArg * lineInfo );
    DAS_API uint64_t temp_string_size ( Context & context );
}
