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
void Module_dasLLVM::initFunctions_41() {
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1281:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueValue *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueValue *) , LLVMDIBuilderInsertDbgValueBefore , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderInsertDbgValueBefore","LLVMDIBuilderInsertDbgValueBefore")
		->args({"Builder","Val","VarInfo","Expr","DebugLoc","Instr"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1299:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueValue *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueBasicBlock *) , LLVMDIBuilderInsertDbgValueAtEnd , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderInsertDbgValueAtEnd","LLVMDIBuilderInsertDbgValueAtEnd")
		->args({"Builder","Val","VarInfo","Expr","DebugLoc","Block"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1319:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata *,int,LLVMDIFlags,unsigned int) , LLVMDIBuilderCreateAutoVariable , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateAutoVariable","LLVMDIBuilderCreateAutoVariable")
		->args({"Builder","Scope","Name","NameLen","File","LineNo","Ty","AlwaysPreserve","Flags","AlignInBits"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1337:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,unsigned int,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata *,int,LLVMDIFlags) , LLVMDIBuilderCreateParameterVariable , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateParameterVariable","LLVMDIBuilderCreateParameterVariable")
		->args({"Builder","Scope","Name","NameLen","ArgNo","File","LineNo","Ty","AlwaysPreserve","Flags"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1347:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueValue *) , LLVMGetSubprogram , SimNode_ExtFuncCall >(lib,"LLVMGetSubprogram","LLVMGetSubprogram")
		->args({"Func"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1354:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueMetadata *) , LLVMSetSubprogram , SimNode_ExtFuncCall >(lib,"LLVMSetSubprogram","LLVMSetSubprogram")
		->args({"Func","SP"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1362:10
	makeExtern< unsigned int (*)(LLVMOpaqueMetadata *) , LLVMDISubprogramGetLine , SimNode_ExtFuncCall >(lib,"LLVMDISubprogramGetLine","LLVMDISubprogramGetLine")
		->args({"Subprogram"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1369:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueValue *) , LLVMInstructionGetDebugLoc , SimNode_ExtFuncCall >(lib,"LLVMInstructionGetDebugLoc","LLVMInstructionGetDebugLoc")
		->args({"Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1378:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueMetadata *) , LLVMInstructionSetDebugLoc , SimNode_ExtFuncCall >(lib,"LLVMInstructionSetDebugLoc","LLVMInstructionSetDebugLoc")
		->args({"Inst","Loc"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1385:18
	makeExtern< unsigned int (*)(LLVMOpaqueMetadata *) , LLVMGetMetadataKind , SimNode_ExtFuncCall >(lib,"LLVMGetMetadataKind","LLVMGetMetadataKind")
		->args({"Metadata"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Disassembler.h:72:5
	makeExtern< int (*)(void *,uint64_t) , LLVMSetDisasmOptions , SimNode_ExtFuncCall >(lib,"LLVMSetDisasmOptions","LLVMSetDisasmOptions")
		->args({"DC","Options"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Disassembler.h:88:6
	makeExtern< void (*)(void *) , LLVMDisasmDispose , SimNode_ExtFuncCall >(lib,"LLVMDisasmDispose","LLVMDisasmDispose")
		->args({"DC"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Disassembler.h:100:8
	makeExtern< size_t (*)(void *,unsigned char *,uint64_t,uint64_t,char *,size_t) , LLVMDisasmInstruction , SimNode_ExtFuncCall >(lib,"LLVMDisasmInstruction","LLVMDisasmInstruction")
		->args({"DC","Bytes","BytesSize","PC","OutString","OutStringSize"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Error.h:44:17
	makeExtern< const void * (*)(LLVMOpaqueError *) , LLVMGetErrorTypeId , SimNode_ExtFuncCall >(lib,"LLVMGetErrorTypeId","LLVMGetErrorTypeId")
		->args({"Err"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Error.h:52:6
	makeExtern< void (*)(LLVMOpaqueError *) , LLVMConsumeError , SimNode_ExtFuncCall >(lib,"LLVMConsumeError","LLVMConsumeError")
		->args({"Err"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Error.h:60:7
	makeExtern< char * (*)(LLVMOpaqueError *) , LLVMGetErrorMessage , SimNode_ExtFuncCall >(lib,"LLVMGetErrorMessage","LLVMGetErrorMessage")
		->args({"Err"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Error.h:65:6
	makeExtern< void (*)(char *) , LLVMDisposeErrorMessage , SimNode_ExtFuncCall >(lib,"LLVMDisposeErrorMessage","LLVMDisposeErrorMessage")
		->args({"ErrMsg"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Error.h:70:17
	makeExtern< const void * (*)() , LLVMGetStringErrorTypeId , SimNode_ExtFuncCall >(lib,"LLVMGetStringErrorTypeId","LLVMGetStringErrorTypeId")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Error.h:75:14
	makeExtern< LLVMOpaqueError * (*)(const char *) , LLVMCreateStringError , SimNode_ExtFuncCall >(lib,"LLVMCreateStringError","LLVMCreateStringError")
		->args({"ErrMsg"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:26:1
	makeExtern< void (*)() , LLVMInitializeX86TargetInfo , SimNode_ExtFuncCall >(lib,"LLVMInitializeX86TargetInfo","LLVMInitializeX86TargetInfo")
		->addToModule(*this, SideEffects::worstDefault);
}
}

