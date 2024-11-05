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
void Module_dasLLVM::initFunctions_46() {
// from D:\Work\libclang\include\llvm-c/Target.h:273:10
	makeExtern< unsigned int (*)(LLVMOpaqueTargetData *,LLVMOpaqueValue *) , LLVMPreferredAlignmentOfGlobal , SimNode_ExtFuncCall >(lib,"LLVMPreferredAlignmentOfGlobal","LLVMPreferredAlignmentOfGlobal")
		->args({"TD","GlobalVar"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:278:10
	makeExtern< unsigned int (*)(LLVMOpaqueTargetData *,LLVMOpaqueType *,unsigned long long) , LLVMElementAtOffset , SimNode_ExtFuncCall >(lib,"LLVMElementAtOffset","LLVMElementAtOffset")
		->args({"TD","StructTy","Offset"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:283:20
	makeExtern< unsigned long long (*)(LLVMOpaqueTargetData *,LLVMOpaqueType *,unsigned int) , LLVMOffsetOfElement , SimNode_ExtFuncCall >(lib,"LLVMOffsetOfElement","LLVMOffsetOfElement")
		->args({"TD","StructTy","Element"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:70:15
	makeExtern< LLVMTarget * (*)() , LLVMGetFirstTarget , SimNode_ExtFuncCall >(lib,"LLVMGetFirstTarget","LLVMGetFirstTarget")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:72:15
	makeExtern< LLVMTarget * (*)(LLVMTarget *) , LLVMGetNextTarget , SimNode_ExtFuncCall >(lib,"LLVMGetNextTarget","LLVMGetNextTarget")
		->args({"T"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:77:15
	makeExtern< LLVMTarget * (*)(const char *) , LLVMGetTargetFromName , SimNode_ExtFuncCall >(lib,"LLVMGetTargetFromName","LLVMGetTargetFromName")
		->args({"Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:82:10
	makeExtern< int (*)(const char *,LLVMTarget **,char **) , LLVMGetTargetFromTriple , SimNode_ExtFuncCall >(lib,"LLVMGetTargetFromTriple","LLVMGetTargetFromTriple")
		->args({"Triple","T","ErrorMessage"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:86:13
	makeExtern< const char * (*)(LLVMTarget *) , LLVMGetTargetName , SimNode_ExtFuncCall >(lib,"LLVMGetTargetName","LLVMGetTargetName")
		->args({"T"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:89:13
	makeExtern< const char * (*)(LLVMTarget *) , LLVMGetTargetDescription , SimNode_ExtFuncCall >(lib,"LLVMGetTargetDescription","LLVMGetTargetDescription")
		->args({"T"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:92:10
	makeExtern< int (*)(LLVMTarget *) , LLVMTargetHasJIT , SimNode_ExtFuncCall >(lib,"LLVMTargetHasJIT","LLVMTargetHasJIT")
		->args({"T"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:95:10
	makeExtern< int (*)(LLVMTarget *) , LLVMTargetHasTargetMachine , SimNode_ExtFuncCall >(lib,"LLVMTargetHasTargetMachine","LLVMTargetHasTargetMachine")
		->args({"T"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:98:10
	makeExtern< int (*)(LLVMTarget *) , LLVMTargetHasAsmBackend , SimNode_ExtFuncCall >(lib,"LLVMTargetHasAsmBackend","LLVMTargetHasAsmBackend")
		->args({"T"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:102:22
	makeExtern< LLVMOpaqueTargetMachine * (*)(LLVMTarget *,const char *,const char *,const char *,LLVMCodeGenOptLevel,LLVMRelocMode,LLVMCodeModel) , LLVMCreateTargetMachine , SimNode_ExtFuncCall >(lib,"LLVMCreateTargetMachine","LLVMCreateTargetMachine")
		->args({"T","Triple","CPU","Features","Level","Reloc","CodeModel"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:108:6
	makeExtern< void (*)(LLVMOpaqueTargetMachine *) , LLVMDisposeTargetMachine , SimNode_ExtFuncCall >(lib,"LLVMDisposeTargetMachine","LLVMDisposeTargetMachine")
		->args({"T"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:111:15
	makeExtern< LLVMTarget * (*)(LLVMOpaqueTargetMachine *) , LLVMGetTargetMachineTarget , SimNode_ExtFuncCall >(lib,"LLVMGetTargetMachineTarget","LLVMGetTargetMachineTarget")
		->args({"T"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:116:7
	makeExtern< char * (*)(LLVMOpaqueTargetMachine *) , LLVMGetTargetMachineTriple , SimNode_ExtFuncCall >(lib,"LLVMGetTargetMachineTriple","LLVMGetTargetMachineTriple")
		->args({"T"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:121:7
	makeExtern< char * (*)(LLVMOpaqueTargetMachine *) , LLVMGetTargetMachineCPU , SimNode_ExtFuncCall >(lib,"LLVMGetTargetMachineCPU","LLVMGetTargetMachineCPU")
		->args({"T"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:126:7
	makeExtern< char * (*)(LLVMOpaqueTargetMachine *) , LLVMGetTargetMachineFeatureString , SimNode_ExtFuncCall >(lib,"LLVMGetTargetMachineFeatureString","LLVMGetTargetMachineFeatureString")
		->args({"T"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:129:19
	makeExtern< LLVMOpaqueTargetData * (*)(LLVMOpaqueTargetMachine *) , LLVMCreateTargetDataLayout , SimNode_ExtFuncCall >(lib,"LLVMCreateTargetDataLayout","LLVMCreateTargetDataLayout")
		->args({"T"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:132:6
	makeExtern< void (*)(LLVMOpaqueTargetMachine *,int) , LLVMSetTargetMachineAsmVerbosity , SimNode_ExtFuncCall >(lib,"LLVMSetTargetMachineAsmVerbosity","LLVMSetTargetMachineAsmVerbosity")
		->args({"T","VerboseAsm"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

