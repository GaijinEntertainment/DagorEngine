require dasbind public
options gen2
module llvm_func shared
require llvm/bindings/llvm_struct
require llvm/bindings/llvm_enum
[extern(cdecl, name="LLVMVerifyModule", library="LLVM.dll")]
def LLVMVerifyModule(M : LLVMOpaqueModule? implicit; Action : LLVMVerifierFailureAction; OutMessage : string? implicit) : int {}
[extern(cdecl, name="LLVMVerifyFunction", library="LLVM.dll")]
def LLVMVerifyFunction(Fn : LLVMOpaqueValue? implicit; Action : LLVMVerifierFailureAction) : int {}
[extern(cdecl, name="LLVMViewFunctionCFG", library="LLVM.dll")]
def LLVMViewFunctionCFG(Fn : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMViewFunctionCFGOnly", library="LLVM.dll")]
def LLVMViewFunctionCFGOnly(Fn : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMParseBitcode", library="LLVM.dll")]
def LLVMParseBitcode(MemBuf : LLVMOpaqueMemoryBuffer? implicit; OutModule : LLVMOpaqueModule ?? implicit; OutMessage : string? implicit) : int {}
[extern(cdecl, name="LLVMParseBitcode2", library="LLVM.dll")]
def LLVMParseBitcode2(MemBuf : LLVMOpaqueMemoryBuffer? implicit; OutModule : LLVMOpaqueModule ?? implicit) : int {}
[extern(cdecl, name="LLVMParseBitcodeInContext", library="LLVM.dll")]
def LLVMParseBitcodeInContext(ContextRef : LLVMOpaqueContext? implicit; MemBuf : LLVMOpaqueMemoryBuffer? implicit; OutModule : LLVMOpaqueModule ?? implicit; OutMessage : string? implicit) : int {}
[extern(cdecl, name="LLVMParseBitcodeInContext2", library="LLVM.dll")]
def LLVMParseBitcodeInContext2(ContextRef : LLVMOpaqueContext? implicit; MemBuf : LLVMOpaqueMemoryBuffer? implicit; OutModule : LLVMOpaqueModule ?? implicit) : int {}
[extern(cdecl, name="LLVMGetBitcodeModuleInContext", library="LLVM.dll")]
def LLVMGetBitcodeModuleInContext(ContextRef : LLVMOpaqueContext? implicit; MemBuf : LLVMOpaqueMemoryBuffer? implicit; OutM : LLVMOpaqueModule ?? implicit; OutMessage : string? implicit) : int {}
[extern(cdecl, name="LLVMGetBitcodeModuleInContext2", library="LLVM.dll")]
def LLVMGetBitcodeModuleInContext2(ContextRef : LLVMOpaqueContext? implicit; MemBuf : LLVMOpaqueMemoryBuffer? implicit; OutM : LLVMOpaqueModule ?? implicit) : int {}
[extern(cdecl, name="LLVMGetBitcodeModule", library="LLVM.dll")]
def LLVMGetBitcodeModule(MemBuf : LLVMOpaqueMemoryBuffer? implicit; OutM : LLVMOpaqueModule ?? implicit; OutMessage : string? implicit) : int {}
[extern(cdecl, name="LLVMGetBitcodeModule2", library="LLVM.dll")]
def LLVMGetBitcodeModule2(MemBuf : LLVMOpaqueMemoryBuffer? implicit; OutM : LLVMOpaqueModule ?? implicit) : int {}
[extern(cdecl, name="LLVMWriteBitcodeToFile", library="LLVM.dll")]
def LLVMWriteBitcodeToFile(M : LLVMOpaqueModule? implicit; Path : string implicit) : int {}
[extern(cdecl, name="LLVMWriteBitcodeToFD", library="LLVM.dll")]
def LLVMWriteBitcodeToFD(M : LLVMOpaqueModule? implicit; FD : int; ShouldClose : int; Unbuffered : int) : int {}
[extern(cdecl, name="LLVMWriteBitcodeToFileHandle", library="LLVM.dll")]
def LLVMWriteBitcodeToFileHandle(M : LLVMOpaqueModule? implicit; Handle : int) : int {}
[extern(cdecl, name="LLVMWriteBitcodeToMemoryBuffer", library="LLVM.dll")]
def LLVMWriteBitcodeToMemoryBuffer(M : LLVMOpaqueModule? implicit) : LLVMOpaqueMemoryBuffer? {}
[extern(cdecl, name="LLVMGetOrInsertComdat", library="LLVM.dll")]
def LLVMGetOrInsertComdat(M : LLVMOpaqueModule? implicit; Name : string implicit) : LLVMComdat? {}
[extern(cdecl, name="LLVMGetComdat", library="LLVM.dll")]
def LLVMGetComdat(V : LLVMOpaqueValue? implicit) : LLVMComdat? {}
[extern(cdecl, name="LLVMSetComdat", library="LLVM.dll")]
def LLVMSetComdat(V : LLVMOpaqueValue? implicit; C : LLVMComdat? implicit) : void {}
[extern(cdecl, name="LLVMGetComdatSelectionKind", library="LLVM.dll")]
def LLVMGetComdatSelectionKind(C : LLVMComdat? implicit) : LLVMComdatSelectionKind {}
[extern(cdecl, name="LLVMSetComdatSelectionKind", library="LLVM.dll")]
def LLVMSetComdatSelectionKind(C : LLVMComdat? implicit; Kind : LLVMComdatSelectionKind) : void {}
[extern(cdecl, name="LLVMResetFatalErrorHandler", library="LLVM.dll")]
def LLVMResetFatalErrorHandler() : void {}
[extern(cdecl, name="LLVMEnablePrettyStackTrace", library="LLVM.dll")]
def LLVMEnablePrettyStackTrace() : void {}
[extern(cdecl, name="LLVMShutdown", library="LLVM.dll")]
def LLVMShutdown() : void {}
[extern(cdecl, name="LLVMGetVersion", library="LLVM.dll")]
def LLVMGetVersion(Major : uint? implicit; Minor : uint? implicit; Patch : uint? implicit) : void {}
[extern(cdecl, name="LLVMCreateMessage", library="LLVM.dll")]
def LLVMCreateMessage(Message : string implicit) : string {}
[extern(cdecl, name="LLVMDisposeMessage", library="LLVM.dll")]
def LLVMDisposeMessage(Message : string implicit) : void {}
[extern(cdecl, name="LLVMContextCreate", library="LLVM.dll")]
def LLVMContextCreate() : LLVMOpaqueContext? {}
[extern(cdecl, name="LLVMGetGlobalContext", library="LLVM.dll")]
def LLVMGetGlobalContext() : LLVMOpaqueContext? {}
[extern(cdecl, name="LLVMContextGetDiagnosticContext", library="LLVM.dll")]
def LLVMContextGetDiagnosticContext(C : LLVMOpaqueContext? implicit) : void? {}
[extern(cdecl, name="LLVMContextShouldDiscardValueNames", library="LLVM.dll")]
def LLVMContextShouldDiscardValueNames(C : LLVMOpaqueContext? implicit) : int {}
[extern(cdecl, name="LLVMContextSetDiscardValueNames", library="LLVM.dll")]
def LLVMContextSetDiscardValueNames(C : LLVMOpaqueContext? implicit; Discard : int) : void {}
[extern(cdecl, name="LLVMContextSetOpaquePointers", library="LLVM.dll")]
def LLVMContextSetOpaquePointers(C : LLVMOpaqueContext? implicit; OpaquePointers : int) : void {}
[extern(cdecl, name="LLVMContextDispose", library="LLVM.dll")]
def LLVMContextDispose(C : LLVMOpaqueContext? implicit) : void {}
[extern(cdecl, name="LLVMGetDiagInfoDescription", library="LLVM.dll")]
def LLVMGetDiagInfoDescription(DI : LLVMOpaqueDiagnosticInfo? implicit) : string {}
[extern(cdecl, name="LLVMGetDiagInfoSeverity", library="LLVM.dll")]
def LLVMGetDiagInfoSeverity(DI : LLVMOpaqueDiagnosticInfo? implicit) : LLVMDiagnosticSeverity {}
[extern(cdecl, name="LLVMGetMDKindIDInContext", library="LLVM.dll")]
def LLVMGetMDKindIDInContext(C : LLVMOpaqueContext? implicit; Name : string implicit; SLen : uint) : uint {}
[extern(cdecl, name="LLVMGetMDKindID", library="LLVM.dll")]
def LLVMGetMDKindID(Name : string implicit; SLen : uint) : uint {}
[extern(cdecl, name="LLVMGetEnumAttributeKindForName", library="LLVM.dll")]
def LLVMGetEnumAttributeKindForName(Name : string implicit; SLen : uint64) : uint {}
[extern(cdecl, name="LLVMGetLastEnumAttributeKind", library="LLVM.dll")]
def LLVMGetLastEnumAttributeKind() : uint {}
[extern(cdecl, name="LLVMCreateEnumAttribute", library="LLVM.dll")]
def LLVMCreateEnumAttribute(C : LLVMOpaqueContext? implicit; KindID : uint; Val : uint64) : LLVMOpaqueAttributeRef? {}
[extern(cdecl, name="LLVMGetEnumAttributeKind", library="LLVM.dll")]
def LLVMGetEnumAttributeKind(A : LLVMOpaqueAttributeRef? implicit) : uint {}
[extern(cdecl, name="LLVMGetEnumAttributeValue", library="LLVM.dll")]
def LLVMGetEnumAttributeValue(A : LLVMOpaqueAttributeRef? implicit) : uint64 {}
[extern(cdecl, name="LLVMCreateTypeAttribute", library="LLVM.dll")]
def LLVMCreateTypeAttribute(C : LLVMOpaqueContext? implicit; KindID : uint; type_ref : LLVMOpaqueType? implicit) : LLVMOpaqueAttributeRef? {}
[extern(cdecl, name="LLVMGetTypeAttributeValue", library="LLVM.dll")]
def LLVMGetTypeAttributeValue(A : LLVMOpaqueAttributeRef? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMCreateStringAttribute", library="LLVM.dll")]
def LLVMCreateStringAttribute(C : LLVMOpaqueContext? implicit; K : string implicit; KLength : uint; V : string implicit; VLength : uint) : LLVMOpaqueAttributeRef? {}
[extern(cdecl, name="LLVMGetStringAttributeKind", library="LLVM.dll")]
def LLVMGetStringAttributeKind(A : LLVMOpaqueAttributeRef? implicit; Length : uint? implicit) : string {}
[extern(cdecl, name="LLVMGetStringAttributeValue", library="LLVM.dll")]
def LLVMGetStringAttributeValue(A : LLVMOpaqueAttributeRef? implicit; Length : uint? implicit) : string {}
[extern(cdecl, name="LLVMIsEnumAttribute", library="LLVM.dll")]
def LLVMIsEnumAttribute(A : LLVMOpaqueAttributeRef? implicit) : int {}
[extern(cdecl, name="LLVMIsStringAttribute", library="LLVM.dll")]
def LLVMIsStringAttribute(A : LLVMOpaqueAttributeRef? implicit) : int {}
[extern(cdecl, name="LLVMIsTypeAttribute", library="LLVM.dll")]
def LLVMIsTypeAttribute(A : LLVMOpaqueAttributeRef? implicit) : int {}
[extern(cdecl, name="LLVMGetTypeByName2", library="LLVM.dll")]
def LLVMGetTypeByName2(C : LLVMOpaqueContext? implicit; Name : string implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMModuleCreateWithName", library="LLVM.dll")]
def LLVMModuleCreateWithName(ModuleID : string implicit) : LLVMOpaqueModule? {}
[extern(cdecl, name="LLVMModuleCreateWithNameInContext", library="LLVM.dll")]
def LLVMModuleCreateWithNameInContext(ModuleID : string implicit; C : LLVMOpaqueContext? implicit) : LLVMOpaqueModule? {}
[extern(cdecl, name="LLVMCloneModule", library="LLVM.dll")]
def LLVMCloneModule(M : LLVMOpaqueModule? implicit) : LLVMOpaqueModule? {}
[extern(cdecl, name="LLVMDisposeModule", library="LLVM.dll")]
def LLVMDisposeModule(M : LLVMOpaqueModule? implicit) : void {}
[extern(cdecl, name="LLVMGetModuleIdentifier", library="LLVM.dll")]
def LLVMGetModuleIdentifier(M : LLVMOpaqueModule? implicit; Len : uint64? implicit) : string {}
[extern(cdecl, name="LLVMSetModuleIdentifier", library="LLVM.dll")]
def LLVMSetModuleIdentifier(M : LLVMOpaqueModule? implicit; Ident : string implicit; Len : uint64) : void {}
[extern(cdecl, name="LLVMGetSourceFileName", library="LLVM.dll")]
def LLVMGetSourceFileName(M : LLVMOpaqueModule? implicit; Len : uint64? implicit) : string {}
[extern(cdecl, name="LLVMSetSourceFileName", library="LLVM.dll")]
def LLVMSetSourceFileName(M : LLVMOpaqueModule? implicit; Name : string implicit; Len : uint64) : void {}
[extern(cdecl, name="LLVMGetDataLayoutStr", library="LLVM.dll")]
def LLVMGetDataLayoutStr(M : LLVMOpaqueModule? implicit) : string {}
[extern(cdecl, name="LLVMGetDataLayout", library="LLVM.dll")]
def LLVMGetDataLayout(M : LLVMOpaqueModule? implicit) : string {}
[extern(cdecl, name="LLVMSetDataLayout", library="LLVM.dll")]
def LLVMSetDataLayout(M : LLVMOpaqueModule? implicit; DataLayoutStr : string implicit) : void {}
[extern(cdecl, name="LLVMGetTarget", library="LLVM.dll")]
def LLVMGetTarget(M : LLVMOpaqueModule? implicit) : string {}
[extern(cdecl, name="LLVMSetTarget", library="LLVM.dll")]
def LLVMSetTarget(M : LLVMOpaqueModule? implicit; Triple : string implicit) : void {}
[extern(cdecl, name="LLVMCopyModuleFlagsMetadata", library="LLVM.dll")]
def LLVMCopyModuleFlagsMetadata(M : LLVMOpaqueModule? implicit; Len : uint64? implicit) : LLVMOpaqueModuleFlagEntry? {}
[extern(cdecl, name="LLVMDisposeModuleFlagsMetadata", library="LLVM.dll")]
def LLVMDisposeModuleFlagsMetadata(Entries : LLVMOpaqueModuleFlagEntry? implicit) : void {}
[extern(cdecl, name="LLVMModuleFlagEntriesGetFlagBehavior", library="LLVM.dll")]
def LLVMModuleFlagEntriesGetFlagBehavior(Entries : LLVMOpaqueModuleFlagEntry? implicit; Index : uint) : LLVMModuleFlagBehavior {}
[extern(cdecl, name="LLVMModuleFlagEntriesGetKey", library="LLVM.dll")]
def LLVMModuleFlagEntriesGetKey(Entries : LLVMOpaqueModuleFlagEntry? implicit; Index : uint; Len : uint64? implicit) : string {}
[extern(cdecl, name="LLVMModuleFlagEntriesGetMetadata", library="LLVM.dll")]
def LLVMModuleFlagEntriesGetMetadata(Entries : LLVMOpaqueModuleFlagEntry? implicit; Index : uint) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMGetModuleFlag", library="LLVM.dll")]
def LLVMGetModuleFlag(M : LLVMOpaqueModule? implicit; Key : string implicit; KeyLen : uint64) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMAddModuleFlag", library="LLVM.dll")]
def LLVMAddModuleFlag(M : LLVMOpaqueModule? implicit; Behavior : LLVMModuleFlagBehavior; Key : string implicit; KeyLen : uint64; Val : LLVMOpaqueMetadata? implicit) : void {}
[extern(cdecl, name="LLVMDumpModule", library="LLVM.dll")]
def LLVMDumpModule(M : LLVMOpaqueModule? implicit) : void {}
[extern(cdecl, name="LLVMPrintModuleToFile", library="LLVM.dll")]
def LLVMPrintModuleToFile(M : LLVMOpaqueModule? implicit; Filename : string implicit; ErrorMessage : string? implicit) : int {}
[extern(cdecl, name="LLVMPrintModuleToString", library="LLVM.dll")]
def LLVMPrintModuleToString(M : LLVMOpaqueModule? implicit) : string {}
[extern(cdecl, name="LLVMGetModuleInlineAsm", library="LLVM.dll")]
def LLVMGetModuleInlineAsm(M : LLVMOpaqueModule? implicit; Len : uint64? implicit) : string {}
[extern(cdecl, name="LLVMSetModuleInlineAsm2", library="LLVM.dll")]
def LLVMSetModuleInlineAsm2(M : LLVMOpaqueModule? implicit; Asm : string implicit; Len : uint64) : void {}
[extern(cdecl, name="LLVMAppendModuleInlineAsm", library="LLVM.dll")]
def LLVMAppendModuleInlineAsm(M : LLVMOpaqueModule? implicit; Asm : string implicit; Len : uint64) : void {}
[extern(cdecl, name="LLVMGetInlineAsm", library="LLVM.dll")]
def LLVMGetInlineAsm(Ty : LLVMOpaqueType? implicit; AsmString : string implicit; AsmStringSize : uint64; Constraints : string implicit; ConstraintsSize : uint64; HasSideEffects : int; IsAlignStack : int; Dialect : LLVMInlineAsmDialect; CanThrow : int) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetModuleContext", library="LLVM.dll")]
def LLVMGetModuleContext(M : LLVMOpaqueModule? implicit) : LLVMOpaqueContext? {}
[extern(cdecl, name="LLVMGetTypeByName", library="LLVM.dll")]
def LLVMGetTypeByName(M : LLVMOpaqueModule? implicit; Name : string implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMGetFirstNamedMetadata", library="LLVM.dll")]
def LLVMGetFirstNamedMetadata(M : LLVMOpaqueModule? implicit) : LLVMOpaqueNamedMDNode? {}
[extern(cdecl, name="LLVMGetLastNamedMetadata", library="LLVM.dll")]
def LLVMGetLastNamedMetadata(M : LLVMOpaqueModule? implicit) : LLVMOpaqueNamedMDNode? {}
[extern(cdecl, name="LLVMGetNextNamedMetadata", library="LLVM.dll")]
def LLVMGetNextNamedMetadata(NamedMDNode : LLVMOpaqueNamedMDNode? implicit) : LLVMOpaqueNamedMDNode? {}
[extern(cdecl, name="LLVMGetPreviousNamedMetadata", library="LLVM.dll")]
def LLVMGetPreviousNamedMetadata(NamedMDNode : LLVMOpaqueNamedMDNode? implicit) : LLVMOpaqueNamedMDNode? {}
[extern(cdecl, name="LLVMGetNamedMetadata", library="LLVM.dll")]
def LLVMGetNamedMetadata(M : LLVMOpaqueModule? implicit; Name : string implicit; NameLen : uint64) : LLVMOpaqueNamedMDNode? {}
[extern(cdecl, name="LLVMGetOrInsertNamedMetadata", library="LLVM.dll")]
def LLVMGetOrInsertNamedMetadata(M : LLVMOpaqueModule? implicit; Name : string implicit; NameLen : uint64) : LLVMOpaqueNamedMDNode? {}
[extern(cdecl, name="LLVMGetNamedMetadataName", library="LLVM.dll")]
def LLVMGetNamedMetadataName(NamedMD : LLVMOpaqueNamedMDNode? implicit; NameLen : uint64? implicit) : string {}
[extern(cdecl, name="LLVMGetNamedMetadataNumOperands", library="LLVM.dll")]
def LLVMGetNamedMetadataNumOperands(M : LLVMOpaqueModule? implicit; Name : string implicit) : uint {}
[extern(cdecl, name="LLVMGetNamedMetadataOperands", library="LLVM.dll")]
def LLVMGetNamedMetadataOperands(M : LLVMOpaqueModule? implicit; Name : string implicit; Dest : LLVMOpaqueValue ?? implicit) : void {}
[extern(cdecl, name="LLVMAddNamedMetadataOperand", library="LLVM.dll")]
def LLVMAddNamedMetadataOperand(M : LLVMOpaqueModule? implicit; Name : string implicit; Val : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMGetDebugLocDirectory", library="LLVM.dll")]
def LLVMGetDebugLocDirectory(Val : LLVMOpaqueValue? implicit; Length : uint? implicit) : string {}
[extern(cdecl, name="LLVMGetDebugLocFilename", library="LLVM.dll")]
def LLVMGetDebugLocFilename(Val : LLVMOpaqueValue? implicit; Length : uint? implicit) : string {}
[extern(cdecl, name="LLVMGetDebugLocLine", library="LLVM.dll")]
def LLVMGetDebugLocLine(Val : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMGetDebugLocColumn", library="LLVM.dll")]
def LLVMGetDebugLocColumn(Val : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMAddFunction", library="LLVM.dll")]
def LLVMAddFunction(M : LLVMOpaqueModule? implicit; Name : string implicit; FunctionTy : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetNamedFunction", library="LLVM.dll")]
def LLVMGetNamedFunction(M : LLVMOpaqueModule? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetFirstFunction", library="LLVM.dll")]
def LLVMGetFirstFunction(M : LLVMOpaqueModule? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetLastFunction", library="LLVM.dll")]
def LLVMGetLastFunction(M : LLVMOpaqueModule? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetNextFunction", library="LLVM.dll")]
def LLVMGetNextFunction(Fn : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetPreviousFunction", library="LLVM.dll")]
def LLVMGetPreviousFunction(Fn : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMSetModuleInlineAsm", library="LLVM.dll")]
def LLVMSetModuleInlineAsm(M : LLVMOpaqueModule? implicit; Asm : string implicit) : void {}
[extern(cdecl, name="LLVMGetTypeKind", library="LLVM.dll")]
def LLVMGetTypeKind(Ty : LLVMOpaqueType? implicit) : LLVMTypeKind {}
[extern(cdecl, name="LLVMTypeIsSized", library="LLVM.dll")]
def LLVMTypeIsSized(Ty : LLVMOpaqueType? implicit) : int {}
[extern(cdecl, name="LLVMGetTypeContext", library="LLVM.dll")]
def LLVMGetTypeContext(Ty : LLVMOpaqueType? implicit) : LLVMOpaqueContext? {}
[extern(cdecl, name="LLVMDumpType", library="LLVM.dll")]
def LLVMDumpType(Val : LLVMOpaqueType? implicit) : void {}
[extern(cdecl, name="LLVMPrintTypeToString", library="LLVM.dll")]
def LLVMPrintTypeToString(Val : LLVMOpaqueType? implicit) : string {}
[extern(cdecl, name="LLVMInt1TypeInContext", library="LLVM.dll")]
def LLVMInt1TypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMInt8TypeInContext", library="LLVM.dll")]
def LLVMInt8TypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMInt16TypeInContext", library="LLVM.dll")]
def LLVMInt16TypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMInt32TypeInContext", library="LLVM.dll")]
def LLVMInt32TypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMInt64TypeInContext", library="LLVM.dll")]
def LLVMInt64TypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMInt128TypeInContext", library="LLVM.dll")]
def LLVMInt128TypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMIntTypeInContext", library="LLVM.dll")]
def LLVMIntTypeInContext(C : LLVMOpaqueContext? implicit; NumBits : uint) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMInt1Type", library="LLVM.dll")]
def LLVMInt1Type() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMInt8Type", library="LLVM.dll")]
def LLVMInt8Type() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMInt16Type", library="LLVM.dll")]
def LLVMInt16Type() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMInt32Type", library="LLVM.dll")]
def LLVMInt32Type() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMInt64Type", library="LLVM.dll")]
def LLVMInt64Type() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMInt128Type", library="LLVM.dll")]
def LLVMInt128Type() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMIntType", library="LLVM.dll")]
def LLVMIntType(NumBits : uint) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMGetIntTypeWidth", library="LLVM.dll")]
def LLVMGetIntTypeWidth(IntegerTy : LLVMOpaqueType? implicit) : uint {}
[extern(cdecl, name="LLVMHalfTypeInContext", library="LLVM.dll")]
def LLVMHalfTypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMBFloatTypeInContext", library="LLVM.dll")]
def LLVMBFloatTypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMFloatTypeInContext", library="LLVM.dll")]
def LLVMFloatTypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMDoubleTypeInContext", library="LLVM.dll")]
def LLVMDoubleTypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMX86FP80TypeInContext", library="LLVM.dll")]
def LLVMX86FP80TypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMFP128TypeInContext", library="LLVM.dll")]
def LLVMFP128TypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMPPCFP128TypeInContext", library="LLVM.dll")]
def LLVMPPCFP128TypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMHalfType", library="LLVM.dll")]
def LLVMHalfType() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMBFloatType", library="LLVM.dll")]
def LLVMBFloatType() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMFloatType", library="LLVM.dll")]
def LLVMFloatType() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMDoubleType", library="LLVM.dll")]
def LLVMDoubleType() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMX86FP80Type", library="LLVM.dll")]
def LLVMX86FP80Type() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMFP128Type", library="LLVM.dll")]
def LLVMFP128Type() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMPPCFP128Type", library="LLVM.dll")]
def LLVMPPCFP128Type() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMFunctionType", library="LLVM.dll")]
def LLVMFunctionType(ReturnType : LLVMOpaqueType? implicit; ParamTypes : LLVMOpaqueType ?? implicit; ParamCount : uint; IsVarArg : int) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMIsFunctionVarArg", library="LLVM.dll")]
def LLVMIsFunctionVarArg(FunctionTy : LLVMOpaqueType? implicit) : int {}
[extern(cdecl, name="LLVMGetReturnType", library="LLVM.dll")]
def LLVMGetReturnType(FunctionTy : LLVMOpaqueType? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMCountParamTypes", library="LLVM.dll")]
def LLVMCountParamTypes(FunctionTy : LLVMOpaqueType? implicit) : uint {}
[extern(cdecl, name="LLVMGetParamTypes", library="LLVM.dll")]
def LLVMGetParamTypes(FunctionTy : LLVMOpaqueType? implicit; Dest : LLVMOpaqueType ?? implicit) : void {}
[extern(cdecl, name="LLVMStructTypeInContext", library="LLVM.dll")]
def LLVMStructTypeInContext(C : LLVMOpaqueContext? implicit; ElementTypes : LLVMOpaqueType ?? implicit; ElementCount : uint; Packed : int) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMStructType", library="LLVM.dll")]
def LLVMStructType(ElementTypes : LLVMOpaqueType ?? implicit; ElementCount : uint; Packed : int) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMStructCreateNamed", library="LLVM.dll")]
def LLVMStructCreateNamed(C : LLVMOpaqueContext? implicit; Name : string implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMGetStructName", library="LLVM.dll")]
def LLVMGetStructName(Ty : LLVMOpaqueType? implicit) : string {}
[extern(cdecl, name="LLVMStructSetBody", library="LLVM.dll")]
def LLVMStructSetBody(StructTy : LLVMOpaqueType? implicit; ElementTypes : LLVMOpaqueType ?? implicit; ElementCount : uint; Packed : int) : void {}
[extern(cdecl, name="LLVMCountStructElementTypes", library="LLVM.dll")]
def LLVMCountStructElementTypes(StructTy : LLVMOpaqueType? implicit) : uint {}
[extern(cdecl, name="LLVMGetStructElementTypes", library="LLVM.dll")]
def LLVMGetStructElementTypes(StructTy : LLVMOpaqueType? implicit; Dest : LLVMOpaqueType ?? implicit) : void {}
[extern(cdecl, name="LLVMStructGetTypeAtIndex", library="LLVM.dll")]
def LLVMStructGetTypeAtIndex(StructTy : LLVMOpaqueType? implicit; i : uint) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMIsPackedStruct", library="LLVM.dll")]
def LLVMIsPackedStruct(StructTy : LLVMOpaqueType? implicit) : int {}
[extern(cdecl, name="LLVMIsOpaqueStruct", library="LLVM.dll")]
def LLVMIsOpaqueStruct(StructTy : LLVMOpaqueType? implicit) : int {}
[extern(cdecl, name="LLVMIsLiteralStruct", library="LLVM.dll")]
def LLVMIsLiteralStruct(StructTy : LLVMOpaqueType? implicit) : int {}
[extern(cdecl, name="LLVMGetElementType", library="LLVM.dll")]
def LLVMGetElementType(Ty : LLVMOpaqueType? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMGetSubtypes", library="LLVM.dll")]
def LLVMGetSubtypes(Tp : LLVMOpaqueType? implicit; Arr : LLVMOpaqueType ?? implicit) : void {}
[extern(cdecl, name="LLVMGetNumContainedTypes", library="LLVM.dll")]
def LLVMGetNumContainedTypes(Tp : LLVMOpaqueType? implicit) : uint {}
[extern(cdecl, name="LLVMArrayType", library="LLVM.dll")]
def LLVMArrayType(ElementType : LLVMOpaqueType? implicit; ElementCount : uint) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMGetArrayLength", library="LLVM.dll")]
def LLVMGetArrayLength(ArrayTy : LLVMOpaqueType? implicit) : uint {}
[extern(cdecl, name="LLVMPointerType", library="LLVM.dll")]
def LLVMPointerType(ElementType : LLVMOpaqueType? implicit; AddressSpace : uint) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMPointerTypeIsOpaque", library="LLVM.dll")]
def LLVMPointerTypeIsOpaque(Ty : LLVMOpaqueType? implicit) : int {}
[extern(cdecl, name="LLVMPointerTypeInContext", library="LLVM.dll")]
def LLVMPointerTypeInContext(C : LLVMOpaqueContext? implicit; AddressSpace : uint) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMGetPointerAddressSpace", library="LLVM.dll")]
def LLVMGetPointerAddressSpace(PointerTy : LLVMOpaqueType? implicit) : uint {}
[extern(cdecl, name="LLVMVectorType", library="LLVM.dll")]
def LLVMVectorType(ElementType : LLVMOpaqueType? implicit; ElementCount : uint) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMScalableVectorType", library="LLVM.dll")]
def LLVMScalableVectorType(ElementType : LLVMOpaqueType? implicit; ElementCount : uint) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMGetVectorSize", library="LLVM.dll")]
def LLVMGetVectorSize(VectorTy : LLVMOpaqueType? implicit) : uint {}
[extern(cdecl, name="LLVMVoidTypeInContext", library="LLVM.dll")]
def LLVMVoidTypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMLabelTypeInContext", library="LLVM.dll")]
def LLVMLabelTypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMX86MMXTypeInContext", library="LLVM.dll")]
def LLVMX86MMXTypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMX86AMXTypeInContext", library="LLVM.dll")]
def LLVMX86AMXTypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMTokenTypeInContext", library="LLVM.dll")]
def LLVMTokenTypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMMetadataTypeInContext", library="LLVM.dll")]
def LLVMMetadataTypeInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMVoidType", library="LLVM.dll")]
def LLVMVoidType() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMLabelType", library="LLVM.dll")]
def LLVMLabelType() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMX86MMXType", library="LLVM.dll")]
def LLVMX86MMXType() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMX86AMXType", library="LLVM.dll")]
def LLVMX86AMXType() : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMTargetExtTypeInContext", library="LLVM.dll")]
def LLVMTargetExtTypeInContext(C : LLVMOpaqueContext? implicit; Name : string implicit; TypeParams : LLVMOpaqueType ?? implicit; TypeParamCount : uint; IntParams : uint? implicit; IntParamCount : uint) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMTypeOf", library="LLVM.dll")]
def LLVMTypeOf(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMGetValueKind", library="LLVM.dll")]
def LLVMGetValueKind(Val : LLVMOpaqueValue? implicit) : LLVMValueKind {}
[extern(cdecl, name="LLVMGetValueName2", library="LLVM.dll")]
def LLVMGetValueName2(Val : LLVMOpaqueValue? implicit; Length : uint64? implicit) : string {}
[extern(cdecl, name="LLVMSetValueName2", library="LLVM.dll")]
def LLVMSetValueName2(Val : LLVMOpaqueValue? implicit; Name : string implicit; NameLen : uint64) : void {}
[extern(cdecl, name="LLVMDumpValue", library="LLVM.dll")]
def LLVMDumpValue(Val : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMPrintValueToString", library="LLVM.dll")]
def LLVMPrintValueToString(Val : LLVMOpaqueValue? implicit) : string {}
[extern(cdecl, name="LLVMReplaceAllUsesWith", library="LLVM.dll")]
def LLVMReplaceAllUsesWith(OldVal : LLVMOpaqueValue? implicit; NewVal : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMIsConstant", library="LLVM.dll")]
def LLVMIsConstant(Val : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMIsUndef", library="LLVM.dll")]
def LLVMIsUndef(Val : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMIsPoison", library="LLVM.dll")]
def LLVMIsPoison(Val : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMIsAArgument", library="LLVM.dll")]
def LLVMIsAArgument(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsABasicBlock", library="LLVM.dll")]
def LLVMIsABasicBlock(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAInlineAsm", library="LLVM.dll")]
def LLVMIsAInlineAsm(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAUser", library="LLVM.dll")]
def LLVMIsAUser(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAConstant", library="LLVM.dll")]
def LLVMIsAConstant(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsABlockAddress", library="LLVM.dll")]
def LLVMIsABlockAddress(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAConstantAggregateZero", library="LLVM.dll")]
def LLVMIsAConstantAggregateZero(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAConstantArray", library="LLVM.dll")]
def LLVMIsAConstantArray(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAConstantDataSequential", library="LLVM.dll")]
def LLVMIsAConstantDataSequential(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAConstantDataArray", library="LLVM.dll")]
def LLVMIsAConstantDataArray(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAConstantDataVector", library="LLVM.dll")]
def LLVMIsAConstantDataVector(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAConstantExpr", library="LLVM.dll")]
def LLVMIsAConstantExpr(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAConstantFP", library="LLVM.dll")]
def LLVMIsAConstantFP(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAConstantInt", library="LLVM.dll")]
def LLVMIsAConstantInt(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAConstantPointerNull", library="LLVM.dll")]
def LLVMIsAConstantPointerNull(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAConstantStruct", library="LLVM.dll")]
def LLVMIsAConstantStruct(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAConstantTokenNone", library="LLVM.dll")]
def LLVMIsAConstantTokenNone(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAConstantVector", library="LLVM.dll")]
def LLVMIsAConstantVector(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAGlobalValue", library="LLVM.dll")]
def LLVMIsAGlobalValue(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAGlobalAlias", library="LLVM.dll")]
def LLVMIsAGlobalAlias(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAGlobalObject", library="LLVM.dll")]
def LLVMIsAGlobalObject(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAFunction", library="LLVM.dll")]
def LLVMIsAFunction(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAGlobalVariable", library="LLVM.dll")]
def LLVMIsAGlobalVariable(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAGlobalIFunc", library="LLVM.dll")]
def LLVMIsAGlobalIFunc(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAUndefValue", library="LLVM.dll")]
def LLVMIsAUndefValue(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAPoisonValue", library="LLVM.dll")]
def LLVMIsAPoisonValue(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAInstruction", library="LLVM.dll")]
def LLVMIsAInstruction(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAUnaryOperator", library="LLVM.dll")]
def LLVMIsAUnaryOperator(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsABinaryOperator", library="LLVM.dll")]
def LLVMIsABinaryOperator(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsACallInst", library="LLVM.dll")]
def LLVMIsACallInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAIntrinsicInst", library="LLVM.dll")]
def LLVMIsAIntrinsicInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsADbgInfoIntrinsic", library="LLVM.dll")]
def LLVMIsADbgInfoIntrinsic(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsADbgVariableIntrinsic", library="LLVM.dll")]
def LLVMIsADbgVariableIntrinsic(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsADbgDeclareInst", library="LLVM.dll")]
def LLVMIsADbgDeclareInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsADbgLabelInst", library="LLVM.dll")]
def LLVMIsADbgLabelInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAMemIntrinsic", library="LLVM.dll")]
def LLVMIsAMemIntrinsic(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAMemCpyInst", library="LLVM.dll")]
def LLVMIsAMemCpyInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAMemMoveInst", library="LLVM.dll")]
def LLVMIsAMemMoveInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAMemSetInst", library="LLVM.dll")]
def LLVMIsAMemSetInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsACmpInst", library="LLVM.dll")]
def LLVMIsACmpInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAFCmpInst", library="LLVM.dll")]
def LLVMIsAFCmpInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAICmpInst", library="LLVM.dll")]
def LLVMIsAICmpInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAExtractElementInst", library="LLVM.dll")]
def LLVMIsAExtractElementInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAGetElementPtrInst", library="LLVM.dll")]
def LLVMIsAGetElementPtrInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAInsertElementInst", library="LLVM.dll")]
def LLVMIsAInsertElementInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAInsertValueInst", library="LLVM.dll")]
def LLVMIsAInsertValueInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsALandingPadInst", library="LLVM.dll")]
def LLVMIsALandingPadInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAPHINode", library="LLVM.dll")]
def LLVMIsAPHINode(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsASelectInst", library="LLVM.dll")]
def LLVMIsASelectInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAShuffleVectorInst", library="LLVM.dll")]
def LLVMIsAShuffleVectorInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAStoreInst", library="LLVM.dll")]
def LLVMIsAStoreInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsABranchInst", library="LLVM.dll")]
def LLVMIsABranchInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAIndirectBrInst", library="LLVM.dll")]
def LLVMIsAIndirectBrInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAInvokeInst", library="LLVM.dll")]
def LLVMIsAInvokeInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAReturnInst", library="LLVM.dll")]
def LLVMIsAReturnInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsASwitchInst", library="LLVM.dll")]
def LLVMIsASwitchInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAUnreachableInst", library="LLVM.dll")]
def LLVMIsAUnreachableInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAResumeInst", library="LLVM.dll")]
def LLVMIsAResumeInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsACleanupReturnInst", library="LLVM.dll")]
def LLVMIsACleanupReturnInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsACatchReturnInst", library="LLVM.dll")]
def LLVMIsACatchReturnInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsACatchSwitchInst", library="LLVM.dll")]
def LLVMIsACatchSwitchInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsACallBrInst", library="LLVM.dll")]
def LLVMIsACallBrInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAFuncletPadInst", library="LLVM.dll")]
def LLVMIsAFuncletPadInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsACatchPadInst", library="LLVM.dll")]
def LLVMIsACatchPadInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsACleanupPadInst", library="LLVM.dll")]
def LLVMIsACleanupPadInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAUnaryInstruction", library="LLVM.dll")]
def LLVMIsAUnaryInstruction(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAAllocaInst", library="LLVM.dll")]
def LLVMIsAAllocaInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsACastInst", library="LLVM.dll")]
def LLVMIsACastInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAAddrSpaceCastInst", library="LLVM.dll")]
def LLVMIsAAddrSpaceCastInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsABitCastInst", library="LLVM.dll")]
def LLVMIsABitCastInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAFPExtInst", library="LLVM.dll")]
def LLVMIsAFPExtInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAFPToSIInst", library="LLVM.dll")]
def LLVMIsAFPToSIInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAFPToUIInst", library="LLVM.dll")]
def LLVMIsAFPToUIInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAFPTruncInst", library="LLVM.dll")]
def LLVMIsAFPTruncInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAIntToPtrInst", library="LLVM.dll")]
def LLVMIsAIntToPtrInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAPtrToIntInst", library="LLVM.dll")]
def LLVMIsAPtrToIntInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsASExtInst", library="LLVM.dll")]
def LLVMIsASExtInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsASIToFPInst", library="LLVM.dll")]
def LLVMIsASIToFPInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsATruncInst", library="LLVM.dll")]
def LLVMIsATruncInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAUIToFPInst", library="LLVM.dll")]
def LLVMIsAUIToFPInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAZExtInst", library="LLVM.dll")]
def LLVMIsAZExtInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAExtractValueInst", library="LLVM.dll")]
def LLVMIsAExtractValueInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsALoadInst", library="LLVM.dll")]
def LLVMIsALoadInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAVAArgInst", library="LLVM.dll")]
def LLVMIsAVAArgInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAFreezeInst", library="LLVM.dll")]
def LLVMIsAFreezeInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAAtomicCmpXchgInst", library="LLVM.dll")]
def LLVMIsAAtomicCmpXchgInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAAtomicRMWInst", library="LLVM.dll")]
def LLVMIsAAtomicRMWInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAFenceInst", library="LLVM.dll")]
def LLVMIsAFenceInst(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAMDNode", library="LLVM.dll")]
def LLVMIsAMDNode(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsAMDString", library="LLVM.dll")]
def LLVMIsAMDString(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetValueName", library="LLVM.dll")]
def LLVMGetValueName(Val : LLVMOpaqueValue? implicit) : string {}
[extern(cdecl, name="LLVMSetValueName", library="LLVM.dll")]
def LLVMSetValueName(Val : LLVMOpaqueValue? implicit; Name : string implicit) : void {}
[extern(cdecl, name="LLVMGetFirstUse", library="LLVM.dll")]
def LLVMGetFirstUse(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueUse? {}
[extern(cdecl, name="LLVMGetNextUse", library="LLVM.dll")]
def LLVMGetNextUse(U : LLVMOpaqueUse? implicit) : LLVMOpaqueUse? {}
[extern(cdecl, name="LLVMGetUser", library="LLVM.dll")]
def LLVMGetUser(U : LLVMOpaqueUse? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetUsedValue", library="LLVM.dll")]
def LLVMGetUsedValue(U : LLVMOpaqueUse? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetOperand", library="LLVM.dll")]
def LLVMGetOperand(Val : LLVMOpaqueValue? implicit; Index : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetOperandUse", library="LLVM.dll")]
def LLVMGetOperandUse(Val : LLVMOpaqueValue? implicit; Index : uint) : LLVMOpaqueUse? {}
[extern(cdecl, name="LLVMSetOperand", library="LLVM.dll")]
def LLVMSetOperand(User : LLVMOpaqueValue? implicit; Index : uint; Val : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMGetNumOperands", library="LLVM.dll")]
def LLVMGetNumOperands(Val : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMConstNull", library="LLVM.dll")]
def LLVMConstNull(Ty : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstAllOnes", library="LLVM.dll")]
def LLVMConstAllOnes(Ty : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetUndef", library="LLVM.dll")]
def LLVMGetUndef(Ty : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetPoison", library="LLVM.dll")]
def LLVMGetPoison(Ty : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsNull", library="LLVM.dll")]
def LLVMIsNull(Val : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMConstPointerNull", library="LLVM.dll")]
def LLVMConstPointerNull(Ty : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstInt", library="LLVM.dll")]
def LLVMConstInt(IntTy : LLVMOpaqueType? implicit; N : uint64; SignExtend : int) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstIntOfArbitraryPrecision", library="LLVM.dll")]
def LLVMConstIntOfArbitraryPrecision(IntTy : LLVMOpaqueType? implicit; NumWords : uint; Words : void? const) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstIntOfString", library="LLVM.dll")]
def LLVMConstIntOfString(IntTy : LLVMOpaqueType? implicit; Text : string implicit; Radix : uint8) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstIntOfStringAndSize", library="LLVM.dll")]
def LLVMConstIntOfStringAndSize(IntTy : LLVMOpaqueType? implicit; Text : string implicit; SLen : uint; Radix : uint8) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstReal", library="LLVM.dll")]
def LLVMConstReal(RealTy : LLVMOpaqueType? implicit; N : double) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstRealOfString", library="LLVM.dll")]
def LLVMConstRealOfString(RealTy : LLVMOpaqueType? implicit; Text : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstRealOfStringAndSize", library="LLVM.dll")]
def LLVMConstRealOfStringAndSize(RealTy : LLVMOpaqueType? implicit; Text : string implicit; SLen : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstIntGetZExtValue", library="LLVM.dll")]
def LLVMConstIntGetZExtValue(ConstantVal : LLVMOpaqueValue? implicit) : uint64 {}
[extern(cdecl, name="LLVMConstIntGetSExtValue", library="LLVM.dll")]
def LLVMConstIntGetSExtValue(ConstantVal : LLVMOpaqueValue? implicit) : int64 {}
[extern(cdecl, name="LLVMConstRealGetDouble", library="LLVM.dll")]
def LLVMConstRealGetDouble(ConstantVal : LLVMOpaqueValue? implicit; losesInfo : int? implicit) : double {}
[extern(cdecl, name="LLVMConstStringInContext", library="LLVM.dll")]
def LLVMConstStringInContext(C : LLVMOpaqueContext? implicit; Str : string implicit; Length : uint; DontNullTerminate : int) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstString", library="LLVM.dll")]
def LLVMConstString(Str : string implicit; Length : uint; DontNullTerminate : int) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsConstantString", library="LLVM.dll")]
def LLVMIsConstantString(c : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMGetAsString", library="LLVM.dll")]
def LLVMGetAsString(c : LLVMOpaqueValue? implicit; Length : uint64? implicit) : string {}
[extern(cdecl, name="LLVMConstStructInContext", library="LLVM.dll")]
def LLVMConstStructInContext(C : LLVMOpaqueContext? implicit; ConstantVals : LLVMOpaqueValue ?? implicit; Count : uint; Packed : int) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstStruct", library="LLVM.dll")]
def LLVMConstStruct(ConstantVals : LLVMOpaqueValue ?? implicit; Count : uint; Packed : int) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstArray", library="LLVM.dll")]
def LLVMConstArray(ElementTy : LLVMOpaqueType? implicit; ConstantVals : LLVMOpaqueValue ?? implicit; Length : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstNamedStruct", library="LLVM.dll")]
def LLVMConstNamedStruct(StructTy : LLVMOpaqueType? implicit; ConstantVals : LLVMOpaqueValue ?? implicit; Count : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetAggregateElement", library="LLVM.dll")]
def LLVMGetAggregateElement(C : LLVMOpaqueValue? implicit; Idx : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetElementAsConstant", library="LLVM.dll")]
def LLVMGetElementAsConstant(C : LLVMOpaqueValue? implicit; idx : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstVector", library="LLVM.dll")]
def LLVMConstVector(ScalarConstantVals : LLVMOpaqueValue ?? implicit; Size : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetConstOpcode", library="LLVM.dll")]
def LLVMGetConstOpcode(ConstantVal : LLVMOpaqueValue? implicit) : LLVMOpcode {}
[extern(cdecl, name="LLVMAlignOf", library="LLVM.dll")]
def LLVMAlignOf(Ty : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMSizeOf", library="LLVM.dll")]
def LLVMSizeOf(Ty : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstNeg", library="LLVM.dll")]
def LLVMConstNeg(ConstantVal : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstNSWNeg", library="LLVM.dll")]
def LLVMConstNSWNeg(ConstantVal : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstNUWNeg", library="LLVM.dll")]
def LLVMConstNUWNeg(ConstantVal : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstNot", library="LLVM.dll")]
def LLVMConstNot(ConstantVal : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstAdd", library="LLVM.dll")]
def LLVMConstAdd(LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstNSWAdd", library="LLVM.dll")]
def LLVMConstNSWAdd(LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstNUWAdd", library="LLVM.dll")]
def LLVMConstNUWAdd(LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstSub", library="LLVM.dll")]
def LLVMConstSub(LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstNSWSub", library="LLVM.dll")]
def LLVMConstNSWSub(LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstNUWSub", library="LLVM.dll")]
def LLVMConstNUWSub(LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstMul", library="LLVM.dll")]
def LLVMConstMul(LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstNSWMul", library="LLVM.dll")]
def LLVMConstNSWMul(LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstNUWMul", library="LLVM.dll")]
def LLVMConstNUWMul(LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstAnd", library="LLVM.dll")]
def LLVMConstAnd(LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstOr", library="LLVM.dll")]
def LLVMConstOr(LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstXor", library="LLVM.dll")]
def LLVMConstXor(LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstICmp", library="LLVM.dll")]
def LLVMConstICmp(Predicate : LLVMIntPredicate; LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstFCmp", library="LLVM.dll")]
def LLVMConstFCmp(Predicate : LLVMRealPredicate; LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstShl", library="LLVM.dll")]
def LLVMConstShl(LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstLShr", library="LLVM.dll")]
def LLVMConstLShr(LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstAShr", library="LLVM.dll")]
def LLVMConstAShr(LHSConstant : LLVMOpaqueValue? implicit; RHSConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstGEP2", library="LLVM.dll")]
def LLVMConstGEP2(Ty : LLVMOpaqueType? implicit; ConstantVal : LLVMOpaqueValue? implicit; ConstantIndices : LLVMOpaqueValue ?? implicit; NumIndices : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstInBoundsGEP2", library="LLVM.dll")]
def LLVMConstInBoundsGEP2(Ty : LLVMOpaqueType? implicit; ConstantVal : LLVMOpaqueValue? implicit; ConstantIndices : LLVMOpaqueValue ?? implicit; NumIndices : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstTrunc", library="LLVM.dll")]
def LLVMConstTrunc(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstSExt", library="LLVM.dll")]
def LLVMConstSExt(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstZExt", library="LLVM.dll")]
def LLVMConstZExt(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstFPTrunc", library="LLVM.dll")]
def LLVMConstFPTrunc(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstFPExt", library="LLVM.dll")]
def LLVMConstFPExt(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstUIToFP", library="LLVM.dll")]
def LLVMConstUIToFP(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstSIToFP", library="LLVM.dll")]
def LLVMConstSIToFP(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstFPToUI", library="LLVM.dll")]
def LLVMConstFPToUI(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstFPToSI", library="LLVM.dll")]
def LLVMConstFPToSI(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstPtrToInt", library="LLVM.dll")]
def LLVMConstPtrToInt(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstIntToPtr", library="LLVM.dll")]
def LLVMConstIntToPtr(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstBitCast", library="LLVM.dll")]
def LLVMConstBitCast(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstAddrSpaceCast", library="LLVM.dll")]
def LLVMConstAddrSpaceCast(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstZExtOrBitCast", library="LLVM.dll")]
def LLVMConstZExtOrBitCast(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstSExtOrBitCast", library="LLVM.dll")]
def LLVMConstSExtOrBitCast(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstTruncOrBitCast", library="LLVM.dll")]
def LLVMConstTruncOrBitCast(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstPointerCast", library="LLVM.dll")]
def LLVMConstPointerCast(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstIntCast", library="LLVM.dll")]
def LLVMConstIntCast(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit; isSigned : int) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstFPCast", library="LLVM.dll")]
def LLVMConstFPCast(ConstantVal : LLVMOpaqueValue? implicit; ToType : LLVMOpaqueType? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstSelect", library="LLVM.dll")]
def LLVMConstSelect(ConstantCondition : LLVMOpaqueValue? implicit; ConstantIfTrue : LLVMOpaqueValue? implicit; ConstantIfFalse : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstExtractElement", library="LLVM.dll")]
def LLVMConstExtractElement(VectorConstant : LLVMOpaqueValue? implicit; IndexConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstInsertElement", library="LLVM.dll")]
def LLVMConstInsertElement(VectorConstant : LLVMOpaqueValue? implicit; ElementValueConstant : LLVMOpaqueValue? implicit; IndexConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstShuffleVector", library="LLVM.dll")]
def LLVMConstShuffleVector(VectorAConstant : LLVMOpaqueValue? implicit; VectorBConstant : LLVMOpaqueValue? implicit; MaskConstant : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBlockAddress", library="LLVM.dll")]
def LLVMBlockAddress(F : LLVMOpaqueValue? implicit; BB : LLVMOpaqueBasicBlock? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMConstInlineAsm", library="LLVM.dll")]
def LLVMConstInlineAsm(Ty : LLVMOpaqueType? implicit; AsmString : string implicit; Constraints : string implicit; HasSideEffects : int; IsAlignStack : int) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetGlobalParent", library="LLVM.dll")]
def LLVMGetGlobalParent(Global : LLVMOpaqueValue? implicit) : LLVMOpaqueModule? {}
[extern(cdecl, name="LLVMIsDeclaration", library="LLVM.dll")]
def LLVMIsDeclaration(Global : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMGetLinkage", library="LLVM.dll")]
def LLVMGetLinkage(Global : LLVMOpaqueValue? implicit) : LLVMLinkage {}
[extern(cdecl, name="LLVMSetLinkage", library="LLVM.dll")]
def LLVMSetLinkage(Global : LLVMOpaqueValue? implicit; Linkage : LLVMLinkage) : void {}
[extern(cdecl, name="LLVMGetSection", library="LLVM.dll")]
def LLVMGetSection(Global : LLVMOpaqueValue? implicit) : string {}
[extern(cdecl, name="LLVMSetSection", library="LLVM.dll")]
def LLVMSetSection(Global : LLVMOpaqueValue? implicit; Section : string implicit) : void {}
[extern(cdecl, name="LLVMGetVisibility", library="LLVM.dll")]
def LLVMGetVisibility(Global : LLVMOpaqueValue? implicit) : LLVMVisibility {}
[extern(cdecl, name="LLVMSetVisibility", library="LLVM.dll")]
def LLVMSetVisibility(Global : LLVMOpaqueValue? implicit; Viz : LLVMVisibility) : void {}
[extern(cdecl, name="LLVMGetDLLStorageClass", library="LLVM.dll")]
def LLVMGetDLLStorageClass(Global : LLVMOpaqueValue? implicit) : LLVMDLLStorageClass {}
[extern(cdecl, name="LLVMSetDLLStorageClass", library="LLVM.dll")]
def LLVMSetDLLStorageClass(Global : LLVMOpaqueValue? implicit; Class : LLVMDLLStorageClass) : void {}
[extern(cdecl, name="LLVMGetUnnamedAddress", library="LLVM.dll")]
def LLVMGetUnnamedAddress(Global : LLVMOpaqueValue? implicit) : LLVMUnnamedAddr {}
[extern(cdecl, name="LLVMSetUnnamedAddress", library="LLVM.dll")]
def LLVMSetUnnamedAddress(Global : LLVMOpaqueValue? implicit; UnnamedAddr : LLVMUnnamedAddr) : void {}
[extern(cdecl, name="LLVMGlobalGetValueType", library="LLVM.dll")]
def LLVMGlobalGetValueType(Global : LLVMOpaqueValue? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMHasUnnamedAddr", library="LLVM.dll")]
def LLVMHasUnnamedAddr(Global : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMSetUnnamedAddr", library="LLVM.dll")]
def LLVMSetUnnamedAddr(Global : LLVMOpaqueValue? implicit; HasUnnamedAddr : int) : void {}
[extern(cdecl, name="LLVMGetAlignment", library="LLVM.dll")]
def LLVMGetAlignment(V : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMSetAlignment", library="LLVM.dll")]
def LLVMSetAlignment(V : LLVMOpaqueValue? implicit; Bytes : uint) : void {}
[extern(cdecl, name="LLVMGlobalSetMetadata", library="LLVM.dll")]
def LLVMGlobalSetMetadata(Global : LLVMOpaqueValue? implicit; Kind : uint; MD : LLVMOpaqueMetadata? implicit) : void {}
[extern(cdecl, name="LLVMGlobalEraseMetadata", library="LLVM.dll")]
def LLVMGlobalEraseMetadata(Global : LLVMOpaqueValue? implicit; Kind : uint) : void {}
[extern(cdecl, name="LLVMGlobalClearMetadata", library="LLVM.dll")]
def LLVMGlobalClearMetadata(Global : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMGlobalCopyAllMetadata", library="LLVM.dll")]
def LLVMGlobalCopyAllMetadata(Value : LLVMOpaqueValue? implicit; NumEntries : uint64? implicit) : LLVMOpaqueValueMetadataEntry? {}
[extern(cdecl, name="LLVMDisposeValueMetadataEntries", library="LLVM.dll")]
def LLVMDisposeValueMetadataEntries(Entries : LLVMOpaqueValueMetadataEntry? implicit) : void {}
[extern(cdecl, name="LLVMValueMetadataEntriesGetKind", library="LLVM.dll")]
def LLVMValueMetadataEntriesGetKind(Entries : LLVMOpaqueValueMetadataEntry? implicit; Index : uint) : uint {}
[extern(cdecl, name="LLVMValueMetadataEntriesGetMetadata", library="LLVM.dll")]
def LLVMValueMetadataEntriesGetMetadata(Entries : LLVMOpaqueValueMetadataEntry? implicit; Index : uint) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMAddGlobal", library="LLVM.dll")]
def LLVMAddGlobal(M : LLVMOpaqueModule? implicit; Ty : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMAddGlobalInAddressSpace", library="LLVM.dll")]
def LLVMAddGlobalInAddressSpace(M : LLVMOpaqueModule? implicit; Ty : LLVMOpaqueType? implicit; Name : string implicit; AddressSpace : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetNamedGlobal", library="LLVM.dll")]
def LLVMGetNamedGlobal(M : LLVMOpaqueModule? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetFirstGlobal", library="LLVM.dll")]
def LLVMGetFirstGlobal(M : LLVMOpaqueModule? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetLastGlobal", library="LLVM.dll")]
def LLVMGetLastGlobal(M : LLVMOpaqueModule? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetNextGlobal", library="LLVM.dll")]
def LLVMGetNextGlobal(GlobalVar : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetPreviousGlobal", library="LLVM.dll")]
def LLVMGetPreviousGlobal(GlobalVar : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMDeleteGlobal", library="LLVM.dll")]
def LLVMDeleteGlobal(GlobalVar : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMGetInitializer", library="LLVM.dll")]
def LLVMGetInitializer(GlobalVar : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMSetInitializer", library="LLVM.dll")]
def LLVMSetInitializer(GlobalVar : LLVMOpaqueValue? implicit; ConstantVal : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMIsThreadLocal", library="LLVM.dll")]
def LLVMIsThreadLocal(GlobalVar : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMSetThreadLocal", library="LLVM.dll")]
def LLVMSetThreadLocal(GlobalVar : LLVMOpaqueValue? implicit; IsThreadLocal : int) : void {}
[extern(cdecl, name="LLVMIsGlobalConstant", library="LLVM.dll")]
def LLVMIsGlobalConstant(GlobalVar : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMSetGlobalConstant", library="LLVM.dll")]
def LLVMSetGlobalConstant(GlobalVar : LLVMOpaqueValue? implicit; IsConstant : int) : void {}
[extern(cdecl, name="LLVMGetThreadLocalMode", library="LLVM.dll")]
def LLVMGetThreadLocalMode(GlobalVar : LLVMOpaqueValue? implicit) : LLVMThreadLocalMode {}
[extern(cdecl, name="LLVMSetThreadLocalMode", library="LLVM.dll")]
def LLVMSetThreadLocalMode(GlobalVar : LLVMOpaqueValue? implicit; Mode : LLVMThreadLocalMode) : void {}
[extern(cdecl, name="LLVMIsExternallyInitialized", library="LLVM.dll")]
def LLVMIsExternallyInitialized(GlobalVar : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMSetExternallyInitialized", library="LLVM.dll")]
def LLVMSetExternallyInitialized(GlobalVar : LLVMOpaqueValue? implicit; IsExtInit : int) : void {}
[extern(cdecl, name="LLVMAddAlias2", library="LLVM.dll")]
def LLVMAddAlias2(M : LLVMOpaqueModule? implicit; ValueTy : LLVMOpaqueType? implicit; AddrSpace : uint; Aliasee : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetNamedGlobalAlias", library="LLVM.dll")]
def LLVMGetNamedGlobalAlias(M : LLVMOpaqueModule? implicit; Name : string implicit; NameLen : uint64) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetFirstGlobalAlias", library="LLVM.dll")]
def LLVMGetFirstGlobalAlias(M : LLVMOpaqueModule? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetLastGlobalAlias", library="LLVM.dll")]
def LLVMGetLastGlobalAlias(M : LLVMOpaqueModule? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetNextGlobalAlias", library="LLVM.dll")]
def LLVMGetNextGlobalAlias(GA : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetPreviousGlobalAlias", library="LLVM.dll")]
def LLVMGetPreviousGlobalAlias(GA : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMAliasGetAliasee", library="LLVM.dll")]
def LLVMAliasGetAliasee(Alias : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMAliasSetAliasee", library="LLVM.dll")]
def LLVMAliasSetAliasee(Alias : LLVMOpaqueValue? implicit; Aliasee : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMDeleteFunction", library="LLVM.dll")]
def LLVMDeleteFunction(Fn : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMHasPersonalityFn", library="LLVM.dll")]
def LLVMHasPersonalityFn(Fn : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMGetPersonalityFn", library="LLVM.dll")]
def LLVMGetPersonalityFn(Fn : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMSetPersonalityFn", library="LLVM.dll")]
def LLVMSetPersonalityFn(Fn : LLVMOpaqueValue? implicit; PersonalityFn : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMLookupIntrinsicID", library="LLVM.dll")]
def LLVMLookupIntrinsicID(Name : string implicit; NameLen : uint64) : uint {}
[extern(cdecl, name="LLVMGetIntrinsicID", library="LLVM.dll")]
def LLVMGetIntrinsicID(Fn : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMGetIntrinsicDeclaration", library="LLVM.dll")]
def LLVMGetIntrinsicDeclaration(Mod : LLVMOpaqueModule? implicit; ID : uint; ParamTypes : LLVMOpaqueType ?? implicit; ParamCount : uint64) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIntrinsicGetType", library="LLVM.dll")]
def LLVMIntrinsicGetType(Ctx : LLVMOpaqueContext? implicit; ID : uint; ParamTypes : LLVMOpaqueType ?? implicit; ParamCount : uint64) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMIntrinsicGetName", library="LLVM.dll")]
def LLVMIntrinsicGetName(ID : uint; NameLength : uint64? implicit) : string {}
[extern(cdecl, name="LLVMIntrinsicCopyOverloadedName", library="LLVM.dll")]
def LLVMIntrinsicCopyOverloadedName(ID : uint; ParamTypes : LLVMOpaqueType ?? implicit; ParamCount : uint64; NameLength : uint64? implicit) : string {}
[extern(cdecl, name="LLVMIntrinsicCopyOverloadedName2", library="LLVM.dll")]
def LLVMIntrinsicCopyOverloadedName2(Mod : LLVMOpaqueModule? implicit; ID : uint; ParamTypes : LLVMOpaqueType ?? implicit; ParamCount : uint64; NameLength : uint64? implicit) : string {}
[extern(cdecl, name="LLVMIntrinsicIsOverloaded", library="LLVM.dll")]
def LLVMIntrinsicIsOverloaded(ID : uint) : int {}
[extern(cdecl, name="LLVMGetFunctionCallConv", library="LLVM.dll")]
def LLVMGetFunctionCallConv(Fn : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMSetFunctionCallConv", library="LLVM.dll")]
def LLVMSetFunctionCallConv(Fn : LLVMOpaqueValue? implicit; CC : uint) : void {}
[extern(cdecl, name="LLVMGetGC", library="LLVM.dll")]
def LLVMGetGC(Fn : LLVMOpaqueValue? implicit) : string {}
[extern(cdecl, name="LLVMSetGC", library="LLVM.dll")]
def LLVMSetGC(Fn : LLVMOpaqueValue? implicit; Name : string implicit) : void {}
[extern(cdecl, name="LLVMAddAttributeAtIndex", library="LLVM.dll")]
def LLVMAddAttributeAtIndex(F : LLVMOpaqueValue? implicit; Idx : uint; A : LLVMOpaqueAttributeRef? implicit) : void {}
[extern(cdecl, name="LLVMGetAttributeCountAtIndex", library="LLVM.dll")]
def LLVMGetAttributeCountAtIndex(F : LLVMOpaqueValue? implicit; Idx : uint) : uint {}
[extern(cdecl, name="LLVMGetAttributesAtIndex", library="LLVM.dll")]
def LLVMGetAttributesAtIndex(F : LLVMOpaqueValue? implicit; Idx : uint; Attrs : LLVMOpaqueAttributeRef ?? implicit) : void {}
[extern(cdecl, name="LLVMGetEnumAttributeAtIndex", library="LLVM.dll")]
def LLVMGetEnumAttributeAtIndex(F : LLVMOpaqueValue? implicit; Idx : uint; KindID : uint) : LLVMOpaqueAttributeRef? {}
[extern(cdecl, name="LLVMGetStringAttributeAtIndex", library="LLVM.dll")]
def LLVMGetStringAttributeAtIndex(F : LLVMOpaqueValue? implicit; Idx : uint; K : string implicit; KLen : uint) : LLVMOpaqueAttributeRef? {}
[extern(cdecl, name="LLVMRemoveEnumAttributeAtIndex", library="LLVM.dll")]
def LLVMRemoveEnumAttributeAtIndex(F : LLVMOpaqueValue? implicit; Idx : uint; KindID : uint) : void {}
[extern(cdecl, name="LLVMRemoveStringAttributeAtIndex", library="LLVM.dll")]
def LLVMRemoveStringAttributeAtIndex(F : LLVMOpaqueValue? implicit; Idx : uint; K : string implicit; KLen : uint) : void {}
[extern(cdecl, name="LLVMAddTargetDependentFunctionAttr", library="LLVM.dll")]
def LLVMAddTargetDependentFunctionAttr(Fn : LLVMOpaqueValue? implicit; A : string implicit; V : string implicit) : void {}
[extern(cdecl, name="LLVMCountParams", library="LLVM.dll")]
def LLVMCountParams(Fn : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMGetParams", library="LLVM.dll")]
def LLVMGetParams(Fn : LLVMOpaqueValue? implicit; Params : LLVMOpaqueValue ?? implicit) : void {}
[extern(cdecl, name="LLVMGetParam", library="LLVM.dll")]
def LLVMGetParam(Fn : LLVMOpaqueValue? implicit; Index : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetParamParent", library="LLVM.dll")]
def LLVMGetParamParent(Inst : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetFirstParam", library="LLVM.dll")]
def LLVMGetFirstParam(Fn : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetLastParam", library="LLVM.dll")]
def LLVMGetLastParam(Fn : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetNextParam", library="LLVM.dll")]
def LLVMGetNextParam(Arg : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetPreviousParam", library="LLVM.dll")]
def LLVMGetPreviousParam(Arg : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMSetParamAlignment", library="LLVM.dll")]
def LLVMSetParamAlignment(Arg : LLVMOpaqueValue? implicit; Align : uint) : void {}
[extern(cdecl, name="LLVMAddGlobalIFunc", library="LLVM.dll")]
def LLVMAddGlobalIFunc(M : LLVMOpaqueModule? implicit; Name : string implicit; NameLen : uint64; Ty : LLVMOpaqueType? implicit; AddrSpace : uint; Resolver : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetNamedGlobalIFunc", library="LLVM.dll")]
def LLVMGetNamedGlobalIFunc(M : LLVMOpaqueModule? implicit; Name : string implicit; NameLen : uint64) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetFirstGlobalIFunc", library="LLVM.dll")]
def LLVMGetFirstGlobalIFunc(M : LLVMOpaqueModule? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetLastGlobalIFunc", library="LLVM.dll")]
def LLVMGetLastGlobalIFunc(M : LLVMOpaqueModule? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetNextGlobalIFunc", library="LLVM.dll")]
def LLVMGetNextGlobalIFunc(IFunc : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetPreviousGlobalIFunc", library="LLVM.dll")]
def LLVMGetPreviousGlobalIFunc(IFunc : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetGlobalIFuncResolver", library="LLVM.dll")]
def LLVMGetGlobalIFuncResolver(IFunc : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMSetGlobalIFuncResolver", library="LLVM.dll")]
def LLVMSetGlobalIFuncResolver(IFunc : LLVMOpaqueValue? implicit; Resolver : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMEraseGlobalIFunc", library="LLVM.dll")]
def LLVMEraseGlobalIFunc(IFunc : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMRemoveGlobalIFunc", library="LLVM.dll")]
def LLVMRemoveGlobalIFunc(IFunc : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMMDStringInContext2", library="LLVM.dll")]
def LLVMMDStringInContext2(C : LLVMOpaqueContext? implicit; Str : string implicit; SLen : uint64) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMMDNodeInContext2", library="LLVM.dll")]
def LLVMMDNodeInContext2(C : LLVMOpaqueContext? implicit; MDs : LLVMOpaqueMetadata ?? implicit; Count : uint64) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMMetadataAsValue", library="LLVM.dll")]
def LLVMMetadataAsValue(C : LLVMOpaqueContext? implicit; MD : LLVMOpaqueMetadata? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMValueAsMetadata", library="LLVM.dll")]
def LLVMValueAsMetadata(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMGetMDString", library="LLVM.dll")]
def LLVMGetMDString(V : LLVMOpaqueValue? implicit; Length : uint? implicit) : string {}
[extern(cdecl, name="LLVMGetMDNodeNumOperands", library="LLVM.dll")]
def LLVMGetMDNodeNumOperands(V : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMGetMDNodeOperands", library="LLVM.dll")]
def LLVMGetMDNodeOperands(V : LLVMOpaqueValue? implicit; Dest : LLVMOpaqueValue ?? implicit) : void {}
[extern(cdecl, name="LLVMMDStringInContext", library="LLVM.dll")]
def LLVMMDStringInContext(C : LLVMOpaqueContext? implicit; Str : string implicit; SLen : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMMDString", library="LLVM.dll")]
def LLVMMDString(Str : string implicit; SLen : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMMDNodeInContext", library="LLVM.dll")]
def LLVMMDNodeInContext(C : LLVMOpaqueContext? implicit; Vals : LLVMOpaqueValue ?? implicit; Count : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMMDNode", library="LLVM.dll")]
def LLVMMDNode(Vals : LLVMOpaqueValue ?? implicit; Count : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBasicBlockAsValue", library="LLVM.dll")]
def LLVMBasicBlockAsValue(BB : LLVMOpaqueBasicBlock? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMValueIsBasicBlock", library="LLVM.dll")]
def LLVMValueIsBasicBlock(Val : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMValueAsBasicBlock", library="LLVM.dll")]
def LLVMValueAsBasicBlock(Val : LLVMOpaqueValue? implicit) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMGetBasicBlockName", library="LLVM.dll")]
def LLVMGetBasicBlockName(BB : LLVMOpaqueBasicBlock? implicit) : string {}
[extern(cdecl, name="LLVMGetBasicBlockParent", library="LLVM.dll")]
def LLVMGetBasicBlockParent(BB : LLVMOpaqueBasicBlock? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetBasicBlockTerminator", library="LLVM.dll")]
def LLVMGetBasicBlockTerminator(BB : LLVMOpaqueBasicBlock? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMCountBasicBlocks", library="LLVM.dll")]
def LLVMCountBasicBlocks(Fn : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMGetBasicBlocks", library="LLVM.dll")]
def LLVMGetBasicBlocks(Fn : LLVMOpaqueValue? implicit; BasicBlocks : LLVMOpaqueBasicBlock ?? implicit) : void {}
[extern(cdecl, name="LLVMGetFirstBasicBlock", library="LLVM.dll")]
def LLVMGetFirstBasicBlock(Fn : LLVMOpaqueValue? implicit) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMGetLastBasicBlock", library="LLVM.dll")]
def LLVMGetLastBasicBlock(Fn : LLVMOpaqueValue? implicit) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMGetNextBasicBlock", library="LLVM.dll")]
def LLVMGetNextBasicBlock(BB : LLVMOpaqueBasicBlock? implicit) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMGetPreviousBasicBlock", library="LLVM.dll")]
def LLVMGetPreviousBasicBlock(BB : LLVMOpaqueBasicBlock? implicit) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMGetEntryBasicBlock", library="LLVM.dll")]
def LLVMGetEntryBasicBlock(Fn : LLVMOpaqueValue? implicit) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMInsertExistingBasicBlockAfterInsertBlock", library="LLVM.dll")]
def LLVMInsertExistingBasicBlockAfterInsertBlock(Builder : LLVMOpaqueBuilder? implicit; BB : LLVMOpaqueBasicBlock? implicit) : void {}
[extern(cdecl, name="LLVMAppendExistingBasicBlock", library="LLVM.dll")]
def LLVMAppendExistingBasicBlock(Fn : LLVMOpaqueValue? implicit; BB : LLVMOpaqueBasicBlock? implicit) : void {}
[extern(cdecl, name="LLVMCreateBasicBlockInContext", library="LLVM.dll")]
def LLVMCreateBasicBlockInContext(C : LLVMOpaqueContext? implicit; Name : string implicit) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMAppendBasicBlockInContext", library="LLVM.dll")]
def LLVMAppendBasicBlockInContext(C : LLVMOpaqueContext? implicit; Fn : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMAppendBasicBlock", library="LLVM.dll")]
def LLVMAppendBasicBlock(Fn : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMInsertBasicBlockInContext", library="LLVM.dll")]
def LLVMInsertBasicBlockInContext(C : LLVMOpaqueContext? implicit; BB : LLVMOpaqueBasicBlock? implicit; Name : string implicit) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMInsertBasicBlock", library="LLVM.dll")]
def LLVMInsertBasicBlock(InsertBeforeBB : LLVMOpaqueBasicBlock? implicit; Name : string implicit) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMDeleteBasicBlock", library="LLVM.dll")]
def LLVMDeleteBasicBlock(BB : LLVMOpaqueBasicBlock? implicit) : void {}
[extern(cdecl, name="LLVMRemoveBasicBlockFromParent", library="LLVM.dll")]
def LLVMRemoveBasicBlockFromParent(BB : LLVMOpaqueBasicBlock? implicit) : void {}
[extern(cdecl, name="LLVMMoveBasicBlockBefore", library="LLVM.dll")]
def LLVMMoveBasicBlockBefore(BB : LLVMOpaqueBasicBlock? implicit; MovePos : LLVMOpaqueBasicBlock? implicit) : void {}
[extern(cdecl, name="LLVMMoveBasicBlockAfter", library="LLVM.dll")]
def LLVMMoveBasicBlockAfter(BB : LLVMOpaqueBasicBlock? implicit; MovePos : LLVMOpaqueBasicBlock? implicit) : void {}
[extern(cdecl, name="LLVMGetFirstInstruction", library="LLVM.dll")]
def LLVMGetFirstInstruction(BB : LLVMOpaqueBasicBlock? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetLastInstruction", library="LLVM.dll")]
def LLVMGetLastInstruction(BB : LLVMOpaqueBasicBlock? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMHasMetadata", library="LLVM.dll")]
def LLVMHasMetadata(Val : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMGetMetadata", library="LLVM.dll")]
def LLVMGetMetadata(Val : LLVMOpaqueValue? implicit; KindID : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMSetMetadata", library="LLVM.dll")]
def LLVMSetMetadata(Val : LLVMOpaqueValue? implicit; KindID : uint; Node : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMInstructionGetAllMetadataOtherThanDebugLoc", library="LLVM.dll")]
def LLVMInstructionGetAllMetadataOtherThanDebugLoc(Instr : LLVMOpaqueValue? implicit; NumEntries : uint64? implicit) : LLVMOpaqueValueMetadataEntry? {}
[extern(cdecl, name="LLVMGetInstructionParent", library="LLVM.dll")]
def LLVMGetInstructionParent(Inst : LLVMOpaqueValue? implicit) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMGetNextInstruction", library="LLVM.dll")]
def LLVMGetNextInstruction(Inst : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetPreviousInstruction", library="LLVM.dll")]
def LLVMGetPreviousInstruction(Inst : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMInstructionRemoveFromParent", library="LLVM.dll")]
def LLVMInstructionRemoveFromParent(Inst : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMInstructionEraseFromParent", library="LLVM.dll")]
def LLVMInstructionEraseFromParent(Inst : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMDeleteInstruction", library="LLVM.dll")]
def LLVMDeleteInstruction(Inst : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMGetInstructionOpcode", library="LLVM.dll")]
def LLVMGetInstructionOpcode(Inst : LLVMOpaqueValue? implicit) : LLVMOpcode {}
[extern(cdecl, name="LLVMGetICmpPredicate", library="LLVM.dll")]
def LLVMGetICmpPredicate(Inst : LLVMOpaqueValue? implicit) : LLVMIntPredicate {}
[extern(cdecl, name="LLVMGetFCmpPredicate", library="LLVM.dll")]
def LLVMGetFCmpPredicate(Inst : LLVMOpaqueValue? implicit) : LLVMRealPredicate {}
[extern(cdecl, name="LLVMInstructionClone", library="LLVM.dll")]
def LLVMInstructionClone(Inst : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsATerminatorInst", library="LLVM.dll")]
def LLVMIsATerminatorInst(Inst : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetNumArgOperands", library="LLVM.dll")]
def LLVMGetNumArgOperands(Instr : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMSetInstructionCallConv", library="LLVM.dll")]
def LLVMSetInstructionCallConv(Instr : LLVMOpaqueValue? implicit; CC : uint) : void {}
[extern(cdecl, name="LLVMGetInstructionCallConv", library="LLVM.dll")]
def LLVMGetInstructionCallConv(Instr : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMSetInstrParamAlignment", library="LLVM.dll")]
def LLVMSetInstrParamAlignment(Instr : LLVMOpaqueValue? implicit; Idx : uint; Align : uint) : void {}
[extern(cdecl, name="LLVMAddCallSiteAttribute", library="LLVM.dll")]
def LLVMAddCallSiteAttribute(C : LLVMOpaqueValue? implicit; Idx : uint; A : LLVMOpaqueAttributeRef? implicit) : void {}
[extern(cdecl, name="LLVMGetCallSiteAttributeCount", library="LLVM.dll")]
def LLVMGetCallSiteAttributeCount(C : LLVMOpaqueValue? implicit; Idx : uint) : uint {}
[extern(cdecl, name="LLVMGetCallSiteAttributes", library="LLVM.dll")]
def LLVMGetCallSiteAttributes(C : LLVMOpaqueValue? implicit; Idx : uint; Attrs : LLVMOpaqueAttributeRef ?? implicit) : void {}
[extern(cdecl, name="LLVMGetCallSiteEnumAttribute", library="LLVM.dll")]
def LLVMGetCallSiteEnumAttribute(C : LLVMOpaqueValue? implicit; Idx : uint; KindID : uint) : LLVMOpaqueAttributeRef? {}
[extern(cdecl, name="LLVMGetCallSiteStringAttribute", library="LLVM.dll")]
def LLVMGetCallSiteStringAttribute(C : LLVMOpaqueValue? implicit; Idx : uint; K : string implicit; KLen : uint) : LLVMOpaqueAttributeRef? {}
[extern(cdecl, name="LLVMRemoveCallSiteEnumAttribute", library="LLVM.dll")]
def LLVMRemoveCallSiteEnumAttribute(C : LLVMOpaqueValue? implicit; Idx : uint; KindID : uint) : void {}
[extern(cdecl, name="LLVMRemoveCallSiteStringAttribute", library="LLVM.dll")]
def LLVMRemoveCallSiteStringAttribute(C : LLVMOpaqueValue? implicit; Idx : uint; K : string implicit; KLen : uint) : void {}
[extern(cdecl, name="LLVMGetCalledFunctionType", library="LLVM.dll")]
def LLVMGetCalledFunctionType(C : LLVMOpaqueValue? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMGetCalledValue", library="LLVM.dll")]
def LLVMGetCalledValue(Instr : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMIsTailCall", library="LLVM.dll")]
def LLVMIsTailCall(CallInst : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMSetTailCall", library="LLVM.dll")]
def LLVMSetTailCall(CallInst : LLVMOpaqueValue? implicit; IsTailCall : int) : void {}
[extern(cdecl, name="LLVMGetNormalDest", library="LLVM.dll")]
def LLVMGetNormalDest(InvokeInst : LLVMOpaqueValue? implicit) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMGetUnwindDest", library="LLVM.dll")]
def LLVMGetUnwindDest(InvokeInst : LLVMOpaqueValue? implicit) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMSetNormalDest", library="LLVM.dll")]
def LLVMSetNormalDest(InvokeInst : LLVMOpaqueValue? implicit; B : LLVMOpaqueBasicBlock? implicit) : void {}
[extern(cdecl, name="LLVMSetUnwindDest", library="LLVM.dll")]
def LLVMSetUnwindDest(InvokeInst : LLVMOpaqueValue? implicit; B : LLVMOpaqueBasicBlock? implicit) : void {}
[extern(cdecl, name="LLVMGetNumSuccessors", library="LLVM.dll")]
def LLVMGetNumSuccessors(Term : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMGetSuccessor", library="LLVM.dll")]
def LLVMGetSuccessor(Term : LLVMOpaqueValue? implicit; i : uint) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMSetSuccessor", library="LLVM.dll")]
def LLVMSetSuccessor(Term : LLVMOpaqueValue? implicit; i : uint; _block : LLVMOpaqueBasicBlock? implicit) : void {}
[extern(cdecl, name="LLVMIsConditional", library="LLVM.dll")]
def LLVMIsConditional(Branch : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMGetCondition", library="LLVM.dll")]
def LLVMGetCondition(Branch : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMSetCondition", library="LLVM.dll")]
def LLVMSetCondition(Branch : LLVMOpaqueValue? implicit; Cond : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMGetSwitchDefaultDest", library="LLVM.dll")]
def LLVMGetSwitchDefaultDest(SwitchInstr : LLVMOpaqueValue? implicit) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMGetAllocatedType", library="LLVM.dll")]
def LLVMGetAllocatedType(Alloca : LLVMOpaqueValue? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMIsInBounds", library="LLVM.dll")]
def LLVMIsInBounds(GEP : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMSetIsInBounds", library="LLVM.dll")]
def LLVMSetIsInBounds(GEP : LLVMOpaqueValue? implicit; InBounds : int) : void {}
[extern(cdecl, name="LLVMGetGEPSourceElementType", library="LLVM.dll")]
def LLVMGetGEPSourceElementType(GEP : LLVMOpaqueValue? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMAddIncoming", library="LLVM.dll")]
def LLVMAddIncoming(PhiNode : LLVMOpaqueValue? implicit; IncomingValues : LLVMOpaqueValue ?? implicit; IncomingBlocks : LLVMOpaqueBasicBlock ?? implicit; Count : uint) : void {}
[extern(cdecl, name="LLVMCountIncoming", library="LLVM.dll")]
def LLVMCountIncoming(PhiNode : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMGetIncomingValue", library="LLVM.dll")]
def LLVMGetIncomingValue(PhiNode : LLVMOpaqueValue? implicit; Index : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetIncomingBlock", library="LLVM.dll")]
def LLVMGetIncomingBlock(PhiNode : LLVMOpaqueValue? implicit; Index : uint) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMGetNumIndices", library="LLVM.dll")]
def LLVMGetNumIndices(Inst : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMGetIndices", library="LLVM.dll")]
def LLVMGetIndices(Inst : LLVMOpaqueValue? implicit) : uint const? {}
[extern(cdecl, name="LLVMCreateBuilderInContext", library="LLVM.dll")]
def LLVMCreateBuilderInContext(C : LLVMOpaqueContext? implicit) : LLVMOpaqueBuilder? {}
[extern(cdecl, name="LLVMCreateBuilder", library="LLVM.dll")]
def LLVMCreateBuilder() : LLVMOpaqueBuilder? {}
[extern(cdecl, name="LLVMPositionBuilder", library="LLVM.dll")]
def LLVMPositionBuilder(Builder : LLVMOpaqueBuilder? implicit; Block : LLVMOpaqueBasicBlock? implicit; Instr : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMPositionBuilderBefore", library="LLVM.dll")]
def LLVMPositionBuilderBefore(Builder : LLVMOpaqueBuilder? implicit; Instr : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMPositionBuilderAtEnd", library="LLVM.dll")]
def LLVMPositionBuilderAtEnd(Builder : LLVMOpaqueBuilder? implicit; Block : LLVMOpaqueBasicBlock? implicit) : void {}
[extern(cdecl, name="LLVMGetInsertBlock", library="LLVM.dll")]
def LLVMGetInsertBlock(Builder : LLVMOpaqueBuilder? implicit) : LLVMOpaqueBasicBlock? {}
[extern(cdecl, name="LLVMClearInsertionPosition", library="LLVM.dll")]
def LLVMClearInsertionPosition(Builder : LLVMOpaqueBuilder? implicit) : void {}
[extern(cdecl, name="LLVMInsertIntoBuilder", library="LLVM.dll")]
def LLVMInsertIntoBuilder(Builder : LLVMOpaqueBuilder? implicit; Instr : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMInsertIntoBuilderWithName", library="LLVM.dll")]
def LLVMInsertIntoBuilderWithName(Builder : LLVMOpaqueBuilder? implicit; Instr : LLVMOpaqueValue? implicit; Name : string implicit) : void {}
[extern(cdecl, name="LLVMDisposeBuilder", library="LLVM.dll")]
def LLVMDisposeBuilder(Builder : LLVMOpaqueBuilder? implicit) : void {}
[extern(cdecl, name="LLVMGetCurrentDebugLocation2", library="LLVM.dll")]
def LLVMGetCurrentDebugLocation2(Builder : LLVMOpaqueBuilder? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMSetCurrentDebugLocation2", library="LLVM.dll")]
def LLVMSetCurrentDebugLocation2(Builder : LLVMOpaqueBuilder? implicit; Loc : LLVMOpaqueMetadata? implicit) : void {}
[extern(cdecl, name="LLVMSetInstDebugLocation", library="LLVM.dll")]
def LLVMSetInstDebugLocation(Builder : LLVMOpaqueBuilder? implicit; Inst : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMAddMetadataToInst", library="LLVM.dll")]
def LLVMAddMetadataToInst(Builder : LLVMOpaqueBuilder? implicit; Inst : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMBuilderGetDefaultFPMathTag", library="LLVM.dll")]
def LLVMBuilderGetDefaultFPMathTag(Builder : LLVMOpaqueBuilder? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMBuilderSetDefaultFPMathTag", library="LLVM.dll")]
def LLVMBuilderSetDefaultFPMathTag(Builder : LLVMOpaqueBuilder? implicit; FPMathTag : LLVMOpaqueMetadata? implicit) : void {}
[extern(cdecl, name="LLVMSetCurrentDebugLocation", library="LLVM.dll")]
def LLVMSetCurrentDebugLocation(Builder : LLVMOpaqueBuilder? implicit; L : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMGetCurrentDebugLocation", library="LLVM.dll")]
def LLVMGetCurrentDebugLocation(Builder : LLVMOpaqueBuilder? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildRetVoid", library="LLVM.dll")]
def LLVMBuildRetVoid(_argument_0x0 : LLVMOpaqueBuilder? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildRet", library="LLVM.dll")]
def LLVMBuildRet(_argument_0x0 : LLVMOpaqueBuilder? implicit; V : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildAggregateRet", library="LLVM.dll")]
def LLVMBuildAggregateRet(_argument_0x0 : LLVMOpaqueBuilder? implicit; RetVals : LLVMOpaqueValue ?? implicit; N : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildBr", library="LLVM.dll")]
def LLVMBuildBr(_argument_0x0 : LLVMOpaqueBuilder? implicit; Dest : LLVMOpaqueBasicBlock? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildCondBr", library="LLVM.dll")]
def LLVMBuildCondBr(_argument_0x0 : LLVMOpaqueBuilder? implicit; If : LLVMOpaqueValue? implicit; Then : LLVMOpaqueBasicBlock? implicit; Else : LLVMOpaqueBasicBlock? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildSwitch", library="LLVM.dll")]
def LLVMBuildSwitch(_argument_0x0 : LLVMOpaqueBuilder? implicit; V : LLVMOpaqueValue? implicit; Else : LLVMOpaqueBasicBlock? implicit; NumCases : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildIndirectBr", library="LLVM.dll")]
def LLVMBuildIndirectBr(B : LLVMOpaqueBuilder? implicit; Addr : LLVMOpaqueValue? implicit; NumDests : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildInvoke2", library="LLVM.dll")]
def LLVMBuildInvoke2(_argument_0x0 : LLVMOpaqueBuilder? implicit; Ty : LLVMOpaqueType? implicit; Fn : LLVMOpaqueValue? implicit; Args : LLVMOpaqueValue ?? implicit; NumArgs : uint; Then : LLVMOpaqueBasicBlock? implicit; Catch : LLVMOpaqueBasicBlock? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildUnreachable", library="LLVM.dll")]
def LLVMBuildUnreachable(_argument_0x0 : LLVMOpaqueBuilder? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildResume", library="LLVM.dll")]
def LLVMBuildResume(B : LLVMOpaqueBuilder? implicit; Exn : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildLandingPad", library="LLVM.dll")]
def LLVMBuildLandingPad(B : LLVMOpaqueBuilder? implicit; Ty : LLVMOpaqueType? implicit; PersFn : LLVMOpaqueValue? implicit; NumClauses : uint; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildCleanupRet", library="LLVM.dll")]
def LLVMBuildCleanupRet(B : LLVMOpaqueBuilder? implicit; CatchPad : LLVMOpaqueValue? implicit; BB : LLVMOpaqueBasicBlock? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildCatchRet", library="LLVM.dll")]
def LLVMBuildCatchRet(B : LLVMOpaqueBuilder? implicit; CatchPad : LLVMOpaqueValue? implicit; BB : LLVMOpaqueBasicBlock? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildCatchPad", library="LLVM.dll")]
def LLVMBuildCatchPad(B : LLVMOpaqueBuilder? implicit; ParentPad : LLVMOpaqueValue? implicit; Args : LLVMOpaqueValue ?? implicit; NumArgs : uint; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildCleanupPad", library="LLVM.dll")]
def LLVMBuildCleanupPad(B : LLVMOpaqueBuilder? implicit; ParentPad : LLVMOpaqueValue? implicit; Args : LLVMOpaqueValue ?? implicit; NumArgs : uint; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildCatchSwitch", library="LLVM.dll")]
def LLVMBuildCatchSwitch(B : LLVMOpaqueBuilder? implicit; ParentPad : LLVMOpaqueValue? implicit; UnwindBB : LLVMOpaqueBasicBlock? implicit; NumHandlers : uint; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMAddCase", library="LLVM.dll")]
def LLVMAddCase(Switch : LLVMOpaqueValue? implicit; OnVal : LLVMOpaqueValue? implicit; Dest : LLVMOpaqueBasicBlock? implicit) : void {}
[extern(cdecl, name="LLVMAddDestination", library="LLVM.dll")]
def LLVMAddDestination(IndirectBr : LLVMOpaqueValue? implicit; Dest : LLVMOpaqueBasicBlock? implicit) : void {}
[extern(cdecl, name="LLVMGetNumClauses", library="LLVM.dll")]
def LLVMGetNumClauses(LandingPad : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMGetClause", library="LLVM.dll")]
def LLVMGetClause(LandingPad : LLVMOpaqueValue? implicit; Idx : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMAddClause", library="LLVM.dll")]
def LLVMAddClause(LandingPad : LLVMOpaqueValue? implicit; ClauseVal : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMIsCleanup", library="LLVM.dll")]
def LLVMIsCleanup(LandingPad : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMSetCleanup", library="LLVM.dll")]
def LLVMSetCleanup(LandingPad : LLVMOpaqueValue? implicit; Val : int) : void {}
[extern(cdecl, name="LLVMAddHandler", library="LLVM.dll")]
def LLVMAddHandler(CatchSwitch : LLVMOpaqueValue? implicit; Dest : LLVMOpaqueBasicBlock? implicit) : void {}
[extern(cdecl, name="LLVMGetNumHandlers", library="LLVM.dll")]
def LLVMGetNumHandlers(CatchSwitch : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMGetHandlers", library="LLVM.dll")]
def LLVMGetHandlers(CatchSwitch : LLVMOpaqueValue? implicit; Handlers : LLVMOpaqueBasicBlock ?? implicit) : void {}
[extern(cdecl, name="LLVMGetArgOperand", library="LLVM.dll")]
def LLVMGetArgOperand(Funclet : LLVMOpaqueValue? implicit; i : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMSetArgOperand", library="LLVM.dll")]
def LLVMSetArgOperand(Funclet : LLVMOpaqueValue? implicit; i : uint; value : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMGetParentCatchSwitch", library="LLVM.dll")]
def LLVMGetParentCatchSwitch(CatchPad : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMSetParentCatchSwitch", library="LLVM.dll")]
def LLVMSetParentCatchSwitch(CatchPad : LLVMOpaqueValue? implicit; CatchSwitch : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMBuildAdd", library="LLVM.dll")]
def LLVMBuildAdd(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildNSWAdd", library="LLVM.dll")]
def LLVMBuildNSWAdd(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildNUWAdd", library="LLVM.dll")]
def LLVMBuildNUWAdd(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildFAdd", library="LLVM.dll")]
def LLVMBuildFAdd(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildSub", library="LLVM.dll")]
def LLVMBuildSub(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildNSWSub", library="LLVM.dll")]
def LLVMBuildNSWSub(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildNUWSub", library="LLVM.dll")]
def LLVMBuildNUWSub(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildFSub", library="LLVM.dll")]
def LLVMBuildFSub(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildMul", library="LLVM.dll")]
def LLVMBuildMul(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildNSWMul", library="LLVM.dll")]
def LLVMBuildNSWMul(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildNUWMul", library="LLVM.dll")]
def LLVMBuildNUWMul(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildFMul", library="LLVM.dll")]
def LLVMBuildFMul(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildUDiv", library="LLVM.dll")]
def LLVMBuildUDiv(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildExactUDiv", library="LLVM.dll")]
def LLVMBuildExactUDiv(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildSDiv", library="LLVM.dll")]
def LLVMBuildSDiv(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildExactSDiv", library="LLVM.dll")]
def LLVMBuildExactSDiv(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildFDiv", library="LLVM.dll")]
def LLVMBuildFDiv(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildURem", library="LLVM.dll")]
def LLVMBuildURem(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildSRem", library="LLVM.dll")]
def LLVMBuildSRem(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildFRem", library="LLVM.dll")]
def LLVMBuildFRem(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildShl", library="LLVM.dll")]
def LLVMBuildShl(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildLShr", library="LLVM.dll")]
def LLVMBuildLShr(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildAShr", library="LLVM.dll")]
def LLVMBuildAShr(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildAnd", library="LLVM.dll")]
def LLVMBuildAnd(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildOr", library="LLVM.dll")]
def LLVMBuildOr(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildXor", library="LLVM.dll")]
def LLVMBuildXor(_argument_0x0 : LLVMOpaqueBuilder? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildBinOp", library="LLVM.dll")]
def LLVMBuildBinOp(B : LLVMOpaqueBuilder? implicit; Op : LLVMOpcode; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildNeg", library="LLVM.dll")]
def LLVMBuildNeg(_argument_0x0 : LLVMOpaqueBuilder? implicit; V : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildNSWNeg", library="LLVM.dll")]
def LLVMBuildNSWNeg(B : LLVMOpaqueBuilder? implicit; V : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildNUWNeg", library="LLVM.dll")]
def LLVMBuildNUWNeg(B : LLVMOpaqueBuilder? implicit; V : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildFNeg", library="LLVM.dll")]
def LLVMBuildFNeg(_argument_0x0 : LLVMOpaqueBuilder? implicit; V : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildNot", library="LLVM.dll")]
def LLVMBuildNot(_argument_0x0 : LLVMOpaqueBuilder? implicit; V : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildMalloc", library="LLVM.dll")]
def LLVMBuildMalloc(_argument_0x0 : LLVMOpaqueBuilder? implicit; Ty : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildArrayMalloc", library="LLVM.dll")]
def LLVMBuildArrayMalloc(_argument_0x0 : LLVMOpaqueBuilder? implicit; Ty : LLVMOpaqueType? implicit; Val : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildMemSet", library="LLVM.dll")]
def LLVMBuildMemSet(B : LLVMOpaqueBuilder? implicit; Ptr : LLVMOpaqueValue? implicit; Val : LLVMOpaqueValue? implicit; Len : LLVMOpaqueValue? implicit; Align : uint) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildMemCpy", library="LLVM.dll")]
def LLVMBuildMemCpy(B : LLVMOpaqueBuilder? implicit; Dst : LLVMOpaqueValue? implicit; DstAlign : uint; Src : LLVMOpaqueValue? implicit; SrcAlign : uint; Size : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildMemMove", library="LLVM.dll")]
def LLVMBuildMemMove(B : LLVMOpaqueBuilder? implicit; Dst : LLVMOpaqueValue? implicit; DstAlign : uint; Src : LLVMOpaqueValue? implicit; SrcAlign : uint; Size : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildAlloca", library="LLVM.dll")]
def LLVMBuildAlloca(_argument_0x0 : LLVMOpaqueBuilder? implicit; Ty : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildArrayAlloca", library="LLVM.dll")]
def LLVMBuildArrayAlloca(_argument_0x0 : LLVMOpaqueBuilder? implicit; Ty : LLVMOpaqueType? implicit; Val : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildFree", library="LLVM.dll")]
def LLVMBuildFree(_argument_0x0 : LLVMOpaqueBuilder? implicit; PointerVal : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildLoad2", library="LLVM.dll")]
def LLVMBuildLoad2(_argument_0x0 : LLVMOpaqueBuilder? implicit; Ty : LLVMOpaqueType? implicit; PointerVal : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildStore", library="LLVM.dll")]
def LLVMBuildStore(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; Ptr : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildGEP2", library="LLVM.dll")]
def LLVMBuildGEP2(B : LLVMOpaqueBuilder? implicit; Ty : LLVMOpaqueType? implicit; Pointer : LLVMOpaqueValue? implicit; Indices : LLVMOpaqueValue ?? implicit; NumIndices : uint; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildInBoundsGEP2", library="LLVM.dll")]
def LLVMBuildInBoundsGEP2(B : LLVMOpaqueBuilder? implicit; Ty : LLVMOpaqueType? implicit; Pointer : LLVMOpaqueValue? implicit; Indices : LLVMOpaqueValue ?? implicit; NumIndices : uint; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildStructGEP2", library="LLVM.dll")]
def LLVMBuildStructGEP2(B : LLVMOpaqueBuilder? implicit; Ty : LLVMOpaqueType? implicit; Pointer : LLVMOpaqueValue? implicit; Idx : uint; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildGlobalString", library="LLVM.dll")]
def LLVMBuildGlobalString(B : LLVMOpaqueBuilder? implicit; Str : string implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildGlobalStringPtr", library="LLVM.dll")]
def LLVMBuildGlobalStringPtr(B : LLVMOpaqueBuilder? implicit; Str : string implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetVolatile", library="LLVM.dll")]
def LLVMGetVolatile(MemoryAccessInst : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMSetVolatile", library="LLVM.dll")]
def LLVMSetVolatile(MemoryAccessInst : LLVMOpaqueValue? implicit; IsVolatile : int) : void {}
[extern(cdecl, name="LLVMGetWeak", library="LLVM.dll")]
def LLVMGetWeak(CmpXchgInst : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMSetWeak", library="LLVM.dll")]
def LLVMSetWeak(CmpXchgInst : LLVMOpaqueValue? implicit; IsWeak : int) : void {}
[extern(cdecl, name="LLVMGetOrdering", library="LLVM.dll")]
def LLVMGetOrdering(MemoryAccessInst : LLVMOpaqueValue? implicit) : LLVMAtomicOrdering {}
[extern(cdecl, name="LLVMSetOrdering", library="LLVM.dll")]
def LLVMSetOrdering(MemoryAccessInst : LLVMOpaqueValue? implicit; Ordering : LLVMAtomicOrdering) : void {}
[extern(cdecl, name="LLVMGetAtomicRMWBinOp", library="LLVM.dll")]
def LLVMGetAtomicRMWBinOp(AtomicRMWInst : LLVMOpaqueValue? implicit) : LLVMAtomicRMWBinOp {}
[extern(cdecl, name="LLVMSetAtomicRMWBinOp", library="LLVM.dll")]
def LLVMSetAtomicRMWBinOp(AtomicRMWInst : LLVMOpaqueValue? implicit; BinOp : LLVMAtomicRMWBinOp) : void {}
[extern(cdecl, name="LLVMBuildTrunc", library="LLVM.dll")]
def LLVMBuildTrunc(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildZExt", library="LLVM.dll")]
def LLVMBuildZExt(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildSExt", library="LLVM.dll")]
def LLVMBuildSExt(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildFPToUI", library="LLVM.dll")]
def LLVMBuildFPToUI(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildFPToSI", library="LLVM.dll")]
def LLVMBuildFPToSI(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildUIToFP", library="LLVM.dll")]
def LLVMBuildUIToFP(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildSIToFP", library="LLVM.dll")]
def LLVMBuildSIToFP(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildFPTrunc", library="LLVM.dll")]
def LLVMBuildFPTrunc(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildFPExt", library="LLVM.dll")]
def LLVMBuildFPExt(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildPtrToInt", library="LLVM.dll")]
def LLVMBuildPtrToInt(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildIntToPtr", library="LLVM.dll")]
def LLVMBuildIntToPtr(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildBitCast", library="LLVM.dll")]
def LLVMBuildBitCast(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildAddrSpaceCast", library="LLVM.dll")]
def LLVMBuildAddrSpaceCast(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildZExtOrBitCast", library="LLVM.dll")]
def LLVMBuildZExtOrBitCast(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildSExtOrBitCast", library="LLVM.dll")]
def LLVMBuildSExtOrBitCast(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildTruncOrBitCast", library="LLVM.dll")]
def LLVMBuildTruncOrBitCast(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildCast", library="LLVM.dll")]
def LLVMBuildCast(B : LLVMOpaqueBuilder? implicit; Op : LLVMOpcode; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildPointerCast", library="LLVM.dll")]
def LLVMBuildPointerCast(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildIntCast2", library="LLVM.dll")]
def LLVMBuildIntCast2(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; IsSigned : int; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildFPCast", library="LLVM.dll")]
def LLVMBuildFPCast(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildIntCast", library="LLVM.dll")]
def LLVMBuildIntCast(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; DestTy : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetCastOpcode", library="LLVM.dll")]
def LLVMGetCastOpcode(Src : LLVMOpaqueValue? implicit; SrcIsSigned : int; DestTy : LLVMOpaqueType? implicit; DestIsSigned : int) : LLVMOpcode {}
[extern(cdecl, name="LLVMBuildICmp", library="LLVM.dll")]
def LLVMBuildICmp(_argument_0x0 : LLVMOpaqueBuilder? implicit; Op : LLVMIntPredicate; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildFCmp", library="LLVM.dll")]
def LLVMBuildFCmp(_argument_0x0 : LLVMOpaqueBuilder? implicit; Op : LLVMRealPredicate; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildPhi", library="LLVM.dll")]
def LLVMBuildPhi(_argument_0x0 : LLVMOpaqueBuilder? implicit; Ty : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildCall2", library="LLVM.dll")]
def LLVMBuildCall2(_argument_0x0 : LLVMOpaqueBuilder? implicit; _argument_0x1 : LLVMOpaqueType? implicit; Fn : LLVMOpaqueValue? implicit; Args : LLVMOpaqueValue ?? implicit; NumArgs : uint; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildSelect", library="LLVM.dll")]
def LLVMBuildSelect(_argument_0x0 : LLVMOpaqueBuilder? implicit; If : LLVMOpaqueValue? implicit; Then : LLVMOpaqueValue? implicit; Else : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildVAArg", library="LLVM.dll")]
def LLVMBuildVAArg(_argument_0x0 : LLVMOpaqueBuilder? implicit; List : LLVMOpaqueValue? implicit; Ty : LLVMOpaqueType? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildExtractElement", library="LLVM.dll")]
def LLVMBuildExtractElement(_argument_0x0 : LLVMOpaqueBuilder? implicit; VecVal : LLVMOpaqueValue? implicit; Index : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildInsertElement", library="LLVM.dll")]
def LLVMBuildInsertElement(_argument_0x0 : LLVMOpaqueBuilder? implicit; VecVal : LLVMOpaqueValue? implicit; EltVal : LLVMOpaqueValue? implicit; Index : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildShuffleVector", library="LLVM.dll")]
def LLVMBuildShuffleVector(_argument_0x0 : LLVMOpaqueBuilder? implicit; V1 : LLVMOpaqueValue? implicit; V2 : LLVMOpaqueValue? implicit; Mask : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildExtractValue", library="LLVM.dll")]
def LLVMBuildExtractValue(_argument_0x0 : LLVMOpaqueBuilder? implicit; AggVal : LLVMOpaqueValue? implicit; Index : uint; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildInsertValue", library="LLVM.dll")]
def LLVMBuildInsertValue(_argument_0x0 : LLVMOpaqueBuilder? implicit; AggVal : LLVMOpaqueValue? implicit; EltVal : LLVMOpaqueValue? implicit; Index : uint; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildFreeze", library="LLVM.dll")]
def LLVMBuildFreeze(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildIsNull", library="LLVM.dll")]
def LLVMBuildIsNull(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildIsNotNull", library="LLVM.dll")]
def LLVMBuildIsNotNull(_argument_0x0 : LLVMOpaqueBuilder? implicit; Val : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildPtrDiff2", library="LLVM.dll")]
def LLVMBuildPtrDiff2(_argument_0x0 : LLVMOpaqueBuilder? implicit; ElemTy : LLVMOpaqueType? implicit; LHS : LLVMOpaqueValue? implicit; RHS : LLVMOpaqueValue? implicit; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildFence", library="LLVM.dll")]
def LLVMBuildFence(B : LLVMOpaqueBuilder? implicit; ordering : LLVMAtomicOrdering; singleThread : int; Name : string implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildAtomicRMW", library="LLVM.dll")]
def LLVMBuildAtomicRMW(B : LLVMOpaqueBuilder? implicit; op : LLVMAtomicRMWBinOp; PTR : LLVMOpaqueValue? implicit; Val : LLVMOpaqueValue? implicit; ordering : LLVMAtomicOrdering; singleThread : int) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMBuildAtomicCmpXchg", library="LLVM.dll")]
def LLVMBuildAtomicCmpXchg(B : LLVMOpaqueBuilder? implicit; Ptr : LLVMOpaqueValue? implicit; Cmp : LLVMOpaqueValue? implicit; New : LLVMOpaqueValue? implicit; SuccessOrdering : LLVMAtomicOrdering; FailureOrdering : LLVMAtomicOrdering; SingleThread : int) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMGetNumMaskElements", library="LLVM.dll")]
def LLVMGetNumMaskElements(ShuffleVectorInst : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMGetUndefMaskElem", library="LLVM.dll")]
def LLVMGetUndefMaskElem() : int {}
[extern(cdecl, name="LLVMGetMaskValue", library="LLVM.dll")]
def LLVMGetMaskValue(ShuffleVectorInst : LLVMOpaqueValue? implicit; Elt : uint) : int {}
[extern(cdecl, name="LLVMIsAtomicSingleThread", library="LLVM.dll")]
def LLVMIsAtomicSingleThread(AtomicInst : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMSetAtomicSingleThread", library="LLVM.dll")]
def LLVMSetAtomicSingleThread(AtomicInst : LLVMOpaqueValue? implicit; SingleThread : int) : void {}
[extern(cdecl, name="LLVMGetCmpXchgSuccessOrdering", library="LLVM.dll")]
def LLVMGetCmpXchgSuccessOrdering(CmpXchgInst : LLVMOpaqueValue? implicit) : LLVMAtomicOrdering {}
[extern(cdecl, name="LLVMSetCmpXchgSuccessOrdering", library="LLVM.dll")]
def LLVMSetCmpXchgSuccessOrdering(CmpXchgInst : LLVMOpaqueValue? implicit; Ordering : LLVMAtomicOrdering) : void {}
[extern(cdecl, name="LLVMGetCmpXchgFailureOrdering", library="LLVM.dll")]
def LLVMGetCmpXchgFailureOrdering(CmpXchgInst : LLVMOpaqueValue? implicit) : LLVMAtomicOrdering {}
[extern(cdecl, name="LLVMSetCmpXchgFailureOrdering", library="LLVM.dll")]
def LLVMSetCmpXchgFailureOrdering(CmpXchgInst : LLVMOpaqueValue? implicit; Ordering : LLVMAtomicOrdering) : void {}
[extern(cdecl, name="LLVMCreateModuleProviderForExistingModule", library="LLVM.dll")]
def LLVMCreateModuleProviderForExistingModule(M : LLVMOpaqueModule? implicit) : LLVMOpaqueModuleProvider? {}
[extern(cdecl, name="LLVMDisposeModuleProvider", library="LLVM.dll")]
def LLVMDisposeModuleProvider(M : LLVMOpaqueModuleProvider? implicit) : void {}
[extern(cdecl, name="LLVMCreateMemoryBufferWithContentsOfFile", library="LLVM.dll")]
def LLVMCreateMemoryBufferWithContentsOfFile(Path : string implicit; OutMemBuf : LLVMOpaqueMemoryBuffer ?? implicit; OutMessage : string? implicit) : int {}
[extern(cdecl, name="LLVMCreateMemoryBufferWithSTDIN", library="LLVM.dll")]
def LLVMCreateMemoryBufferWithSTDIN(OutMemBuf : LLVMOpaqueMemoryBuffer ?? implicit; OutMessage : string? implicit) : int {}
[extern(cdecl, name="LLVMCreateMemoryBufferWithMemoryRange", library="LLVM.dll")]
def LLVMCreateMemoryBufferWithMemoryRange(InputData : string implicit; InputDataLength : uint64; BufferName : string implicit; RequiresNullTerminator : int) : LLVMOpaqueMemoryBuffer? {}
[extern(cdecl, name="LLVMCreateMemoryBufferWithMemoryRangeCopy", library="LLVM.dll")]
def LLVMCreateMemoryBufferWithMemoryRangeCopy(InputData : string implicit; InputDataLength : uint64; BufferName : string implicit) : LLVMOpaqueMemoryBuffer? {}
[extern(cdecl, name="LLVMGetBufferStart", library="LLVM.dll")]
def LLVMGetBufferStart(MemBuf : LLVMOpaqueMemoryBuffer? implicit) : string {}
[extern(cdecl, name="LLVMGetBufferSize", library="LLVM.dll")]
def LLVMGetBufferSize(MemBuf : LLVMOpaqueMemoryBuffer? implicit) : uint64 {}
[extern(cdecl, name="LLVMDisposeMemoryBuffer", library="LLVM.dll")]
def LLVMDisposeMemoryBuffer(MemBuf : LLVMOpaqueMemoryBuffer? implicit) : void {}
[extern(cdecl, name="LLVMGetGlobalPassRegistry", library="LLVM.dll")]
def LLVMGetGlobalPassRegistry() : LLVMOpaquePassRegistry? {}
[extern(cdecl, name="LLVMCreatePassManager", library="LLVM.dll")]
def LLVMCreatePassManager() : LLVMOpaquePassManager? {}
[extern(cdecl, name="LLVMCreateFunctionPassManagerForModule", library="LLVM.dll")]
def LLVMCreateFunctionPassManagerForModule(M : LLVMOpaqueModule? implicit) : LLVMOpaquePassManager? {}
[extern(cdecl, name="LLVMCreateFunctionPassManager", library="LLVM.dll")]
def LLVMCreateFunctionPassManager(MP : LLVMOpaqueModuleProvider? implicit) : LLVMOpaquePassManager? {}
[extern(cdecl, name="LLVMRunPassManager", library="LLVM.dll")]
def LLVMRunPassManager(PM : LLVMOpaquePassManager? implicit; M : LLVMOpaqueModule? implicit) : int {}
[extern(cdecl, name="LLVMInitializeFunctionPassManager", library="LLVM.dll")]
def LLVMInitializeFunctionPassManager(FPM : LLVMOpaquePassManager? implicit) : int {}
[extern(cdecl, name="LLVMRunFunctionPassManager", library="LLVM.dll")]
def LLVMRunFunctionPassManager(FPM : LLVMOpaquePassManager? implicit; F : LLVMOpaqueValue? implicit) : int {}
[extern(cdecl, name="LLVMFinalizeFunctionPassManager", library="LLVM.dll")]
def LLVMFinalizeFunctionPassManager(FPM : LLVMOpaquePassManager? implicit) : int {}
[extern(cdecl, name="LLVMDisposePassManager", library="LLVM.dll")]
def LLVMDisposePassManager(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMStartMultithreaded", library="LLVM.dll")]
def LLVMStartMultithreaded() : int {}
[extern(cdecl, name="LLVMStopMultithreaded", library="LLVM.dll")]
def LLVMStopMultithreaded() : void {}
[extern(cdecl, name="LLVMIsMultithreaded", library="LLVM.dll")]
def LLVMIsMultithreaded() : int {}
[extern(cdecl, name="LLVMDebugMetadataVersion", library="LLVM.dll")]
def LLVMDebugMetadataVersion() : uint {}
[extern(cdecl, name="LLVMGetModuleDebugMetadataVersion", library="LLVM.dll")]
def LLVMGetModuleDebugMetadataVersion(Module : LLVMOpaqueModule? implicit) : uint {}
[extern(cdecl, name="LLVMStripModuleDebugInfo", library="LLVM.dll")]
def LLVMStripModuleDebugInfo(Module : LLVMOpaqueModule? implicit) : int {}
[extern(cdecl, name="LLVMCreateDIBuilderDisallowUnresolved", library="LLVM.dll")]
def LLVMCreateDIBuilderDisallowUnresolved(M : LLVMOpaqueModule? implicit) : LLVMOpaqueDIBuilder? {}
[extern(cdecl, name="LLVMCreateDIBuilder", library="LLVM.dll")]
def LLVMCreateDIBuilder(M : LLVMOpaqueModule? implicit) : LLVMOpaqueDIBuilder? {}
[extern(cdecl, name="LLVMDisposeDIBuilder", library="LLVM.dll")]
def LLVMDisposeDIBuilder(Builder : LLVMOpaqueDIBuilder? implicit) : void {}
[extern(cdecl, name="LLVMDIBuilderFinalize", library="LLVM.dll")]
def LLVMDIBuilderFinalize(Builder : LLVMOpaqueDIBuilder? implicit) : void {}
[extern(cdecl, name="LLVMDIBuilderFinalizeSubprogram", library="LLVM.dll")]
def LLVMDIBuilderFinalizeSubprogram(Builder : LLVMOpaqueDIBuilder? implicit; Subprogram : LLVMOpaqueMetadata? implicit) : void {}
// Function `LLVMDIBuilderCreateCompileUnit` has too many arguments. Ignore.

[extern(cdecl, name="LLVMDIBuilderCreateFile", library="LLVM.dll")]
def LLVMDIBuilderCreateFile(Builder : LLVMOpaqueDIBuilder? implicit; Filename : string implicit; FilenameLen : uint64; Directory : string implicit; DirectoryLen : uint64) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateModule", library="LLVM.dll")]
def LLVMDIBuilderCreateModule(Builder : LLVMOpaqueDIBuilder? implicit; ParentScope : LLVMOpaqueMetadata? implicit; Name : string implicit; NameLen : uint64; ConfigMacros : string implicit; ConfigMacrosLen : uint64; IncludePath : string implicit; IncludePathLen : uint64; APINotesFile : string implicit; APINotesFileLen : uint64) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateNameSpace", library="LLVM.dll")]
def LLVMDIBuilderCreateNameSpace(Builder : LLVMOpaqueDIBuilder? implicit; ParentScope : LLVMOpaqueMetadata? implicit; Name : string implicit; NameLen : uint64; ExportSymbols : int) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateFunction", library="LLVM.dll")]
def LLVMDIBuilderCreateFunction(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit; Name : string implicit; NameLen : uint64; LinkageName : string implicit; LinkageNameLen : uint64; File : LLVMOpaqueMetadata? implicit; LineNo : uint; Ty : LLVMOpaqueMetadata? implicit; IsLocalToUnit : int; IsDefinition : int; ScopeLine : uint; Flags : LLVMDIFlags; IsOptimized : int) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateLexicalBlock", library="LLVM.dll")]
def LLVMDIBuilderCreateLexicalBlock(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit; File : LLVMOpaqueMetadata? implicit; Line : uint; Column : uint) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateLexicalBlockFile", library="LLVM.dll")]
def LLVMDIBuilderCreateLexicalBlockFile(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit; File : LLVMOpaqueMetadata? implicit; Discriminator : uint) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateImportedModuleFromNamespace", library="LLVM.dll")]
def LLVMDIBuilderCreateImportedModuleFromNamespace(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit; NS : LLVMOpaqueMetadata? implicit; File : LLVMOpaqueMetadata? implicit; Line : uint) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateImportedModuleFromAlias", library="LLVM.dll")]
def LLVMDIBuilderCreateImportedModuleFromAlias(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit; ImportedEntity : LLVMOpaqueMetadata? implicit; File : LLVMOpaqueMetadata? implicit; Line : uint; Elements : LLVMOpaqueMetadata ?? implicit; NumElements : uint) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateImportedModuleFromModule", library="LLVM.dll")]
def LLVMDIBuilderCreateImportedModuleFromModule(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit; M : LLVMOpaqueMetadata? implicit; File : LLVMOpaqueMetadata? implicit; Line : uint; Elements : LLVMOpaqueMetadata ?? implicit; NumElements : uint) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateImportedDeclaration", library="LLVM.dll")]
def LLVMDIBuilderCreateImportedDeclaration(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit; Decl : LLVMOpaqueMetadata? implicit; File : LLVMOpaqueMetadata? implicit; Line : uint; Name : string implicit; NameLen : uint64; Elements : LLVMOpaqueMetadata ?? implicit; NumElements : uint) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateDebugLocation", library="LLVM.dll")]
def LLVMDIBuilderCreateDebugLocation(Ctx : LLVMOpaqueContext? implicit; Line : uint; Column : uint; Scope : LLVMOpaqueMetadata? implicit; InlinedAt : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDILocationGetLine", library="LLVM.dll")]
def LLVMDILocationGetLine(Location : LLVMOpaqueMetadata? implicit) : uint {}
[extern(cdecl, name="LLVMDILocationGetColumn", library="LLVM.dll")]
def LLVMDILocationGetColumn(Location : LLVMOpaqueMetadata? implicit) : uint {}
[extern(cdecl, name="LLVMDILocationGetScope", library="LLVM.dll")]
def LLVMDILocationGetScope(Location : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDILocationGetInlinedAt", library="LLVM.dll")]
def LLVMDILocationGetInlinedAt(Location : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIScopeGetFile", library="LLVM.dll")]
def LLVMDIScopeGetFile(Scope : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIFileGetDirectory", library="LLVM.dll")]
def LLVMDIFileGetDirectory(File : LLVMOpaqueMetadata? implicit; Len : uint? implicit) : string {}
[extern(cdecl, name="LLVMDIFileGetFilename", library="LLVM.dll")]
def LLVMDIFileGetFilename(File : LLVMOpaqueMetadata? implicit; Len : uint? implicit) : string {}
[extern(cdecl, name="LLVMDIFileGetSource", library="LLVM.dll")]
def LLVMDIFileGetSource(File : LLVMOpaqueMetadata? implicit; Len : uint? implicit) : string {}
[extern(cdecl, name="LLVMDIBuilderGetOrCreateTypeArray", library="LLVM.dll")]
def LLVMDIBuilderGetOrCreateTypeArray(Builder : LLVMOpaqueDIBuilder? implicit; Data : LLVMOpaqueMetadata ?? implicit; NumElements : uint64) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateSubroutineType", library="LLVM.dll")]
def LLVMDIBuilderCreateSubroutineType(Builder : LLVMOpaqueDIBuilder? implicit; File : LLVMOpaqueMetadata? implicit; ParameterTypes : LLVMOpaqueMetadata ?? implicit; NumParameterTypes : uint; Flags : LLVMDIFlags) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateMacro", library="LLVM.dll")]
def LLVMDIBuilderCreateMacro(Builder : LLVMOpaqueDIBuilder? implicit; ParentMacroFile : LLVMOpaqueMetadata? implicit; Line : uint; RecordType : LLVMDWARFMacinfoRecordType; Name : string implicit; NameLen : uint64; Value : string implicit; ValueLen : uint64) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateTempMacroFile", library="LLVM.dll")]
def LLVMDIBuilderCreateTempMacroFile(Builder : LLVMOpaqueDIBuilder? implicit; ParentMacroFile : LLVMOpaqueMetadata? implicit; Line : uint; File : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateEnumerator", library="LLVM.dll")]
def LLVMDIBuilderCreateEnumerator(Builder : LLVMOpaqueDIBuilder? implicit; Name : string implicit; NameLen : uint64; Value : int64; IsUnsigned : int) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateEnumerationType", library="LLVM.dll")]
def LLVMDIBuilderCreateEnumerationType(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit; Name : string implicit; NameLen : uint64; File : LLVMOpaqueMetadata? implicit; LineNumber : uint; SizeInBits : uint64; AlignInBits : uint; Elements : LLVMOpaqueMetadata ?? implicit; NumElements : uint; ClassTy : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateUnionType", library="LLVM.dll")]
def LLVMDIBuilderCreateUnionType(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit; Name : string implicit; NameLen : uint64; File : LLVMOpaqueMetadata? implicit; LineNumber : uint; SizeInBits : uint64; AlignInBits : uint; Flags : LLVMDIFlags; Elements : LLVMOpaqueMetadata ?? implicit; NumElements : uint; RunTimeLang : uint; UniqueId : string implicit; UniqueIdLen : uint64) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateArrayType", library="LLVM.dll")]
def LLVMDIBuilderCreateArrayType(Builder : LLVMOpaqueDIBuilder? implicit; Size : uint64; AlignInBits : uint; Ty : LLVMOpaqueMetadata? implicit; Subscripts : LLVMOpaqueMetadata ?? implicit; NumSubscripts : uint) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateVectorType", library="LLVM.dll")]
def LLVMDIBuilderCreateVectorType(Builder : LLVMOpaqueDIBuilder? implicit; Size : uint64; AlignInBits : uint; Ty : LLVMOpaqueMetadata? implicit; Subscripts : LLVMOpaqueMetadata ?? implicit; NumSubscripts : uint) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateUnspecifiedType", library="LLVM.dll")]
def LLVMDIBuilderCreateUnspecifiedType(Builder : LLVMOpaqueDIBuilder? implicit; Name : string implicit; NameLen : uint64) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateBasicType", library="LLVM.dll")]
def LLVMDIBuilderCreateBasicType(Builder : LLVMOpaqueDIBuilder? implicit; Name : string implicit; NameLen : uint64; SizeInBits : uint64; Encoding : uint; Flags : LLVMDIFlags) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreatePointerType", library="LLVM.dll")]
def LLVMDIBuilderCreatePointerType(Builder : LLVMOpaqueDIBuilder? implicit; PointeeTy : LLVMOpaqueMetadata? implicit; SizeInBits : uint64; AlignInBits : uint; AddressSpace : uint; Name : string implicit; NameLen : uint64) : LLVMOpaqueMetadata? {}
// Function `LLVMDIBuilderCreateStructType` has too many arguments. Ignore.

[extern(cdecl, name="LLVMDIBuilderCreateMemberType", library="LLVM.dll")]
def LLVMDIBuilderCreateMemberType(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit; Name : string implicit; NameLen : uint64; File : LLVMOpaqueMetadata? implicit; LineNo : uint; SizeInBits : uint64; AlignInBits : uint; OffsetInBits : uint64; Flags : LLVMDIFlags; Ty : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateStaticMemberType", library="LLVM.dll")]
def LLVMDIBuilderCreateStaticMemberType(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit; Name : string implicit; NameLen : uint64; File : LLVMOpaqueMetadata? implicit; LineNumber : uint; Type : LLVMOpaqueMetadata? implicit; Flags : LLVMDIFlags; ConstantVal : LLVMOpaqueValue? implicit; AlignInBits : uint) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateMemberPointerType", library="LLVM.dll")]
def LLVMDIBuilderCreateMemberPointerType(Builder : LLVMOpaqueDIBuilder? implicit; PointeeType : LLVMOpaqueMetadata? implicit; ClassType : LLVMOpaqueMetadata? implicit; SizeInBits : uint64; AlignInBits : uint; Flags : LLVMDIFlags) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateObjCIVar", library="LLVM.dll")]
def LLVMDIBuilderCreateObjCIVar(Builder : LLVMOpaqueDIBuilder? implicit; Name : string implicit; NameLen : uint64; File : LLVMOpaqueMetadata? implicit; LineNo : uint; SizeInBits : uint64; AlignInBits : uint; OffsetInBits : uint64; Flags : LLVMDIFlags; Ty : LLVMOpaqueMetadata? implicit; PropertyNode : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateObjCProperty", library="LLVM.dll")]
def LLVMDIBuilderCreateObjCProperty(Builder : LLVMOpaqueDIBuilder? implicit; Name : string implicit; NameLen : uint64; File : LLVMOpaqueMetadata? implicit; LineNo : uint; GetterName : string implicit; GetterNameLen : uint64; SetterName : string implicit; SetterNameLen : uint64; PropertyAttributes : uint; Ty : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateObjectPointerType", library="LLVM.dll")]
def LLVMDIBuilderCreateObjectPointerType(Builder : LLVMOpaqueDIBuilder? implicit; Type : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateQualifiedType", library="LLVM.dll")]
def LLVMDIBuilderCreateQualifiedType(Builder : LLVMOpaqueDIBuilder? implicit; Tag : uint; Type : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateReferenceType", library="LLVM.dll")]
def LLVMDIBuilderCreateReferenceType(Builder : LLVMOpaqueDIBuilder? implicit; Tag : uint; Type : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateNullPtrType", library="LLVM.dll")]
def LLVMDIBuilderCreateNullPtrType(Builder : LLVMOpaqueDIBuilder? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateTypedef", library="LLVM.dll")]
def LLVMDIBuilderCreateTypedef(Builder : LLVMOpaqueDIBuilder? implicit; Type : LLVMOpaqueMetadata? implicit; Name : string implicit; NameLen : uint64; File : LLVMOpaqueMetadata? implicit; LineNo : uint; Scope : LLVMOpaqueMetadata? implicit; AlignInBits : uint) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateInheritance", library="LLVM.dll")]
def LLVMDIBuilderCreateInheritance(Builder : LLVMOpaqueDIBuilder? implicit; Ty : LLVMOpaqueMetadata? implicit; BaseTy : LLVMOpaqueMetadata? implicit; BaseOffset : uint64; VBPtrOffset : uint; Flags : LLVMDIFlags) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateForwardDecl", library="LLVM.dll")]
def LLVMDIBuilderCreateForwardDecl(Builder : LLVMOpaqueDIBuilder? implicit; Tag : uint; Name : string implicit; NameLen : uint64; Scope : LLVMOpaqueMetadata? implicit; File : LLVMOpaqueMetadata? implicit; Line : uint; RuntimeLang : uint; SizeInBits : uint64; AlignInBits : uint; UniqueIdentifier : string implicit; UniqueIdentifierLen : uint64) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateReplaceableCompositeType", library="LLVM.dll")]
def LLVMDIBuilderCreateReplaceableCompositeType(Builder : LLVMOpaqueDIBuilder? implicit; Tag : uint; Name : string implicit; NameLen : uint64; Scope : LLVMOpaqueMetadata? implicit; File : LLVMOpaqueMetadata? implicit; Line : uint; RuntimeLang : uint; SizeInBits : uint64; AlignInBits : uint; Flags : LLVMDIFlags; UniqueIdentifier : string implicit; UniqueIdentifierLen : uint64) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateBitFieldMemberType", library="LLVM.dll")]
def LLVMDIBuilderCreateBitFieldMemberType(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit; Name : string implicit; NameLen : uint64; File : LLVMOpaqueMetadata? implicit; LineNumber : uint; SizeInBits : uint64; OffsetInBits : uint64; StorageOffsetInBits : uint64; Flags : LLVMDIFlags; Type : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
// Function `LLVMDIBuilderCreateClassType` has too many arguments. Ignore.

[extern(cdecl, name="LLVMDIBuilderCreateArtificialType", library="LLVM.dll")]
def LLVMDIBuilderCreateArtificialType(Builder : LLVMOpaqueDIBuilder? implicit; Type : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDITypeGetName", library="LLVM.dll")]
def LLVMDITypeGetName(DType : LLVMOpaqueMetadata? implicit; Length : uint64? implicit) : string {}
[extern(cdecl, name="LLVMDITypeGetSizeInBits", library="LLVM.dll")]
def LLVMDITypeGetSizeInBits(DType : LLVMOpaqueMetadata? implicit) : uint64 {}
[extern(cdecl, name="LLVMDITypeGetOffsetInBits", library="LLVM.dll")]
def LLVMDITypeGetOffsetInBits(DType : LLVMOpaqueMetadata? implicit) : uint64 {}
[extern(cdecl, name="LLVMDITypeGetAlignInBits", library="LLVM.dll")]
def LLVMDITypeGetAlignInBits(DType : LLVMOpaqueMetadata? implicit) : uint {}
[extern(cdecl, name="LLVMDITypeGetLine", library="LLVM.dll")]
def LLVMDITypeGetLine(DType : LLVMOpaqueMetadata? implicit) : uint {}
[extern(cdecl, name="LLVMDITypeGetFlags", library="LLVM.dll")]
def LLVMDITypeGetFlags(DType : LLVMOpaqueMetadata? implicit) : LLVMDIFlags {}
[extern(cdecl, name="LLVMDIBuilderGetOrCreateSubrange", library="LLVM.dll")]
def LLVMDIBuilderGetOrCreateSubrange(Builder : LLVMOpaqueDIBuilder? implicit; LowerBound : int64; Count : int64) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderGetOrCreateArray", library="LLVM.dll")]
def LLVMDIBuilderGetOrCreateArray(Builder : LLVMOpaqueDIBuilder? implicit; Data : LLVMOpaqueMetadata ?? implicit; NumElements : uint64) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateExpression", library="LLVM.dll")]
def LLVMDIBuilderCreateExpression(Builder : LLVMOpaqueDIBuilder? implicit; Addr : uint64? implicit; Length : uint64) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateConstantValueExpression", library="LLVM.dll")]
def LLVMDIBuilderCreateConstantValueExpression(Builder : LLVMOpaqueDIBuilder? implicit; Value : uint64) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateGlobalVariableExpression", library="LLVM.dll")]
def LLVMDIBuilderCreateGlobalVariableExpression(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit; Name : string implicit; NameLen : uint64; Linkage : string implicit; LinkLen : uint64; File : LLVMOpaqueMetadata? implicit; LineNo : uint; Ty : LLVMOpaqueMetadata? implicit; LocalToUnit : int; Expr : LLVMOpaqueMetadata? implicit; Decl : LLVMOpaqueMetadata? implicit; AlignInBits : uint) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIGlobalVariableExpressionGetVariable", library="LLVM.dll")]
def LLVMDIGlobalVariableExpressionGetVariable(GVE : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIGlobalVariableExpressionGetExpression", library="LLVM.dll")]
def LLVMDIGlobalVariableExpressionGetExpression(GVE : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIVariableGetFile", library="LLVM.dll")]
def LLVMDIVariableGetFile(Var : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIVariableGetScope", library="LLVM.dll")]
def LLVMDIVariableGetScope(Var : LLVMOpaqueMetadata? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIVariableGetLine", library="LLVM.dll")]
def LLVMDIVariableGetLine(Var : LLVMOpaqueMetadata? implicit) : uint {}
[extern(cdecl, name="LLVMTemporaryMDNode", library="LLVM.dll")]
def LLVMTemporaryMDNode(Ctx : LLVMOpaqueContext? implicit; Data : LLVMOpaqueMetadata ?? implicit; NumElements : uint64) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDisposeTemporaryMDNode", library="LLVM.dll")]
def LLVMDisposeTemporaryMDNode(TempNode : LLVMOpaqueMetadata? implicit) : void {}
[extern(cdecl, name="LLVMMetadataReplaceAllUsesWith", library="LLVM.dll")]
def LLVMMetadataReplaceAllUsesWith(TempTargetMetadata : LLVMOpaqueMetadata? implicit; Replacement : LLVMOpaqueMetadata? implicit) : void {}
[extern(cdecl, name="LLVMDIBuilderCreateTempGlobalVariableFwdDecl", library="LLVM.dll")]
def LLVMDIBuilderCreateTempGlobalVariableFwdDecl(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit; Name : string implicit; NameLen : uint64; Linkage : string implicit; LnkLen : uint64; File : LLVMOpaqueMetadata? implicit; LineNo : uint; Ty : LLVMOpaqueMetadata? implicit; LocalToUnit : int; Decl : LLVMOpaqueMetadata? implicit; AlignInBits : uint) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderInsertDeclareBefore", library="LLVM.dll")]
def LLVMDIBuilderInsertDeclareBefore(Builder : LLVMOpaqueDIBuilder? implicit; Storage : LLVMOpaqueValue? implicit; VarInfo : LLVMOpaqueMetadata? implicit; Expr : LLVMOpaqueMetadata? implicit; DebugLoc : LLVMOpaqueMetadata? implicit; Instr : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMDIBuilderInsertDeclareAtEnd", library="LLVM.dll")]
def LLVMDIBuilderInsertDeclareAtEnd(Builder : LLVMOpaqueDIBuilder? implicit; Storage : LLVMOpaqueValue? implicit; VarInfo : LLVMOpaqueMetadata? implicit; Expr : LLVMOpaqueMetadata? implicit; DebugLoc : LLVMOpaqueMetadata? implicit; Block : LLVMOpaqueBasicBlock? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMDIBuilderInsertDbgValueBefore", library="LLVM.dll")]
def LLVMDIBuilderInsertDbgValueBefore(Builder : LLVMOpaqueDIBuilder? implicit; Val : LLVMOpaqueValue? implicit; VarInfo : LLVMOpaqueMetadata? implicit; Expr : LLVMOpaqueMetadata? implicit; DebugLoc : LLVMOpaqueMetadata? implicit; Instr : LLVMOpaqueValue? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMDIBuilderInsertDbgValueAtEnd", library="LLVM.dll")]
def LLVMDIBuilderInsertDbgValueAtEnd(Builder : LLVMOpaqueDIBuilder? implicit; Val : LLVMOpaqueValue? implicit; VarInfo : LLVMOpaqueMetadata? implicit; Expr : LLVMOpaqueMetadata? implicit; DebugLoc : LLVMOpaqueMetadata? implicit; Block : LLVMOpaqueBasicBlock? implicit) : LLVMOpaqueValue? {}
[extern(cdecl, name="LLVMDIBuilderCreateAutoVariable", library="LLVM.dll")]
def LLVMDIBuilderCreateAutoVariable(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit; Name : string implicit; NameLen : uint64; File : LLVMOpaqueMetadata? implicit; LineNo : uint; Ty : LLVMOpaqueMetadata? implicit; AlwaysPreserve : int; Flags : LLVMDIFlags; AlignInBits : uint) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMDIBuilderCreateParameterVariable", library="LLVM.dll")]
def LLVMDIBuilderCreateParameterVariable(Builder : LLVMOpaqueDIBuilder? implicit; Scope : LLVMOpaqueMetadata? implicit; Name : string implicit; NameLen : uint64; ArgNo : uint; File : LLVMOpaqueMetadata? implicit; LineNo : uint; Ty : LLVMOpaqueMetadata? implicit; AlwaysPreserve : int; Flags : LLVMDIFlags) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMGetSubprogram", library="LLVM.dll")]
def LLVMGetSubprogram(Func : LLVMOpaqueValue? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMSetSubprogram", library="LLVM.dll")]
def LLVMSetSubprogram(Func : LLVMOpaqueValue? implicit; SP : LLVMOpaqueMetadata? implicit) : void {}
[extern(cdecl, name="LLVMDISubprogramGetLine", library="LLVM.dll")]
def LLVMDISubprogramGetLine(Subprogram : LLVMOpaqueMetadata? implicit) : uint {}
[extern(cdecl, name="LLVMInstructionGetDebugLoc", library="LLVM.dll")]
def LLVMInstructionGetDebugLoc(Inst : LLVMOpaqueValue? implicit) : LLVMOpaqueMetadata? {}
[extern(cdecl, name="LLVMInstructionSetDebugLoc", library="LLVM.dll")]
def LLVMInstructionSetDebugLoc(Inst : LLVMOpaqueValue? implicit; Loc : LLVMOpaqueMetadata? implicit) : void {}
[extern(cdecl, name="LLVMGetMetadataKind", library="LLVM.dll")]
def LLVMGetMetadataKind(Metadata : LLVMOpaqueMetadata? implicit) : uint {}
[extern(cdecl, name="LLVMSetDisasmOptions", library="LLVM.dll")]
def LLVMSetDisasmOptions(DC : void? implicit; Options : uint64) : int {}
[extern(cdecl, name="LLVMDisasmDispose", library="LLVM.dll")]
def LLVMDisasmDispose(DC : void? implicit) : void {}
[extern(cdecl, name="LLVMDisasmInstruction", library="LLVM.dll")]
def LLVMDisasmInstruction(DC : void? implicit; Bytes : uint8? implicit; BytesSize : uint64; PC : uint64; OutString : string implicit; OutStringSize : uint64) : uint64 {}
[extern(cdecl, name="LLVMGetErrorTypeId", library="LLVM.dll")]
def LLVMGetErrorTypeId(Err : LLVMOpaqueError? implicit) : void const? {}
[extern(cdecl, name="LLVMConsumeError", library="LLVM.dll")]
def LLVMConsumeError(Err : LLVMOpaqueError? implicit) : void {}
[extern(cdecl, name="LLVMGetErrorMessage", library="LLVM.dll")]
def LLVMGetErrorMessage(Err : LLVMOpaqueError? implicit) : string {}
[extern(cdecl, name="LLVMDisposeErrorMessage", library="LLVM.dll")]
def LLVMDisposeErrorMessage(ErrMsg : string implicit) : void {}
[extern(cdecl, name="LLVMGetStringErrorTypeId", library="LLVM.dll")]
def LLVMGetStringErrorTypeId() : void const? {}
[extern(cdecl, name="LLVMCreateStringError", library="LLVM.dll")]
def LLVMCreateStringError(ErrMsg : string implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMInitializeAArch64TargetInfo", library="LLVM.dll")]
def LLVMInitializeAArch64TargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeAMDGPUTargetInfo", library="LLVM.dll")]
def LLVMInitializeAMDGPUTargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeARMTargetInfo", library="LLVM.dll")]
def LLVMInitializeARMTargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeAVRTargetInfo", library="LLVM.dll")]
def LLVMInitializeAVRTargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeBPFTargetInfo", library="LLVM.dll")]
def LLVMInitializeBPFTargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeHexagonTargetInfo", library="LLVM.dll")]
def LLVMInitializeHexagonTargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeLanaiTargetInfo", library="LLVM.dll")]
def LLVMInitializeLanaiTargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeLoongArchTargetInfo", library="LLVM.dll")]
def LLVMInitializeLoongArchTargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeMipsTargetInfo", library="LLVM.dll")]
def LLVMInitializeMipsTargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeMSP430TargetInfo", library="LLVM.dll")]
def LLVMInitializeMSP430TargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeNVPTXTargetInfo", library="LLVM.dll")]
def LLVMInitializeNVPTXTargetInfo() : void {}
[extern(cdecl, name="LLVMInitializePowerPCTargetInfo", library="LLVM.dll")]
def LLVMInitializePowerPCTargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeRISCVTargetInfo", library="LLVM.dll")]
def LLVMInitializeRISCVTargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeSparcTargetInfo", library="LLVM.dll")]
def LLVMInitializeSparcTargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeSystemZTargetInfo", library="LLVM.dll")]
def LLVMInitializeSystemZTargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeVETargetInfo", library="LLVM.dll")]
def LLVMInitializeVETargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeWebAssemblyTargetInfo", library="LLVM.dll")]
def LLVMInitializeWebAssemblyTargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeX86TargetInfo", library="LLVM.dll")]
def LLVMInitializeX86TargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeXCoreTargetInfo", library="LLVM.dll")]
def LLVMInitializeXCoreTargetInfo() : void {}
[extern(cdecl, name="LLVMInitializeAArch64Target", library="LLVM.dll")]
def LLVMInitializeAArch64Target() : void {}
[extern(cdecl, name="LLVMInitializeAMDGPUTarget", library="LLVM.dll")]
def LLVMInitializeAMDGPUTarget() : void {}
[extern(cdecl, name="LLVMInitializeARMTarget", library="LLVM.dll")]
def LLVMInitializeARMTarget() : void {}
[extern(cdecl, name="LLVMInitializeAVRTarget", library="LLVM.dll")]
def LLVMInitializeAVRTarget() : void {}
[extern(cdecl, name="LLVMInitializeBPFTarget", library="LLVM.dll")]
def LLVMInitializeBPFTarget() : void {}
[extern(cdecl, name="LLVMInitializeHexagonTarget", library="LLVM.dll")]
def LLVMInitializeHexagonTarget() : void {}
[extern(cdecl, name="LLVMInitializeLanaiTarget", library="LLVM.dll")]
def LLVMInitializeLanaiTarget() : void {}
[extern(cdecl, name="LLVMInitializeLoongArchTarget", library="LLVM.dll")]
def LLVMInitializeLoongArchTarget() : void {}
[extern(cdecl, name="LLVMInitializeMipsTarget", library="LLVM.dll")]
def LLVMInitializeMipsTarget() : void {}
[extern(cdecl, name="LLVMInitializeMSP430Target", library="LLVM.dll")]
def LLVMInitializeMSP430Target() : void {}
[extern(cdecl, name="LLVMInitializeNVPTXTarget", library="LLVM.dll")]
def LLVMInitializeNVPTXTarget() : void {}
[extern(cdecl, name="LLVMInitializePowerPCTarget", library="LLVM.dll")]
def LLVMInitializePowerPCTarget() : void {}
[extern(cdecl, name="LLVMInitializeRISCVTarget", library="LLVM.dll")]
def LLVMInitializeRISCVTarget() : void {}
[extern(cdecl, name="LLVMInitializeSparcTarget", library="LLVM.dll")]
def LLVMInitializeSparcTarget() : void {}
[extern(cdecl, name="LLVMInitializeSystemZTarget", library="LLVM.dll")]
def LLVMInitializeSystemZTarget() : void {}
[extern(cdecl, name="LLVMInitializeVETarget", library="LLVM.dll")]
def LLVMInitializeVETarget() : void {}
[extern(cdecl, name="LLVMInitializeWebAssemblyTarget", library="LLVM.dll")]
def LLVMInitializeWebAssemblyTarget() : void {}
[extern(cdecl, name="LLVMInitializeX86Target", library="LLVM.dll")]
def LLVMInitializeX86Target() : void {}
[extern(cdecl, name="LLVMInitializeXCoreTarget", library="LLVM.dll")]
def LLVMInitializeXCoreTarget() : void {}
[extern(cdecl, name="LLVMInitializeAArch64TargetMC", library="LLVM.dll")]
def LLVMInitializeAArch64TargetMC() : void {}
[extern(cdecl, name="LLVMInitializeAMDGPUTargetMC", library="LLVM.dll")]
def LLVMInitializeAMDGPUTargetMC() : void {}
[extern(cdecl, name="LLVMInitializeARMTargetMC", library="LLVM.dll")]
def LLVMInitializeARMTargetMC() : void {}
[extern(cdecl, name="LLVMInitializeAVRTargetMC", library="LLVM.dll")]
def LLVMInitializeAVRTargetMC() : void {}
[extern(cdecl, name="LLVMInitializeBPFTargetMC", library="LLVM.dll")]
def LLVMInitializeBPFTargetMC() : void {}
[extern(cdecl, name="LLVMInitializeHexagonTargetMC", library="LLVM.dll")]
def LLVMInitializeHexagonTargetMC() : void {}
[extern(cdecl, name="LLVMInitializeLanaiTargetMC", library="LLVM.dll")]
def LLVMInitializeLanaiTargetMC() : void {}
[extern(cdecl, name="LLVMInitializeLoongArchTargetMC", library="LLVM.dll")]
def LLVMInitializeLoongArchTargetMC() : void {}
[extern(cdecl, name="LLVMInitializeMipsTargetMC", library="LLVM.dll")]
def LLVMInitializeMipsTargetMC() : void {}
[extern(cdecl, name="LLVMInitializeMSP430TargetMC", library="LLVM.dll")]
def LLVMInitializeMSP430TargetMC() : void {}
[extern(cdecl, name="LLVMInitializeNVPTXTargetMC", library="LLVM.dll")]
def LLVMInitializeNVPTXTargetMC() : void {}
[extern(cdecl, name="LLVMInitializePowerPCTargetMC", library="LLVM.dll")]
def LLVMInitializePowerPCTargetMC() : void {}
[extern(cdecl, name="LLVMInitializeRISCVTargetMC", library="LLVM.dll")]
def LLVMInitializeRISCVTargetMC() : void {}
[extern(cdecl, name="LLVMInitializeSparcTargetMC", library="LLVM.dll")]
def LLVMInitializeSparcTargetMC() : void {}
[extern(cdecl, name="LLVMInitializeSystemZTargetMC", library="LLVM.dll")]
def LLVMInitializeSystemZTargetMC() : void {}
[extern(cdecl, name="LLVMInitializeVETargetMC", library="LLVM.dll")]
def LLVMInitializeVETargetMC() : void {}
[extern(cdecl, name="LLVMInitializeWebAssemblyTargetMC", library="LLVM.dll")]
def LLVMInitializeWebAssemblyTargetMC() : void {}
[extern(cdecl, name="LLVMInitializeX86TargetMC", library="LLVM.dll")]
def LLVMInitializeX86TargetMC() : void {}
[extern(cdecl, name="LLVMInitializeXCoreTargetMC", library="LLVM.dll")]
def LLVMInitializeXCoreTargetMC() : void {}
[extern(cdecl, name="LLVMInitializeAArch64AsmPrinter", library="LLVM.dll")]
def LLVMInitializeAArch64AsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeAMDGPUAsmPrinter", library="LLVM.dll")]
def LLVMInitializeAMDGPUAsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeARMAsmPrinter", library="LLVM.dll")]
def LLVMInitializeARMAsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeAVRAsmPrinter", library="LLVM.dll")]
def LLVMInitializeAVRAsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeBPFAsmPrinter", library="LLVM.dll")]
def LLVMInitializeBPFAsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeHexagonAsmPrinter", library="LLVM.dll")]
def LLVMInitializeHexagonAsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeLanaiAsmPrinter", library="LLVM.dll")]
def LLVMInitializeLanaiAsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeLoongArchAsmPrinter", library="LLVM.dll")]
def LLVMInitializeLoongArchAsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeMipsAsmPrinter", library="LLVM.dll")]
def LLVMInitializeMipsAsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeMSP430AsmPrinter", library="LLVM.dll")]
def LLVMInitializeMSP430AsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeNVPTXAsmPrinter", library="LLVM.dll")]
def LLVMInitializeNVPTXAsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializePowerPCAsmPrinter", library="LLVM.dll")]
def LLVMInitializePowerPCAsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeRISCVAsmPrinter", library="LLVM.dll")]
def LLVMInitializeRISCVAsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeSparcAsmPrinter", library="LLVM.dll")]
def LLVMInitializeSparcAsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeSystemZAsmPrinter", library="LLVM.dll")]
def LLVMInitializeSystemZAsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeVEAsmPrinter", library="LLVM.dll")]
def LLVMInitializeVEAsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeWebAssemblyAsmPrinter", library="LLVM.dll")]
def LLVMInitializeWebAssemblyAsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeX86AsmPrinter", library="LLVM.dll")]
def LLVMInitializeX86AsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeXCoreAsmPrinter", library="LLVM.dll")]
def LLVMInitializeXCoreAsmPrinter() : void {}
[extern(cdecl, name="LLVMInitializeAArch64AsmParser", library="LLVM.dll")]
def LLVMInitializeAArch64AsmParser() : void {}
[extern(cdecl, name="LLVMInitializeAMDGPUAsmParser", library="LLVM.dll")]
def LLVMInitializeAMDGPUAsmParser() : void {}
[extern(cdecl, name="LLVMInitializeARMAsmParser", library="LLVM.dll")]
def LLVMInitializeARMAsmParser() : void {}
[extern(cdecl, name="LLVMInitializeAVRAsmParser", library="LLVM.dll")]
def LLVMInitializeAVRAsmParser() : void {}
[extern(cdecl, name="LLVMInitializeBPFAsmParser", library="LLVM.dll")]
def LLVMInitializeBPFAsmParser() : void {}
[extern(cdecl, name="LLVMInitializeHexagonAsmParser", library="LLVM.dll")]
def LLVMInitializeHexagonAsmParser() : void {}
[extern(cdecl, name="LLVMInitializeLanaiAsmParser", library="LLVM.dll")]
def LLVMInitializeLanaiAsmParser() : void {}
[extern(cdecl, name="LLVMInitializeLoongArchAsmParser", library="LLVM.dll")]
def LLVMInitializeLoongArchAsmParser() : void {}
[extern(cdecl, name="LLVMInitializeMipsAsmParser", library="LLVM.dll")]
def LLVMInitializeMipsAsmParser() : void {}
[extern(cdecl, name="LLVMInitializeMSP430AsmParser", library="LLVM.dll")]
def LLVMInitializeMSP430AsmParser() : void {}
[extern(cdecl, name="LLVMInitializePowerPCAsmParser", library="LLVM.dll")]
def LLVMInitializePowerPCAsmParser() : void {}
[extern(cdecl, name="LLVMInitializeRISCVAsmParser", library="LLVM.dll")]
def LLVMInitializeRISCVAsmParser() : void {}
[extern(cdecl, name="LLVMInitializeSparcAsmParser", library="LLVM.dll")]
def LLVMInitializeSparcAsmParser() : void {}
[extern(cdecl, name="LLVMInitializeSystemZAsmParser", library="LLVM.dll")]
def LLVMInitializeSystemZAsmParser() : void {}
[extern(cdecl, name="LLVMInitializeVEAsmParser", library="LLVM.dll")]
def LLVMInitializeVEAsmParser() : void {}
[extern(cdecl, name="LLVMInitializeWebAssemblyAsmParser", library="LLVM.dll")]
def LLVMInitializeWebAssemblyAsmParser() : void {}
[extern(cdecl, name="LLVMInitializeX86AsmParser", library="LLVM.dll")]
def LLVMInitializeX86AsmParser() : void {}
[extern(cdecl, name="LLVMInitializeAArch64Disassembler", library="LLVM.dll")]
def LLVMInitializeAArch64Disassembler() : void {}
[extern(cdecl, name="LLVMInitializeAMDGPUDisassembler", library="LLVM.dll")]
def LLVMInitializeAMDGPUDisassembler() : void {}
[extern(cdecl, name="LLVMInitializeARMDisassembler", library="LLVM.dll")]
def LLVMInitializeARMDisassembler() : void {}
[extern(cdecl, name="LLVMInitializeAVRDisassembler", library="LLVM.dll")]
def LLVMInitializeAVRDisassembler() : void {}
[extern(cdecl, name="LLVMInitializeBPFDisassembler", library="LLVM.dll")]
def LLVMInitializeBPFDisassembler() : void {}
[extern(cdecl, name="LLVMInitializeHexagonDisassembler", library="LLVM.dll")]
def LLVMInitializeHexagonDisassembler() : void {}
[extern(cdecl, name="LLVMInitializeLanaiDisassembler", library="LLVM.dll")]
def LLVMInitializeLanaiDisassembler() : void {}
[extern(cdecl, name="LLVMInitializeLoongArchDisassembler", library="LLVM.dll")]
def LLVMInitializeLoongArchDisassembler() : void {}
[extern(cdecl, name="LLVMInitializeMipsDisassembler", library="LLVM.dll")]
def LLVMInitializeMipsDisassembler() : void {}
[extern(cdecl, name="LLVMInitializeMSP430Disassembler", library="LLVM.dll")]
def LLVMInitializeMSP430Disassembler() : void {}
[extern(cdecl, name="LLVMInitializePowerPCDisassembler", library="LLVM.dll")]
def LLVMInitializePowerPCDisassembler() : void {}
[extern(cdecl, name="LLVMInitializeRISCVDisassembler", library="LLVM.dll")]
def LLVMInitializeRISCVDisassembler() : void {}
[extern(cdecl, name="LLVMInitializeSparcDisassembler", library="LLVM.dll")]
def LLVMInitializeSparcDisassembler() : void {}
[extern(cdecl, name="LLVMInitializeSystemZDisassembler", library="LLVM.dll")]
def LLVMInitializeSystemZDisassembler() : void {}
[extern(cdecl, name="LLVMInitializeVEDisassembler", library="LLVM.dll")]
def LLVMInitializeVEDisassembler() : void {}
[extern(cdecl, name="LLVMInitializeWebAssemblyDisassembler", library="LLVM.dll")]
def LLVMInitializeWebAssemblyDisassembler() : void {}
[extern(cdecl, name="LLVMInitializeX86Disassembler", library="LLVM.dll")]
def LLVMInitializeX86Disassembler() : void {}
[extern(cdecl, name="LLVMInitializeXCoreDisassembler", library="LLVM.dll")]
def LLVMInitializeXCoreDisassembler() : void {}
[extern(cdecl, name="LLVMGetModuleDataLayout", library="LLVM.dll")]
def LLVMGetModuleDataLayout(M : LLVMOpaqueModule? implicit) : LLVMOpaqueTargetData? {}
[extern(cdecl, name="LLVMSetModuleDataLayout", library="LLVM.dll")]
def LLVMSetModuleDataLayout(M : LLVMOpaqueModule? implicit; DL : LLVMOpaqueTargetData? implicit) : void {}
[extern(cdecl, name="LLVMCreateTargetData", library="LLVM.dll")]
def LLVMCreateTargetData(StringRep : string implicit) : LLVMOpaqueTargetData? {}
[extern(cdecl, name="LLVMDisposeTargetData", library="LLVM.dll")]
def LLVMDisposeTargetData(TD : LLVMOpaqueTargetData? implicit) : void {}
[extern(cdecl, name="LLVMAddTargetLibraryInfo", library="LLVM.dll")]
def LLVMAddTargetLibraryInfo(TLI : LLVMOpaqueTargetLibraryInfotData? implicit; PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMCopyStringRepOfTargetData", library="LLVM.dll")]
def LLVMCopyStringRepOfTargetData(TD : LLVMOpaqueTargetData? implicit) : string {}
[extern(cdecl, name="LLVMByteOrder", library="LLVM.dll")]
def LLVMByteOrder(TD : LLVMOpaqueTargetData? implicit) : LLVMByteOrdering {}
[extern(cdecl, name="LLVMPointerSize", library="LLVM.dll")]
def LLVMPointerSize(TD : LLVMOpaqueTargetData? implicit) : uint {}
[extern(cdecl, name="LLVMPointerSizeForAS", library="LLVM.dll")]
def LLVMPointerSizeForAS(TD : LLVMOpaqueTargetData? implicit; AS : uint) : uint {}
[extern(cdecl, name="LLVMIntPtrType", library="LLVM.dll")]
def LLVMIntPtrType(TD : LLVMOpaqueTargetData? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMIntPtrTypeForAS", library="LLVM.dll")]
def LLVMIntPtrTypeForAS(TD : LLVMOpaqueTargetData? implicit; AS : uint) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMIntPtrTypeInContext", library="LLVM.dll")]
def LLVMIntPtrTypeInContext(C : LLVMOpaqueContext? implicit; TD : LLVMOpaqueTargetData? implicit) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMIntPtrTypeForASInContext", library="LLVM.dll")]
def LLVMIntPtrTypeForASInContext(C : LLVMOpaqueContext? implicit; TD : LLVMOpaqueTargetData? implicit; AS : uint) : LLVMOpaqueType? {}
[extern(cdecl, name="LLVMSizeOfTypeInBits", library="LLVM.dll")]
def LLVMSizeOfTypeInBits(TD : LLVMOpaqueTargetData? implicit; Ty : LLVMOpaqueType? implicit) : uint64 {}
[extern(cdecl, name="LLVMStoreSizeOfType", library="LLVM.dll")]
def LLVMStoreSizeOfType(TD : LLVMOpaqueTargetData? implicit; Ty : LLVMOpaqueType? implicit) : uint64 {}
[extern(cdecl, name="LLVMABISizeOfType", library="LLVM.dll")]
def LLVMABISizeOfType(TD : LLVMOpaqueTargetData? implicit; Ty : LLVMOpaqueType? implicit) : uint64 {}
[extern(cdecl, name="LLVMABIAlignmentOfType", library="LLVM.dll")]
def LLVMABIAlignmentOfType(TD : LLVMOpaqueTargetData? implicit; Ty : LLVMOpaqueType? implicit) : uint {}
[extern(cdecl, name="LLVMCallFrameAlignmentOfType", library="LLVM.dll")]
def LLVMCallFrameAlignmentOfType(TD : LLVMOpaqueTargetData? implicit; Ty : LLVMOpaqueType? implicit) : uint {}
[extern(cdecl, name="LLVMPreferredAlignmentOfType", library="LLVM.dll")]
def LLVMPreferredAlignmentOfType(TD : LLVMOpaqueTargetData? implicit; Ty : LLVMOpaqueType? implicit) : uint {}
[extern(cdecl, name="LLVMPreferredAlignmentOfGlobal", library="LLVM.dll")]
def LLVMPreferredAlignmentOfGlobal(TD : LLVMOpaqueTargetData? implicit; GlobalVar : LLVMOpaqueValue? implicit) : uint {}
[extern(cdecl, name="LLVMElementAtOffset", library="LLVM.dll")]
def LLVMElementAtOffset(TD : LLVMOpaqueTargetData? implicit; StructTy : LLVMOpaqueType? implicit; Offset : uint64) : uint {}
[extern(cdecl, name="LLVMOffsetOfElement", library="LLVM.dll")]
def LLVMOffsetOfElement(TD : LLVMOpaqueTargetData? implicit; StructTy : LLVMOpaqueType? implicit; Element : uint) : uint64 {}
[extern(cdecl, name="LLVMGetFirstTarget", library="LLVM.dll")]
def LLVMGetFirstTarget() : LLVMTarget? {}
[extern(cdecl, name="LLVMGetNextTarget", library="LLVM.dll")]
def LLVMGetNextTarget(T : LLVMTarget? implicit) : LLVMTarget? {}
[extern(cdecl, name="LLVMGetTargetFromName", library="LLVM.dll")]
def LLVMGetTargetFromName(Name : string implicit) : LLVMTarget? {}
[extern(cdecl, name="LLVMGetTargetFromTriple", library="LLVM.dll")]
def LLVMGetTargetFromTriple(Triple : string implicit; T : LLVMTarget ?? implicit; ErrorMessage : string? implicit) : int {}
[extern(cdecl, name="LLVMGetTargetName", library="LLVM.dll")]
def LLVMGetTargetName(T : LLVMTarget? implicit) : string {}
[extern(cdecl, name="LLVMGetTargetDescription", library="LLVM.dll")]
def LLVMGetTargetDescription(T : LLVMTarget? implicit) : string {}
[extern(cdecl, name="LLVMTargetHasJIT", library="LLVM.dll")]
def LLVMTargetHasJIT(T : LLVMTarget? implicit) : int {}
[extern(cdecl, name="LLVMTargetHasTargetMachine", library="LLVM.dll")]
def LLVMTargetHasTargetMachine(T : LLVMTarget? implicit) : int {}
[extern(cdecl, name="LLVMTargetHasAsmBackend", library="LLVM.dll")]
def LLVMTargetHasAsmBackend(T : LLVMTarget? implicit) : int {}
[extern(cdecl, name="LLVMCreateTargetMachine", library="LLVM.dll")]
def LLVMCreateTargetMachine(T : LLVMTarget? implicit; Triple : string implicit; CPU : string implicit; Features : string implicit; Level : LLVMCodeGenOptLevel; Reloc : LLVMRelocMode; CodeModel : LLVMCodeModel) : LLVMOpaqueTargetMachine? {}
[extern(cdecl, name="LLVMDisposeTargetMachine", library="LLVM.dll")]
def LLVMDisposeTargetMachine(T : LLVMOpaqueTargetMachine? implicit) : void {}
[extern(cdecl, name="LLVMGetTargetMachineTarget", library="LLVM.dll")]
def LLVMGetTargetMachineTarget(T : LLVMOpaqueTargetMachine? implicit) : LLVMTarget? {}
[extern(cdecl, name="LLVMGetTargetMachineTriple", library="LLVM.dll")]
def LLVMGetTargetMachineTriple(T : LLVMOpaqueTargetMachine? implicit) : string {}
[extern(cdecl, name="LLVMGetTargetMachineCPU", library="LLVM.dll")]
def LLVMGetTargetMachineCPU(T : LLVMOpaqueTargetMachine? implicit) : string {}
[extern(cdecl, name="LLVMGetTargetMachineFeatureString", library="LLVM.dll")]
def LLVMGetTargetMachineFeatureString(T : LLVMOpaqueTargetMachine? implicit) : string {}
[extern(cdecl, name="LLVMCreateTargetDataLayout", library="LLVM.dll")]
def LLVMCreateTargetDataLayout(T : LLVMOpaqueTargetMachine? implicit) : LLVMOpaqueTargetData? {}
[extern(cdecl, name="LLVMSetTargetMachineAsmVerbosity", library="LLVM.dll")]
def LLVMSetTargetMachineAsmVerbosity(T : LLVMOpaqueTargetMachine? implicit; VerboseAsm : int) : void {}
[extern(cdecl, name="LLVMTargetMachineEmitToFile", library="LLVM.dll")]
def LLVMTargetMachineEmitToFile(T : LLVMOpaqueTargetMachine? implicit; M : LLVMOpaqueModule? implicit; Filename : string implicit; codegen : LLVMCodeGenFileType; ErrorMessage : string? implicit) : int {}
[extern(cdecl, name="LLVMTargetMachineEmitToMemoryBuffer", library="LLVM.dll")]
def LLVMTargetMachineEmitToMemoryBuffer(T : LLVMOpaqueTargetMachine? implicit; M : LLVMOpaqueModule? implicit; codegen : LLVMCodeGenFileType; ErrorMessage : string? implicit; OutMemBuf : LLVMOpaqueMemoryBuffer ?? implicit) : int {}
[extern(cdecl, name="LLVMGetDefaultTargetTriple", library="LLVM.dll")]
def LLVMGetDefaultTargetTriple() : string {}
[extern(cdecl, name="LLVMNormalizeTargetTriple", library="LLVM.dll")]
def LLVMNormalizeTargetTriple(triple : string implicit) : string {}
[extern(cdecl, name="LLVMGetHostCPUName", library="LLVM.dll")]
def LLVMGetHostCPUName() : string {}
[extern(cdecl, name="LLVMGetHostCPUFeatures", library="LLVM.dll")]
def LLVMGetHostCPUFeatures() : string {}
[extern(cdecl, name="LLVMAddAnalysisPasses", library="LLVM.dll")]
def LLVMAddAnalysisPasses(T : LLVMOpaqueTargetMachine? implicit; PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMLinkInMCJIT", library="LLVM.dll")]
def LLVMLinkInMCJIT() : void {}
[extern(cdecl, name="LLVMLinkInInterpreter", library="LLVM.dll")]
def LLVMLinkInInterpreter() : void {}
[extern(cdecl, name="LLVMCreateGenericValueOfInt", library="LLVM.dll")]
def LLVMCreateGenericValueOfInt(Ty : LLVMOpaqueType? implicit; N : uint64; IsSigned : int) : LLVMOpaqueGenericValue? {}
[extern(cdecl, name="LLVMCreateGenericValueOfPointer", library="LLVM.dll")]
def LLVMCreateGenericValueOfPointer(P : void? implicit) : LLVMOpaqueGenericValue? {}
[extern(cdecl, name="LLVMCreateGenericValueOfFloat", library="LLVM.dll")]
def LLVMCreateGenericValueOfFloat(Ty : LLVMOpaqueType? implicit; N : double) : LLVMOpaqueGenericValue? {}
[extern(cdecl, name="LLVMGenericValueIntWidth", library="LLVM.dll")]
def LLVMGenericValueIntWidth(GenValRef : LLVMOpaqueGenericValue? implicit) : uint {}
[extern(cdecl, name="LLVMGenericValueToInt", library="LLVM.dll")]
def LLVMGenericValueToInt(GenVal : LLVMOpaqueGenericValue? implicit; IsSigned : int) : uint64 {}
[extern(cdecl, name="LLVMGenericValueToPointer", library="LLVM.dll")]
def LLVMGenericValueToPointer(GenVal : LLVMOpaqueGenericValue? implicit) : void? {}
[extern(cdecl, name="LLVMGenericValueToFloat", library="LLVM.dll")]
def LLVMGenericValueToFloat(TyRef : LLVMOpaqueType? implicit; GenVal : LLVMOpaqueGenericValue? implicit) : double {}
[extern(cdecl, name="LLVMDisposeGenericValue", library="LLVM.dll")]
def LLVMDisposeGenericValue(GenVal : LLVMOpaqueGenericValue? implicit) : void {}
[extern(cdecl, name="LLVMCreateExecutionEngineForModule", library="LLVM.dll")]
def LLVMCreateExecutionEngineForModule(OutEE : LLVMOpaqueExecutionEngine ?? implicit; M : LLVMOpaqueModule? implicit; OutError : string? implicit) : int {}
[extern(cdecl, name="LLVMCreateInterpreterForModule", library="LLVM.dll")]
def LLVMCreateInterpreterForModule(OutInterp : LLVMOpaqueExecutionEngine ?? implicit; M : LLVMOpaqueModule? implicit; OutError : string? implicit) : int {}
[extern(cdecl, name="LLVMCreateJITCompilerForModule", library="LLVM.dll")]
def LLVMCreateJITCompilerForModule(OutJIT : LLVMOpaqueExecutionEngine ?? implicit; M : LLVMOpaqueModule? implicit; OptLevel : uint; OutError : string? implicit) : int {}
[extern(cdecl, name="LLVMInitializeMCJITCompilerOptions", library="LLVM.dll")]
def LLVMInitializeMCJITCompilerOptions(Options : LLVMMCJITCompilerOptions? implicit; SizeOfOptions : uint64) : void {}
[extern(cdecl, name="LLVMCreateMCJITCompilerForModule", library="LLVM.dll")]
def LLVMCreateMCJITCompilerForModule(OutJIT : LLVMOpaqueExecutionEngine ?? implicit; M : LLVMOpaqueModule? implicit; Options : LLVMMCJITCompilerOptions? implicit; SizeOfOptions : uint64; OutError : string? implicit) : int {}
[extern(cdecl, name="LLVMDisposeExecutionEngine", library="LLVM.dll")]
def LLVMDisposeExecutionEngine(EE : LLVMOpaqueExecutionEngine? implicit) : void {}
[extern(cdecl, name="LLVMRunStaticConstructors", library="LLVM.dll")]
def LLVMRunStaticConstructors(EE : LLVMOpaqueExecutionEngine? implicit) : void {}
[extern(cdecl, name="LLVMRunStaticDestructors", library="LLVM.dll")]
def LLVMRunStaticDestructors(EE : LLVMOpaqueExecutionEngine? implicit) : void {}
[extern(cdecl, name="LLVMRunFunctionAsMain", library="LLVM.dll")]
def LLVMRunFunctionAsMain(EE : LLVMOpaqueExecutionEngine? implicit; F : LLVMOpaqueValue? implicit; ArgC : uint; ArgV : string const? implicit; EnvP : string const? implicit) : int {}
[extern(cdecl, name="LLVMRunFunction", library="LLVM.dll")]
def LLVMRunFunction(EE : LLVMOpaqueExecutionEngine? implicit; F : LLVMOpaqueValue? implicit; NumArgs : uint; Args : LLVMOpaqueGenericValue ?? implicit) : LLVMOpaqueGenericValue? {}
[extern(cdecl, name="LLVMFreeMachineCodeForFunction", library="LLVM.dll")]
def LLVMFreeMachineCodeForFunction(EE : LLVMOpaqueExecutionEngine? implicit; F : LLVMOpaqueValue? implicit) : void {}
[extern(cdecl, name="LLVMAddModule", library="LLVM.dll")]
def LLVMAddModule(EE : LLVMOpaqueExecutionEngine? implicit; M : LLVMOpaqueModule? implicit) : void {}
[extern(cdecl, name="LLVMRemoveModule", library="LLVM.dll")]
def LLVMRemoveModule(EE : LLVMOpaqueExecutionEngine? implicit; M : LLVMOpaqueModule? implicit; OutMod : LLVMOpaqueModule ?? implicit; OutError : string? implicit) : int {}
[extern(cdecl, name="LLVMFindFunction", library="LLVM.dll")]
def LLVMFindFunction(EE : LLVMOpaqueExecutionEngine? implicit; Name : string implicit; OutFn : LLVMOpaqueValue ?? implicit) : int {}
[extern(cdecl, name="LLVMRecompileAndRelinkFunction", library="LLVM.dll")]
def LLVMRecompileAndRelinkFunction(EE : LLVMOpaqueExecutionEngine? implicit; Fn : LLVMOpaqueValue? implicit) : void? {}
[extern(cdecl, name="LLVMGetExecutionEngineTargetData", library="LLVM.dll")]
def LLVMGetExecutionEngineTargetData(EE : LLVMOpaqueExecutionEngine? implicit) : LLVMOpaqueTargetData? {}
[extern(cdecl, name="LLVMGetExecutionEngineTargetMachine", library="LLVM.dll")]
def LLVMGetExecutionEngineTargetMachine(EE : LLVMOpaqueExecutionEngine? implicit) : LLVMOpaqueTargetMachine? {}
[extern(cdecl, name="LLVMAddGlobalMapping", library="LLVM.dll")]
def LLVMAddGlobalMapping(EE : LLVMOpaqueExecutionEngine? implicit; Global : LLVMOpaqueValue? implicit; Addr : void? implicit) : void {}
[extern(cdecl, name="LLVMGetPointerToGlobal", library="LLVM.dll")]
def LLVMGetPointerToGlobal(EE : LLVMOpaqueExecutionEngine? implicit; Global : LLVMOpaqueValue? implicit) : void? {}
[extern(cdecl, name="LLVMGetGlobalValueAddress", library="LLVM.dll")]
def LLVMGetGlobalValueAddress(EE : LLVMOpaqueExecutionEngine? implicit; Name : string implicit) : uint64 {}
[extern(cdecl, name="LLVMGetFunctionAddress", library="LLVM.dll")]
def LLVMGetFunctionAddress(EE : LLVMOpaqueExecutionEngine? implicit; Name : string implicit) : uint64 {}
[extern(cdecl, name="LLVMExecutionEngineGetErrMsg", library="LLVM.dll")]
def LLVMExecutionEngineGetErrMsg(EE : LLVMOpaqueExecutionEngine? implicit; OutError : string? implicit) : int {}
[extern(cdecl, name="LLVMDisposeMCJITMemoryManager", library="LLVM.dll")]
def LLVMDisposeMCJITMemoryManager(MM : LLVMOpaqueMCJITMemoryManager? implicit) : void {}
[extern(cdecl, name="LLVMCreateGDBRegistrationListener", library="LLVM.dll")]
def LLVMCreateGDBRegistrationListener() : LLVMOpaqueJITEventListener? {}
[extern(cdecl, name="LLVMCreateIntelJITEventListener", library="LLVM.dll")]
def LLVMCreateIntelJITEventListener() : LLVMOpaqueJITEventListener? {}
[extern(cdecl, name="LLVMCreateOProfileJITEventListener", library="LLVM.dll")]
def LLVMCreateOProfileJITEventListener() : LLVMOpaqueJITEventListener? {}
[extern(cdecl, name="LLVMCreatePerfJITEventListener", library="LLVM.dll")]
def LLVMCreatePerfJITEventListener() : LLVMOpaqueJITEventListener? {}
[extern(cdecl, name="LLVMInitializeTransformUtils", library="LLVM.dll")]
def LLVMInitializeTransformUtils(R : LLVMOpaquePassRegistry? implicit) : void {}
[extern(cdecl, name="LLVMInitializeScalarOpts", library="LLVM.dll")]
def LLVMInitializeScalarOpts(R : LLVMOpaquePassRegistry? implicit) : void {}
[extern(cdecl, name="LLVMInitializeVectorization", library="LLVM.dll")]
def LLVMInitializeVectorization(R : LLVMOpaquePassRegistry? implicit) : void {}
[extern(cdecl, name="LLVMInitializeInstCombine", library="LLVM.dll")]
def LLVMInitializeInstCombine(R : LLVMOpaquePassRegistry? implicit) : void {}
[extern(cdecl, name="LLVMInitializeIPO", library="LLVM.dll")]
def LLVMInitializeIPO(R : LLVMOpaquePassRegistry? implicit) : void {}
[extern(cdecl, name="LLVMInitializeAnalysis", library="LLVM.dll")]
def LLVMInitializeAnalysis(R : LLVMOpaquePassRegistry? implicit) : void {}
[extern(cdecl, name="LLVMInitializeIPA", library="LLVM.dll")]
def LLVMInitializeIPA(R : LLVMOpaquePassRegistry? implicit) : void {}
[extern(cdecl, name="LLVMInitializeCodeGen", library="LLVM.dll")]
def LLVMInitializeCodeGen(R : LLVMOpaquePassRegistry? implicit) : void {}
[extern(cdecl, name="LLVMInitializeTarget", library="LLVM.dll")]
def LLVMInitializeTarget(R : LLVMOpaquePassRegistry? implicit) : void {}
[extern(cdecl, name="LLVMParseIRInContext", library="LLVM.dll")]
def LLVMParseIRInContext(ContextRef : LLVMOpaqueContext? implicit; MemBuf : LLVMOpaqueMemoryBuffer? implicit; OutM : LLVMOpaqueModule ?? implicit; OutMessage : string? implicit) : int {}
[extern(cdecl, name="LLVMLinkModules2", library="LLVM.dll")]
def LLVMLinkModules2(Dest : LLVMOpaqueModule? implicit; Src : LLVMOpaqueModule? implicit) : int {}
[extern(cdecl, name="LLVMOrcExecutionSessionGetSymbolStringPool", library="LLVM.dll")]
def LLVMOrcExecutionSessionGetSymbolStringPool(ES : LLVMOrcOpaqueExecutionSession? implicit) : LLVMOrcOpaqueSymbolStringPool? {}
[extern(cdecl, name="LLVMOrcSymbolStringPoolClearDeadEntries", library="LLVM.dll")]
def LLVMOrcSymbolStringPoolClearDeadEntries(SSP : LLVMOrcOpaqueSymbolStringPool? implicit) : void {}
[extern(cdecl, name="LLVMOrcExecutionSessionIntern", library="LLVM.dll")]
def LLVMOrcExecutionSessionIntern(ES : LLVMOrcOpaqueExecutionSession? implicit; Name : string implicit) : LLVMOrcOpaqueSymbolStringPoolEntry? {}
[extern(cdecl, name="LLVMOrcRetainSymbolStringPoolEntry", library="LLVM.dll")]
def LLVMOrcRetainSymbolStringPoolEntry(S : LLVMOrcOpaqueSymbolStringPoolEntry? implicit) : void {}
[extern(cdecl, name="LLVMOrcReleaseSymbolStringPoolEntry", library="LLVM.dll")]
def LLVMOrcReleaseSymbolStringPoolEntry(S : LLVMOrcOpaqueSymbolStringPoolEntry? implicit) : void {}
[extern(cdecl, name="LLVMOrcSymbolStringPoolEntryStr", library="LLVM.dll")]
def LLVMOrcSymbolStringPoolEntryStr(S : LLVMOrcOpaqueSymbolStringPoolEntry? implicit) : string {}
[extern(cdecl, name="LLVMOrcReleaseResourceTracker", library="LLVM.dll")]
def LLVMOrcReleaseResourceTracker(RT : LLVMOrcOpaqueResourceTracker? implicit) : void {}
[extern(cdecl, name="LLVMOrcResourceTrackerTransferTo", library="LLVM.dll")]
def LLVMOrcResourceTrackerTransferTo(SrcRT : LLVMOrcOpaqueResourceTracker? implicit; DstRT : LLVMOrcOpaqueResourceTracker? implicit) : void {}
[extern(cdecl, name="LLVMOrcResourceTrackerRemove", library="LLVM.dll")]
def LLVMOrcResourceTrackerRemove(RT : LLVMOrcOpaqueResourceTracker? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcDisposeDefinitionGenerator", library="LLVM.dll")]
def LLVMOrcDisposeDefinitionGenerator(DG : LLVMOrcOpaqueDefinitionGenerator? implicit) : void {}
[extern(cdecl, name="LLVMOrcDisposeMaterializationUnit", library="LLVM.dll")]
def LLVMOrcDisposeMaterializationUnit(MU : LLVMOrcOpaqueMaterializationUnit? implicit) : void {}
[extern(cdecl, name="LLVMOrcAbsoluteSymbols", library="LLVM.dll")]
def LLVMOrcAbsoluteSymbols(Syms : LLVMOrcCSymbolMapPair? implicit; NumPairs : uint64) : LLVMOrcOpaqueMaterializationUnit? {}
[extern(cdecl, name="LLVMOrcLazyReexports", library="LLVM.dll")]
def LLVMOrcLazyReexports(LCTM : LLVMOrcOpaqueLazyCallThroughManager? implicit; ISM : LLVMOrcOpaqueIndirectStubsManager? implicit; SourceRef : LLVMOrcOpaqueJITDylib? implicit; CallableAliases : LLVMOrcCSymbolAliasMapPair? implicit; NumPairs : uint64) : LLVMOrcOpaqueMaterializationUnit? {}
[extern(cdecl, name="LLVMOrcDisposeMaterializationResponsibility", library="LLVM.dll")]
def LLVMOrcDisposeMaterializationResponsibility(MR : LLVMOrcOpaqueMaterializationResponsibility? implicit) : void {}
[extern(cdecl, name="LLVMOrcMaterializationResponsibilityGetTargetDylib", library="LLVM.dll")]
def LLVMOrcMaterializationResponsibilityGetTargetDylib(MR : LLVMOrcOpaqueMaterializationResponsibility? implicit) : LLVMOrcOpaqueJITDylib? {}
[extern(cdecl, name="LLVMOrcMaterializationResponsibilityGetExecutionSession", library="LLVM.dll")]
def LLVMOrcMaterializationResponsibilityGetExecutionSession(MR : LLVMOrcOpaqueMaterializationResponsibility? implicit) : LLVMOrcOpaqueExecutionSession? {}
[extern(cdecl, name="LLVMOrcMaterializationResponsibilityGetSymbols", library="LLVM.dll")]
def LLVMOrcMaterializationResponsibilityGetSymbols(MR : LLVMOrcOpaqueMaterializationResponsibility? implicit; NumPairs : uint64? implicit) : LLVMOrcCSymbolFlagsMapPair? {}
[extern(cdecl, name="LLVMOrcDisposeCSymbolFlagsMap", library="LLVM.dll")]
def LLVMOrcDisposeCSymbolFlagsMap(Pairs : LLVMOrcCSymbolFlagsMapPair? implicit) : void {}
[extern(cdecl, name="LLVMOrcMaterializationResponsibilityGetInitializerSymbol", library="LLVM.dll")]
def LLVMOrcMaterializationResponsibilityGetInitializerSymbol(MR : LLVMOrcOpaqueMaterializationResponsibility? implicit) : LLVMOrcOpaqueSymbolStringPoolEntry? {}
[extern(cdecl, name="LLVMOrcMaterializationResponsibilityGetRequestedSymbols", library="LLVM.dll")]
def LLVMOrcMaterializationResponsibilityGetRequestedSymbols(MR : LLVMOrcOpaqueMaterializationResponsibility? implicit; NumSymbols : uint64? implicit) : LLVMOrcOpaqueSymbolStringPoolEntry ?? {}
[extern(cdecl, name="LLVMOrcDisposeSymbols", library="LLVM.dll")]
def LLVMOrcDisposeSymbols(Symbols : LLVMOrcOpaqueSymbolStringPoolEntry ?? implicit) : void {}
[extern(cdecl, name="LLVMOrcMaterializationResponsibilityNotifyResolved", library="LLVM.dll")]
def LLVMOrcMaterializationResponsibilityNotifyResolved(MR : LLVMOrcOpaqueMaterializationResponsibility? implicit; Symbols : LLVMOrcCSymbolMapPair? implicit; NumPairs : uint64) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcMaterializationResponsibilityNotifyEmitted", library="LLVM.dll")]
def LLVMOrcMaterializationResponsibilityNotifyEmitted(MR : LLVMOrcOpaqueMaterializationResponsibility? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcMaterializationResponsibilityDefineMaterializing", library="LLVM.dll")]
def LLVMOrcMaterializationResponsibilityDefineMaterializing(MR : LLVMOrcOpaqueMaterializationResponsibility? implicit; Pairs : LLVMOrcCSymbolFlagsMapPair? implicit; NumPairs : uint64) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcMaterializationResponsibilityFailMaterialization", library="LLVM.dll")]
def LLVMOrcMaterializationResponsibilityFailMaterialization(MR : LLVMOrcOpaqueMaterializationResponsibility? implicit) : void {}
[extern(cdecl, name="LLVMOrcMaterializationResponsibilityReplace", library="LLVM.dll")]
def LLVMOrcMaterializationResponsibilityReplace(MR : LLVMOrcOpaqueMaterializationResponsibility? implicit; MU : LLVMOrcOpaqueMaterializationUnit? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcMaterializationResponsibilityDelegate", library="LLVM.dll")]
def LLVMOrcMaterializationResponsibilityDelegate(MR : LLVMOrcOpaqueMaterializationResponsibility? implicit; Symbols : LLVMOrcOpaqueSymbolStringPoolEntry ?? implicit; NumSymbols : uint64; Result : LLVMOrcOpaqueMaterializationResponsibility ?? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcMaterializationResponsibilityAddDependencies", library="LLVM.dll")]
def LLVMOrcMaterializationResponsibilityAddDependencies(MR : LLVMOrcOpaqueMaterializationResponsibility? implicit; Name : LLVMOrcOpaqueSymbolStringPoolEntry? implicit; Dependencies : LLVMOrcCDependenceMapPair? implicit; NumPairs : uint64) : void {}
[extern(cdecl, name="LLVMOrcMaterializationResponsibilityAddDependenciesForAll", library="LLVM.dll")]
def LLVMOrcMaterializationResponsibilityAddDependenciesForAll(MR : LLVMOrcOpaqueMaterializationResponsibility? implicit; Dependencies : LLVMOrcCDependenceMapPair? implicit; NumPairs : uint64) : void {}
[extern(cdecl, name="LLVMOrcExecutionSessionCreateBareJITDylib", library="LLVM.dll")]
def LLVMOrcExecutionSessionCreateBareJITDylib(ES : LLVMOrcOpaqueExecutionSession? implicit; Name : string implicit) : LLVMOrcOpaqueJITDylib? {}
[extern(cdecl, name="LLVMOrcExecutionSessionCreateJITDylib", library="LLVM.dll")]
def LLVMOrcExecutionSessionCreateJITDylib(ES : LLVMOrcOpaqueExecutionSession? implicit; Result : LLVMOrcOpaqueJITDylib ?? implicit; Name : string implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcExecutionSessionGetJITDylibByName", library="LLVM.dll")]
def LLVMOrcExecutionSessionGetJITDylibByName(ES : LLVMOrcOpaqueExecutionSession? implicit; Name : string implicit) : LLVMOrcOpaqueJITDylib? {}
[extern(cdecl, name="LLVMOrcJITDylibCreateResourceTracker", library="LLVM.dll")]
def LLVMOrcJITDylibCreateResourceTracker(JD : LLVMOrcOpaqueJITDylib? implicit) : LLVMOrcOpaqueResourceTracker? {}
[extern(cdecl, name="LLVMOrcJITDylibGetDefaultResourceTracker", library="LLVM.dll")]
def LLVMOrcJITDylibGetDefaultResourceTracker(JD : LLVMOrcOpaqueJITDylib? implicit) : LLVMOrcOpaqueResourceTracker? {}
[extern(cdecl, name="LLVMOrcJITDylibDefine", library="LLVM.dll")]
def LLVMOrcJITDylibDefine(JD : LLVMOrcOpaqueJITDylib? implicit; MU : LLVMOrcOpaqueMaterializationUnit? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcJITDylibClear", library="LLVM.dll")]
def LLVMOrcJITDylibClear(JD : LLVMOrcOpaqueJITDylib? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcJITDylibAddGenerator", library="LLVM.dll")]
def LLVMOrcJITDylibAddGenerator(JD : LLVMOrcOpaqueJITDylib? implicit; DG : LLVMOrcOpaqueDefinitionGenerator? implicit) : void {}
[extern(cdecl, name="LLVMOrcLookupStateContinueLookup", library="LLVM.dll")]
def LLVMOrcLookupStateContinueLookup(S : LLVMOrcOpaqueLookupState? implicit; Err : LLVMOpaqueError? implicit) : void {}
[extern(cdecl, name="LLVMOrcCreateStaticLibrarySearchGeneratorForPath", library="LLVM.dll")]
def LLVMOrcCreateStaticLibrarySearchGeneratorForPath(Result : LLVMOrcOpaqueDefinitionGenerator ?? implicit; ObjLayer : LLVMOrcOpaqueObjectLayer? implicit; FileName : string implicit; TargetTriple : string implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcCreateNewThreadSafeContext", library="LLVM.dll")]
def LLVMOrcCreateNewThreadSafeContext() : LLVMOrcOpaqueThreadSafeContext? {}
[extern(cdecl, name="LLVMOrcThreadSafeContextGetContext", library="LLVM.dll")]
def LLVMOrcThreadSafeContextGetContext(TSCtx : LLVMOrcOpaqueThreadSafeContext? implicit) : LLVMOpaqueContext? {}
[extern(cdecl, name="LLVMOrcDisposeThreadSafeContext", library="LLVM.dll")]
def LLVMOrcDisposeThreadSafeContext(TSCtx : LLVMOrcOpaqueThreadSafeContext? implicit) : void {}
[extern(cdecl, name="LLVMOrcCreateNewThreadSafeModule", library="LLVM.dll")]
def LLVMOrcCreateNewThreadSafeModule(M : LLVMOpaqueModule? implicit; TSCtx : LLVMOrcOpaqueThreadSafeContext? implicit) : LLVMOrcOpaqueThreadSafeModule? {}
[extern(cdecl, name="LLVMOrcDisposeThreadSafeModule", library="LLVM.dll")]
def LLVMOrcDisposeThreadSafeModule(TSM : LLVMOrcOpaqueThreadSafeModule? implicit) : void {}
[extern(cdecl, name="LLVMOrcJITTargetMachineBuilderDetectHost", library="LLVM.dll")]
def LLVMOrcJITTargetMachineBuilderDetectHost(Result : LLVMOrcOpaqueJITTargetMachineBuilder ?? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcJITTargetMachineBuilderCreateFromTargetMachine", library="LLVM.dll")]
def LLVMOrcJITTargetMachineBuilderCreateFromTargetMachine(TM : LLVMOpaqueTargetMachine? implicit) : LLVMOrcOpaqueJITTargetMachineBuilder? {}
[extern(cdecl, name="LLVMOrcDisposeJITTargetMachineBuilder", library="LLVM.dll")]
def LLVMOrcDisposeJITTargetMachineBuilder(JTMB : LLVMOrcOpaqueJITTargetMachineBuilder? implicit) : void {}
[extern(cdecl, name="LLVMOrcJITTargetMachineBuilderGetTargetTriple", library="LLVM.dll")]
def LLVMOrcJITTargetMachineBuilderGetTargetTriple(JTMB : LLVMOrcOpaqueJITTargetMachineBuilder? implicit) : string {}
[extern(cdecl, name="LLVMOrcJITTargetMachineBuilderSetTargetTriple", library="LLVM.dll")]
def LLVMOrcJITTargetMachineBuilderSetTargetTriple(JTMB : LLVMOrcOpaqueJITTargetMachineBuilder? implicit; TargetTriple : string implicit) : void {}
[extern(cdecl, name="LLVMOrcObjectLayerAddObjectFile", library="LLVM.dll")]
def LLVMOrcObjectLayerAddObjectFile(ObjLayer : LLVMOrcOpaqueObjectLayer? implicit; JD : LLVMOrcOpaqueJITDylib? implicit; ObjBuffer : LLVMOpaqueMemoryBuffer? implicit) : LLVMOpaqueError? {}
[extern(cdecl, late, name="LLVMOrcObjectLayerAddObjectFileWithRT", library="LLVM.dll")]
def LLVMOrcObjectLayerAddObjectFileWithRT(ObjLayer : LLVMOrcOpaqueObjectLayer? implicit; RT : LLVMOrcOpaqueResourceTracker? implicit; ObjBuffer : LLVMOpaqueMemoryBuffer? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcObjectLayerEmit", library="LLVM.dll")]
def LLVMOrcObjectLayerEmit(ObjLayer : LLVMOrcOpaqueObjectLayer? implicit; R : LLVMOrcOpaqueMaterializationResponsibility? implicit; ObjBuffer : LLVMOpaqueMemoryBuffer? implicit) : void {}
[extern(cdecl, name="LLVMOrcDisposeObjectLayer", library="LLVM.dll")]
def LLVMOrcDisposeObjectLayer(ObjLayer : LLVMOrcOpaqueObjectLayer? implicit) : void {}
[extern(cdecl, name="LLVMOrcIRTransformLayerEmit", library="LLVM.dll")]
def LLVMOrcIRTransformLayerEmit(IRTransformLayer : LLVMOrcOpaqueIRTransformLayer? implicit; MR : LLVMOrcOpaqueMaterializationResponsibility? implicit; TSM : LLVMOrcOpaqueThreadSafeModule? implicit) : void {}
[extern(cdecl, name="LLVMOrcCreateLocalIndirectStubsManager", library="LLVM.dll")]
def LLVMOrcCreateLocalIndirectStubsManager(TargetTriple : string implicit) : LLVMOrcOpaqueIndirectStubsManager? {}
[extern(cdecl, name="LLVMOrcDisposeIndirectStubsManager", library="LLVM.dll")]
def LLVMOrcDisposeIndirectStubsManager(ISM : LLVMOrcOpaqueIndirectStubsManager? implicit) : void {}
[extern(cdecl, name="LLVMOrcCreateLocalLazyCallThroughManager", library="LLVM.dll")]
def LLVMOrcCreateLocalLazyCallThroughManager(TargetTriple : string implicit; ES : LLVMOrcOpaqueExecutionSession? implicit; ErrorHandlerAddr : uint64; LCTM : LLVMOrcOpaqueLazyCallThroughManager ?? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcDisposeLazyCallThroughManager", library="LLVM.dll")]
def LLVMOrcDisposeLazyCallThroughManager(LCTM : LLVMOrcOpaqueLazyCallThroughManager? implicit) : void {}
[extern(cdecl, name="LLVMOrcCreateDumpObjects", library="LLVM.dll")]
def LLVMOrcCreateDumpObjects(DumpDir : string implicit; IdentifierOverride : string implicit) : LLVMOrcOpaqueDumpObjects? {}
[extern(cdecl, name="LLVMOrcDisposeDumpObjects", library="LLVM.dll")]
def LLVMOrcDisposeDumpObjects(DumpObjects : LLVMOrcOpaqueDumpObjects? implicit) : void {}
[extern(cdecl, name="LLVMOrcDumpObjects_CallOperator", library="LLVM.dll")]
def LLVMOrcDumpObjects_CallOperator(DumpObjects : LLVMOrcOpaqueDumpObjects? implicit; ObjBuffer : LLVMOpaqueMemoryBuffer ?? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcCreateLLJITBuilder", library="LLVM.dll")]
def LLVMOrcCreateLLJITBuilder() : LLVMOrcOpaqueLLJITBuilder? {}
[extern(cdecl, name="LLVMOrcDisposeLLJITBuilder", library="LLVM.dll")]
def LLVMOrcDisposeLLJITBuilder(Builder : LLVMOrcOpaqueLLJITBuilder? implicit) : void {}
[extern(cdecl, name="LLVMOrcLLJITBuilderSetJITTargetMachineBuilder", library="LLVM.dll")]
def LLVMOrcLLJITBuilderSetJITTargetMachineBuilder(Builder : LLVMOrcOpaqueLLJITBuilder? implicit; JTMB : LLVMOrcOpaqueJITTargetMachineBuilder? implicit) : void {}
[extern(cdecl, name="LLVMOrcCreateLLJIT", library="LLVM.dll")]
def LLVMOrcCreateLLJIT(Result : LLVMOrcOpaqueLLJIT ?? implicit; Builder : LLVMOrcOpaqueLLJITBuilder? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcDisposeLLJIT", library="LLVM.dll")]
def LLVMOrcDisposeLLJIT(J : LLVMOrcOpaqueLLJIT? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcLLJITGetExecutionSession", library="LLVM.dll")]
def LLVMOrcLLJITGetExecutionSession(J : LLVMOrcOpaqueLLJIT? implicit) : LLVMOrcOpaqueExecutionSession? {}
[extern(cdecl, name="LLVMOrcLLJITGetMainJITDylib", library="LLVM.dll")]
def LLVMOrcLLJITGetMainJITDylib(J : LLVMOrcOpaqueLLJIT? implicit) : LLVMOrcOpaqueJITDylib? {}
[extern(cdecl, name="LLVMOrcLLJITGetTripleString", library="LLVM.dll")]
def LLVMOrcLLJITGetTripleString(J : LLVMOrcOpaqueLLJIT? implicit) : string {}
[extern(cdecl, name="LLVMOrcLLJITGetGlobalPrefix", library="LLVM.dll")]
def LLVMOrcLLJITGetGlobalPrefix(J : LLVMOrcOpaqueLLJIT? implicit) : int8 {}
[extern(cdecl, name="LLVMOrcLLJITMangleAndIntern", library="LLVM.dll")]
def LLVMOrcLLJITMangleAndIntern(J : LLVMOrcOpaqueLLJIT? implicit; UnmangledName : string implicit) : LLVMOrcOpaqueSymbolStringPoolEntry? {}
[extern(cdecl, name="LLVMOrcLLJITAddObjectFile", library="LLVM.dll")]
def LLVMOrcLLJITAddObjectFile(J : LLVMOrcOpaqueLLJIT? implicit; JD : LLVMOrcOpaqueJITDylib? implicit; ObjBuffer : LLVMOpaqueMemoryBuffer? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcLLJITAddObjectFileWithRT", library="LLVM.dll")]
def LLVMOrcLLJITAddObjectFileWithRT(J : LLVMOrcOpaqueLLJIT? implicit; RT : LLVMOrcOpaqueResourceTracker? implicit; ObjBuffer : LLVMOpaqueMemoryBuffer? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcLLJITAddLLVMIRModule", library="LLVM.dll")]
def LLVMOrcLLJITAddLLVMIRModule(J : LLVMOrcOpaqueLLJIT? implicit; JD : LLVMOrcOpaqueJITDylib? implicit; TSM : LLVMOrcOpaqueThreadSafeModule? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcLLJITAddLLVMIRModuleWithRT", library="LLVM.dll")]
def LLVMOrcLLJITAddLLVMIRModuleWithRT(J : LLVMOrcOpaqueLLJIT? implicit; JD : LLVMOrcOpaqueResourceTracker? implicit; TSM : LLVMOrcOpaqueThreadSafeModule? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcLLJITLookup", library="LLVM.dll")]
def LLVMOrcLLJITLookup(J : LLVMOrcOpaqueLLJIT? implicit; Result : uint64? implicit; Name : string implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMOrcLLJITGetObjLinkingLayer", library="LLVM.dll")]
def LLVMOrcLLJITGetObjLinkingLayer(J : LLVMOrcOpaqueLLJIT? implicit) : LLVMOrcOpaqueObjectLayer? {}
[extern(cdecl, name="LLVMOrcLLJITGetObjTransformLayer", library="LLVM.dll")]
def LLVMOrcLLJITGetObjTransformLayer(J : LLVMOrcOpaqueLLJIT? implicit) : LLVMOrcOpaqueObjectTransformLayer? {}
[extern(cdecl, name="LLVMOrcLLJITGetIRTransformLayer", library="LLVM.dll")]
def LLVMOrcLLJITGetIRTransformLayer(J : LLVMOrcOpaqueLLJIT? implicit) : LLVMOrcOpaqueIRTransformLayer? {}
[extern(cdecl, name="LLVMOrcLLJITGetDataLayoutStr", library="LLVM.dll")]
def LLVMOrcLLJITGetDataLayoutStr(J : LLVMOrcOpaqueLLJIT? implicit) : string {}
[extern(cdecl, late, name="lto_get_version", library="LLVM.dll")]
def lto_get_version() : string {}
[extern(cdecl, late, name="lto_get_error_message", library="LLVM.dll")]
def lto_get_error_message() : string {}
[extern(cdecl, late, name="lto_module_is_object_file", library="LLVM.dll")]
def lto_module_is_object_file(path : string implicit) : bool {}
[extern(cdecl, late, name="lto_module_is_object_file_for_target", library="LLVM.dll")]
def lto_module_is_object_file_for_target(path : string implicit; target_triple_prefix : string implicit) : bool {}
[extern(cdecl, late, name="lto_module_has_objc_category", library="LLVM.dll")]
def lto_module_has_objc_category(mem : void const? implicit; length : uint64) : bool {}
[extern(cdecl, late, name="lto_module_is_object_file_in_memory", library="LLVM.dll")]
def lto_module_is_object_file_in_memory(mem : void const? implicit; length : uint64) : bool {}
[extern(cdecl, late, name="lto_module_is_object_file_in_memory_for_target", library="LLVM.dll")]
def lto_module_is_object_file_in_memory_for_target(mem : void const? implicit; length : uint64; target_triple_prefix : string implicit) : bool {}
[extern(cdecl, late, name="lto_module_create", library="LLVM.dll")]
def lto_module_create(path : string implicit) : LLVMOpaqueLTOModule? {}
[extern(cdecl, late, name="lto_module_create_from_memory", library="LLVM.dll")]
def lto_module_create_from_memory(mem : void const? implicit; length : uint64) : LLVMOpaqueLTOModule? {}
[extern(cdecl, late, name="lto_module_create_from_memory_with_path", library="LLVM.dll")]
def lto_module_create_from_memory_with_path(mem : void const? implicit; length : uint64; path : string implicit) : LLVMOpaqueLTOModule? {}
[extern(cdecl, late, name="lto_module_create_in_local_context", library="LLVM.dll")]
def lto_module_create_in_local_context(mem : void const? implicit; length : uint64; path : string implicit) : LLVMOpaqueLTOModule? {}
[extern(cdecl, late, name="lto_module_create_in_codegen_context", library="LLVM.dll")]
def lto_module_create_in_codegen_context(mem : void const? implicit; length : uint64; path : string implicit; cg : LLVMOpaqueLTOCodeGenerator? implicit) : LLVMOpaqueLTOModule? {}
[extern(cdecl, late, name="lto_module_create_from_fd", library="LLVM.dll")]
def lto_module_create_from_fd(fd : int; path : string implicit; file_size : uint64) : LLVMOpaqueLTOModule? {}
[extern(cdecl, late, name="lto_module_create_from_fd_at_offset", library="LLVM.dll")]
def lto_module_create_from_fd_at_offset(fd : int; path : string implicit; file_size : uint64; map_size : uint64; offset : int64) : LLVMOpaqueLTOModule? {}
[extern(cdecl, late, name="lto_module_dispose", library="LLVM.dll")]
def lto_module_dispose(mod : LLVMOpaqueLTOModule? implicit) : void {}
[extern(cdecl, late, name="lto_module_get_target_triple", library="LLVM.dll")]
def lto_module_get_target_triple(mod : LLVMOpaqueLTOModule? implicit) : string {}
[extern(cdecl, late, name="lto_module_set_target_triple", library="LLVM.dll")]
def lto_module_set_target_triple(mod : LLVMOpaqueLTOModule? implicit; triple : string implicit) : void {}
[extern(cdecl, late, name="lto_module_get_num_symbols", library="LLVM.dll")]
def lto_module_get_num_symbols(mod : LLVMOpaqueLTOModule? implicit) : uint {}
[extern(cdecl, late, name="lto_module_get_symbol_name", library="LLVM.dll")]
def lto_module_get_symbol_name(mod : LLVMOpaqueLTOModule? implicit; index : uint) : string {}
[extern(cdecl, late, name="lto_module_get_symbol_attribute", library="LLVM.dll")]
def lto_module_get_symbol_attribute(mod : LLVMOpaqueLTOModule? implicit; index : uint) : lto_symbol_attributes {}
[extern(cdecl, late, name="lto_module_get_linkeropts", library="LLVM.dll")]
def lto_module_get_linkeropts(mod : LLVMOpaqueLTOModule? implicit) : string {}
[extern(cdecl, late, name="lto_module_get_macho_cputype", library="LLVM.dll")]
def lto_module_get_macho_cputype(mod : LLVMOpaqueLTOModule? implicit; out_cputype : uint? implicit; out_cpusubtype : uint? implicit) : bool {}
[extern(cdecl, late, name="lto_module_has_ctor_dtor", library="LLVM.dll")]
def lto_module_has_ctor_dtor(mod : LLVMOpaqueLTOModule? implicit) : bool {}
[extern(cdecl, late, name="lto_codegen_create", library="LLVM.dll")]
def lto_codegen_create() : LLVMOpaqueLTOCodeGenerator? {}
[extern(cdecl, late, name="lto_codegen_create_in_local_context", library="LLVM.dll")]
def lto_codegen_create_in_local_context() : LLVMOpaqueLTOCodeGenerator? {}
[extern(cdecl, late, name="lto_codegen_dispose", library="LLVM.dll")]
def lto_codegen_dispose(_argument_0x0 : LLVMOpaqueLTOCodeGenerator? implicit) : void {}
[extern(cdecl, late, name="lto_codegen_add_module", library="LLVM.dll")]
def lto_codegen_add_module(cg : LLVMOpaqueLTOCodeGenerator? implicit; mod : LLVMOpaqueLTOModule? implicit) : bool {}
[extern(cdecl, late, name="lto_codegen_set_module", library="LLVM.dll")]
def lto_codegen_set_module(cg : LLVMOpaqueLTOCodeGenerator? implicit; mod : LLVMOpaqueLTOModule? implicit) : void {}
[extern(cdecl, late, name="lto_codegen_set_debug_model", library="LLVM.dll")]
def lto_codegen_set_debug_model(cg : LLVMOpaqueLTOCodeGenerator? implicit; _argument_0x1 : lto_debug_model) : bool {}
[extern(cdecl, late, name="lto_codegen_set_pic_model", library="LLVM.dll")]
def lto_codegen_set_pic_model(cg : LLVMOpaqueLTOCodeGenerator? implicit; _argument_0x1 : lto_codegen_model) : bool {}
[extern(cdecl, late, name="lto_codegen_set_cpu", library="LLVM.dll")]
def lto_codegen_set_cpu(cg : LLVMOpaqueLTOCodeGenerator? implicit; cpu : string implicit) : void {}
[extern(cdecl, late, name="lto_codegen_set_assembler_path", library="LLVM.dll")]
def lto_codegen_set_assembler_path(cg : LLVMOpaqueLTOCodeGenerator? implicit; path : string implicit) : void {}
[extern(cdecl, late, name="lto_codegen_set_assembler_args", library="LLVM.dll")]
def lto_codegen_set_assembler_args(cg : LLVMOpaqueLTOCodeGenerator? implicit; args : string? implicit; nargs : int) : void {}
[extern(cdecl, late, name="lto_codegen_add_must_preserve_symbol", library="LLVM.dll")]
def lto_codegen_add_must_preserve_symbol(cg : LLVMOpaqueLTOCodeGenerator? implicit; symbol : string implicit) : void {}
[extern(cdecl, late, name="lto_codegen_write_merged_modules", library="LLVM.dll")]
def lto_codegen_write_merged_modules(cg : LLVMOpaqueLTOCodeGenerator? implicit; path : string implicit) : bool {}
[extern(cdecl, late, name="lto_codegen_compile", library="LLVM.dll")]
def lto_codegen_compile(cg : LLVMOpaqueLTOCodeGenerator? implicit; length : uint64? implicit) : void const? {}
[extern(cdecl, late, name="lto_codegen_compile_to_file", library="LLVM.dll")]
def lto_codegen_compile_to_file(cg : LLVMOpaqueLTOCodeGenerator? implicit; name : string? implicit) : bool {}
[extern(cdecl, late, name="lto_codegen_optimize", library="LLVM.dll")]
def lto_codegen_optimize(cg : LLVMOpaqueLTOCodeGenerator? implicit) : bool {}
[extern(cdecl, late, name="lto_codegen_compile_optimized", library="LLVM.dll")]
def lto_codegen_compile_optimized(cg : LLVMOpaqueLTOCodeGenerator? implicit; length : uint64? implicit) : void const? {}
[extern(cdecl, late, name="lto_api_version", library="LLVM.dll")]
def lto_api_version() : uint {}
[extern(cdecl, late, name="lto_set_debug_options", library="LLVM.dll")]
def lto_set_debug_options(_options : string const? implicit; number : int) : void {}
[extern(cdecl, late, name="lto_codegen_debug_options", library="LLVM.dll")]
def lto_codegen_debug_options(cg : LLVMOpaqueLTOCodeGenerator? implicit; _argument_0x1 : string implicit) : void {}
[extern(cdecl, late, name="lto_codegen_debug_options_array", library="LLVM.dll")]
def lto_codegen_debug_options_array(cg : LLVMOpaqueLTOCodeGenerator? implicit; _argument_0x1 : string const? implicit; number : int) : void {}
[extern(cdecl, late, name="lto_initialize_disassembler", library="LLVM.dll")]
def lto_initialize_disassembler() : void {}
[extern(cdecl, late, name="lto_codegen_set_should_internalize", library="LLVM.dll")]
def lto_codegen_set_should_internalize(cg : LLVMOpaqueLTOCodeGenerator? implicit; ShouldInternalize : bool) : void {}
[extern(cdecl, late, name="lto_codegen_set_should_embed_uselists", library="LLVM.dll")]
def lto_codegen_set_should_embed_uselists(cg : LLVMOpaqueLTOCodeGenerator? implicit; ShouldEmbedUselists : bool) : void {}
[extern(cdecl, late, name="lto_input_create", library="LLVM.dll")]
def lto_input_create(buffer : void const? implicit; buffer_size : uint64; path : string implicit) : LLVMOpaqueLTOInput? {}
[extern(cdecl, late, name="lto_input_dispose", library="LLVM.dll")]
def lto_input_dispose(input : LLVMOpaqueLTOInput? implicit) : void {}
[extern(cdecl, late, name="lto_input_get_num_dependent_libraries", library="LLVM.dll")]
def lto_input_get_num_dependent_libraries(input : LLVMOpaqueLTOInput? implicit) : uint {}
[extern(cdecl, late, name="lto_input_get_dependent_library", library="LLVM.dll")]
def lto_input_get_dependent_library(input : LLVMOpaqueLTOInput? implicit; index : uint64; size : uint64? implicit) : string {}
[extern(cdecl, late, name="lto_runtime_lib_symbols_list", library="LLVM.dll")]
def lto_runtime_lib_symbols_list(size : uint64? implicit) : string const? {}
[extern(cdecl, late, name="thinlto_create_codegen", library="LLVM.dll")]
def thinlto_create_codegen() : LLVMOpaqueThinLTOCodeGenerator? {}
[extern(cdecl, late, name="thinlto_codegen_dispose", library="LLVM.dll")]
def thinlto_codegen_dispose(cg : LLVMOpaqueThinLTOCodeGenerator? implicit) : void {}
[extern(cdecl, late, name="thinlto_codegen_add_module", library="LLVM.dll")]
def thinlto_codegen_add_module(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; identifier : string implicit; data : string implicit; length : int) : void {}
[extern(cdecl, late, name="thinlto_codegen_process", library="LLVM.dll")]
def thinlto_codegen_process(cg : LLVMOpaqueThinLTOCodeGenerator? implicit) : void {}
[extern(cdecl, late, name="thinlto_module_get_num_objects", library="LLVM.dll")]
def thinlto_module_get_num_objects(cg : LLVMOpaqueThinLTOCodeGenerator? implicit) : uint {}
[extern(cdecl, late, name="thinlto_module_get_object", library="LLVM.dll")]
def thinlto_module_get_object(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; index : uint) : LTOObjectBuffer {}
[extern(cdecl, late, name="thinlto_module_get_num_object_files", library="LLVM.dll")]
def thinlto_module_get_num_object_files(cg : LLVMOpaqueThinLTOCodeGenerator? implicit) : uint {}
[extern(cdecl, late, name="thinlto_module_get_object_file", library="LLVM.dll")]
def thinlto_module_get_object_file(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; index : uint) : string {}
[extern(cdecl, late, name="thinlto_codegen_set_pic_model", library="LLVM.dll")]
def thinlto_codegen_set_pic_model(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; _argument_0x1 : lto_codegen_model) : bool {}
[extern(cdecl, late, name="thinlto_codegen_set_savetemps_dir", library="LLVM.dll")]
def thinlto_codegen_set_savetemps_dir(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; save_temps_dir : string implicit) : void {}
[extern(cdecl, late, name="thinlto_set_generated_objects_dir", library="LLVM.dll")]
def thinlto_set_generated_objects_dir(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; save_temps_dir : string implicit) : void {}
[extern(cdecl, late, name="thinlto_codegen_set_cpu", library="LLVM.dll")]
def thinlto_codegen_set_cpu(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; cpu : string implicit) : void {}
[extern(cdecl, late, name="thinlto_codegen_disable_codegen", library="LLVM.dll")]
def thinlto_codegen_disable_codegen(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; disable : bool) : void {}
[extern(cdecl, late, name="thinlto_codegen_set_codegen_only", library="LLVM.dll")]
def thinlto_codegen_set_codegen_only(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; codegen_only : bool) : void {}
[extern(cdecl, late, name="thinlto_debug_options", library="LLVM.dll")]
def thinlto_debug_options(_options : string const? implicit; number : int) : void {}
[extern(cdecl, late, name="lto_module_is_thinlto", library="LLVM.dll")]
def lto_module_is_thinlto(mod : LLVMOpaqueLTOModule? implicit) : bool {}
[extern(cdecl, late, name="thinlto_codegen_add_must_preserve_symbol", library="LLVM.dll")]
def thinlto_codegen_add_must_preserve_symbol(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; name : string implicit; length : int) : void {}
[extern(cdecl, late, name="thinlto_codegen_add_cross_referenced_symbol", library="LLVM.dll")]
def thinlto_codegen_add_cross_referenced_symbol(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; name : string implicit; length : int) : void {}
[extern(cdecl, late, name="thinlto_codegen_set_cache_dir", library="LLVM.dll")]
def thinlto_codegen_set_cache_dir(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; cache_dir : string implicit) : void {}
[extern(cdecl, late, name="thinlto_codegen_set_cache_pruning_interval", library="LLVM.dll")]
def thinlto_codegen_set_cache_pruning_interval(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; interval : int) : void {}
[extern(cdecl, late, name="thinlto_codegen_set_final_cache_size_relative_to_available_space", library="LLVM.dll")]
def thinlto_codegen_set_final_cache_size_relative_to_available_space(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; percentage : uint) : void {}
[extern(cdecl, late, name="thinlto_codegen_set_cache_entry_expiration", library="LLVM.dll")]
def thinlto_codegen_set_cache_entry_expiration(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; expiration : uint) : void {}
[extern(cdecl, late, name="thinlto_codegen_set_cache_size_bytes", library="LLVM.dll")]
def thinlto_codegen_set_cache_size_bytes(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; max_size_bytes : uint) : void {}
[extern(cdecl, late, name="thinlto_codegen_set_cache_size_megabytes", library="LLVM.dll")]
def thinlto_codegen_set_cache_size_megabytes(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; max_size_megabytes : uint) : void {}
[extern(cdecl, late, name="thinlto_codegen_set_cache_size_files", library="LLVM.dll")]
def thinlto_codegen_set_cache_size_files(cg : LLVMOpaqueThinLTOCodeGenerator? implicit; max_size_files : uint) : void {}
[extern(cdecl, name="LLVMCreateBinary", library="LLVM.dll")]
def LLVMCreateBinary(MemBuf : LLVMOpaqueMemoryBuffer? implicit; Context : LLVMOpaqueContext? implicit; ErrorMessage : string? implicit) : LLVMOpaqueBinary? {}
[extern(cdecl, name="LLVMDisposeBinary", library="LLVM.dll")]
def LLVMDisposeBinary(BR : LLVMOpaqueBinary? implicit) : void {}
[extern(cdecl, name="LLVMBinaryCopyMemoryBuffer", library="LLVM.dll")]
def LLVMBinaryCopyMemoryBuffer(BR : LLVMOpaqueBinary? implicit) : LLVMOpaqueMemoryBuffer? {}
[extern(cdecl, name="LLVMBinaryGetType", library="LLVM.dll")]
def LLVMBinaryGetType(BR : LLVMOpaqueBinary? implicit) : LLVMBinaryType {}
[extern(cdecl, name="LLVMMachOUniversalBinaryCopyObjectForArch", library="LLVM.dll")]
def LLVMMachOUniversalBinaryCopyObjectForArch(BR : LLVMOpaqueBinary? implicit; Arch : string implicit; ArchLen : uint64; ErrorMessage : string? implicit) : LLVMOpaqueBinary? {}
[extern(cdecl, name="LLVMObjectFileCopySectionIterator", library="LLVM.dll")]
def LLVMObjectFileCopySectionIterator(BR : LLVMOpaqueBinary? implicit) : LLVMOpaqueSectionIterator? {}
[extern(cdecl, name="LLVMObjectFileIsSectionIteratorAtEnd", library="LLVM.dll")]
def LLVMObjectFileIsSectionIteratorAtEnd(BR : LLVMOpaqueBinary? implicit; SI : LLVMOpaqueSectionIterator? implicit) : int {}
[extern(cdecl, name="LLVMObjectFileCopySymbolIterator", library="LLVM.dll")]
def LLVMObjectFileCopySymbolIterator(BR : LLVMOpaqueBinary? implicit) : LLVMOpaqueSymbolIterator? {}
[extern(cdecl, name="LLVMObjectFileIsSymbolIteratorAtEnd", library="LLVM.dll")]
def LLVMObjectFileIsSymbolIteratorAtEnd(BR : LLVMOpaqueBinary? implicit; SI : LLVMOpaqueSymbolIterator? implicit) : int {}
[extern(cdecl, name="LLVMDisposeSectionIterator", library="LLVM.dll")]
def LLVMDisposeSectionIterator(SI : LLVMOpaqueSectionIterator? implicit) : void {}
[extern(cdecl, name="LLVMMoveToNextSection", library="LLVM.dll")]
def LLVMMoveToNextSection(SI : LLVMOpaqueSectionIterator? implicit) : void {}
[extern(cdecl, name="LLVMMoveToContainingSection", library="LLVM.dll")]
def LLVMMoveToContainingSection(Sect : LLVMOpaqueSectionIterator? implicit; Sym : LLVMOpaqueSymbolIterator? implicit) : void {}
[extern(cdecl, name="LLVMDisposeSymbolIterator", library="LLVM.dll")]
def LLVMDisposeSymbolIterator(SI : LLVMOpaqueSymbolIterator? implicit) : void {}
[extern(cdecl, name="LLVMMoveToNextSymbol", library="LLVM.dll")]
def LLVMMoveToNextSymbol(SI : LLVMOpaqueSymbolIterator? implicit) : void {}
[extern(cdecl, name="LLVMGetSectionName", library="LLVM.dll")]
def LLVMGetSectionName(SI : LLVMOpaqueSectionIterator? implicit) : string {}
[extern(cdecl, name="LLVMGetSectionSize", library="LLVM.dll")]
def LLVMGetSectionSize(SI : LLVMOpaqueSectionIterator? implicit) : uint64 {}
[extern(cdecl, name="LLVMGetSectionContents", library="LLVM.dll")]
def LLVMGetSectionContents(SI : LLVMOpaqueSectionIterator? implicit) : string {}
[extern(cdecl, name="LLVMGetSectionAddress", library="LLVM.dll")]
def LLVMGetSectionAddress(SI : LLVMOpaqueSectionIterator? implicit) : uint64 {}
[extern(cdecl, name="LLVMGetSectionContainsSymbol", library="LLVM.dll")]
def LLVMGetSectionContainsSymbol(SI : LLVMOpaqueSectionIterator? implicit; Sym : LLVMOpaqueSymbolIterator? implicit) : int {}
[extern(cdecl, name="LLVMGetRelocations", library="LLVM.dll")]
def LLVMGetRelocations(Section : LLVMOpaqueSectionIterator? implicit) : LLVMOpaqueRelocationIterator? {}
[extern(cdecl, name="LLVMDisposeRelocationIterator", library="LLVM.dll")]
def LLVMDisposeRelocationIterator(RI : LLVMOpaqueRelocationIterator? implicit) : void {}
[extern(cdecl, name="LLVMIsRelocationIteratorAtEnd", library="LLVM.dll")]
def LLVMIsRelocationIteratorAtEnd(Section : LLVMOpaqueSectionIterator? implicit; RI : LLVMOpaqueRelocationIterator? implicit) : int {}
[extern(cdecl, name="LLVMMoveToNextRelocation", library="LLVM.dll")]
def LLVMMoveToNextRelocation(RI : LLVMOpaqueRelocationIterator? implicit) : void {}
[extern(cdecl, name="LLVMGetSymbolName", library="LLVM.dll")]
def LLVMGetSymbolName(SI : LLVMOpaqueSymbolIterator? implicit) : string {}
[extern(cdecl, name="LLVMGetSymbolAddress", library="LLVM.dll")]
def LLVMGetSymbolAddress(SI : LLVMOpaqueSymbolIterator? implicit) : uint64 {}
[extern(cdecl, name="LLVMGetSymbolSize", library="LLVM.dll")]
def LLVMGetSymbolSize(SI : LLVMOpaqueSymbolIterator? implicit) : uint64 {}
[extern(cdecl, name="LLVMGetRelocationOffset", library="LLVM.dll")]
def LLVMGetRelocationOffset(RI : LLVMOpaqueRelocationIterator? implicit) : uint64 {}
[extern(cdecl, name="LLVMGetRelocationSymbol", library="LLVM.dll")]
def LLVMGetRelocationSymbol(RI : LLVMOpaqueRelocationIterator? implicit) : LLVMOpaqueSymbolIterator? {}
[extern(cdecl, name="LLVMGetRelocationType", library="LLVM.dll")]
def LLVMGetRelocationType(RI : LLVMOpaqueRelocationIterator? implicit) : uint64 {}
[extern(cdecl, name="LLVMGetRelocationTypeName", library="LLVM.dll")]
def LLVMGetRelocationTypeName(RI : LLVMOpaqueRelocationIterator? implicit) : string {}
[extern(cdecl, name="LLVMGetRelocationValueString", library="LLVM.dll")]
def LLVMGetRelocationValueString(RI : LLVMOpaqueRelocationIterator? implicit) : string {}
[extern(cdecl, name="LLVMCreateObjectFile", library="LLVM.dll")]
def LLVMCreateObjectFile(MemBuf : LLVMOpaqueMemoryBuffer? implicit) : LLVMOpaqueObjectFile? {}
[extern(cdecl, name="LLVMDisposeObjectFile", library="LLVM.dll")]
def LLVMDisposeObjectFile(ObjectFile : LLVMOpaqueObjectFile? implicit) : void {}
[extern(cdecl, name="LLVMGetSections", library="LLVM.dll")]
def LLVMGetSections(ObjectFile : LLVMOpaqueObjectFile? implicit) : LLVMOpaqueSectionIterator? {}
[extern(cdecl, name="LLVMIsSectionIteratorAtEnd", library="LLVM.dll")]
def LLVMIsSectionIteratorAtEnd(ObjectFile : LLVMOpaqueObjectFile? implicit; SI : LLVMOpaqueSectionIterator? implicit) : int {}
[extern(cdecl, name="LLVMGetSymbols", library="LLVM.dll")]
def LLVMGetSymbols(ObjectFile : LLVMOpaqueObjectFile? implicit) : LLVMOpaqueSymbolIterator? {}
[extern(cdecl, name="LLVMIsSymbolIteratorAtEnd", library="LLVM.dll")]
def LLVMIsSymbolIteratorAtEnd(ObjectFile : LLVMOpaqueObjectFile? implicit; SI : LLVMOpaqueSymbolIterator? implicit) : int {}
[extern(cdecl, name="LLVMOrcCreateRTDyldObjectLinkingLayerWithSectionMemoryManager", library="LLVM.dll")]
def LLVMOrcCreateRTDyldObjectLinkingLayerWithSectionMemoryManager(ES : LLVMOrcOpaqueExecutionSession? implicit) : LLVMOrcOpaqueObjectLayer? {}
[extern(cdecl, name="LLVMOrcRTDyldObjectLinkingLayerRegisterJITEventListener", library="LLVM.dll")]
def LLVMOrcRTDyldObjectLinkingLayerRegisterJITEventListener(RTDyldObjLinkingLayer : LLVMOrcOpaqueObjectLayer? implicit; Listener : LLVMOpaqueJITEventListener? implicit) : void {}
[extern(cdecl, late, name="LLVMRemarkStringGetData", library="LLVM.dll")]
def LLVMRemarkStringGetData(String : LLVMRemarkOpaqueString? implicit) : string {}
[extern(cdecl, late, name="LLVMRemarkStringGetLen", library="LLVM.dll")]
def LLVMRemarkStringGetLen(String : LLVMRemarkOpaqueString? implicit) : uint {}
[extern(cdecl, late, name="LLVMRemarkDebugLocGetSourceFilePath", library="LLVM.dll")]
def LLVMRemarkDebugLocGetSourceFilePath(DL : LLVMRemarkOpaqueDebugLoc? implicit) : LLVMRemarkOpaqueString? {}
[extern(cdecl, late, name="LLVMRemarkDebugLocGetSourceLine", library="LLVM.dll")]
def LLVMRemarkDebugLocGetSourceLine(DL : LLVMRemarkOpaqueDebugLoc? implicit) : uint {}
[extern(cdecl, late, name="LLVMRemarkDebugLocGetSourceColumn", library="LLVM.dll")]
def LLVMRemarkDebugLocGetSourceColumn(DL : LLVMRemarkOpaqueDebugLoc? implicit) : uint {}
[extern(cdecl, late, name="LLVMRemarkArgGetKey", library="LLVM.dll")]
def LLVMRemarkArgGetKey(Arg : LLVMRemarkOpaqueArg? implicit) : LLVMRemarkOpaqueString? {}
[extern(cdecl, late, name="LLVMRemarkArgGetValue", library="LLVM.dll")]
def LLVMRemarkArgGetValue(Arg : LLVMRemarkOpaqueArg? implicit) : LLVMRemarkOpaqueString? {}
[extern(cdecl, late, name="LLVMRemarkArgGetDebugLoc", library="LLVM.dll")]
def LLVMRemarkArgGetDebugLoc(Arg : LLVMRemarkOpaqueArg? implicit) : LLVMRemarkOpaqueDebugLoc? {}
[extern(cdecl, late, name="LLVMRemarkEntryDispose", library="LLVM.dll")]
def LLVMRemarkEntryDispose(Remark : LLVMRemarkOpaqueEntry? implicit) : void {}
[extern(cdecl, late, name="LLVMRemarkEntryGetType", library="LLVM.dll")]
def LLVMRemarkEntryGetType(Remark : LLVMRemarkOpaqueEntry? implicit) : LLVMRemarkType {}
[extern(cdecl, late, name="LLVMRemarkEntryGetPassName", library="LLVM.dll")]
def LLVMRemarkEntryGetPassName(Remark : LLVMRemarkOpaqueEntry? implicit) : LLVMRemarkOpaqueString? {}
[extern(cdecl, late, name="LLVMRemarkEntryGetRemarkName", library="LLVM.dll")]
def LLVMRemarkEntryGetRemarkName(Remark : LLVMRemarkOpaqueEntry? implicit) : LLVMRemarkOpaqueString? {}
[extern(cdecl, late, name="LLVMRemarkEntryGetFunctionName", library="LLVM.dll")]
def LLVMRemarkEntryGetFunctionName(Remark : LLVMRemarkOpaqueEntry? implicit) : LLVMRemarkOpaqueString? {}
[extern(cdecl, late, name="LLVMRemarkEntryGetDebugLoc", library="LLVM.dll")]
def LLVMRemarkEntryGetDebugLoc(Remark : LLVMRemarkOpaqueEntry? implicit) : LLVMRemarkOpaqueDebugLoc? {}
[extern(cdecl, late, name="LLVMRemarkEntryGetHotness", library="LLVM.dll")]
def LLVMRemarkEntryGetHotness(Remark : LLVMRemarkOpaqueEntry? implicit) : uint64 {}
[extern(cdecl, late, name="LLVMRemarkEntryGetNumArgs", library="LLVM.dll")]
def LLVMRemarkEntryGetNumArgs(Remark : LLVMRemarkOpaqueEntry? implicit) : uint {}
[extern(cdecl, late, name="LLVMRemarkEntryGetFirstArg", library="LLVM.dll")]
def LLVMRemarkEntryGetFirstArg(Remark : LLVMRemarkOpaqueEntry? implicit) : LLVMRemarkOpaqueArg? {}
[extern(cdecl, late, name="LLVMRemarkEntryGetNextArg", library="LLVM.dll")]
def LLVMRemarkEntryGetNextArg(It : LLVMRemarkOpaqueArg? implicit; Remark : LLVMRemarkOpaqueEntry? implicit) : LLVMRemarkOpaqueArg? {}
[extern(cdecl, late, name="LLVMRemarkParserCreateYAML", library="LLVM.dll")]
def LLVMRemarkParserCreateYAML(Buf : void const? implicit; Size : uint64) : LLVMRemarkOpaqueParser? {}
[extern(cdecl, late, name="LLVMRemarkParserCreateBitstream", library="LLVM.dll")]
def LLVMRemarkParserCreateBitstream(Buf : void const? implicit; Size : uint64) : LLVMRemarkOpaqueParser? {}
[extern(cdecl, late, name="LLVMRemarkParserGetNext", library="LLVM.dll")]
def LLVMRemarkParserGetNext(Parser : LLVMRemarkOpaqueParser? implicit) : LLVMRemarkOpaqueEntry? {}
[extern(cdecl, late, name="LLVMRemarkParserHasError", library="LLVM.dll")]
def LLVMRemarkParserHasError(Parser : LLVMRemarkOpaqueParser? implicit) : int {}
[extern(cdecl, late, name="LLVMRemarkParserGetErrorMessage", library="LLVM.dll")]
def LLVMRemarkParserGetErrorMessage(Parser : LLVMRemarkOpaqueParser? implicit) : string {}
[extern(cdecl, late, name="LLVMRemarkParserDispose", library="LLVM.dll")]
def LLVMRemarkParserDispose(Parser : LLVMRemarkOpaqueParser? implicit) : void {}
[extern(cdecl, late, name="LLVMRemarkVersion", library="LLVM.dll")]
def LLVMRemarkVersion() : uint {}
[extern(cdecl, name="LLVMLoadLibraryPermanently", library="LLVM.dll")]
def LLVMLoadLibraryPermanently(Filename : string implicit) : int {}
[extern(cdecl, name="LLVMParseCommandLineOptions", library="LLVM.dll")]
def LLVMParseCommandLineOptions(argc : int; argv : string const? implicit; Overview : string implicit) : void {}
[extern(cdecl, name="LLVMSearchForAddressOfSymbol", library="LLVM.dll")]
def LLVMSearchForAddressOfSymbol(symbolName : string implicit) : void? {}
[extern(cdecl, name="LLVMAddSymbol", library="LLVM.dll")]
def LLVMAddSymbol(symbolName : string implicit; symbolValue : void? implicit) : void {}
[extern(cdecl, name="LLVMAddConstantMergePass", library="LLVM.dll")]
def LLVMAddConstantMergePass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddMergeFunctionsPass", library="LLVM.dll")]
def LLVMAddMergeFunctionsPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddCalledValuePropagationPass", library="LLVM.dll")]
def LLVMAddCalledValuePropagationPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddDeadArgEliminationPass", library="LLVM.dll")]
def LLVMAddDeadArgEliminationPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddFunctionAttrsPass", library="LLVM.dll")]
def LLVMAddFunctionAttrsPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddFunctionInliningPass", library="LLVM.dll")]
def LLVMAddFunctionInliningPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddAlwaysInlinerPass", library="LLVM.dll")]
def LLVMAddAlwaysInlinerPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddGlobalDCEPass", library="LLVM.dll")]
def LLVMAddGlobalDCEPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddGlobalOptimizerPass", library="LLVM.dll")]
def LLVMAddGlobalOptimizerPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddIPSCCPPass", library="LLVM.dll")]
def LLVMAddIPSCCPPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddInternalizePass", library="LLVM.dll")]
def LLVMAddInternalizePass(_argument_0x0 : LLVMOpaquePassManager? implicit; AllButMain : uint) : void {}
[extern(cdecl, name="LLVMAddStripDeadPrototypesPass", library="LLVM.dll")]
def LLVMAddStripDeadPrototypesPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddStripSymbolsPass", library="LLVM.dll")]
def LLVMAddStripSymbolsPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMRunPasses", library="LLVM.dll")]
def LLVMRunPasses(M : LLVMOpaqueModule? implicit; Passes : string implicit; TM : LLVMOpaqueTargetMachine? implicit; Options : LLVMOpaquePassBuilderOptions? implicit) : LLVMOpaqueError? {}
[extern(cdecl, name="LLVMCreatePassBuilderOptions", library="LLVM.dll")]
def LLVMCreatePassBuilderOptions() : LLVMOpaquePassBuilderOptions? {}
[extern(cdecl, name="LLVMPassBuilderOptionsSetVerifyEach", library="LLVM.dll")]
def LLVMPassBuilderOptionsSetVerifyEach(Options : LLVMOpaquePassBuilderOptions? implicit; VerifyEach : int) : void {}
[extern(cdecl, name="LLVMPassBuilderOptionsSetDebugLogging", library="LLVM.dll")]
def LLVMPassBuilderOptionsSetDebugLogging(Options : LLVMOpaquePassBuilderOptions? implicit; DebugLogging : int) : void {}
[extern(cdecl, name="LLVMPassBuilderOptionsSetLoopInterleaving", library="LLVM.dll")]
def LLVMPassBuilderOptionsSetLoopInterleaving(Options : LLVMOpaquePassBuilderOptions? implicit; LoopInterleaving : int) : void {}
[extern(cdecl, name="LLVMPassBuilderOptionsSetLoopVectorization", library="LLVM.dll")]
def LLVMPassBuilderOptionsSetLoopVectorization(Options : LLVMOpaquePassBuilderOptions? implicit; LoopVectorization : int) : void {}
[extern(cdecl, name="LLVMPassBuilderOptionsSetSLPVectorization", library="LLVM.dll")]
def LLVMPassBuilderOptionsSetSLPVectorization(Options : LLVMOpaquePassBuilderOptions? implicit; SLPVectorization : int) : void {}
[extern(cdecl, name="LLVMPassBuilderOptionsSetLoopUnrolling", library="LLVM.dll")]
def LLVMPassBuilderOptionsSetLoopUnrolling(Options : LLVMOpaquePassBuilderOptions? implicit; LoopUnrolling : int) : void {}
[extern(cdecl, name="LLVMPassBuilderOptionsSetForgetAllSCEVInLoopUnroll", library="LLVM.dll")]
def LLVMPassBuilderOptionsSetForgetAllSCEVInLoopUnroll(Options : LLVMOpaquePassBuilderOptions? implicit; ForgetAllSCEVInLoopUnroll : int) : void {}
[extern(cdecl, name="LLVMPassBuilderOptionsSetLicmMssaOptCap", library="LLVM.dll")]
def LLVMPassBuilderOptionsSetLicmMssaOptCap(Options : LLVMOpaquePassBuilderOptions? implicit; LicmMssaOptCap : uint) : void {}
[extern(cdecl, name="LLVMPassBuilderOptionsSetLicmMssaNoAccForPromotionCap", library="LLVM.dll")]
def LLVMPassBuilderOptionsSetLicmMssaNoAccForPromotionCap(Options : LLVMOpaquePassBuilderOptions? implicit; LicmMssaNoAccForPromotionCap : uint) : void {}
[extern(cdecl, name="LLVMPassBuilderOptionsSetCallGraphProfile", library="LLVM.dll")]
def LLVMPassBuilderOptionsSetCallGraphProfile(Options : LLVMOpaquePassBuilderOptions? implicit; CallGraphProfile : int) : void {}
[extern(cdecl, name="LLVMPassBuilderOptionsSetMergeFunctions", library="LLVM.dll")]
def LLVMPassBuilderOptionsSetMergeFunctions(Options : LLVMOpaquePassBuilderOptions? implicit; MergeFunctions : int) : void {}
[extern(cdecl, name="LLVMDisposePassBuilderOptions", library="LLVM.dll")]
def LLVMDisposePassBuilderOptions(Options : LLVMOpaquePassBuilderOptions? implicit) : void {}
[extern(cdecl, name="LLVMPassManagerBuilderCreate", library="LLVM.dll")]
def LLVMPassManagerBuilderCreate() : LLVMOpaquePassManagerBuilder? {}
[extern(cdecl, name="LLVMPassManagerBuilderDispose", library="LLVM.dll")]
def LLVMPassManagerBuilderDispose(PMB : LLVMOpaquePassManagerBuilder? implicit) : void {}
[extern(cdecl, name="LLVMPassManagerBuilderSetOptLevel", library="LLVM.dll")]
def LLVMPassManagerBuilderSetOptLevel(PMB : LLVMOpaquePassManagerBuilder? implicit; OptLevel : uint) : void {}
[extern(cdecl, name="LLVMPassManagerBuilderSetSizeLevel", library="LLVM.dll")]
def LLVMPassManagerBuilderSetSizeLevel(PMB : LLVMOpaquePassManagerBuilder? implicit; SizeLevel : uint) : void {}
[extern(cdecl, name="LLVMPassManagerBuilderSetDisableUnitAtATime", library="LLVM.dll")]
def LLVMPassManagerBuilderSetDisableUnitAtATime(PMB : LLVMOpaquePassManagerBuilder? implicit; Value : int) : void {}
[extern(cdecl, name="LLVMPassManagerBuilderSetDisableUnrollLoops", library="LLVM.dll")]
def LLVMPassManagerBuilderSetDisableUnrollLoops(PMB : LLVMOpaquePassManagerBuilder? implicit; Value : int) : void {}
[extern(cdecl, name="LLVMPassManagerBuilderSetDisableSimplifyLibCalls", library="LLVM.dll")]
def LLVMPassManagerBuilderSetDisableSimplifyLibCalls(PMB : LLVMOpaquePassManagerBuilder? implicit; Value : int) : void {}
[extern(cdecl, name="LLVMPassManagerBuilderUseInlinerWithThreshold", library="LLVM.dll")]
def LLVMPassManagerBuilderUseInlinerWithThreshold(PMB : LLVMOpaquePassManagerBuilder? implicit; Threshold : uint) : void {}
[extern(cdecl, name="LLVMPassManagerBuilderPopulateFunctionPassManager", library="LLVM.dll")]
def LLVMPassManagerBuilderPopulateFunctionPassManager(PMB : LLVMOpaquePassManagerBuilder? implicit; PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMPassManagerBuilderPopulateModulePassManager", library="LLVM.dll")]
def LLVMPassManagerBuilderPopulateModulePassManager(PMB : LLVMOpaquePassManagerBuilder? implicit; PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddAggressiveDCEPass", library="LLVM.dll")]
def LLVMAddAggressiveDCEPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddDCEPass", library="LLVM.dll")]
def LLVMAddDCEPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddBitTrackingDCEPass", library="LLVM.dll")]
def LLVMAddBitTrackingDCEPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddAlignmentFromAssumptionsPass", library="LLVM.dll")]
def LLVMAddAlignmentFromAssumptionsPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddCFGSimplificationPass", library="LLVM.dll")]
def LLVMAddCFGSimplificationPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddDeadStoreEliminationPass", library="LLVM.dll")]
def LLVMAddDeadStoreEliminationPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddScalarizerPass", library="LLVM.dll")]
def LLVMAddScalarizerPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddMergedLoadStoreMotionPass", library="LLVM.dll")]
def LLVMAddMergedLoadStoreMotionPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddGVNPass", library="LLVM.dll")]
def LLVMAddGVNPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddNewGVNPass", library="LLVM.dll")]
def LLVMAddNewGVNPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddIndVarSimplifyPass", library="LLVM.dll")]
def LLVMAddIndVarSimplifyPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddInstructionCombiningPass", library="LLVM.dll")]
def LLVMAddInstructionCombiningPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddInstructionSimplifyPass", library="LLVM.dll")]
def LLVMAddInstructionSimplifyPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddJumpThreadingPass", library="LLVM.dll")]
def LLVMAddJumpThreadingPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddLICMPass", library="LLVM.dll")]
def LLVMAddLICMPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddLoopDeletionPass", library="LLVM.dll")]
def LLVMAddLoopDeletionPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddLoopIdiomPass", library="LLVM.dll")]
def LLVMAddLoopIdiomPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddLoopRotatePass", library="LLVM.dll")]
def LLVMAddLoopRotatePass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddLoopRerollPass", library="LLVM.dll")]
def LLVMAddLoopRerollPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddLoopUnrollPass", library="LLVM.dll")]
def LLVMAddLoopUnrollPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddLoopUnrollAndJamPass", library="LLVM.dll")]
def LLVMAddLoopUnrollAndJamPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddLowerAtomicPass", library="LLVM.dll")]
def LLVMAddLowerAtomicPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddMemCpyOptPass", library="LLVM.dll")]
def LLVMAddMemCpyOptPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddPartiallyInlineLibCallsPass", library="LLVM.dll")]
def LLVMAddPartiallyInlineLibCallsPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddReassociatePass", library="LLVM.dll")]
def LLVMAddReassociatePass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddSCCPPass", library="LLVM.dll")]
def LLVMAddSCCPPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddScalarReplAggregatesPass", library="LLVM.dll")]
def LLVMAddScalarReplAggregatesPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddScalarReplAggregatesPassSSA", library="LLVM.dll")]
def LLVMAddScalarReplAggregatesPassSSA(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddScalarReplAggregatesPassWithThreshold", library="LLVM.dll")]
def LLVMAddScalarReplAggregatesPassWithThreshold(PM : LLVMOpaquePassManager? implicit; Threshold : int) : void {}
[extern(cdecl, name="LLVMAddSimplifyLibCallsPass", library="LLVM.dll")]
def LLVMAddSimplifyLibCallsPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddTailCallEliminationPass", library="LLVM.dll")]
def LLVMAddTailCallEliminationPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddDemoteMemoryToRegisterPass", library="LLVM.dll")]
def LLVMAddDemoteMemoryToRegisterPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddVerifierPass", library="LLVM.dll")]
def LLVMAddVerifierPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddCorrelatedValuePropagationPass", library="LLVM.dll")]
def LLVMAddCorrelatedValuePropagationPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddEarlyCSEPass", library="LLVM.dll")]
def LLVMAddEarlyCSEPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddEarlyCSEMemSSAPass", library="LLVM.dll")]
def LLVMAddEarlyCSEMemSSAPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddLowerExpectIntrinsicPass", library="LLVM.dll")]
def LLVMAddLowerExpectIntrinsicPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddLowerConstantIntrinsicsPass", library="LLVM.dll")]
def LLVMAddLowerConstantIntrinsicsPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddTypeBasedAliasAnalysisPass", library="LLVM.dll")]
def LLVMAddTypeBasedAliasAnalysisPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddScopedNoAliasAAPass", library="LLVM.dll")]
def LLVMAddScopedNoAliasAAPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddBasicAliasAnalysisPass", library="LLVM.dll")]
def LLVMAddBasicAliasAnalysisPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddUnifyFunctionExitNodesPass", library="LLVM.dll")]
def LLVMAddUnifyFunctionExitNodesPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddLowerSwitchPass", library="LLVM.dll")]
def LLVMAddLowerSwitchPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddPromoteMemoryToRegisterPass", library="LLVM.dll")]
def LLVMAddPromoteMemoryToRegisterPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddAddDiscriminatorsPass", library="LLVM.dll")]
def LLVMAddAddDiscriminatorsPass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddLoopVectorizePass", library="LLVM.dll")]
def LLVMAddLoopVectorizePass(PM : LLVMOpaquePassManager? implicit) : void {}
[extern(cdecl, name="LLVMAddSLPVectorizePass", library="LLVM.dll")]
def LLVMAddSLPVectorizePass(PM : LLVMOpaquePassManager? implicit) : void {}
