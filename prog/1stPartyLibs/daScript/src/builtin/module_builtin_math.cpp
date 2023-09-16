#include "daScript/misc/platform.h"

#include "module_builtin.h"

#include "daScript/simulate/simulate_nodes.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_policy_types.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/simulate/sim_policy.h"
#include "daScript/simulate/aot_builtin_math.h"
#include "daScript/simulate/simulate_visit_op.h"
#include "daScript/misc/performance_time.h"

namespace das {
#define MATH_FUN_OP1(fun)\
      DEFINE_POLICY(fun);\
      IMPLEMENT_OP1_FUNCTION_POLICY(fun,Float,float);  \
      IMPLEMENT_OP1_EVAL_FUNCTION_POLICY(fun, float2); \
      IMPLEMENT_OP1_EVAL_FUNCTION_POLICY(fun, float3); \
      IMPLEMENT_OP1_EVAL_FUNCTION_POLICY(fun, float4);

#define MATH_FUN_OP1_INT(fun)\
      DEFINE_POLICY(fun);\
      IMPLEMENT_OP1_FUNCTION_POLICY_EX(fun,Int,int32_t,Float,float);\
      IMPLEMENT_OP1_EVAL_FUNCTION_POLICY(fun, float2); \
      IMPLEMENT_OP1_EVAL_FUNCTION_POLICY(fun, float3); \
      IMPLEMENT_OP1_EVAL_FUNCTION_POLICY(fun, float4);

#define MATH_FUN_OP2(fun)\
      DEFINE_POLICY(fun);\
      IMPLEMENT_OP2_FUNCTION_POLICY(fun,Float,float);  \
      IMPLEMENT_OP2_EVAL_FUNCTION_POLICY(fun, float2); \
      IMPLEMENT_OP2_EVAL_FUNCTION_POLICY(fun, float3); \
      IMPLEMENT_OP2_EVAL_FUNCTION_POLICY(fun, float4);

#define MATH_FUN_OP1I(fun)\
      DEFINE_POLICY(fun);\
      IMPLEMENT_OP1_FUNCTION_POLICY(fun,Int,int32_t); \
      IMPLEMENT_OP1_EVAL_FUNCTION_POLICY(fun, int2);  \
      IMPLEMENT_OP1_EVAL_FUNCTION_POLICY(fun, int3);  \
      IMPLEMENT_OP1_EVAL_FUNCTION_POLICY(fun, int4);  \
      IMPLEMENT_OP1_EVAL_FUNCTION_POLICY(fun, uint2); \
      IMPLEMENT_OP1_EVAL_FUNCTION_POLICY(fun, uint3); \
      IMPLEMENT_OP1_EVAL_FUNCTION_POLICY(fun, uint4);

#define MATH_FUN_OP2I(fun)\
      IMPLEMENT_OP2_FUNCTION_POLICY(fun,Int,int32_t); \
      IMPLEMENT_OP2_EVAL_FUNCTION_POLICY(fun, int2);  \
      IMPLEMENT_OP2_EVAL_FUNCTION_POLICY(fun, int3);  \
      IMPLEMENT_OP2_EVAL_FUNCTION_POLICY(fun, int4);  \
      IMPLEMENT_OP2_EVAL_FUNCTION_POLICY(fun, uint2); \
      IMPLEMENT_OP2_EVAL_FUNCTION_POLICY(fun, uint3); \
      IMPLEMENT_OP2_EVAL_FUNCTION_POLICY(fun, uint4);

#define MATH_FUN_OP3I(fun)\
      IMPLEMENT_OP3_FUNCTION_POLICY(fun,Int,int32_t); \
      IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(fun, int2);  \
      IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(fun, int3);  \
      IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(fun, int4);  \
      IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(fun, uint2); \
      IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(fun, uint3); \
      IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(fun, uint4);

#define MATH_FUN_OP3(fun)\
      DEFINE_POLICY(fun);\
      IMPLEMENT_OP3_FUNCTION_POLICY(fun,Float,float);  \
      IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(fun, float2); \
      IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(fun, float3); \
      IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(fun, float4);

#define MATH_FUN_OP1A(fun)                               \
    DEFINE_POLICY(fun);                                  \
    MATH_FUN_OP1(fun);                                   \
    MATH_FUN_OP1I(fun);                                  \
    IMPLEMENT_OP1_FUNCTION_POLICY(fun,UInt,uint32_t);    \
    IMPLEMENT_OP1_FUNCTION_POLICY(fun,Int64,int64_t);    \
    IMPLEMENT_OP1_FUNCTION_POLICY(fun,UInt64,uint64_t);  \
    IMPLEMENT_OP1_FUNCTION_POLICY(fun,Double,double);


#define MATH_FUN_OP2A(fun)                              \
    MATH_FUN_OP2(fun);                                  \
    MATH_FUN_OP2I(fun);                                 \
    IMPLEMENT_OP2_FUNCTION_POLICY(fun,UInt,uint32_t);   \
    IMPLEMENT_OP2_FUNCTION_POLICY(fun,Int64,int64_t);   \
    IMPLEMENT_OP2_FUNCTION_POLICY(fun,UInt64,uint64_t); \
    IMPLEMENT_OP2_FUNCTION_POLICY(fun,Double,double);

#define MATH_FUN_OP3A(fun)                              \
    MATH_FUN_OP3(fun);                                  \
    MATH_FUN_OP3I(fun);                                 \
    IMPLEMENT_OP3_FUNCTION_POLICY(fun,UInt,uint32_t);   \
    IMPLEMENT_OP3_FUNCTION_POLICY(fun,Int64,int64_t);   \
    IMPLEMENT_OP3_FUNCTION_POLICY(fun,UInt64,uint64_t); \
    IMPLEMENT_OP3_FUNCTION_POLICY(fun,Double,double);

    // everything
    MATH_FUN_OP2A(Min)
    MATH_FUN_OP2A(Max)
    MATH_FUN_OP3A(Clamp)
    MATH_FUN_OP1A(Sign)
    MATH_FUN_OP1A(Abs)

    //common
    MATH_FUN_OP1(Floor)
    MATH_FUN_OP1(Ceil)
    MATH_FUN_OP1(Fract)
    MATH_FUN_OP1(Sqrt)
    MATH_FUN_OP1(RSqrt)
    MATH_FUN_OP1(RSqrtEst)
    MATH_FUN_OP1(Sat)
    MATH_FUN_OP3(Lerp)
    IMPLEMENT_OP3_FUNCTION_POLICY(Lerp,Double,double);

    // mad
    MATH_FUN_OP3A(Mad)

    MATH_FUN_OP1_INT(Trunci)
    MATH_FUN_OP1_INT(Floori)
    MATH_FUN_OP1_INT(Ceili)
    MATH_FUN_OP1_INT(Roundi)

