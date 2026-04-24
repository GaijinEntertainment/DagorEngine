#pragma once

namespace das {
    DAS_API bool das_is_dll_build();
    DAS_API bool is_in_aot();
    DAS_API void set_aot();
    DAS_API void reset_aot();
    DAS_API bool is_in_completion();
    DAS_API bool is_folding();
    DAS_API const char * compiling_file_name ( );
    DAS_API const char * compiling_module_name ( );
    DAS_API const char * get_module_file_name ( const char * name, Context * context );
    DAS_API void setCommandLineArguments ( int argc, char * argv[] );
    DAS_API void getCommandLineArguments( Array & arr );

    DAS_API bool is_compiling ( );
    DAS_API bool is_compiling_macros ( );
    DAS_API uint64_t get_context_share_counter ( Context * context );

    DAS_API char * builtin_das_root ( Context * context, LineInfoArg * at );
    DAS_API char * builtin_get_das_version ( Context * context, LineInfoArg * at );
    DAS_API void builtin_throw ( char * text, Context * context, LineInfoArg * at );
    DAS_API void builtin_print ( char * text, Context * context, LineInfoArg * at );
    DAS_API void builtin_error ( char * text, Context * context, LineInfoArg * at );
    DAS_API void builtin_print ( char * text, Context * context, LineInfoArg * at );
    DAS_API void builtin_feint ( char * text, Context * context, LineInfoArg * at );
    DAS_API vec4f builtin_sprint ( Context & context, SimNode_CallBase * call, vec4f * args );
    DAS_API vec4f builtin_json_sprint ( Context & context, SimNode_CallBase * call, vec4f * args );
    DAS_API vec4f builtin_json_sscan ( Context & context, SimNode_CallBase * call, vec4f * args );
    DAS_API char * builtin_print_data ( const void * data, const TypeInfo * typeInfo, Bitfield flags, Context * context, LineInfoArg * at );
    DAS_API char * builtin_print_data_v ( float4 data, const TypeInfo * typeInfo, Bitfield flags, Context * context, LineInfoArg * at );
    DAS_API char * builtin_debug_type ( const TypeInfo * typeInfo, Context * context, LineInfoArg * at );
    DAS_API char * builtin_debug_line ( const LineInfo & at, bool fully, Context * context, LineInfoArg * lineInfo );
    DAS_API char * builtin_get_typeinfo_mangled_name ( const TypeInfo * typeInfo, Context * context, LineInfoArg * at );
    DAS_API const FuncInfo * builtin_get_function_info_by_mnh ( Context & context, Func fun );
    DAS_API Func builtin_SimFunction_by_MNH ( Context & context, uint64_t MNH );
    DAS_API vec4f builtin_breakpoint ( Context & context, SimNode_CallBase * call, vec4f * );
    DAS_API void builtin_stackwalk ( bool args, bool vars, Context * context, LineInfoArg * lineInfo );
    DAS_API void builtin_terminate ( Context * context, LineInfoArg * lineInfo );
    DAS_API int builtin_table_size ( const Table & arr );
    DAS_API int builtin_table_capacity ( const Table & arr );
    DAS_API void builtin_table_clear ( Table & arr, Context * context, LineInfoArg * at );
    DAS_API vec4f builtin_table_reserve ( Context & context, SimNode_CallBase * call, vec4f * args );
    DAS_API void heap_stats ( Context & context, uint64_t * bytes );
    DAS_API urange64 heap_allocation_stats ( Context * context );
    DAS_API uint64_t heap_allocation_count ( Context * context );
    DAS_API urange64 string_heap_allocation_stats ( Context * context );
    DAS_API uint64_t string_heap_allocation_count ( Context * context );
    DAS_API uint64_t heap_bytes_allocated ( Context * context );
    DAS_API int32_t heap_depth ( Context * context );
    DAS_API uint64_t string_heap_bytes_allocated ( Context * context );
    DAS_API int32_t string_heap_depth ( Context * context );
    DAS_API void string_heap_report ( Context * context, LineInfoArg * info );
    DAS_API bool is_intern_strings ( Context * context );
    DAS_API void heap_collect ( bool stringHeap, bool validate, Context * context, LineInfoArg * info );
    DAS_API void heap_report ( Context * context, LineInfoArg * info );
    DAS_API void memory_report ( bool errorsOnly, Context * context, LineInfoArg * info );
    DAS_API void builtin_table_lock_mutable ( const Table & arr, Context * context, LineInfoArg * at );
    DAS_API void builtin_table_unlock_mutable ( const Table & arr, Context * context, LineInfoArg * at );
    DAS_API void builtin_table_lock ( Table & arr, Context * context, LineInfoArg * at );
    DAS_API void builtin_table_unlock ( Table & arr, Context * context, LineInfoArg * at );
    DAS_API void builtin_table_clear_lock ( const Table & arr, Context * context );
    DAS_API int builtin_array_size ( const Array & arr );
    DAS_API int builtin_array_capacity ( const Array & arr );
    DAS_API int builtin_array_lock_count ( const Array & arr );
    DAS_API void builtin_array_resize ( Array & pArray, int newSize, int stride, Context * context, LineInfoArg * at );
    DAS_API void builtin_array_resize_no_init ( Array & pArray, int newSize, int stride, Context * context, LineInfoArg * at );
    DAS_API void builtin_array_reserve ( Array & pArray, int newSize, int stride, Context * context, LineInfoArg * at );
    DAS_API void builtin_array_erase ( Array & pArray, int index, int stride, Context * context, LineInfoArg * at );
    DAS_API void builtin_array_erase_range ( Array & pArray, int index, int count, int stride, Context * context, LineInfoArg * at );
    DAS_API void builtin_array_clear ( Array & pArray, Context * context, LineInfoArg * at );
    DAS_API void builtin_array_lock_mutable ( const Array & arr, Context * context, LineInfoArg * at );
    DAS_API void builtin_array_unlock_mutable ( const Array & arr, Context * context, LineInfoArg * at );
    DAS_API void builtin_array_lock ( Array & arr, Context * context, LineInfoArg * at );
    DAS_API void builtin_array_unlock ( Array & arr, Context * context, LineInfoArg * at );
    DAS_API void builtin_array_clear_lock ( const Array & arr, Context * );
    DAS_API void builtin_temp_array ( void * data, int size, const Block & block, Context * context, LineInfoArg * lineinfo );
    DAS_API void builtin_make_temp_array ( Array & arr, void * data, int size );
    DAS_API void builtin_array_free ( Array & dim, int szt, Context * __context__, LineInfoArg * at );
    DAS_API void builtin_table_free ( Table & tab, int szk, int szv, Context * __context__, LineInfoArg * at );
    DAS_API vec4f builtin_collect_local_and_zero ( Context & context, SimNode_CallBase * call, vec4f * args );

