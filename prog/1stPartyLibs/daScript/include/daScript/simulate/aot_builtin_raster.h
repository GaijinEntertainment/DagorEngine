#pragma once

namespace das {

    void rast_hspan_u8 ( TArray<uint8_t> & Span, int32_t spanOffset, const TArray<uint8_t> & Tspan, int32_t tspanOffset,
        float uvY, float dUVY, int32_t _count, LineInfoArg * at, Context * context );
    void rast_hspan_masked_u8 ( TArray<uint8_t> & Span, int32_t spanOffset, const TArray<uint8_t> & Tspan, int32_t tspanOffset,
        float uvY, float dUVY, int32_t _count, LineInfoArg * at, Context * context );
    void rast_hspan_masked_solid_u8 ( uint8_t solid, TArray<uint8_t> & Span, int32_t spanOffset, const TArray<uint8_t> & Tspan, int32_t tspanOffset,
        float uvY, float dUVY, int32_t _count, LineInfoArg * at, Context * context );

    __forceinline vec4f v_gather ( const void * _ptr, vec4f index ) {
        // read 4 floats from memory, using 4 uint32_t indices
        #if defined(__AVX2__)
            // note: this is not faster than the scalar version
            return (_mm_i32gather_ps((const float *) _ptr, v_cast_vec4i(index), 4));
        #else
            auto ptr = (const int32_t *) _ptr;
            auto i = v_cast_vec4i(index);
            return v_cast_vec4f(v_make_vec4i(
                ptr[uint32_t(v_extract_xi(i))],
                ptr[uint32_t(v_extract_yi(i))],
                ptr[uint32_t(v_extract_zi(i))],
                ptr[uint32_t(v_extract_wi(i))]
            ));
        #endif
    }

    __forceinline void v_scatter ( void * _ptr, vec4f index, vec4f _value ) {
        // write 4 floats to memory, using 4 uint32_t indices
        vec4i value = v_cast_vec4i(_value);
        auto ptr = (int32_t *) _ptr;
        auto i = v_cast_vec4i(index);
        ptr[uint32_t(v_extract_xi(i))] = v_extract_xi(value);
        ptr[uint32_t(v_extract_yi(i))] = v_extract_yi(value);
        ptr[uint32_t(v_extract_zi(i))] = v_extract_zi(value);
        ptr[uint32_t(v_extract_wi(i))] = v_extract_wi(value);
    }

    __forceinline void v_scatter_mask ( void * _ptr, vec4f index, vec4f _value, vec4f mask_v ) {
        // write 4 floats to memory, using 4 uint32_t indices, but only for floats, where value[i]!=mask_v[i]
        vec4i value = v_cast_vec4i(_value);
        auto ptr = (int32_t *) _ptr;
        auto mask = v_cmp_eqi(v_cast_vec4i(mask_v), value);
        auto i = v_cast_vec4i(index);
        if ( !v_extract_xi(mask) ) ptr[uint32_t(v_extract_xi(i))] = v_extract_xi(value);
        if ( !v_extract_yi(mask) ) ptr[uint32_t(v_extract_yi(i))] = v_extract_yi(value);
        if ( !v_extract_zi(mask) ) ptr[uint32_t(v_extract_zi(i))] = v_extract_zi(value);
        if ( !v_extract_wi(mask) ) ptr[uint32_t(v_extract_wi(i))] = v_extract_wi(value);
    }

