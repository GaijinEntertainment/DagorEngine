// this file is generated via Daslang automatic binder
// all user modifications will be lost after this file is re-generated

options gen2
module llvm_struct shared
typedef LLVMBool = int
struct LLVMOpaqueMemoryBuffer {}
typedef LLVMMemoryBufferRef = LLVMOpaqueMemoryBuffer?
struct LLVMOpaqueContext {}
typedef LLVMContextRef = LLVMOpaqueContext?
struct LLVMOpaqueModule {}
typedef LLVMModuleRef = LLVMOpaqueModule?
struct LLVMOpaqueType {}
typedef LLVMTypeRef = LLVMOpaqueType?
struct LLVMOpaqueValue {}
typedef LLVMValueRef = LLVMOpaqueValue?
struct LLVMOpaqueBasicBlock {}
typedef LLVMBasicBlockRef = LLVMOpaqueBasicBlock?
struct LLVMOpaqueMetadata {}
typedef LLVMMetadataRef = LLVMOpaqueMetadata?
struct LLVMOpaqueNamedMDNode {}
typedef LLVMNamedMDNodeRef = LLVMOpaqueNamedMDNode?
struct LLVMOpaqueValueMetadataEntry {}
typedef LLVMValueMetadataEntry = LLVMOpaqueValueMetadataEntry
struct LLVMOpaqueBuilder {}
typedef LLVMBuilderRef = LLVMOpaqueBuilder?
struct LLVMOpaqueDIBuilder {}
typedef LLVMDIBuilderRef = LLVMOpaqueDIBuilder?
struct LLVMOpaqueModuleProvider {}
typedef LLVMModuleProviderRef = LLVMOpaqueModuleProvider?
struct LLVMOpaquePassManager {}
typedef LLVMPassManagerRef = LLVMOpaquePassManager?
struct LLVMOpaquePassRegistry {}
typedef LLVMPassRegistryRef = LLVMOpaquePassRegistry?
struct LLVMOpaqueUse {}
typedef LLVMUseRef = LLVMOpaqueUse?
struct LLVMOpaqueAttributeRef {}
typedef LLVMAttributeRef = LLVMOpaqueAttributeRef?
struct LLVMOpaqueDiagnosticInfo {}
typedef LLVMDiagnosticInfoRef = LLVMOpaqueDiagnosticInfo?
struct LLVMComdat {}
typedef LLVMComdatRef = LLVMComdat?
struct LLVMOpaqueModuleFlagEntry {}
typedef LLVMModuleFlagEntry = LLVMOpaqueModuleFlagEntry
struct LLVMOpaqueJITEventListener {}
typedef LLVMJITEventListenerRef = LLVMOpaqueJITEventListener?
struct LLVMOpaqueBinary {}
typedef LLVMBinaryRef = LLVMOpaqueBinary?
typedef LLVMFatalErrorHandler = void ??
typedef LLVMAttributeIndex = uint
typedef LLVMDiagnosticHandler = void ??
typedef LLVMYieldCallback = void ??
typedef LLVMMetadataKind = uint
typedef LLVMDWARFTypeEncoding = uint
typedef LLVMDisasmContextRef = void?
typedef LLVMOpInfoCallback = void ??
struct LLVMOpInfoSymbol1 {}
struct LLVMOpInfo1 {}
typedef LLVMSymbolLookupCallback = void ??
struct LLVMOpaqueError {}
typedef LLVMErrorRef = LLVMOpaqueError?
typedef LLVMErrorTypeId = void const?
struct LLVMOpaqueTargetData {}
typedef LLVMTargetDataRef = LLVMOpaqueTargetData?
struct LLVMOpaqueTargetLibraryInfotData {}
typedef LLVMTargetLibraryInfoRef = LLVMOpaqueTargetLibraryInfotData?
struct LLVMOpaqueTargetMachine {}
typedef LLVMTargetMachineRef = LLVMOpaqueTargetMachine?
struct LLVMTarget {}
typedef LLVMTargetRef = LLVMTarget?
struct LLVMOpaqueGenericValue {}
typedef LLVMGenericValueRef = LLVMOpaqueGenericValue?
struct LLVMOpaqueExecutionEngine {}
typedef LLVMExecutionEngineRef = LLVMOpaqueExecutionEngine?
struct LLVMOpaqueMCJITMemoryManager {}
typedef LLVMMCJITMemoryManagerRef = LLVMOpaqueMCJITMemoryManager?
struct LLVMMCJITCompilerOptions {}
typedef LLVMMemoryManagerAllocateCodeSectionCallback = void ??
typedef LLVMMemoryManagerAllocateDataSectionCallback = void ??
typedef LLVMMemoryManagerFinalizeMemoryCallback = void ??
typedef LLVMMemoryManagerDestroyCallback = void ??
typedef LLVMOrcJITTargetAddress = uint64
typedef LLVMOrcExecutorAddress = uint64
typedef LLVMJITSymbolTargetFlags = uint8
struct LLVMJITSymbolFlags {}
struct LLVMJITEvaluatedSymbol {}
struct LLVMOrcOpaqueExecutionSession {}
typedef LLVMOrcExecutionSessionRef = LLVMOrcOpaqueExecutionSession?
typedef LLVMOrcErrorReporterFunction = void ??
struct LLVMOrcOpaqueSymbolStringPool {}
typedef LLVMOrcSymbolStringPoolRef = LLVMOrcOpaqueSymbolStringPool?
struct LLVMOrcOpaqueSymbolStringPoolEntry {}
typedef LLVMOrcSymbolStringPoolEntryRef = LLVMOrcOpaqueSymbolStringPoolEntry?
struct LLVMOrcCSymbolFlagsMapPair {}
typedef LLVMOrcCSymbolFlagsMapPairs = LLVMOrcCSymbolFlagsMapPair?
struct LLVMOrcCSymbolMapPair {}
typedef LLVMOrcCSymbolMapPairs = LLVMOrcCSymbolMapPair?
struct LLVMOrcCSymbolAliasMapEntry {}
struct LLVMOrcCSymbolAliasMapPair {}
typedef LLVMOrcCSymbolAliasMapPairs = LLVMOrcCSymbolAliasMapPair?
struct LLVMOrcOpaqueJITDylib {}
typedef LLVMOrcJITDylibRef = LLVMOrcOpaqueJITDylib?
struct LLVMOrcCSymbolsList {}
struct LLVMOrcCDependenceMapPair {}
typedef LLVMOrcCDependenceMapPairs = LLVMOrcCDependenceMapPair?
struct LLVMOrcCJITDylibSearchOrderElement {}
typedef LLVMOrcCJITDylibSearchOrder = LLVMOrcCJITDylibSearchOrderElement?
struct LLVMOrcCLookupSetElement {}
typedef LLVMOrcCLookupSet = LLVMOrcCLookupSetElement?
struct LLVMOrcOpaqueMaterializationUnit {}
typedef LLVMOrcMaterializationUnitRef = LLVMOrcOpaqueMaterializationUnit?
struct LLVMOrcOpaqueMaterializationResponsibility {}
typedef LLVMOrcMaterializationResponsibilityRef = LLVMOrcOpaqueMaterializationResponsibility?
typedef LLVMOrcMaterializationUnitMaterializeFunction = void ??
typedef LLVMOrcMaterializationUnitDiscardFunction = void ??
typedef LLVMOrcMaterializationUnitDestroyFunction = void ??
struct LLVMOrcOpaqueResourceTracker {}
typedef LLVMOrcResourceTrackerRef = LLVMOrcOpaqueResourceTracker?
struct LLVMOrcOpaqueDefinitionGenerator {}
typedef LLVMOrcDefinitionGeneratorRef = LLVMOrcOpaqueDefinitionGenerator?
struct LLVMOrcOpaqueLookupState {}
typedef LLVMOrcLookupStateRef = LLVMOrcOpaqueLookupState?
typedef LLVMOrcCAPIDefinitionGeneratorTryToGenerateFunction = void ??
typedef LLVMOrcDisposeCAPIDefinitionGeneratorFunction = void ??
typedef LLVMOrcSymbolPredicate = void ??
struct LLVMOrcOpaqueThreadSafeContext {}
typedef LLVMOrcThreadSafeContextRef = LLVMOrcOpaqueThreadSafeContext?
struct LLVMOrcOpaqueThreadSafeModule {}
typedef LLVMOrcThreadSafeModuleRef = LLVMOrcOpaqueThreadSafeModule?
typedef LLVMOrcGenericIRModuleOperationFunction = void ??
struct LLVMOrcOpaqueJITTargetMachineBuilder {}
typedef LLVMOrcJITTargetMachineBuilderRef = LLVMOrcOpaqueJITTargetMachineBuilder?
struct LLVMOrcOpaqueObjectLayer {}
typedef LLVMOrcObjectLayerRef = LLVMOrcOpaqueObjectLayer?
struct LLVMOrcOpaqueObjectLinkingLayer {}
typedef LLVMOrcObjectLinkingLayerRef = LLVMOrcOpaqueObjectLinkingLayer?
struct LLVMOrcOpaqueIRTransformLayer {}
typedef LLVMOrcIRTransformLayerRef = LLVMOrcOpaqueIRTransformLayer?
typedef LLVMOrcIRTransformLayerTransformFunction = void ??
struct LLVMOrcOpaqueObjectTransformLayer {}
typedef LLVMOrcObjectTransformLayerRef = LLVMOrcOpaqueObjectTransformLayer?
typedef LLVMOrcObjectTransformLayerTransformFunction = void ??
struct LLVMOrcOpaqueIndirectStubsManager {}
typedef LLVMOrcIndirectStubsManagerRef = LLVMOrcOpaqueIndirectStubsManager?
struct LLVMOrcOpaqueLazyCallThroughManager {}
typedef LLVMOrcLazyCallThroughManagerRef = LLVMOrcOpaqueLazyCallThroughManager?
struct LLVMOrcOpaqueDumpObjects {}
typedef LLVMOrcDumpObjectsRef = LLVMOrcOpaqueDumpObjects?
typedef LLVMOrcExecutionSessionLookupHandleResultFunction = void ??
typedef LLVMOrcLLJITBuilderObjectLinkingLayerCreatorFunction = void ??
struct LLVMOrcOpaqueLLJITBuilder {}
typedef LLVMOrcLLJITBuilderRef = LLVMOrcOpaqueLLJITBuilder?
struct LLVMOrcOpaqueLLJIT {}
typedef LLVMOrcLLJITRef = LLVMOrcOpaqueLLJIT?
typedef lto_bool_t = bool
struct LLVMOpaqueLTOModule {}
typedef lto_module_t = LLVMOpaqueLTOModule?
struct LLVMOpaqueLTOCodeGenerator {}
typedef lto_code_gen_t = LLVMOpaqueLTOCodeGenerator?
struct LLVMOpaqueThinLTOCodeGenerator {}
typedef thinlto_code_gen_t = LLVMOpaqueThinLTOCodeGenerator?
typedef lto_diagnostic_handler_t = void ??
struct LLVMOpaqueLTOInput {}
typedef lto_input_t = LLVMOpaqueLTOInput?
struct LTOObjectBuffer {}
struct LLVMOpaqueSectionIterator {}
typedef LLVMSectionIteratorRef = LLVMOpaqueSectionIterator?
struct LLVMOpaqueSymbolIterator {}
typedef LLVMSymbolIteratorRef = LLVMOpaqueSymbolIterator?
struct LLVMOpaqueRelocationIterator {}
typedef LLVMRelocationIteratorRef = LLVMOpaqueRelocationIterator?
struct LLVMOpaqueObjectFile {}
typedef LLVMObjectFileRef = LLVMOpaqueObjectFile?
typedef LLVMMemoryManagerCreateContextCallback = void ??
typedef LLVMMemoryManagerNotifyTerminatingCallback = void ??
struct LLVMRemarkOpaqueString {}
typedef LLVMRemarkStringRef = LLVMRemarkOpaqueString?
struct LLVMRemarkOpaqueDebugLoc {}
typedef LLVMRemarkDebugLocRef = LLVMRemarkOpaqueDebugLoc?
struct LLVMRemarkOpaqueArg {}
typedef LLVMRemarkArgRef = LLVMRemarkOpaqueArg?
struct LLVMRemarkOpaqueEntry {}
typedef LLVMRemarkEntryRef = LLVMRemarkOpaqueEntry?
struct LLVMRemarkOpaqueParser {}
typedef LLVMRemarkParserRef = LLVMRemarkOpaqueParser?
struct LLVMOpaquePassBuilderOptions {}
typedef LLVMPassBuilderOptionsRef = LLVMOpaquePassBuilderOptions?
struct LLVMOpaquePassManagerBuilder {}
typedef LLVMPassManagerBuilderRef = LLVMOpaquePassManagerBuilder?
