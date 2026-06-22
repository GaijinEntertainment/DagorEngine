#pragma once

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/simulate_nodes.h"

#ifndef DAS_FUSION
    #define DAS_FUSION  0
#endif

// Safe vec4f load for a workhorse-sized source. Picks the load width by sizeof(CTYPE)
// at compile time so we never v_ldu past a 4/8/12-byte source — important when the
// source sits within 16 bytes of an unmapped page (real SIGSEGV under TSan, not just
// a sanitizer over-read warning). Each branch is a single SIMD intrinsic; memcpy
// fallback only on exotic sizes.
#define DAS_LDU_WORKHORSE(dst, src_ptr, CTYPE) \
    do { \
        if constexpr (sizeof(CTYPE) == sizeof(vec4f))         (dst) = v_ldu((const float *)(src_ptr)); \
        else if constexpr (sizeof(CTYPE) == sizeof(float) * 3) (dst) = v_ldu_p3_safe((const float *)(src_ptr)); \
        else if constexpr (sizeof(CTYPE) == sizeof(float) * 2) (dst) = v_ldu_half((const float *)(src_ptr)); \
        else if constexpr (sizeof(CTYPE) == sizeof(float))     (dst) = v_set_x(*(const float *)(src_ptr)); \
        else { vec4f __r = v_zero(); memcpy(&__r, (src_ptr), sizeof(CTYPE)); (dst) = __r; } \
    } while (0)

namespace das {

    typedef char * StringPtr;
    typedef void * VoidPtr;

    struct SimNodeInfo {
        string  name;
        string  typeName;
        size_t  typeSize;
    };

    typedef das_hash_map<SimNode *,SimNodeInfo> SimNodeInfoLookup;

    struct FusionPoint {
        FusionPoint () {}
        virtual ~FusionPoint() {}
        virtual SimNode * fuse ( const SimNodeInfoLookup &, SimNode * node, Context * ) { return node; }
        static bool is ( const SimNodeInfoLookup & info, SimNode * node, const char * name );
        static bool is2 ( const SimNodeInfoLookup & info, SimNode * lnode, SimNode * rnode, const char * lname, const char * rname );
        static bool is ( const SimNodeInfoLookup & info, SimNode * node, const char * name, const char * typeName );
    };
    typedef unique_ptr<FusionPoint> FusionPointPtr;

    typedef das_hash_map<string,vector<FusionPointPtr>> FusionEngine;   // note: unordered map for thread safety

    const char * getSimSourceName(SimSourceType st);

    struct SimNode_Op1Fusion : SimNode_WithErrorMessage {
        SimNode_Op1Fusion(const char * msg = nullptr) : SimNode_WithErrorMessage(LineInfo(),msg) {}
        void set(const char * opn, Type bt, const LineInfo & at) {
            op = opn;
            baseType = bt;
            debugInfo = at;
        }
        virtual SimNode * visit(SimVisitor & vis) override;
        const char *    op = nullptr;
        Type            baseType;
        SimSource       subexpr;
    };

    struct SimNode_Op2Fusion : SimNode_WithErrorMessage {
        SimNode_Op2Fusion(const char * msg = nullptr) : SimNode_WithErrorMessage(LineInfo(),msg) {}
        void set(const char * opn, Type bt, const LineInfo & at) {
            op = opn;
            baseType = bt;
            debugInfo = at;
        }
        virtual SimNode * visit(SimVisitor & vis) override;
        const char *    op = nullptr;
        Type            baseType;
        SimSource       l, r;
    };

    struct FusionPointOp1 : FusionPoint {
        virtual SimNode * match(const SimNodeInfoLookup &, SimNode *, SimNode *, Context *) = 0;
        virtual void set(SimNode_Op1Fusion * result, SimNode * node) = 0;
        virtual SimNode * fuseOp1(const SimNodeInfoLookup & info, SimNode * node, SimNode * node_x, Context * context);
    };

    struct FusionPointOp2 : FusionPoint {
        FusionPointOp2() {}
        virtual SimNode * match(const SimNodeInfoLookup &, SimNode *, SimNode *, SimNode *, Context *) = 0;
        virtual void set(SimNode_Op2Fusion * result, SimNode * node) = 0;
        virtual SimNode * fuseOp2(const SimNodeInfoLookup & info, SimNode * node, SimNode * node_l, SimNode * node_r, Context * context);
        bool anyLeft, anyRight;
    };


    // function pointers for fusion library decoupling
    // runtime defines these; fusion lib sets them via register_fusion()
    DAS_API extern void (*g_fusionContextFn) ( Context & context, TextWriter & logs, bool enableFusion );
    DAS_API extern void (*g_resetFusionEngineFn) ();

    string fuseName ( const string & name, const string & typeName );
    unique_ptr<FusionEngine> &getFusionEngine();
    void resetFusionEngine();
    void createFusionEngine();
    void registerFusion ( const char * OpName, const char * CTypeName, FusionPoint * node );

#if DAS_FUSION
    // fusion engine subsections
    // misc (note, misc before everything)
    void createFusionEngine_misc_copy_reference();
    // op1
    void createFusionEngine_op1();
    void createFusionEngine_return();
    void createFusionEngine_ptrfdr();
    void createFusionEngine_if();
    // scalar
    void createFusionEngine_op2();
    void createFusionEngine_op2_set();
    void createFusionEngine_op2_bool();
    void createFusionEngine_op2_bin();
    // vector
    void createFusionEngine_op2_vec();
    void createFusionEngine_op2_set_vec();
    void createFusionEngine_op2_bool_vec();
    void createFusionEngine_op2_bin_vec();
    void createFusionEngine_op2_scalar_vec();
    // at
    void createFusionEngine_at();
    void createFusionEngine_at_array();
    void createFusionEngine_tableindex();
    void createFusionEngine_tablewithhash();
    // call
    void createFusionEngine_call1();
    void createFusionEngine_call2();
#endif
}

extern DAS_CC_API void register_fusion ();
