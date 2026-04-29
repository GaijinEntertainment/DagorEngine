// this file is generated via Daslang automatic C++ binder
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
void Module_dasIMGUI::initFunctions_31() {
	using _method_197 = das::das_call_member< void (ImFont::*)(),&ImFont::ClearOutputData >;
// from imgui/imgui.h:4027:33
	makeExtern<DAS_CALL_METHOD(_method_197), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearOutputData","das_call_member< void (ImFont::*)() , &ImFont::ClearOutputData >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_198 = das::das_call_member< void (ImFont::*)(unsigned short,unsigned short),&ImFont::AddRemapChar >;
// from imgui/imgui.h:4028:33
	makeExtern<DAS_CALL_METHOD(_method_198), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddRemapChar","das_call_member< void (ImFont::*)(unsigned short,unsigned short) , &ImFont::AddRemapChar >::invoke")
		->args({"self","from_codepoint","to_codepoint"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_199 = das::das_call_member< bool (ImFont::*)(unsigned int,unsigned int),&ImFont::IsGlyphRangeUnused >;
// from imgui/imgui.h:4029:33
	makeExtern<DAS_CALL_METHOD(_method_199), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsGlyphRangeUnused","das_call_member< bool (ImFont::*)(unsigned int,unsigned int) , &ImFont::IsGlyphRangeUnused >::invoke")
		->args({"self","c_begin","c_last"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiViewport>(*this,lib,"ImGuiViewport","ImGuiViewport");
	using _method_200 = das::das_call_member< ImVec2 (ImGuiViewport::*)() const,&ImGuiViewport::GetCenter >;
// from imgui/imgui.h:4115:25
	makeExtern<DAS_CALL_METHOD(_method_200), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCenter","das_call_member< ImVec2 (ImGuiViewport::*)() const , &ImGuiViewport::GetCenter >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_201 = das::das_call_member< ImVec2 (ImGuiViewport::*)() const,&ImGuiViewport::GetWorkCenter >;
// from imgui/imgui.h:4116:25
	makeExtern<DAS_CALL_METHOD(_method_201), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWorkCenter","das_call_member< ImVec2 (ImGuiViewport::*)() const , &ImGuiViewport::GetWorkCenter >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiPlatformIO>(*this,lib,"ImGuiPlatformIO","ImGuiPlatformIO");
	using _method_202 = das::das_call_member< void (ImGuiPlatformIO::*)(),&ImGuiPlatformIO::ClearPlatformHandlers >;
// from imgui/imgui.h:4276:20
	makeExtern<DAS_CALL_METHOD(_method_202), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearPlatformHandlers","das_call_member< void (ImGuiPlatformIO::*)() , &ImGuiPlatformIO::ClearPlatformHandlers >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_203 = das::das_call_member< void (ImGuiPlatformIO::*)(),&ImGuiPlatformIO::ClearRendererHandlers >;
// from imgui/imgui.h:4277:20
	makeExtern<DAS_CALL_METHOD(_method_203), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearRendererHandlers","das_call_member< void (ImGuiPlatformIO::*)() , &ImGuiPlatformIO::ClearRendererHandlers >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiPlatformMonitor>(*this,lib,"ImGuiPlatformMonitor","ImGuiPlatformMonitor");
	addCtorAndUsing<ImGuiPlatformImeData>(*this,lib,"ImGuiPlatformImeData","ImGuiPlatformImeData");
}
}

