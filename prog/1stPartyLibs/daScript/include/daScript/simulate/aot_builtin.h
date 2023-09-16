#pragma once

namespace das {
    bool is_in_aot();
    bool is_in_completion();
    bool is_folding();
    void setCommandLineArguments ( int argc, char * argv[] );
    void getCommandLineArguments( Array & arr );
    bool is_compiling ( );
    bool is_compiling_macros ( );
    void builtin_throw ( char * text, Context * context, LineInfoArg * at );
    void builtin_print ( char * text, Context * context, LineInfoArg * at );
    void builtin_error ( char * text, Context * context, LineInfoArg * at );
    vec4f builtin_sprint ( Context & context, SimNode_CallBase * call, vec4f * args );
    vec4f builtin_json_sprint ( Context & context, SimNode_CallBase * call, vec4f * args );
    char * builtin_print_data ( void * data, const TypeInfo * typeInfo, Bitfield flags, Context * context );
    char * builtin_print_data_v ( float4 data, const TypeInfo * typeInfo, Bitfield flags, Context * context );
    char * builtin_debug_type ( const TypeInfo * typeInfo, Context * context );
    char * builtin_debug_line ( const LineInfo & at, bool fully, Context * context );
    char * builtin_get_typeinfo_mangled_name ( const TypeInfo * typeInfo, Context * context );
    const FuncInfo * builtin_get_function_info_by_mnh ( Context & context, Func fun );
    Func builtin_SimFunction_by_MNH ( Context & context, uint64_t MNH );
    vec4f builtin_breakpoint ( Context & context, SimNode_CallBase * call, vec4f * );
    void builtin_stackwalk ( bool args, bool vars, Context * context, LineInfoArg * lineInfo );
    void builtin_terminate ( Context * context, LineInfoArg * lineInfo );
    int builtin_table_size ( const Table & arr );
    int builtin_table_capacity ( const Table & arr );
    void builtin_table_clear ( Table & arr, Context * context, LineInfoArg * at );
    vec4f _builtin_hash ( Context & context, SimNode_CallBase * call, vec4f * args );
    void heap_stats ( Context & context, uint64_t * bytes );
    uint64_t heap_bytes_allocated ( Context * context );
    int32_t heap_depth ( Context * context );
    uint64_t string_heap_bytes_allocated ( Context * context );
    int32_t string_heap_depth ( Context * context );
    void string_heap_collect ( bool validate, Context * context, LineInfoArg * info );
    void string_heap_report ( Context * context, LineInfoArg * info );
    void heap_collect ( bool stringHeap, bool validate, Context * context, LineInfoArg * info );
    void heap_report ( Context * context, LineInfoArg * info );
    void memory_report ( bool errorsOnly, Context * context, LineInfoArg * info );
    void builtin_table_lock ( const Table & arr, Context * context, LineInfoArg * at );
    void builtin_table_unlock ( const Table & arr, Context * context, LineInfoArg * at );
    void builtin_table_clear_lock ( const Table & arr, Context * context );
    int builtin_array_size ( const Array & arr );
    int builtin_array_capacity ( const Array & arr );
    int builtin_array_lock_count ( const Array & arr );
    void builtin_array_resize ( Array & pArray, int newSize, int stride, Context * context, LineInfoArg * at );
    void builtin_array_resize_no_init ( Array & pArray, int newSize, int stride, Context * context, LineInfoArg * at );
    void builtin_array_reserve ( Array & pArray, int newSize, int stride, Context * context, LineInfoArg * at );
    void builtin_array_erase ( Array & pArray, int index, int stride, Context * context, LineInfoArg * at );
    void builtin_array_erase_range ( Array & pArray, int index, int count, int stride, Context * context, LineInfoArg * at );
    void builtin_array_clear ( Array & pArray, Context * context, LineInfoArg * at );
    void builtin_array_lock ( const Array & arr, Context * context, LineInfoArg * at );
    void builtin_array_unlock ( const Array & arr, Context * context, LineInfoArg * at );
    void builtin_array_clear_lock ( const Array & arr, Context * );
    void builtin_temp_array ( void * data, int size, const Block & block, Context * context, LineInfoArg * lineinfo );
    void builtin_make_temp_array ( Array & arr, void * data, int size );
    void builtin_array_free ( Array & dim, int szt, Context * __context__, LineInfoArg * at );
    void builtin_table_free ( Table & tab, int szk, int szv, Context * __context__, LineInfoArg * at );

