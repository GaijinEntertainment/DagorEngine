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
// from D:\Work\libclang\include\llvm-c/Core.h:1041:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetDebugLocColumn , SimNode_ExtFuncCall >(lib,"LLVMGetDebugLocColumn","LLVMGetDebugLocColumn")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1048:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,const char *,LLVMOpaqueType *) , LLVMAddFunction , SimNode_ExtFuncCall >(lib,"LLVMAddFunction","LLVMAddFunction")
		->args({"M","Name","FunctionTy"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1058:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,const char *) , LLVMGetNamedFunction , SimNode_ExtFuncCall >(lib,"LLVMGetNamedFunction","LLVMGetNamedFunction")
		->args({"M","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1065:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *) , LLVMGetFirstFunction , SimNode_ExtFuncCall >(lib,"LLVMGetFirstFunction","LLVMGetFirstFunction")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1072:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *) , LLVMGetLastFunction , SimNode_ExtFuncCall >(lib,"LLVMGetLastFunction","LLVMGetLastFunction")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1080:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetNextFunction , SimNode_ExtFuncCall >(lib,"LLVMGetNextFunction","LLVMGetNextFunction")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1088:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetPreviousFunction , SimNode_ExtFuncCall >(lib,"LLVMGetPreviousFunction","LLVMGetPreviousFunction")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1091:6
	makeExtern< void (*)(LLVMOpaqueModule *,const char *) , LLVMSetModuleInlineAsm , SimNode_ExtFuncCall >(lib,"LLVMSetModuleInlineAsm","LLVMSetModuleInlineAsm")
		->args({"M","Asm"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1131:14
	makeExtern< LLVMTypeKind (*)(LLVMOpaqueType *) , LLVMGetTypeKind , SimNode_ExtFuncCall >(lib,"LLVMGetTypeKind","LLVMGetTypeKind")
		->args({"Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1140:10
	makeExtern< int (*)(LLVMOpaqueType *) , LLVMTypeIsSized , SimNode_ExtFuncCall >(lib,"LLVMTypeIsSized","LLVMTypeIsSized")
		->args({"Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1147:16
	makeExtern< LLVMOpaqueContext * (*)(LLVMOpaqueType *) , LLVMGetTypeContext , SimNode_ExtFuncCall >(lib,"LLVMGetTypeContext","LLVMGetTypeContext")
		->args({"Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1154:6
	makeExtern< void (*)(LLVMOpaqueType *) , LLVMDumpType , SimNode_ExtFuncCall >(lib,"LLVMDumpType","LLVMDumpType")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1162:7
	makeExtern< char * (*)(LLVMOpaqueType *) , LLVMPrintTypeToString , SimNode_ExtFuncCall >(lib,"LLVMPrintTypeToString","LLVMPrintTypeToString")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1175:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMInt1TypeInContext , SimNode_ExtFuncCall >(lib,"LLVMInt1TypeInContext","LLVMInt1TypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1176:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMInt8TypeInContext , SimNode_ExtFuncCall >(lib,"LLVMInt8TypeInContext","LLVMInt8TypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1177:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMInt16TypeInContext , SimNode_ExtFuncCall >(lib,"LLVMInt16TypeInContext","LLVMInt16TypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1178:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMInt32TypeInContext , SimNode_ExtFuncCall >(lib,"LLVMInt32TypeInContext","LLVMInt32TypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1179:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMInt64TypeInContext , SimNode_ExtFuncCall >(lib,"LLVMInt64TypeInContext","LLVMInt64TypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1180:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMInt128TypeInContext , SimNode_ExtFuncCall >(lib,"LLVMInt128TypeInContext","LLVMInt128TypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1181:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,unsigned int) , LLVMIntTypeInContext , SimNode_ExtFuncCall >(lib,"LLVMIntTypeInContext","LLVMIntTypeInContext")
		->args({"C","NumBits"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