    IMPLEMENT_OP1_FUNCTION_POLICY_EX(Trunci,Int,int32_t,Double,double);
    IMPLEMENT_OP1_FUNCTION_POLICY_EX(Floori,Int,int32_t,Double,double);
    IMPLEMENT_OP1_FUNCTION_POLICY_EX(Ceili,Int,int32_t,Double,double);
    IMPLEMENT_OP1_FUNCTION_POLICY_EX(Roundi,Int,int32_t,Double,double);

    //exp
    MATH_FUN_OP1(Exp)
    MATH_FUN_OP1(Log)
    MATH_FUN_OP1(Exp2)
    MATH_FUN_OP1(Log2)
    MATH_FUN_OP2(Pow)
    MATH_FUN_OP1(Rcp)
    MATH_FUN_OP1(RcpEst)

    //trig
    MATH_FUN_OP1(Sin)
    MATH_FUN_OP1(Cos)
    MATH_FUN_OP1(Tan)
    MATH_FUN_OP1(ATan)
    MATH_FUN_OP1(ATan_est)
    MATH_FUN_OP1(ASin)
    MATH_FUN_OP1(ACos)

    MATH_FUN_OP2(ATan2)
    MATH_FUN_OP2(ATan2_est)

    DEFINE_POLICY(MadS)     // vector_a*scalar_b + vector_c
    IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(MadS,float2);
    IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(MadS,float3);
    IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(MadS,float4);
    IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(MadS,int2);
    IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(MadS,int3);
    IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(MadS,int4);
    IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(MadS,uint2);
    IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(MadS,uint3);
    IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(MadS,uint4);

    // trig types
    template <typename TT>
    void addFunctionTrig(Module & mod, const ModuleLibrary & lib) {
        //                                    policy              ret   arg1 arg2     name
        mod.addFunction( make_smart<BuiltInFn<Sim_Sin<TT>,        TT,   TT>        >("sin",       lib, "Sin")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_Cos<TT>,        TT,   TT>        >("cos",       lib, "Cos")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_Tan<TT>,        TT,   TT>        >("tan",       lib, "Tan")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_ATan<TT>,       TT,   TT>        >("atan",      lib, "ATan")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_ATan_est<TT>,   TT,   TT>        >("atan_est",  lib, "ATan_est")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_ASin<TT>,       TT,   TT>        >("asin",      lib, "ASin")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_ACos<TT>,       TT,   TT>        >("acos",      lib, "ACos")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_ATan2<TT>,      TT,   TT,  TT>   >("atan2",     lib, "ATan2")->args({"y","x"}) );
        mod.addFunction( make_smart<BuiltInFn<Sim_ATan2_est<TT>,  TT,   TT,  TT>   >("atan2_est", lib, "ATan2_est")->args({"y","x"}) );
    }

    template <typename TT>
    void addFunctionCommonTyped(Module & mod, const ModuleLibrary & lib) {
        //                                    policy              ret   arg1 arg2   name
        mod.addFunction( make_smart<BuiltInFn<Sim_Min <TT>, TT,   TT,   TT>      >("min",   lib, "Min")->args({"x","y"}) );
        mod.addFunction( make_smart<BuiltInFn<Sim_Max <TT>, TT,   TT,   TT>      >("max",   lib, "Max")->args({"x","y"}) );
        mod.addFunction( make_smart<BuiltInFn<Sim_Clamp<TT>,TT,   TT,   TT,  TT> >("clamp", lib, "Clamp")->args({"t","a","b"}) );
        mod.addFunction( make_smart<BuiltInFn<Sim_Abs<TT>,  TT,   TT>            >("abs",   lib, "Abs")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_Sign<TT>, TT,   TT>            >("sign",  lib, "Sign")->arg("x") );
    }

    template <typename TT>
    void addFunctionCommon(Module & mod, const ModuleLibrary & lib) {
        //                                    policy            ret   arg1     name
        mod.addFunction( make_smart<BuiltInFn<Sim_Floor<TT>,    TT,   TT>   >("floor",       lib, "Floor")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_Ceil<TT>,     TT,   TT>   >("ceil",        lib, "Ceil")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_Fract<TT>,    TT,   TT>   >("fract",       lib, "Fract")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_Sqrt<TT>,     TT,   TT>   >("sqrt",        lib, "Sqrt")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_RSqrt<TT>,    TT,   TT>   >("rsqrt",       lib, "RSqrt")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_RSqrtEst<TT>, TT,   TT>   >("rsqrt_est",   lib, "RSqrtEst")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_Sat<TT>,      TT,   TT>   >("saturate",    lib, "Sat")->arg("x") );
    }
    template <typename Ret, typename TT>
    void addFunctionCommonConversion(Module & mod, const ModuleLibrary & lib) {
        //                                    policy          ret    arg1     name
        mod.addFunction( make_smart<BuiltInFn<Sim_Floori<TT>, Ret,   TT>   >("floori",  lib, "Floori")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_Ceili <TT>, Ret,   TT>   >("ceili",   lib, "Ceili")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_Roundi<TT>, Ret,   TT>   >("roundi",  lib, "Roundi")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_Trunci<TT>, Ret,   TT>   >("trunci",  lib, "Trunci")->arg("x") );
    }

    template <typename TT>
    void addFunctionPow(Module & mod, const ModuleLibrary & lib) {
        //                                    policy           ret   arg1   name
        mod.addFunction( make_smart<BuiltInFn<Sim_Exp<TT>,     TT,   TT> >("exp",      lib, "Exp")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_Log<TT>,     TT,   TT> >("log",      lib, "Log")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_Exp2<TT>,    TT,   TT> >("exp2",     lib, "Exp2")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_Log2<TT>,    TT,   TT> >("log2",     lib, "Log2")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_Rcp<TT>,     TT,   TT> >("rcp",      lib, "Rcp")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_RcpEst<TT>,  TT,   TT> >("rcp_est",  lib, "RcpEst")->arg("x") );
        mod.addFunction( make_smart<BuiltInFn<Sim_Pow<TT>,  TT,   TT,   TT> >("pow",   lib, "Pow")->args({"x","y"}) );
    }

    template <typename TT>
    void addFunctionOp3i(Module & mod, const ModuleLibrary & lib) {
        //                                    policy         ret arg1 arg2 arg3   name
        mod.addFunction( make_smart<BuiltInFn<Sim_Mad<TT>,   TT, TT,  TT,  TT> >("mad",   lib, "Mad")->args({"a","b","c"}) );
    }

    template <typename TT>
    void addFunctionOp3(Module & mod, const ModuleLibrary & lib) {
        addFunctionOp3i<TT>(mod, lib);
        //                                    policy         ret arg1 arg2 arg3   name
        mod.addFunction( make_smart<BuiltInFn<Sim_Lerp<TT>,  TT, TT,  TT,  TT> >("lerp",  lib, "Lerp")->args({"a","b","t"}) );
    }

