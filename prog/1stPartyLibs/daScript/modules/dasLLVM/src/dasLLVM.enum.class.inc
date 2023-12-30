// this file is generated via daScript automatic C++ binder
// all user modifications will be lost after this file is re-generated

// from D:\Work\libclang\include\llvm-c/Analysis.h:34:9
class Enumeration_LLVMVerifierFailureAction : public das::Enumeration{
public:
	Enumeration_LLVMVerifierFailureAction() : das::Enumeration("LLVMVerifierFailureAction") {
		external = true;
		cppName = "LLVMVerifierFailureAction";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMAbortProcessAction", "LLVMAbortProcessAction", int64_t(LLVMVerifierFailureAction::LLVMAbortProcessAction), das::LineInfo());
		addIEx("LLVMPrintMessageAction", "LLVMPrintMessageAction", int64_t(LLVMVerifierFailureAction::LLVMPrintMessageAction), das::LineInfo());
		addIEx("LLVMReturnStatusAction", "LLVMReturnStatusAction", int64_t(LLVMVerifierFailureAction::LLVMReturnStatusAction), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Comdat.h:29:9
class Enumeration_LLVMComdatSelectionKind : public das::Enumeration{
public:
	Enumeration_LLVMComdatSelectionKind() : das::Enumeration("LLVMComdatSelectionKind") {
		external = true;
		cppName = "LLVMComdatSelectionKind";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMAnyComdatSelectionKind", "LLVMAnyComdatSelectionKind", int64_t(LLVMComdatSelectionKind::LLVMAnyComdatSelectionKind), das::LineInfo());
		addIEx("LLVMExactMatchComdatSelectionKind", "LLVMExactMatchComdatSelectionKind", int64_t(LLVMComdatSelectionKind::LLVMExactMatchComdatSelectionKind), das::LineInfo());
		addIEx("LLVMLargestComdatSelectionKind", "LLVMLargestComdatSelectionKind", int64_t(LLVMComdatSelectionKind::LLVMLargestComdatSelectionKind), das::LineInfo());
		addIEx("LLVMNoDeduplicateComdatSelectionKind", "LLVMNoDeduplicateComdatSelectionKind", int64_t(LLVMComdatSelectionKind::LLVMNoDeduplicateComdatSelectionKind), das::LineInfo());
		addIEx("LLVMSameSizeComdatSelectionKind", "LLVMSameSizeComdatSelectionKind", int64_t(LLVMComdatSelectionKind::LLVMSameSizeComdatSelectionKind), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:60:9
class Enumeration_LLVMOpcode : public das::Enumeration{
public:
	Enumeration_LLVMOpcode() : das::Enumeration("LLVMOpcode") {
		external = true;
		cppName = "LLVMOpcode";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMRet", "LLVMRet", int64_t(LLVMOpcode::LLVMRet), das::LineInfo());
		addIEx("LLVMBr", "LLVMBr", int64_t(LLVMOpcode::LLVMBr), das::LineInfo());
		addIEx("LLVMSwitch", "LLVMSwitch", int64_t(LLVMOpcode::LLVMSwitch), das::LineInfo());
		addIEx("LLVMIndirectBr", "LLVMIndirectBr", int64_t(LLVMOpcode::LLVMIndirectBr), das::LineInfo());
		addIEx("LLVMInvoke", "LLVMInvoke", int64_t(LLVMOpcode::LLVMInvoke), das::LineInfo());
		addIEx("LLVMUnreachable", "LLVMUnreachable", int64_t(LLVMOpcode::LLVMUnreachable), das::LineInfo());
		addIEx("LLVMCallBr", "LLVMCallBr", int64_t(LLVMOpcode::LLVMCallBr), das::LineInfo());
		addIEx("LLVMFNeg", "LLVMFNeg", int64_t(LLVMOpcode::LLVMFNeg), das::LineInfo());
		addIEx("LLVMAdd", "LLVMAdd", int64_t(LLVMOpcode::LLVMAdd), das::LineInfo());
		addIEx("LLVMFAdd", "LLVMFAdd", int64_t(LLVMOpcode::LLVMFAdd), das::LineInfo());
		addIEx("LLVMSub", "LLVMSub", int64_t(LLVMOpcode::LLVMSub), das::LineInfo());
		addIEx("LLVMFSub", "LLVMFSub", int64_t(LLVMOpcode::LLVMFSub), das::LineInfo());
		addIEx("LLVMMul", "LLVMMul", int64_t(LLVMOpcode::LLVMMul), das::LineInfo());
		addIEx("LLVMFMul", "LLVMFMul", int64_t(LLVMOpcode::LLVMFMul), das::LineInfo());
		addIEx("LLVMUDiv", "LLVMUDiv", int64_t(LLVMOpcode::LLVMUDiv), das::LineInfo());
		addIEx("LLVMSDiv", "LLVMSDiv", int64_t(LLVMOpcode::LLVMSDiv), das::LineInfo());
		addIEx("LLVMFDiv", "LLVMFDiv", int64_t(LLVMOpcode::LLVMFDiv), das::LineInfo());
		addIEx("LLVMURem", "LLVMURem", int64_t(LLVMOpcode::LLVMURem), das::LineInfo());
		addIEx("LLVMSRem", "LLVMSRem", int64_t(LLVMOpcode::LLVMSRem), das::LineInfo());
		addIEx("LLVMFRem", "LLVMFRem", int64_t(LLVMOpcode::LLVMFRem), das::LineInfo());
		addIEx("LLVMShl", "LLVMShl", int64_t(LLVMOpcode::LLVMShl), das::LineInfo());
		addIEx("LLVMLShr", "LLVMLShr", int64_t(LLVMOpcode::LLVMLShr), das::LineInfo());
		addIEx("LLVMAShr", "LLVMAShr", int64_t(LLVMOpcode::LLVMAShr), das::LineInfo());
		addIEx("LLVMAnd", "LLVMAnd", int64_t(LLVMOpcode::LLVMAnd), das::LineInfo());
		addIEx("LLVMOr", "LLVMOr", int64_t(LLVMOpcode::LLVMOr), das::LineInfo());
		addIEx("LLVMXor", "LLVMXor", int64_t(LLVMOpcode::LLVMXor), das::LineInfo());
		addIEx("LLVMAlloca", "LLVMAlloca", int64_t(LLVMOpcode::LLVMAlloca), das::LineInfo());
		addIEx("LLVMLoad", "LLVMLoad", int64_t(LLVMOpcode::LLVMLoad), das::LineInfo());
		addIEx("LLVMStore", "LLVMStore", int64_t(LLVMOpcode::LLVMStore), das::LineInfo());
		addIEx("LLVMGetElementPtr", "LLVMGetElementPtr", int64_t(LLVMOpcode::LLVMGetElementPtr), das::LineInfo());
		addIEx("LLVMTrunc", "LLVMTrunc", int64_t(LLVMOpcode::LLVMTrunc), das::LineInfo());
		addIEx("LLVMZExt", "LLVMZExt", int64_t(LLVMOpcode::LLVMZExt), das::LineInfo());
		addIEx("LLVMSExt", "LLVMSExt", int64_t(LLVMOpcode::LLVMSExt), das::LineInfo());
		addIEx("LLVMFPToUI", "LLVMFPToUI", int64_t(LLVMOpcode::LLVMFPToUI), das::LineInfo());
		addIEx("LLVMFPToSI", "LLVMFPToSI", int64_t(LLVMOpcode::LLVMFPToSI), das::LineInfo());
		addIEx("LLVMUIToFP", "LLVMUIToFP", int64_t(LLVMOpcode::LLVMUIToFP), das::LineInfo());
		addIEx("LLVMSIToFP", "LLVMSIToFP", int64_t(LLVMOpcode::LLVMSIToFP), das::LineInfo());
		addIEx("LLVMFPTrunc", "LLVMFPTrunc", int64_t(LLVMOpcode::LLVMFPTrunc), das::LineInfo());
		addIEx("LLVMFPExt", "LLVMFPExt", int64_t(LLVMOpcode::LLVMFPExt), das::LineInfo());
		addIEx("LLVMPtrToInt", "LLVMPtrToInt", int64_t(LLVMOpcode::LLVMPtrToInt), das::LineInfo());
		addIEx("LLVMIntToPtr", "LLVMIntToPtr", int64_t(LLVMOpcode::LLVMIntToPtr), das::LineInfo());
		addIEx("LLVMBitCast", "LLVMBitCast", int64_t(LLVMOpcode::LLVMBitCast), das::LineInfo());
		addIEx("LLVMAddrSpaceCast", "LLVMAddrSpaceCast", int64_t(LLVMOpcode::LLVMAddrSpaceCast), das::LineInfo());
		addIEx("LLVMICmp", "LLVMICmp", int64_t(LLVMOpcode::LLVMICmp), das::LineInfo());
		addIEx("LLVMFCmp", "LLVMFCmp", int64_t(LLVMOpcode::LLVMFCmp), das::LineInfo());
		addIEx("LLVMPHI", "LLVMPHI", int64_t(LLVMOpcode::LLVMPHI), das::LineInfo());
		addIEx("LLVMCall", "LLVMCall", int64_t(LLVMOpcode::LLVMCall), das::LineInfo());
		addIEx("LLVMSelect", "LLVMSelect", int64_t(LLVMOpcode::LLVMSelect), das::LineInfo());
		addIEx("LLVMUserOp1", "LLVMUserOp1", int64_t(LLVMOpcode::LLVMUserOp1), das::LineInfo());
		addIEx("LLVMUserOp2", "LLVMUserOp2", int64_t(LLVMOpcode::LLVMUserOp2), das::LineInfo());
		addIEx("LLVMVAArg", "LLVMVAArg", int64_t(LLVMOpcode::LLVMVAArg), das::LineInfo());
		addIEx("LLVMExtractElement", "LLVMExtractElement", int64_t(LLVMOpcode::LLVMExtractElement), das::LineInfo());
		addIEx("LLVMInsertElement", "LLVMInsertElement", int64_t(LLVMOpcode::LLVMInsertElement), das::LineInfo());
		addIEx("LLVMShuffleVector", "LLVMShuffleVector", int64_t(LLVMOpcode::LLVMShuffleVector), das::LineInfo());
		addIEx("LLVMExtractValue", "LLVMExtractValue", int64_t(LLVMOpcode::LLVMExtractValue), das::LineInfo());
		addIEx("LLVMInsertValue", "LLVMInsertValue", int64_t(LLVMOpcode::LLVMInsertValue), das::LineInfo());
		addIEx("LLVMFreeze", "LLVMFreeze", int64_t(LLVMOpcode::LLVMFreeze), das::LineInfo());
		addIEx("LLVMFence", "LLVMFence", int64_t(LLVMOpcode::LLVMFence), das::LineInfo());
		addIEx("LLVMAtomicCmpXchg", "LLVMAtomicCmpXchg", int64_t(LLVMOpcode::LLVMAtomicCmpXchg), das::LineInfo());
		addIEx("LLVMAtomicRMW", "LLVMAtomicRMW", int64_t(LLVMOpcode::LLVMAtomicRMW), das::LineInfo());
		addIEx("LLVMResume", "LLVMResume", int64_t(LLVMOpcode::LLVMResume), das::LineInfo());
		addIEx("LLVMLandingPad", "LLVMLandingPad", int64_t(LLVMOpcode::LLVMLandingPad), das::LineInfo());
		addIEx("LLVMCleanupRet", "LLVMCleanupRet", int64_t(LLVMOpcode::LLVMCleanupRet), das::LineInfo());
		addIEx("LLVMCatchRet", "LLVMCatchRet", int64_t(LLVMOpcode::LLVMCatchRet), das::LineInfo());
		addIEx("LLVMCatchPad", "LLVMCatchPad", int64_t(LLVMOpcode::LLVMCatchPad), das::LineInfo());
		addIEx("LLVMCleanupPad", "LLVMCleanupPad", int64_t(LLVMOpcode::LLVMCleanupPad), das::LineInfo());
		addIEx("LLVMCatchSwitch", "LLVMCatchSwitch", int64_t(LLVMOpcode::LLVMCatchSwitch), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:148:9
class Enumeration_LLVMTypeKind : public das::Enumeration{
public:
	Enumeration_LLVMTypeKind() : das::Enumeration("LLVMTypeKind") {
		external = true;
		cppName = "LLVMTypeKind";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMVoidTypeKind", "LLVMVoidTypeKind", int64_t(LLVMTypeKind::LLVMVoidTypeKind), das::LineInfo());
		addIEx("LLVMHalfTypeKind", "LLVMHalfTypeKind", int64_t(LLVMTypeKind::LLVMHalfTypeKind), das::LineInfo());
		addIEx("LLVMFloatTypeKind", "LLVMFloatTypeKind", int64_t(LLVMTypeKind::LLVMFloatTypeKind), das::LineInfo());
		addIEx("LLVMDoubleTypeKind", "LLVMDoubleTypeKind", int64_t(LLVMTypeKind::LLVMDoubleTypeKind), das::LineInfo());
		addIEx("LLVMX86_FP80TypeKind", "LLVMX86_FP80TypeKind", int64_t(LLVMTypeKind::LLVMX86_FP80TypeKind), das::LineInfo());
		addIEx("LLVMFP128TypeKind", "LLVMFP128TypeKind", int64_t(LLVMTypeKind::LLVMFP128TypeKind), das::LineInfo());
		addIEx("LLVMPPC_FP128TypeKind", "LLVMPPC_FP128TypeKind", int64_t(LLVMTypeKind::LLVMPPC_FP128TypeKind), das::LineInfo());
		addIEx("LLVMLabelTypeKind", "LLVMLabelTypeKind", int64_t(LLVMTypeKind::LLVMLabelTypeKind), das::LineInfo());
		addIEx("LLVMIntegerTypeKind", "LLVMIntegerTypeKind", int64_t(LLVMTypeKind::LLVMIntegerTypeKind), das::LineInfo());
		addIEx("LLVMFunctionTypeKind", "LLVMFunctionTypeKind", int64_t(LLVMTypeKind::LLVMFunctionTypeKind), das::LineInfo());
		addIEx("LLVMStructTypeKind", "LLVMStructTypeKind", int64_t(LLVMTypeKind::LLVMStructTypeKind), das::LineInfo());
		addIEx("LLVMArrayTypeKind", "LLVMArrayTypeKind", int64_t(LLVMTypeKind::LLVMArrayTypeKind), das::LineInfo());
		addIEx("LLVMPointerTypeKind", "LLVMPointerTypeKind", int64_t(LLVMTypeKind::LLVMPointerTypeKind), das::LineInfo());
		addIEx("LLVMVectorTypeKind", "LLVMVectorTypeKind", int64_t(LLVMTypeKind::LLVMVectorTypeKind), das::LineInfo());
		addIEx("LLVMMetadataTypeKind", "LLVMMetadataTypeKind", int64_t(LLVMTypeKind::LLVMMetadataTypeKind), das::LineInfo());
		addIEx("LLVMX86_MMXTypeKind", "LLVMX86_MMXTypeKind", int64_t(LLVMTypeKind::LLVMX86_MMXTypeKind), das::LineInfo());
		addIEx("LLVMTokenTypeKind", "LLVMTokenTypeKind", int64_t(LLVMTypeKind::LLVMTokenTypeKind), das::LineInfo());
		addIEx("LLVMScalableVectorTypeKind", "LLVMScalableVectorTypeKind", int64_t(LLVMTypeKind::LLVMScalableVectorTypeKind), das::LineInfo());
		addIEx("LLVMBFloatTypeKind", "LLVMBFloatTypeKind", int64_t(LLVMTypeKind::LLVMBFloatTypeKind), das::LineInfo());
		addIEx("LLVMX86_AMXTypeKind", "LLVMX86_AMXTypeKind", int64_t(LLVMTypeKind::LLVMX86_AMXTypeKind), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:171:9
class Enumeration_LLVMLinkage : public das::Enumeration{
public:
	Enumeration_LLVMLinkage() : das::Enumeration("LLVMLinkage") {
		external = true;
		cppName = "LLVMLinkage";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMExternalLinkage", "LLVMExternalLinkage", int64_t(LLVMLinkage::LLVMExternalLinkage), das::LineInfo());
		addIEx("LLVMAvailableExternallyLinkage", "LLVMAvailableExternallyLinkage", int64_t(LLVMLinkage::LLVMAvailableExternallyLinkage), das::LineInfo());
		addIEx("LLVMLinkOnceAnyLinkage", "LLVMLinkOnceAnyLinkage", int64_t(LLVMLinkage::LLVMLinkOnceAnyLinkage), das::LineInfo());
		addIEx("LLVMLinkOnceODRLinkage", "LLVMLinkOnceODRLinkage", int64_t(LLVMLinkage::LLVMLinkOnceODRLinkage), das::LineInfo());
		addIEx("LLVMLinkOnceODRAutoHideLinkage", "LLVMLinkOnceODRAutoHideLinkage", int64_t(LLVMLinkage::LLVMLinkOnceODRAutoHideLinkage), das::LineInfo());
		addIEx("LLVMWeakAnyLinkage", "LLVMWeakAnyLinkage", int64_t(LLVMLinkage::LLVMWeakAnyLinkage), das::LineInfo());
		addIEx("LLVMWeakODRLinkage", "LLVMWeakODRLinkage", int64_t(LLVMLinkage::LLVMWeakODRLinkage), das::LineInfo());
		addIEx("LLVMAppendingLinkage", "LLVMAppendingLinkage", int64_t(LLVMLinkage::LLVMAppendingLinkage), das::LineInfo());
		addIEx("LLVMInternalLinkage", "LLVMInternalLinkage", int64_t(LLVMLinkage::LLVMInternalLinkage), das::LineInfo());
		addIEx("LLVMPrivateLinkage", "LLVMPrivateLinkage", int64_t(LLVMLinkage::LLVMPrivateLinkage), das::LineInfo());
		addIEx("LLVMDLLImportLinkage", "LLVMDLLImportLinkage", int64_t(LLVMLinkage::LLVMDLLImportLinkage), das::LineInfo());
		addIEx("LLVMDLLExportLinkage", "LLVMDLLExportLinkage", int64_t(LLVMLinkage::LLVMDLLExportLinkage), das::LineInfo());
		addIEx("LLVMExternalWeakLinkage", "LLVMExternalWeakLinkage", int64_t(LLVMLinkage::LLVMExternalWeakLinkage), das::LineInfo());
		addIEx("LLVMGhostLinkage", "LLVMGhostLinkage", int64_t(LLVMLinkage::LLVMGhostLinkage), das::LineInfo());
		addIEx("LLVMCommonLinkage", "LLVMCommonLinkage", int64_t(LLVMLinkage::LLVMCommonLinkage), das::LineInfo());
		addIEx("LLVMLinkerPrivateLinkage", "LLVMLinkerPrivateLinkage", int64_t(LLVMLinkage::LLVMLinkerPrivateLinkage), das::LineInfo());
		addIEx("LLVMLinkerPrivateWeakLinkage", "LLVMLinkerPrivateWeakLinkage", int64_t(LLVMLinkage::LLVMLinkerPrivateWeakLinkage), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:194:9
class Enumeration_LLVMVisibility : public das::Enumeration{
public:
	Enumeration_LLVMVisibility() : das::Enumeration("LLVMVisibility") {
		external = true;
		cppName = "LLVMVisibility";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMDefaultVisibility", "LLVMDefaultVisibility", int64_t(LLVMVisibility::LLVMDefaultVisibility), das::LineInfo());
		addIEx("LLVMHiddenVisibility", "LLVMHiddenVisibility", int64_t(LLVMVisibility::LLVMHiddenVisibility), das::LineInfo());
		addIEx("LLVMProtectedVisibility", "LLVMProtectedVisibility", int64_t(LLVMVisibility::LLVMProtectedVisibility), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:200:9
class Enumeration_LLVMUnnamedAddr : public das::Enumeration{
public:
	Enumeration_LLVMUnnamedAddr() : das::Enumeration("LLVMUnnamedAddr") {
		external = true;
		cppName = "LLVMUnnamedAddr";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMNoUnnamedAddr", "LLVMNoUnnamedAddr", int64_t(LLVMUnnamedAddr::LLVMNoUnnamedAddr), das::LineInfo());
		addIEx("LLVMLocalUnnamedAddr", "LLVMLocalUnnamedAddr", int64_t(LLVMUnnamedAddr::LLVMLocalUnnamedAddr), das::LineInfo());
		addIEx("LLVMGlobalUnnamedAddr", "LLVMGlobalUnnamedAddr", int64_t(LLVMUnnamedAddr::LLVMGlobalUnnamedAddr), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:206:9
class Enumeration_LLVMDLLStorageClass : public das::Enumeration{
public:
	Enumeration_LLVMDLLStorageClass() : das::Enumeration("LLVMDLLStorageClass") {
		external = true;
		cppName = "LLVMDLLStorageClass";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMDefaultStorageClass", "LLVMDefaultStorageClass", int64_t(LLVMDLLStorageClass::LLVMDefaultStorageClass), das::LineInfo());
		addIEx("LLVMDLLImportStorageClass", "LLVMDLLImportStorageClass", int64_t(LLVMDLLStorageClass::LLVMDLLImportStorageClass), das::LineInfo());
		addIEx("LLVMDLLExportStorageClass", "LLVMDLLExportStorageClass", int64_t(LLVMDLLStorageClass::LLVMDLLExportStorageClass), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:212:9
class Enumeration_LLVMCallConv : public das::Enumeration{
public:
	Enumeration_LLVMCallConv() : das::Enumeration("LLVMCallConv") {
		external = true;
		cppName = "LLVMCallConv";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMCCallConv", "LLVMCCallConv", int64_t(LLVMCallConv::LLVMCCallConv), das::LineInfo());
		addIEx("LLVMFastCallConv", "LLVMFastCallConv", int64_t(LLVMCallConv::LLVMFastCallConv), das::LineInfo());
		addIEx("LLVMColdCallConv", "LLVMColdCallConv", int64_t(LLVMCallConv::LLVMColdCallConv), das::LineInfo());
		addIEx("LLVMGHCCallConv", "LLVMGHCCallConv", int64_t(LLVMCallConv::LLVMGHCCallConv), das::LineInfo());
		addIEx("LLVMHiPECallConv", "LLVMHiPECallConv", int64_t(LLVMCallConv::LLVMHiPECallConv), das::LineInfo());
		addIEx("LLVMWebKitJSCallConv", "LLVMWebKitJSCallConv", int64_t(LLVMCallConv::LLVMWebKitJSCallConv), das::LineInfo());
		addIEx("LLVMAnyRegCallConv", "LLVMAnyRegCallConv", int64_t(LLVMCallConv::LLVMAnyRegCallConv), das::LineInfo());
		addIEx("LLVMPreserveMostCallConv", "LLVMPreserveMostCallConv", int64_t(LLVMCallConv::LLVMPreserveMostCallConv), das::LineInfo());
		addIEx("LLVMPreserveAllCallConv", "LLVMPreserveAllCallConv", int64_t(LLVMCallConv::LLVMPreserveAllCallConv), das::LineInfo());
		addIEx("LLVMSwiftCallConv", "LLVMSwiftCallConv", int64_t(LLVMCallConv::LLVMSwiftCallConv), das::LineInfo());
		addIEx("LLVMCXXFASTTLSCallConv", "LLVMCXXFASTTLSCallConv", int64_t(LLVMCallConv::LLVMCXXFASTTLSCallConv), das::LineInfo());
		addIEx("LLVMX86StdcallCallConv", "LLVMX86StdcallCallConv", int64_t(LLVMCallConv::LLVMX86StdcallCallConv), das::LineInfo());
		addIEx("LLVMX86FastcallCallConv", "LLVMX86FastcallCallConv", int64_t(LLVMCallConv::LLVMX86FastcallCallConv), das::LineInfo());
		addIEx("LLVMARMAPCSCallConv", "LLVMARMAPCSCallConv", int64_t(LLVMCallConv::LLVMARMAPCSCallConv), das::LineInfo());
		addIEx("LLVMARMAAPCSCallConv", "LLVMARMAAPCSCallConv", int64_t(LLVMCallConv::LLVMARMAAPCSCallConv), das::LineInfo());
		addIEx("LLVMARMAAPCSVFPCallConv", "LLVMARMAAPCSVFPCallConv", int64_t(LLVMCallConv::LLVMARMAAPCSVFPCallConv), das::LineInfo());
		addIEx("LLVMMSP430INTRCallConv", "LLVMMSP430INTRCallConv", int64_t(LLVMCallConv::LLVMMSP430INTRCallConv), das::LineInfo());
		addIEx("LLVMX86ThisCallCallConv", "LLVMX86ThisCallCallConv", int64_t(LLVMCallConv::LLVMX86ThisCallCallConv), das::LineInfo());
		addIEx("LLVMPTXKernelCallConv", "LLVMPTXKernelCallConv", int64_t(LLVMCallConv::LLVMPTXKernelCallConv), das::LineInfo());
		addIEx("LLVMPTXDeviceCallConv", "LLVMPTXDeviceCallConv", int64_t(LLVMCallConv::LLVMPTXDeviceCallConv), das::LineInfo());
		addIEx("LLVMSPIRFUNCCallConv", "LLVMSPIRFUNCCallConv", int64_t(LLVMCallConv::LLVMSPIRFUNCCallConv), das::LineInfo());
		addIEx("LLVMSPIRKERNELCallConv", "LLVMSPIRKERNELCallConv", int64_t(LLVMCallConv::LLVMSPIRKERNELCallConv), das::LineInfo());
		addIEx("LLVMIntelOCLBICallConv", "LLVMIntelOCLBICallConv", int64_t(LLVMCallConv::LLVMIntelOCLBICallConv), das::LineInfo());
		addIEx("LLVMX8664SysVCallConv", "LLVMX8664SysVCallConv", int64_t(LLVMCallConv::LLVMX8664SysVCallConv), das::LineInfo());
		addIEx("LLVMWin64CallConv", "LLVMWin64CallConv", int64_t(LLVMCallConv::LLVMWin64CallConv), das::LineInfo());
		addIEx("LLVMX86VectorCallCallConv", "LLVMX86VectorCallCallConv", int64_t(LLVMCallConv::LLVMX86VectorCallCallConv), das::LineInfo());
		addIEx("LLVMHHVMCallConv", "LLVMHHVMCallConv", int64_t(LLVMCallConv::LLVMHHVMCallConv), das::LineInfo());
		addIEx("LLVMHHVMCCallConv", "LLVMHHVMCCallConv", int64_t(LLVMCallConv::LLVMHHVMCCallConv), das::LineInfo());
		addIEx("LLVMX86INTRCallConv", "LLVMX86INTRCallConv", int64_t(LLVMCallConv::LLVMX86INTRCallConv), das::LineInfo());
		addIEx("LLVMAVRINTRCallConv", "LLVMAVRINTRCallConv", int64_t(LLVMCallConv::LLVMAVRINTRCallConv), das::LineInfo());
		addIEx("LLVMAVRSIGNALCallConv", "LLVMAVRSIGNALCallConv", int64_t(LLVMCallConv::LLVMAVRSIGNALCallConv), das::LineInfo());
		addIEx("LLVMAVRBUILTINCallConv", "LLVMAVRBUILTINCallConv", int64_t(LLVMCallConv::LLVMAVRBUILTINCallConv), das::LineInfo());
		addIEx("LLVMAMDGPUVSCallConv", "LLVMAMDGPUVSCallConv", int64_t(LLVMCallConv::LLVMAMDGPUVSCallConv), das::LineInfo());
		addIEx("LLVMAMDGPUGSCallConv", "LLVMAMDGPUGSCallConv", int64_t(LLVMCallConv::LLVMAMDGPUGSCallConv), das::LineInfo());
		addIEx("LLVMAMDGPUPSCallConv", "LLVMAMDGPUPSCallConv", int64_t(LLVMCallConv::LLVMAMDGPUPSCallConv), das::LineInfo());
		addIEx("LLVMAMDGPUCSCallConv", "LLVMAMDGPUCSCallConv", int64_t(LLVMCallConv::LLVMAMDGPUCSCallConv), das::LineInfo());
		addIEx("LLVMAMDGPUKERNELCallConv", "LLVMAMDGPUKERNELCallConv", int64_t(LLVMCallConv::LLVMAMDGPUKERNELCallConv), das::LineInfo());
		addIEx("LLVMX86RegCallCallConv", "LLVMX86RegCallCallConv", int64_t(LLVMCallConv::LLVMX86RegCallCallConv), das::LineInfo());
		addIEx("LLVMAMDGPUHSCallConv", "LLVMAMDGPUHSCallConv", int64_t(LLVMCallConv::LLVMAMDGPUHSCallConv), das::LineInfo());
		addIEx("LLVMMSP430BUILTINCallConv", "LLVMMSP430BUILTINCallConv", int64_t(LLVMCallConv::LLVMMSP430BUILTINCallConv), das::LineInfo());
		addIEx("LLVMAMDGPULSCallConv", "LLVMAMDGPULSCallConv", int64_t(LLVMCallConv::LLVMAMDGPULSCallConv), das::LineInfo());
		addIEx("LLVMAMDGPUESCallConv", "LLVMAMDGPUESCallConv", int64_t(LLVMCallConv::LLVMAMDGPUESCallConv), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:257:9
class Enumeration_LLVMValueKind : public das::Enumeration{
public:
	Enumeration_LLVMValueKind() : das::Enumeration("LLVMValueKind") {
		external = true;
		cppName = "LLVMValueKind";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMArgumentValueKind", "LLVMArgumentValueKind", int64_t(LLVMValueKind::LLVMArgumentValueKind), das::LineInfo());
		addIEx("LLVMBasicBlockValueKind", "LLVMBasicBlockValueKind", int64_t(LLVMValueKind::LLVMBasicBlockValueKind), das::LineInfo());
		addIEx("LLVMMemoryUseValueKind", "LLVMMemoryUseValueKind", int64_t(LLVMValueKind::LLVMMemoryUseValueKind), das::LineInfo());
		addIEx("LLVMMemoryDefValueKind", "LLVMMemoryDefValueKind", int64_t(LLVMValueKind::LLVMMemoryDefValueKind), das::LineInfo());
		addIEx("LLVMMemoryPhiValueKind", "LLVMMemoryPhiValueKind", int64_t(LLVMValueKind::LLVMMemoryPhiValueKind), das::LineInfo());
		addIEx("LLVMFunctionValueKind", "LLVMFunctionValueKind", int64_t(LLVMValueKind::LLVMFunctionValueKind), das::LineInfo());
		addIEx("LLVMGlobalAliasValueKind", "LLVMGlobalAliasValueKind", int64_t(LLVMValueKind::LLVMGlobalAliasValueKind), das::LineInfo());
		addIEx("LLVMGlobalIFuncValueKind", "LLVMGlobalIFuncValueKind", int64_t(LLVMValueKind::LLVMGlobalIFuncValueKind), das::LineInfo());
		addIEx("LLVMGlobalVariableValueKind", "LLVMGlobalVariableValueKind", int64_t(LLVMValueKind::LLVMGlobalVariableValueKind), das::LineInfo());
		addIEx("LLVMBlockAddressValueKind", "LLVMBlockAddressValueKind", int64_t(LLVMValueKind::LLVMBlockAddressValueKind), das::LineInfo());
		addIEx("LLVMConstantExprValueKind", "LLVMConstantExprValueKind", int64_t(LLVMValueKind::LLVMConstantExprValueKind), das::LineInfo());
		addIEx("LLVMConstantArrayValueKind", "LLVMConstantArrayValueKind", int64_t(LLVMValueKind::LLVMConstantArrayValueKind), das::LineInfo());
		addIEx("LLVMConstantStructValueKind", "LLVMConstantStructValueKind", int64_t(LLVMValueKind::LLVMConstantStructValueKind), das::LineInfo());
		addIEx("LLVMConstantVectorValueKind", "LLVMConstantVectorValueKind", int64_t(LLVMValueKind::LLVMConstantVectorValueKind), das::LineInfo());
		addIEx("LLVMUndefValueValueKind", "LLVMUndefValueValueKind", int64_t(LLVMValueKind::LLVMUndefValueValueKind), das::LineInfo());
		addIEx("LLVMConstantAggregateZeroValueKind", "LLVMConstantAggregateZeroValueKind", int64_t(LLVMValueKind::LLVMConstantAggregateZeroValueKind), das::LineInfo());
		addIEx("LLVMConstantDataArrayValueKind", "LLVMConstantDataArrayValueKind", int64_t(LLVMValueKind::LLVMConstantDataArrayValueKind), das::LineInfo());
		addIEx("LLVMConstantDataVectorValueKind", "LLVMConstantDataVectorValueKind", int64_t(LLVMValueKind::LLVMConstantDataVectorValueKind), das::LineInfo());
		addIEx("LLVMConstantIntValueKind", "LLVMConstantIntValueKind", int64_t(LLVMValueKind::LLVMConstantIntValueKind), das::LineInfo());
		addIEx("LLVMConstantFPValueKind", "LLVMConstantFPValueKind", int64_t(LLVMValueKind::LLVMConstantFPValueKind), das::LineInfo());
		addIEx("LLVMConstantPointerNullValueKind", "LLVMConstantPointerNullValueKind", int64_t(LLVMValueKind::LLVMConstantPointerNullValueKind), das::LineInfo());
		addIEx("LLVMConstantTokenNoneValueKind", "LLVMConstantTokenNoneValueKind", int64_t(LLVMValueKind::LLVMConstantTokenNoneValueKind), das::LineInfo());
		addIEx("LLVMMetadataAsValueValueKind", "LLVMMetadataAsValueValueKind", int64_t(LLVMValueKind::LLVMMetadataAsValueValueKind), das::LineInfo());
		addIEx("LLVMInlineAsmValueKind", "LLVMInlineAsmValueKind", int64_t(LLVMValueKind::LLVMInlineAsmValueKind), das::LineInfo());
		addIEx("LLVMInstructionValueKind", "LLVMInstructionValueKind", int64_t(LLVMValueKind::LLVMInstructionValueKind), das::LineInfo());
		addIEx("LLVMPoisonValueValueKind", "LLVMPoisonValueValueKind", int64_t(LLVMValueKind::LLVMPoisonValueValueKind), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:290:9
class Enumeration_LLVMIntPredicate : public das::Enumeration{
public:
	Enumeration_LLVMIntPredicate() : das::Enumeration("LLVMIntPredicate") {
		external = true;
		cppName = "LLVMIntPredicate";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMIntEQ", "LLVMIntEQ", int64_t(LLVMIntPredicate::LLVMIntEQ), das::LineInfo());
		addIEx("LLVMIntNE", "LLVMIntNE", int64_t(LLVMIntPredicate::LLVMIntNE), das::LineInfo());
		addIEx("LLVMIntUGT", "LLVMIntUGT", int64_t(LLVMIntPredicate::LLVMIntUGT), das::LineInfo());
		addIEx("LLVMIntUGE", "LLVMIntUGE", int64_t(LLVMIntPredicate::LLVMIntUGE), das::LineInfo());
		addIEx("LLVMIntULT", "LLVMIntULT", int64_t(LLVMIntPredicate::LLVMIntULT), das::LineInfo());
		addIEx("LLVMIntULE", "LLVMIntULE", int64_t(LLVMIntPredicate::LLVMIntULE), das::LineInfo());
		addIEx("LLVMIntSGT", "LLVMIntSGT", int64_t(LLVMIntPredicate::LLVMIntSGT), das::LineInfo());
		addIEx("LLVMIntSGE", "LLVMIntSGE", int64_t(LLVMIntPredicate::LLVMIntSGE), das::LineInfo());
		addIEx("LLVMIntSLT", "LLVMIntSLT", int64_t(LLVMIntPredicate::LLVMIntSLT), das::LineInfo());
		addIEx("LLVMIntSLE", "LLVMIntSLE", int64_t(LLVMIntPredicate::LLVMIntSLE), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:303:9
class Enumeration_LLVMRealPredicate : public das::Enumeration{
public:
	Enumeration_LLVMRealPredicate() : das::Enumeration("LLVMRealPredicate") {
		external = true;
		cppName = "LLVMRealPredicate";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("False", "LLVMRealPredicateFalse", int64_t(LLVMRealPredicate::LLVMRealPredicateFalse), das::LineInfo());
		addIEx("LLVMRealOEQ", "LLVMRealOEQ", int64_t(LLVMRealPredicate::LLVMRealOEQ), das::LineInfo());
		addIEx("LLVMRealOGT", "LLVMRealOGT", int64_t(LLVMRealPredicate::LLVMRealOGT), das::LineInfo());
		addIEx("LLVMRealOGE", "LLVMRealOGE", int64_t(LLVMRealPredicate::LLVMRealOGE), das::LineInfo());
		addIEx("LLVMRealOLT", "LLVMRealOLT", int64_t(LLVMRealPredicate::LLVMRealOLT), das::LineInfo());
		addIEx("LLVMRealOLE", "LLVMRealOLE", int64_t(LLVMRealPredicate::LLVMRealOLE), das::LineInfo());
		addIEx("LLVMRealONE", "LLVMRealONE", int64_t(LLVMRealPredicate::LLVMRealONE), das::LineInfo());
		addIEx("LLVMRealORD", "LLVMRealORD", int64_t(LLVMRealPredicate::LLVMRealORD), das::LineInfo());
		addIEx("LLVMRealUNO", "LLVMRealUNO", int64_t(LLVMRealPredicate::LLVMRealUNO), das::LineInfo());
		addIEx("LLVMRealUEQ", "LLVMRealUEQ", int64_t(LLVMRealPredicate::LLVMRealUEQ), das::LineInfo());
		addIEx("LLVMRealUGT", "LLVMRealUGT", int64_t(LLVMRealPredicate::LLVMRealUGT), das::LineInfo());
		addIEx("LLVMRealUGE", "LLVMRealUGE", int64_t(LLVMRealPredicate::LLVMRealUGE), das::LineInfo());
		addIEx("LLVMRealULT", "LLVMRealULT", int64_t(LLVMRealPredicate::LLVMRealULT), das::LineInfo());
		addIEx("LLVMRealULE", "LLVMRealULE", int64_t(LLVMRealPredicate::LLVMRealULE), das::LineInfo());
		addIEx("LLVMRealUNE", "LLVMRealUNE", int64_t(LLVMRealPredicate::LLVMRealUNE), das::LineInfo());
		addIEx("True", "LLVMRealPredicateTrue", int64_t(LLVMRealPredicate::LLVMRealPredicateTrue), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:322:9
class Enumeration_LLVMLandingPadClauseTy : public das::Enumeration{
public:
	Enumeration_LLVMLandingPadClauseTy() : das::Enumeration("LLVMLandingPadClauseTy") {
		external = true;
		cppName = "LLVMLandingPadClauseTy";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMLandingPadCatch", "LLVMLandingPadCatch", int64_t(LLVMLandingPadClauseTy::LLVMLandingPadCatch), das::LineInfo());
		addIEx("LLVMLandingPadFilter", "LLVMLandingPadFilter", int64_t(LLVMLandingPadClauseTy::LLVMLandingPadFilter), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:327:9
class Enumeration_LLVMThreadLocalMode : public das::Enumeration{
public:
	Enumeration_LLVMThreadLocalMode() : das::Enumeration("LLVMThreadLocalMode") {
		external = true;
		cppName = "LLVMThreadLocalMode";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMNotThreadLocal", "LLVMNotThreadLocal", int64_t(LLVMThreadLocalMode::LLVMNotThreadLocal), das::LineInfo());
		addIEx("LLVMGeneralDynamicTLSModel", "LLVMGeneralDynamicTLSModel", int64_t(LLVMThreadLocalMode::LLVMGeneralDynamicTLSModel), das::LineInfo());
		addIEx("LLVMLocalDynamicTLSModel", "LLVMLocalDynamicTLSModel", int64_t(LLVMThreadLocalMode::LLVMLocalDynamicTLSModel), das::LineInfo());
		addIEx("LLVMInitialExecTLSModel", "LLVMInitialExecTLSModel", int64_t(LLVMThreadLocalMode::LLVMInitialExecTLSModel), das::LineInfo());
		addIEx("LLVMLocalExecTLSModel", "LLVMLocalExecTLSModel", int64_t(LLVMThreadLocalMode::LLVMLocalExecTLSModel), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:335:9
class Enumeration_LLVMAtomicOrdering : public das::Enumeration{
public:
	Enumeration_LLVMAtomicOrdering() : das::Enumeration("LLVMAtomicOrdering") {
		external = true;
		cppName = "LLVMAtomicOrdering";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("NotAtomic", "LLVMAtomicOrderingNotAtomic", int64_t(LLVMAtomicOrdering::LLVMAtomicOrderingNotAtomic), das::LineInfo());
		addIEx("Unordered", "LLVMAtomicOrderingUnordered", int64_t(LLVMAtomicOrdering::LLVMAtomicOrderingUnordered), das::LineInfo());
		addIEx("Monotonic", "LLVMAtomicOrderingMonotonic", int64_t(LLVMAtomicOrdering::LLVMAtomicOrderingMonotonic), das::LineInfo());
		addIEx("Acquire", "LLVMAtomicOrderingAcquire", int64_t(LLVMAtomicOrdering::LLVMAtomicOrderingAcquire), das::LineInfo());
		addIEx("Release", "LLVMAtomicOrderingRelease", int64_t(LLVMAtomicOrdering::LLVMAtomicOrderingRelease), das::LineInfo());
		addIEx("AcquireRelease", "LLVMAtomicOrderingAcquireRelease", int64_t(LLVMAtomicOrdering::LLVMAtomicOrderingAcquireRelease), das::LineInfo());
		addIEx("SequentiallyConsistent", "LLVMAtomicOrderingSequentiallyConsistent", int64_t(LLVMAtomicOrdering::LLVMAtomicOrderingSequentiallyConsistent), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:362:9
class Enumeration_LLVMAtomicRMWBinOp : public das::Enumeration{
public:
	Enumeration_LLVMAtomicRMWBinOp() : das::Enumeration("LLVMAtomicRMWBinOp") {
		external = true;
		cppName = "LLVMAtomicRMWBinOp";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("Xchg", "LLVMAtomicRMWBinOpXchg", int64_t(LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpXchg), das::LineInfo());
		addIEx("Add", "LLVMAtomicRMWBinOpAdd", int64_t(LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpAdd), das::LineInfo());
		addIEx("Sub", "LLVMAtomicRMWBinOpSub", int64_t(LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpSub), das::LineInfo());
		addIEx("And", "LLVMAtomicRMWBinOpAnd", int64_t(LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpAnd), das::LineInfo());
		addIEx("Nand", "LLVMAtomicRMWBinOpNand", int64_t(LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpNand), das::LineInfo());
		addIEx("Or", "LLVMAtomicRMWBinOpOr", int64_t(LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpOr), das::LineInfo());
		addIEx("Xor", "LLVMAtomicRMWBinOpXor", int64_t(LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpXor), das::LineInfo());
		addIEx("Max", "LLVMAtomicRMWBinOpMax", int64_t(LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpMax), das::LineInfo());
		addIEx("Min", "LLVMAtomicRMWBinOpMin", int64_t(LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpMin), das::LineInfo());
		addIEx("UMax", "LLVMAtomicRMWBinOpUMax", int64_t(LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpUMax), das::LineInfo());
		addIEx("UMin", "LLVMAtomicRMWBinOpUMin", int64_t(LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpUMin), das::LineInfo());
		addIEx("FAdd", "LLVMAtomicRMWBinOpFAdd", int64_t(LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpFAdd), das::LineInfo());
		addIEx("FSub", "LLVMAtomicRMWBinOpFSub", int64_t(LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpFSub), das::LineInfo());
		addIEx("FMax", "LLVMAtomicRMWBinOpFMax", int64_t(LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpFMax), das::LineInfo());
		addIEx("FMin", "LLVMAtomicRMWBinOpFMin", int64_t(LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpFMin), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:394:9
class Enumeration_LLVMDiagnosticSeverity : public das::Enumeration{
public:
	Enumeration_LLVMDiagnosticSeverity() : das::Enumeration("LLVMDiagnosticSeverity") {
		external = true;
		cppName = "LLVMDiagnosticSeverity";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMDSError", "LLVMDSError", int64_t(LLVMDiagnosticSeverity::LLVMDSError), das::LineInfo());
		addIEx("LLVMDSWarning", "LLVMDSWarning", int64_t(LLVMDiagnosticSeverity::LLVMDSWarning), das::LineInfo());
		addIEx("LLVMDSRemark", "LLVMDSRemark", int64_t(LLVMDiagnosticSeverity::LLVMDSRemark), das::LineInfo());
		addIEx("LLVMDSNote", "LLVMDSNote", int64_t(LLVMDiagnosticSeverity::LLVMDSNote), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:401:9
class Enumeration_LLVMInlineAsmDialect : public das::Enumeration{
public:
	Enumeration_LLVMInlineAsmDialect() : das::Enumeration("LLVMInlineAsmDialect") {
		external = true;
		cppName = "LLVMInlineAsmDialect";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("ATT", "LLVMInlineAsmDialectATT", int64_t(LLVMInlineAsmDialect::LLVMInlineAsmDialectATT), das::LineInfo());
		addIEx("Intel", "LLVMInlineAsmDialectIntel", int64_t(LLVMInlineAsmDialect::LLVMInlineAsmDialectIntel), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Core.h:406:9
class Enumeration_LLVMModuleFlagBehavior : public das::Enumeration{
public:
	Enumeration_LLVMModuleFlagBehavior() : das::Enumeration("LLVMModuleFlagBehavior") {
		external = true;
		cppName = "LLVMModuleFlagBehavior";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("Error", "LLVMModuleFlagBehaviorError", int64_t(LLVMModuleFlagBehavior::LLVMModuleFlagBehaviorError), das::LineInfo());
		addIEx("Warning", "LLVMModuleFlagBehaviorWarning", int64_t(LLVMModuleFlagBehavior::LLVMModuleFlagBehaviorWarning), das::LineInfo());
		addIEx("Require", "LLVMModuleFlagBehaviorRequire", int64_t(LLVMModuleFlagBehavior::LLVMModuleFlagBehaviorRequire), das::LineInfo());
		addIEx("Override", "LLVMModuleFlagBehaviorOverride", int64_t(LLVMModuleFlagBehavior::LLVMModuleFlagBehaviorOverride), das::LineInfo());
		addIEx("Append", "LLVMModuleFlagBehaviorAppend", int64_t(LLVMModuleFlagBehavior::LLVMModuleFlagBehaviorAppend), das::LineInfo());
		addIEx("AppendUnique", "LLVMModuleFlagBehaviorAppendUnique", int64_t(LLVMModuleFlagBehavior::LLVMModuleFlagBehaviorAppendUnique), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/DebugInfo.h:34:9
class Enumeration_LLVMDIFlags : public das::Enumeration{
public:
	Enumeration_LLVMDIFlags() : das::Enumeration("LLVMDIFlags") {
		external = true;
		cppName = "LLVMDIFlags";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMDIFlagZero", "LLVMDIFlagZero", int64_t(LLVMDIFlags::LLVMDIFlagZero), das::LineInfo());
		addIEx("LLVMDIFlagPrivate", "LLVMDIFlagPrivate", int64_t(LLVMDIFlags::LLVMDIFlagPrivate), das::LineInfo());
		addIEx("LLVMDIFlagProtected", "LLVMDIFlagProtected", int64_t(LLVMDIFlags::LLVMDIFlagProtected), das::LineInfo());
		addIEx("LLVMDIFlagPublic", "LLVMDIFlagPublic", int64_t(LLVMDIFlags::LLVMDIFlagPublic), das::LineInfo());
		addIEx("LLVMDIFlagFwdDecl", "LLVMDIFlagFwdDecl", int64_t(LLVMDIFlags::LLVMDIFlagFwdDecl), das::LineInfo());
		addIEx("LLVMDIFlagAppleBlock", "LLVMDIFlagAppleBlock", int64_t(LLVMDIFlags::LLVMDIFlagAppleBlock), das::LineInfo());
		addIEx("LLVMDIFlagReservedBit4", "LLVMDIFlagReservedBit4", int64_t(LLVMDIFlags::LLVMDIFlagReservedBit4), das::LineInfo());
		addIEx("LLVMDIFlagVirtual", "LLVMDIFlagVirtual", int64_t(LLVMDIFlags::LLVMDIFlagVirtual), das::LineInfo());
		addIEx("LLVMDIFlagArtificial", "LLVMDIFlagArtificial", int64_t(LLVMDIFlags::LLVMDIFlagArtificial), das::LineInfo());
		addIEx("LLVMDIFlagExplicit", "LLVMDIFlagExplicit", int64_t(LLVMDIFlags::LLVMDIFlagExplicit), das::LineInfo());
		addIEx("LLVMDIFlagPrototyped", "LLVMDIFlagPrototyped", int64_t(LLVMDIFlags::LLVMDIFlagPrototyped), das::LineInfo());
		addIEx("LLVMDIFlagObjcClassComplete", "LLVMDIFlagObjcClassComplete", int64_t(LLVMDIFlags::LLVMDIFlagObjcClassComplete), das::LineInfo());
		addIEx("LLVMDIFlagObjectPointer", "LLVMDIFlagObjectPointer", int64_t(LLVMDIFlags::LLVMDIFlagObjectPointer), das::LineInfo());
		addIEx("LLVMDIFlagVector", "LLVMDIFlagVector", int64_t(LLVMDIFlags::LLVMDIFlagVector), das::LineInfo());
		addIEx("LLVMDIFlagStaticMember", "LLVMDIFlagStaticMember", int64_t(LLVMDIFlags::LLVMDIFlagStaticMember), das::LineInfo());
		addIEx("LLVMDIFlagLValueReference", "LLVMDIFlagLValueReference", int64_t(LLVMDIFlags::LLVMDIFlagLValueReference), das::LineInfo());
		addIEx("LLVMDIFlagRValueReference", "LLVMDIFlagRValueReference", int64_t(LLVMDIFlags::LLVMDIFlagRValueReference), das::LineInfo());
		addIEx("LLVMDIFlagReserved", "LLVMDIFlagReserved", int64_t(LLVMDIFlags::LLVMDIFlagReserved), das::LineInfo());
		addIEx("LLVMDIFlagSingleInheritance", "LLVMDIFlagSingleInheritance", int64_t(LLVMDIFlags::LLVMDIFlagSingleInheritance), das::LineInfo());
		addIEx("LLVMDIFlagMultipleInheritance", "LLVMDIFlagMultipleInheritance", int64_t(LLVMDIFlags::LLVMDIFlagMultipleInheritance), das::LineInfo());
		addIEx("LLVMDIFlagVirtualInheritance", "LLVMDIFlagVirtualInheritance", int64_t(LLVMDIFlags::LLVMDIFlagVirtualInheritance), das::LineInfo());
		addIEx("LLVMDIFlagIntroducedVirtual", "LLVMDIFlagIntroducedVirtual", int64_t(LLVMDIFlags::LLVMDIFlagIntroducedVirtual), das::LineInfo());
		addIEx("LLVMDIFlagBitField", "LLVMDIFlagBitField", int64_t(LLVMDIFlags::LLVMDIFlagBitField), das::LineInfo());
		addIEx("LLVMDIFlagNoReturn", "LLVMDIFlagNoReturn", int64_t(LLVMDIFlags::LLVMDIFlagNoReturn), das::LineInfo());
		addIEx("LLVMDIFlagTypePassByValue", "LLVMDIFlagTypePassByValue", int64_t(LLVMDIFlags::LLVMDIFlagTypePassByValue), das::LineInfo());
		addIEx("LLVMDIFlagTypePassByReference", "LLVMDIFlagTypePassByReference", int64_t(LLVMDIFlags::LLVMDIFlagTypePassByReference), das::LineInfo());
		addIEx("LLVMDIFlagEnumClass", "LLVMDIFlagEnumClass", int64_t(LLVMDIFlags::LLVMDIFlagEnumClass), das::LineInfo());
		addIEx("LLVMDIFlagFixedEnum", "LLVMDIFlagFixedEnum", int64_t(LLVMDIFlags::LLVMDIFlagFixedEnum), das::LineInfo());
		addIEx("LLVMDIFlagThunk", "LLVMDIFlagThunk", int64_t(LLVMDIFlags::LLVMDIFlagThunk), das::LineInfo());
		addIEx("LLVMDIFlagNonTrivial", "LLVMDIFlagNonTrivial", int64_t(LLVMDIFlags::LLVMDIFlagNonTrivial), das::LineInfo());
		addIEx("LLVMDIFlagBigEndian", "LLVMDIFlagBigEndian", int64_t(LLVMDIFlags::LLVMDIFlagBigEndian), das::LineInfo());
		addIEx("LLVMDIFlagLittleEndian", "LLVMDIFlagLittleEndian", int64_t(LLVMDIFlags::LLVMDIFlagLittleEndian), das::LineInfo());
		addIEx("LLVMDIFlagIndirectVirtualBase", "LLVMDIFlagIndirectVirtualBase", int64_t(LLVMDIFlags::LLVMDIFlagIndirectVirtualBase), das::LineInfo());
		addIEx("LLVMDIFlagAccessibility", "LLVMDIFlagAccessibility", int64_t(LLVMDIFlags::LLVMDIFlagAccessibility), das::LineInfo());
		addIEx("LLVMDIFlagPtrToMemberRep", "LLVMDIFlagPtrToMemberRep", int64_t(LLVMDIFlags::LLVMDIFlagPtrToMemberRep), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/DebugInfo.h:78:9
class Enumeration_LLVMDWARFSourceLanguage : public das::Enumeration{
public:
	Enumeration_LLVMDWARFSourceLanguage() : das::Enumeration("LLVMDWARFSourceLanguage") {
		external = true;
		cppName = "LLVMDWARFSourceLanguage";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("C89", "LLVMDWARFSourceLanguageC89", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageC89), das::LineInfo());
		addIEx("C", "LLVMDWARFSourceLanguageC", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageC), das::LineInfo());
		addIEx("Ada83", "LLVMDWARFSourceLanguageAda83", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageAda83), das::LineInfo());
		addIEx("C_plus_plus", "LLVMDWARFSourceLanguageC_plus_plus", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageC_plus_plus), das::LineInfo());
		addIEx("Cobol74", "LLVMDWARFSourceLanguageCobol74", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageCobol74), das::LineInfo());
		addIEx("Cobol85", "LLVMDWARFSourceLanguageCobol85", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageCobol85), das::LineInfo());
		addIEx("Fortran77", "LLVMDWARFSourceLanguageFortran77", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageFortran77), das::LineInfo());
		addIEx("Fortran90", "LLVMDWARFSourceLanguageFortran90", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageFortran90), das::LineInfo());
		addIEx("Pascal83", "LLVMDWARFSourceLanguagePascal83", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguagePascal83), das::LineInfo());
		addIEx("Modula2", "LLVMDWARFSourceLanguageModula2", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageModula2), das::LineInfo());
		addIEx("Java", "LLVMDWARFSourceLanguageJava", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageJava), das::LineInfo());
		addIEx("C99", "LLVMDWARFSourceLanguageC99", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageC99), das::LineInfo());
		addIEx("Ada95", "LLVMDWARFSourceLanguageAda95", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageAda95), das::LineInfo());
		addIEx("Fortran95", "LLVMDWARFSourceLanguageFortran95", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageFortran95), das::LineInfo());
		addIEx("PLI", "LLVMDWARFSourceLanguagePLI", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguagePLI), das::LineInfo());
		addIEx("ObjC", "LLVMDWARFSourceLanguageObjC", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageObjC), das::LineInfo());
		addIEx("ObjC_plus_plus", "LLVMDWARFSourceLanguageObjC_plus_plus", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageObjC_plus_plus), das::LineInfo());
		addIEx("UPC", "LLVMDWARFSourceLanguageUPC", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageUPC), das::LineInfo());
		addIEx("D", "LLVMDWARFSourceLanguageD", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageD), das::LineInfo());
		addIEx("Python", "LLVMDWARFSourceLanguagePython", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguagePython), das::LineInfo());
		addIEx("OpenCL", "LLVMDWARFSourceLanguageOpenCL", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageOpenCL), das::LineInfo());
		addIEx("Go", "LLVMDWARFSourceLanguageGo", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageGo), das::LineInfo());
		addIEx("Modula3", "LLVMDWARFSourceLanguageModula3", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageModula3), das::LineInfo());
		addIEx("Haskell", "LLVMDWARFSourceLanguageHaskell", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageHaskell), das::LineInfo());
		addIEx("C_plus_plus_03", "LLVMDWARFSourceLanguageC_plus_plus_03", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageC_plus_plus_03), das::LineInfo());
		addIEx("C_plus_plus_11", "LLVMDWARFSourceLanguageC_plus_plus_11", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageC_plus_plus_11), das::LineInfo());
		addIEx("OCaml", "LLVMDWARFSourceLanguageOCaml", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageOCaml), das::LineInfo());
		addIEx("Rust", "LLVMDWARFSourceLanguageRust", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageRust), das::LineInfo());
		addIEx("C11", "LLVMDWARFSourceLanguageC11", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageC11), das::LineInfo());
		addIEx("Swift", "LLVMDWARFSourceLanguageSwift", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageSwift), das::LineInfo());
		addIEx("Julia", "LLVMDWARFSourceLanguageJulia", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageJulia), das::LineInfo());
		addIEx("Dylan", "LLVMDWARFSourceLanguageDylan", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageDylan), das::LineInfo());
		addIEx("C_plus_plus_14", "LLVMDWARFSourceLanguageC_plus_plus_14", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageC_plus_plus_14), das::LineInfo());
		addIEx("Fortran03", "LLVMDWARFSourceLanguageFortran03", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageFortran03), das::LineInfo());
		addIEx("Fortran08", "LLVMDWARFSourceLanguageFortran08", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageFortran08), das::LineInfo());
		addIEx("RenderScript", "LLVMDWARFSourceLanguageRenderScript", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageRenderScript), das::LineInfo());
		addIEx("BLISS", "LLVMDWARFSourceLanguageBLISS", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageBLISS), das::LineInfo());
		addIEx("Mips_Assembler", "LLVMDWARFSourceLanguageMips_Assembler", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageMips_Assembler), das::LineInfo());
		addIEx("GOOGLE_RenderScript", "LLVMDWARFSourceLanguageGOOGLE_RenderScript", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageGOOGLE_RenderScript), das::LineInfo());
		addIEx("BORLAND_Delphi", "LLVMDWARFSourceLanguageBORLAND_Delphi", int64_t(LLVMDWARFSourceLanguage::LLVMDWARFSourceLanguageBORLAND_Delphi), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/DebugInfo.h:128:9
class Enumeration_LLVMDWARFEmissionKind : public das::Enumeration{
public:
	Enumeration_LLVMDWARFEmissionKind() : das::Enumeration("LLVMDWARFEmissionKind") {
		external = true;
		cppName = "LLVMDWARFEmissionKind";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMDWARFEmissionNone", "LLVMDWARFEmissionNone", int64_t(LLVMDWARFEmissionKind::LLVMDWARFEmissionNone), das::LineInfo());
		addIEx("LLVMDWARFEmissionFull", "LLVMDWARFEmissionFull", int64_t(LLVMDWARFEmissionKind::LLVMDWARFEmissionFull), das::LineInfo());
		addIEx("LLVMDWARFEmissionLineTablesOnly", "LLVMDWARFEmissionLineTablesOnly", int64_t(LLVMDWARFEmissionKind::LLVMDWARFEmissionLineTablesOnly), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/DebugInfo.h:186:9
class Enumeration_LLVMDWARFMacinfoRecordType : public das::Enumeration{
public:
	Enumeration_LLVMDWARFMacinfoRecordType() : das::Enumeration("LLVMDWARFMacinfoRecordType") {
		external = true;
		cppName = "LLVMDWARFMacinfoRecordType";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("Define", "LLVMDWARFMacinfoRecordTypeDefine", int64_t(LLVMDWARFMacinfoRecordType::LLVMDWARFMacinfoRecordTypeDefine), das::LineInfo());
		addIEx("Macro", "LLVMDWARFMacinfoRecordTypeMacro", int64_t(LLVMDWARFMacinfoRecordType::LLVMDWARFMacinfoRecordTypeMacro), das::LineInfo());
		addIEx("StartFile", "LLVMDWARFMacinfoRecordTypeStartFile", int64_t(LLVMDWARFMacinfoRecordType::LLVMDWARFMacinfoRecordTypeStartFile), das::LineInfo());
		addIEx("EndFile", "LLVMDWARFMacinfoRecordTypeEndFile", int64_t(LLVMDWARFMacinfoRecordType::LLVMDWARFMacinfoRecordTypeEndFile), das::LineInfo());
		addIEx("VendorExt", "LLVMDWARFMacinfoRecordTypeVendorExt", int64_t(LLVMDWARFMacinfoRecordType::LLVMDWARFMacinfoRecordTypeVendorExt), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Target.h:35:6
class Enumeration_LLVMByteOrdering : public das::Enumeration{
public:
	Enumeration_LLVMByteOrdering() : das::Enumeration("LLVMByteOrdering") {
		external = true;
		cppName = "LLVMByteOrdering";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMBigEndian", "LLVMBigEndian", int64_t(LLVMByteOrdering::LLVMBigEndian), das::LineInfo());
		addIEx("LLVMLittleEndian", "LLVMLittleEndian", int64_t(LLVMByteOrdering::LLVMLittleEndian), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/TargetMachine.h:37:9
class Enumeration_LLVMCodeGenOptLevel : public das::Enumeration{
public:
	Enumeration_LLVMCodeGenOptLevel() : das::Enumeration("LLVMCodeGenOptLevel") {
		external = true;
		cppName = "LLVMCodeGenOptLevel";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMCodeGenLevelNone", "LLVMCodeGenLevelNone", int64_t(LLVMCodeGenOptLevel::LLVMCodeGenLevelNone), das::LineInfo());
		addIEx("LLVMCodeGenLevelLess", "LLVMCodeGenLevelLess", int64_t(LLVMCodeGenOptLevel::LLVMCodeGenLevelLess), das::LineInfo());
		addIEx("LLVMCodeGenLevelDefault", "LLVMCodeGenLevelDefault", int64_t(LLVMCodeGenOptLevel::LLVMCodeGenLevelDefault), das::LineInfo());
		addIEx("LLVMCodeGenLevelAggressive", "LLVMCodeGenLevelAggressive", int64_t(LLVMCodeGenOptLevel::LLVMCodeGenLevelAggressive), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/TargetMachine.h:44:9
class Enumeration_LLVMRelocMode : public das::Enumeration{
public:
	Enumeration_LLVMRelocMode() : das::Enumeration("LLVMRelocMode") {
		external = true;
		cppName = "LLVMRelocMode";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMRelocDefault", "LLVMRelocDefault", int64_t(LLVMRelocMode::LLVMRelocDefault), das::LineInfo());
		addIEx("LLVMRelocStatic", "LLVMRelocStatic", int64_t(LLVMRelocMode::LLVMRelocStatic), das::LineInfo());
		addIEx("LLVMRelocPIC", "LLVMRelocPIC", int64_t(LLVMRelocMode::LLVMRelocPIC), das::LineInfo());
		addIEx("LLVMRelocDynamicNoPic", "LLVMRelocDynamicNoPic", int64_t(LLVMRelocMode::LLVMRelocDynamicNoPic), das::LineInfo());
		addIEx("LLVMRelocROPI", "LLVMRelocROPI", int64_t(LLVMRelocMode::LLVMRelocROPI), das::LineInfo());
		addIEx("LLVMRelocRWPI", "LLVMRelocRWPI", int64_t(LLVMRelocMode::LLVMRelocRWPI), das::LineInfo());
		addIEx("LLVMRelocROPI_RWPI", "LLVMRelocROPI_RWPI", int64_t(LLVMRelocMode::LLVMRelocROPI_RWPI), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/TargetMachine.h:54:9
class Enumeration_LLVMCodeModel : public das::Enumeration{
public:
	Enumeration_LLVMCodeModel() : das::Enumeration("LLVMCodeModel") {
		external = true;
		cppName = "LLVMCodeModel";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("Default", "LLVMCodeModelDefault", int64_t(LLVMCodeModel::LLVMCodeModelDefault), das::LineInfo());
		addIEx("JITDefault", "LLVMCodeModelJITDefault", int64_t(LLVMCodeModel::LLVMCodeModelJITDefault), das::LineInfo());
		addIEx("Tiny", "LLVMCodeModelTiny", int64_t(LLVMCodeModel::LLVMCodeModelTiny), das::LineInfo());
		addIEx("Small", "LLVMCodeModelSmall", int64_t(LLVMCodeModel::LLVMCodeModelSmall), das::LineInfo());
		addIEx("Kernel", "LLVMCodeModelKernel", int64_t(LLVMCodeModel::LLVMCodeModelKernel), das::LineInfo());
		addIEx("Medium", "LLVMCodeModelMedium", int64_t(LLVMCodeModel::LLVMCodeModelMedium), das::LineInfo());
		addIEx("Large", "LLVMCodeModelLarge", int64_t(LLVMCodeModel::LLVMCodeModelLarge), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/TargetMachine.h:64:9
class Enumeration_LLVMCodeGenFileType : public das::Enumeration{
public:
	Enumeration_LLVMCodeGenFileType() : das::Enumeration("LLVMCodeGenFileType") {
		external = true;
		cppName = "LLVMCodeGenFileType";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMAssemblyFile", "LLVMAssemblyFile", int64_t(LLVMCodeGenFileType::LLVMAssemblyFile), das::LineInfo());
		addIEx("LLVMObjectFile", "LLVMObjectFile", int64_t(LLVMCodeGenFileType::LLVMObjectFile), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Linker.h:30:9
class Enumeration_LLVMLinkerMode : public das::Enumeration{
public:
	Enumeration_LLVMLinkerMode() : das::Enumeration("LLVMLinkerMode") {
		external = true;
		cppName = "LLVMLinkerMode";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LLVMLinkerDestroySource", "LLVMLinkerDestroySource", int64_t(LLVMLinkerMode::LLVMLinkerDestroySource), das::LineInfo());
		addIEx("LLVMLinkerPreserveSource_Removed", "LLVMLinkerPreserveSource_Removed", int64_t(LLVMLinkerMode::LLVMLinkerPreserveSource_Removed), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Orc.h:56:9
class Enumeration_LLVMJITSymbolGenericFlags : public das::Enumeration{
public:
	Enumeration_LLVMJITSymbolGenericFlags() : das::Enumeration("LLVMJITSymbolGenericFlags") {
		external = true;
		cppName = "LLVMJITSymbolGenericFlags";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "LLVMJITSymbolGenericFlagsNone", int64_t(LLVMJITSymbolGenericFlags::LLVMJITSymbolGenericFlagsNone), das::LineInfo());
		addIEx("Exported", "LLVMJITSymbolGenericFlagsExported", int64_t(LLVMJITSymbolGenericFlags::LLVMJITSymbolGenericFlagsExported), das::LineInfo());
		addIEx("Weak", "LLVMJITSymbolGenericFlagsWeak", int64_t(LLVMJITSymbolGenericFlags::LLVMJITSymbolGenericFlagsWeak), das::LineInfo());
		addIEx("Callable", "LLVMJITSymbolGenericFlagsCallable", int64_t(LLVMJITSymbolGenericFlags::LLVMJITSymbolGenericFlagsCallable), das::LineInfo());
		addIEx("MaterializationSideEffectsOnly", "LLVMJITSymbolGenericFlagsMaterializationSideEffectsOnly", int64_t(LLVMJITSymbolGenericFlags::LLVMJITSymbolGenericFlagsMaterializationSideEffectsOnly), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Orc.h:190:9
class Enumeration_LLVMOrcLookupKind : public das::Enumeration{
public:
	Enumeration_LLVMOrcLookupKind() : das::Enumeration("LLVMOrcLookupKind") {
		external = true;
		cppName = "LLVMOrcLookupKind";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("Static", "LLVMOrcLookupKindStatic", int64_t(LLVMOrcLookupKind::LLVMOrcLookupKindStatic), das::LineInfo());
		addIEx("DLSym", "LLVMOrcLookupKindDLSym", int64_t(LLVMOrcLookupKind::LLVMOrcLookupKindDLSym), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Orc.h:201:9
class Enumeration_LLVMOrcJITDylibLookupFlags : public das::Enumeration{
public:
	Enumeration_LLVMOrcJITDylibLookupFlags() : das::Enumeration("LLVMOrcJITDylibLookupFlags") {
		external = true;
		cppName = "LLVMOrcJITDylibLookupFlags";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("MatchExportedSymbolsOnly", "LLVMOrcJITDylibLookupFlagsMatchExportedSymbolsOnly", int64_t(LLVMOrcJITDylibLookupFlags::LLVMOrcJITDylibLookupFlagsMatchExportedSymbolsOnly), das::LineInfo());
		addIEx("MatchAllSymbols", "LLVMOrcJITDylibLookupFlagsMatchAllSymbols", int64_t(LLVMOrcJITDylibLookupFlags::LLVMOrcJITDylibLookupFlagsMatchAllSymbols), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Orc.h:226:9
class Enumeration_LLVMOrcSymbolLookupFlags : public das::Enumeration{
public:
	Enumeration_LLVMOrcSymbolLookupFlags() : das::Enumeration("LLVMOrcSymbolLookupFlags") {
		external = true;
		cppName = "LLVMOrcSymbolLookupFlags";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("RequiredSymbol", "LLVMOrcSymbolLookupFlagsRequiredSymbol", int64_t(LLVMOrcSymbolLookupFlags::LLVMOrcSymbolLookupFlagsRequiredSymbol), das::LineInfo());
		addIEx("WeaklyReferencedSymbol", "LLVMOrcSymbolLookupFlagsWeaklyReferencedSymbol", int64_t(LLVMOrcSymbolLookupFlags::LLVMOrcSymbolLookupFlagsWeaklyReferencedSymbol), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/lto.h:54:9
class Enumeration_lto_symbol_attributes : public das::Enumeration{
public:
	Enumeration_lto_symbol_attributes() : das::Enumeration("lto_symbol_attributes") {
		external = true;
		cppName = "lto_symbol_attributes";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LTO_SYMBOL_ALIGNMENT_MASK", "LTO_SYMBOL_ALIGNMENT_MASK", int64_t(lto_symbol_attributes::LTO_SYMBOL_ALIGNMENT_MASK), das::LineInfo());
		addIEx("LTO_SYMBOL_PERMISSIONS_MASK", "LTO_SYMBOL_PERMISSIONS_MASK", int64_t(lto_symbol_attributes::LTO_SYMBOL_PERMISSIONS_MASK), das::LineInfo());
		addIEx("LTO_SYMBOL_PERMISSIONS_CODE", "LTO_SYMBOL_PERMISSIONS_CODE", int64_t(lto_symbol_attributes::LTO_SYMBOL_PERMISSIONS_CODE), das::LineInfo());
		addIEx("LTO_SYMBOL_PERMISSIONS_DATA", "LTO_SYMBOL_PERMISSIONS_DATA", int64_t(lto_symbol_attributes::LTO_SYMBOL_PERMISSIONS_DATA), das::LineInfo());
		addIEx("LTO_SYMBOL_PERMISSIONS_RODATA", "LTO_SYMBOL_PERMISSIONS_RODATA", int64_t(lto_symbol_attributes::LTO_SYMBOL_PERMISSIONS_RODATA), das::LineInfo());
		addIEx("LTO_SYMBOL_DEFINITION_MASK", "LTO_SYMBOL_DEFINITION_MASK", int64_t(lto_symbol_attributes::LTO_SYMBOL_DEFINITION_MASK), das::LineInfo());
		addIEx("LTO_SYMBOL_DEFINITION_REGULAR", "LTO_SYMBOL_DEFINITION_REGULAR", int64_t(lto_symbol_attributes::LTO_SYMBOL_DEFINITION_REGULAR), das::LineInfo());
		addIEx("LTO_SYMBOL_DEFINITION_TENTATIVE", "LTO_SYMBOL_DEFINITION_TENTATIVE", int64_t(lto_symbol_attributes::LTO_SYMBOL_DEFINITION_TENTATIVE), das::LineInfo());
		addIEx("LTO_SYMBOL_DEFINITION_WEAK", "LTO_SYMBOL_DEFINITION_WEAK", int64_t(lto_symbol_attributes::LTO_SYMBOL_DEFINITION_WEAK), das::LineInfo());
		addIEx("LTO_SYMBOL_DEFINITION_UNDEFINED", "LTO_SYMBOL_DEFINITION_UNDEFINED", int64_t(lto_symbol_attributes::LTO_SYMBOL_DEFINITION_UNDEFINED), das::LineInfo());
		addIEx("LTO_SYMBOL_DEFINITION_WEAKUNDEF", "LTO_SYMBOL_DEFINITION_WEAKUNDEF", int64_t(lto_symbol_attributes::LTO_SYMBOL_DEFINITION_WEAKUNDEF), das::LineInfo());
		addIEx("LTO_SYMBOL_SCOPE_MASK", "LTO_SYMBOL_SCOPE_MASK", int64_t(lto_symbol_attributes::LTO_SYMBOL_SCOPE_MASK), das::LineInfo());
		addIEx("LTO_SYMBOL_SCOPE_INTERNAL", "LTO_SYMBOL_SCOPE_INTERNAL", int64_t(lto_symbol_attributes::LTO_SYMBOL_SCOPE_INTERNAL), das::LineInfo());
		addIEx("LTO_SYMBOL_SCOPE_HIDDEN", "LTO_SYMBOL_SCOPE_HIDDEN", int64_t(lto_symbol_attributes::LTO_SYMBOL_SCOPE_HIDDEN), das::LineInfo());
		addIEx("LTO_SYMBOL_SCOPE_PROTECTED", "LTO_SYMBOL_SCOPE_PROTECTED", int64_t(lto_symbol_attributes::LTO_SYMBOL_SCOPE_PROTECTED), das::LineInfo());
		addIEx("LTO_SYMBOL_SCOPE_DEFAULT", "LTO_SYMBOL_SCOPE_DEFAULT", int64_t(lto_symbol_attributes::LTO_SYMBOL_SCOPE_DEFAULT), das::LineInfo());
		addIEx("LTO_SYMBOL_SCOPE_DEFAULT_CAN_BE_HIDDEN", "LTO_SYMBOL_SCOPE_DEFAULT_CAN_BE_HIDDEN", int64_t(lto_symbol_attributes::LTO_SYMBOL_SCOPE_DEFAULT_CAN_BE_HIDDEN), das::LineInfo());
		addIEx("LTO_SYMBOL_COMDAT", "LTO_SYMBOL_COMDAT", int64_t(lto_symbol_attributes::LTO_SYMBOL_COMDAT), das::LineInfo());
		addIEx("LTO_SYMBOL_ALIAS", "LTO_SYMBOL_ALIAS", int64_t(lto_symbol_attributes::LTO_SYMBOL_ALIAS), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/lto.h:79:9
class Enumeration_lto_debug_model : public das::Enumeration{
public:
	Enumeration_lto_debug_model() : das::Enumeration("lto_debug_model") {
		external = true;
		cppName = "lto_debug_model";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("NONE", "LTO_DEBUG_MODEL_NONE", int64_t(lto_debug_model::LTO_DEBUG_MODEL_NONE), das::LineInfo());
		addIEx("DWARF", "LTO_DEBUG_MODEL_DWARF", int64_t(lto_debug_model::LTO_DEBUG_MODEL_DWARF), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/lto.h:87:9
class Enumeration_lto_codegen_model : public das::Enumeration{
public:
	Enumeration_lto_codegen_model() : das::Enumeration("lto_codegen_model") {
		external = true;
		cppName = "lto_codegen_model";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LTO_CODEGEN_PIC_MODEL_STATIC", "LTO_CODEGEN_PIC_MODEL_STATIC", int64_t(lto_codegen_model::LTO_CODEGEN_PIC_MODEL_STATIC), das::LineInfo());
		addIEx("LTO_CODEGEN_PIC_MODEL_DYNAMIC", "LTO_CODEGEN_PIC_MODEL_DYNAMIC", int64_t(lto_codegen_model::LTO_CODEGEN_PIC_MODEL_DYNAMIC), das::LineInfo());
		addIEx("LTO_CODEGEN_PIC_MODEL_DYNAMIC_NO_PIC", "LTO_CODEGEN_PIC_MODEL_DYNAMIC_NO_PIC", int64_t(lto_codegen_model::LTO_CODEGEN_PIC_MODEL_DYNAMIC_NO_PIC), das::LineInfo());
		addIEx("LTO_CODEGEN_PIC_MODEL_DEFAULT", "LTO_CODEGEN_PIC_MODEL_DEFAULT", int64_t(lto_codegen_model::LTO_CODEGEN_PIC_MODEL_DEFAULT), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/lto.h:330:9
class Enumeration_lto_codegen_diagnostic_severity_t : public das::Enumeration{
public:
	Enumeration_lto_codegen_diagnostic_severity_t() : das::Enumeration("lto_codegen_diagnostic_severity_t") {
		external = true;
		cppName = "lto_codegen_diagnostic_severity_t";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("LTO_DS_ERROR", "LTO_DS_ERROR", int64_t(lto_codegen_diagnostic_severity_t::LTO_DS_ERROR), das::LineInfo());
		addIEx("LTO_DS_WARNING", "LTO_DS_WARNING", int64_t(lto_codegen_diagnostic_severity_t::LTO_DS_WARNING), das::LineInfo());
		addIEx("LTO_DS_REMARK", "LTO_DS_REMARK", int64_t(lto_codegen_diagnostic_severity_t::LTO_DS_REMARK), das::LineInfo());
		addIEx("LTO_DS_NOTE", "LTO_DS_NOTE", int64_t(lto_codegen_diagnostic_severity_t::LTO_DS_NOTE), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Object.h:40:9
class Enumeration_LLVMBinaryType : public das::Enumeration{
public:
	Enumeration_LLVMBinaryType() : das::Enumeration("LLVMBinaryType") {
		external = true;
		cppName = "LLVMBinaryType";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("Archive", "LLVMBinaryTypeArchive", int64_t(LLVMBinaryType::LLVMBinaryTypeArchive), das::LineInfo());
		addIEx("MachOUniversalBinary", "LLVMBinaryTypeMachOUniversalBinary", int64_t(LLVMBinaryType::LLVMBinaryTypeMachOUniversalBinary), das::LineInfo());
		addIEx("COFFImportFile", "LLVMBinaryTypeCOFFImportFile", int64_t(LLVMBinaryType::LLVMBinaryTypeCOFFImportFile), das::LineInfo());
		addIEx("IR", "LLVMBinaryTypeIR", int64_t(LLVMBinaryType::LLVMBinaryTypeIR), das::LineInfo());
		addIEx("WinRes", "LLVMBinaryTypeWinRes", int64_t(LLVMBinaryType::LLVMBinaryTypeWinRes), das::LineInfo());
		addIEx("COFF", "LLVMBinaryTypeCOFF", int64_t(LLVMBinaryType::LLVMBinaryTypeCOFF), das::LineInfo());
		addIEx("ELF32L", "LLVMBinaryTypeELF32L", int64_t(LLVMBinaryType::LLVMBinaryTypeELF32L), das::LineInfo());
		addIEx("ELF32B", "LLVMBinaryTypeELF32B", int64_t(LLVMBinaryType::LLVMBinaryTypeELF32B), das::LineInfo());
		addIEx("ELF64L", "LLVMBinaryTypeELF64L", int64_t(LLVMBinaryType::LLVMBinaryTypeELF64L), das::LineInfo());
		addIEx("ELF64B", "LLVMBinaryTypeELF64B", int64_t(LLVMBinaryType::LLVMBinaryTypeELF64B), das::LineInfo());
		addIEx("MachO32L", "LLVMBinaryTypeMachO32L", int64_t(LLVMBinaryType::LLVMBinaryTypeMachO32L), das::LineInfo());
		addIEx("MachO32B", "LLVMBinaryTypeMachO32B", int64_t(LLVMBinaryType::LLVMBinaryTypeMachO32B), das::LineInfo());
		addIEx("MachO64L", "LLVMBinaryTypeMachO64L", int64_t(LLVMBinaryType::LLVMBinaryTypeMachO64L), das::LineInfo());
		addIEx("MachO64B", "LLVMBinaryTypeMachO64B", int64_t(LLVMBinaryType::LLVMBinaryTypeMachO64B), das::LineInfo());
		addIEx("Wasm", "LLVMBinaryTypeWasm", int64_t(LLVMBinaryType::LLVMBinaryTypeWasm), das::LineInfo());
		addIEx("Offload", "LLVMBinaryTypeOffload", int64_t(LLVMBinaryType::LLVMBinaryTypeOffload), das::LineInfo());
	}
};

// from D:\Work\libclang\include\llvm-c/Remarks.h:41:6
class Enumeration_LLVMRemarkType : public das::Enumeration{
public:
	Enumeration_LLVMRemarkType() : das::Enumeration("LLVMRemarkType") {
		external = true;
		cppName = "LLVMRemarkType";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("Unknown", "LLVMRemarkTypeUnknown", int64_t(LLVMRemarkType::LLVMRemarkTypeUnknown), das::LineInfo());
		addIEx("Passed", "LLVMRemarkTypePassed", int64_t(LLVMRemarkType::LLVMRemarkTypePassed), das::LineInfo());
		addIEx("Missed", "LLVMRemarkTypeMissed", int64_t(LLVMRemarkType::LLVMRemarkTypeMissed), das::LineInfo());
		addIEx("Analysis", "LLVMRemarkTypeAnalysis", int64_t(LLVMRemarkType::LLVMRemarkTypeAnalysis), das::LineInfo());
		addIEx("AnalysisFPCommute", "LLVMRemarkTypeAnalysisFPCommute", int64_t(LLVMRemarkType::LLVMRemarkTypeAnalysisFPCommute), das::LineInfo());
		addIEx("AnalysisAliasing", "LLVMRemarkTypeAnalysisAliasing", int64_t(LLVMRemarkType::LLVMRemarkTypeAnalysisAliasing), das::LineInfo());
		addIEx("Failure", "LLVMRemarkTypeFailure", int64_t(LLVMRemarkType::LLVMRemarkTypeFailure), das::LineInfo());
	}
};

