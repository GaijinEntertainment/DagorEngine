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
// from imgui/imgui.h:767:29
	makeExtern< void (*)(bool,int) , ImGui::SetNextItemOpen , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextItemOpen","ImGui::SetNextItemOpen")
		->args({"is_open","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:768:29
	makeExtern< void (*)(unsigned int) , ImGui::SetNextItemStorageID , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextItemStorageID","ImGui::SetNextItemStorageID")
		->args({"storage_id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:773:29
	makeExtern< bool (*)(const char *,bool,int,const ImVec2 &) , ImGui::Selectable , SimNode_ExtFuncCall , imguiTempFn>(lib,"Selectable","ImGui::Selectable")
		->args({"label","selected","flags","size"})
		->arg_init(1,make_smart<ExprConstBool>(false))
		->arg_type(2,makeType<ImGuiSelectableFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSelectableFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:774:29
	makeExtern< bool (*)(const char *,bool *,int,const ImVec2 &) , ImGui::Selectable , SimNode_ExtFuncCall , imguiTempFn>(lib,"Selectable","ImGui::Selectable")
		->args({"label","p_selected","flags","size"})
		->arg_type(2,makeType<ImGuiSelectableFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSelectableFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:783:37
	makeExtern< ImGuiMultiSelectIO * (*)(int,int,int) , ImGui::BeginMultiSelect , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginMultiSelect","ImGui::BeginMultiSelect")
		->args({"flags","selection_size","items_count"})
		->arg_init(1,make_smart<ExprConstInt>(-1))
		->arg_init(2,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:784:37
	makeExtern< ImGuiMultiSelectIO * (*)() , ImGui::EndMultiSelect , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndMultiSelect","ImGui::EndMultiSelect")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:785:37
	makeExtern< void (*)(ImGuiSelectionUserData) , ImGui::SetNextItemSelectionUserData , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextItemSelectionUserData","ImGui::SetNextItemSelectionUserData")
		->args({"selection_user_data"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:786:37
	makeExtern< bool (*)() , ImGui::IsItemToggledSelection , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemToggledSelection","ImGui::IsItemToggledSelection")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:795:29
	makeExtern< bool (*)(const char *,const ImVec2 &) , ImGui::BeginListBox , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginListBox","ImGui::BeginListBox")
		->args({"label","size"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:796:29
	makeExtern< void (*)() , ImGui::EndListBox , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndListBox","ImGui::EndListBox")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:797:29
	makeExtern< bool (*)(const char *,int *,const char *const[],int,int) , ImGui::ListBox , SimNode_ExtFuncCall , imguiTempFn>(lib,"ListBox","ImGui::ListBox")
		->args({"label","current_item","items","items_count","height_in_items"})
		->arg_init(4,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:802:29
	makeExtern< void (*)(const char *,const float *,int,int,const char *,float,float,ImVec2,int) , ImGui::PlotLines , SimNode_ExtFuncCall , imguiTempFn>(lib,"PlotLines","ImGui::PlotLines")
		->args({"label","values","values_count","values_offset","overlay_text","scale_min","scale_max","graph_size","stride"})
		->arg_init(3,make_smart<ExprConstInt>(0))
		->arg_init(4,make_smart<ExprConstString>(""))
		->arg_init(5,make_smart<ExprConstFloat>(3.4028234663852886e+38))
		->arg_init(6,make_smart<ExprConstFloat>(3.4028234663852886e+38))
		->arg_init(8,make_smart<ExprConstInt>(4))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:804:29
	makeExtern< void (*)(const char *,const float *,int,int,const char *,float,float,ImVec2,int) , ImGui::PlotHistogram , SimNode_ExtFuncCall , imguiTempFn>(lib,"PlotHistogram","ImGui::PlotHistogram")
		->args({"label","values","values_count","values_offset","overlay_text","scale_min","scale_max","graph_size","stride"})
		->arg_init(3,make_smart<ExprConstInt>(0))
		->arg_init(4,make_smart<ExprConstString>(""))
		->arg_init(5,make_smart<ExprConstFloat>(3.4028234663852886e+38))
		->arg_init(6,make_smart<ExprConstFloat>(3.4028234663852886e+38))
		->arg_init(8,make_smart<ExprConstInt>(4))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:809:29
	makeExtern< void (*)(const char *,bool) , ImGui::Value , SimNode_ExtFuncCall , imguiTempFn>(lib,"Value","ImGui::Value")
		->args({"prefix","b"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:810:29
	makeExtern< void (*)(const char *,int) , ImGui::Value , SimNode_ExtFuncCall , imguiTempFn>(lib,"Value","ImGui::Value")
		->args({"prefix","v"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:811:29
	makeExtern< void (*)(const char *,unsigned int) , ImGui::Value , SimNode_ExtFuncCall , imguiTempFn>(lib,"Value","ImGui::Value")
		->args({"prefix","v"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:812:29
	makeExtern< void (*)(const char *,float,const char *) , ImGui::Value , SimNode_ExtFuncCall , imguiTempFn>(lib,"Value","ImGui::Value")
		->args({"prefix","v","float_format"})
		->arg_init(2,make_smart<ExprConstString>(""))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:819:29
	makeExtern< bool (*)() , ImGui::BeginMenuBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginMenuBar","ImGui::BeginMenuBar")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:820:29
	makeExtern< void (*)() , ImGui::EndMenuBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndMenuBar","ImGui::EndMenuBar")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:821:29
	makeExtern< bool (*)() , ImGui::BeginMainMenuBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginMainMenuBar","ImGui::BeginMainMenuBar")
		->addToModule(*this, SideEffects::worstDefault);
}
}

