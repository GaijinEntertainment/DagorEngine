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
// from imgui/imgui.h:327:29
	makeExtern< ImGuiContext * (*)(ImFontAtlas *) , ImGui::CreateContext , SimNode_ExtFuncCall , imguiTempFn>(lib,"CreateContext","ImGui::CreateContext")
		->args({"shared_font_atlas"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:328:29
	makeExtern< void (*)(ImGuiContext *) , ImGui::DestroyContext , SimNode_ExtFuncCall , imguiTempFn>(lib,"DestroyContext","ImGui::DestroyContext")
		->args({"ctx"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:329:29
	makeExtern< ImGuiContext * (*)() , ImGui::GetCurrentContext , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCurrentContext","ImGui::GetCurrentContext")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:330:29
	makeExtern< void (*)(ImGuiContext *) , ImGui::SetCurrentContext , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetCurrentContext","ImGui::SetCurrentContext")
		->args({"ctx"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:333:29
	makeExtern< ImGuiIO & (*)() , ImGui::GetIO , SimNode_ExtFuncCallRef , imguiTempFn>(lib,"GetIO","ImGui::GetIO")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:334:29
	makeExtern< ImGuiStyle & (*)() , ImGui::GetStyle , SimNode_ExtFuncCallRef , imguiTempFn>(lib,"GetStyle","ImGui::GetStyle")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:335:29
	makeExtern< void (*)() , ImGui::NewFrame , SimNode_ExtFuncCall , imguiTempFn>(lib,"NewFrame","ImGui::NewFrame")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:336:29
	makeExtern< void (*)() , ImGui::EndFrame , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndFrame","ImGui::EndFrame")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:337:29
	makeExtern< void (*)() , ImGui::Render , SimNode_ExtFuncCall , imguiTempFn>(lib,"Render","ImGui::Render")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:338:29
	makeExtern< ImDrawData * (*)() , ImGui::GetDrawData , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetDrawData","ImGui::GetDrawData")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:341:29
	makeExtern< void (*)(bool *) , ImGui::ShowDemoWindow , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowDemoWindow","ImGui::ShowDemoWindow")
		->args({"p_open"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:342:29
	makeExtern< void (*)(bool *) , ImGui::ShowMetricsWindow , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowMetricsWindow","ImGui::ShowMetricsWindow")
		->args({"p_open"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:343:29
	makeExtern< void (*)(bool *) , ImGui::ShowDebugLogWindow , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowDebugLogWindow","ImGui::ShowDebugLogWindow")
		->args({"p_open"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:344:29
	makeExtern< void (*)(bool *) , ImGui::ShowIDStackToolWindow , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowIDStackToolWindow","ImGui::ShowIDStackToolWindow")
		->args({"p_open"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:345:29
	makeExtern< void (*)(bool *) , ImGui::ShowAboutWindow , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowAboutWindow","ImGui::ShowAboutWindow")
		->args({"p_open"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:346:29
	makeExtern< void (*)(ImGuiStyle *) , ImGui::ShowStyleEditor , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowStyleEditor","ImGui::ShowStyleEditor")
		->args({"ref"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:347:29
	makeExtern< bool (*)(const char *) , ImGui::ShowStyleSelector , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowStyleSelector","ImGui::ShowStyleSelector")
		->args({"label"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:348:29
	makeExtern< void (*)(const char *) , ImGui::ShowFontSelector , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowFontSelector","ImGui::ShowFontSelector")
		->args({"label"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:349:29
	makeExtern< void (*)() , ImGui::ShowUserGuide , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowUserGuide","ImGui::ShowUserGuide")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:350:29
	makeExtern< const char * (*)() , ImGui::GetVersion , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetVersion","ImGui::GetVersion")
		->addToModule(*this, SideEffects::worstDefault);
}
}

