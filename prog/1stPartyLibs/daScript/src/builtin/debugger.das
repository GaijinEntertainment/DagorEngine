options indenting = 4
options remove_unused_symbols = false
options no_aot = true
options strict_smart_pointers = true

require debugapi

require rtti public

class DapiDebugAgent
    def abstract onInstall ( agent:DebugAgent? ) : void
    def abstract onUninstall ( agent:DebugAgent? ) : void
    def abstract onCreateContext ( var ctx:Context ) : void
    def abstract onDestroyContext ( var ctx:Context ) : void
    def abstract onSimulateContext ( var ctx:Context ) : void
    def abstract onSingleStep ( var ctx:Context; at:LineInfo ) : void
    def abstract onInstrument ( var ctx:Context; at:LineInfo ) : void
    def abstract onInstrumentFunction ( var ctx:Context; fn:SimFunction?; entering:bool; userData:uint64 ) : void
    def abstract onBreakpoint ( var ctx:Context; at:LineInfo; reason,text:string ) : void
    def abstract onVariable ( var ctx:Context; category,name:string; info:TypeInfo; data:void? ) : void
    def abstract onBreakpointsReset ( file:string; breakpointsNum:int ) : void
    def abstract onTick : void
    def abstract onCollect ( var ctx:Context; at:LineInfo ) : void
    def abstract onLog ( context : Context?; at : LineInfo const?; level:int; text:string# ) : bool
    def abstract onBeforeGC ( var ctx:Context ) : void
    def abstract onAfterGC ( var ctx:Context ) : void
    def abstract onUserCommand ( command : string# ) : bool
    def abstract onAllocate ( var ctx:Context; data:void?; size:uint64; at:LineInfo ) : void
    def abstract onReallocate ( var ctx:Context; data:void?; size:uint64; newData:void?; newSize:uint64; at:LineInfo ) : void
    def abstract onFree ( var ctx:Context; data:void?; at:LineInfo ) : void
    def abstract onAllocateString ( var ctx:Context; data:void?; size:uint64; at:LineInfo ) : void
    def abstract onFreeString ( var ctx:Context; data:void?; at:LineInfo ) : void
    [[do_not_delete]] thisAgent:DebugAgent?

def install_new_debug_agent ( var agentPtr; category:string )
    static_if typeinfo(is_class *agentPtr)
        let agentInfo = class_info(*agentPtr)
        var inscope agentAdapter <- make_debug_agent(agentPtr,agentInfo)
        unsafe
            agentPtr.thisAgent = reinterpret<DebugAgent?>(agentAdapter)
        install_debug_agent(agentAdapter,category)
        this_context().name := "debug agent {category}"
    else
        concept_assert(false,"can't make debug agent of non-class")

def install_new_thread_local_debug_agent ( var agentPtr )
    static_if typeinfo(is_class *agentPtr)
        let agentInfo = class_info(*agentPtr)
        var inscope agentAdapter <- make_debug_agent(agentPtr,agentInfo)
        unsafe
            agentPtr.thisAgent = reinterpret<DebugAgent?>(agentAdapter)
        install_debug_agent_thread_local(agentAdapter)
        this_context().name := "thread local debug agent"
    else
        concept_assert(false,"can't make debug agent of non-class")

// das::Array. should we bind C++ structure?
struct DapiArray
    data : void?
    size : uint
    capacity : uint
    lock : uint
    flags : bitfield<_shared;_hopeless>

// das::Table. should we bind C++ structure?
struct DapiTable : DapiArray
    keys : void?
    hashes : uint?

// das::Block
struct DapiBlock
    stackOffset : uint
    argumentsOffset : uint
    body : void?                    // SimNode *
    aotFunction : void?
    functionArguments : float4?
    info: FuncInfo?

// das::Func
struct DapiFunc
    index : int

// das::Lambda
struct DapiLambda
    capture : void?

// das::Sequence
struct DapiSequence
    iter : void?        // Iterator *

class DapiDataWalker
    def abstract canVisitHandle ( ps:void?; ti:TypeInfo ) : bool
    def abstract canVisitStructure ( ps:void?; si:StructInfo ) : bool
    def abstract canVisitArray ( pa:void?; ti:TypeInfo ) : bool
    def abstract canVisitArrayData ( ti:TypeInfo; count:uint ) : bool
    def abstract canVisitTuple ( ps:void?; ti:TypeInfo ) : bool
    def abstract canVisitVariant ( ps:void?; ti:TypeInfo ) : bool
    def abstract canVisitTable ( ps:void?; ti:TypeInfo ) : bool
    def abstract canVisitTableData ( ti:TypeInfo ) : bool
    def abstract canVisitPointer ( ti:TypeInfo ) : bool
    def abstract canVisitLambda ( ti:TypeInfo ) : bool
    def abstract canVisitIterator ( ti:TypeInfo ) : bool
    def abstract beforeStructure ( ps:void?; si:StructInfo ) : void
    def abstract afterStructure ( ps:void?; si:StructInfo ) : void
    def abstract afterStructureCancel ( ps:void?; si:StructInfo ) : void
    def abstract beforeStructureField ( ps:void?; si:StructInfo; pv:void?; vi:VarInfo; last:bool ) : void
    def abstract afterStructureField ( ps:void?; si:StructInfo; pv:void?; vi:VarInfo; last:bool ) : void
    def abstract beforeTuple ( ps:void?; ti:TypeInfo ) : void
    def abstract afterTuple ( ps:void?; ti:TypeInfo ) : void
    def abstract beforeTupleEntry ( ps:void?; ti:TypeInfo; pv:void?; vi:TypeInfo; last:bool ) : void
    def abstract afterTupleEntry ( ps:void?; ti:TypeInfo; pv:void?; vi:TypeInfo; last:bool ) : void
    def abstract beforeVariant ( ps:void?; ti:TypeInfo ) : void
    def abstract afterVariant ( ps:void?; ti:TypeInfo ) : void
    def abstract beforeArrayData ( ps:void?; stride:uint; count:uint; ti:TypeInfo ) : void
    def abstract afterArrayData ( ps:void?; stride:uint; count:uint; ti:TypeInfo ) : void
    def abstract beforeArrayElement ( ps:void?; ti:TypeInfo; pe:void?; index:uint; last:bool ) : void
    def abstract afterArrayElement ( ps:void?; ti:TypeInfo; pe:void?; index:uint; last:bool ) : void
    def abstract beforeDim ( ps:void?; ti:TypeInfo ) : void
    def abstract afterDim ( ps:void?; ti:TypeInfo ) : void
    def abstract beforeArray ( pa:DapiArray; ti:TypeInfo ) : void
    def abstract afterArray ( pa:DapiArray; ti:TypeInfo ) : void
    def abstract beforeTable ( pa:DapiTable; ti:TypeInfo ) : void
    def abstract beforeTableKey ( pa:DapiTable; ti:TypeInfo; pk:void?; ki:TypeInfo; index:uint; last:bool ) : void
    def abstract afterTableKey ( pa:DapiTable; ti:TypeInfo; pk:void?; ki:TypeInfo; index:uint; last:bool ) : void
    def abstract beforeTableValue ( pa:DapiTable; ti:TypeInfo; pv:void?; kv:TypeInfo; index:uint; last:bool ) : void
    def abstract afterTableValue ( pa:DapiTable; ti:TypeInfo; pv:void?; kv:TypeInfo; index:uint; last:bool ) : void
    def abstract afterTable ( pa:DapiTable; ti:TypeInfo ) : void
    def abstract beforeRef ( ps:void?; ti:TypeInfo ) : void
    def abstract afterRef ( ps:void?; ti:TypeInfo ) : void
    def abstract beforePtr ( ps:void?; ti:TypeInfo ) : void
    def abstract afterPtr ( ps:void?; ti:TypeInfo ) : void
    def abstract beforeHandle ( ps:void?; ti:TypeInfo ) : void
    def abstract afterHandle ( ps:void?; ti:TypeInfo ) : void
    def abstract beforeLambda ( var value:DapiLambda; ti:TypeInfo ) : void
    def abstract afterLambda ( var value:DapiLambda; ti:TypeInfo ) : void
    def abstract beforeIterator ( var value:DapiSequence; ti:TypeInfo ) : void
    def abstract afterIterator ( var value:DapiSequence; ti:TypeInfo ) : void
    def abstract Null ( ti:TypeInfo ) : void
    def abstract VoidPtr ( var value:void? & ) : void
    def abstract Bool ( var value:bool & ) : void
    def abstract Int8 ( var value:int8 & ) : void
    def abstract UInt8 ( var value:uint8 & ) : void
    def abstract Int16 ( var value:int16 & ) : void
    def abstract UInt16 ( var value:uint16 & ) : void
    def abstract Int64 ( var value:int64 & ) : void
    def abstract UInt64 ( var value:uint64 & ) : void
    def abstract String ( var value:string & ) : void
    def abstract Double ( var value:double & ) : void
    def abstract Float ( var value:float & ) : void
    def abstract Int ( var value:int & ) : void
    def abstract UInt ( var value:uint & ) : void
    def abstract Bitfield ( var value:uint &; ti:TypeInfo ) : void
    def abstract Int2 ( var value:int2 & ) : void
    def abstract Int3 ( var value:int3 & ) : void
    def abstract Int4 ( var value:int4 & ) : void
    def abstract UInt2 ( var value:uint2 & ) : void
    def abstract UInt3 ( var value:uint3 & ) : void
    def abstract UInt4 ( var value:uint4 & ) : void
    def abstract Float2 ( var value:float2 & ) : void
    def abstract Float3 ( var value:float3 & ) : void
    def abstract Float4 ( var value:float4 & ) : void
    def abstract Range ( var value:range & ) : void
    def abstract URange ( var value:urange & ) : void
    def abstract Range64 ( var value:range64 & ) : void
    def abstract URange64 ( var value:urange64 & ) : void
    def abstract WalkBlock ( var value:DapiBlock ) : void
    def abstract WalkFunction ( var value:DapiFunc ) : void
    def abstract WalkEnumeration ( var value:int &; ei:EnumInfo ) : void
    def abstract WalkEnumeration8  ( var value:int8 &; ei:EnumInfo ) : void
    def abstract WalkEnumeration16 ( var value:int16 &; ei:EnumInfo ) : void
    def abstract WalkEnumeration64 ( var value:int64 &; ei:EnumInfo ) : void
    def abstract FakeContext ( var value:Context ) : void
    def abstract InvalidData : void

def make_data_walker ( classPtr ) : smart_ptr<DataWalker>
    static_if typeinfo(is_class *classPtr)
        let classInfo = class_info(*classPtr)
        return <- make_data_walker(classPtr, classInfo)
    else
        concept_assert(false,"can't make data walker of non-class")
        return <- [[smart_ptr<DataWalker>]]

class DapiStackWalker
    def abstract canWalkArguments : bool
    def abstract canWalkVariables : bool
    def abstract canWalkOutOfScopeVariables : bool
    def abstract onBeforeCall ( pp:Prologue; sp:void? ) : void
    def abstract onCallAOT ( pp:Prologue; fileName:string# ) : void
    def abstract onCallAt ( pp:Prologue; info:FuncInfo; at:LineInfo ) : void
    def abstract onCall ( pp:Prologue; info:FuncInfo ) : void
    def abstract onAfterPrologue ( pp:Prologue; sp:void? ) : void
    def abstract onArgument ( info:FuncInfo; index:int; vinfo:VarInfo; arg:float4 ) : void
    def abstract onBeforeVariables : void
    def abstract onVariable ( inf:FuncInfo; vinfo:LocalVariableInfo; arg:void?; inScope:bool ) : void
    def abstract onAfterCall ( pp:Prologue ) : bool

def make_stack_walker ( classPtr ) : smart_ptr<StackWalker>
    static_if typeinfo(is_class *classPtr)
        let classInfo = class_info(*classPtr)
        return <- make_stack_walker(classPtr, classInfo)
    else
        concept_assert(false,"can't make stack walker of non-class")
        return <- [[smart_ptr<StackWalker>]]