    void toLog ( int level, const char * text, Context * context, LineInfoArg * at );
    void toCompilerLog ( const char * text, Context * context, LineInfoArg * at );

    vec4f builtin_verify_locks ( Context & context, SimNode_CallBase * node, vec4f * args );
    bool builtin_set_verify_array_locks ( Array & arr, bool value );
    bool builtin_set_verify_table_locks ( Table & tab, bool value );
    bool builtin_set_verify_context ( bool slc, Context * context );

    bool builtin_iterator_first ( const Sequence & it, void * data, Context * context, LineInfoArg * at );
    bool builtin_iterator_next  ( const Sequence & it, void * data, Context * context, LineInfoArg * at );
    void builtin_iterator_close ( const Sequence & it, void * data, Context * context );
    bool builtin_iterator_iterate ( const Sequence & it, void * data, Context * context );
    void builtin_iterator_delete ( const Sequence & it, Context * context );
    __forceinline bool builtin_iterator_empty ( const Sequence & seq ) { return seq.iter==nullptr; }

    void builtin_make_good_array_iterator ( Sequence & result, const Array & arr, int stride, Context * context );
    void builtin_make_fixed_array_iterator ( Sequence & result, void * data, int size, int stride, Context * context );
    void builtin_make_range_iterator ( Sequence & result, range rng, Context * context );
    void builtin_make_lambda_iterator ( Sequence & result, const Lambda lambda, int stride, Context * context );
    void builtin_make_nil_iterator ( Sequence & result, Context * context );
    vec4f builtin_make_enum_iterator ( Context & context, SimNode_CallBase * call, vec4f * );
    void builtin_make_string_iterator ( Sequence & result, char * str, Context * context );

    void resetProfiler( Context * context );
    void dumpProfileInfo( Context * context );
    char * collectProfileInfo( Context * context );

    template <typename TT>
    __forceinline void builtin_sort ( TT * data, int32_t length ) {
        if ( length>1 ) sort ( data, data + length );
    }

    void builtin_sort_string ( void * data, int32_t length );
    void builtin_sort_any_cblock ( void * anyData, int32_t elementSize, int32_t length, const Block & cmp, Context * context, LineInfoArg * lineinfo );
    void builtin_sort_any_ref_cblock ( void * anyData, int32_t elementSize, int32_t length, const Block & cmp, Context * context, LineInfoArg * lineinfo );

    __forceinline int32_t variant_index(const Variant & v) { return v.index; }
    __forceinline void set_variant_index(Variant & v, int32_t index) { v.index = index; }

    void builtin_smart_ptr_clone_ptr ( smart_ptr_raw<void> & dest, const void * src, Context * context, LineInfoArg * at );
    void builtin_smart_ptr_clone ( smart_ptr_raw<void> & dest, const smart_ptr_raw<void> src, Context * context, LineInfoArg * at );
    uint32_t builtin_smart_ptr_use_count ( const smart_ptr_raw<void> src, Context * context, LineInfoArg * at );
    void builtin_smart_ptr_move_new ( smart_ptr_raw<void> & dest, smart_ptr_raw<void> src, Context * context, LineInfoArg * at );
    void builtin_smart_ptr_move_ptr ( smart_ptr_raw<void> & dest, const void * src, Context * context, LineInfoArg * at );
    void builtin_smart_ptr_move ( smart_ptr_raw<void> & dest, smart_ptr_raw<void> & src, Context * context, LineInfoArg * at );