    template <typename VecT, int RowC>
    class MatrixAnnotation : public ManagedStructureAnnotation<Matrix<VecT,RowC>> {
        enum { ColC = sizeof(VecT) / sizeof(float) };
    public:
        MatrixAnnotation(ModuleLibrary & lib) :
            ManagedStructureAnnotation<Matrix<VecT,RowC>>( "float" + to_string(ColC) + "x" + to_string(RowC), lib,
                 "float" + to_string(ColC) + "x" + to_string(RowC)) {
            const char * fieldNames [] = {"x","y","z","w"};
            for ( int r=0; r!=RowC; ++r ) {
                this->addFieldEx(fieldNames[r], "m[" + to_string(r) + "]", r*ColC*sizeof(float),
                    make_smart<TypeDecl>(TypeDecl::getVectorType(Type::tFloat, ColC)));
            }
        }
        virtual bool isIndexable ( const TypeDeclPtr & decl ) const override {
            return decl->isIndex();
        }
        virtual TypeDeclPtr makeIndexType ( const ExpressionPtr &, const ExpressionPtr & idx ) const override {
            auto decl = idx->type;
            if ( !decl->isIndex() ) return nullptr;
            auto bt = TypeDecl::getVectorType(Type::tFloat, ColC);
            auto pt = make_smart<TypeDecl>(bt);
            pt->ref = true;
            return pt;
        }
        SimNode * trySimulate ( Context & context, const ExpressionPtr & subexpr, const ExpressionPtr & index,
                               const TypeDeclPtr & r2vType, uint32_t ofs ) const {
            if ( index->rtti_isConstant() ) {
                // if its constant index, like a[3]..., we try to let node bellow simulate
                auto idxCE = static_pointer_cast<ExprConst>(index);
                uint32_t idxC = cast<uint32_t>::to(idxCE->value);
                if ( idxC >= RowC ) {
                    context.thisProgram->error("matrix index out of range", "", "",
                        subexpr->at, CompilationError::index_out_of_range);
                    return nullptr;
                }
                uint32_t stride = sizeof(float)*ColC;
                auto tnode = subexpr->trySimulate(context, idxC*stride + ofs, r2vType);
                if ( tnode ) {
                    return tnode;
                }
            }
            return nullptr;
        }
        virtual SimNode * simulateGetAt ( Context & context, const LineInfo & at, const TypeDeclPtr &,
                                         const ExpressionPtr & rv, const ExpressionPtr & idx, uint32_t ofs ) const override {
            if ( auto tnode = trySimulate(context, rv, idx, make_smart<TypeDecl>(Type::none), ofs) ) {
                return tnode;
            } else {
                return context.code->makeNode<SimNode_At>(at,
                                                          rv->simulate(context),
                                                          idx->simulate(context),
                                                          uint32_t(sizeof(float)*ColC), ofs, RowC);
            }
        }
        virtual SimNode * simulateGetAtR2V ( Context & context, const LineInfo & at, const TypeDeclPtr &,
                                            const ExpressionPtr & rv, const ExpressionPtr & idx, uint32_t ofs ) const override {
            Type r2vType = (Type) ToBasicType<VecT>::type;
            if ( auto tnode = trySimulate(context, rv, idx, make_smart<TypeDecl>(r2vType), ofs) ) {
                return tnode;
            } else {
                return context.code->makeValueNode<SimNode_AtR2V>(  r2vType, at,
                                                                    rv->simulate(context),
                                                                    idx->simulate(context),
                                                                    uint32_t(sizeof(float)*ColC), ofs, RowC);
            }
        }
        virtual bool isRawPod() const override {
            return true;
        }
        virtual bool isPod() const override {
            return true;
        }
    };

    typedef MatrixAnnotation<float4,4> float4x4_ann;
    typedef MatrixAnnotation<float3,4> float3x4_ann;
    typedef MatrixAnnotation<float3,3> float3x3_ann;


