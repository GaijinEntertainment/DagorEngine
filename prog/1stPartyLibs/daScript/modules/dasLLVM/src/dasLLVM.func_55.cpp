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
void Module_dasLLVM::initFunctions_55() {
// from D:\Work\libclang\include\llvm-c/lto.h:404:1
	makeExtern< bool (*)(LLVMOpaqueLTOCodeGenerator *,LLVMOpaqueLTOModule *) , lto_codegen_add_module , SimNode_ExtFuncCall >(lib,"lto_codegen_add_module","lto_codegen_add_module")
		->args({"cg","mod"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:415:1
	makeExtern< void (*)(LLVMOpaqueLTOCodeGenerator *,LLVMOpaqueLTOModule *) , lto_codegen_set_module , SimNode_ExtFuncCall >(lib,"lto_codegen_set_module","lto_codegen_set_module")
		->args({"cg","mod"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:424:1
	makeExtern< bool (*)(LLVMOpaqueLTOCodeGenerator *,lto_debug_model) , lto_codegen_set_debug_model , SimNode_ExtFuncCall >(lib,"lto_codegen_set_debug_model","lto_codegen_set_debug_model")
		->args({"cg",""})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:433:1
	makeExtern< bool (*)(LLVMOpaqueLTOCodeGenerator *,lto_codegen_model) , lto_codegen_set_pic_model , SimNode_ExtFuncCall >(lib,"lto_codegen_set_pic_model","lto_codegen_set_pic_model")
		->args({"cg",""})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:441:1
	makeExtern< void (*)(LLVMOpaqueLTOCodeGenerator *,const char *) , lto_codegen_set_cpu , SimNode_ExtFuncCall >(lib,"lto_codegen_set_cpu","lto_codegen_set_cpu")
		->args({"cg","cpu"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:450:1
	makeExtern< void (*)(LLVMOpaqueLTOCodeGenerator *,const char *) , lto_codegen_set_assembler_path , SimNode_ExtFuncCall >(lib,"lto_codegen_set_assembler_path","lto_codegen_set_assembler_path")
		->args({"cg","path"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:458:1
	makeExtern< void (*)(LLVMOpaqueLTOCodeGenerator *,const char **,int) , lto_codegen_set_assembler_args , SimNode_ExtFuncCall >(lib,"lto_codegen_set_assembler_args","lto_codegen_set_assembler_args")
		->args({"cg","args","nargs"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:469:1
	makeExtern< void (*)(LLVMOpaqueLTOCodeGenerator *,const char *) , lto_codegen_add_must_preserve_symbol , SimNode_ExtFuncCall >(lib,"lto_codegen_add_must_preserve_symbol","lto_codegen_add_must_preserve_symbol")
		->args({"cg","symbol"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:479:1
	makeExtern< bool (*)(LLVMOpaqueLTOCodeGenerator *,const char *) , lto_codegen_write_merged_modules , SimNode_ExtFuncCall >(lib,"lto_codegen_write_merged_modules","lto_codegen_write_merged_modules")
		->args({"cg","path"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:494:1
	makeExtern< const void * (*)(LLVMOpaqueLTOCodeGenerator *,size_t *) , lto_codegen_compile , SimNode_ExtFuncCall >(lib,"lto_codegen_compile","lto_codegen_compile")
		->args({"cg","length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:506:1
	makeExtern< bool (*)(LLVMOpaqueLTOCodeGenerator *,const char **) , lto_codegen_compile_to_file , SimNode_ExtFuncCall >(lib,"lto_codegen_compile_to_file","lto_codegen_compile_to_file")
		->args({"cg","name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:514:1
	makeExtern< bool (*)(LLVMOpaqueLTOCodeGenerator *) , lto_codegen_optimize , SimNode_ExtFuncCall >(lib,"lto_codegen_optimize","lto_codegen_optimize")
		->args({"cg"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:529:1
	makeExtern< const void * (*)(LLVMOpaqueLTOCodeGenerator *,size_t *) , lto_codegen_compile_optimized , SimNode_ExtFuncCall >(lib,"lto_codegen_compile_optimized","lto_codegen_compile_optimized")
		->args({"cg","length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:537:1
	makeExtern< unsigned int (*)() , lto_api_version , SimNode_ExtFuncCall >(lib,"lto_api_version","lto_api_version")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:551:13
	makeExtern< void (*)(const char *const *,int) , lto_set_debug_options , SimNode_ExtFuncCall >(lib,"lto_set_debug_options","lto_set_debug_options")
		->args({"options","number"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:565:1
	makeExtern< void (*)(LLVMOpaqueLTOCodeGenerator *,const char *) , lto_codegen_debug_options , SimNode_ExtFuncCall >(lib,"lto_codegen_debug_options","lto_codegen_debug_options")
		->args({"cg",""})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:573:13
	makeExtern< void (*)(LLVMOpaqueLTOCodeGenerator *,const char *const *,int) , lto_codegen_debug_options_array , SimNode_ExtFuncCall >(lib,"lto_codegen_debug_options_array","lto_codegen_debug_options_array")
		->args({"cg","","number"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:583:1
	makeExtern< void (*)() , lto_initialize_disassembler , SimNode_ExtFuncCall >(lib,"lto_initialize_disassembler","lto_initialize_disassembler")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:592:1
	makeExtern< void (*)(LLVMOpaqueLTOCodeGenerator *,bool) , lto_codegen_set_should_internalize , SimNode_ExtFuncCall >(lib,"lto_codegen_set_should_internalize","lto_codegen_set_should_internalize")
		->args({"cg","ShouldInternalize"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:604:1
	makeExtern< void (*)(LLVMOpaqueLTOCodeGenerator *,bool) , lto_codegen_set_should_embed_uselists , SimNode_ExtFuncCall >(lib,"lto_codegen_set_should_embed_uselists","lto_codegen_set_should_embed_uselists")
		->args({"cg","ShouldEmbedUselists"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

