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
void Module_dasIMGUI::initFunctions_7() {
// from imgui/imgui.h:614:29
	makeExtern< void (*)(const char *,const char *) , ImGui::PushID , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushID","ImGui::PushID")
		->args({"str_id_begin","str_id_end"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:615:29
	makeExtern< void (*)(const void *) , ImGui::PushID , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushID","ImGui::PushID")
		->args({"ptr_id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:616:29
	makeExtern< void (*)(int) , ImGui::PushID , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushID","ImGui::PushID")
		->args({"int_id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:617:29
	makeExtern< void (*)() , ImGui::PopID , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopID","ImGui::PopID")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:618:29
	makeExtern< unsigned int (*)(const char *) , ImGui::GetID , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetID","ImGui::GetID")
		->args({"str_id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:619:29
	makeExtern< unsigned int (*)(const char *,const char *) , ImGui::GetID , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetID","ImGui::GetID")
		->args({"str_id_begin","str_id_end"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:620:29
	makeExtern< unsigned int (*)(const void *) , ImGui::GetID , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetID","ImGui::GetID")
		->args({"ptr_id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:621:29
	makeExtern< unsigned int (*)(int) , ImGui::GetID , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetID","ImGui::GetID")
		->args({"int_id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:637:29
	makeExtern< void (*)(const char *) , ImGui::SeparatorText , SimNode_ExtFuncCall , imguiTempFn>(lib,"SeparatorText","ImGui::SeparatorText")
		->args({"label"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:642:29
	makeExtern< bool (*)(const char *,const ImVec2 &) , ImGui::Button , SimNode_ExtFuncCall , imguiTempFn>(lib,"Button","ImGui::Button")
		->args({"label","size"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:643:29
	makeExtern< bool (*)(const char *) , ImGui::SmallButton , SimNode_ExtFuncCall , imguiTempFn>(lib,"SmallButton","ImGui::SmallButton")
		->args({"label"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:644:29
	makeExtern< bool (*)(const char *,const ImVec2 &,int) , ImGui::InvisibleButton , SimNode_ExtFuncCall , imguiTempFn>(lib,"InvisibleButton","ImGui::InvisibleButton")
		->args({"str_id","size","flags"})
		->arg_type(2,makeType<ImGuiButtonFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiButtonFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:645:29
	makeExtern< bool (*)(const char *,ImGuiDir) , ImGui::ArrowButton , SimNode_ExtFuncCall , imguiTempFn>(lib,"ArrowButton","ImGui::ArrowButton")
		->args({"str_id","dir"})
		->arg_type(1,makeType<ImGuiDir>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:646:29
	makeExtern< bool (*)(const char *,bool *) , ImGui::Checkbox , SimNode_ExtFuncCall , imguiTempFn>(lib,"Checkbox","ImGui::Checkbox")
		->args({"label","v"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:647:29
	makeExtern< bool (*)(const char *,int *,int) , ImGui::CheckboxFlags , SimNode_ExtFuncCall , imguiTempFn>(lib,"CheckboxFlags","ImGui::CheckboxFlags")
		->args({"label","flags","flags_value"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:648:29
	makeExtern< bool (*)(const char *,unsigned int *,unsigned int) , ImGui::CheckboxFlags , SimNode_ExtFuncCall , imguiTempFn>(lib,"CheckboxFlags","ImGui::CheckboxFlags")
		->args({"label","flags","flags_value"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:649:29
	makeExtern< bool (*)(const char *,bool) , ImGui::RadioButton , SimNode_ExtFuncCall , imguiTempFn>(lib,"RadioButton","ImGui::RadioButton")
		->args({"label","active"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:650:29
	makeExtern< bool (*)(const char *,int *,int) , ImGui::RadioButton , SimNode_ExtFuncCall , imguiTempFn>(lib,"RadioButton","ImGui::RadioButton")
		->args({"label","v","v_button"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:651:29
	makeExtern< void (*)(float,const ImVec2 &,const char *) , ImGui::ProgressBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"ProgressBar","ImGui::ProgressBar")
		->args({"fraction","size_arg","overlay"})
		->arg_init(2,make_smart<ExprConstString>(""))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:652:29
	makeExtern< void (*)() , ImGui::Bullet , SimNode_ExtFuncCall , imguiTempFn>(lib,"Bullet","ImGui::Bullet")
		->addToModule(*this, SideEffects::worstDefault);
}
}