    __forceinline bool equ_sptr_sptr ( const smart_ptr_raw<void> left, const smart_ptr_raw<void> right ) { return left.get() == right.get(); }
    __forceinline bool nequ_sptr_sptr ( const smart_ptr_raw<void> left, const smart_ptr_raw<void> right ) { return left.get() != right.get(); }
    __forceinline bool equ_sptr_ptr ( const smart_ptr_raw<void> left, const void * right ) { return left.get() == right; }
    __forceinline bool nequ_sptr_ptr ( const smart_ptr_raw<void> left, const void * right ) { return left.get() != right; }
    __forceinline bool equ_ptr_sptr ( const void * left, const smart_ptr_raw<void> right ) { return left == right.get(); }
    __forceinline bool nequ_ptr_sptr ( const void * left, const smart_ptr_raw<void> right ) { return left != right.get(); }

    void gc0_save_ptr ( char * name, void * data, Context * context, LineInfoArg * line );
    void gc0_save_smart_ptr ( char * name, smart_ptr_raw<void> data, Context * context, LineInfoArg * line );
    void * gc0_restore_ptr ( char * name, Context * context );
    smart_ptr_raw<void> gc0_restore_smart_ptr ( char * name, Context * context );
    void gc0_reset();

    __forceinline void array_grow ( Context & context, Array & arr, uint32_t stride, LineInfo * at ) {
        if ( arr.isLocked() ) context.throw_error_at(at, "can't resize locked array");
        uint32_t newSize = arr.size + 1;
        if ( newSize > arr.capacity ) {
            uint32_t newCapacity = 1 << (32 - das_clz (das::max(newSize,2u) - 1));
            newCapacity = das::max(newCapacity, 16u);
            array_reserve(context, arr, newCapacity, stride, at);
        }
        arr.size = newSize;
    }

    __forceinline int builtin_array_push ( Array & pArray, int index, int stride, Context * context, LineInfoArg * at ) {
        uint32_t idx = pArray.size;
        array_grow(*context, pArray, stride, at);
        if ( uint32_t(index) >= pArray.size ) context->throw_error_at(at, "insert index out of range, %u of %u", uint32_t(index), pArray.size);
        memmove ( pArray.data+(index+1)*stride, pArray.data+index*stride, size_t(idx-index)*size_t(stride) );
        return index;
    }

    __forceinline int builtin_array_push_zero ( Array & pArray, int index, int stride, Context * context, LineInfoArg * at ) {
        uint32_t idx = pArray.size;
        array_grow(*context, pArray, stride, at);
        if ( uint32_t(index) >= pArray.size ) context->throw_error_at(at, "insert index out of range, %u of %u", uint32_t(index), pArray.size);
        memmove ( pArray.data+(index+1)*stride, pArray.data+index*stride, size_t(idx-index)*size_t(stride) );
        memset ( pArray.data + index*stride, 0, stride );
        return index;
    }

    __forceinline int builtin_array_push_back ( Array & pArray, int stride, Context * context, LineInfoArg * at ) {
        uint32_t idx = pArray.size;
        array_grow(*context, pArray, stride, at);
        return idx;
    }

    __forceinline int builtin_array_push_back_zero ( Array & pArray, int stride, Context * context, LineInfoArg * at ) {
        uint32_t idx = pArray.size;
        array_grow(*context, pArray, stride, at);
        memset(pArray.data + idx*stride, 0, stride);
        return idx;
    }

    __forceinline void concept_assert ( bool, const char * ) {}
    __forceinline void das_static_assert ( bool, const char * ) {}

    LineInfo getCurrentLineInfo( LineInfoArg * lineInfo );
    LineInfo rtti_get_line_info ( int depth, Context * context, LineInfoArg * at );

    void builtin_main_loop ( const TBlock<bool> & block, Context * context, LineInfoArg * at );
}
