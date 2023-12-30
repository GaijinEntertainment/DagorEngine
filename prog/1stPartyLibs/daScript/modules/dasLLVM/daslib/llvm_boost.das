options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module llvm_boost shared

require llvm public

require daslib/safe_addr
require daslib/defer
require strings

def LLVMIsVector3 ( typ:LLVMOpaqueType? ) : bool
    return (LLVMGetTypeKind(typ)==LLVMTypeKind LLVMVectorTypeKind) && (LLVMGetVectorSize(typ)==3u)

def LLVMBuildLoadVector3 ( builder:LLVMOpaqueBuilder?; ptr:LLVMOpaqueValue?; name:string )
    var typ = LLVMGetElementType(LLVMTypeOf(ptr))
    verify(LLVMGetVectorSize(typ)==3u)
    let elemT = LLVMGetElementType(typ)
    var ld = LLVMBuildLoad2(builder, LLVMVectorType(elemT, 4u), ptr, name)
    LLVMSetAlignment(ld, 4u)
    var x = LLVMBuildExtractElement(builder, ld, LLVMConstInt(LLVMInt32Type(),0ul,0), "")
    var y = LLVMBuildExtractElement(builder, ld, LLVMConstInt(LLVMInt32Type(),1ul,0), "")
    var z = LLVMBuildExtractElement(builder, ld, LLVMConstInt(LLVMInt32Type(),2ul,0), "")
    var xyz = LLVMGetUndef(LLVMVectorType(elemT, 3u))
    xyz = LLVMBuildInsertElement(builder, xyz, x, LLVMConstInt(LLVMInt32Type(),0ul,0), "")
    xyz = LLVMBuildInsertElement(builder, xyz, y, LLVMConstInt(LLVMInt32Type(),1ul,0), "")
    xyz = LLVMBuildInsertElement(builder, xyz, z, LLVMConstInt(LLVMInt32Type(),2ul,0), "")
    return xyz

def LLVMBuildLoadAligned ( builder:LLVMOpaqueBuilder?; ptr:LLVMOpaqueValue?; alignment:uint; name:string )
    let ld = LLVMBuildLoad(builder, ptr, name)
    LLVMSetAlignment(ld, alignment)
    return ld

def LLVMBuildLoad2Aligned ( builder:LLVMOpaqueBuilder?; typ:LLVMOpaqueType?; ptr:LLVMOpaqueValue?; alignment:uint; name:string )
    let ld = LLVMBuildLoad2(builder, typ, ptr, name)
    LLVMSetAlignment(ld, alignment)
    return ld

def LLVMBuildStoreAligned(builder:LLVMOpaqueBuilder?; val, ptr:LLVMOpaqueValue?; alignment:uint)
    let st = LLVMBuildStore(builder, val, ptr)
    LLVMSetAlignment(st, alignment)
    return st

def LLVMBuildStore2 ( builder:LLVMOpaqueBuilder?; typ:LLVMOpaqueType?; val:LLVMOpaqueValue?; ptr:LLVMOpaqueValue?  )
    let tptr = LLVMBuildPointerCast(builder, ptr, LLVMPointerType(typ,0u), "")
    return LLVMBuildStore(builder, val, tptr)

def LLVMBuildStore2Aligned ( builder:LLVMOpaqueBuilder?; typ:LLVMOpaqueType?; val:LLVMOpaqueValue?; ptr:LLVMOpaqueValue?; alignment:uint )
    let tptr = LLVMBuildPointerCast(builder, ptr, LLVMPointerType(typ,0u), "")
    let st = LLVMBuildStore(builder, val, tptr)
    LLVMSetAlignment(st, alignment)
    return st

def LLVMStructType ( var fields:LLVMOpaqueType? [] )
    return LLVMStructType(unsafe(addr(fields[0])), uint(length(fields)), 0)

def LLVMStructType ( var fields:array<LLVMOpaqueType?> )
    return LLVMStructType(unsafe(addr(fields[0])), uint(length(fields)), 0)

def LLVMGetEnumAttributeKindForName(attr_name:string)
    return LLVMGetEnumAttributeKindForName(attr_name,uint64(length(attr_name)))

def LLVMGetEnumAttribute(attr_name:string) : LLVMOpaqueAttributeRef?
    var attr_id = LLVMGetEnumAttributeKindForName(attr_name)
    var attr = LLVMCreateEnumAttribute(LLVMGetGlobalContext(), attr_id, 0ul)
    if attr==null
        panic("{attr_name} attribute not found")
    return attr

