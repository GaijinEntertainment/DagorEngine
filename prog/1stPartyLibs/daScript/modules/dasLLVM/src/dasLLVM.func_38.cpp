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
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:470:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueContext *,unsigned int,unsigned int,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateDebugLocation , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateDebugLocation","LLVMDIBuilderCreateDebugLocation")
		->args({"Ctx","Line","Column","Scope","InlinedAt"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:480:10
	makeExtern< unsigned int (*)(LLVMOpaqueMetadata *) , LLVMDILocationGetLine , SimNode_ExtFuncCall >(lib,"LLVMDILocationGetLine","LLVMDILocationGetLine")
		->args({"Location"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:488:10
	makeExtern< unsigned int (*)(LLVMOpaqueMetadata *) , LLVMDILocationGetColumn , SimNode_ExtFuncCall >(lib,"LLVMDILocationGetColumn","LLVMDILocationGetColumn")
		->args({"Location"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:496:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueMetadata *) , LLVMDILocationGetScope , SimNode_ExtFuncCall >(lib,"LLVMDILocationGetScope","LLVMDILocationGetScope")
		->args({"Location"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:504:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueMetadata *) , LLVMDILocationGetInlinedAt , SimNode_ExtFuncCall >(lib,"LLVMDILocationGetInlinedAt","LLVMDILocationGetInlinedAt")
		->args({"Location"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:512:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueMetadata *) , LLVMDIScopeGetFile , SimNode_ExtFuncCall >(lib,"LLVMDIScopeGetFile","LLVMDIScopeGetFile")
		->args({"Scope"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:521:13
	makeExtern< const char * (*)(LLVMOpaqueMetadata *,unsigned int *) , LLVMDIFileGetDirectory , SimNode_ExtFuncCall >(lib,"LLVMDIFileGetDirectory","LLVMDIFileGetDirectory")
		->args({"File","Len"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:530:13
	makeExtern< const char * (*)(LLVMOpaqueMetadata *,unsigned int *) , LLVMDIFileGetFilename , SimNode_ExtFuncCall >(lib,"LLVMDIFileGetFilename","LLVMDIFileGetFilename")
		->args({"File","Len"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:539:13
	makeExtern< const char * (*)(LLVMOpaqueMetadata *,unsigned int *) , LLVMDIFileGetSource , SimNode_ExtFuncCall >(lib,"LLVMDIFileGetSource","LLVMDIFileGetSource")
		->args({"File","Len"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:547:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata **,size_t) , LLVMDIBuilderGetOrCreateTypeArray , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderGetOrCreateTypeArray","LLVMDIBuilderGetOrCreateTypeArray")
		->args({"Builder","Data","NumElements"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:562:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata **,unsigned int,LLVMDIFlags) , LLVMDIBuilderCreateSubroutineType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateSubroutineType","LLVMDIBuilderCreateSubroutineType")
		->args({"Builder","File","ParameterTypes","NumParameterTypes","Flags"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:579:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,unsigned int,LLVMDWARFMacinfoRecordType,const char *,size_t,const char *,size_t) , LLVMDIBuilderCreateMacro , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateMacro","LLVMDIBuilderCreateMacro")
		->args({"Builder","ParentMacroFile","Line","RecordType","Name","NameLen","Value","ValueLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:596:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateTempMacroFile , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateTempMacroFile","LLVMDIBuilderCreateTempMacroFile")
		->args({"Builder","ParentMacroFile","Line","File"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:608:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,const char *,size_t,int64_t,int) , LLVMDIBuilderCreateEnumerator , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateEnumerator","LLVMDIBuilderCreateEnumerator")
		->args({"Builder","Name","NameLen","Value","IsUnsigned"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:627:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,uint64_t,unsigned int,LLVMOpaqueMetadata **,unsigned int,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateEnumerationType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateEnumerationType","LLVMDIBuilderCreateEnumerationType")
		->args({"Builder","Scope","Name","NameLen","File","LineNumber","SizeInBits","AlignInBits","Elements","NumElements","ClassTy"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:650:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,uint64_t,unsigned int,LLVMDIFlags,LLVMOpaqueMetadata **,unsigned int,unsigned int,const char *,size_t) , LLVMDIBuilderCreateUnionType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateUnionType","LLVMDIBuilderCreateUnionType")
		->args({"Builder","Scope","Name","NameLen","File","LineNumber","SizeInBits","AlignInBits","Flags","Elements","NumElements","RunTimeLang","UniqueId","UniqueIdLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:668:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,uint64_t,unsigned int,LLVMOpaqueMetadata *,LLVMOpaqueMetadata **,unsigned int) , LLVMDIBuilderCreateArrayType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateArrayType","LLVMDIBuilderCreateArrayType")
		->args({"Builder","Size","AlignInBits","Ty","Subscripts","NumSubscripts"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:683:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,uint64_t,unsigned int,LLVMOpaqueMetadata *,LLVMOpaqueMetadata **,unsigned int) , LLVMDIBuilderCreateVectorType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateVectorType","LLVMDIBuilderCreateVectorType")
		->args({"Builder","Size","AlignInBits","Ty","Subscripts","NumSubscripts"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:695:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,const char *,size_t) , LLVMDIBuilderCreateUnspecifiedType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateUnspecifiedType","LLVMDIBuilderCreateUnspecifiedType")
		->args({"Builder","Name","NameLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:709:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,const char *,size_t,uint64_t,unsigned int,LLVMDIFlags) , LLVMDIBuilderCreateBasicType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateBasicType","LLVMDIBuilderCreateBasicType")
		->args({"Builder","Name","NameLen","SizeInBits","Encoding","Flags"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

