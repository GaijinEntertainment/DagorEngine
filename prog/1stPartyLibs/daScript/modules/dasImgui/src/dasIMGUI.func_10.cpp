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
void Module_dasIMGUI::initFunctions_10() {
// from imgui/imgui.h:648:29
	makeExtern< bool (*)(const char *,float[4],int) , ImGui::ColorEdit4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorEdit4","ImGui::ColorEdit4")
		->args({"label","col","flags"})
		->arg_type(2,makeType<ImGuiColorEditFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiColorEditFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:649:29
	makeExtern< bool (*)(const char *,float[3],int) , ImGui::ColorPicker3 , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorPicker3","ImGui::ColorPicker3")
		->args({"label","col","flags"})
		->arg_type(2,makeType<ImGuiColorEditFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiColorEditFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:650:29
	makeExtern< bool (*)(const char *,float[4],int,const float *) , ImGui::ColorPicker4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorPicker4","ImGui::ColorPicker4")
		->args({"label","col","flags","ref_col"})
		->arg_type(2,makeType<ImGuiColorEditFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiColorEditFlags_>(lib)))
		->arg_init(3,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:651:29
	makeExtern< bool (*)(const char *,const ImVec4 &,int,const ImVec2 &) , ImGui::ColorButton , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorButton","ImGui::ColorButton")
		->args({"desc_id","col","flags","size"})
		->arg_type(2,makeType<ImGuiColorEditFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiColorEditFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:652:29
	makeExtern< void (*)(int) , ImGui::SetColorEditOptions , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetColorEditOptions","ImGui::SetColorEditOptions")
		->args({"flags"})
		->arg_type(0,makeType<ImGuiColorEditFlags_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:656:29
	makeExtern< bool (*)(const char *) , ImGui::TreeNode , SimNode_ExtFuncCall , imguiTempFn>(lib,"TreeNode","ImGui::TreeNode")
		->args({"label"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:661:29
	makeExtern< bool (*)(const char *,int) , ImGui::TreeNodeEx , SimNode_ExtFuncCall , imguiTempFn>(lib,"TreeNodeEx","ImGui::TreeNodeEx")
		->args({"label","flags"})
		->arg_type(1,makeType<ImGuiTreeNodeFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTreeNodeFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:666:29
	makeExtern< void (*)(const char *) , ImGui::TreePush , SimNode_ExtFuncCall , imguiTempFn>(lib,"TreePush","ImGui::TreePush")
		->args({"str_id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:667:29
	makeExtern< void (*)(const void *) , ImGui::TreePush , SimNode_ExtFuncCall , imguiTempFn>(lib,"TreePush","ImGui::TreePush")
		->args({"ptr_id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:668:29
	makeExtern< void (*)() , ImGui::TreePop , SimNode_ExtFuncCall , imguiTempFn>(lib,"TreePop","ImGui::TreePop")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:669:29
	makeExtern< float (*)() , ImGui::GetTreeNodeToLabelSpacing , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTreeNodeToLabelSpacing","ImGui::GetTreeNodeToLabelSpacing")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:670:29
	makeExtern< bool (*)(const char *,int) , ImGui::CollapsingHeader , SimNode_ExtFuncCall , imguiTempFn>(lib,"CollapsingHeader","ImGui::CollapsingHeader")
		->args({"label","flags"})
		->arg_type(1,makeType<ImGuiTreeNodeFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTreeNodeFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:671:29
	makeExtern< bool (*)(const char *,bool *,int) , ImGui::CollapsingHeader , SimNode_ExtFuncCall , imguiTempFn>(lib,"CollapsingHeader","ImGui::CollapsingHeader")
		->args({"label","p_visible","flags"})
		->arg_type(2,makeType<ImGuiTreeNodeFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTreeNodeFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:672:29
	makeExtern< void (*)(bool,int) , ImGui::SetNextItemOpen , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextItemOpen","ImGui::SetNextItemOpen")
		->args({"is_open","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:673:29
	makeExtern< void (*)(unsigned int) , ImGui::SetNextItemStorageID , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextItemStorageID","ImGui::SetNextItemStorageID")
		->args({"storage_id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:678:29
	makeExtern< bool (*)(const char *,bool,int,const ImVec2 &) , ImGui::Selectable , SimNode_ExtFuncCall , imguiTempFn>(lib,"Selectable","ImGui::Selectable")
		->args({"label","selected","flags","size"})
		->arg_init(1,make_smart<ExprConstBool>(false))
		->arg_type(2,makeType<ImGuiSelectableFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSelectableFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:679:29
	makeExtern< bool (*)(const char *,bool *,int,const ImVec2 &) , ImGui::Selectable , SimNode_ExtFuncCall , imguiTempFn>(lib,"Selectable","ImGui::Selectable")
		->args({"label","p_selected","flags","size"})
		->arg_type(2,makeType<ImGuiSelectableFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSelectableFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:688:37
	makeExtern< ImGuiMultiSelectIO * (*)(int,int,int) , ImGui::BeginMultiSelect , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginMultiSelect","ImGui::BeginMultiSelect")
		->args({"flags","selection_size","items_count"})
		->arg_init(1,make_smart<ExprConstInt>(-1))
		->arg_init(2,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:689:37
	makeExtern< ImGuiMultiSelectIO * (*)() , ImGui::EndMultiSelect , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndMultiSelect","ImGui::EndMultiSelect")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:690:37
	makeExtern< void (*)(ImGuiSelectionUserData) , ImGui::SetNextItemSelectionUserData , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextItemSelectionUserData","ImGui::SetNextItemSelectionUserData")
		->args({"selection_user_data"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