def LLVMAddAttributeToFunction(ffunc:LLVMOpaqueValue?; attr:LLVMOpaqueAttributeRef?)
    LLVMAddAttributeAtIndex(ffunc, ~0u, attr)

def LLVMAddAttributesToFunction(ffunc:LLVMOpaqueValue?; attr:LLVMOpaqueAttributeRef? [])
    for a in attr
        LLVMAddAttributeAtIndex(ffunc, ~0u, a)

def LLVMAddAttributeToFunctionResult(ffunc:LLVMOpaqueValue?; attr:LLVMOpaqueAttributeRef?)
    LLVMAddAttributeAtIndex(ffunc, 0u, attr)

def LLVMAddAttributeToFunctionArgument(ffunc:LLVMOpaqueValue?; ai:uint; attr:LLVMOpaqueAttributeRef?)
    LLVMAddAttributeAtIndex(ffunc, ai+1u, attr)

def LLVMBuildShuffleVector(builder:LLVMOpaqueBuilder?; v1, v2:LLVMOpaqueValue?; mask:array<int>; name:string )
    unsafe
        var inscope elem : array<LLVMOpaqueValue?>
        for m in mask
            if m!=-1
                elem |> push(LLVMConstInt(LLVMInt32Type(), uint64(m), 0))
            else
                elem |> push(LLVMGetUndef(LLVMInt32Type()))
        return LLVMBuildShuffleVector(builder, v1, v2, LLVMConstVector(unsafe(addr(elem[0])), uint(length(elem))), name)

def LLVMAddIncoming(phi:LLVMOpaqueValue?; var phi_vals:LLVMOpaqueValue? []; var phi_blocks:LLVMOpaqueBasicBlock? [])
    static_if typeinfo(dim phi_vals) != typeinfo(dim phi_blocks)
        concept_assert(false, "LLVMAddIncoming: phi_vals and phi_blocks must have the same length")
    LLVMAddIncoming(phi, unsafe(addr(phi_vals[0])), unsafe(addr(phi_blocks[0])), uint(length(phi_vals)))

def LLVMAddIncoming(phi:LLVMOpaqueValue?; var phi_vals:array<LLVMOpaqueValue?>; var phi_blocks:array<LLVMOpaqueBasicBlock?>)
    assert(length(phi_vals) == length(phi_blocks), "LLVMAddIncoming: phi_vals and phi_blocks must have the same length")
    LLVMAddIncoming(phi, unsafe(addr(phi_vals[0])), unsafe(addr(phi_blocks[0])), uint(length(phi_vals)))

def LLVMBuildGEP ( builder:LLVMOpaqueBuilder?; ptr:LLVMOpaqueValue?; var index:LLVMOpaqueValue?; name:string; inbounds:bool=true )
    var gep = LLVMBuildGEP(builder, ptr, unsafe(addr(index)), 1u, name)
    if inbounds
        LLVMSetIsInBounds(gep, 1)
    return gep

def LLVMBuildGEP2 ( builder:LLVMOpaqueBuilder?; ty:LLVMOpaqueType?; ptr:LLVMOpaqueValue?; var index:LLVMOpaqueValue?; name:string; inbounds:bool=true )
    var gep = LLVMBuildGEP2(builder, ty, ptr, unsafe(addr(index)), 1u, name)
    if inbounds
        LLVMSetIsInBounds(gep, 1)
    return gep

def LLVMFloat4Type
    return LLVMVectorType(LLVMFloatType(), 4u)

def LLVMFloat3Type
    return LLVMVectorType(LLVMFloatType(), 3u)

def LLVMFloat2Type
    return LLVMVectorType(LLVMFloatType(), 2u)

def LLVMInt4Type
    return LLVMVectorType(LLVMInt32Type(), 4u)

def LLVMInt3Type
    return LLVMVectorType(LLVMInt32Type(), 3u)

def LLVMInt2Type
    return LLVMVectorType(LLVMInt32Type(), 2u)

def LLVMRange64Type
    return LLVMVectorType(LLVMInt64Type(), 2u)

def LLVMIntPtrType : LLVMOpaqueType?
    static_if typeinfo(sizeof type<void?>)==8
        return LLVMInt64Type()
    else
        return LLVMInt32Type()