    __forceinline void v_store_mask ( void * _ptr, vec4f value, vec4f mask_v ) {
        // write 4 floats to memory, using 4 uint32_t indices, but only for floats, where value[i]!=mask_v[i]
        auto ptr = (float *) _ptr;
        auto mask = v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(mask_v), v_cast_vec4i(value)));
        v_stu(ptr, v_or(v_and(mask,v_ldu(ptr)),v_andnot(mask, value)));
    }

    __forceinline void v_gather_scatter ( void * _to, vec4f to_index, const void * _from, vec4f from_index ) {
        // read 4 floats from memory, using 4 uint32_t indices and then write them to memory
        auto ptr = (uint32_t *) _from;
        auto to =  (uint32_t *) _to;
        auto i = v_cast_vec4i(from_index);
        auto j = v_cast_vec4i(to_index);
        to[uint32_t(v_extract_xi(j))] = ptr[uint32_t(v_extract_xi(i))];
        to[uint32_t(v_extract_yi(j))] = ptr[uint32_t(v_extract_yi(i))];
        to[uint32_t(v_extract_zi(j))] = ptr[uint32_t(v_extract_zi(i))];
        to[uint32_t(v_extract_wi(j))] = ptr[uint32_t(v_extract_wi(i))];
    }

    __forceinline void v_gather_scatter_mask ( void * _to, vec4f to_index, const void * _from, vec4f from_index, vec4f mask_v ) {
        // read 4 floats from memory, using 4 uint32_t indices and then write them to memory, but only for floats, where value[i]!=mask_v[i]
        auto ptr = (uint32_t *) _from;
        auto to =  (uint32_t *) _to;
        auto i = v_cast_vec4i(from_index);
        auto j = v_cast_vec4i(to_index);
        auto m = v_cast_vec4i(mask_v);
        auto X = ptr[uint32_t(v_extract_xi(i))];    if ( X != uint32_t(v_extract_xi(m)) ) to[uint32_t(v_extract_xi(j))] = X;
        auto Y = ptr[uint32_t(v_extract_yi(i))];    if ( Y != uint32_t(v_extract_yi(m)) ) to[uint32_t(v_extract_yi(j))] = Y;
        auto Z = ptr[uint32_t(v_extract_zi(i))];    if ( Z != uint32_t(v_extract_zi(m)) ) to[uint32_t(v_extract_zi(j))] = Z;
        auto W = ptr[uint32_t(v_extract_wi(i))];    if ( W != uint32_t(v_extract_wi(m)) ) to[uint32_t(v_extract_wi(j))] = W;
    }

    __forceinline vec4f v_gather_store_mask ( void * _to, const void * _from, vec4f from_index, vec4f mask_v ) {
        // read 4 floats from memory, using 4 uint32_t indices and then write them to memory, but only for floats, where value[i]!=mask_v[i]
        auto from = (const float *) _from;
        auto to =  (float *) _to;
        auto value = v_gather(from, from_index);
        v_store_mask ( to, value, mask_v );
        return value;
    }

    __forceinline void v_gather_store_stride ( void * _to, int stride, const void * _from, vec4f from_index ) {
        // read 4 floats from memory, using 4 uint32_t indices and then write them to memory, but only for floats, where value[i]!=mask_v[i]
        auto ptr = (uint32_t *) _from;
        auto to =  (uint32_t *) _to;
        auto i = v_cast_vec4i(from_index);
        to[       0] = ptr[uint32_t(v_extract_xi(i))];
        to[  stride] = ptr[uint32_t(v_extract_yi(i))];
        to[2*stride] = ptr[uint32_t(v_extract_zi(i))];
        to[3*stride] = ptr[uint32_t(v_extract_wi(i))];
    }

    __forceinline void u8x4_gather_store ( void * _to, const void * _from, vec4f from_index ) {
        // read 4 floats from memory, using 4 uint32_t indices and then write them to memory, but only for floats, where value[i]!=mask_v[i]
        auto from = (uint8_t *) _from;
        auto i = v_cast_vec4i(from_index);
        auto b0 = from[uint32_t(v_extract_xi(i))];
        auto b1 = from[uint32_t(v_extract_yi(i))];
        auto b2 = from[uint32_t(v_extract_zi(i))];
        auto b3 = from[uint32_t(v_extract_wi(i))];
    #if 1
        auto to = (uint32_t *) _to;
        *to = uint32_t(b0) + (uint32_t(b1) << 8u) + (uint32_t(b2) << 16u) + (uint32_t(b3) << 24u);
    #else
        auto to = (uint8_t *) _to;
        to[0] = b0; to[1] = b1; to[2] = b2; to[3] = b3;
    #endif
    }
}
