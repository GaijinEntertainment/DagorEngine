#include "daScript/misc/platform.h"

#include "daScript/simulate/simulate_nodes.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_policy_types.h"
#include "daScript/ast/ast_handle.h"

#include "daScript/simulate/aot_builtin_raster.h"

namespace das
{
    void rast_hspan_u8 ( TArray<uint8_t> & Span, int32_t spanOffset, const TArray<uint8_t> & Tspan, int32_t tspanOffset, float uvY, float dUVY, int32_t _count, LineInfoArg * at, Context * context ) {
        if ( uint32_t(spanOffset+_count) > Span.size ) {
            context->throw_error_at(at,"rast_hspan: span out of range %i+%i >= %i", spanOffset, _count, Span.size);
        }
        if ( tspanOffset<0 ) {
            context->throw_error_at(at,"rast_hspan: tspan offset is negative");
        }
        uint8_t * pspan = (uint8_t *) Span.data + spanOffset;
        uint8_t * tspan = (uint8_t *) Tspan.data + tspanOffset;
        int32_t count = _count;
        int32_t count4 = count >> 2;
        count &= 3;
        uint8_t * PSP = pspan;
        vec4f uv4 = v_add(v_splats(uvY),v_mul(v_make_vec4f(0.,1.,2.,3.),v_splats(dUVY)));
        vec4f duv4 = v_mul(v_splats(dUVY),v_splats(4.));
        // uint32_t maxi = uint32_t(Tspan.size - tspanOffset);
        vec4i maxi = v_splatsi(uint32_t(Tspan.size - tspanOffset));

        for ( int32_t P=0; P!=count4; ++P ) {
            vec4i iuv4 = v_cvt_vec4i(uv4);
            auto mask = v_signmask(v_cast_vec4f(v_cmp_lti(maxi,iuv4)));
            if ( mask!=0 ) context->throw_error_at(at,"rast_hspan: tspan out of range");
            auto i0 = uint32_t(v_extract_xi(iuv4));
            auto i1 = uint32_t(v_extract_yi(iuv4));
            auto i2 = uint32_t(v_extract_zi(iuv4));
            auto i3 = uint32_t(v_extract_wi(iuv4));
            auto b0 = tspan[i0];
            auto b1 = tspan[i1];
            auto b2 = tspan[i2];
            auto b3 = tspan[i3];
            *((uint32_t *)PSP) = uint32_t(b0) + (uint32_t(b1) << 8u) + (uint32_t(b2) << 16u) + (uint32_t(b3) << 24u);
            PSP += 4;
            uv4 = v_add(uv4,duv4);
        }
        if ( count ) {
            vec4i iuv4 = v_cvt_vec4i(uv4);
            auto mask = v_signmask(v_cast_vec4f(v_cmp_lti(maxi,iuv4)));
            mask &= ((1<<count)-1);
            if ( mask!=0 ) context->throw_error_at(at,"rast_hspan: tspan out of range");
            auto i0 = uint32_t(v_extract_xi(iuv4));
            auto b0 = tspan[i0];
            *PSP++ = b0;
            if ( count > 1 ) {
                auto i1 = uint32_t(v_extract_yi(iuv4));
                auto b1 = tspan[i1];
                *PSP++ = b1;
                if ( count > 2 ) {
                    auto i2 = uint32_t(v_extract_zi(iuv4));
                    auto b2 = tspan[i2];
                    *PSP++ = b2;
                }
            }
        }
    }

