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
void Module_dasIMGUI::initFunctions_4() {
// from imgui/imgui.h:436:29
	makeExtern< float (*)() , ImGui::GetScrollMaxX , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetScrollMaxX","ImGui::GetScrollMaxX")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:437:29
	makeExtern< float (*)() , ImGui::GetScrollMaxY , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetScrollMaxY","ImGui::GetScrollMaxY")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:438:29
	makeExtern< void (*)(float) , ImGui::SetScrollHereX , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetScrollHereX","ImGui::SetScrollHereX")
		->args({"center_x_ratio"})
		->arg_init(0,make_smart<ExprConstFloat>(0.5))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:439:29
	makeExtern< void (*)(float) , ImGui::SetScrollHereY , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetScrollHereY","ImGui::SetScrollHereY")
		->args({"center_y_ratio"})
		->arg_init(0,make_smart<ExprConstFloat>(0.5))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:440:29
	makeExtern< void (*)(float,float) , ImGui::SetScrollFromPosX , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetScrollFromPosX","ImGui::SetScrollFromPosX")
		->args({"local_x","center_x_ratio"})
		->arg_init(1,make_smart<ExprConstFloat>(0.5))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:441:29
	makeExtern< void (*)(float,float) , ImGui::SetScrollFromPosY , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetScrollFromPosY","ImGui::SetScrollFromPosY")
		->args({"local_y","center_y_ratio"})
		->arg_init(1,make_smart<ExprConstFloat>(0.5))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:444:29
	makeExtern< void (*)(ImFont *) , ImGui::PushFont , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushFont","ImGui::PushFont")
		->args({"font"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:445:29
	makeExtern< void (*)() , ImGui::PopFont , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopFont","ImGui::PopFont")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:446:29
	makeExtern< void (*)(int,unsigned int) , ImGui::PushStyleColor , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushStyleColor","ImGui::PushStyleColor")
		->args({"idx","col"})
		->arg_type(0,makeType<ImGuiCol_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:447:29
	makeExtern< void (*)(int,const ImVec4 &) , ImGui::PushStyleColor , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushStyleColor","ImGui::PushStyleColor")
		->args({"idx","col"})
		->arg_type(0,makeType<ImGuiCol_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:448:29
	makeExtern< void (*)(int) , ImGui::PopStyleColor , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopStyleColor","ImGui::PopStyleColor")
		->args({"count"})
		->arg_init(0,make_smart<ExprConstInt>(1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:449:29
	makeExtern< void (*)(int,float) , ImGui::PushStyleVar , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushStyleVar","ImGui::PushStyleVar")
		->args({"idx","val"})
		->arg_type(0,makeType<ImGuiStyleVar_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:450:29
	makeExtern< void (*)(int,const ImVec2 &) , ImGui::PushStyleVar , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushStyleVar","ImGui::PushStyleVar")
		->args({"idx","val"})
		->arg_type(0,makeType<ImGuiStyleVar_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:451:29
	makeExtern< void (*)(int) , ImGui::PopStyleVar , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopStyleVar","ImGui::PopStyleVar")
		->args({"count"})
		->arg_init(0,make_smart<ExprConstInt>(1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:452:29
	makeExtern< void (*)(int,bool) , ImGui::PushItemFlag , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushItemFlag","ImGui::PushItemFlag")
		->args({"option","enabled"})
		->arg_type(0,makeType<ImGuiItemFlags_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:453:29
	makeExtern< void (*)() , ImGui::PopItemFlag , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopItemFlag","ImGui::PopItemFlag")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:456:29
	makeExtern< void (*)(float) , ImGui::PushItemWidth , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushItemWidth","ImGui::PushItemWidth")
		->args({"item_width"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:457:29
	makeExtern< void (*)() , ImGui::PopItemWidth , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopItemWidth","ImGui::PopItemWidth")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:458:29
	makeExtern< void (*)(float) , ImGui::SetNextItemWidth , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextItemWidth","ImGui::SetNextItemWidth")
		->args({"item_width"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:459:29
	makeExtern< float (*)() , ImGui::CalcItemWidth , SimNode_ExtFuncCall , imguiTempFn>(lib,"CalcItemWidth","ImGui::CalcItemWidth")
		->addToModule(*this, SideEffects::worstDefault);
}
}

