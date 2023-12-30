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
void Module_dasLLVM::initFunctions_6() {
// from D:\Work\libclang\include\llvm-c/Core.h:1036:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,const char *,LLVMOpaqueType *) , LLVMAddFunction >(*this,lib,"LLVMAddFunction",SideEffects::worstDefault,"LLVMAddFunction")
		->args({"M","Name","FunctionTy"});
// from D:\Work\libclang\include\llvm-c/Core.h:1046:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,const char *) , LLVMGetNamedFunction >(*this,lib,"LLVMGetNamedFunction",SideEffects::worstDefault,"LLVMGetNamedFunction")
		->args({"M","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:1053:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *) , LLVMGetFirstFunction >(*this,lib,"LLVMGetFirstFunction",SideEffects::worstDefault,"LLVMGetFirstFunction")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:1060:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *) , LLVMGetLastFunction >(*this,lib,"LLVMGetLastFunction",SideEffects::worstDefault,"LLVMGetLastFunction")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:1068:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetNextFunction >(*this,lib,"LLVMGetNextFunction",SideEffects::worstDefault,"LLVMGetNextFunction")
		->args({"Fn"});
// from D:\Work\libclang\include\llvm-c/Core.h:1076:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetPreviousFunction >(*this,lib,"LLVMGetPreviousFunction",SideEffects::worstDefault,"LLVMGetPreviousFunction")
		->args({"Fn"});
// from D:\Work\libclang\include\llvm-c/Core.h:1079:6
	addExtern< void (*)(LLVMOpaqueModule *,const char *) , LLVMSetModuleInlineAsm >(*this,lib,"LLVMSetModuleInlineAsm",SideEffects::worstDefault,"LLVMSetModuleInlineAsm")
		->args({"M","Asm"});
// from D:\Work\libclang\include\llvm-c/Core.h:1119:14
	addExtern< LLVMTypeKind (*)(LLVMOpaqueType *) , LLVMGetTypeKind >(*this,lib,"LLVMGetTypeKind",SideEffects::worstDefault,"LLVMGetTypeKind")
		->args({"Ty"});
// from D:\Work\libclang\include\llvm-c/Core.h:1128:10
	addExtern< int (*)(LLVMOpaqueType *) , LLVMTypeIsSized >(*this,lib,"LLVMTypeIsSized",SideEffects::worstDefault,"LLVMTypeIsSized")
		->args({"Ty"});
// from D:\Work\libclang\include\llvm-c/Core.h:1135:16
	addExtern< LLVMOpaqueContext * (*)(LLVMOpaqueType *) , LLVMGetTypeContext >(*this,lib,"LLVMGetTypeContext",SideEffects::worstDefault,"LLVMGetTypeContext")
		->args({"Ty"});
// from D:\Work\libclang\include\llvm-c/Core.h:1142:6
	addExtern< void (*)(LLVMOpaqueType *) , LLVMDumpType >(*this,lib,"LLVMDumpType",SideEffects::worstDefault,"LLVMDumpType")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1150:7
	addExtern< char * (*)(LLVMOpaqueType *) , LLVMPrintTypeToString >(*this,lib,"LLVMPrintTypeToString",SideEffects::worstDefault,"LLVMPrintTypeToString")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1163:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMInt1TypeInContext >(*this,lib,"LLVMInt1TypeInContext",SideEffects::worstDefault,"LLVMInt1TypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1164:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMInt8TypeInContext >(*this,lib,"LLVMInt8TypeInContext",SideEffects::worstDefault,"LLVMInt8TypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1165:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMInt16TypeInContext >(*this,lib,"LLVMInt16TypeInContext",SideEffects::worstDefault,"LLVMInt16TypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1166:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMInt32TypeInContext >(*this,lib,"LLVMInt32TypeInContext",SideEffects::worstDefault,"LLVMInt32TypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1167:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMInt64TypeInContext >(*this,lib,"LLVMInt64TypeInContext",SideEffects::worstDefault,"LLVMInt64TypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1168:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMInt128TypeInContext >(*this,lib,"LLVMInt128TypeInContext",SideEffects::worstDefault,"LLVMInt128TypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1169:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,unsigned int) , LLVMIntTypeInContext >(*this,lib,"LLVMIntTypeInContext",SideEffects::worstDefault,"LLVMIntTypeInContext")
		->args({"C","NumBits"});
// from D:\Work\libclang\include\llvm-c/Core.h:1175:13
	addExtern< LLVMOpaqueType * (*)() , LLVMInt1Type >(*this,lib,"LLVMInt1Type",SideEffects::worstDefault,"LLVMInt1Type");
}
}

