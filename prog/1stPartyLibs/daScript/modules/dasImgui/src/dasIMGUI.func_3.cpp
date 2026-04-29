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
void Module_dasIMGUI::initFunctions_3() {
// from imgui/imgui.h:477:29
	makeExtern< float (*)() , ImGui::GetWindowWidth , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowWidth","ImGui::GetWindowWidth")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:478:29
	makeExtern< float (*)() , ImGui::GetWindowHeight , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowHeight","ImGui::GetWindowHeight")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:479:29
	makeExtern< ImGuiViewport * (*)() , ImGui::GetWindowViewport , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowViewport","ImGui::GetWindowViewport")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:483:29
	makeExtern< void (*)(const ImVec2 &,int,const ImVec2 &) , ImGui::SetNextWindowPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowPos","ImGui::SetNextWindowPos")
		->args({"pos","cond","pivot"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:484:29
	makeExtern< void (*)(const ImVec2 &,int) , ImGui::SetNextWindowSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowSize","ImGui::SetNextWindowSize")
		->args({"size","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:486:29
	makeExtern< void (*)(const ImVec2 &) , ImGui::SetNextWindowContentSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowContentSize","ImGui::SetNextWindowContentSize")
		->args({"size"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:487:29
	makeExtern< void (*)(bool,int) , ImGui::SetNextWindowCollapsed , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowCollapsed","ImGui::SetNextWindowCollapsed")
		->args({"collapsed","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:488:29
	makeExtern< void (*)() , ImGui::SetNextWindowFocus , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowFocus","ImGui::SetNextWindowFocus")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:489:29
	makeExtern< void (*)(const ImVec2 &) , ImGui::SetNextWindowScroll , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowScroll","ImGui::SetNextWindowScroll")
		->args({"scroll"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:490:29
	makeExtern< void (*)(float) , ImGui::SetNextWindowBgAlpha , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowBgAlpha","ImGui::SetNextWindowBgAlpha")
		->args({"alpha"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:491:29
	makeExtern< void (*)(unsigned int) , ImGui::SetNextWindowViewport , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowViewport","ImGui::SetNextWindowViewport")
		->args({"viewport_id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:492:29
	makeExtern< void (*)(const ImVec2 &,int) , ImGui::SetWindowPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowPos","ImGui::SetWindowPos")
		->args({"pos","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:493:29
	makeExtern< void (*)(const ImVec2 &,int) , ImGui::SetWindowSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowSize","ImGui::SetWindowSize")
		->args({"size","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:494:29
	makeExtern< void (*)(bool,int) , ImGui::SetWindowCollapsed , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowCollapsed","ImGui::SetWindowCollapsed")
		->args({"collapsed","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:495:29
	makeExtern< void (*)() , ImGui::SetWindowFocus , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowFocus","ImGui::SetWindowFocus")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:496:29
	makeExtern< void (*)(const char *,const ImVec2 &,int) , ImGui::SetWindowPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowPos","ImGui::SetWindowPos")
		->args({"name","pos","cond"})
		->arg_type(2,makeType<ImGuiCond_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:497:29
	makeExtern< void (*)(const char *,const ImVec2 &,int) , ImGui::SetWindowSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowSize","ImGui::SetWindowSize")
		->args({"name","size","cond"})
		->arg_type(2,makeType<ImGuiCond_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:498:29
	makeExtern< void (*)(const char *,bool,int) , ImGui::SetWindowCollapsed , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowCollapsed","ImGui::SetWindowCollapsed")
		->args({"name","collapsed","cond"})
		->arg_type(2,makeType<ImGuiCond_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:499:29
	makeExtern< void (*)(const char *) , ImGui::SetWindowFocus , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowFocus","ImGui::SetWindowFocus")
		->args({"name"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:504:29
	makeExtern< float (*)() , ImGui::GetScrollX , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetScrollX","ImGui::GetScrollX")
		->addToModule(*this, SideEffects::worstDefault);
}
}