    template <typename MatT>
    struct SimNode_MatrixCtor : SimNode_CallBase {
        SimNode_MatrixCtor(const LineInfo & at) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(MatrixCtor);
            V_CALL();
            V_END();
        }
        virtual vec4f DAS_EVAL_ABI eval(Context & context) override {
            auto cmres = cmresEval->evalPtr(context);
            memset ( cmres, 0, sizeof(MatT) );
            return cast<void *>::from(cmres);
        }
    };

    template <int r, int c>
    __forceinline void matrix_identity ( float * mat ) {
        for ( int y=0; y!=r; ++y ) {
            for ( int x=0; x!=c; ++x ) {
                *mat++ = x==y ? 1.0f : 0.0f;
            }
        }
    }

    float4x4 float4x4_from_float34 ( const float3x4 & mat ) {
        mat44f res;
        v_mat44_make_from_43ca(res, (const float*)&mat);
        return reinterpret_cast<float4x4&>(res);
    }

    float3x3 float3x3_from_float44 ( const float4x4 & mat ) {
        float3x3 res;
        res.m[0] = vec4f(mat.m[0]);
        res.m[1] = vec4f(mat.m[1]);
        res.m[2] = vec4f(mat.m[2]);
        return res;
    }

    float3x3 float3x3_from_float34 ( const float3x4 & mat ) {
        float3x3 res;
        res.m[0] = vec4f(mat.m[0]);
        res.m[1] = vec4f(mat.m[1]);
        res.m[2] = vec4f(mat.m[2]);
        return res;
    }

    float3x4 float3x4_from_float44 ( const float4x4 & mat ) {
        float3x4 res;
        res.m[0] = vec4f(mat.m[0]);
        res.m[1] = vec4f(mat.m[1]);
        res.m[2] = vec4f(mat.m[2]);
        res.m[3] = vec4f(mat.m[3]);
        return res;
    }

    void float4x4_identity ( float4x4 & mat ) {
        matrix_identity<4,4>((float*)&mat);
    }

    void float3x4_identity ( float3x4 & mat ) {
        matrix_identity<4,3>((float*)&mat);
    }

    void float3x3_identity ( float3x3 & mat ) {
        matrix_identity<3,3>((float*)&mat);
    }

    float3x3 float3x3_neg ( const float3x3 & mat ) {
        float3x3 res;
        res.m[0] = v_neg(mat.m[0]);
        res.m[1] = v_neg(mat.m[1]);
        res.m[2] = v_neg(mat.m[2]);
        return res;
    }

    float3x4 float3x4_neg ( const float3x4 & mat ) {
        float3x4 res;
        res.m[0] = v_neg(mat.m[0]);
        res.m[1] = v_neg(mat.m[1]);
        res.m[2] = v_neg(mat.m[2]);
        res.m[3] = v_neg(mat.m[3]);
        return res;
    }

    float4x4 float4x4_identity_m ( void ) {
        float4x4 mat;
        matrix_identity<4,4>((float*)&mat);
        return mat;
    }

    float3x4 float3x4_identity_m ( void ) {
        float3x4 mat;
        matrix_identity<4,3>((float*)&mat);
        return mat;
    }

    float3x3 float3x3_identity_m ( void ) {
        float3x3 mat;
        matrix_identity<3,3>((float*)&mat);
        return mat;
    }

    float4x4 float4x4_translation(float3 xyz) {
        float4x4 mat;
        matrix_identity<4,4>((float*)&mat);
        mat.m[3].x = xyz.x;
        mat.m[3].y = xyz.y;
        mat.m[3].z = xyz.z;
        return mat;
    }

    float4x4 float4x4_mul(const float4x4 &a, const float4x4 &b) {
        mat44f va,vb,res;
        memcpy(&va,&a,sizeof(float4x4));
        memcpy(&vb,&b,sizeof(float4x4));
        v_mat44_mul(res,va,vb);
        return reinterpret_cast<float4x4&>(res);;
    }

    float3x3 float3x3_mul(const float3x3 &a, const float3x3 &b) {
        float3x3 res;
        mat33f va;  va.col0 = a.m[0]; va.col1 = a.m[1]; va.col2 = a.m[2];
        res.m[0] = v_mat33_mul_vec3(va, b.m[0]);
        res.m[1] = v_mat33_mul_vec3(va, b.m[1]);
        res.m[2] = v_mat33_mul_vec3(va, b.m[2]);
        return res;
    }

    float4x4 float4x4_transpose ( const float4x4 & src ) {
        mat44f res;
        memcpy ( &res, &src, sizeof(float4x4) );
        v_mat44_transpose(res.col0, res.col1, res.col2, res.col3);
        return reinterpret_cast<float4x4&>(res);
    }

    float4x4 float4x4_neg( const float4x4 & src) {
        mat44f res;
        res.col0 = v_neg(src.m[0]);
        res.col1 = v_neg(src.m[1]);
        res.col2 = v_neg(src.m[2]);
        res.col3 = v_neg(src.m[3]);
        return reinterpret_cast<float4x4&>(res);
    }

    float4x4 float4x4_inverse( const float4x4 & src) {
        mat44f mat, invMat;
        memcpy(&mat, &src, sizeof(float4x4));
        v_mat44_inverse(invMat, mat);
        return reinterpret_cast<float4x4&>(invMat);;
    }

    float3x3 float3x3_inverse( const float3x3 & src) {
        mat33f mat, invMat; mat.col0 = src.m[0]; mat.col1 = src.m[1]; mat.col2 = src.m[2];
        memcpy(&mat, &src, sizeof(float3x3));
        v_mat33_inverse(invMat, mat);
        float3x3 res; res.m[0] = invMat.col0; res.m[1] = invMat.col1; res.m[2] = invMat.col2;
        return res;
    }

    float4x4 float4x4_orthonormal_inverse( const float4x4 & src) {
        mat44f mat, invMat;
        memcpy(&mat, &src, sizeof(float4x4));
        v_mat44_orthonormal_inverse43(invMat, mat);
        return reinterpret_cast<float4x4&>(invMat);;
    }

    float4x4 float4x4_persp_forward(float wk, float hk, float zn, float zf) {
        mat44f mat;
        v_mat44_make_persp_forward(mat, wk, hk, zn, zf);
        return reinterpret_cast<float4x4&>(mat);;
    }

    float4x4 float4x4_persp_reverse(float wk, float hk, float zn, float zf) {
        mat44f mat;
        v_mat44_make_persp_reverse(mat, wk, hk, zn, zf);
        return reinterpret_cast<float4x4&>(mat);;
    }

    float4x4 float4x4_look_at(float4 eye, float4 at, float4 up) {
        mat44f mat;
        v_mat44_make_look_at(mat, eye, at, up);
        return reinterpret_cast<float4x4&>(mat);;
    }

    float4x4 float4x4_compose(float4 pos, float4 rot, float4 scale) {
        mat44f mat;
        v_mat44_compose(mat, pos, rot, scale);
        return reinterpret_cast<float4x4&>(mat);;
    }

    void float4x4_decompose(const float4x4 & mat, float3 & pos, float4 & rot, float3 & scale) {
        mat44f gmat;
        memcpy(&gmat, &mat, sizeof(float4x4));
        vec3f gpos;
        quat4f grot;
        vec4f gscale;
        v_mat4_decompose(gmat, gpos, grot, gscale);
        pos = gpos;
        rot = grot;
        scale = gscale;
    }

    float4 quat_from_unit_arc(float3 v0, float3 v1) {
        return v_quat_from_unit_arc(v_ldu(&v0.x), v_ldu(&v1.x));
    }

    float4 quat_from_unit_vec_ang(float3 v, float ang) {
        return v_quat_from_unit_vec_ang(v_ldu(&v.x), v_splats(ang));
    }

    float4 quat_from_euler_vec(float3 v) {
        return v_quat_from_euler(v_ldu(&v.x));
    }

    float4 quat_from_euler(float x, float y, float z) {
        return v_quat_from_euler(v_make_vec4f(x, y, z, 0.f));
    }

    float3 euler_from_un_quat_vec(float4 v) {
        return v_euler_from_un_quat(v);
    }

    float4 un_quat(const float4x4 & m) {
        mat44f vm;
        memcpy(&vm, &m, sizeof(float4x4));
        return v_un_quat_from_mat4(vm);
    }

    float4 quat_mul(float4 q1, float4 q2) {
        return v_quat_mul_quat(q1, q2);
    }

    float3 quat_mul_vec(float4 q, float3 v) {
        return v_quat_mul_vec3(q, v);
    }

    float4 quat_conjugate(float4 q) {
        return v_quat_conjugate(q);
    }

    static void initFloatNxNIndex ( const FunctionPtr & ptr ) {
        ptr->jitOnly = true;
        ptr->arguments[0]->type->explicitConst = true;
    }

    class Module_Math : public Module {
    public:
        Module_Math() : Module("math") {
            DAS_PROFILE_SECTION("Module_Math");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            // constants
            addConstant(*this,"PI",(float)M_PI);
            addConstant(*this,"FLT_EPSILON",FLT_EPSILON);
            addConstant(*this,"DBL_EPSILON",DBL_EPSILON);
            // trigonometry functions
            addFunctionTrig<float>(*this, lib);
            addFunctionTrig<float2>(*this,lib);
            addFunctionTrig<float3>(*this,lib);
            addFunctionTrig<float4>(*this,lib);
            // exp functions
            addFunctionPow<float>(*this, lib);
            addFunctionPow<float2>(*this,lib);
            addFunctionPow<float3>(*this,lib);
            addFunctionPow<float4>(*this,lib);
            // op3
            addFunctionOp3<float >(*this,lib);
            addFunctionOp3<float2>(*this,lib);
            addFunctionOp3<float3>(*this,lib);
            addFunctionOp3<float4>(*this,lib);
            addFunction( make_smart<BuiltInFn<Sim_MadS<float2>,   float2, float2,  float,  float2> >("mad", lib, "MadS")->args({"a","b","c"}) );
            addFunction( make_smart<BuiltInFn<Sim_MadS<float3>,   float3, float3,  float,  float3> >("mad", lib, "MadS")->args({"a","b","c"}) );
            addFunction( make_smart<BuiltInFn<Sim_MadS<float4>,   float4, float4,  float,  float4> >("mad", lib, "MadS")->args({"a","b","c"}) );
            // op3i - int
            addFunctionOp3i<int32_t >(*this,lib);
            addFunctionOp3i<int2>(*this,lib);
            addFunctionOp3i<int3>(*this,lib);
            addFunctionOp3i<int4>(*this,lib);
            addFunction( make_smart<BuiltInFn<Sim_MadS<int2>,   int2, int2,  int,  int2> >("mad", lib, "MadS")->args({"a","b","c"}) );
            addFunction( make_smart<BuiltInFn<Sim_MadS<int3>,   int3, int3,  int,  int3> >("mad", lib, "MadS")->args({"a","b","c"}) );
            addFunction( make_smart<BuiltInFn<Sim_MadS<int4>,   int4, int4,  int,  int4> >("mad", lib, "MadS")->args({"a","b","c"}) );
            // op3i - uint
            addFunctionOp3i<uint32_t>(*this,lib);
            addFunctionOp3i<uint2>(*this,lib);
            addFunctionOp3i<uint3>(*this,lib);
            addFunctionOp3i<uint4>(*this,lib);
            addFunction( make_smart<BuiltInFn<Sim_MadS<uint2>,   uint2, uint2,  uint32_t,  uint2> >("mad", lib, "MadS")->args({"a","b","c"}) );
            addFunction( make_smart<BuiltInFn<Sim_MadS<uint3>,   uint3, uint3,  uint32_t,  uint3> >("mad", lib, "MadS")->args({"a","b","c"}) );
            addFunction( make_smart<BuiltInFn<Sim_MadS<uint4>,   uint4, uint4,  uint32_t,  uint4> >("mad", lib, "MadS")->args({"a","b","c"}) );
            // and double
            addFunctionOp3<double>(*this,lib);
            //common
            addFunctionCommon<float>(*this, lib);
            addFunctionCommon<float2>(*this,lib);
            addFunctionCommon<float3>(*this,lib);
            addFunctionCommon<float4>(*this,lib);
            addFunctionCommonTyped<int32_t>(*this, lib);
            addFunctionCommonTyped<int2>(*this, lib);
            addFunctionCommonTyped<int3>(*this, lib);
            addFunctionCommonTyped<int4>(*this, lib);
            addFunctionCommonTyped<uint32_t>(*this, lib);
            addFunctionCommonTyped<uint2>(*this, lib);
            addFunctionCommonTyped<uint3>(*this, lib);
            addFunctionCommonTyped<uint4>(*this, lib);
            addFunctionCommonTyped<float>(*this, lib);
            addFunctionCommonTyped<float2>(*this, lib);
            addFunctionCommonTyped<float3>(*this, lib);
            addFunctionCommonTyped<float4>(*this, lib);
            addFunctionCommonTyped<double>(*this, lib);
            addFunctionCommonTyped<int64_t>(*this, lib);
            addFunctionCommonTyped<uint64_t>(*this, lib);
            addExtern<DAS_BIND_FUN(uint32_hash)>(*this, lib, "uint32_hash", SideEffects::none, "uint32_hash")->arg("seed");
            addExtern<DAS_BIND_FUN(uint_noise1D)>(*this, lib, "uint_noise_1D", SideEffects::none, "uint_noise1D")->args({"position","seed"});
            addExtern<DAS_BIND_FUN(uint_noise2D_int2)>(*this, lib, "uint_noise_2D", SideEffects::none, "uint_noise2D_int2")->args({"position","seed"});
            addExtern<DAS_BIND_FUN(uint_noise3D_int3)>(*this, lib, "uint_noise_3D", SideEffects::none, "uint_noise3D_int3")->args({"position","seed"});
            addExternEx<float(float2,float2),DAS_BIND_FUN(dot2)>(*this, lib, "dot", SideEffects::none, "dot2")->args({"x","y"});
            addExternEx<float(float3,float3),DAS_BIND_FUN(dot3)>(*this, lib, "dot", SideEffects::none, "dot3")->args({"x","y"});
            addExternEx<float(float4,float4),DAS_BIND_FUN(dot4)>(*this, lib, "dot", SideEffects::none, "dot4")->args({"x","y"});
            addExternEx<float3(float3,float3),DAS_BIND_FUN(cross3)>(*this, lib, "cross", SideEffects::none, "cross3")->args({"x","y"});
            addExternEx<float2(float2),DAS_BIND_FUN(normalize2)>(*this, lib, "fast_normalize", SideEffects::none, "normalize2")->arg("x");
            addExternEx<float3(float3),DAS_BIND_FUN(normalize3)>(*this, lib, "fast_normalize", SideEffects::none, "normalize3")->arg("x");
            addExternEx<float4(float4),DAS_BIND_FUN(normalize4)>(*this, lib, "fast_normalize", SideEffects::none, "normalize4")->arg("x");
            addExternEx<float2(float2),DAS_BIND_FUN(safe_normalize2)>(*this, lib, "normalize", SideEffects::none, "safe_normalize2")->arg("x");
            addExternEx<float3(float3),DAS_BIND_FUN(safe_normalize3)>(*this, lib, "normalize", SideEffects::none, "safe_normalize3")->arg("x");
            addExternEx<float4(float4),DAS_BIND_FUN(safe_normalize4)>(*this, lib, "normalize", SideEffects::none, "safe_normalize4")->arg("x");
            addExternEx<float(float2),DAS_BIND_FUN(length2)>(*this, lib, "length", SideEffects::none, "length2")->arg("x");
            addExternEx<float(float3),DAS_BIND_FUN(length3)>(*this, lib, "length", SideEffects::none, "length3")->arg("x");
            addExternEx<float(float4),DAS_BIND_FUN(length4)>(*this, lib, "length", SideEffects::none, "length4")->arg("x");
            addExternEx<float(float2),DAS_BIND_FUN(invlength2)>(*this, lib, "inv_length", SideEffects::none,"invlength2")->arg("x");
            addExternEx<float(float3),DAS_BIND_FUN(invlength3)>(*this, lib, "inv_length", SideEffects::none,"invlength3")->arg("x");
            addExternEx<float(float4),DAS_BIND_FUN(invlength4)>(*this, lib, "inv_length", SideEffects::none,"invlength4")->arg("x");
            addExternEx<float(float2),DAS_BIND_FUN(invlengthSq2)>(*this, lib, "inv_length_sq", SideEffects::none, "invlengthSq2")->arg("x");
            addExternEx<float(float3),DAS_BIND_FUN(invlengthSq3)>(*this, lib, "inv_length_sq", SideEffects::none, "invlengthSq3")->arg("x");
            addExternEx<float(float4),DAS_BIND_FUN(invlengthSq4)>(*this, lib, "inv_length_sq", SideEffects::none, "invlengthSq4")->arg("x");
            addExternEx<float(float2),DAS_BIND_FUN(lengthSq2)>(*this, lib, "length_sq", SideEffects::none, "lengthSq2")->arg("x");
            addExternEx<float(float3),DAS_BIND_FUN(lengthSq3)>(*this, lib, "length_sq", SideEffects::none, "lengthSq3")->arg("x");
            addExternEx<float(float4),DAS_BIND_FUN(lengthSq4)>(*this, lib, "length_sq", SideEffects::none, "lengthSq4")->arg("x");
            addExternEx<float(float2,float2),DAS_BIND_FUN(distance2)>(*this, lib, "distance", SideEffects::none, "distance2")->args({"x","y"});
            addExternEx<float(float2,float2),DAS_BIND_FUN(distanceSq2)>(*this, lib, "distance_sq", SideEffects::none, "distanceSq2")->args({"x","y"});
            addExternEx<float(float2,float2),DAS_BIND_FUN(invdistance2)>(*this, lib, "inv_distance", SideEffects::none, "invdistance2")->args({"x","y"});
            addExternEx<float(float2,float2),DAS_BIND_FUN(invdistanceSq2)>(*this, lib, "inv_distance_sq", SideEffects::none, "invdistanceSq2")->args({"x","y"});
            addExternEx<float(float3,float3),DAS_BIND_FUN(distance3)>(*this, lib, "distance", SideEffects::none, "distance3")->args({"x","y"});
            addExternEx<float(float3,float3),DAS_BIND_FUN(distanceSq3)>(*this, lib, "distance_sq", SideEffects::none, "distanceSq3")->args({"x","y"});
            addExternEx<float(float3,float3),DAS_BIND_FUN(invdistance3)>(*this, lib, "inv_distance", SideEffects::none, "invdistance3")->args({"x","y"});
            addExternEx<float(float3,float3),DAS_BIND_FUN(invdistanceSq3)>(*this, lib, "inv_distance_sq", SideEffects::none, "invdistanceSq3")->args({"x","y"});
            addExternEx<float(float4,float4),DAS_BIND_FUN(distance4)>(*this, lib, "distance", SideEffects::none, "distance4")->args({"x","y"});
            addExternEx<float(float4,float4),DAS_BIND_FUN(distanceSq4)>(*this, lib, "distance_sq", SideEffects::none, "distanceSq4")->args({"x","y"});
            addExternEx<float(float4,float4),DAS_BIND_FUN(invdistance4)>(*this, lib, "inv_distance", SideEffects::none, "invdistance4")->args({"x","y"});
            addExternEx<float(float4,float4),DAS_BIND_FUN(invdistanceSq4)>(*this, lib, "inv_distance_sq", SideEffects::none, "invdistanceSq4")->args({"x","y"});
            addExternEx<float2(float2,float2,float),DAS_BIND_FUN(lerp_vec_float)>(*this, lib, "lerp", SideEffects::none, "lerp_vec_float")->args({"a", "b", "t"});
            addExternEx<float3(float3,float3,float),DAS_BIND_FUN(lerp_vec_float)>(*this, lib, "lerp", SideEffects::none, "lerp_vec_float")->args({"a", "b", "t"});
            addExternEx<float4(float4,float4,float),DAS_BIND_FUN(lerp_vec_float)>(*this, lib, "lerp", SideEffects::none, "lerp_vec_float")->args({"a", "b", "t"});

            // unique float functions
            addExtern<DAS_BIND_FUN(fisnan)>(*this, lib, "is_nan", SideEffects::none, "fisnan")->arg("x");
            addExtern<DAS_BIND_FUN(fisfinite)>(*this, lib, "is_finite", SideEffects::none, "fisfinite")->arg("x");
            //double functions
            addExtern<DAS_BIND_FUN(disnan)>(*this, lib, "is_nan", SideEffects::none, "disnan")->arg("x");
            addExtern<DAS_BIND_FUN(disfinite)>(*this, lib, "is_finite", SideEffects::none, "disfinite")->arg("x");
            addExtern<DAS_BIND_FUN(dsqrt)>(*this, lib, "sqrt",   SideEffects::none, "dsqrt")->arg("x");
            addExtern<DAS_BIND_FUN(dexp)>(*this, lib, "exp",     SideEffects::none, "dexp")->arg("x");
            addExtern<DAS_BIND_FUN(drcp)>(*this, lib, "rcp",     SideEffects::none, "drcp")->arg("x");
            addExtern<DAS_BIND_FUN(dlog)>(*this, lib, "log",     SideEffects::none, "dlog")->arg("x");
            addExtern<DAS_BIND_FUN(dpow)>(*this, lib, "pow",     SideEffects::none, "dpow")->args({"x","y"});
            addExtern<DAS_BIND_FUN(dexp2)>(*this, lib, "exp2",   SideEffects::none, "dexp2")->arg("x");
            addExtern<DAS_BIND_FUN(dlog2)>(*this, lib, "log2",   SideEffects::none, "dlog2")->arg("x");
            addExtern<DAS_BIND_FUN(dsin)>(*this, lib, "sin",     SideEffects::none, "dsin")->arg("x");
            addExtern<DAS_BIND_FUN(dcos)>(*this, lib, "cos",     SideEffects::none, "dcos")->arg("x");
            addExtern<DAS_BIND_FUN(dasin)>(*this, lib, "asin",   SideEffects::none, "dasin")->arg("x");
            addExtern<DAS_BIND_FUN(dacos)>(*this, lib, "acos",   SideEffects::none, "dacos")->arg("x");
            addExtern<DAS_BIND_FUN(dtan)>(*this, lib, "tan",     SideEffects::none, "dtan")->arg("x");
            addExtern<DAS_BIND_FUN(datan)>(*this, lib, "atan",   SideEffects::none, "datan")->arg("x");
            addExtern<DAS_BIND_FUN(datan2)>(*this, lib, "atan2", SideEffects::none, "datan2")->args({"y","x"});
            addExtern<DAS_BIND_FUN(sincosF)>(*this, lib, "sincos", SideEffects::modifyArgument, "sincosF")->args({"x","s","c"});
            addExtern<DAS_BIND_FUN(sincosD)>(*this, lib, "sincos", SideEffects::modifyArgument, "sincosD")->args({"x","s","c"});
            addExternEx<float3(float3,float3),DAS_BIND_FUN(reflect)>(*this, lib, "reflect",
                SideEffects::none, "reflect")->args({"v","n"});
            addExternEx<float2(float2,float2),DAS_BIND_FUN(reflect2)>(*this, lib, "reflect",
                SideEffects::none, "reflect2")->args({"v","n"});
            addExternEx<float3(float3,float3,float),DAS_BIND_FUN(refract)>(*this, lib, "refract",
                SideEffects::none, "refract")->args({"v","n","nint"});
            addExternEx<float2(float2,float2,float),DAS_BIND_FUN(refract2)>(*this, lib, "refract",
                SideEffects::none, "refract2")->args({"v","n","nint"});
            addFunctionCommonConversion<int, float>  (*this, lib);
            addFunctionCommonConversion<int, double>  (*this, lib);
            addFunctionCommonConversion<int2, float2>(*this,lib);
            addFunctionCommonConversion<int3, float3>(*this,lib);
            addFunctionCommonConversion<int4, float4>(*this,lib);
            // structure annotations
            addAnnotation(make_smart<float4x4_ann>(lib));
            addAnnotation(make_smart<float3x4_ann>(lib));
            addAnnotation(make_smart<float3x3_ann>(lib));
            // c-tor
            addFunction ( make_smart< BuiltInFn< SimNode_MatrixCtor<float3x3>,float3x3 > >("float3x3",lib) );
            addFunction ( make_smart< BuiltInFn< SimNode_MatrixCtor<float3x4>,float3x4 > >("float3x4",lib) );
            addFunction ( make_smart< BuiltInFn< SimNode_MatrixCtor<float4x4>,float4x4 > >("float4x4",lib) );
            // 4x4
            addExtern<DAS_BIND_FUN(float4x4_from_float34), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "float4x4",
                SideEffects::none,"float4x4_from_float34");
            addExtern<DAS_BIND_FUN(float4x4_identity)>(*this, lib, "identity",
                SideEffects::modifyArgument, "float4x4_identity")->arg("x");
            addExtern<DAS_BIND_FUN(float4x4_identity_m), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "identity4x4",
                SideEffects::none,"float4x4_identity_m");
            addExtern<DAS_BIND_FUN(float4x4_translation), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "translation",
                 SideEffects::none, "float4x4_translation")->arg("xyz");
            addExtern<DAS_BIND_FUN(float4x4_transpose), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "transpose",
                SideEffects::none, "float4x4_transpose")->arg("x");
            addExtern<DAS_BIND_FUN(float4x4_persp_forward), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "persp_forward",
                SideEffects::none, "float4x4_persp_forward")->args({"wk", "hk", "zn", "zf"});
            addExtern<DAS_BIND_FUN(float4x4_persp_reverse), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "persp_reverse",
                SideEffects::none, "float4x4_persp_reverse")->args({"wk", "hk", "zn", "zf"});
            addExtern<DAS_BIND_FUN(float4x4_look_at), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "look_at",
                SideEffects::none, "float4x4_look_at")->args({"eye", "at", "up"});
            addExtern<DAS_BIND_FUN(float4x4_compose), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "compose",
                SideEffects::none, "float4x4_compose")->args({"pos", "rot", "scale"});
            addExtern<DAS_BIND_FUN(float4x4_mul), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "*",
                SideEffects::none,"float4x4_mul")->args({"x", "y"});
            addExtern<DAS_BIND_FUN(float4x4_decompose)>(*this, lib, "decompose",
                SideEffects::modifyArgument, "float4x4_decompose")->args({"mat","pos","rot","scale"});
            addExtern<DAS_BIND_FUN(float4x4_equ)>(*this, lib, "==",
                SideEffects::none, "float4x4_equ")->args({"x","y"});
            addExtern<DAS_BIND_FUN(float4x4_nequ)>(*this, lib, "!=",
                SideEffects::none, "float4x4_nequ")->args({"x","y"});
            addExtern<DAS_BIND_FUN(float4x4_neg), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "-",
                SideEffects::none,"float4x4_neg")->arg("x");
            initFloatNxNIndex(addExtern<DAS_BIND_FUN((floatNxN_ati<float4x4>)), SimNode_ExtFuncCallRef>(*this, lib,
                "[]", SideEffects::none, "floatNxN_ati<float4>")->args({"m","i","context","at"}));
            initFloatNxNIndex(addExtern<DAS_BIND_FUN((floatNxN_atci<float4x4>)), SimNode_ExtFuncCallRef>(*this, lib,
                "[]", SideEffects::none, "floatNxN_atci<float4>")->args({"m","i","context","at"}));
            initFloatNxNIndex(addExtern<DAS_BIND_FUN((floatNxN_atu<float4x4>)), SimNode_ExtFuncCallRef>(*this, lib,
                "[]", SideEffects::none, "floatNxN_ati<float4>")->args({"m","i","context","at"}));
            initFloatNxNIndex(addExtern<DAS_BIND_FUN((floatNxN_atcu<float4x4>)), SimNode_ExtFuncCallRef>(*this, lib,
                "[]", SideEffects::none, "floatNxN_atci<float4>")->args({"m","i","context","at"}));
            // 3x4
            addExtern<DAS_BIND_FUN(float3x4_from_float44), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "float3x4",
                SideEffects::none,"float3x4_from_float44");
            addExtern<DAS_BIND_FUN(float3x4_identity)>(*this, lib, "identity",
                SideEffects::modifyArgument,"float3x4_identity")->arg("x");
            addExtern<DAS_BIND_FUN(float3x4_identity_m), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "identity3x4",
                SideEffects::none,"float3x4_identity_m");
            addExtern<DAS_BIND_FUN(float3x4_mul), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "*",
                SideEffects::none,"float3x4_mul")->args({"x","y"});
            addExtern<DAS_BIND_FUN(float3x4_mul_vec3p), SimNode_ExtFuncCall>(*this, lib, "*",
                SideEffects::none,"float3x4_mul_vec3p")->args({"x","y"});
            addExtern<DAS_BIND_FUN(float4x4_mul_vec4), SimNode_ExtFuncCall>(*this, lib, "*",
                SideEffects::none,"float4x4_mul_vec4")->args({"x","y"});
            addExtern<DAS_BIND_FUN(float3x4_inverse), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
                "inverse", SideEffects::none, "float3x4_inverse")->arg("x");
            addExtern<DAS_BIND_FUN(float4x4_inverse), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
                "inverse", SideEffects::none, "float4x4_inverse")->arg("m");
            addExtern<DAS_BIND_FUN(float4x4_orthonormal_inverse), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
                "orthonormal_inverse", SideEffects::none, "float4x4_orthonormal_inverse")->arg("m");
            addExtern<DAS_BIND_FUN(rotate)>(*this, lib, "rotate",
                SideEffects::none, "rotate")->args({"x","y"});
            addExtern<DAS_BIND_FUN(float3x4_equ)>(*this, lib, "==",
                SideEffects::none, "float3x4_equ")->args({"x","y"});
            addExtern<DAS_BIND_FUN(float3x4_nequ)>(*this, lib, "!=",
                SideEffects::none, "float3x4_nequ")->args({"x","y"});
            addExtern<DAS_BIND_FUN(float3x4_neg), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "-",
                SideEffects::none,"float3x4_neg")->arg("x");
            initFloatNxNIndex(addExtern<DAS_BIND_FUN((floatNxN_ati<float3x4>)), SimNode_ExtFuncCallRef>(*this, lib,
                "[]", SideEffects::none, "floatNxN_ati<float4>")->args({"m","i","context","at"}));
            initFloatNxNIndex(addExtern<DAS_BIND_FUN((floatNxN_atci<float3x4>)), SimNode_ExtFuncCallRef>(*this, lib,
                "[]", SideEffects::none, "floatNxN_atci<float4>")->args({"m","i","context","at"}));
            initFloatNxNIndex(addExtern<DAS_BIND_FUN((floatNxN_atu<float3x4>)), SimNode_ExtFuncCallRef>(*this, lib,
                "[]", SideEffects::none, "floatNxN_ati<float4>")->args({"m","i","context","at"}));
            initFloatNxNIndex(addExtern<DAS_BIND_FUN((floatNxN_atcu<float3x4>)), SimNode_ExtFuncCallRef>(*this, lib,
                "[]", SideEffects::none, "floatNxN_atci<float4>")->args({"m","i","context","at"}));
            // quat
            addExtern<DAS_BIND_FUN(quat_from_unit_arc)>(*this, lib, "quat_from_unit_arc",
                SideEffects::none, "quat_from_unit_arc")->args({"v0","v1"});
            addExtern<DAS_BIND_FUN(quat_from_unit_vec_ang)>(*this, lib, "quat_from_unit_vec_ang",
                SideEffects::none, "quat_from_unit_vec_ang")->args({"v","ang"});
            addExtern<DAS_BIND_FUN(quat_from_euler_vec)>(*this, lib, "quat_from_euler",
                SideEffects::none, "quat_from_euler_vec")->args({"angles"});
            addExtern<DAS_BIND_FUN(quat_from_euler)>(*this, lib, "quat_from_euler",
                SideEffects::none, "quat_from_euler")->args({"x", "y", "z"});
            addExtern<DAS_BIND_FUN(euler_from_un_quat_vec)>(*this, lib, "euler_from_un_quat",
                SideEffects::none, "euler_from_un_quat_vec")->args({"angles"});
            addExtern<DAS_BIND_FUN(un_quat)>(*this, lib, "un_quat",
                SideEffects::none, "un_quat")->arg("m");
            addExtern<DAS_BIND_FUN(quat_mul)>(*this, lib, "quat_mul",
                SideEffects::none, "quat_mul")->args({"q1","q2"});
            addExtern<DAS_BIND_FUN(quat_mul_vec)>(*this, lib, "quat_mul_vec",
                SideEffects::none, "quat_mul_vec")->args({"q","v"});
            addExtern<DAS_BIND_FUN(quat_conjugate)>(*this, lib, "quat_conjugate",
                SideEffects::none, "quat_conjugate")->arg("q");
            // 3x3
            addExtern<DAS_BIND_FUN(float3x3_from_float44), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "float3x3",
                SideEffects::none,"float3x3_from_float44");
            addExtern<DAS_BIND_FUN(float3x3_from_float34), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "float3x3",
                SideEffects::none,"float3x3_from_float34");
            addExtern<DAS_BIND_FUN(float3x3_identity)>(*this, lib, "identity",
                SideEffects::modifyArgument,"float3x3_identity")->arg("x");
            addExtern<DAS_BIND_FUN(float3x3_identity_m), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "identity3x3",
                SideEffects::none,"float3x3_identity_m");
            addExtern<DAS_BIND_FUN(float3x3_mul), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "*",
                SideEffects::none,"float3x3_mul")->args({"x", "y"});
            addExtern<DAS_BIND_FUN(float3x3_mul_vec3), SimNode_ExtFuncCall>(*this, lib, "*",
                SideEffects::none,"float3x3_mul_vec3")->args({"x","y"});
            addExtern<DAS_BIND_FUN(float3x3_equ)>(*this, lib, "==",
                SideEffects::none, "float3x3_equ")->args({"x","y"});
            addExtern<DAS_BIND_FUN(float3x3_nequ)>(*this, lib, "!=",
                SideEffects::none, "float3x3_nequ")->args({"x","y"});
            addExtern<DAS_BIND_FUN(float3x3_neg), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "-",
                SideEffects::none,"float3x3_neg")->arg("x");
           initFloatNxNIndex(addExtern<DAS_BIND_FUN((floatNxN_ati<float3x3>)), SimNode_ExtFuncCallRef>(*this, lib,
                "[]", SideEffects::none, "floatNxN_ati<float4>")->args({"m","i","context","at"}));
            initFloatNxNIndex(addExtern<DAS_BIND_FUN((floatNxN_atci<float3x3>)), SimNode_ExtFuncCallRef>(*this, lib,
                "[]", SideEffects::none, "floatNxN_atci<float4>")->args({"m","i","context","at"}));
            initFloatNxNIndex(addExtern<DAS_BIND_FUN((floatNxN_atu<float3x3>)), SimNode_ExtFuncCallRef>(*this, lib,
                "[]", SideEffects::none, "floatNxN_ati<float4>")->args({"m","i","context","at"}));
            initFloatNxNIndex(addExtern<DAS_BIND_FUN((floatNxN_atcu<float3x3>)), SimNode_ExtFuncCallRef>(*this, lib,
                "[]", SideEffects::none, "floatNxN_atci<float4>")->args({"m","i","context","at"}));
            // packing
            addExtern<DAS_BIND_FUN(pack_float_to_byte)>(*this, lib, "pack_float_to_byte",
                SideEffects::none,"pack_float_to_byte")->arg("x");
            addExtern<DAS_BIND_FUN(unpack_byte_to_float)>(*this, lib, "unpack_byte_to_float",
                SideEffects::none,"unpack_byte_to_float")->arg("x");
            // and check everything
            verifyAotReady();
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            tw << "#include \"daScript/simulate/aot_builtin_math.h\"\n";
            return ModuleAotType::cpp;
        }
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_Math,das);

IMPLEMENT_EXTERNAL_TYPE_FACTORY(float4x4, das::float4x4)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(float3x4, das::float3x4)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(float3x3, das::float3x3)
