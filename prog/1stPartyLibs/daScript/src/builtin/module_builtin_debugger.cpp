#include "daScript/misc/platform.h"

#include "daScript/simulate/simulate_nodes.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_policy_types.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/simulate/aot_builtin_debugger.h"
#include "module_builtin_rtti.h"
#include "daScript/misc/performance_time.h"
#include "daScript/misc/sysos.h"

using namespace das;

MAKE_TYPE_FACTORY(DebugAgent,DebugAgent)
MAKE_TYPE_FACTORY(DataWalker,DataWalker)
MAKE_TYPE_FACTORY(StackWalker,StackWalker)
MAKE_TYPE_FACTORY(Prologue,Prologue)

namespace das
{
    struct PrologueAnnotation : ManagedStructureAnnotation<Prologue,false> {
        PrologueAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Prologue", ml) {
            addField<DAS_BIND_MANAGED_FIELD(info)>("info");
            addField<DAS_BIND_MANAGED_FIELD(block)>("_block", "block");
            addField<DAS_BIND_MANAGED_FIELD(fileName)>("fileName");
            addField<DAS_BIND_MANAGED_FIELD(stackSize)>("stackSize");
            addField<DAS_BIND_MANAGED_FIELD(arguments)>("arguments");
            addField<DAS_BIND_MANAGED_FIELD(cmres)>("cmres");
            addField<DAS_BIND_MANAGED_FIELD(line)>("line");
        }
    };

// we declare Dapi version of each structure in debugger.das
// to fake the API we need C++ version, which we now make locally
namespace debugapi {
    typedef Array DapiArray;
    typedef Table DapiTable;
    typedef Lambda DapiLambda;
    typedef Sequence DapiSequence;
    typedef Block DapiBlock;
    typedef Func DapiFunc;
}

