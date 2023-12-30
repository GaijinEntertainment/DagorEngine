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
void Module_dasLLVM::initFunctions_49() {
// from D:\Work\libclang\include\llvm-c/Target.h:265:10
	addExtern< unsigned int (*)(LLVMOpaqueTargetData *,LLVMOpaqueType *) , LLVMCallFrameAlignmentOfType >(*this,lib,"LLVMCallFrameAlignmentOfType",SideEffects::worstDefault,"LLVMCallFrameAlignmentOfType")
		->args({"TD","Ty"});
// from D:\Work\libclang\include\llvm-c/Target.h:269:10
	addExtern< unsigned int (*)(LLVMOpaqueTargetData *,LLVMOpaqueType *) , LLVMPreferredAlignmentOfType >(*this,lib,"LLVMPreferredAlignmentOfType",SideEffects::worstDefault,"LLVMPreferredAlignmentOfType")
		->args({"TD","Ty"});
// from D:\Work\libclang\include\llvm-c/Target.h:273:10
	addExtern< unsigned int (*)(LLVMOpaqueTargetData *,LLVMOpaqueValue *) , LLVMPreferredAlignmentOfGlobal >(*this,lib,"LLVMPreferredAlignmentOfGlobal",SideEffects::worstDefault,"LLVMPreferredAlignmentOfGlobal")
		->args({"TD","GlobalVar"});
// from D:\Work\libclang\include\llvm-c/Target.h:278:10
	addExtern< unsigned int (*)(LLVMOpaqueTargetData *,LLVMOpaqueType *,unsigned long long) , LLVMElementAtOffset >(*this,lib,"LLVMElementAtOffset",SideEffects::worstDefault,"LLVMElementAtOffset")
		->args({"TD","StructTy","Offset"});
// from D:\Work\libclang\include\llvm-c/Target.h:283:20
	addExtern< unsigned long long (*)(LLVMOpaqueTargetData *,LLVMOpaqueType *,unsigned int) , LLVMOffsetOfElement >(*this,lib,"LLVMOffsetOfElement",SideEffects::worstDefault,"LLVMOffsetOfElement")
		->args({"TD","StructTy","Element"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:70:15
	addExtern< LLVMTarget * (*)() , LLVMGetFirstTarget >(*this,lib,"LLVMGetFirstTarget",SideEffects::worstDefault,"LLVMGetFirstTarget");
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:72:15
	addExtern< LLVMTarget * (*)(LLVMTarget *) , LLVMGetNextTarget >(*this,lib,"LLVMGetNextTarget",SideEffects::worstDefault,"LLVMGetNextTarget")
		->args({"T"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:77:15
	addExtern< LLVMTarget * (*)(const char *) , LLVMGetTargetFromName >(*this,lib,"LLVMGetTargetFromName",SideEffects::worstDefault,"LLVMGetTargetFromName")
		->args({"Name"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:82:10
	addExtern< int (*)(const char *,LLVMTarget **,char **) , LLVMGetTargetFromTriple >(*this,lib,"LLVMGetTargetFromTriple",SideEffects::worstDefault,"LLVMGetTargetFromTriple")
		->args({"Triple","T","ErrorMessage"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:86:13
	addExtern< const char * (*)(LLVMTarget *) , LLVMGetTargetName >(*this,lib,"LLVMGetTargetName",SideEffects::worstDefault,"LLVMGetTargetName")
		->args({"T"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:89:13
	addExtern< const char * (*)(LLVMTarget *) , LLVMGetTargetDescription >(*this,lib,"LLVMGetTargetDescription",SideEffects::worstDefault,"LLVMGetTargetDescription")
		->args({"T"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:92:10
	addExtern< int (*)(LLVMTarget *) , LLVMTargetHasJIT >(*this,lib,"LLVMTargetHasJIT",SideEffects::worstDefault,"LLVMTargetHasJIT")
		->args({"T"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:95:10
	addExtern< int (*)(LLVMTarget *) , LLVMTargetHasTargetMachine >(*this,lib,"LLVMTargetHasTargetMachine",SideEffects::worstDefault,"LLVMTargetHasTargetMachine")
		->args({"T"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:98:10
	addExtern< int (*)(LLVMTarget *) , LLVMTargetHasAsmBackend >(*this,lib,"LLVMTargetHasAsmBackend",SideEffects::worstDefault,"LLVMTargetHasAsmBackend")
		->args({"T"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:102:22
	addExtern< LLVMOpaqueTargetMachine * (*)(LLVMTarget *,const char *,const char *,const char *,LLVMCodeGenOptLevel,LLVMRelocMode,LLVMCodeModel) , LLVMCreateTargetMachine >(*this,lib,"LLVMCreateTargetMachine",SideEffects::worstDefault,"LLVMCreateTargetMachine")
		->args({"T","Triple","CPU","Features","Level","Reloc","CodeModel"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:108:6
	addExtern< void (*)(LLVMOpaqueTargetMachine *) , LLVMDisposeTargetMachine >(*this,lib,"LLVMDisposeTargetMachine",SideEffects::worstDefault,"LLVMDisposeTargetMachine")
		->args({"T"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:111:15
	addExtern< LLVMTarget * (*)(LLVMOpaqueTargetMachine *) , LLVMGetTargetMachineTarget >(*this,lib,"LLVMGetTargetMachineTarget",SideEffects::worstDefault,"LLVMGetTargetMachineTarget")
		->args({"T"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:116:7
	addExtern< char * (*)(LLVMOpaqueTargetMachine *) , LLVMGetTargetMachineTriple >(*this,lib,"LLVMGetTargetMachineTriple",SideEffects::worstDefault,"LLVMGetTargetMachineTriple")
		->args({"T"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:121:7
	addExtern< char * (*)(LLVMOpaqueTargetMachine *) , LLVMGetTargetMachineCPU >(*this,lib,"LLVMGetTargetMachineCPU",SideEffects::worstDefault,"LLVMGetTargetMachineCPU")
		->args({"T"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:126:7
	addExtern< char * (*)(LLVMOpaqueTargetMachine *) , LLVMGetTargetMachineFeatureString >(*this,lib,"LLVMGetTargetMachineFeatureString",SideEffects::worstDefault,"LLVMGetTargetMachineFeatureString")
		->args({"T"});
}
}