def LLVMFunctionType ( var ret_type:LLVMTypeRef; var param_types:LLVMTypeRef[]; isVariadic:bool = false )
    return LLVMFunctionType( ret_type, unsafe(addr(param_types[0])), uint(length(param_types)), isVariadic ? 1 : 0)

def LLVMFunctionType ( var ret_type:LLVMTypeRef; var param_types:array<LLVMTypeRef>; isVariadic:bool = false )
    let nparam = length(param_types)
    if nparam == 0
        return LLVMFunctionType( ret_type, null, 0u, isVariadic ? 1 : 0)
    else
        return LLVMFunctionType( ret_type, unsafe(addr(param_types[0])), uint(nparam), isVariadic ? 1 : 0)

def LLVMFunctionType ( var ret_type:LLVMTypeRef; isVariadic:bool = false )
    return LLVMFunctionType( ret_type, null, 0u, isVariadic ? 1 : 0)

def LLVMVerifyModule(mod:LLVMOpaqueModule?; action:LLVMVerifierFailureAction; fail:bool=true)
    var error : string
    if LLVMVerifyModule(mod, action, safe_addr(error)) != 0
        if fail
            let msg = clone_string(error)
            LLVMDisposeMessage(error)
            panic(msg)
        else
            LLVMDisposeMessage(error)
        return false
    return true

def LLVMCreateJITCompilerForModule(var engine:LLVMExecutionEngineRef&; mod:LLVMOpaqueModule?; optlevel:uint=2u )
    var error : string
    if LLVMCreateJITCompilerForModule(unsafe(addr(engine)), mod, optlevel, safe_addr(error)) != 0
        let msg = clone_string(error)
        LLVMDisposeMessage(error)
        panic(msg)

def LLVMCreateExecutionEngineForModule(var engine:LLVMExecutionEngineRef&; mod:LLVMOpaqueModule? )
    var error : string
    if LLVMCreateExecutionEngineForModule(unsafe(addr(engine)), mod, safe_addr(error)) != 0
        let msg = clone_string(error)
        LLVMDisposeMessage(error)
        panic(msg)

