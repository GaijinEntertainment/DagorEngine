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
void Module_dasIMGUI::initFunctions_1() {
	addCtorAndUsing<ImTextureRef>(*this,lib,"ImTextureRef","ImTextureRef");
	addCtorAndUsing<ImTextureRef,void *>(*this,lib,"ImTextureRef","ImTextureRef")
		->args({"tex_id"});
	using _method_1 = das::das_call_member< void * (ImTextureRef::*)() const,&ImTextureRef::GetTexID >;
// from imgui/imgui.h:380:25
	makeExtern<DAS_CALL_METHOD(_method_1), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTexID","das_call_member< void * (ImTextureRef::*)() const , &ImTextureRef::GetTexID >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:399:29
	makeExtern< ImGuiContext * (*)(ImFontAtlas *) , ImGui::CreateContext , SimNode_ExtFuncCall , imguiTempFn>(lib,"CreateContext","ImGui::CreateContext")
		->args({"shared_font_atlas"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:400:29
	makeExtern< void (*)(ImGuiContext *) , ImGui::DestroyContext , SimNode_ExtFuncCall , imguiTempFn>(lib,"DestroyContext","ImGui::DestroyContext")
		->args({"ctx"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:401:29
	makeExtern< ImGuiContext * (*)() , ImGui::GetCurrentContext , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCurrentContext","ImGui::GetCurrentContext")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:402:29
	makeExtern< void (*)(ImGuiContext *) , ImGui::SetCurrentContext , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetCurrentContext","ImGui::SetCurrentContext")
		->args({"ctx"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:405:29
	makeExtern< ImGuiIO & (*)() , ImGui::GetIO , SimNode_ExtFuncCallRef , imguiTempFn>(lib,"GetIO","ImGui::GetIO")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:406:32
	makeExtern< ImGuiPlatformIO & (*)() , ImGui::GetPlatformIO , SimNode_ExtFuncCallRef , imguiTempFn>(lib,"GetPlatformIO","ImGui::GetPlatformIO")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:407:29
	makeExtern< ImGuiStyle & (*)() , ImGui::GetStyle , SimNode_ExtFuncCallRef , imguiTempFn>(lib,"GetStyle","ImGui::GetStyle")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:408:29
	makeExtern< void (*)() , ImGui::NewFrame , SimNode_ExtFuncCall , imguiTempFn>(lib,"NewFrame","ImGui::NewFrame")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:409:29
	makeExtern< void (*)() , ImGui::EndFrame , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndFrame","ImGui::EndFrame")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:410:29
	makeExtern< void (*)() , ImGui::Render , SimNode_ExtFuncCall , imguiTempFn>(lib,"Render","ImGui::Render")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:411:29
	makeExtern< ImDrawData * (*)() , ImGui::GetDrawData , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetDrawData","ImGui::GetDrawData")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:414:29
	makeExtern< void (*)(bool *) , ImGui::ShowDemoWindow , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowDemoWindow","ImGui::ShowDemoWindow")
		->args({"p_open"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:415:29
	makeExtern< void (*)(bool *) , ImGui::ShowMetricsWindow , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowMetricsWindow","ImGui::ShowMetricsWindow")
		->args({"p_open"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:416:29
	makeExtern< void (*)(bool *) , ImGui::ShowDebugLogWindow , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowDebugLogWindow","ImGui::ShowDebugLogWindow")
		->args({"p_open"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:417:29
	makeExtern< void (*)(bool *) , ImGui::ShowIDStackToolWindow , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowIDStackToolWindow","ImGui::ShowIDStackToolWindow")
		->args({"p_open"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:418:29
	makeExtern< void (*)(bool *) , ImGui::ShowAboutWindow , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowAboutWindow","ImGui::ShowAboutWindow")
		->args({"p_open"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:419:29
	makeExtern< void (*)(ImGuiStyle *) , ImGui::ShowStyleEditor , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowStyleEditor","ImGui::ShowStyleEditor")
		->args({"ref"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
}
}

