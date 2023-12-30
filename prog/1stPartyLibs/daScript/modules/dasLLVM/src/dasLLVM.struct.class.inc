// this file is generated via daScript automatic C++ binder
// all user modifications will be lost after this file is re-generated

// from D:\Work\libclang\include\llvm-c/DisassemblerTypes.h:72:8
struct LLVMOpInfoSymbol1_GeneratedAnnotation : ManagedStructureAnnotation<LLVMOpInfoSymbol1> {
	LLVMOpInfoSymbol1_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LLVMOpInfoSymbol1", ml, "LLVMOpInfoSymbol1") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Present)>("Present","Present");
		addField<DAS_BIND_MANAGED_FIELD(Name)>("Name","Name");
		addField<DAS_BIND_MANAGED_FIELD(Value)>("Value","Value");
	}
};

// from D:\Work\libclang\include\llvm-c/DisassemblerTypes.h:78:8
struct LLVMOpInfo1_GeneratedAnnotation : ManagedStructureAnnotation<LLVMOpInfo1> {
	LLVMOpInfo1_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LLVMOpInfo1", ml, "LLVMOpInfo1") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(AddSymbol)>("AddSymbol","AddSymbol");
		addField<DAS_BIND_MANAGED_FIELD(SubtractSymbol)>("SubtractSymbol","SubtractSymbol");
		addField<DAS_BIND_MANAGED_FIELD(Value)>("Value","Value");
		addField<DAS_BIND_MANAGED_FIELD(VariantKind)>("VariantKind","VariantKind");
	}
};

// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:43:8
struct LLVMMCJITCompilerOptions_GeneratedAnnotation : ManagedStructureAnnotation<LLVMMCJITCompilerOptions> {
	LLVMMCJITCompilerOptions_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LLVMMCJITCompilerOptions", ml, "LLVMMCJITCompilerOptions") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(OptLevel)>("OptLevel","OptLevel");
		addField<DAS_BIND_MANAGED_FIELD(CodeModel)>("CodeModel","CodeModel");
		addField<DAS_BIND_MANAGED_FIELD(NoFramePointerElim)>("NoFramePointerElim","NoFramePointerElim");
		addField<DAS_BIND_MANAGED_FIELD(EnableFastISel)>("EnableFastISel","EnableFastISel");
		addField<DAS_BIND_MANAGED_FIELD(MCJMM)>("MCJMM","MCJMM");
	}
};

// from D:\Work\libclang\include\llvm-c/Orc.h:72:9
struct LLVMJITSymbolFlags_GeneratedAnnotation : ManagedStructureAnnotation<LLVMJITSymbolFlags> {
	LLVMJITSymbolFlags_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LLVMJITSymbolFlags", ml, "LLVMJITSymbolFlags") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(GenericFlags)>("GenericFlags","GenericFlags");
		addField<DAS_BIND_MANAGED_FIELD(TargetFlags)>("TargetFlags","TargetFlags");
	}
};

// from D:\Work\libclang\include\llvm-c/Orc.h:80:9
struct LLVMJITEvaluatedSymbol_GeneratedAnnotation : ManagedStructureAnnotation<LLVMJITEvaluatedSymbol> {
	LLVMJITEvaluatedSymbol_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LLVMJITEvaluatedSymbol", ml, "LLVMJITEvaluatedSymbol") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Address)>("Address","Address");
		addField<DAS_BIND_MANAGED_FIELD(Flags)>("Flags","Flags");
	}
};

// from D:\Work\libclang\include\llvm-c/Orc.h:109:9
struct LLVMOrcCSymbolFlagsMapPair_GeneratedAnnotation : ManagedStructureAnnotation<LLVMOrcCSymbolFlagsMapPair> {
	LLVMOrcCSymbolFlagsMapPair_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LLVMOrcCSymbolFlagsMapPair", ml, "LLVMOrcCSymbolFlagsMapPair") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Name)>("Name","Name");
		addField<DAS_BIND_MANAGED_FIELD(Flags)>("Flags","Flags");
	}
};