def LLVMBuildAnyInt2_xy ( builder:LLVMOpaqueBuilder?; int2type:LLVMOpaqueType?; x,y:LLVMOpaqueValue?; name:string )
    var vres = LLVMGetUndef(int2type)
    vres = LLVMBuildInsertElement(builder, vres, x, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
    return LLVMBuildInsertElement(builder, vres, y, LLVMConstInt(LLVMInt32Type(), 1ul, 0), name)

def LLVMBuildInt2_xy ( builder:LLVMOpaqueBuilder?; x,y:LLVMOpaqueValue?; name:string )
    var vres = LLVMGetUndef(LLVMInt2Type())
    vres = LLVMBuildInsertElement(builder, vres, x, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
    return LLVMBuildInsertElement(builder, vres, y, LLVMConstInt(LLVMInt32Type(), 1ul, 0), name)

def LLVMBuildInt3_xyz ( builder:LLVMOpaqueBuilder?; x,y,z:LLVMOpaqueValue?; name:string )
    var vres = LLVMGetUndef(LLVMInt3Type())
    vres = LLVMBuildInsertElement(builder, vres, x, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
    vres = LLVMBuildInsertElement(builder, vres, y, LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
    return LLVMBuildInsertElement(builder, vres, z, LLVMConstInt(LLVMInt32Type(), 2ul, 0), name)

def LLVMBuildInt4_xyzw ( builder:LLVMOpaqueBuilder?; x,y,z,w:LLVMOpaqueValue?; name:string )
    var vres = LLVMGetUndef(LLVMInt4Type())
    vres = LLVMBuildInsertElement(builder, vres, x, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
    vres = LLVMBuildInsertElement(builder, vres, y, LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
    vres = LLVMBuildInsertElement(builder, vres, z, LLVMConstInt(LLVMInt32Type(), 2ul, 0), "")
    return LLVMBuildInsertElement(builder, vres, w, LLVMConstInt(LLVMInt32Type(), 3ul, 0), name)

def LLVMBuildFloat2_xy ( builder:LLVMOpaqueBuilder?; x,y:LLVMOpaqueValue?; name:string )
    var vres = LLVMGetUndef(LLVMFloat2Type())
    vres = LLVMBuildInsertElement(builder, vres, x, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
    return LLVMBuildInsertElement(builder, vres, y, LLVMConstInt(LLVMInt32Type(), 1ul, 0), name)

def LLVMBuildFloat3_xyz ( builder:LLVMOpaqueBuilder?; x,y,z:LLVMOpaqueValue?; name:string )
    var vres = LLVMGetUndef(LLVMFloat3Type())
    vres = LLVMBuildInsertElement(builder, vres, x, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
    vres = LLVMBuildInsertElement(builder, vres, y, LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
    return LLVMBuildInsertElement(builder, vres, z, LLVMConstInt(LLVMInt32Type(), 2ul, 0), name)

def LLVMBuildFloat4_xyzw ( builder:LLVMOpaqueBuilder?; x,y,z,w:LLVMOpaqueValue?; name:string )
    var vres = LLVMGetUndef(LLVMFloat4Type())
    vres = LLVMBuildInsertElement(builder, vres, x, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
    vres = LLVMBuildInsertElement(builder, vres, y, LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
    vres = LLVMBuildInsertElement(builder, vres, z, LLVMConstInt(LLVMInt32Type(), 2ul, 0), "")
    return LLVMBuildInsertElement(builder, vres, w, LLVMConstInt(LLVMInt32Type(), 3ul, 0), name)

def LLVMBuildCall ( builder:LLVMOpaqueBuilder?; func:LLVMOpaqueValue?; var args:LLVMOpaqueValue? []; name:string )
    return LLVMBuildCall( builder, func, unsafe(addr(args[0])), uint(length(args)), name)

def LLVMBuildCall ( builder:LLVMOpaqueBuilder?; func:LLVMOpaqueValue?; name:string )
    return LLVMBuildCall( builder, func, null, 0u, name)

def LLVMBuildCall ( builder:LLVMOpaqueBuilder?; func:LLVMOpaqueValue?; var args:array<LLVMOpaqueValue?>; name:string )
    let nparams = length(args)
    if nparams != 0
        return LLVMBuildCall( builder, func, unsafe(addr(args[0])), uint(nparams), name)
    else
        return LLVMBuildCall( builder, func, null, 0u, name)

def LLVMLookupIntrinsicID ( name:string )
    return LLVMLookupIntrinsicID(name,uint64(length(name)))

def LLVMGetIntrinsicDeclaration ( mod:LLVMOpaqueModule?; id:uint; var argTypes:array<LLVMOpaqueType?> )
    return LLVMGetIntrinsicDeclaration(mod, id, unsafe(addr(argTypes[0])), uint64(length(argTypes)))

def describe ( typ:LLVMOpaqueType? )
    return "null" if typ==null
    var st = LLVMPrintTypeToString(typ)
    var stc = clone_string(st)
    LLVMDisposeMessage(st)
    return stc

def describe ( typ:LLVMOpaqueValue? )
    return "null" if typ==null
    var st = LLVMPrintValueToString(typ)
    var stc = clone_string(st)
    LLVMDisposeMessage(st)
    return stc

def describe ( blk:LLVMOpaqueBasicBlock? )
    return "null" if blk==null
    var st := LLVMGetBasicBlockName(blk)
    return st

def LLVMSetFunctionCallConv(ofunc:LLVMOpaqueValue?; conv:LLVMCallConv)
    LLVMSetFunctionCallConv(ofunc, uint(conv))

def LLVMSetInstructionCallConv(ccall:LLVMOpaqueValue?; conv:LLVMCallConv)
    LLVMSetInstructionCallConv(ccall, uint(conv))

def set_meta_dereferenceable ( instr:LLVMOpaqueValue?; bytes:int; nil:bool )
    var values : LLVMOpaqueValue? [1]
    values[0] = LLVMConstInt(LLVMInt64Type(), uint64(bytes), 0)
    let msg = nil ? "dereferenceable_or_null" : "dereferenceable"
    LLVMSetMetadata(
        instr,
        LLVMGetMDKindID(msg, uint(length(msg))),
        LLVMMDNode(unsafe(addr(values[0])), 1u))

def LLVMAddAttributeToFunctionArgumentRange ( func:LLVMOpaqueValue?; rng:urange; attr:LLVMOpaqueAttributeRef? )
    for t in rng
        LLVMAddAttributeToFunctionArgument(func, t, attr)
