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
void Module_dasIMGUI::initFunctions_5() {
// from imgui/imgui.h:544:29
	makeExtern< void (*)(int,float) , ImGui::PushStyleVarY , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushStyleVarY","ImGui::PushStyleVarY")
		->args({"idx","val_y"})
		->arg_type(0,makeType<ImGuiStyleVar_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:545:29
	makeExtern< void (*)(int) , ImGui::PopStyleVar , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopStyleVar","ImGui::PopStyleVar")
		->args({"count"})
		->arg_init(0,make_smart<ExprConstInt>(1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:546:29
	makeExtern< void (*)(int,bool) , ImGui::PushItemFlag , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushItemFlag","ImGui::PushItemFlag")
		->args({"option","enabled"})
		->arg_type(0,makeType<ImGuiItemFlags_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:547:29
	makeExtern< void (*)() , ImGui::PopItemFlag , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopItemFlag","ImGui::PopItemFlag")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:550:29
	makeExtern< void (*)(float) , ImGui::PushItemWidth , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushItemWidth","ImGui::PushItemWidth")
		->args({"item_width"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:551:29
	makeExtern< void (*)() , ImGui::PopItemWidth , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopItemWidth","ImGui::PopItemWidth")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:552:29
	makeExtern< void (*)(float) , ImGui::SetNextItemWidth , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextItemWidth","ImGui::SetNextItemWidth")
		->args({"item_width"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:553:29
	makeExtern< float (*)() , ImGui::CalcItemWidth , SimNode_ExtFuncCall , imguiTempFn>(lib,"CalcItemWidth","ImGui::CalcItemWidth")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:554:29
	makeExtern< void (*)(float) , ImGui::PushTextWrapPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushTextWrapPos","ImGui::PushTextWrapPos")
		->args({"wrap_local_pos_x"})
		->arg_init(0,make_smart<ExprConstFloat>(0))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:555:29
	makeExtern< void (*)() , ImGui::PopTextWrapPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopTextWrapPos","ImGui::PopTextWrapPos")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:559:29
	makeExtern< ImVec2 (*)() , ImGui::GetFontTexUvWhitePixel , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFontTexUvWhitePixel","ImGui::GetFontTexUvWhitePixel")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:560:29
	makeExtern< unsigned int (*)(int,float) , ImGui::GetColorU32 , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColorU32","ImGui::GetColorU32")
		->args({"idx","alpha_mul"})
		->arg_type(0,makeType<ImGuiCol_>(lib))
		->arg_init(1,make_smart<ExprConstFloat>(1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:561:29
	makeExtern< unsigned int (*)(const ImVec4 &) , ImGui::GetColorU32 , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColorU32","ImGui::GetColorU32")
		->args({"col"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:562:29
	makeExtern< unsigned int (*)(unsigned int,float) , ImGui::GetColorU32 , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColorU32","ImGui::GetColorU32")
		->args({"col","alpha_mul"})
		->arg_init(1,make_smart<ExprConstFloat>(1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:563:29
	makeExtern< const ImVec4 & (*)(int) , ImGui::GetStyleColorVec4 , SimNode_ExtFuncCallRef , imguiTempFn>(lib,"GetStyleColorVec4","ImGui::GetStyleColorVec4")
		->args({"idx"})
		->arg_type(0,makeType<ImGuiCol_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:575:29
	makeExtern< ImVec2 (*)() , ImGui::GetCursorScreenPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCursorScreenPos","ImGui::GetCursorScreenPos")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:576:29
	makeExtern< void (*)(const ImVec2 &) , ImGui::SetCursorScreenPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetCursorScreenPos","ImGui::SetCursorScreenPos")
		->args({"pos"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:577:29
	makeExtern< ImVec2 (*)() , ImGui::GetContentRegionAvail , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetContentRegionAvail","ImGui::GetContentRegionAvail")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:578:29
	makeExtern< ImVec2 (*)() , ImGui::GetCursorPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCursorPos","ImGui::GetCursorPos")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:579:29
	makeExtern< float (*)() , ImGui::GetCursorPosX , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCursorPosX","ImGui::GetCursorPosX")
		->addToModule(*this, SideEffects::worstDefault);
}
}

