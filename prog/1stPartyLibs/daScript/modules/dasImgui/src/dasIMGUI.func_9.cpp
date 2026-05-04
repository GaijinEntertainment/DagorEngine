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
// from imgui/imgui.h:698:29
	makeExtern< bool (*)(const char *,int,void *,int,float,const void *,const void *,const char *,int) , ImGui::DragScalarN , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragScalarN","ImGui::DragScalarN")
		->args({"label","data_type","p_data","components","v_speed","p_min","p_max","format","flags"})
		->arg_type(1,makeType<ImGuiDataType_>(lib))
		->arg_init(4,make_smart<ExprConstFloat>(1))
		->arg_init(5,make_smart<ExprConstPtr>())
		->arg_init(6,make_smart<ExprConstPtr>())
		->arg_init(7,make_smart<ExprConstString>(""))
		->arg_type(8,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(8,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:706:29
	makeExtern< bool (*)(const char *,float *,float,float,const char *,int) , ImGui::SliderFloat , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderFloat","ImGui::SliderFloat")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%.3f"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:707:29
	makeExtern< bool (*)(const char *,float[2],float,float,const char *,int) , ImGui::SliderFloat2 , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderFloat2","ImGui::SliderFloat2")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%.3f"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:708:29
	makeExtern< bool (*)(const char *,float[3],float,float,const char *,int) , ImGui::SliderFloat3 , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderFloat3","ImGui::SliderFloat3")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%.3f"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:709:29
	makeExtern< bool (*)(const char *,float[4],float,float,const char *,int) , ImGui::SliderFloat4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderFloat4","ImGui::SliderFloat4")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%.3f"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:710:29
	makeExtern< bool (*)(const char *,float *,float,float,const char *,int) , ImGui::SliderAngle , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderAngle","ImGui::SliderAngle")
		->args({"label","v_rad","v_degrees_min","v_degrees_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(-360))
		->arg_init(3,make_smart<ExprConstFloat>(360))
		->arg_init(4,make_smart<ExprConstString>("%.0f deg"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:711:29
	makeExtern< bool (*)(const char *,int *,int,int,const char *,int) , ImGui::SliderInt , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderInt","ImGui::SliderInt")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%d"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:712:29
	makeExtern< bool (*)(const char *,int[2],int,int,const char *,int) , ImGui::SliderInt2 , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderInt2","ImGui::SliderInt2")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%d"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:713:29
	makeExtern< bool (*)(const char *,int[3],int,int,const char *,int) , ImGui::SliderInt3 , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderInt3","ImGui::SliderInt3")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%d"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:714:29
	makeExtern< bool (*)(const char *,int[4],int,int,const char *,int) , ImGui::SliderInt4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderInt4","ImGui::SliderInt4")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%d"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:715:29
	makeExtern< bool (*)(const char *,int,void *,const void *,const void *,const char *,int) , ImGui::SliderScalar , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderScalar","ImGui::SliderScalar")
		->args({"label","data_type","p_data","p_min","p_max","format","flags"})
		->arg_type(1,makeType<ImGuiDataType_>(lib))
		->arg_init(5,make_smart<ExprConstString>(""))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:716:29
	makeExtern< bool (*)(const char *,int,void *,int,const void *,const void *,const char *,int) , ImGui::SliderScalarN , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderScalarN","ImGui::SliderScalarN")
		->args({"label","data_type","p_data","components","p_min","p_max","format","flags"})
		->arg_type(1,makeType<ImGuiDataType_>(lib))
		->arg_init(6,make_smart<ExprConstString>(""))
		->arg_type(7,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(7,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:717:29
	makeExtern< bool (*)(const char *,const ImVec2 &,float *,float,float,const char *,int) , ImGui::VSliderFloat , SimNode_ExtFuncCall , imguiTempFn>(lib,"VSliderFloat","ImGui::VSliderFloat")
		->args({"label","size","v","v_min","v_max","format","flags"})
		->arg_init(5,make_smart<ExprConstString>("%.3f"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:718:29
	makeExtern< bool (*)(const char *,const ImVec2 &,int *,int,int,const char *,int) , ImGui::VSliderInt , SimNode_ExtFuncCall , imguiTempFn>(lib,"VSliderInt","ImGui::VSliderInt")
		->args({"label","size","v","v_min","v_max","format","flags"})
		->arg_init(5,make_smart<ExprConstString>("%d"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:719:29
	makeExtern< bool (*)(const char *,const ImVec2 &,int,void *,const void *,const void *,const char *,int) , ImGui::VSliderScalar , SimNode_ExtFuncCall , imguiTempFn>(lib,"VSliderScalar","ImGui::VSliderScalar")
		->args({"label","size","data_type","p_data","p_min","p_max","format","flags"})
		->arg_type(2,makeType<ImGuiDataType_>(lib))
		->arg_init(6,make_smart<ExprConstString>(""))
		->arg_type(7,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(7,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:727:29
	makeExtern< bool (*)(const char *,float *,float,float,const char *,int) , ImGui::InputFloat , SimNode_ExtFuncCall , imguiTempFn>(lib,"InputFloat","ImGui::InputFloat")
		->args({"label","v","step","step_fast","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(0))
		->arg_init(3,make_smart<ExprConstFloat>(0))
		->arg_init(4,make_smart<ExprConstString>("%.3f"))
		->arg_type(5,makeType<ImGuiInputTextFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiInputTextFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:728:29
	makeExtern< bool (*)(const char *,float[2],const char *,int) , ImGui::InputFloat2 , SimNode_ExtFuncCall , imguiTempFn>(lib,"InputFloat2","ImGui::InputFloat2")
		->args({"label","v","format","flags"})
		->arg_init(2,make_smart<ExprConstString>("%.3f"))
		->arg_type(3,makeType<ImGuiInputTextFlags_>(lib))
		->arg_init(3,make_smart<ExprConstEnumeration>(0,makeType<ImGuiInputTextFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:729:29
	makeExtern< bool (*)(const char *,float[3],const char *,int) , ImGui::InputFloat3 , SimNode_ExtFuncCall , imguiTempFn>(lib,"InputFloat3","ImGui::InputFloat3")
		->args({"label","v","format","flags"})
		->arg_init(2,make_smart<ExprConstString>("%.3f"))
		->arg_type(3,makeType<ImGuiInputTextFlags_>(lib))
		->arg_init(3,make_smart<ExprConstEnumeration>(0,makeType<ImGuiInputTextFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:730:29
	makeExtern< bool (*)(const char *,float[4],const char *,int) , ImGui::InputFloat4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"InputFloat4","ImGui::InputFloat4")
		->args({"label","v","format","flags"})
		->arg_init(2,make_smart<ExprConstString>("%.3f"))
		->arg_type(3,makeType<ImGuiInputTextFlags_>(lib))
		->arg_init(3,make_smart<ExprConstEnumeration>(0,makeType<ImGuiInputTextFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:731:29
	makeExtern< bool (*)(const char *,int *,int,int,int) , ImGui::InputInt , SimNode_ExtFuncCall , imguiTempFn>(lib,"InputInt","ImGui::InputInt")
		->args({"label","v","step","step_fast","flags"})
		->arg_init(2,make_smart<ExprConstInt>(1))
		->arg_init(3,make_smart<ExprConstInt>(100))
		->arg_type(4,makeType<ImGuiInputTextFlags_>(lib))
		->arg_init(4,make_smart<ExprConstEnumeration>(0,makeType<ImGuiInputTextFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
}
}

