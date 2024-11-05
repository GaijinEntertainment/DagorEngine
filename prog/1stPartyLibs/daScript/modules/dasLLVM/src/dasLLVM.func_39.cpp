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
void Module_dasLLVM::initFunctions_39() {
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:724:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,uint64_t,unsigned int,unsigned int,const char *,size_t) , LLVMDIBuilderCreatePointerType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreatePointerType","LLVMDIBuilderCreatePointerType")
		->args({"Builder","PointeeTy","SizeInBits","AlignInBits","AddressSpace","Name","NameLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:747:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,uint64_t,unsigned int,LLVMDIFlags,LLVMOpaqueMetadata *,LLVMOpaqueMetadata **,unsigned int,unsigned int,LLVMOpaqueMetadata *,const char *,size_t) , LLVMDIBuilderCreateStructType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateStructType","LLVMDIBuilderCreateStructType")
		->args({"Builder","Scope","Name","NameLen","File","LineNumber","SizeInBits","AlignInBits","Flags","DerivedFrom","Elements","NumElements","RunTimeLang","VTableHolder","UniqueId","UniqueIdLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:769:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,uint64_t,unsigned int,uint64_t,LLVMDIFlags,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateMemberType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateMemberType","LLVMDIBuilderCreateMemberType")
		->args({"Builder","Scope","Name","NameLen","File","LineNo","SizeInBits","AlignInBits","OffsetInBits","Flags","Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:790:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata *,LLVMDIFlags,LLVMOpaqueValue *,unsigned int) , LLVMDIBuilderCreateStaticMemberType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateStaticMemberType","LLVMDIBuilderCreateStaticMemberType")
		->args({"Builder","Scope","Name","NameLen","File","LineNumber","Type","Flags","ConstantVal","AlignInBits"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:806:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,uint64_t,unsigned int,LLVMDIFlags) , LLVMDIBuilderCreateMemberPointerType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateMemberPointerType","LLVMDIBuilderCreateMemberPointerType")
		->args({"Builder","PointeeType","ClassType","SizeInBits","AlignInBits","Flags"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:827:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,uint64_t,unsigned int,uint64_t,LLVMDIFlags,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateObjCIVar , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateObjCIVar","LLVMDIBuilderCreateObjCIVar")
		->args({"Builder","Name","NameLen","File","LineNo","SizeInBits","AlignInBits","OffsetInBits","Flags","Ty","PropertyNode"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:849:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,const char *,size_t,const char *,size_t,unsigned int,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateObjCProperty , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateObjCProperty","LLVMDIBuilderCreateObjCProperty")
		->args({"Builder","Name","NameLen","File","LineNo","GetterName","GetterNameLen","SetterName","SetterNameLen","PropertyAttributes","Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:863:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateObjectPointerType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateObjectPointerType","LLVMDIBuilderCreateObjectPointerType")
		->args({"Builder","Type"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:875:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,unsigned int,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateQualifiedType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateQualifiedType","LLVMDIBuilderCreateQualifiedType")
		->args({"Builder","Tag","Type"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:886:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,unsigned int,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateReferenceType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateReferenceType","LLVMDIBuilderCreateReferenceType")
		->args({"Builder","Tag","Type"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:894:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *) , LLVMDIBuilderCreateNullPtrType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateNullPtrType","LLVMDIBuilderCreateNullPtrType")
		->args({"Builder"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:906:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata *,unsigned int) , LLVMDIBuilderCreateTypedef , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateTypedef","LLVMDIBuilderCreateTypedef")
		->args({"Builder","Type","Name","NameLen","File","LineNo","Scope","AlignInBits"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:922:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,uint64_t,unsigned int,LLVMDIFlags) , LLVMDIBuilderCreateInheritance , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateInheritance","LLVMDIBuilderCreateInheritance")
		->args({"Builder","Ty","BaseTy","BaseOffset","VBPtrOffset","Flags"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:943:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,unsigned int,const char *,size_t,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int,unsigned int,uint64_t,unsigned int,const char *,size_t) , LLVMDIBuilderCreateForwardDecl , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateForwardDecl","LLVMDIBuilderCreateForwardDecl")
		->args({"Builder","Tag","Name","NameLen","Scope","File","Line","RuntimeLang","SizeInBits","AlignInBits","UniqueIdentifier","UniqueIdentifierLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:967:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,unsigned int,const char *,size_t,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int,unsigned int,uint64_t,unsigned int,LLVMDIFlags,const char *,size_t) , LLVMDIBuilderCreateReplaceableCompositeType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateReplaceableCompositeType","LLVMDIBuilderCreateReplaceableCompositeType")
		->args({"Builder","Tag","Name","NameLen","Scope","File","Line","RuntimeLang","SizeInBits","AlignInBits","Flags","UniqueIdentifier","UniqueIdentifierLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:989:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,uint64_t,uint64_t,uint64_t,LLVMDIFlags,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateBitFieldMemberType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateBitFieldMemberType","LLVMDIBuilderCreateBitFieldMemberType")
		->args({"Builder","Scope","Name","NameLen","File","LineNumber","SizeInBits","OffsetInBits","StorageOffsetInBits","Flags","Type"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1020:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,uint64_t,unsigned int,uint64_t,LLVMDIFlags,LLVMOpaqueMetadata *,LLVMOpaqueMetadata **,unsigned int,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,const char *,size_t) , LLVMDIBuilderCreateClassType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateClassType","LLVMDIBuilderCreateClassType")
		->args({"Builder","Scope","Name","NameLen","File","LineNumber","SizeInBits","AlignInBits","OffsetInBits","Flags","DerivedFrom","Elements","NumElements","VTableHolder","TemplateParamsNode","UniqueIdentifier","UniqueIdentifierLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1035:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateArtificialType , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateArtificialType","LLVMDIBuilderCreateArtificialType")
		->args({"Builder","Type"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1045:13
	makeExtern< const char * (*)(LLVMOpaqueMetadata *,size_t *) , LLVMDITypeGetName , SimNode_ExtFuncCall >(lib,"LLVMDITypeGetName","LLVMDITypeGetName")
		->args({"DType","Length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1053:10
	makeExtern< uint64_t (*)(LLVMOpaqueMetadata *) , LLVMDITypeGetSizeInBits , SimNode_ExtFuncCall >(lib,"LLVMDITypeGetSizeInBits","LLVMDITypeGetSizeInBits")
		->args({"DType"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

