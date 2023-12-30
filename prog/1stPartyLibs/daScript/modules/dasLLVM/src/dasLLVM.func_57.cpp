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
void Module_dasLLVM::initFunctions_57() {
// from D:\Work\libclang\include\llvm-c/lto.h:119:1
	addExtern< const char * (*)() , lto_get_error_message >(*this,lib,"lto_get_error_message",SideEffects::worstDefault,"lto_get_error_message");
// from D:\Work\libclang\include\llvm-c/lto.h:127:1
	addExtern< bool (*)(const char *) , lto_module_is_object_file >(*this,lib,"lto_module_is_object_file",SideEffects::worstDefault,"lto_module_is_object_file")
		->args({"path"});
// from D:\Work\libclang\include\llvm-c/lto.h:135:1
	addExtern< bool (*)(const char *,const char *) , lto_module_is_object_file_for_target >(*this,lib,"lto_module_is_object_file_for_target",SideEffects::worstDefault,"lto_module_is_object_file_for_target")
		->args({"path","target_triple_prefix"});
// from D:\Work\libclang\include\llvm-c/lto.h:145:1
	addExtern< bool (*)(const void *,size_t) , lto_module_has_objc_category >(*this,lib,"lto_module_has_objc_category",SideEffects::worstDefault,"lto_module_has_objc_category")
		->args({"mem","length"});
// from D:\Work\libclang\include\llvm-c/lto.h:152:19
	addExtern< bool (*)(const void *,size_t) , lto_module_is_object_file_in_memory >(*this,lib,"lto_module_is_object_file_in_memory",SideEffects::worstDefault,"lto_module_is_object_file_in_memory")
		->args({"mem","length"});
// from D:\Work\libclang\include\llvm-c/lto.h:161:1
	addExtern< bool (*)(const void *,size_t,const char *) , lto_module_is_object_file_in_memory_for_target >(*this,lib,"lto_module_is_object_file_in_memory_for_target",SideEffects::worstDefault,"lto_module_is_object_file_in_memory_for_target")
		->args({"mem","length","target_triple_prefix"});
// from D:\Work\libclang\include\llvm-c/lto.h:171:1
	addExtern< LLVMOpaqueLTOModule * (*)(const char *) , lto_module_create >(*this,lib,"lto_module_create",SideEffects::worstDefault,"lto_module_create")
		->args({"path"});
// from D:\Work\libclang\include\llvm-c/lto.h:180:1
	addExtern< LLVMOpaqueLTOModule * (*)(const void *,size_t) , lto_module_create_from_memory >(*this,lib,"lto_module_create_from_memory",SideEffects::worstDefault,"lto_module_create_from_memory")
		->args({"mem","length"});
// from D:\Work\libclang\include\llvm-c/lto.h:189:1
	addExtern< LLVMOpaqueLTOModule * (*)(const void *,size_t,const char *) , lto_module_create_from_memory_with_path >(*this,lib,"lto_module_create_from_memory_with_path",SideEffects::worstDefault,"lto_module_create_from_memory_with_path")
		->args({"mem","length","path"});
// from D:\Work\libclang\include\llvm-c/lto.h:204:1
	addExtern< LLVMOpaqueLTOModule * (*)(const void *,size_t,const char *) , lto_module_create_in_local_context >(*this,lib,"lto_module_create_in_local_context",SideEffects::worstDefault,"lto_module_create_in_local_context")
		->args({"mem","length","path"});
// from D:\Work\libclang\include\llvm-c/lto.h:218:1
	addExtern< LLVMOpaqueLTOModule * (*)(const void *,size_t,const char *,LLVMOpaqueLTOCodeGenerator *) , lto_module_create_in_codegen_context >(*this,lib,"lto_module_create_in_codegen_context",SideEffects::worstDefault,"lto_module_create_in_codegen_context")
		->args({"mem","length","path","cg"});
// from D:\Work\libclang\include\llvm-c/lto.h:228:1
	addExtern< LLVMOpaqueLTOModule * (*)(int,const char *,size_t) , lto_module_create_from_fd >(*this,lib,"lto_module_create_from_fd",SideEffects::worstDefault,"lto_module_create_from_fd")
		->args({"fd","path","file_size"});
// from D:\Work\libclang\include\llvm-c/lto.h:237:1
	addExtern< LLVMOpaqueLTOModule * (*)(int,const char *,size_t,size_t,off_t) , lto_module_create_from_fd_at_offset >(*this,lib,"lto_module_create_from_fd_at_offset",SideEffects::worstDefault,"lto_module_create_from_fd_at_offset")
		->args({"fd","path","file_size","map_size","offset"});
// from D:\Work\libclang\include\llvm-c/lto.h:247:1
	addExtern< void (*)(LLVMOpaqueLTOModule *) , lto_module_dispose >(*this,lib,"lto_module_dispose",SideEffects::worstDefault,"lto_module_dispose")
		->args({"mod"});
// from D:\Work\libclang\include\llvm-c/lto.h:255:1
	addExtern< const char * (*)(LLVMOpaqueLTOModule *) , lto_module_get_target_triple >(*this,lib,"lto_module_get_target_triple",SideEffects::worstDefault,"lto_module_get_target_triple")
		->args({"mod"});
// from D:\Work\libclang\include\llvm-c/lto.h:263:1
	addExtern< void (*)(LLVMOpaqueLTOModule *,const char *) , lto_module_set_target_triple >(*this,lib,"lto_module_set_target_triple",SideEffects::worstDefault,"lto_module_set_target_triple")
		->args({"mod","triple"});
// from D:\Work\libclang\include\llvm-c/lto.h:271:1
	addExtern< unsigned int (*)(LLVMOpaqueLTOModule *) , lto_module_get_num_symbols >(*this,lib,"lto_module_get_num_symbols",SideEffects::worstDefault,"lto_module_get_num_symbols")
		->args({"mod"});
// from D:\Work\libclang\include\llvm-c/lto.h:279:1
	addExtern< const char * (*)(LLVMOpaqueLTOModule *,unsigned int) , lto_module_get_symbol_name >(*this,lib,"lto_module_get_symbol_name",SideEffects::worstDefault,"lto_module_get_symbol_name")
		->args({"mod","index"});
// from D:\Work\libclang\include\llvm-c/lto.h:287:1
	addExtern< lto_symbol_attributes (*)(LLVMOpaqueLTOModule *,unsigned int) , lto_module_get_symbol_attribute >(*this,lib,"lto_module_get_symbol_attribute",SideEffects::worstDefault,"lto_module_get_symbol_attribute")
		->args({"mod","index"});
// from D:\Work\libclang\include\llvm-c/lto.h:298:1
	addExtern< const char * (*)(LLVMOpaqueLTOModule *) , lto_module_get_linkeropts >(*this,lib,"lto_module_get_linkeropts",SideEffects::worstDefault,"lto_module_get_linkeropts")
		->args({"mod"});
}
}