#include "debugapi_gen.inc"

    struct DebugAgentAdapter : DebugAgent, DapiDebugAgent_Adapter {
        DebugAgentAdapter ( char * pClass, const StructInfo * info, Context * ctx )
            : DapiDebugAgent_Adapter(info), classPtr(pClass), classInfo(info), context(ctx) {
       }
        virtual void onInstall ( DebugAgent * agent ) override {
            if ( auto fnOnInstall = get_onInstall(classPtr) ) {
                context->lock();
                invoke_onInstall(context,fnOnInstall,classPtr,agent);
                context->unlock();
            }
        }
        virtual void onUninstall ( DebugAgent * agent ) override {
            if ( auto fnOnUninstall = get_onUninstall(classPtr) ) {
                context->lock();
                invoke_onUninstall(context,fnOnUninstall,classPtr,agent);
                context->unlock();
            }
        }
        virtual void onCreateContext ( Context * ctx ) override {
            if ( auto fnOnCreateContext = get_onCreateContext(classPtr)) {
                context->lock();
                invoke_onCreateContext(context,fnOnCreateContext,classPtr,*ctx);
                context->unlock();
            }
        }
        virtual void onDestroyContext ( Context * ctx ) override {
            if ( auto fnOnDestroyContext = get_onDestroyContext(classPtr) ) {
                context->lock();
                invoke_onDestroyContext(context,fnOnDestroyContext,classPtr,*ctx);
                context->unlock();
            }
        }
        virtual void onSimulateContext ( Context * ctx ) override {
            if ( auto fnOnSimulateContext = get_onSimulateContext(classPtr)) {
                context->lock();
                invoke_onSimulateContext(context,fnOnSimulateContext,classPtr,*ctx);
                context->unlock();
            }
        }
        virtual void onSingleStep ( Context * ctx, const LineInfo & at ) override {
            if ( auto fnOnSingleStep = get_onSingleStep(classPtr) ) {
                context->lock();
                invoke_onSingleStep(context,fnOnSingleStep,classPtr,*ctx,at);
                context->unlock();
            }
        }
        virtual void onInstrument ( Context * ctx, const LineInfo & at ) override {
            if ( ctx==context ) return; // do not step into the same context
            if ( auto fnOnInstrument = get_onInstrument(classPtr) ) {
                context->lock();
                invoke_onInstrument(context,fnOnInstrument,classPtr,*ctx,at);
                context->unlock();
            }
        }
        virtual void onInstrumentFunction ( Context * ctx, SimFunction * sim, bool entering, uint64_t userData ) override {
            if ( ctx==context ) return;  // do not step into the same context
            if ( auto fnOnInstrumentFunction = get_onInstrumentFunction(classPtr) ) {
                context->lock();
                invoke_onInstrumentFunction(context,fnOnInstrumentFunction,classPtr,*ctx,sim,entering,userData);
                context->unlock();
            }
        }
        virtual void onBreakpoint ( Context * ctx, const LineInfo & at, const char * reason, const char * text ) override {
            if ( auto fnOnBreakpoint = get_onBreakpoint(classPtr) ) {
                context->lock();
                invoke_onBreakpoint(context,fnOnBreakpoint,classPtr,*ctx,at,(char *)reason,(char *)text);
                context->unlock();
            }
        }
        virtual void onVariable ( Context * ctx, const char * category, const char * name, TypeInfo * info, void * data ) override {
            if ( auto fnOnVariable = get_onVariable(classPtr) ) {
                context->lock();
                invoke_onVariable(context,fnOnVariable,classPtr,*ctx,(char *)category,(char *)name,*info,data);
                context->unlock();
            }
        }
        virtual void onTick () override {
            if ( auto fnOnTick = get_onTick(classPtr) ) {
                context->lock();
                invoke_onTick(context,fnOnTick,classPtr);
                context->unlock();
            }
        }
        virtual void onCollect ( Context * ctx, const LineInfo & at ) override {
            if ( auto fnOnCollect = get_onCollect(classPtr) ) {
                context->lock();
                invoke_onCollect(context,fnOnCollect,classPtr,*ctx,at);
                context->unlock();
            }
        }
        virtual bool onLog ( Context * ctx, const LineInfo * at, int level, const char * text ) override {
            if ( auto fnOnLog = get_onLog(classPtr) ) {
                context->lock();
                auto res = invoke_onLog(context,fnOnLog,classPtr,ctx,at,level,(char *)text);
                context->unlock();
                return res;
            } else {
                return false;
            }
        }
        virtual void onBreakpointsReset ( const char * file, int breakpointsNum ) override {
            if ( auto fnOnBreakpointsReset = get_onBreakpointsReset(classPtr) ) {
                context->lock();
                invoke_onBreakpointsReset(context,fnOnBreakpointsReset,classPtr,(char *)file, breakpointsNum);
                context->unlock();
            }
        }
    public:
        void *              classPtr = nullptr;
        const StructInfo *  classInfo = nullptr;
        Context *           context = nullptr;
    };

    struct AstDebugAgentAnnotation : ManagedStructureAnnotation<DebugAgent,false,true> {
        AstDebugAgentAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("DebugAgent", ml) {
            addField<DAS_BIND_MANAGED_FIELD(isThreadLocal)>("isThreadLocal");
        }
    };

    struct DataWalkerAdapter : DataWalker, DapiDataWalker_Adapter {
        DataWalkerAdapter ( char * pClass, const StructInfo * info, Context * ctx )
            : DapiDataWalker_Adapter(info), classPtr(pClass) {
            context = ctx;
        }
    // data structures
        virtual bool canVisitHandle ( char * ps, TypeInfo * ti ) override {
            if ( auto fn_canVisitHandle = get_canVisitHandle(classPtr) ) {
                return invoke_canVisitHandle(context,fn_canVisitHandle,classPtr,ps,*ti);
            } else {
                return true;
            }
        }
        virtual bool canVisitStructure ( char * ps, StructInfo * si ) override {
            if ( auto fn_canVisitStructure = get_canVisitStructure(classPtr) ) {
                return invoke_canVisitStructure(context,fn_canVisitStructure,classPtr,ps,*si);
            } else {
                return true;
            }
        }
        virtual void beforeStructure ( char * ps, StructInfo * si ) override {
            if ( auto fn_beforeStructure = get_beforeStructure(classPtr) ) {
                invoke_beforeStructure(context,fn_beforeStructure,classPtr,ps,*si);
            }
        }
        virtual void afterStructure ( char * ps, StructInfo * si ) override {
            if ( auto fn_afterStructure = get_afterStructure(classPtr) ) {
                invoke_afterStructure(context,fn_afterStructure,classPtr,ps,*si);
            }
        }
        virtual void afterStructureCancel ( char * ps, StructInfo * si ) override {
            if ( auto fn_afterStructureCancel = get_afterStructureCancel(classPtr) ) {
                invoke_afterStructureCancel(context,fn_afterStructureCancel,classPtr,ps,*si);
            }
        }
        virtual void beforeStructureField ( char * ps, StructInfo * si, char * pv, VarInfo * vi, bool last ) override {
            if ( auto fn_beforeStructureField = get_beforeStructureField(classPtr) ) {
                invoke_beforeStructureField(context,fn_beforeStructureField,classPtr,ps,*si,pv,*vi,last);
            }
        }
        virtual void afterStructureField ( char * ps, StructInfo * si, char * pv, VarInfo * vi, bool last ) override {
            if ( auto fn_afterStructureField = get_afterStructureField(classPtr) ) {
                invoke_afterStructureField(context,fn_afterStructureField,classPtr,ps,*si,pv,*vi,last);
            }
        }
        virtual bool canVisitTuple ( char * ps, TypeInfo * ti ) override {
            if ( auto fn_canVisitTuple = get_canVisitTuple(classPtr) ) {
                return invoke_canVisitTuple(context,fn_canVisitTuple,classPtr,ps,*ti);
            } else {
                return true;
            }
        }
        virtual void beforeTuple ( char * ps, TypeInfo * ti ) override {
            if ( auto fn_beforeTuple = get_beforeTuple(classPtr) ) {
                invoke_beforeTuple(context,fn_beforeTuple,classPtr,ps,*ti);
            }
        }
        virtual void afterTuple ( char * ps, TypeInfo * ti ) override {
            if ( auto fn_afterTuple = get_afterTuple(classPtr) ) {
                invoke_afterTuple(context,fn_afterTuple,classPtr,ps,*ti);
            }
        }
        virtual void beforeTupleEntry ( char * ps, TypeInfo * ti, char * pv, TypeInfo * vi, bool last ) override {
            if ( auto fn_beforeTupleEntry = get_beforeTupleEntry(classPtr) ) {
                invoke_beforeTupleEntry(context,fn_beforeTupleEntry,classPtr,ps,*ti,pv,*vi,last);
            }
        }
        virtual void afterTupleEntry ( char * ps, TypeInfo * ti, char * pv, TypeInfo * vi, bool last ) override {
            if ( auto fn_afterTupleEntry = get_afterTupleEntry(classPtr) ) {
                invoke_afterTupleEntry(context,fn_afterTupleEntry,classPtr,ps,*ti,pv,*vi,last);
            }
        }
        virtual bool canVisitVariant ( char * ps, TypeInfo * ti ) override {
            if ( auto fn_canVisitVariant = get_canVisitVariant(classPtr) ) {
                return invoke_canVisitVariant(context,fn_canVisitVariant,classPtr,ps,*ti);
            } else {
                return true;
            }
        }
        virtual void beforeVariant ( char * ps, TypeInfo * ti ) override {
            if ( auto fn_beforeVariant = get_beforeVariant(classPtr) ) {
                invoke_beforeVariant(context,fn_beforeVariant,classPtr,ps,*ti);
            }
        }
        virtual void afterVariant ( char * ps, TypeInfo * ti ) override {
            if ( auto fn_afterVariant = get_afterVariant(classPtr) ) {
                invoke_afterVariant(context,fn_afterVariant,classPtr,ps,*ti);
            }
        }
        virtual bool canVisitArray ( Array * pa, TypeInfo * ti ) override {
            if ( auto fn_canVisitArray = get_canVisitArray(classPtr) ) {
                return invoke_canVisitArray(context,fn_canVisitArray,classPtr,pa,*ti);
            } else {
                return true;
            }
        }
        virtual bool canVisitArrayData ( TypeInfo * ti, uint32_t count ) override {
            if ( auto fn_canVisitArrayData = get_canVisitArrayData(classPtr) ) {
                return invoke_canVisitArrayData(context,fn_canVisitArrayData,classPtr,*ti,count);
            } else {
                return true;
            }
        }
        virtual void beforeArrayData ( char * pa, uint32_t stride, uint32_t count, TypeInfo * ti ) override {
            if ( auto fn_beforeArrayData = get_beforeArrayData(classPtr) ) {
                invoke_beforeArrayData(context,fn_beforeArrayData,classPtr,pa,stride,count,*ti);
            }
        }
        virtual void afterArrayData ( char * pa, uint32_t stride, uint32_t count, TypeInfo * ti ) override {
            if ( auto fn_afterArrayData = get_afterArrayData(classPtr) ) {
                invoke_afterArrayData(context,fn_afterArrayData,classPtr,pa,stride,count,*ti);
            }
        }
        virtual void beforeArrayElement ( char * pa, TypeInfo * ti, char * pe, uint32_t index, bool last ) override {
            if ( auto fn_beforeArrayElement = get_beforeArrayElement(classPtr) ) {
                invoke_beforeArrayElement(context,fn_beforeArrayElement,classPtr,pa,*ti,pe,index,last);
            }
        }
        virtual void afterArrayElement ( char * pa, TypeInfo * ti, char * pe, uint32_t index, bool last ) override {
            if ( auto fn_afterArrayElement = get_afterArrayElement(classPtr) ) {
                invoke_afterArrayElement(context,fn_afterArrayElement,classPtr,pa,*ti,pe,index,last);
            }
        }
        virtual void beforeDim ( char * pa, TypeInfo * ti ) override {
            if ( auto fn_beforeDim = get_beforeDim(classPtr) ) {
                invoke_beforeDim(context,fn_beforeDim,classPtr,pa,*ti);
            }
        }
        virtual void afterDim ( char * pa, TypeInfo * ti ) override {
            if ( auto fn_afterDim = get_afterDim(classPtr) ) {
                invoke_afterDim(context,fn_afterDim,classPtr,pa,*ti);
            }
        }
        virtual void beforeArray ( Array * pa, TypeInfo * ti ) override {
            if ( auto fn_beforeArray = get_beforeArray(classPtr) ) {
                invoke_beforeArray(context,fn_beforeArray,classPtr,*pa,*ti);
            }
        }
        virtual void afterArray ( Array * pa, TypeInfo * ti ) override {
            if ( auto fn_afterArray = get_afterArray(classPtr) ) {
                invoke_afterArray(context,fn_afterArray,classPtr,*pa,*ti);
            }
        }
        virtual bool canVisitTable ( char * pa, TypeInfo * ti ) override {
            if ( auto fn_canVisitTable = get_canVisitTable(classPtr) ) {
                return invoke_canVisitTable(context,fn_canVisitTable,classPtr,pa,*ti);
            } else {
                return true;
            }
        }
        virtual bool canVisitTableData ( TypeInfo * ti ) override {
            if ( auto fn_canVisitTableData = get_canVisitTableData(classPtr) ) {
                return invoke_canVisitTableData(context,fn_canVisitTableData,classPtr,*ti);
            } else {
                return true;
            }
        }
        virtual void beforeTable ( Table * pa, TypeInfo * ti ) override {
           if ( auto fn_beforeTable = get_beforeTable(classPtr) ) {
                invoke_beforeTable(context,fn_beforeTable,classPtr,*pa,*ti);
            }
        }
        virtual void beforeTableKey ( Table * pa, TypeInfo * ti, char * pk, TypeInfo * ki, uint32_t index, bool last ) override {
           if ( auto fn_beforeTableKey = get_beforeTableKey(classPtr) ) {
                invoke_beforeTableKey(context,fn_beforeTableKey,classPtr,*pa,*ti,pk,*ki,index,last);
            }
        }
        virtual void afterTableKey ( Table * pa, TypeInfo * ti, char * pk, TypeInfo * ki, uint32_t index, bool last ) override {
           if ( auto fn_afterTableKey = get_afterTableKey(classPtr) ) {
                invoke_afterTableKey(context,fn_afterTableKey,classPtr,*pa,*ti,pk,*ki,index,last);
            }
        }
        virtual void beforeTableValue ( Table * pa, TypeInfo * ti, char * pv, TypeInfo * kv, uint32_t index, bool last ) override {
           if ( auto fn_beforeTableValue = get_beforeTableValue(classPtr) ) {
                invoke_beforeTableValue(context,fn_beforeTableValue,classPtr,*pa,*ti,pv,*kv,index,last);
            }
        }
        virtual void afterTableValue ( Table * pa, TypeInfo * ti, char * pv, TypeInfo * kv, uint32_t index, bool last ) override {
           if ( auto fn_afterTableValue = get_afterTableValue(classPtr) ) {
                invoke_afterTableValue(context,fn_afterTableValue,classPtr,*pa,*ti,pv,*kv,index,last);
            }
        }
        virtual void afterTable ( Table * pa, TypeInfo * ti ) override {
           if ( auto fn_afterTable = get_afterTable(classPtr) ) {
                invoke_afterTable(context,fn_afterTable,classPtr,*pa,*ti);
            }
        }
        virtual void beforeRef ( char * pa, TypeInfo * ti ) override {
           if ( auto fn_beforeRef = get_beforeRef(classPtr) ) {
                invoke_beforeRef(context,fn_beforeRef,classPtr,pa,*ti);
            }
        }
        virtual void afterRef ( char * pa, TypeInfo * ti ) override {
           if ( auto fn_afterRef = get_afterRef(classPtr) ) {
                invoke_afterRef(context,fn_afterRef,classPtr,pa,*ti);
            }
        }
        virtual bool canVisitPointer ( TypeInfo * ti ) override {
           if ( auto fn_canVisitPointer = get_canVisitPointer(classPtr) ) {
                return invoke_canVisitPointer(context,fn_canVisitPointer,classPtr,*ti);
            } else {
                return true;
            }
        }
        virtual void beforePtr ( char * pa, TypeInfo * ti ) override {
           if ( auto fn_beforePtr = get_beforePtr(classPtr) ) {
                invoke_beforePtr(context,fn_beforePtr,classPtr,pa,*ti);
            }
        }
        virtual void afterPtr ( char * pa, TypeInfo * ti ) override {
           if ( auto fn_afterPtr = get_afterPtr(classPtr) ) {
                invoke_afterPtr(context,fn_afterPtr,classPtr,pa,*ti);
            }
        }
        virtual void beforeHandle ( char * pa, TypeInfo * ti ) override {
           if ( auto fn_beforeHandle = get_beforeHandle(classPtr) ) {
                invoke_beforeHandle(context,fn_beforeHandle,classPtr,pa,*ti);
            }
        }
        virtual void afterHandle ( char * pa, TypeInfo * ti ) override {
           if ( auto fn_afterHandle = get_afterHandle(classPtr) ) {
                invoke_afterHandle(context,fn_afterHandle,classPtr,pa,*ti);
            }
        }
        virtual bool canVisitLambda ( TypeInfo * ti ) override {
           if ( auto fn_canVisitLambda = get_canVisitLambda(classPtr) ) {
                return invoke_canVisitLambda(context,fn_canVisitLambda,classPtr,*ti);
            } else {
                return true;
            }
        }
        virtual void beforeLambda ( Lambda * pa, TypeInfo * ti ) override {
           if ( auto fn_beforeLambda = get_beforeLambda(classPtr) ) {
                invoke_beforeLambda(context,fn_beforeLambda,classPtr,*pa,*ti);
            }
        }
        virtual void afterLambda ( Lambda * pa, TypeInfo * ti ) override {
           if ( auto fn_afterLambda = get_afterLambda(classPtr) ) {
                invoke_afterLambda(context,fn_afterLambda,classPtr,*pa,*ti);
            }
        }
        virtual bool canVisitIterator ( TypeInfo * ti ) override {
           if ( auto fn_canVisitIterator = get_canVisitIterator(classPtr) ) {
                return invoke_canVisitIterator(context,fn_canVisitIterator,classPtr,*ti);
            } else {
                return true;
            }
        }
        virtual void beforeIterator ( Sequence * pa, TypeInfo * ti ) override {
           if ( auto fn_beforeIterator = get_beforeIterator(classPtr) ) {
                invoke_beforeIterator(context,fn_beforeIterator,classPtr,*pa,*ti);
            }
        }
        virtual void afterIterator ( Sequence * pa, TypeInfo * ti ) override {
           if ( auto fn_afterIterator = get_afterIterator(classPtr) ) {
                invoke_afterIterator(context,fn_afterIterator,classPtr,*pa,*ti);
            }
        }
    // types
        virtual void Null ( TypeInfo * ti ) override {
           if ( auto fn_Null = get_Null(classPtr) ) {
                invoke_Null(context,fn_Null,classPtr,*ti);
            }
        }
        virtual void VoidPtr ( void * & value ) override {
           if ( auto fn_VoidPtr = get_VoidPtr(classPtr) ) {
                invoke_VoidPtr(context,fn_VoidPtr,classPtr,value);
            }
        }
        virtual void Bool ( bool & value ) override {
           if ( auto fn_Bool = get_Bool(classPtr) ) {
                invoke_Bool(context,fn_Bool,classPtr,value);
            }
        }
        virtual void Int8 ( int8_t & value ) override {
           if ( auto fn_Int8 = get_Int8(classPtr) ) {
                invoke_Int8(context,fn_Int8,classPtr,value);
            }
        }
        virtual void UInt8 ( uint8_t & value ) override {
           if ( auto fn_UInt8 = get_UInt8(classPtr) ) {
                invoke_UInt8(context,fn_UInt8,classPtr,value);
            }
        }
        virtual void Int16 ( int16_t & value ) override {
           if ( auto fn_Int16 = get_Int16(classPtr) ) {
                invoke_Int16(context,fn_Int16,classPtr,value);
            }
        }
        virtual void UInt16 ( uint16_t & value ) override {
           if ( auto fn_UInt16 = get_UInt16(classPtr) ) {
                invoke_UInt16(context,fn_UInt16,classPtr,value);
            }
        }
        virtual void Int64 ( int64_t & value ) override {
           if ( auto fn_Int64 = get_Int64(classPtr) ) {
                invoke_Int64(context,fn_Int64,classPtr,value);
            }
        }
        virtual void UInt64 ( uint64_t & value ) override {
           if ( auto fn_UInt64 = get_UInt64(classPtr) ) {
                invoke_UInt64(context,fn_UInt64,classPtr,value);
            }
        }
        virtual void String ( char * & value ) override {
           if ( auto fn_String = get_String(classPtr) ) {
                invoke_String(context,fn_String,classPtr,value);
            }
        }
        virtual void Double ( double & value ) override {
           if ( auto fn_Double = get_Double(classPtr) ) {
                invoke_Double(context,fn_Double,classPtr,value);
            }
        }
        virtual void Float ( float & value ) override {
           if ( auto fn_Float = get_Float(classPtr) ) {
                invoke_Float(context,fn_Float,classPtr,value);
            }
        }
        virtual void Int ( int32_t & value ) override {
           if ( auto fn_Int = get_Int(classPtr) ) {
                invoke_Int(context,fn_Int,classPtr,value);
            }
        }
        virtual void UInt ( uint32_t & value ) override {
           if ( auto fn_UInt = get_UInt(classPtr) ) {
                invoke_UInt(context,fn_UInt,classPtr,value);
            }
        }
        virtual void Bitfield ( uint32_t & value, TypeInfo * ti ) override {
           if ( auto fn_Bitfield = get_Bitfield(classPtr) ) {
                invoke_Bitfield(context,fn_Bitfield,classPtr,value,*ti);
            }
        }
        virtual void Int2 ( int2 & value ) override {
           if ( auto fn_Int2 = get_Int2(classPtr) ) {
                invoke_Int2(context,fn_Int2,classPtr,value);
            }
        }
        virtual void Int3 ( int3 & value ) override {
           if ( auto fn_Int3 = get_Int3(classPtr) ) {
                invoke_Int3(context,fn_Int3,classPtr,value);
            }
        }
        virtual void Int4 ( int4 & value ) override {
           if ( auto fn_Int4 = get_Int4(classPtr) ) {
                invoke_Int4(context,fn_Int4,classPtr,value);
            }
        }
        virtual void UInt2 ( uint2 & value ) override {
           if ( auto fn_UInt2 = get_UInt2(classPtr) ) {
                invoke_UInt2(context,fn_UInt2,classPtr,value);
            }
        }
        virtual void UInt3 ( uint3 & value ) override {
           if ( auto fn_UInt3 = get_UInt3(classPtr) ) {
                invoke_UInt3(context,fn_UInt3,classPtr,value);
            }
        }
        virtual void UInt4 ( uint4 & value ) override {
           if ( auto fn_UInt4 = get_UInt4(classPtr) ) {
                invoke_UInt4(context,fn_UInt4,classPtr,value);
            }
        }
        virtual void Float2 ( float2 & value ) override {
           if ( auto fn_Float2 = get_Float2(classPtr) ) {
                invoke_Float2(context,fn_Float2,classPtr,value);
            }
        }
        virtual void Float3 ( float3 & value ) override {
           if ( auto fn_Float3 = get_Float3(classPtr) ) {
                invoke_Float3(context,fn_Float3,classPtr,value);
            }
        }
        virtual void Float4 ( float4 & value ) override {
           if ( auto fn_Float4 = get_Float4(classPtr) ) {
                invoke_Float4(context,fn_Float4,classPtr,value);
            }
        }
        virtual void Range ( range & value ) override {
           if ( auto fn_Range = get_Range(classPtr) ) {
                invoke_Range(context,fn_Range,classPtr,value);
            }
        }
        virtual void URange ( urange & value ) override {
           if ( auto fn_URange = get_URange(classPtr) ) {
                invoke_URange(context,fn_URange,classPtr,value);
            }
        }
        virtual void Range64 ( range64 & value ) override {
           if ( auto fn_Range = get_Range64(classPtr) ) {
                invoke_Range64(context,fn_Range,classPtr,value);
            }
        }
        virtual void URange64 ( urange64 & value ) override {
           if ( auto fn_URange = get_URange64(classPtr) ) {
                invoke_URange64(context,fn_URange,classPtr,value);
            }
        }
        virtual void WalkBlock ( Block * value ) override {
           if ( auto fn_WalkBlock = get_WalkBlock(classPtr) ) {
                invoke_WalkBlock(context,fn_WalkBlock,classPtr,*value);
            }
        }
        virtual void WalkFunction ( Func * value ) override {
           if ( auto fn_WalkFunction = get_WalkFunction(classPtr) ) {
                invoke_WalkFunction(context,fn_WalkFunction,classPtr,*value);
            }
        }
        virtual void WalkEnumeration ( int32_t & value, EnumInfo * ei ) override {
           if ( auto fn_WalkEnumeration = get_WalkEnumeration(classPtr) ) {
                invoke_WalkEnumeration(context,fn_WalkEnumeration,classPtr,value,*ei);
            }
        }
        virtual void WalkEnumeration8  ( int8_t & value, EnumInfo * ei ) override {
           if ( auto fn_WalkEnumeration8 = get_WalkEnumeration8(classPtr) ) {
                invoke_WalkEnumeration8(context,fn_WalkEnumeration8,classPtr,value,*ei);
            }
        }
        virtual void WalkEnumeration16 ( int16_t & value, EnumInfo * ei ) override {
           if ( auto fn_WalkEnumeration16 = get_WalkEnumeration16(classPtr) ) {
                invoke_WalkEnumeration16(context,fn_WalkEnumeration16,classPtr,value,*ei);
            }
        }
        virtual void FakeContext ( Context * value ) override {
           if ( auto fn_FakeContext = get_FakeContext(classPtr) ) {
                invoke_FakeContext(context,fn_FakeContext,classPtr,*value);
            }
        }
        virtual void invalidData ( ) override {
           if ( auto fnInvalidData = get_InvalidData(classPtr) ) {
                invoke_InvalidData(context,fnInvalidData,classPtr);
            }
        }
    protected:
        void *      classPtr;
    };

    struct AstDataWalkerAnnotation : ManagedStructureAnnotation<DataWalker,false,true> {
        AstDataWalkerAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("DataWalker", ml) {
        }
    };

    class StackWalkerAdapter : public StackWalker, DapiStackWalker_Adapter {
    public:
        StackWalkerAdapter ( char * pClass, const StructInfo * info, Context * ctx )
            : DapiStackWalker_Adapter(info), classPtr(pClass), context(ctx) {
        }
        virtual bool canWalkArguments () override {
            if ( auto fnCanWalkArguments = get_canWalkArguments(classPtr) ) {
                return invoke_canWalkArguments(context,fnCanWalkArguments,classPtr);
            } else {
                return true;
            }
        }
        virtual bool canWalkVariables () override {
            if ( auto fnCanWalkVariables = get_canWalkVariables(classPtr) ) {
                return invoke_canWalkArguments(context,fnCanWalkVariables,classPtr);
            } else {
                return true;
            }
        }
        virtual bool canWalkOutOfScopeVariables() override {
            if ( auto fnCanWalkOutOfScopeVariables = get_canWalkOutOfScopeVariables(classPtr) ) {
                return invoke_canWalkOutOfScopeVariables(context,fnCanWalkOutOfScopeVariables,classPtr);
            } else {
                return true;
            }
        }
        virtual void onBeforeCall ( Prologue * pp, char * sp ) override {
            if ( auto fnOnBeforeCall = get_onBeforeCall(classPtr) ) {
                invoke_onBeforeCall(context, fnOnBeforeCall, classPtr, *pp, sp);
            }
        }
        virtual void onCallAOT ( Prologue * pp, const char * fileName ) override {
            if ( auto fnOnCallAOT = get_onCallAOT(classPtr) ) {
                invoke_onCallAOT(context, fnOnCallAOT, classPtr, *pp, (char *)fileName);
            }
        }
        virtual void onCallAt ( Prologue * pp, FuncInfo * info, LineInfo * at ) override {
            if ( auto fnOnCallAt = get_onCallAt(classPtr) ) {
                invoke_onCallAt(context, fnOnCallAt, classPtr, *pp, *info, *at);
            }
        }
        virtual void onCall ( Prologue * pp, FuncInfo * info ) override {
            if ( auto fnOnCall = get_onCall(classPtr) ) {
                invoke_onCall(context, fnOnCall, classPtr, *pp, *info);
            }
        }
        virtual void onAfterPrologue ( Prologue * pp, char * sp ) override {
            if ( auto fnOnAfterPrologue = get_onAfterPrologue(classPtr) ) {
                invoke_onAfterPrologue(context, fnOnAfterPrologue, classPtr, *pp, sp);
            }
        }
        virtual void onArgument ( FuncInfo * info, int index, VarInfo * vinfo, vec4f arg ) override {
            if ( auto fnOnArgument = get_onArgument(classPtr) ) {
                invoke_onArgument(context, fnOnArgument, classPtr, *info, index, *vinfo, arg);
            }
        }
        virtual void onBeforeVariables ( ) override {
            if ( auto fnOnBeforeVariables = get_onBeforeVariables(classPtr) ) {
                invoke_onBeforeVariables(context, fnOnBeforeVariables, classPtr);
            }
        }
        virtual void onVariable ( FuncInfo * info, LocalVariableInfo * vinfo, void * addr, bool inScope ) override {
            if ( auto fnOnVariable = get_onVariable(classPtr) ) {
                invoke_onVariable(context, fnOnVariable, classPtr, *info, *vinfo, addr, inScope);
            }
        }
        virtual bool onAfterCall ( Prologue * pp ) override {
            if ( auto fnOnAfterCall = get_onAfterCall(classPtr) ) {
                return invoke_onAfterCall(context,fnOnAfterCall,classPtr,*pp);
            } else {
                return true;
            }
        }
    protected:
        void *      classPtr;
        Context *   context;
    };

    struct AstStackWalkerAnnotation : ManagedStructureAnnotation<StackWalker,false,true> {
        AstStackWalkerAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("StackWalker", ml) {
        }
    };


    #include "debugger.das.inc"

    StackWalkerPtr makeStackWalker ( const void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<StackWalkerAdapter>((char *)pClass,info,context);
    }

    DebugAgentPtr makeDebugAgent ( const void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<DebugAgentAdapter>((char *)pClass,info,context);
    }

    void debuggerSetContextSingleStep ( Context & context, bool step ) {
        context.setSingleStep(step);
    }

    void debuggerStackWalk ( Context & context, const LineInfo & lineInfo ) {
        context.stackWalk(&lineInfo, true, true);
    }

    DataWalkerPtr makeDataWalker ( const void * pClass, const StructInfo * info, Context * context ) {
        return make_smart<DataWalkerAdapter>((char *)pClass,info,context);
    }

    void dapiWalkData ( DataWalkerPtr walker, void * data, const TypeInfo & info ) {
        walker->walk((char *)data,(TypeInfo*)&info);
    }

    void dapiWalkDataV ( DataWalkerPtr walker, float4 data, const TypeInfo & info ) {
        walker->walk((vec4f)data,(TypeInfo*)&info);
    }

    int32_t dapiStackDepth ( Context & context ) {
    #if DAS_ENABLE_STACK_WALK
        char * sp = context.stack.ap();
        int32_t depth = 0;
        while (  sp < context.stack.top() ) {
            Prologue * pp = (Prologue *) sp;
            Block * block = nullptr;
            FuncInfo * info = nullptr;
            // char * SP = sp;
            if ( pp->info ) {
                intptr_t iblock = intptr_t(pp->block);
                if ( iblock & 1 ) {
                    block = (Block *) (iblock & ~1);
                    info = block->info;
                    // SP = context.stack.bottom() + block->stackOffset;
                } else {
                    info = pp->info;
                }
            }
            sp += info ? info->stackSize : pp->stackSize;
            depth ++;
        }
        return depth;
    #else
        context.throw_error("stack walking disabled");
        return 0;
    #endif
    }

    void dapiStackWalk ( StackWalkerPtr walker, Context & context, const LineInfo & at ) {
    #if DAS_ENABLE_STACK_WALK
        char * sp = context.stack.ap();
        const LineInfo * lineAt = &at;
        while (  sp < context.stack.top() ) {
            Prologue * pp = (Prologue *) sp;
            Block * block = nullptr;
            FuncInfo * info = nullptr;
            char * SP = sp;
            if ( pp->info ) {
                intptr_t iblock = intptr_t(pp->block);
                if ( iblock & 1 ) {
                    block = (Block *) (iblock & ~1);
                    info = block->info;
                    SP = context.stack.bottom() + block->stackOffset;
                } else {
                    info = pp->info;
                }
            }
            walker->onBeforeCall(pp,SP);
            if ( !info ) {
                walker->onCallAOT(pp,pp->fileName);
            } else if ( pp->line ) {
                walker->onCallAt(pp,info,pp->line);
            } else {
                walker->onCall(pp,info);
            }
            if ( info ) {
                if ( walker->canWalkArguments() ) {
                    for ( uint32_t i=0, is=info->count; i!=is; ++i ) {
                        walker->onArgument(info, i, info->fields[i], pp->arguments[i]);
                    }
                }
            }
            walker->onAfterPrologue(pp,SP);
            if ( info && info->locals ) {
                if ( walker->canWalkVariables() ) {
                    walker->onBeforeVariables();
                    for ( uint32_t i=0, is=info->localCount; i!=is; ++i ) {
                        auto lv = info->locals[i];
                        bool inScope = lineAt ? lineAt->inside(lv->visibility) : false;
                        if ( !walker->canWalkOutOfScopeVariables() && !inScope ) {
                            continue;
                        }
                        char * addr = nullptr;
                        if ( !inScope ) {
                            addr = nullptr;
                        } else if ( lv->cmres ) {
                            addr = (char *)pp->cmres;
                        } else if ( lv->isRefValue( ) ) {
                            addr = SP + lv->stackTop;
                        } else {
                            addr = SP + lv->stackTop;
                        }
                        walker->onVariable(info, lv, addr, inScope);
                    }
                }
            }
            lineAt = info ? pp->line : nullptr;
            sp += info ? info->stackSize : pp->stackSize;
            if ( !walker->onAfterCall(pp) ) break;
        }
    #else
        context.throw_error("stack walking disabled");
    #endif
    }

    // pinvoke(context,"function",....)

    vec4f pinvoke_impl ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        auto invCtx = cast<Context *>::to(args[0]);
        if ( !invCtx ) context.throw_error_at(call->debugInfo, "pinvoke with null context");
        auto fn = cast<const char *>::to(args[1]);
        if ( !fn ) context.throw_error_at(call->debugInfo, "can't pinvoke empty string");
        if ( !invCtx->contextMutex ) context.throw_error_at(call->debugInfo,"threadlock_context is not set");
        vec4f res = v_zero();
        LineInfo exAt;
        string exText;
        invCtx->threadlock_context([&](){
            auto simFn = invCtx->findFunction(fn);
            if ( !simFn ) {
                exAt = call->debugInfo;
                exText = string("pinvoke can't find ") + fn + " function";
                return;
            }
            if ( simFn->debugInfo->count!=uint32_t(call->nArguments-2) ) {
                exAt = call->debugInfo;
                exText = string("pinvoke ") + fn + " function expects " + to_string(simFn->debugInfo->count)
                    + " arguments, but " + to_string(call->nArguments-2) + " provided";
                return;
            }
            invCtx->exception = nullptr;
            invCtx->runWithCatch([&](){
                if ( !invCtx->ownStack ) {
                    StackAllocator sharedStack(8*1024);
                    SharedStackGuard guard(*invCtx, sharedStack);
                    res = invCtx->callOrFastcall(simFn, args+2, &call->debugInfo);
                } else {
                    res = invCtx->callOrFastcall(simFn, args+2, &call->debugInfo);
                }
            });
            if ( invCtx->exception ) {
                exAt = invCtx->exceptionAt;
                exText = invCtx->exception;
            }
        });
        if ( !exText.empty() ) context.throw_error_at(exAt, "%s", exText.c_str());
        return res;
    }

    vec4f pinvoke_impl2 ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        auto invCtx = cast<Context *>::to(args[0]);
        if ( !invCtx ) context.throw_error_at(call->debugInfo, "pinvoke with null context");
        auto fn = cast<Func>::to(args[1]);
        if ( !fn.PTR ) context.throw_error_at(call->debugInfo, "pnvoke can't invoke null function");
        auto simFn = fn.PTR;
        if ( !simFn ) context.throw_error_at(call->debugInfo, "pinvoke can't find function #%p", (void *)simFn);
        if ( !invCtx->contextMutex ) context.throw_error_at(call->debugInfo,"threadlock_context is not set");
        if ( simFn->debugInfo->count!=uint32_t(call->nArguments-2) ) context.throw_error_at(call->debugInfo,
            "pinvoke function expects %u arguments, but %u provided", simFn->debugInfo->count, call->nArguments-2);
        vec4f res = v_zero();
        LineInfo exAt;
        string exText;
        invCtx->threadlock_context([&](){
            invCtx->exception = nullptr;
            invCtx->runWithCatch([&](){
                if ( !invCtx->ownStack ) {
                    StackAllocator sharedStack(8*1024);
                    SharedStackGuard guard(*invCtx, sharedStack);
                    res = invCtx->callOrFastcall(simFn, args+2, &call->debugInfo);
                } else {
                    res = invCtx->callOrFastcall(simFn, args+2, &call->debugInfo);
                }
            });
            if ( invCtx->exception ) {
                exAt = invCtx->exceptionAt;
                exText = invCtx->exception;
            }
        });
        if ( !exText.empty() ) context.throw_error_at(exAt, "%s", exText.c_str());
        return res;
    }

    vec4f pinvoke_impl3 ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        auto invCtx = cast<Context *>::to(args[0]);
        if ( !invCtx ) context.throw_error_at(call->debugInfo, "pinvoke with null context");
        auto fn = cast<Lambda>::to(args[1]);
        SimFunction ** fnMnh = (SimFunction **)fn.capture;
        if (!fnMnh) context.throw_error_at(call->debugInfo, "invoke null lambda");
        SimFunction * simFn = *fnMnh;
        if ( !simFn ) context.throw_error_at(call->debugInfo, "pinvoke can't find function #%p", (void*)simFn);
        if ( simFn->debugInfo->count!=uint32_t(call->nArguments-1) ) context.throw_error_at(call->debugInfo,
            "pinvoke function expects %u arguments, but %u provided", simFn->debugInfo->count, call->nArguments-2);
        if ( !invCtx->contextMutex ) context.throw_error_at(call->debugInfo,"threadlock_context is not set");
        vec4f res = v_zero();
        LineInfo exAt;
        string exText;
        invCtx->threadlock_context([&](){
            invCtx->exception = nullptr;
            invCtx->runWithCatch([&](){
                if ( !invCtx->ownStack ) {
                    StackAllocator sharedStack(8*1024);
                    SharedStackGuard guard(*invCtx, sharedStack);
                    res = invCtx->callOrFastcall(simFn, args+1, &call->debugInfo);
                } else {
                    res = invCtx->callOrFastcall(simFn, args+1, &call->debugInfo);
                }
            });
            if ( invCtx->exception ) {
                exAt = invCtx->exceptionAt;
                exText = invCtx->exception;
            }
        });
        if ( !exText.empty() ) context.throw_error_at(exAt, "%s", exText.c_str());
        return res;
    }

    // invoke_debug_agent_method("category","function",....)

    extern std::recursive_mutex g_DebugAgentMutex;
    extern das_safe_map<string, DebugAgentInstance>   g_DebugAgents;

