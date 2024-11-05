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
void Module_dasLLVM::initFunctions_56() {
// from D:\Work\libclang\include\llvm-c/lto.h:618:20
	makeExtern< LLVMOpaqueLTOInput * (*)(const void *,size_t,const char *) , lto_input_create , SimNode_ExtFuncCall >(lib,"lto_input_create","lto_input_create")
		->args({"buffer","buffer_size","path"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:628:13
	makeExtern< void (*)(LLVMOpaqueLTOInput *) , lto_input_dispose , SimNode_ExtFuncCall >(lib,"lto_input_dispose","lto_input_dispose")
		->args({"input"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:636:17
	makeExtern< unsigned int (*)(LLVMOpaqueLTOInput *) , lto_input_get_num_dependent_libraries , SimNode_ExtFuncCall >(lib,"lto_input_get_num_dependent_libraries","lto_input_get_num_dependent_libraries")
		->args({"input"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:645:21
	makeExtern< const char * (*)(LLVMOpaqueLTOInput *,size_t,size_t *) , lto_input_get_dependent_library , SimNode_ExtFuncCall >(lib,"lto_input_get_dependent_library","lto_input_get_dependent_library")
		->args({"input","index","size"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:655:27
	makeExtern< const char *const * (*)(size_t *) , lto_runtime_lib_symbols_list , SimNode_ExtFuncCall >(lib,"lto_runtime_lib_symbols_list","lto_runtime_lib_symbols_list")
		->args({"size"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:687:27
	makeExtern< LLVMOpaqueThinLTOCodeGenerator * (*)() , thinlto_create_codegen , SimNode_ExtFuncCall >(lib,"thinlto_create_codegen","thinlto_create_codegen")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:695:13
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *) , thinlto_codegen_dispose , SimNode_ExtFuncCall >(lib,"thinlto_codegen_dispose","thinlto_codegen_dispose")
		->args({"cg"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:708:13
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,const char *,const char *,int) , thinlto_codegen_add_module , SimNode_ExtFuncCall >(lib,"thinlto_codegen_add_module","thinlto_codegen_add_module")
		->args({"cg","identifier","data","length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:718:13
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *) , thinlto_codegen_process , SimNode_ExtFuncCall >(lib,"thinlto_codegen_process","thinlto_codegen_process")
		->args({"cg"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:729:21
	makeExtern< unsigned int (*)(LLVMOpaqueThinLTOCodeGenerator *) , thinlto_module_get_num_objects , SimNode_ExtFuncCall >(lib,"thinlto_module_get_num_objects","thinlto_module_get_num_objects")
		->args({"cg"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:740:24
	makeExtern< LTOObjectBuffer (*)(LLVMOpaqueThinLTOCodeGenerator *,unsigned int) , thinlto_module_get_object , SimNode_ExtFuncCallAndCopyOrMove >(lib,"thinlto_module_get_object","thinlto_module_get_object")
		->args({"cg","index"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:752:14
	makeExtern< unsigned int (*)(LLVMOpaqueThinLTOCodeGenerator *) , thinlto_module_get_num_object_files , SimNode_ExtFuncCall >(lib,"thinlto_module_get_num_object_files","thinlto_module_get_num_object_files")
		->args({"cg"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:763:13
	makeExtern< const char * (*)(LLVMOpaqueThinLTOCodeGenerator *,unsigned int) , thinlto_module_get_object_file , SimNode_ExtFuncCall >(lib,"thinlto_module_get_object_file","thinlto_module_get_object_file")
		->args({"cg","index"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:772:19
	makeExtern< bool (*)(LLVMOpaqueThinLTOCodeGenerator *,lto_codegen_model) , thinlto_codegen_set_pic_model , SimNode_ExtFuncCall >(lib,"thinlto_codegen_set_pic_model","thinlto_codegen_set_pic_model")
		->args({"cg",""})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:782:13
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,const char *) , thinlto_codegen_set_savetemps_dir , SimNode_ExtFuncCall >(lib,"thinlto_codegen_set_savetemps_dir","thinlto_codegen_set_savetemps_dir")
		->args({"cg","save_temps_dir"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:793:6
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,const char *) , thinlto_set_generated_objects_dir , SimNode_ExtFuncCall >(lib,"thinlto_set_generated_objects_dir","thinlto_set_generated_objects_dir")
		->args({"cg","save_temps_dir"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:801:13
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,const char *) , thinlto_codegen_set_cpu , SimNode_ExtFuncCall >(lib,"thinlto_codegen_set_cpu","thinlto_codegen_set_cpu")
		->args({"cg","cpu"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:809:13
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,bool) , thinlto_codegen_disable_codegen , SimNode_ExtFuncCall >(lib,"thinlto_codegen_disable_codegen","thinlto_codegen_disable_codegen")
		->args({"cg","disable"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:817:13
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,bool) , thinlto_codegen_set_codegen_only , SimNode_ExtFuncCall >(lib,"thinlto_codegen_set_codegen_only","thinlto_codegen_set_codegen_only")
		->args({"cg","codegen_only"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:825:13
	makeExtern< void (*)(const char *const *,int) , thinlto_debug_options , SimNode_ExtFuncCall >(lib,"thinlto_debug_options","thinlto_debug_options")
		->args({"options","number"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

