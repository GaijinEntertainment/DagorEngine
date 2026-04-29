#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "dasLLVM.h"

namespace das {

float2 test_abi_mad2 ( float2 a, float2 b, float2 c ) {
    return v_add(v_mul(a,b),c);
}

float3 test_abi_mad3 ( float3 a, float3 b, float3 c ) {
    return v_add(v_mul(a,b),c);
}

float4 test_abi_mad4 ( float4 a, float4 b, float4 c ) {
    return v_add(v_mul(a,b),c);
}

Func test_abi_func ( Func a, Context * ctx ) {
    bool found = false;
    for ( int i=0; i!=ctx->getTotalFunctions(); ++i ) {
        if ( ctx->getFunction(i)==a.PTR ) {
            found = true;
            break;
        }
    }
    if ( !found ) ctx->throw_error("function not found");
    return a;
}

/*
void diagnosticHandler(LLVMDiagnosticInfoRef DI, void *) {
    int ll = LogLevel::info;
    switch (LLVMGetDiagInfoSeverity(DI)) {
    case LLVMDSError:   ll = LogLevel::error; break;
    case LLVMDSWarning: ll = LogLevel::warning; break;
    case LLVMDSRemark:  ll = LogLevel::info; break;
    case LLVMDSNote:    ll = LogLevel::debug; break;
    }
    auto msg = LLVMGetDiagInfoDescription(DI);
    LOG(ll) << msg << "\n";
    LLVMDisposeMessage(msg);
}

void set_context_diagnostics_to_log ( LLVMContextRef ctx ) {
    LLVMContextSetDiagnosticHandler(ctx, diagnosticHandler, nullptr);
}
*/

Module_dasLLVM::Module_dasLLVM() : Module("llvm") {
}
bool Module_dasLLVM::initDependencies() {
	if ( initialized ) return true;
	initialized = true;
	lib.addModule(this);
	lib.addBuiltInModule();
	initMain();
	return true;
}

void Module_dasLLVM::initMain() {
    // abi tests
	addExtern<DAS_BIND_FUN(test_abi_mad2) >(*this,lib,"test_abi_mad",SideEffects::worstDefault,"test_abi_mad2")
		->args({"a","b","c"});
	addExtern<DAS_BIND_FUN(test_abi_mad3) >(*this,lib,"test_abi_mad",SideEffects::worstDefault,"test_abi_mad3")
		->args({"a","b","c"});
	addExtern<DAS_BIND_FUN(test_abi_mad4) >(*this,lib,"test_abi_mad",SideEffects::worstDefault,"test_abi_mad4")
		->args({"a","b","c"});
	addExtern<DAS_BIND_FUN(test_abi_func) >(*this,lib,"test_abi_func",SideEffects::worstDefault,"test_abi_func")
		->args({"fn","context"});
}

ModuleAotType Module_dasLLVM::aotRequire ( TextWriter & tw ) const {
    return ModuleAotType::cpp;
}

}

REGISTER_MODULE_IN_NAMESPACE(Module_dasLLVM,das);


