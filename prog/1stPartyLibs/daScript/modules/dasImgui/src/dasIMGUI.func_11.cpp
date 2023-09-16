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
void Module_dasIMGUI::initFunctions_11() {
	makeExtern< void (*)(const char *,float,const char *) , ImGui::Value , SimNode_ExtFuncCall , imguiTempFn>(lib,"Value","ImGui::Value")
		->args({"prefix","v","float_format"})
		->arg_init(2,make_smart<ExprConstString>(""))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::BeginMenuBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginMenuBar","ImGui::BeginMenuBar")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::EndMenuBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndMenuBar","ImGui::EndMenuBar")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::BeginMainMenuBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginMainMenuBar","ImGui::BeginMainMenuBar")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::EndMainMenuBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndMainMenuBar","ImGui::EndMainMenuBar")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,bool) , ImGui::BeginMenu , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginMenu","ImGui::BeginMenu")
		->args({"label","enabled"})
		->arg_init(1,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::EndMenu , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndMenu","ImGui::EndMenu")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,const char *,bool,bool) , ImGui::MenuItem , SimNode_ExtFuncCall , imguiTempFn>(lib,"MenuItem","ImGui::MenuItem")
		->args({"label","shortcut","selected","enabled"})
		->arg_init(1,make_smart<ExprConstString>(""))
		->arg_init(2,make_smart<ExprConstBool>(false))
		->arg_init(3,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,const char *,bool *,bool) , ImGui::MenuItem , SimNode_ExtFuncCall , imguiTempFn>(lib,"MenuItem","ImGui::MenuItem")
		->args({"label","shortcut","p_selected","enabled"})
		->arg_init(3,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::BeginTooltip , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginTooltip","ImGui::BeginTooltip")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::EndTooltip , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndTooltip","ImGui::EndTooltip")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int) , ImGui::BeginPopup , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginPopup","ImGui::BeginPopup")
		->args({"str_id","flags"})
		->arg_type(1,makeType<ImGuiWindowFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiWindowFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,bool *,int) , ImGui::BeginPopupModal , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginPopupModal","ImGui::BeginPopupModal")
		->args({"name","p_open","flags"})
		->arg_init(1,make_smart<ExprConstPtr>())
		->arg_type(2,makeType<ImGuiWindowFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiWindowFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::EndPopup , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndPopup","ImGui::EndPopup")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *,int) , ImGui::OpenPopup , SimNode_ExtFuncCall , imguiTempFn>(lib,"OpenPopup","ImGui::OpenPopup")
		->args({"str_id","popup_flags"})
		->arg_type(1,makeType<ImGuiPopupFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiPopupFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *,int) , ImGui::OpenPopupOnItemClick , SimNode_ExtFuncCall , imguiTempFn>(lib,"OpenPopupOnItemClick","ImGui::OpenPopupOnItemClick")
		->args({"str_id","popup_flags"})
		->arg_init(0,make_smart<ExprConstString>(""))
		->arg_type(1,makeType<ImGuiPopupFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(1,makeType<ImGuiPopupFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::CloseCurrentPopup , SimNode_ExtFuncCall , imguiTempFn>(lib,"CloseCurrentPopup","ImGui::CloseCurrentPopup")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int) , ImGui::BeginPopupContextItem , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginPopupContextItem","ImGui::BeginPopupContextItem")
		->args({"str_id","popup_flags"})
		->arg_init(0,make_smart<ExprConstString>(""))
		->arg_type(1,makeType<ImGuiPopupFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(1,makeType<ImGuiPopupFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int) , ImGui::BeginPopupContextWindow , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginPopupContextWindow","ImGui::BeginPopupContextWindow")
		->args({"str_id","popup_flags"})
		->arg_init(0,make_smart<ExprConstString>(""))
		->arg_type(1,makeType<ImGuiPopupFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(1,makeType<ImGuiPopupFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int) , ImGui::BeginPopupContextVoid , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginPopupContextVoid","ImGui::BeginPopupContextVoid")
		->args({"str_id","popup_flags"})
		->arg_init(0,make_smart<ExprConstString>(""))
		->arg_type(1,makeType<ImGuiPopupFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(1,makeType<ImGuiPopupFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
}
}

