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
void Module_dasLLVM::initFunctions_37() {
// from D:\Work\libclang\include\llvm-c/Core.h:4221:10
	makeExtern< int (*)() , LLVMIsMultithreaded , SimNode_ExtFuncCall >(lib,"LLVMIsMultithreaded","LLVMIsMultithreaded")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:207:10
	makeExtern< unsigned int (*)() , LLVMDebugMetadataVersion , SimNode_ExtFuncCall >(lib,"LLVMDebugMetadataVersion","LLVMDebugMetadataVersion")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:212:10
	makeExtern< unsigned int (*)(LLVMOpaqueModule *) , LLVMGetModuleDebugMetadataVersion , SimNode_ExtFuncCall >(lib,"LLVMGetModuleDebugMetadataVersion","LLVMGetModuleDebugMetadataVersion")
		->args({"Module"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:220:10
	makeExtern< int (*)(LLVMOpaqueModule *) , LLVMStripModuleDebugInfo , SimNode_ExtFuncCall >(lib,"LLVMStripModuleDebugInfo","LLVMStripModuleDebugInfo")
		->args({"Module"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:226:18
	makeExtern< LLVMOpaqueDIBuilder * (*)(LLVMOpaqueModule *) , LLVMCreateDIBuilderDisallowUnresolved , SimNode_ExtFuncCall >(lib,"LLVMCreateDIBuilderDisallowUnresolved","LLVMCreateDIBuilderDisallowUnresolved")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:233:18
	makeExtern< LLVMOpaqueDIBuilder * (*)(LLVMOpaqueModule *) , LLVMCreateDIBuilder , SimNode_ExtFuncCall >(lib,"LLVMCreateDIBuilder","LLVMCreateDIBuilder")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:239:6
	makeExtern< void (*)(LLVMOpaqueDIBuilder *) , LLVMDisposeDIBuilder , SimNode_ExtFuncCall >(lib,"LLVMDisposeDIBuilder","LLVMDisposeDIBuilder")
		->args({"Builder"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:244:6
	makeExtern< void (*)(LLVMOpaqueDIBuilder *) , LLVMDIBuilderFinalize , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderFinalize","LLVMDIBuilderFinalize")
		->args({"Builder"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:250:6
	makeExtern< void (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *) , LLVMDIBuilderFinalizeSubprogram , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderFinalizeSubprogram","LLVMDIBuilderFinalizeSubprogram")
		->args({"Builder","Subprogram"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:285:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMDWARFSourceLanguage,LLVMOpaqueMetadata *,const char *,size_t,int,const char *,size_t,unsigned int,const char *,size_t,LLVMDWARFEmissionKind,unsigned int,int,int,const char *,size_t,const char *,size_t) , LLVMDIBuilderCreateCompileUnit , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateCompileUnit","LLVMDIBuilderCreateCompileUnit")
		->args({"Builder","Lang","FileRef","Producer","ProducerLen","isOptimized","Flags","FlagsLen","RuntimeVer","SplitName","SplitNameLen","Kind","DWOId","SplitDebugInlining","DebugInfoForProfiling","SysRoot","SysRootLen","SDK","SDKLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:303:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,const char *,size_t,const char *,size_t) , LLVMDIBuilderCreateFile , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateFile","LLVMDIBuilderCreateFile")
		->args({"Builder","Filename","FilenameLen","Directory","DirectoryLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:322:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,const char *,size_t,const char *,size_t,const char *,size_t) , LLVMDIBuilderCreateModule , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateModule","LLVMDIBuilderCreateModule")
		->args({"Builder","ParentScope","Name","NameLen","ConfigMacros","ConfigMacrosLen","IncludePath","IncludePathLen","APINotesFile","APINotesFileLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:338:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,int) , LLVMDIBuilderCreateNameSpace , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateNameSpace","LLVMDIBuilderCreateNameSpace")
		->args({"Builder","ParentScope","Name","NameLen","ExportSymbols"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:361:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata *,int,int,unsigned int,LLVMDIFlags,int) , LLVMDIBuilderCreateFunction , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateFunction","LLVMDIBuilderCreateFunction")
		->args({"Builder","Scope","Name","NameLen","LinkageName","LinkageNameLen","File","LineNo","Ty","IsLocalToUnit","IsDefinition","ScopeLine","Flags","IsOptimized"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:376:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int,unsigned int) , LLVMDIBuilderCreateLexicalBlock , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateLexicalBlock","LLVMDIBuilderCreateLexicalBlock")
		->args({"Builder","Scope","File","Line","Column"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:388:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int) , LLVMDIBuilderCreateLexicalBlockFile , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateLexicalBlockFile","LLVMDIBuilderCreateLexicalBlockFile")
		->args({"Builder","Scope","File","Discriminator"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:402:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int) , LLVMDIBuilderCreateImportedModuleFromNamespace , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateImportedModuleFromNamespace","LLVMDIBuilderCreateImportedModuleFromNamespace")
		->args({"Builder","Scope","NS","File","Line"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:419:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata **,unsigned int) , LLVMDIBuilderCreateImportedModuleFromAlias , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateImportedModuleFromAlias","LLVMDIBuilderCreateImportedModuleFromAlias")
		->args({"Builder","Scope","ImportedEntity","File","Line","Elements","NumElements"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:434:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata **,unsigned int) , LLVMDIBuilderCreateImportedModuleFromModule , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateImportedModuleFromModule","LLVMDIBuilderCreateImportedModuleFromModule")
		->args({"Builder","Scope","M","File","Line","Elements","NumElements"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:454:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int,const char *,size_t,LLVMOpaqueMetadata **,unsigned int) , LLVMDIBuilderCreateImportedDeclaration , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateImportedDeclaration","LLVMDIBuilderCreateImportedDeclaration")
		->args({"Builder","Scope","Decl","File","Line","Name","NameLen","Elements","NumElements"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