    DAS_API void toLog ( int level, const char * text, Context * context, LineInfoArg * at );
    void toCompilerLog ( const char * text, Context * context, LineInfoArg * at );

    DAS_API bool builtin_iterator_first ( Sequence & it, void * data, Context * context, LineInfoArg * at );
    DAS_API bool builtin_iterator_next  ( Sequence & it, void * data, Context * context, LineInfoArg * at );
    DAS_API void builtin_iterator_close ( Sequence & it, void * data, Context * context );
    DAS_API bool builtin_iterator_iterate ( Sequence & it, void * data, Context * context );
    DAS_API void builtin_iterator_delete ( Sequence & it, Context * context );
    DAS_API __forceinline bool builtin_iterator_empty ( const Sequence & seq ) { return seq.iter==nullptr; }

    DAS_API void builtin_make_good_array_iterator ( Sequence & result, const Array & arr, int stride, Context * context, LineInfoArg * at );
    DAS_API void builtin_make_fixed_array_iterator ( Sequence & result, void * data, int size, int stride, Context * context, LineInfoArg * at );
    DAS_API void builtin_make_range_iterator ( Sequence & result, range rng, Context * context, LineInfoArg * at );
    DAS_API void builtin_make_urange_iterator ( Sequence & result, urange rng, Context * context, LineInfoArg * at );
    DAS_API void builtin_make_range64_iterator ( Sequence & result, range64 rng, Context * context, LineInfoArg * at );
    DAS_API void builtin_make_urange64_iterator ( Sequence & result, urange64 rng, Context * context, LineInfoArg * at );
    DAS_API void builtin_make_lambda_iterator ( Sequence & result, const Lambda lambda, int stride, Context * context, LineInfoArg * at );
    DAS_API void builtin_make_nil_iterator ( Sequence & result, Context * context, LineInfoArg * at );
    DAS_API vec4f builtin_make_enum_iterator ( Context & context, SimNode_CallBase * call, vec4f * );
    DAS_API void builtin_make_string_iterator ( Sequence & result, char * str, Context * context, LineInfoArg * at );

