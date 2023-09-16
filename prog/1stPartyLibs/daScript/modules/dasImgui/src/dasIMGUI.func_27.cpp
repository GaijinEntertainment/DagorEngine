// this file is generated via daScript automatic C++ binder
// all user modifications will be lost after this file is re-generated

#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/bind_enum.h"
#include "dasIMGUI.h"
#include "need_dasIMGUI.h"
namespace das {
#include "dasIMGUI.func.aot.decl.inc"
void Module_dasIMGUI::initFunctions_27() {
	addCtorAndUsing<ImGuiViewport>(*this,lib,"ImGuiViewport","ImGuiViewport");
	using _method_165 = das::das_call_member< ImVec2 (ImGuiViewport::*)() const,&ImGuiViewport::GetCenter >;
	makeExtern<DAS_CALL_METHOD(_method_165), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCenter","das_call_member< ImVec2 (ImGuiViewport::*)() const , &ImGuiViewport::GetCenter >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_166 = das::das_call_member< ImVec2 (ImGuiViewport::*)() const,&ImGuiViewport::GetWorkCenter >;
	makeExtern<DAS_CALL_METHOD(_method_166), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWorkCenter","das_call_member< ImVec2 (ImGuiViewport::*)() const , &ImGuiViewport::GetWorkCenter >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

