#pragma once

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/simulate_nodes.h"

#ifndef DAS_FUSION
    #define DAS_FUSION  0
#endif

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
    extern DAS_THREAD_LOCAL unique_ptr<FusionEngine> g_fusionEngine;

    const char * getSimSourceName(SimSourceType st);

    struct SimNode_Op1Fusion : SimNode {
        SimNode_Op1Fusion() : SimNode(LineInfo()) {}
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

    struct SimNode_Op2Fusion : SimNode {
        SimNode_Op2Fusion() : SimNode(LineInfo()) {}
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


    string fuseName ( const string & name, const string & typeName );
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
    // call
    void createFusionEngine_call1();
    void createFusionEngine_call2();
#endif
}
