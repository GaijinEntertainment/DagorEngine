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
// from D:\Work\libclang\include\llvm-c/Core.h:4194:20
	addExtern< LLVMOpaquePassManager * (*)(LLVMOpaqueModule *) , LLVMCreateFunctionPassManagerForModule >(*this,lib,"LLVMCreateFunctionPassManagerForModule",SideEffects::worstDefault,"LLVMCreateFunctionPassManagerForModule")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:4197:20
	addExtern< LLVMOpaquePassManager * (*)(LLVMOpaqueModuleProvider *) , LLVMCreateFunctionPassManager >(*this,lib,"LLVMCreateFunctionPassManager",SideEffects::worstDefault,"LLVMCreateFunctionPassManager")
		->args({"MP"});
// from D:\Work\libclang\include\llvm-c/Core.h:4203:10
	addExtern< int (*)(LLVMOpaquePassManager *,LLVMOpaqueModule *) , LLVMRunPassManager >(*this,lib,"LLVMRunPassManager",SideEffects::worstDefault,"LLVMRunPassManager")
		->args({"PM","M"});
// from D:\Work\libclang\include\llvm-c/Core.h:4208:10
	addExtern< int (*)(LLVMOpaquePassManager *) , LLVMInitializeFunctionPassManager >(*this,lib,"LLVMInitializeFunctionPassManager",SideEffects::worstDefault,"LLVMInitializeFunctionPassManager")
		->args({"FPM"});
// from D:\Work\libclang\include\llvm-c/Core.h:4214:10
	addExtern< int (*)(LLVMOpaquePassManager *,LLVMOpaqueValue *) , LLVMRunFunctionPassManager >(*this,lib,"LLVMRunFunctionPassManager",SideEffects::worstDefault,"LLVMRunFunctionPassManager")
		->args({"FPM","F"});
// from D:\Work\libclang\include\llvm-c/Core.h:4219:10
	addExtern< int (*)(LLVMOpaquePassManager *) , LLVMFinalizeFunctionPassManager >(*this,lib,"LLVMFinalizeFunctionPassManager",SideEffects::worstDefault,"LLVMFinalizeFunctionPassManager")
		->args({"FPM"});
// from D:\Work\libclang\include\llvm-c/Core.h:4224:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMDisposePassManager >(*this,lib,"LLVMDisposePassManager",SideEffects::worstDefault,"LLVMDisposePassManager")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Core.h:4241:10
	addExtern< int (*)() , LLVMStartMultithreaded >(*this,lib,"LLVMStartMultithreaded",SideEffects::worstDefault,"LLVMStartMultithreaded");
// from D:\Work\libclang\include\llvm-c/Core.h:4245:6
	addExtern< void (*)() , LLVMStopMultithreaded >(*this,lib,"LLVMStopMultithreaded",SideEffects::worstDefault,"LLVMStopMultithreaded");
// from D:\Work\libclang\include\llvm-c/Core.h:4249:10
	addExtern< int (*)() , LLVMIsMultithreaded >(*this,lib,"LLVMIsMultithreaded",SideEffects::worstDefault,"LLVMIsMultithreaded");
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:197:10
	addExtern< unsigned int (*)() , LLVMDebugMetadataVersion >(*this,lib,"LLVMDebugMetadataVersion",SideEffects::worstDefault,"LLVMDebugMetadataVersion");
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:202:10
	addExtern< unsigned int (*)(LLVMOpaqueModule *) , LLVMGetModuleDebugMetadataVersion >(*this,lib,"LLVMGetModuleDebugMetadataVersion",SideEffects::worstDefault,"LLVMGetModuleDebugMetadataVersion")
		->args({"Module"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:210:10
	addExtern< int (*)(LLVMOpaqueModule *) , LLVMStripModuleDebugInfo >(*this,lib,"LLVMStripModuleDebugInfo",SideEffects::worstDefault,"LLVMStripModuleDebugInfo")
		->args({"Module"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:216:18
	addExtern< LLVMOpaqueDIBuilder * (*)(LLVMOpaqueModule *) , LLVMCreateDIBuilderDisallowUnresolved >(*this,lib,"LLVMCreateDIBuilderDisallowUnresolved",SideEffects::worstDefault,"LLVMCreateDIBuilderDisallowUnresolved")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:223:18
	addExtern< LLVMOpaqueDIBuilder * (*)(LLVMOpaqueModule *) , LLVMCreateDIBuilder >(*this,lib,"LLVMCreateDIBuilder",SideEffects::worstDefault,"LLVMCreateDIBuilder")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:229:6
	addExtern< void (*)(LLVMOpaqueDIBuilder *) , LLVMDisposeDIBuilder >(*this,lib,"LLVMDisposeDIBuilder",SideEffects::worstDefault,"LLVMDisposeDIBuilder")
		->args({"Builder"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:234:6
	addExtern< void (*)(LLVMOpaqueDIBuilder *) , LLVMDIBuilderFinalize >(*this,lib,"LLVMDIBuilderFinalize",SideEffects::worstDefault,"LLVMDIBuilderFinalize")
		->args({"Builder"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:240:6
	addExtern< void (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *) , LLVMDIBuilderFinalizeSubprogram >(*this,lib,"LLVMDIBuilderFinalizeSubprogram",SideEffects::worstDefault,"LLVMDIBuilderFinalizeSubprogram")
		->args({"Builder","Subprogram"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:275:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMDWARFSourceLanguage,LLVMOpaqueMetadata *,const char *,size_t,int,const char *,size_t,unsigned int,const char *,size_t,LLVMDWARFEmissionKind,unsigned int,int,int,const char *,size_t,const char *,size_t) , LLVMDIBuilderCreateCompileUnit >(*this,lib,"LLVMDIBuilderCreateCompileUnit",SideEffects::worstDefault,"LLVMDIBuilderCreateCompileUnit")
		->args({"Builder","Lang","FileRef","Producer","ProducerLen","isOptimized","Flags","FlagsLen","RuntimeVer","SplitName","SplitNameLen","Kind","DWOId","SplitDebugInlining","DebugInfoForProfiling","SysRoot","SysRootLen","SDK","SDKLen"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:293:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,const char *,size_t,const char *,size_t) , LLVMDIBuilderCreateFile >(*this,lib,"LLVMDIBuilderCreateFile",SideEffects::worstDefault,"LLVMDIBuilderCreateFile")
		->args({"Builder","Filename","FilenameLen","Directory","DirectoryLen"});
}
}

