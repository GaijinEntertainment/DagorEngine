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
// from imgui/imgui.h:460:29
	makeExtern< void (*)(float) , ImGui::PushTextWrapPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushTextWrapPos","ImGui::PushTextWrapPos")
		->args({"wrap_local_pos_x"})
		->arg_init(0,make_smart<ExprConstFloat>(0))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:461:29
	makeExtern< void (*)() , ImGui::PopTextWrapPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopTextWrapPos","ImGui::PopTextWrapPos")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:465:29
	makeExtern< ImFont * (*)() , ImGui::GetFont , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFont","ImGui::GetFont")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:466:29
	makeExtern< float (*)() , ImGui::GetFontSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFontSize","ImGui::GetFontSize")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:467:29
	makeExtern< ImVec2 (*)() , ImGui::GetFontTexUvWhitePixel , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFontTexUvWhitePixel","ImGui::GetFontTexUvWhitePixel")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:468:29
	makeExtern< unsigned int (*)(int,float) , ImGui::GetColorU32 , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColorU32","ImGui::GetColorU32")
		->args({"idx","alpha_mul"})
		->arg_type(0,makeType<ImGuiCol_>(lib))
		->arg_init(1,make_smart<ExprConstFloat>(1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:469:29
	makeExtern< unsigned int (*)(const ImVec4 &) , ImGui::GetColorU32 , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColorU32","ImGui::GetColorU32")
		->args({"col"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:470:29
	makeExtern< unsigned int (*)(unsigned int,float) , ImGui::GetColorU32 , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColorU32","ImGui::GetColorU32")
		->args({"col","alpha_mul"})
		->arg_init(1,make_smart<ExprConstFloat>(1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:471:29
	makeExtern< const ImVec4 & (*)(int) , ImGui::GetStyleColorVec4 , SimNode_ExtFuncCallRef , imguiTempFn>(lib,"GetStyleColorVec4","ImGui::GetStyleColorVec4")
		->args({"idx"})
		->arg_type(0,makeType<ImGuiCol_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:483:29
	makeExtern< ImVec2 (*)() , ImGui::GetCursorScreenPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCursorScreenPos","ImGui::GetCursorScreenPos")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:484:29
	makeExtern< void (*)(const ImVec2 &) , ImGui::SetCursorScreenPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetCursorScreenPos","ImGui::SetCursorScreenPos")
		->args({"pos"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:485:29
	makeExtern< ImVec2 (*)() , ImGui::GetContentRegionAvail , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetContentRegionAvail","ImGui::GetContentRegionAvail")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:486:29
	makeExtern< ImVec2 (*)() , ImGui::GetCursorPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCursorPos","ImGui::GetCursorPos")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:487:29
	makeExtern< float (*)() , ImGui::GetCursorPosX , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCursorPosX","ImGui::GetCursorPosX")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:488:29
	makeExtern< float (*)() , ImGui::GetCursorPosY , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCursorPosY","ImGui::GetCursorPosY")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:489:29
	makeExtern< void (*)(const ImVec2 &) , ImGui::SetCursorPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetCursorPos","ImGui::SetCursorPos")
		->args({"local_pos"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:490:29
	makeExtern< void (*)(float) , ImGui::SetCursorPosX , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetCursorPosX","ImGui::SetCursorPosX")
		->args({"local_x"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:491:29
	makeExtern< void (*)(float) , ImGui::SetCursorPosY , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetCursorPosY","ImGui::SetCursorPosY")
		->args({"local_y"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:492:29
	makeExtern< ImVec2 (*)() , ImGui::GetCursorStartPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCursorStartPos","ImGui::GetCursorStartPos")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:495:29
	makeExtern< void (*)() , ImGui::Separator , SimNode_ExtFuncCall , imguiTempFn>(lib,"Separator","ImGui::Separator")
		->addToModule(*this, SideEffects::worstDefault);
}
}