    void rast_hspan_masked_u8 ( TArray<uint8_t> & Span, int32_t spanOffset, const TArray<uint8_t> & Tspan, int32_t tspanOffset, float uvY, float dUVY, int32_t _count, LineInfoArg * at, Context * context ) {
        if ( uint32_t(spanOffset+_count) > Span.size ) {
            context->throw_error_at(at,"rast_hspan: span out of range %i+%i >= %i", spanOffset, _count, Span.size);
        }
        if ( tspanOffset<0 ) {
            context->throw_error_at(at,"rast_hspan: tspan offset is negative");
        }
        uint8_t * pspan = (uint8_t *) Span.data + spanOffset;
        uint8_t * tspan = (uint8_t *) Tspan.data + tspanOffset;
        int32_t count = _count;
        int32_t count4 = count >> 2;
        count &= 3;
        uint8_t * PSP = pspan;
        vec4f uv4 = v_add(v_splats(uvY),v_mul(v_make_vec4f(0.,1.,2.,3.),v_splats(dUVY)));
        vec4f duv4 = v_mul(v_splats(dUVY),v_splats(4.));
        // uint32_t maxi = uint32_t(Tspan.size - tspanOffset);
        vec4i maxi = v_splatsi(uint32_t(Tspan.size - tspanOffset));

        for ( int32_t P=0; P!=count4; ++P ) {
            vec4i iuv4 = v_cvt_vec4i(uv4);
            auto mask = v_signmask(v_cast_vec4f(v_cmp_lti(maxi,iuv4)));
            if ( mask!=0 ) context->throw_error_at(at,"rast_hspan: tspan out of range");
            auto i0 = uint32_t(v_extract_xi(iuv4));
            auto i1 = uint32_t(v_extract_yi(iuv4));
            auto i2 = uint32_t(v_extract_zi(iuv4));
            auto i3 = uint32_t(v_extract_wi(iuv4));
            if ( auto b0 = tspan[i0] ) PSP[0] = b0;
            if ( auto b1 = tspan[i1] ) PSP[1] = b1;
            if ( auto b2 = tspan[i2] ) PSP[2] = b2;
            if ( auto b3 = tspan[i3] ) PSP[3] = b3;
            PSP += 4;
            uv4 = v_add(uv4,duv4);
        }
        if ( count ) {
            vec4i iuv4 = v_cvt_vec4i(uv4);
            auto mask = v_signmask(v_cast_vec4f(v_cmp_lti(maxi,iuv4)));
            mask &= ((1<<count)-1);
            if ( mask!=0 ) context->throw_error_at(at,"rast_hspan: tspan out of range");
            auto i0 = uint32_t(v_extract_xi(iuv4));
            if ( auto b0 = tspan[i0] ) PSP[0] = b0;
            if ( count > 1 ) {
                auto i1 = uint32_t(v_extract_yi(iuv4));
                if ( auto b1 = tspan[i1] ) PSP[1] = b1;
                if ( count > 2 ) {
                    auto i2 = uint32_t(v_extract_zi(iuv4));
                    if ( auto b2 = tspan[i2] ) PSP[2] = b2;
                }
            }
        }
    }

    void rast_hspan_masked_solid_u8 ( uint8_t solid, TArray<uint8_t> & Span, int32_t spanOffset, const TArray<uint8_t> & Tspan, int32_t tspanOffset, float uvY, float dUVY, int32_t _count, LineInfoArg * at, Context * context ) {
        if ( uint32_t(spanOffset+_count) > Span.size ) {
            context->throw_error_at(at,"rast_hspan: span out of range %i+%i >= %i", spanOffset, _count, Span.size);
        }
        if ( tspanOffset<0 ) {
            context->throw_error_at(at,"rast_hspan: tspan offset is negative");
        }
        uint8_t * pspan = (uint8_t *) Span.data + spanOffset;
        uint8_t * tspan = (uint8_t *) Tspan.data + tspanOffset;
        int32_t count = _count;
        int32_t count4 = count >> 2;
        count &= 3;
        uint8_t * PSP = pspan;
        vec4f uv4 = v_add(v_splats(uvY),v_mul(v_make_vec4f(0.,1.,2.,3.),v_splats(dUVY)));
        vec4f duv4 = v_mul(v_splats(dUVY),v_splats(4.));
        // uint32_t maxi = uint32_t(Tspan.size - tspanOffset);
        vec4i maxi = v_splatsi(uint32_t(Tspan.size - tspanOffset));

        for ( int32_t P=0; P!=count4; ++P ) {
            vec4i iuv4 = v_cvt_vec4i(uv4);
            auto mask = v_signmask(v_cast_vec4f(v_cmp_lti(maxi,iuv4)));
            if ( mask!=0 ) context->throw_error_at(at,"rast_hspan: tspan out of range");
            auto i0 = uint32_t(v_extract_xi(iuv4));
            auto i1 = uint32_t(v_extract_yi(iuv4));
            auto i2 = uint32_t(v_extract_zi(iuv4));
            auto i3 = uint32_t(v_extract_wi(iuv4));
            if ( auto b0 = tspan[i0] ) PSP[0] = solid;
            if ( auto b1 = tspan[i1] ) PSP[1] = solid;
            if ( auto b2 = tspan[i2] ) PSP[2] = solid;
            if ( auto b3 = tspan[i3] ) PSP[3] = solid;
            PSP += 4;
            uv4 = v_add(uv4,duv4);
        }
        if ( count ) {
            vec4i iuv4 = v_cvt_vec4i(uv4);
            auto mask = v_signmask(v_cast_vec4f(v_cmp_lti(maxi,iuv4)));
            mask &= ((1<<count)-1);
            if ( mask!=0 ) context->throw_error_at(at,"rast_hspan: tspan out of range");
            auto i0 = uint32_t(v_extract_xi(iuv4));
            if ( auto b0 = tspan[i0] ) PSP[0] = solid;
            if ( count > 1 ) {
                auto i1 = uint32_t(v_extract_yi(iuv4));
                if ( auto b1 = tspan[i1] ) PSP[1] = solid;
                if ( count > 2 ) {
                    auto i2 = uint32_t(v_extract_zi(iuv4));
                    if ( auto b2 = tspan[i2] ) PSP[2] = solid;
                }
            }
        }
    }

