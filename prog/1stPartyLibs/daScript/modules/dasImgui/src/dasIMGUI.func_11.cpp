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
void Module_dasIMGUI::initFunctions_11() {
// from imgui/imgui.h:691:37
	makeExtern< bool (*)() , ImGui::IsItemToggledSelection , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemToggledSelection","ImGui::IsItemToggledSelection")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:699:29
	makeExtern< bool (*)(const char *,const ImVec2 &) , ImGui::BeginListBox , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginListBox","ImGui::BeginListBox")
		->args({"label","size"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:700:29
	makeExtern< void (*)() , ImGui::EndListBox , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndListBox","ImGui::EndListBox")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:701:29
	makeExtern< bool (*)(const char *,int *,const char *const[],int,int) , ImGui::ListBox , SimNode_ExtFuncCall , imguiTempFn>(lib,"ListBox","ImGui::ListBox")
		->args({"label","current_item","items","items_count","height_in_items"})
		->arg_init(4,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:706:29
	makeExtern< void (*)(const char *,const float *,int,int,const char *,float,float,ImVec2,int) , ImGui::PlotLines , SimNode_ExtFuncCall , imguiTempFn>(lib,"PlotLines","ImGui::PlotLines")
		->args({"label","values","values_count","values_offset","overlay_text","scale_min","scale_max","graph_size","stride"})
		->arg_init(3,make_smart<ExprConstInt>(0))
		->arg_init(4,make_smart<ExprConstString>(""))
		->arg_init(5,make_smart<ExprConstFloat>(3.4028234663852886e+38))
		->arg_init(6,make_smart<ExprConstFloat>(3.4028234663852886e+38))
		->arg_init(8,make_smart<ExprConstInt>(4))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:708:29
	makeExtern< void (*)(const char *,const float *,int,int,const char *,float,float,ImVec2,int) , ImGui::PlotHistogram , SimNode_ExtFuncCall , imguiTempFn>(lib,"PlotHistogram","ImGui::PlotHistogram")
		->args({"label","values","values_count","values_offset","overlay_text","scale_min","scale_max","graph_size","stride"})
		->arg_init(3,make_smart<ExprConstInt>(0))
		->arg_init(4,make_smart<ExprConstString>(""))
		->arg_init(5,make_smart<ExprConstFloat>(3.4028234663852886e+38))
		->arg_init(6,make_smart<ExprConstFloat>(3.4028234663852886e+38))
		->arg_init(8,make_smart<ExprConstInt>(4))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:713:29
	makeExtern< void (*)(const char *,bool) , ImGui::Value , SimNode_ExtFuncCall , imguiTempFn>(lib,"Value","ImGui::Value")
		->args({"prefix","b"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:714:29
	makeExtern< void (*)(const char *,int) , ImGui::Value , SimNode_ExtFuncCall , imguiTempFn>(lib,"Value","ImGui::Value")
		->args({"prefix","v"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:715:29
	makeExtern< void (*)(const char *,unsigned int) , ImGui::Value , SimNode_ExtFuncCall , imguiTempFn>(lib,"Value","ImGui::Value")
		->args({"prefix","v"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:716:29
	makeExtern< void (*)(const char *,float,const char *) , ImGui::Value , SimNode_ExtFuncCall , imguiTempFn>(lib,"Value","ImGui::Value")
		->args({"prefix","v","float_format"})
		->arg_init(2,make_smart<ExprConstString>(""))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:723:29
	makeExtern< bool (*)() , ImGui::BeginMenuBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginMenuBar","ImGui::BeginMenuBar")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:724:29
	makeExtern< void (*)() , ImGui::EndMenuBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndMenuBar","ImGui::EndMenuBar")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:725:29
	makeExtern< bool (*)() , ImGui::BeginMainMenuBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginMainMenuBar","ImGui::BeginMainMenuBar")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:726:29
	makeExtern< void (*)() , ImGui::EndMainMenuBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndMainMenuBar","ImGui::EndMainMenuBar")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:727:29
	makeExtern< bool (*)(const char *,bool) , ImGui::BeginMenu , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginMenu","ImGui::BeginMenu")
		->args({"label","enabled"})
		->arg_init(1,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:728:29
	makeExtern< void (*)() , ImGui::EndMenu , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndMenu","ImGui::EndMenu")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:729:29
	makeExtern< bool (*)(const char *,const char *,bool,bool) , ImGui::MenuItem , SimNode_ExtFuncCall , imguiTempFn>(lib,"MenuItem","ImGui::MenuItem")
		->args({"label","shortcut","selected","enabled"})
		->arg_init(1,make_smart<ExprConstString>(""))
		->arg_init(2,make_smart<ExprConstBool>(false))
		->arg_init(3,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:730:29
	makeExtern< bool (*)(const char *,const char *,bool *,bool) , ImGui::MenuItem , SimNode_ExtFuncCall , imguiTempFn>(lib,"MenuItem","ImGui::MenuItem")
		->args({"label","shortcut","p_selected","enabled"})
		->arg_init(3,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:736:29
	makeExtern< bool (*)() , ImGui::BeginTooltip , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginTooltip","ImGui::BeginTooltip")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:737:29
	makeExtern< void (*)() , ImGui::EndTooltip , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndTooltip","ImGui::EndTooltip")
		->addToModule(*this, SideEffects::worstDefault);
}
}

