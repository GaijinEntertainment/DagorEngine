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
void Module_dasIMGUI::initFunctions_12() {
// from imgui/imgui.h:822:29
	makeExtern< void (*)() , ImGui::EndMainMenuBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndMainMenuBar","ImGui::EndMainMenuBar")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:823:29
	makeExtern< bool (*)(const char *,bool) , ImGui::BeginMenu , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginMenu","ImGui::BeginMenu")
		->args({"label","enabled"})
		->arg_init(1,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:824:29
	makeExtern< void (*)() , ImGui::EndMenu , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndMenu","ImGui::EndMenu")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:825:29
	makeExtern< bool (*)(const char *,const char *,bool,bool) , ImGui::MenuItem , SimNode_ExtFuncCall , imguiTempFn>(lib,"MenuItem","ImGui::MenuItem")
		->args({"label","shortcut","selected","enabled"})
		->arg_init(1,make_smart<ExprConstString>(""))
		->arg_init(2,make_smart<ExprConstBool>(false))
		->arg_init(3,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:826:29
	makeExtern< bool (*)(const char *,const char *,bool *,bool) , ImGui::MenuItem , SimNode_ExtFuncCall , imguiTempFn>(lib,"MenuItem","ImGui::MenuItem")
		->args({"label","shortcut","p_selected","enabled"})
		->arg_init(3,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:832:29
	makeExtern< bool (*)() , ImGui::BeginTooltip , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginTooltip","ImGui::BeginTooltip")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:833:29
	makeExtern< void (*)() , ImGui::EndTooltip , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndTooltip","ImGui::EndTooltip")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:841:29
	makeExtern< bool (*)() , ImGui::BeginItemTooltip , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginItemTooltip","ImGui::BeginItemTooltip")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:855:29
	makeExtern< bool (*)(const char *,int) , ImGui::BeginPopup , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginPopup","ImGui::BeginPopup")
		->args({"str_id","flags"})
		->arg_type(1,makeType<ImGuiWindowFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiWindowFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:856:29
	makeExtern< bool (*)(const char *,bool *,int) , ImGui::BeginPopupModal , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginPopupModal","ImGui::BeginPopupModal")
		->args({"name","p_open","flags"})
		->arg_init(1,make_smart<ExprConstPtr>())
		->arg_type(2,makeType<ImGuiWindowFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiWindowFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:857:29
	makeExtern< void (*)() , ImGui::EndPopup , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndPopup","ImGui::EndPopup")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:866:29
	makeExtern< void (*)(const char *,int) , ImGui::OpenPopup , SimNode_ExtFuncCall , imguiTempFn>(lib,"OpenPopup","ImGui::OpenPopup")
		->args({"str_id","popup_flags"})
		->arg_type(1,makeType<ImGuiPopupFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiPopupFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:867:29
	makeExtern< void (*)(unsigned int,int) , ImGui::OpenPopup , SimNode_ExtFuncCall , imguiTempFn>(lib,"OpenPopup","ImGui::OpenPopup")
		->args({"id","popup_flags"})
		->arg_type(1,makeType<ImGuiPopupFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiPopupFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:868:29
	makeExtern< void (*)(const char *,int) , ImGui::OpenPopupOnItemClick , SimNode_ExtFuncCall , imguiTempFn>(lib,"OpenPopupOnItemClick","ImGui::OpenPopupOnItemClick")
		->args({"str_id","popup_flags"})
		->arg_init(0,make_smart<ExprConstString>(""))
		->arg_type(1,makeType<ImGuiPopupFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiPopupFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:869:29
	makeExtern< void (*)() , ImGui::CloseCurrentPopup , SimNode_ExtFuncCall , imguiTempFn>(lib,"CloseCurrentPopup","ImGui::CloseCurrentPopup")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:880:29
	makeExtern< bool (*)(const char *,int) , ImGui::BeginPopupContextItem , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginPopupContextItem","ImGui::BeginPopupContextItem")
		->args({"str_id","popup_flags"})
		->arg_init(0,make_smart<ExprConstString>(""))
		->arg_type(1,makeType<ImGuiPopupFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiPopupFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:881:29
	makeExtern< bool (*)(const char *,int) , ImGui::BeginPopupContextWindow , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginPopupContextWindow","ImGui::BeginPopupContextWindow")
		->args({"str_id","popup_flags"})
		->arg_init(0,make_smart<ExprConstString>(""))
		->arg_type(1,makeType<ImGuiPopupFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiPopupFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:882:29
	makeExtern< bool (*)(const char *,int) , ImGui::BeginPopupContextVoid , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginPopupContextVoid","ImGui::BeginPopupContextVoid")
		->args({"str_id","popup_flags"})
		->arg_init(0,make_smart<ExprConstString>(""))
		->arg_type(1,makeType<ImGuiPopupFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiPopupFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:888:29
	makeExtern< bool (*)(const char *,int) , ImGui::IsPopupOpen , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsPopupOpen","ImGui::IsPopupOpen")
		->args({"str_id","flags"})
		->arg_type(1,makeType<ImGuiPopupFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiPopupFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:911:29
	makeExtern< bool (*)(const char *,int,int,const ImVec2 &,float) , ImGui::BeginTable , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginTable","ImGui::BeginTable")
		->args({"str_id","columns","flags","outer_size","inner_width"})
		->arg_type(2,makeType<ImGuiTableFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTableFlags_>(lib)))
		->arg_init(4,make_smart<ExprConstFloat>(0))
		->addToModule(*this, SideEffects::worstDefault);
}
}

