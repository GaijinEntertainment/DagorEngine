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
void Module_dasLLVM::initFunctions_8() {
// from D:\Work\libclang\include\llvm-c/Core.h:1252:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMFP128Type , SimNode_ExtFuncCall >(lib,"LLVMFP128Type","LLVMFP128Type")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1253:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMPPCFP128Type , SimNode_ExtFuncCall >(lib,"LLVMPPCFP128Type","LLVMPPCFP128Type")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1271:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueType *,LLVMOpaqueType **,unsigned int,int) , LLVMFunctionType , SimNode_ExtFuncCall >(lib,"LLVMFunctionType","LLVMFunctionType")
		->args({"ReturnType","ParamTypes","ParamCount","IsVarArg"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1278:10
	makeExtern< int (*)(LLVMOpaqueType *) , LLVMIsFunctionVarArg , SimNode_ExtFuncCall >(lib,"LLVMIsFunctionVarArg","LLVMIsFunctionVarArg")
		->args({"FunctionTy"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1283:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueType *) , LLVMGetReturnType , SimNode_ExtFuncCall >(lib,"LLVMGetReturnType","LLVMGetReturnType")
		->args({"FunctionTy"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1288:10
	makeExtern< unsigned int (*)(LLVMOpaqueType *) , LLVMCountParamTypes , SimNode_ExtFuncCall >(lib,"LLVMCountParamTypes","LLVMCountParamTypes")
		->args({"FunctionTy"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1301:6
	makeExtern< void (*)(LLVMOpaqueType *,LLVMOpaqueType **) , LLVMGetParamTypes , SimNode_ExtFuncCall >(lib,"LLVMGetParamTypes","LLVMGetParamTypes")
		->args({"FunctionTy","Dest"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1325:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,LLVMOpaqueType **,unsigned int,int) , LLVMStructTypeInContext , SimNode_ExtFuncCall >(lib,"LLVMStructTypeInContext","LLVMStructTypeInContext")
		->args({"C","ElementTypes","ElementCount","Packed"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1333:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueType **,unsigned int,int) , LLVMStructType , SimNode_ExtFuncCall >(lib,"LLVMStructType","LLVMStructType")
		->args({"ElementTypes","ElementCount","Packed"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1341:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,const char *) , LLVMStructCreateNamed , SimNode_ExtFuncCall >(lib,"LLVMStructCreateNamed","LLVMStructCreateNamed")
		->args({"C","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1348:13
	makeExtern< const char * (*)(LLVMOpaqueType *) , LLVMGetStructName , SimNode_ExtFuncCall >(lib,"LLVMGetStructName","LLVMGetStructName")
		->args({"Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1355:6
	makeExtern< void (*)(LLVMOpaqueType *,LLVMOpaqueType **,unsigned int,int) , LLVMStructSetBody , SimNode_ExtFuncCall >(lib,"LLVMStructSetBody","LLVMStructSetBody")
		->args({"StructTy","ElementTypes","ElementCount","Packed"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1363:10
	makeExtern< unsigned int (*)(LLVMOpaqueType *) , LLVMCountStructElementTypes , SimNode_ExtFuncCall >(lib,"LLVMCountStructElementTypes","LLVMCountStructElementTypes")
		->args({"StructTy"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1375:6
	makeExtern< void (*)(LLVMOpaqueType *,LLVMOpaqueType **) , LLVMGetStructElementTypes , SimNode_ExtFuncCall >(lib,"LLVMGetStructElementTypes","LLVMGetStructElementTypes")
		->args({"StructTy","Dest"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1382:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueType *,unsigned int) , LLVMStructGetTypeAtIndex , SimNode_ExtFuncCall >(lib,"LLVMStructGetTypeAtIndex","LLVMStructGetTypeAtIndex")
		->args({"StructTy","i"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1389:10
	makeExtern< int (*)(LLVMOpaqueType *) , LLVMIsPackedStruct , SimNode_ExtFuncCall >(lib,"LLVMIsPackedStruct","LLVMIsPackedStruct")
		->args({"StructTy"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1396:10
	makeExtern< int (*)(LLVMOpaqueType *) , LLVMIsOpaqueStruct , SimNode_ExtFuncCall >(lib,"LLVMIsOpaqueStruct","LLVMIsOpaqueStruct")
		->args({"StructTy"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1403:10
	makeExtern< int (*)(LLVMOpaqueType *) , LLVMIsLiteralStruct , SimNode_ExtFuncCall >(lib,"LLVMIsLiteralStruct","LLVMIsLiteralStruct")
		->args({"StructTy"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1425:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueType *) , LLVMGetElementType , SimNode_ExtFuncCall >(lib,"LLVMGetElementType","LLVMGetElementType")
		->args({"Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1432:6
	makeExtern< void (*)(LLVMOpaqueType *,LLVMOpaqueType **) , LLVMGetSubtypes , SimNode_ExtFuncCall >(lib,"LLVMGetSubtypes","LLVMGetSubtypes")
		->args({"Tp","Arr"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