    class Module_Raster : public Module {
    public:
        Module_Raster() : Module("raster") {
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            // gather
            addExternEx<float4(const float *,uint4),DAS_BIND_FUN(v_gather)>(*this, lib, "gather",
                SideEffects::none,"v_gather")->args({"from","index4"})->unsafeOperation = true;
            addExternEx<int4(const int32_t *,uint4),DAS_BIND_FUN(v_gather)>(*this, lib, "gather",
                SideEffects::none,"v_gather")->args({"from","index4"})->unsafeOperation = true;
            addExternEx<uint4(const uint32_t *,uint4),DAS_BIND_FUN(v_gather)>(*this, lib, "gather",
                SideEffects::none,"v_gather")->args({"from","index4"})->unsafeOperation = true;

            addExternEx<float4(const float *,int4),DAS_BIND_FUN(v_gather)>(*this, lib, "gather",
                SideEffects::none,"v_gather")->args({"from","index4"})->unsafeOperation = true;
            addExternEx<int4(const int32_t *,int4),DAS_BIND_FUN(v_gather)>(*this, lib, "gather",
                SideEffects::none,"v_gather")->args({"from","index4"})->unsafeOperation = true;
            addExternEx<uint4(const uint32_t *,int4),DAS_BIND_FUN(v_gather)>(*this, lib, "gather",
                SideEffects::none,"v_gather")->args({"from","index4"})->unsafeOperation = true;
            // scatter
            addExternEx<void(float *,uint4,float4),DAS_BIND_FUN(v_scatter)>(*this, lib, "scatter",
                SideEffects::modifyArgument,"v_scatter")->args({"to","index4","from"})->unsafeOperation = true;
            addExternEx<void(int32_t *,uint4,int4),DAS_BIND_FUN(v_scatter)>(*this, lib, "scatter",
                SideEffects::modifyArgument,"v_scatter")->args({"to","index4","from"})->unsafeOperation = true;
            addExternEx<void(uint32_t *,uint4,uint4),DAS_BIND_FUN(v_scatter)>(*this, lib, "scatter",
                SideEffects::modifyArgument,"v_scatter")->args({"to","index4","from"})->unsafeOperation = true;

            addExternEx<void(float *,int4,float4),DAS_BIND_FUN(v_scatter)>(*this, lib, "scatter",
                SideEffects::modifyArgument,"v_scatter")->args({"to","index4","from"})->unsafeOperation = true;
            addExternEx<void(int32_t *,int4,int4),DAS_BIND_FUN(v_scatter)>(*this, lib, "scatter",
                SideEffects::modifyArgument,"v_scatter")->args({"to","index4","from"})->unsafeOperation = true;
            addExternEx<void(uint32_t *,int4,uint4),DAS_BIND_FUN(v_scatter)>(*this, lib, "scatter",
                SideEffects::modifyArgument,"v_scatter")->args({"to","index4","from"})->unsafeOperation = true;
            // scatter mask
            addExternEx<void(float *,uint4,float4,float4),DAS_BIND_FUN(v_scatter_mask)>(*this, lib, "scatter_neq_mask",
                SideEffects::modifyArgument,"v_scatter_mask")->args({"to","index4","from","mask"})->unsafeOperation = true;
            addExternEx<void(int32_t *,uint4,int4,int4),DAS_BIND_FUN(v_scatter_mask)>(*this, lib, "scatter_neq_mask",
                SideEffects::modifyArgument,"v_scatter_mask")->args({"to","index4","from","mask"})->unsafeOperation = true;
            addExternEx<void(uint32_t *,uint4,uint4,uint4),DAS_BIND_FUN(v_scatter_mask)>(*this, lib, "scatter_neq_mask",
                SideEffects::modifyArgument,"v_scatter_mask")->args({"to","index4","from","mask"})->unsafeOperation = true;

            addExternEx<void(float *,int4,float4,float4),DAS_BIND_FUN(v_scatter_mask)>(*this, lib, "scatter_neq_mask",
                SideEffects::modifyArgument,"v_scatter_mask")->args({"to","index4","from","mask"})->unsafeOperation = true;
            addExternEx<void(int32_t *,int4,int4,int4),DAS_BIND_FUN(v_scatter_mask)>(*this, lib, "scatter_neq_mask",
                SideEffects::modifyArgument,"v_scatter_mask")->args({"to","index4","from","mask"})->unsafeOperation = true;
            addExternEx<void(uint32_t *,int4,uint4,uint4),DAS_BIND_FUN(v_scatter_mask)>(*this, lib, "scatter_neq_mask",
                SideEffects::modifyArgument,"v_scatter_mask")->args({"to","index4","from","mask"})->unsafeOperation = true;
            // store mask
            addExternEx<void(float4 *,float4,float4),DAS_BIND_FUN(v_store_mask)>(*this, lib, "store_neq_mask",
                SideEffects::modifyArgument,"v_store_mask")->args({"to","from","mask"});
            addExternEx<void(int4 *,int4,int4),DAS_BIND_FUN(v_store_mask)>(*this, lib, "store_neq_mask",
                SideEffects::modifyArgument,"v_store_mask")->args({"to","from","mask"});
            addExternEx<void(uint4 *,uint4,uint4),DAS_BIND_FUN(v_store_mask)>(*this, lib, "store_neq_mask",
                SideEffects::modifyArgument,"v_store_mask")->args({"to","from","mask"});
            // gather-scatter
            addExternEx<void(float *,uint4,const float *,uint4),DAS_BIND_FUN(v_gather_scatter)>(*this, lib, "gather_scatter",
                SideEffects::modifyArgument,"v_gather_scatter")->args({"to","to_index4","from","from_index4"})->unsafeOperation = true;
            addExternEx<void(int32_t *,uint4,const int32_t *,uint4),DAS_BIND_FUN(v_gather_scatter)>(*this, lib, "gather_scatter",
                SideEffects::modifyArgument,"v_gather_scatter")->args({"to","to_index4","from","from_index4"})->unsafeOperation = true;
            addExternEx<void(uint32_t *,uint4,const uint32_t *,uint4),DAS_BIND_FUN(v_gather_scatter)>(*this, lib, "gather_scatter",
                SideEffects::modifyArgument,"v_gather_scatter")->args({"to","to_index4","from","from_index4"})->unsafeOperation = true;

            addExternEx<void(float *,int4,const float *,int4),DAS_BIND_FUN(v_gather_scatter)>(*this, lib, "gather_scatter",
                SideEffects::modifyArgument,"v_gather_scatter")->args({"to","to_index4","from","from_index4"})->unsafeOperation = true;
            addExternEx<void(int32_t *,int4,const int32_t *,int4),DAS_BIND_FUN(v_gather_scatter)>(*this, lib, "gather_scatter",
                SideEffects::modifyArgument,"v_gather_scatter")->args({"to","to_index4","from","from_index4"})->unsafeOperation = true;
            addExternEx<void(uint32_t *,int4,const uint32_t *,int4),DAS_BIND_FUN(v_gather_scatter)>(*this, lib, "gather_scatter",
                SideEffects::modifyArgument,"v_gather_scatter")->args({"to","to_index4","from","from_index4"})->unsafeOperation = true;
            // gather-scatter mask
            addExternEx<void(float *,uint4,const float *,uint4,float4),DAS_BIND_FUN(v_gather_scatter_mask)>(*this, lib, "gather_scatter_neq_mask",
                SideEffects::modifyArgument,"v_gather_scatter_mask")->args({"to","to_index4","from","from_index4","mask"})->unsafeOperation = true;
            addExternEx<void(int32_t *,uint4,const int32_t *,uint4,int4),DAS_BIND_FUN(v_gather_scatter_mask)>(*this, lib, "gather_scatter_neq_mask",
                SideEffects::modifyArgument,"v_gather_scatter_mask")->args({"to","to_index4","from","from_index4","mask"})->unsafeOperation = true;
            addExternEx<void(uint32_t *,uint4,const uint32_t *,uint4,uint4),DAS_BIND_FUN(v_gather_scatter_mask)>(*this, lib, "gather_scatter_neq_mask",
                SideEffects::modifyArgument,"v_gather_scatter_mask")->args({"to","to_index4","from","from_index4","mask"})->unsafeOperation = true;

            addExternEx<void(float *,int4,const float *,int4,float4),DAS_BIND_FUN(v_gather_scatter_mask)>(*this, lib, "gather_scatter_neq_mask",
                SideEffects::modifyArgument,"v_gather_scatter_mask")->args({"to","to_index4","from","from_index4","mask"})->unsafeOperation = true;
            addExternEx<void(int32_t *,int4,const int32_t *,int4,int4),DAS_BIND_FUN(v_gather_scatter_mask)>(*this, lib, "gather_scatter_neq_mask",
                SideEffects::modifyArgument,"v_gather_scatter_mask")->args({"to","to_index4","from","from_index4","mask"})->unsafeOperation = true;
            addExternEx<void(uint32_t *,int4,const uint32_t *,int4,uint4),DAS_BIND_FUN(v_gather_scatter_mask)>(*this, lib, "gather_scatter_neq_mask",
                SideEffects::modifyArgument,"v_gather_scatter_mask")->args({"to","to_index4","from","from_index4","mask"})->unsafeOperation = true;
            // gather-store-mask
            addExternEx<void(float4 *,const float *,uint4,float4),DAS_BIND_FUN(v_gather_store_mask)>(*this, lib, "gather_store_neq_mask",
                SideEffects::modifyArgument,"v_gather_store_mask")->args({"to","from","from_index4","mask"})->unsafeOperation = true;
            addExternEx<void(int4 *,const int32_t *,uint4,int4),DAS_BIND_FUN(v_gather_store_mask)>(*this, lib, "gather_store_neq_mask",
                SideEffects::modifyArgument,"v_gather_store_mask")->args({"to","from","from_index4","mask"})->unsafeOperation = true;
            addExternEx<void(uint4 *,const uint32_t *,uint4,uint4),DAS_BIND_FUN(v_gather_store_mask)>(*this, lib, "gather_store_neq_mask",
                SideEffects::modifyArgument,"v_gather_store_mask")->args({"to","from","from_index4","mask"})->unsafeOperation = true;

            addExternEx<void(float4 *,const float *,int4,float4),DAS_BIND_FUN(v_gather_store_mask)>(*this, lib, "gather_store_neq_mask",
                SideEffects::modifyArgument,"v_gather_store_mask")->args({"to","from","from_index4","mask"})->unsafeOperation = true;
            addExternEx<void(int4 *,const int32_t *,int4,int4),DAS_BIND_FUN(v_gather_store_mask)>(*this, lib, "gather_store_neq_mask",
                SideEffects::modifyArgument,"v_gather_store_mask")->args({"to","from","from_index4","mask"})->unsafeOperation = true;
            addExternEx<void(uint4 *,const uint32_t *,int4,uint4),DAS_BIND_FUN(v_gather_store_mask)>(*this, lib, "gather_store_neq_mask",
                SideEffects::modifyArgument,"v_gather_store_mask")->args({"to","from","from_index4","mask"})->unsafeOperation = true;            // lets make sure its all aot ready
            verifyAotReady();
            // gather-store-with stride
            addExternEx<void(float *,int32_t,const float *,uint4),DAS_BIND_FUN(v_gather_store_stride)>(*this, lib, "gather_store_stride",
                SideEffects::modifyArgument,"v_gather_store_stride")->args({"to","stride","from","from_index4"})->unsafeOperation = true;
            addExternEx<void(int32_t *,int32_t,const int32_t *,uint4),DAS_BIND_FUN(v_gather_store_stride)>(*this, lib, "gather_store_stride",
                SideEffects::modifyArgument,"v_gather_store_stride")->args({"to","stride","from","from_index4"})->unsafeOperation = true;
            addExternEx<void(uint32_t *,int32_t,const uint32_t *,uint4),DAS_BIND_FUN(v_gather_store_stride)>(*this, lib, "gather_store_stride",
                SideEffects::modifyArgument,"v_gather_store_stride")->args({"to","stride","from","from_index4"})->unsafeOperation = true;

            addExternEx<void(float *,int32_t,const float *,int4),DAS_BIND_FUN(v_gather_store_stride)>(*this, lib, "gather_store_stride",
                SideEffects::modifyArgument,"v_gather_store_stride")->args({"to","stride","from","from_index4"})->unsafeOperation = true;
            addExternEx<void(int32_t *,int32_t,const int32_t *,int4),DAS_BIND_FUN(v_gather_store_stride)>(*this, lib, "gather_store_stride",
                SideEffects::modifyArgument,"v_gather_store_stride")->args({"to","stride","from","from_index4"})->unsafeOperation = true;
            addExternEx<void(uint32_t *,int32_t,const uint32_t *,int4),DAS_BIND_FUN(v_gather_store_stride)>(*this, lib, "gather_store_stride",
                SideEffects::modifyArgument,"v_gather_store_stride")->args({"to","stride","from","from_index4"})->unsafeOperation = true;            // lets make sure its all aot ready
            // gather-store u8x4
            addExternEx<void(uint8_t *,const uint8_t *,uint4),DAS_BIND_FUN(u8x4_gather_store)>(*this, lib, "u8x4_gather_store",
                SideEffects::modifyArgument,"u8x4_gather_store")->args({"to","from","from_index4"})->unsafeOperation = true;

            addExternEx<void(uint8_t *,const uint8_t *,int4),DAS_BIND_FUN(u8x4_gather_store)>(*this, lib, "u8x4_gather_store",
                SideEffects::modifyArgument,"u8x4_gather_store")->args({"to","from","from_index4"})->unsafeOperation = true;
            // span rasters
            addExtern<DAS_BIND_FUN(rast_hspan_u8)>(*this, lib, "rast_hspan_u8", SideEffects::modifyArgument,"rast_hspan_u8")
                ->args({"span","spanOffset","tspan","tspanOffset","uvY","dUVY","count","at","context"});
            addExtern<DAS_BIND_FUN(rast_hspan_masked_u8)>(*this, lib, "rast_hspan_masked_u8", SideEffects::modifyArgument,"rast_hspan_masked_u8")
                ->args({"span","spanOffset","tspan","tspanOffset","uvY","dUVY","count","at","context"});
            addExtern<DAS_BIND_FUN(rast_hspan_masked_solid_u8)>(*this, lib, "rast_hspan_masked_solid_u8", SideEffects::modifyArgument,"rast_hspan_masked_solid_u8")
                ->args({"solid","span","spanOffset","tspan","tspanOffset","uvY","dUVY","count","at","context"});
            // lets make sure its all aot ready
            verifyAotReady();
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            tw << "#include \"daScript/simulate/aot_builtin_raster.h\"\n";
            return ModuleAotType::cpp;
        }
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_Raster,das);