    DAS_API void resetProfiler( Context * context );
    DAS_API void dumpProfileInfo( Context * context );
    DAS_API char * collectProfileInfo( Context * context, LineInfoArg * at );

    template <typename TT>
    __forceinline void builtin_sort ( TT * data, int32_t length ) {
        if ( length>1 ) sort ( data, data + length );
    }

    DAS_API void builtin_sort_string ( void * data, int32_t length );
    DAS_API void builtin_sort_any_cblock ( void * anyData, int32_t elementSize, int32_t length, const Block & cmp, Context * context, LineInfoArg * lineinfo );
    DAS_API void builtin_sort_any_ref_cblock ( void * anyData, int32_t elementSize, int32_t length, const Block & cmp, Context * context, LineInfoArg * lineinfo );

    __forceinline int32_t variant_index(const Variant & v) { return v.index; }
    __forceinline void set_variant_index(Variant & v, int32_t index) { v.index = index; }

    DAS_API void builtin_smart_ptr_clone_ptr ( smart_ptr_raw<void> & dest, const void * src, Context * context, LineInfoArg * at );
    DAS_API void builtin_smart_ptr_clone ( smart_ptr_raw<void> & dest, const smart_ptr_raw<void> src, Context * context, LineInfoArg * at );
    DAS_API uint32_t builtin_smart_ptr_use_count ( const smart_ptr_raw<void> src, Context * context, LineInfoArg * at );
    DAS_API void builtin_smart_ptr_move_new ( smart_ptr_raw<void> & dest, smart_ptr_raw<void> src, Context * context, LineInfoArg * at );
    DAS_API void builtin_smart_ptr_move_ptr ( smart_ptr_raw<void> & dest, const void * src, Context * context, LineInfoArg * at );
    DAS_API void builtin_smart_ptr_move ( smart_ptr_raw<void> & dest, smart_ptr_raw<void> & src, Context * context, LineInfoArg * at );

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

    void *builtin_das_aligned_alloc16(uint64_t size);

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

    LineInfo rtti_get_line_info ( int depth, Context * context, LineInfoArg * at );

    void builtin_main_loop ( const TBlock<bool> & block, Context * context, LineInfoArg * at );

    vec4f _builtin_hash ( Context & context, SimNode_CallBase * call, vec4f * args );

    const char * das_get_platform_name();
    const char * das_get_architecture_name();

    DAS_API char * fmt_i8 ( const char * fmt, int8_t value, Context * context, LineInfoArg * at );
    DAS_API char * fmt_u8 ( const char * fmt, uint8_t value, Context * context, LineInfoArg * at );
    DAS_API char * fmt_i16 ( const char * fmt, int16_t value, Context * context, LineInfoArg * at );
    DAS_API char * fmt_u16 ( const char * fmt, uint16_t value, Context * context, LineInfoArg * at );
    DAS_API char * fmt_i32 ( const char * fmt, int32_t value, Context * context, LineInfoArg * at );
    DAS_API char * fmt_u32 ( const char * fmt, uint32_t value, Context * context, LineInfoArg * at );
    DAS_API char * fmt_i64 ( const char * fmt, int64_t value, Context * context, LineInfoArg * at );
    DAS_API char * fmt_u64 ( const char * fmt, uint64_t value, Context * context, LineInfoArg * at );
    DAS_API char * fmt_f ( const char * fmt, float value, Context * context, LineInfoArg * at );
    DAS_API char * fmt_d ( const char * fmt, double value, Context * context, LineInfoArg * at );
}
