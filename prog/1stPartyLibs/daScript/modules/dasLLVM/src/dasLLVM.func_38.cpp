// this file is generated via daScript automatic C++ binder
// all user modifications will be lost after this file is re-generated

#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/bind_enum.h"
#include "dasLLVM.h"
#include "need_dasLLVM.h"
namespace das {
#include "dasLLVM.func.aot.decl.inc"
void Module_dasLLVM::initFunctions_38() {
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:312:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,const char *,size_t,const char *,size_t,const char *,size_t) , LLVMDIBuilderCreateModule >(*this,lib,"LLVMDIBuilderCreateModule",SideEffects::worstDefault,"LLVMDIBuilderCreateModule")
		->args({"Builder","ParentScope","Name","NameLen","ConfigMacros","ConfigMacrosLen","IncludePath","IncludePathLen","APINotesFile","APINotesFileLen"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:328:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,int) , LLVMDIBuilderCreateNameSpace >(*this,lib,"LLVMDIBuilderCreateNameSpace",SideEffects::worstDefault,"LLVMDIBuilderCreateNameSpace")
		->args({"Builder","ParentScope","Name","NameLen","ExportSymbols"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:351:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata *,int,int,unsigned int,LLVMDIFlags,int) , LLVMDIBuilderCreateFunction >(*this,lib,"LLVMDIBuilderCreateFunction",SideEffects::worstDefault,"LLVMDIBuilderCreateFunction")
		->args({"Builder","Scope","Name","NameLen","LinkageName","LinkageNameLen","File","LineNo","Ty","IsLocalToUnit","IsDefinition","ScopeLine","Flags","IsOptimized"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:366:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int,unsigned int) , LLVMDIBuilderCreateLexicalBlock >(*this,lib,"LLVMDIBuilderCreateLexicalBlock",SideEffects::worstDefault,"LLVMDIBuilderCreateLexicalBlock")
		->args({"Builder","Scope","File","Line","Column"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:378:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int) , LLVMDIBuilderCreateLexicalBlockFile >(*this,lib,"LLVMDIBuilderCreateLexicalBlockFile",SideEffects::worstDefault,"LLVMDIBuilderCreateLexicalBlockFile")
		->args({"Builder","Scope","File","Discriminator"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:392:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int) , LLVMDIBuilderCreateImportedModuleFromNamespace >(*this,lib,"LLVMDIBuilderCreateImportedModuleFromNamespace",SideEffects::worstDefault,"LLVMDIBuilderCreateImportedModuleFromNamespace")
		->args({"Builder","Scope","NS","File","Line"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:409:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata **,unsigned int) , LLVMDIBuilderCreateImportedModuleFromAlias >(*this,lib,"LLVMDIBuilderCreateImportedModuleFromAlias",SideEffects::worstDefault,"LLVMDIBuilderCreateImportedModuleFromAlias")
		->args({"Builder","Scope","ImportedEntity","File","Line","Elements","NumElements"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:424:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata **,unsigned int) , LLVMDIBuilderCreateImportedModuleFromModule >(*this,lib,"LLVMDIBuilderCreateImportedModuleFromModule",SideEffects::worstDefault,"LLVMDIBuilderCreateImportedModuleFromModule")
		->args({"Builder","Scope","M","File","Line","Elements","NumElements"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:444:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int,const char *,size_t,LLVMOpaqueMetadata **,unsigned int) , LLVMDIBuilderCreateImportedDeclaration >(*this,lib,"LLVMDIBuilderCreateImportedDeclaration",SideEffects::worstDefault,"LLVMDIBuilderCreateImportedDeclaration")
		->args({"Builder","Scope","Decl","File","Line","Name","NameLen","Elements","NumElements"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:460:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueContext *,unsigned int,unsigned int,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateDebugLocation >(*this,lib,"LLVMDIBuilderCreateDebugLocation",SideEffects::worstDefault,"LLVMDIBuilderCreateDebugLocation")
		->args({"Ctx","Line","Column","Scope","InlinedAt"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:470:10
	addExtern< unsigned int (*)(LLVMOpaqueMetadata *) , LLVMDILocationGetLine >(*this,lib,"LLVMDILocationGetLine",SideEffects::worstDefault,"LLVMDILocationGetLine")
		->args({"Location"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:478:10
	addExtern< unsigned int (*)(LLVMOpaqueMetadata *) , LLVMDILocationGetColumn >(*this,lib,"LLVMDILocationGetColumn",SideEffects::worstDefault,"LLVMDILocationGetColumn")
		->args({"Location"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:486:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueMetadata *) , LLVMDILocationGetScope >(*this,lib,"LLVMDILocationGetScope",SideEffects::worstDefault,"LLVMDILocationGetScope")
		->args({"Location"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:494:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueMetadata *) , LLVMDILocationGetInlinedAt >(*this,lib,"LLVMDILocationGetInlinedAt",SideEffects::worstDefault,"LLVMDILocationGetInlinedAt")
		->args({"Location"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:502:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueMetadata *) , LLVMDIScopeGetFile >(*this,lib,"LLVMDIScopeGetFile",SideEffects::worstDefault,"LLVMDIScopeGetFile")
		->args({"Scope"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:511:13
	addExtern< const char * (*)(LLVMOpaqueMetadata *,unsigned int *) , LLVMDIFileGetDirectory >(*this,lib,"LLVMDIFileGetDirectory",SideEffects::worstDefault,"LLVMDIFileGetDirectory")
		->args({"File","Len"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:520:13
	addExtern< const char * (*)(LLVMOpaqueMetadata *,unsigned int *) , LLVMDIFileGetFilename >(*this,lib,"LLVMDIFileGetFilename",SideEffects::worstDefault,"LLVMDIFileGetFilename")
		->args({"File","Len"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:529:13
	addExtern< const char * (*)(LLVMOpaqueMetadata *,unsigned int *) , LLVMDIFileGetSource >(*this,lib,"LLVMDIFileGetSource",SideEffects::worstDefault,"LLVMDIFileGetSource")
		->args({"File","Len"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:537:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata **,size_t) , LLVMDIBuilderGetOrCreateTypeArray >(*this,lib,"LLVMDIBuilderGetOrCreateTypeArray",SideEffects::worstDefault,"LLVMDIBuilderGetOrCreateTypeArray")
		->args({"Builder","Data","NumElements"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:552:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata **,unsigned int,LLVMDIFlags) , LLVMDIBuilderCreateSubroutineType >(*this,lib,"LLVMDIBuilderCreateSubroutineType",SideEffects::worstDefault,"LLVMDIBuilderCreateSubroutineType")
		->args({"Builder","File","ParameterTypes","NumParameterTypes","Flags"});
}
}