// from D:\Work\libclang\include\llvm-c/Orc.h:123:9
struct LLVMOrcCSymbolMapPair_GeneratedAnnotation : ManagedStructureAnnotation<LLVMOrcCSymbolMapPair> {
	LLVMOrcCSymbolMapPair_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LLVMOrcCSymbolMapPair", ml, "LLVMOrcCSymbolMapPair") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Name)>("Name","Name");
		addField<DAS_BIND_MANAGED_FIELD(Sym)>("Sym","Sym");
	}
};

// from D:\Work\libclang\include\llvm-c/Orc.h:137:9
struct LLVMOrcCSymbolAliasMapEntry_GeneratedAnnotation : ManagedStructureAnnotation<LLVMOrcCSymbolAliasMapEntry> {
	LLVMOrcCSymbolAliasMapEntry_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LLVMOrcCSymbolAliasMapEntry", ml, "LLVMOrcCSymbolAliasMapEntry") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Name)>("Name","Name");
		addField<DAS_BIND_MANAGED_FIELD(Flags)>("Flags","Flags");
	}
};

// from D:\Work\libclang\include\llvm-c/Orc.h:145:9
struct LLVMOrcCSymbolAliasMapPair_GeneratedAnnotation : ManagedStructureAnnotation<LLVMOrcCSymbolAliasMapPair> {
	LLVMOrcCSymbolAliasMapPair_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LLVMOrcCSymbolAliasMapPair", ml, "LLVMOrcCSymbolAliasMapPair") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Name)>("Name","Name");
		addField<DAS_BIND_MANAGED_FIELD(Entry)>("Entry","Entry");
	}
};

// from D:\Work\libclang\include\llvm-c/Orc.h:165:9
struct LLVMOrcCSymbolsList_GeneratedAnnotation : ManagedStructureAnnotation<LLVMOrcCSymbolsList> {
	LLVMOrcCSymbolsList_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LLVMOrcCSymbolsList", ml, "LLVMOrcCSymbolsList") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Symbols)>("Symbols","Symbols");
		addField<DAS_BIND_MANAGED_FIELD(Length)>("Length","Length");
	}
};

// from D:\Work\libclang\include\llvm-c/Orc.h:173:9
struct LLVMOrcCDependenceMapPair_GeneratedAnnotation : ManagedStructureAnnotation<LLVMOrcCDependenceMapPair> {
	LLVMOrcCDependenceMapPair_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LLVMOrcCDependenceMapPair", ml, "LLVMOrcCDependenceMapPair") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(JD)>("JD","JD");
		addField<DAS_BIND_MANAGED_FIELD(Names)>("Names","Names");
	}
};

// from D:\Work\libclang\include\llvm-c/Orc.h:209:9
struct LLVMOrcCJITDylibSearchOrderElement_GeneratedAnnotation : ManagedStructureAnnotation<LLVMOrcCJITDylibSearchOrderElement> {
	LLVMOrcCJITDylibSearchOrderElement_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LLVMOrcCJITDylibSearchOrderElement", ml, "LLVMOrcCJITDylibSearchOrderElement") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(JD)>("JD","JD");
		addField<DAS_BIND_MANAGED_FIELD(JDLookupFlags)>("JDLookupFlags","JDLookupFlags");
	}
};

// from D:\Work\libclang\include\llvm-c/Orc.h:234:9
struct LLVMOrcCLookupSetElement_GeneratedAnnotation : ManagedStructureAnnotation<LLVMOrcCLookupSetElement> {
	LLVMOrcCLookupSetElement_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LLVMOrcCLookupSetElement", ml, "LLVMOrcCLookupSetElement") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Name)>("Name","Name");
		addField<DAS_BIND_MANAGED_FIELD(LookupFlags)>("LookupFlags","LookupFlags");
	}
};

// from D:\Work\libclang\include\llvm-c/lto.h:670:9
struct LTOObjectBuffer_GeneratedAnnotation : ManagedStructureAnnotation<LTOObjectBuffer> {
	LTOObjectBuffer_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LTOObjectBuffer", ml, "LTOObjectBuffer") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Buffer)>("Buffer","Buffer");
		addField<DAS_BIND_MANAGED_FIELD(Size)>("Size","Size");
	}
};

