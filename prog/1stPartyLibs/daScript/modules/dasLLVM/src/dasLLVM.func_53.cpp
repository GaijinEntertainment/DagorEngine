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
void Module_dasLLVM::initFunctions_53() {
// from D:\Work\libclang\include\llvm-c/LLJIT.h:130:28
	makeExtern< LLVMOrcOpaqueExecutionSession * (*)(LLVMOrcOpaqueLLJIT *) , LLVMOrcLLJITGetExecutionSession , SimNode_ExtFuncCall >(lib,"LLVMOrcLLJITGetExecutionSession","LLVMOrcLLJITGetExecutionSession")
		->args({"J"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:138:20
	makeExtern< LLVMOrcOpaqueJITDylib * (*)(LLVMOrcOpaqueLLJIT *) , LLVMOrcLLJITGetMainJITDylib , SimNode_ExtFuncCall >(lib,"LLVMOrcLLJITGetMainJITDylib","LLVMOrcLLJITGetMainJITDylib")
		->args({"J"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:144:13
	makeExtern< const char * (*)(LLVMOrcOpaqueLLJIT *) , LLVMOrcLLJITGetTripleString , SimNode_ExtFuncCall >(lib,"LLVMOrcLLJITGetTripleString","LLVMOrcLLJITGetTripleString")
		->args({"J"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:149:6
	makeExtern< char (*)(LLVMOrcOpaqueLLJIT *) , LLVMOrcLLJITGetGlobalPrefix , SimNode_ExtFuncCall >(lib,"LLVMOrcLLJITGetGlobalPrefix","LLVMOrcLLJITGetGlobalPrefix")
		->args({"J"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:159:1
	makeExtern< LLVMOrcOpaqueSymbolStringPoolEntry * (*)(LLVMOrcOpaqueLLJIT *,const char *) , LLVMOrcLLJITMangleAndIntern , SimNode_ExtFuncCall >(lib,"LLVMOrcLLJITMangleAndIntern","LLVMOrcLLJITMangleAndIntern")
		->args({"J","UnmangledName"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:170:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueLLJIT *,LLVMOrcOpaqueJITDylib *,LLVMOpaqueMemoryBuffer *) , LLVMOrcLLJITAddObjectFile , SimNode_ExtFuncCall >(lib,"LLVMOrcLLJITAddObjectFile","LLVMOrcLLJITAddObjectFile")
		->args({"J","JD","ObjBuffer"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:182:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueLLJIT *,LLVMOrcOpaqueResourceTracker *,LLVMOpaqueMemoryBuffer *) , LLVMOrcLLJITAddObjectFileWithRT , SimNode_ExtFuncCall >(lib,"LLVMOrcLLJITAddObjectFileWithRT","LLVMOrcLLJITAddObjectFileWithRT")
		->args({"J","RT","ObjBuffer"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:195:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueLLJIT *,LLVMOrcOpaqueJITDylib *,LLVMOrcOpaqueThreadSafeModule *) , LLVMOrcLLJITAddLLVMIRModule , SimNode_ExtFuncCall >(lib,"LLVMOrcLLJITAddLLVMIRModule","LLVMOrcLLJITAddLLVMIRModule")
		->args({"J","JD","TSM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:208:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueLLJIT *,LLVMOrcOpaqueResourceTracker *,LLVMOrcOpaqueThreadSafeModule *) , LLVMOrcLLJITAddLLVMIRModuleWithRT , SimNode_ExtFuncCall >(lib,"LLVMOrcLLJITAddLLVMIRModuleWithRT","LLVMOrcLLJITAddLLVMIRModuleWithRT")
		->args({"J","JD","TSM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:217:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueLLJIT *,unsigned long long *,const char *) , LLVMOrcLLJITLookup , SimNode_ExtFuncCall >(lib,"LLVMOrcLLJITLookup","LLVMOrcLLJITLookup")
		->args({"J","Result","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:224:23
	makeExtern< LLVMOrcOpaqueObjectLayer * (*)(LLVMOrcOpaqueLLJIT *) , LLVMOrcLLJITGetObjLinkingLayer , SimNode_ExtFuncCall >(lib,"LLVMOrcLLJITGetObjLinkingLayer","LLVMOrcLLJITGetObjLinkingLayer")
		->args({"J"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:230:1
	makeExtern< LLVMOrcOpaqueObjectTransformLayer * (*)(LLVMOrcOpaqueLLJIT *) , LLVMOrcLLJITGetObjTransformLayer , SimNode_ExtFuncCall >(lib,"LLVMOrcLLJITGetObjTransformLayer","LLVMOrcLLJITGetObjTransformLayer")
		->args({"J"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:235:28
	makeExtern< LLVMOrcOpaqueIRTransformLayer * (*)(LLVMOrcOpaqueLLJIT *) , LLVMOrcLLJITGetIRTransformLayer , SimNode_ExtFuncCall >(lib,"LLVMOrcLLJITGetIRTransformLayer","LLVMOrcLLJITGetIRTransformLayer")
		->args({"J"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:243:13
	makeExtern< const char * (*)(LLVMOrcOpaqueLLJIT *) , LLVMOrcLLJITGetDataLayoutStr , SimNode_ExtFuncCall >(lib,"LLVMOrcLLJITGetDataLayoutStr","LLVMOrcLLJITGetDataLayoutStr")
		->args({"J"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:111:1
	makeExtern< const char * (*)() , lto_get_version , SimNode_ExtFuncCall >(lib,"lto_get_version","lto_get_version")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:119:1
	makeExtern< const char * (*)() , lto_get_error_message , SimNode_ExtFuncCall >(lib,"lto_get_error_message","lto_get_error_message")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:127:1
	makeExtern< bool (*)(const char *) , lto_module_is_object_file , SimNode_ExtFuncCall >(lib,"lto_module_is_object_file","lto_module_is_object_file")
		->args({"path"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:135:1
	makeExtern< bool (*)(const char *,const char *) , lto_module_is_object_file_for_target , SimNode_ExtFuncCall >(lib,"lto_module_is_object_file_for_target","lto_module_is_object_file_for_target")
		->args({"path","target_triple_prefix"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:145:1
	makeExtern< bool (*)(const void *,size_t) , lto_module_has_objc_category , SimNode_ExtFuncCall >(lib,"lto_module_has_objc_category","lto_module_has_objc_category")
		->args({"mem","length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:152:19
	makeExtern< bool (*)(const void *,size_t) , lto_module_is_object_file_in_memory , SimNode_ExtFuncCall >(lib,"lto_module_is_object_file_in_memory","lto_module_is_object_file_in_memory")
		->args({"mem","length"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

