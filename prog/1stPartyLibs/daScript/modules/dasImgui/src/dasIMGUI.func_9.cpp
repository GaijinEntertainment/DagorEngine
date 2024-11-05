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
void Module_dasIMGUI::initFunctions_9() {
// from imgui/imgui.h:617:29
	makeExtern< bool (*)(const char *,int[2],int,int,const char *,int) , ImGui::SliderInt2 , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderInt2","ImGui::SliderInt2")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%d"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:618:29
	makeExtern< bool (*)(const char *,int[3],int,int,const char *,int) , ImGui::SliderInt3 , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderInt3","ImGui::SliderInt3")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%d"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:619:29
	makeExtern< bool (*)(const char *,int[4],int,int,const char *,int) , ImGui::SliderInt4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderInt4","ImGui::SliderInt4")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%d"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:620:29
	makeExtern< bool (*)(const char *,int,void *,const void *,const void *,const char *,int) , ImGui::SliderScalar , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderScalar","ImGui::SliderScalar")
		->args({"label","data_type","p_data","p_min","p_max","format","flags"})
		->arg_type(1,makeType<ImGuiDataType_>(lib))
		->arg_init(5,make_smart<ExprConstString>(""))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:621:29
	makeExtern< bool (*)(const char *,int,void *,int,const void *,const void *,const char *,int) , ImGui::SliderScalarN , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderScalarN","ImGui::SliderScalarN")
		->args({"label","data_type","p_data","components","p_min","p_max","format","flags"})
		->arg_type(1,makeType<ImGuiDataType_>(lib))
		->arg_init(6,make_smart<ExprConstString>(""))
		->arg_type(7,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(7,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:622:29
	makeExtern< bool (*)(const char *,const ImVec2 &,float *,float,float,const char *,int) , ImGui::VSliderFloat , SimNode_ExtFuncCall , imguiTempFn>(lib,"VSliderFloat","ImGui::VSliderFloat")
		->args({"label","size","v","v_min","v_max","format","flags"})
		->arg_init(5,make_smart<ExprConstString>("%.3f"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:623:29
	makeExtern< bool (*)(const char *,const ImVec2 &,int *,int,int,const char *,int) , ImGui::VSliderInt , SimNode_ExtFuncCall , imguiTempFn>(lib,"VSliderInt","ImGui::VSliderInt")
		->args({"label","size","v","v_min","v_max","format","flags"})
		->arg_init(5,make_smart<ExprConstString>("%d"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:624:29
	makeExtern< bool (*)(const char *,const ImVec2 &,int,void *,const void *,const void *,const char *,int) , ImGui::VSliderScalar , SimNode_ExtFuncCall , imguiTempFn>(lib,"VSliderScalar","ImGui::VSliderScalar")
		->args({"label","size","data_type","p_data","p_min","p_max","format","flags"})
		->arg_type(2,makeType<ImGuiDataType_>(lib))
		->arg_init(6,make_smart<ExprConstString>(""))
		->arg_type(7,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(7,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:632:29
	makeExtern< bool (*)(const char *,float *,float,float,const char *,int) , ImGui::InputFloat , SimNode_ExtFuncCall , imguiTempFn>(lib,"InputFloat","ImGui::InputFloat")
		->args({"label","v","step","step_fast","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(0))
		->arg_init(3,make_smart<ExprConstFloat>(0))
		->arg_init(4,make_smart<ExprConstString>("%.3f"))
		->arg_type(5,makeType<ImGuiInputTextFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiInputTextFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:633:29
	makeExtern< bool (*)(const char *,float[2],const char *,int) , ImGui::InputFloat2 , SimNode_ExtFuncCall , imguiTempFn>(lib,"InputFloat2","ImGui::InputFloat2")
		->args({"label","v","format","flags"})
		->arg_init(2,make_smart<ExprConstString>("%.3f"))
		->arg_type(3,makeType<ImGuiInputTextFlags_>(lib))
		->arg_init(3,make_smart<ExprConstEnumeration>(0,makeType<ImGuiInputTextFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:634:29
	makeExtern< bool (*)(const char *,float[3],const char *,int) , ImGui::InputFloat3 , SimNode_ExtFuncCall , imguiTempFn>(lib,"InputFloat3","ImGui::InputFloat3")
		->args({"label","v","format","flags"})
		->arg_init(2,make_smart<ExprConstString>("%.3f"))
		->arg_type(3,makeType<ImGuiInputTextFlags_>(lib))
		->arg_init(3,make_smart<ExprConstEnumeration>(0,makeType<ImGuiInputTextFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:635:29
	makeExtern< bool (*)(const char *,float[4],const char *,int) , ImGui::InputFloat4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"InputFloat4","ImGui::InputFloat4")
		->args({"label","v","format","flags"})
		->arg_init(2,make_smart<ExprConstString>("%.3f"))
		->arg_type(3,makeType<ImGuiInputTextFlags_>(lib))
		->arg_init(3,make_smart<ExprConstEnumeration>(0,makeType<ImGuiInputTextFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:636:29
	makeExtern< bool (*)(const char *,int *,int,int,int) , ImGui::InputInt , SimNode_ExtFuncCall , imguiTempFn>(lib,"InputInt","ImGui::InputInt")
		->args({"label","v","step","step_fast","flags"})
		->arg_init(2,make_smart<ExprConstInt>(1))
		->arg_init(3,make_smart<ExprConstInt>(100))
		->arg_type(4,makeType<ImGuiInputTextFlags_>(lib))
		->arg_init(4,make_smart<ExprConstEnumeration>(0,makeType<ImGuiInputTextFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:637:29
	makeExtern< bool (*)(const char *,int[2],int) , ImGui::InputInt2 , SimNode_ExtFuncCall , imguiTempFn>(lib,"InputInt2","ImGui::InputInt2")
		->args({"label","v","flags"})
		->arg_type(2,makeType<ImGuiInputTextFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiInputTextFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:638:29
	makeExtern< bool (*)(const char *,int[3],int) , ImGui::InputInt3 , SimNode_ExtFuncCall , imguiTempFn>(lib,"InputInt3","ImGui::InputInt3")
		->args({"label","v","flags"})
		->arg_type(2,makeType<ImGuiInputTextFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiInputTextFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:639:29
	makeExtern< bool (*)(const char *,int[4],int) , ImGui::InputInt4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"InputInt4","ImGui::InputInt4")
		->args({"label","v","flags"})
		->arg_type(2,makeType<ImGuiInputTextFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiInputTextFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:640:29
	makeExtern< bool (*)(const char *,double *,double,double,const char *,int) , ImGui::InputDouble , SimNode_ExtFuncCall , imguiTempFn>(lib,"InputDouble","ImGui::InputDouble")
		->args({"label","v","step","step_fast","format","flags"})
		->arg_init(2,make_smart<ExprConstDouble>(0))
		->arg_init(3,make_smart<ExprConstDouble>(0))
		->arg_init(4,make_smart<ExprConstString>("%.6f"))
		->arg_type(5,makeType<ImGuiInputTextFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiInputTextFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:641:29
	makeExtern< bool (*)(const char *,int,void *,const void *,const void *,const char *,int) , ImGui::InputScalar , SimNode_ExtFuncCall , imguiTempFn>(lib,"InputScalar","ImGui::InputScalar")
		->args({"label","data_type","p_data","p_step","p_step_fast","format","flags"})
		->arg_type(1,makeType<ImGuiDataType_>(lib))
		->arg_init(3,make_smart<ExprConstPtr>())
		->arg_init(4,make_smart<ExprConstPtr>())
		->arg_init(5,make_smart<ExprConstString>(""))
		->arg_type(6,makeType<ImGuiInputTextFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiInputTextFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:642:29
	makeExtern< bool (*)(const char *,int,void *,int,const void *,const void *,const char *,int) , ImGui::InputScalarN , SimNode_ExtFuncCall , imguiTempFn>(lib,"InputScalarN","ImGui::InputScalarN")
		->args({"label","data_type","p_data","components","p_step","p_step_fast","format","flags"})
		->arg_type(1,makeType<ImGuiDataType_>(lib))
		->arg_init(4,make_smart<ExprConstPtr>())
		->arg_init(5,make_smart<ExprConstPtr>())
		->arg_init(6,make_smart<ExprConstString>(""))
		->arg_type(7,makeType<ImGuiInputTextFlags_>(lib))
		->arg_init(7,make_smart<ExprConstEnumeration>(0,makeType<ImGuiInputTextFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:647:29
	makeExtern< bool (*)(const char *,float[3],int) , ImGui::ColorEdit3 , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorEdit3","ImGui::ColorEdit3")
		->args({"label","col","flags"})
		->arg_type(2,makeType<ImGuiColorEditFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiColorEditFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
}
}