#define MAX_DEBUG_AGENT_ARGS 16

    vec4f invokeInDebugAgent ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        if ( call->nArguments>=MAX_DEBUG_AGENT_ARGS-3 ) context.throw_error_at(call->debugInfo, "too many arguments");
        const char * category = cast<char *>::to(args[0]);
        if ( !category ) context.throw_error_at(call->debugInfo, "need to specify category");
        const char * function_name = cast<char *>::to(args[1]);
        if ( !function_name ) context.throw_error_at(call->debugInfo, "need to specify method name");
        g_DebugAgentMutex.lock();
        auto it = g_DebugAgents.find(category);
        if ( it == g_DebugAgents.end() ) {
            g_DebugAgentMutex.unlock();
            context.throw_error_at(call->debugInfo, "can't get debug agent '%s'", category);
        }
        auto invCtx = it->second.debugAgentContext.get();
        if ( !invCtx ) {
            g_DebugAgentMutex.unlock();
            context.throw_error_at(call->debugInfo, "debug agent '%s' is a CPP-only agent", category);
        }
        Func * func = nullptr;
        auto adapter = (DebugAgentAdapter *) it->second.debugAgent.get();
        auto sinfo = adapter->classInfo;
        for ( uint32_t fi=0, fis=sinfo->count; fi!=fis; ++fi ) {
            auto field = sinfo->fields[fi];
            if ( strcmp(field->name, function_name)==0 ) {
                func = (Func *) ( ((char *) adapter->classPtr) + field->offset );
                vec4f args2[MAX_DEBUG_AGENT_ARGS];
                args2[0] = cast<Context *>::from(invCtx);
                args2[1] = cast<Func>::from(*func);
                args2[2] = cast<void *>::from(adapter->classPtr);
                for ( int32_t ai=2, ais=call->nArguments; ai!=ais; ++ai ) {
                    args2[ai + 1] = args[ai];
                }
                auto result = pinvoke_impl2(context, call, args2);
                g_DebugAgentMutex.unlock();
                return result;
            }
        }

        g_DebugAgentMutex.unlock();
        context.throw_error_at(call->debugInfo, "debug agent '%s' is a CPP-only agent", category);
        return v_zero();
    }

    // invoke_debug_agent_function("category","function",....)

    vec4f invokeInDebugAgent2 ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        if ( call->nArguments>=MAX_DEBUG_AGENT_ARGS-3 ) context.throw_error_at(call->debugInfo, "too many arguments");
        const char * category = cast<char *>::to(args[0]);
        if ( !category ) context.throw_error_at(call->debugInfo, "need to specify category");
        const char * function_name = cast<char *>::to(args[1]);
        if ( !function_name ) context.throw_error_at(call->debugInfo, "need to specify method name");
        g_DebugAgentMutex.lock();
        auto it = g_DebugAgents.find(category);
        if ( it == g_DebugAgents.end() ) {
            g_DebugAgentMutex.unlock();
            context.throw_error_at(call->debugInfo, "can't get debug agent '%s'", category);
        }
        auto invCtx = it->second.debugAgentContext.get();
        if ( !invCtx ) {
            g_DebugAgentMutex.unlock();
            context.throw_error_at(call->debugInfo, "debug agent '%s' is a CPP-only agent", category);
        }
        Func func;
        func.PTR = invCtx->findFunction(function_name);
        if ( !func.PTR ) {
            g_DebugAgentMutex.unlock();
            context.throw_error_at(call->debugInfo, "function '%s' not found in debug agent '%s'", function_name, category);
        }
        auto adapter = (DebugAgentAdapter *) it->second.debugAgent.get();
        vec4f args2[MAX_DEBUG_AGENT_ARGS];
        args2[0] = cast<Context *>::from(invCtx);
        args2[1] = cast<Func>::from(func);
        args2[2] = cast<void *>::from(adapter->classPtr);
        for ( int32_t ai=2, ais=call->nArguments; ai!=ais; ++ai ) {
            args2[ai + 1] = args[ai];
        }
        auto result = pinvoke_impl2(context, call, args2);
        g_DebugAgentMutex.unlock();
        return result;
    }

    vec4f get_global_variable ( Context & context, SimNode_CallBase * node, vec4f * args ) {
        auto ctx = cast<Context *>::to(args[0]);
        if ( ctx==nullptr ) context.throw_error_at(node->debugInfo, "expecting context pointer");
        auto name = cast<char *>::to(args[1]);
        auto vidx = ctx->findVariable(name);
        return cast<void *>::from(ctx->getVariable(vidx));
    }

    vec4f get_global_variable_by_index ( Context & context, SimNode_CallBase * node, vec4f * args ) {
        auto ctx = cast<Context *>::to(args[0]);
        if ( ctx==nullptr ) context.throw_error_at(node->debugInfo, "expecting context pointer");
        auto vidx = cast<int32_t>::to(args[1]);
        return cast<void *>::from(ctx->getVariable(vidx));
    }

    void instrument_context ( Context & ctx, bool isInstrumenting, const TBlock<bool,LineInfo> & blk, Context * context, LineInfoArg * line ) {
        ctx.instrumentContextNode(blk, isInstrumenting, context, line);
    }

    void instrument_function ( Context & ctx, Func fn, bool isInstrumenting, uint64_t userData, Context * context, LineInfoArg * arg ) {
        if ( !fn ) context->throw_error_at(arg, "expecting function");
        ctx.instrumentFunction(fn.PTR, isInstrumenting, userData, false);
    }

    void instrument_all_functions ( Context & ctx ) {
        ctx.instrumentFunction(0, true, 0ul, false);
    }

    void instrument_all_functions_thread_local ( Context & ctx ) {
        ctx.instrumentFunction(0, true, 0ul, true);
    }

    void instrument_all_functions_ex ( Context & ctx, const TBlock<uint64_t,Func,const SimFunction *> & blk, Context * context, LineInfoArg * arg ) {
        for ( int fni=0, fnis=ctx.getTotalFunctions(); fni!=fnis; ++fni ) {
            Func fn;
            fn.PTR = ctx.getFunction(fni);
            uint64_t userData = das_invoke<uint64_t>::invoke(context,arg,blk,fn,fn.PTR);
            ctx.instrumentFunction(fn.PTR, true, userData, false);
        }
    }

    void instrument_all_functions_thread_local_ex ( Context & ctx, const TBlock<uint64_t,Func,const SimFunction *> & blk, Context * context, LineInfoArg * arg ) {
        for ( int fni=0, fnis=ctx.getTotalFunctions(); fni!=fnis; ++fni ) {
            Func fn;
            fn.PTR = ctx.getFunction(fni);
            uint64_t userData = das_invoke<uint64_t>::invoke(context,arg,blk,fn,fn.PTR);
            ctx.instrumentFunction(fn.PTR, true, userData, true);
        }
    }

    void clear_instruments ( Context & ctx ) {
        ctx.clearInstruments();
    }

    bool has_function ( Context & ctx, const char * name ) {
        return ctx.findFunction(name ? name : "") != nullptr;
    }

    Context * hw_bp_context[DAS_MAX_HW_BREAKPOINTS] = {
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    };
    bool hw_bp_handler_set = false;

    void hw_bp_handler ( int bp, void * address ) {
        Context * ctx = hw_bp_context[bp];
        if ( ctx!=nullptr ) ctx->triggerHwBreakpoint(address, bp);
    }

    int32_t set_hw_breakpoint ( Context & ctx, void * address, int32_t size, bool writeOnly ) {
        HwBpSize sz;
        switch ( size ) {
        case 1: sz = HwBpSize::HwBp_1; break;
        case 2: sz = HwBpSize::HwBp_2; break;
        case 3: case 4: sz = HwBpSize::HwBp_4; break;
        case 5: case 6: case 7: case 8: sz = HwBpSize::HwBp_8; break;
        default:    return -1;
        }
        if ( !hw_bp_handler_set ) hwSetBreakpointHandler(hw_bp_handler);
        int32_t bp = hwBreakpointSet(address, sz, writeOnly ? HwBpType::HwBp_Write : HwBpType::HwBp_ReadWrite);
        if ( bp!=-1 ) hw_bp_context[bp] = &ctx;
        return bp;
    }

    bool clear_hw_breakpoint ( int32_t bpi ) {
        if ( hwBreakpointClear(bpi) ) {
            hw_bp_context[bpi] = nullptr;
            return true;
        } else {
            return false;
        }
    }

    void break_on_free ( Context & ctx, void * ptr, uint32_t size ) {
        ctx.heap->breakOnFree(ptr,size);
    }

    class Module_Debugger : public Module {
    public:
        Module_Debugger() : Module("debugapi") {
            DAS_PROFILE_SECTION("Module_Debugger");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            addBuiltinDependency(lib, Module::require("rtti"));
            // annotations
            addAnnotation(make_smart<PrologueAnnotation>(lib));
            addAnnotation(make_smart<AstDebugAgentAnnotation>(lib));
            addAnnotation(make_smart<AstDataWalkerAnnotation>(lib));
            addAnnotation(make_smart<AstStackWalkerAnnotation>(lib));
            // debug agent
            addExtern<DAS_BIND_FUN(makeDebugAgent)>(*this, lib,  "make_debug_agent",
                SideEffects::modifyExternal, "makeDebugAgent")
                    ->args({"class","info","context"});
            addExtern<DAS_BIND_FUN(tickDebugAgent)>(*this, lib,  "tick_debug_agent",
                SideEffects::modifyExternal, "tickDebugAgent");
            addExtern<DAS_BIND_FUN(tickSpecificDebugAgent)>(*this, lib,  "tick_debug_agent",
                SideEffects::modifyExternal, "tickSpecificDebugAgent")
                    ->arg("agent");
            addExtern<DAS_BIND_FUN(collectDebugAgentState)>(*this, lib,  "collect_debug_agent_state",
                SideEffects::modifyExternal, "collectDebugAgentState")
                    ->args({"context","at"});
            addExtern<DAS_BIND_FUN(onBreakpointsReset)>(*this, lib,  "on_breakpoints_reset",
                SideEffects::modifyExternal, "onBreakpointsReset")
                ->args({"file", "breakpointsNum"});
            addExtern<DAS_BIND_FUN(installThreadLocalDebugAgent)>(*this, lib,  "install_debug_agent_thread_local",
                SideEffects::modifyExternal, "installThreadLocalDebugAgent")
                    ->args({"agent","line","context"});
            addExtern<DAS_BIND_FUN(installDebugAgent)>(*this, lib,  "install_debug_agent",
                SideEffects::modifyExternal, "installDebugAgent")
                    ->args({"agent","category","line","context"});
            addExtern<DAS_BIND_FUN(getDebugAgentContext), SimNode_ExtFuncCallRef>(*this, lib,  "get_debug_agent_context",
                SideEffects::modifyExternal, "getDebugAgentContext")
                    ->args({"category","line","context"});
            addExtern<DAS_BIND_FUN(hasDebugAgentContext)>(*this, lib,  "has_debug_agent_context",
                SideEffects::modifyExternal, "hasDebugAgentContext")
                    ->args({"category","line","context"});;
            addExtern<DAS_BIND_FUN(forkDebugAgentContext)>(*this, lib,  "fork_debug_agent_context",
                SideEffects::modifyExternal, "forkDebugAgentContext")
                    ->args({"function","context","line"});;
            addExtern<DAS_BIND_FUN(isInDebugAgentCreation)>(*this, lib, "is_in_debug_agent_creation",
                SideEffects::accessExternal, "isInDebugAgentCreation");
            addExtern<DAS_BIND_FUN(debuggerSetContextSingleStep)>(*this, lib,  "set_single_step",
                SideEffects::modifyExternal, "debuggerSetContextSingleStep")
                    ->args({"context","enabled"});
            addExtern<DAS_BIND_FUN(debuggerStackWalk)>(*this, lib, "stackwalk",
                SideEffects::modifyExternal, "debuggerStackWalk")
                    ->args({"context","line"});
            addExtern<DAS_BIND_FUN(dapiReportContextState)>(*this, lib, "report_context_state",
                SideEffects::modifyExternal, "dapiReportContextState")
                    ->args({"context","category","name","info","data"});
            // instrumentation
            addExtern<DAS_BIND_FUN(instrument_context)>(*this, lib,  "instrument_node",
                SideEffects::modifyExternal, "instrument_context")
                    ->args({"context","isInstrumenting","block","context","line"});
            addExtern<DAS_BIND_FUN(instrument_function)>(*this, lib,  "instrument_function",
                SideEffects::modifyExternal, "instrument_function")
                    ->args({"context","function","isInstrumenting","userData","context","line"});;
            addExtern<DAS_BIND_FUN(instrument_all_functions)>(*this, lib,  "instrument_all_functions",
                SideEffects::modifyExternal, "instrument_all_functions")
                    ->arg("context");
            addExtern<DAS_BIND_FUN(instrument_all_functions_thread_local)>(*this, lib,  "instrument_all_functions_thread_local",
                SideEffects::modifyExternal, "instrument_all_functions_thread_local")
                    ->arg("context");
            addExtern<DAS_BIND_FUN(instrument_all_functions_ex)>(*this, lib,  "instrument_all_functions",
                SideEffects::modifyExternal|SideEffects::invoke, "instrument_all_functions_ex")
                    ->args({"ctx","block","context","line"});
            addExtern<DAS_BIND_FUN(instrument_all_functions_thread_local_ex)>(*this, lib,  "instrument_all_functions_thread_local",
                SideEffects::modifyExternal|SideEffects::invoke, "instrument_all_functions_thread_local_ex")
                    ->args({"ctx","block","context","line"});
            addExtern<DAS_BIND_FUN(clear_instruments)>(*this, lib,  "clear_instruments",
                SideEffects::modifyExternal, "clear_instruments")
                    ->arg("context");
            // data walker
            addExtern<DAS_BIND_FUN(makeDataWalker)>(*this, lib,  "make_data_walker",
                SideEffects::modifyExternal, "makeDataWalker")
                    ->args({"class","info","context"});
            addExtern<DAS_BIND_FUN(dapiWalkData)>(*this, lib,  "walk_data",
                SideEffects::modifyExternal, "dapiWalkData")
                    ->args({"walker","data","info"});
            addExtern<DAS_BIND_FUN(dapiWalkDataV)>(*this, lib,  "walk_data",
                SideEffects::modifyExternal, "dapiWalkDataV")
                    ->args({"walker","data","info"});
            // stack walker
            addExtern<DAS_BIND_FUN(makeStackWalker)>(*this, lib,  "make_stack_walker",
                SideEffects::modifyExternal, "makeStackWalker")
                    ->args({"class","info","context"});
            addExtern<DAS_BIND_FUN(dapiStackWalk)>(*this, lib,  "walk_stack",
                SideEffects::modifyExternal, "dapiStackWalk")
                    ->args({"walker","context","line"});
            addExtern<DAS_BIND_FUN(dapiStackDepth)>(*this, lib,  "stack_depth",
                SideEffects::modifyExternal, "dapiStacDepth")
                    ->arg("context");
            // global variable
            addInterop<get_global_variable,void *,vec4f,const char *>(*this,lib,"get_context_global_variable",
                SideEffects::accessExternal,"get_global_variable")
                    ->args({"context","name"})->unsafeOperation = true;
            addInterop<get_global_variable_by_index,void *,vec4f,int32_t>(*this,lib,"get_context_global_variable",
                SideEffects::accessExternal,"get_global_variable")
                    ->args({"context","index"})->unsafeOperation = true;
            // has function
            addExtern<DAS_BIND_FUN(has_function)>(*this, lib,  "has_function",
                SideEffects::none, "has_function")
                    ->args({"context","function_name"});
            // pinvoke
            addInterop<pinvoke_impl,void,vec4f,const char *>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl")->unsafeOperation = true;
            addInterop<pinvoke_impl,void,vec4f,const char *,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl")->unsafeOperation = true;
            addInterop<pinvoke_impl,void,vec4f,const char *,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl")->unsafeOperation = true;
            addInterop<pinvoke_impl,void,vec4f,const char *,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl")->unsafeOperation = true;
            addInterop<pinvoke_impl,void,vec4f,const char *,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl")->unsafeOperation = true;
            addInterop<pinvoke_impl,void,vec4f,const char *,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl")->unsafeOperation = true;
            addInterop<pinvoke_impl,void,vec4f,const char *,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl")->unsafeOperation = true;
            addInterop<pinvoke_impl,void,vec4f,const char *,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl")->unsafeOperation = true;
            addInterop<pinvoke_impl,void,vec4f,const char *,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl")->unsafeOperation = true;
            addInterop<pinvoke_impl,void,vec4f,const char *,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl")->unsafeOperation = true;
            addInterop<pinvoke_impl,void,vec4f,const char *,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl")->unsafeOperation = true;
            // pinvoke2
            addInterop<pinvoke_impl2,void,vec4f,Func>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl2")->unsafeOperation = true;
            addInterop<pinvoke_impl2,void,vec4f,Func,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl2")->unsafeOperation = true;
            addInterop<pinvoke_impl2,void,vec4f,Func,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl2")->unsafeOperation = true;
            addInterop<pinvoke_impl2,void,vec4f,Func,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl2")->unsafeOperation = true;
            addInterop<pinvoke_impl2,void,vec4f,Func,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl2")->unsafeOperation = true;
            addInterop<pinvoke_impl2,void,vec4f,Func,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl2")->unsafeOperation = true;
            addInterop<pinvoke_impl2,void,vec4f,Func,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl2")->unsafeOperation = true;
            addInterop<pinvoke_impl2,void,vec4f,Func,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl2")->unsafeOperation = true;
            addInterop<pinvoke_impl2,void,vec4f,Func,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl2")->unsafeOperation = true;
            addInterop<pinvoke_impl2,void,vec4f,Func,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl2")->unsafeOperation = true;
            addInterop<pinvoke_impl2,void,vec4f,Func,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl2")->unsafeOperation = true;
            // pinvoke3
            addInterop<pinvoke_impl3,void,vec4f,Lambda>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl3")->unsafeOperation = true;
            addInterop<pinvoke_impl3,void,vec4f,Lambda,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl3")->unsafeOperation = true;
            addInterop<pinvoke_impl3,void,vec4f,Lambda,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl3")->unsafeOperation = true;
            addInterop<pinvoke_impl3,void,vec4f,Lambda,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl3")->unsafeOperation = true;
            addInterop<pinvoke_impl3,void,vec4f,Lambda,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl3")->unsafeOperation = true;
            addInterop<pinvoke_impl3,void,vec4f,Lambda,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl3")->unsafeOperation = true;
            addInterop<pinvoke_impl3,void,vec4f,Lambda,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl3")->unsafeOperation = true;
            addInterop<pinvoke_impl3,void,vec4f,Lambda,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl3")->unsafeOperation = true;
            addInterop<pinvoke_impl3,void,vec4f,Lambda,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl3")->unsafeOperation = true;
            addInterop<pinvoke_impl3,void,vec4f,Lambda,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl3")->unsafeOperation = true;
            addInterop<pinvoke_impl3,void,vec4f,Lambda,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_in_context",
                SideEffects::worstDefault,"pinvoke_impl3")->unsafeOperation = true;
            // invoke debug agent function (local to da context)
            addInterop<invokeInDebugAgent,void,char *,char *>(*this,lib,"invoke_debug_agent_method",
                SideEffects::worstDefault,"invokeInDebugAgent")->unsafeOperation = true;
            addInterop<invokeInDebugAgent,void,char *,char *,vec4f>(*this,lib,"invoke_debug_agent_method",
                SideEffects::worstDefault,"invokeInDebugAgent")->unsafeOperation = true;
            addInterop<invokeInDebugAgent,void,char *,char *,vec4f,vec4f>(*this,lib,"invoke_debug_agent_method",
                SideEffects::worstDefault,"invokeInDebugAgent")->unsafeOperation = true;
            addInterop<invokeInDebugAgent,void,char *,char *,vec4f,vec4f,vec4f>(*this,lib,"invoke_debug_agent_method",
                SideEffects::worstDefault,"invokeInDebugAgent")->unsafeOperation = true;
            addInterop<invokeInDebugAgent,void,char *,char *,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_debug_agent_method",
                SideEffects::worstDefault,"invokeInDebugAgent")->unsafeOperation = true;
            addInterop<invokeInDebugAgent,void,char *,char *,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_debug_agent_method",
                SideEffects::worstDefault,"invokeInDebugAgent")->unsafeOperation = true;
            addInterop<invokeInDebugAgent,void,char *,char *,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_debug_agent_method",
                SideEffects::worstDefault,"invokeInDebugAgent")->unsafeOperation = true;
            addInterop<invokeInDebugAgent,void,char *,char *,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_debug_agent_method",
                SideEffects::worstDefault,"invokeInDebugAgent")->unsafeOperation = true;
            addInterop<invokeInDebugAgent,void,char *,char *,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_debug_agent_method",
                SideEffects::worstDefault,"invokeInDebugAgent")->unsafeOperation = true;
            addInterop<invokeInDebugAgent,void,char *,char *,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_debug_agent_method",
                SideEffects::worstDefault,"invokeInDebugAgent")->unsafeOperation = true;
            addInterop<invokeInDebugAgent,void,char *,char *,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_debug_agent_method",
                SideEffects::worstDefault,"invokeInDebugAgent")->unsafeOperation = true;
            // invoke debug agent function (local to da context)
            addInterop<invokeInDebugAgent2,void,char *,char *>(*this,lib,"invoke_debug_agent_function",
                SideEffects::worstDefault,"invokeInDebugAgent2")->unsafeOperation = true;
            addInterop<invokeInDebugAgent2,void,char *,char *,vec4f>(*this,lib,"invoke_debug_agent_function",
                SideEffects::worstDefault,"invokeInDebugAgent2")->unsafeOperation = true;
            addInterop<invokeInDebugAgent2,void,char *,char *,vec4f,vec4f>(*this,lib,"invoke_debug_agent_function",
                SideEffects::worstDefault,"invokeInDebugAgent2")->unsafeOperation = true;
            addInterop<invokeInDebugAgent2,void,char *,char *,vec4f,vec4f,vec4f>(*this,lib,"invoke_debug_agent_function",
                SideEffects::worstDefault,"invokeInDebugAgent2")->unsafeOperation = true;
            addInterop<invokeInDebugAgent2,void,char *,char *,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_debug_agent_function",
                SideEffects::worstDefault,"invokeInDebugAgent2")->unsafeOperation = true;
            addInterop<invokeInDebugAgent2,void,char *,char *,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_debug_agent_function",
                SideEffects::worstDefault,"invokeInDebugAgent2")->unsafeOperation = true;
            addInterop<invokeInDebugAgent2,void,char *,char *,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_debug_agent_function",
                SideEffects::worstDefault,"invokeInDebugAgent2")->unsafeOperation = true;
            addInterop<invokeInDebugAgent2,void,char *,char *,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_debug_agent_function",
                SideEffects::worstDefault,"invokeInDebugAgent2")->unsafeOperation = true;
            addInterop<invokeInDebugAgent2,void,char *,char *,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_debug_agent_function",
                SideEffects::worstDefault,"invokeInDebugAgent2")->unsafeOperation = true;
            addInterop<invokeInDebugAgent2,void,char *,char *,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_debug_agent_function",
                SideEffects::worstDefault,"invokeInDebugAgent2")->unsafeOperation = true;
            addInterop<invokeInDebugAgent2,void,char *,char *,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f,vec4f>(*this,lib,"invoke_debug_agent_function",
                SideEffects::worstDefault,"invokeInDebugAgent2")->unsafeOperation = true;
            // lock debug context
            addExtern<DAS_BIND_FUN(lockDebugAgent)>(*this, lib, "lock_debug_agent",
                SideEffects::modifyExternal, "lockDebugAgent")
                    ->args({"block","context","line"})->unsafeOperation = true;
            // hw breakpoints
            addExtern<DAS_BIND_FUN(set_hw_breakpoint)>(*this, lib,  "set_hw_breakpoint",
                SideEffects::modifyExternal, "set_hw_breakpoint")
                    ->args({"context","address","size","writeOnly"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(clear_hw_breakpoint)>(*this, lib,  "clear_hw_breakpoint",
                SideEffects::modifyExternal, "clear_hw_breakpoint")->unsafeOperation = true;
            // heap
            addExtern<DAS_BIND_FUN(heap_stats)>(*this, lib, "get_heap_stats",
                SideEffects::modifyArgumentAndAccessExternal, "heap_stats")
                    ->args({"context","bytes"})->unsafeOperation = true;
            // heap debugger
            addExtern<DAS_BIND_FUN(break_on_free)>(*this, lib, "break_on_free",
                SideEffects::modifyArgumentAndAccessExternal, "break_on_free")
                    ->args({"context","ptr","size"})->unsafeOperation = true;
            // add builtin module
            compileBuiltinModule("debugger.das",debugger_das,sizeof(debugger_das));
            // lets make sure its all aot ready
            verifyAotReady();
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            tw << "#include \"daScript/simulate/aot_builtin_debugger.h\"\n";
            return ModuleAotType::cpp;
        }
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_Debugger,das);

