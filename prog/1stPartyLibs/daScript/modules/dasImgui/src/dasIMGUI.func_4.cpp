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
// from imgui/imgui.h:505:29
	makeExtern< float (*)() , ImGui::GetScrollY , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetScrollY","ImGui::GetScrollY")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:506:29
	makeExtern< void (*)(float) , ImGui::SetScrollX , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetScrollX","ImGui::SetScrollX")
		->args({"scroll_x"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:507:29
	makeExtern< void (*)(float) , ImGui::SetScrollY , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetScrollY","ImGui::SetScrollY")
		->args({"scroll_y"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:508:29
	makeExtern< float (*)() , ImGui::GetScrollMaxX , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetScrollMaxX","ImGui::GetScrollMaxX")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:509:29
	makeExtern< float (*)() , ImGui::GetScrollMaxY , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetScrollMaxY","ImGui::GetScrollMaxY")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:510:29
	makeExtern< void (*)(float) , ImGui::SetScrollHereX , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetScrollHereX","ImGui::SetScrollHereX")
		->args({"center_x_ratio"})
		->arg_init(0,make_smart<ExprConstFloat>(0.5))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:511:29
	makeExtern< void (*)(float) , ImGui::SetScrollHereY , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetScrollHereY","ImGui::SetScrollHereY")
		->args({"center_y_ratio"})
		->arg_init(0,make_smart<ExprConstFloat>(0.5))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:512:29
	makeExtern< void (*)(float,float) , ImGui::SetScrollFromPosX , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetScrollFromPosX","ImGui::SetScrollFromPosX")
		->args({"local_x","center_x_ratio"})
		->arg_init(1,make_smart<ExprConstFloat>(0.5))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:513:29
	makeExtern< void (*)(float,float) , ImGui::SetScrollFromPosY , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetScrollFromPosY","ImGui::SetScrollFromPosY")
		->args({"local_y","center_y_ratio"})
		->arg_init(1,make_smart<ExprConstFloat>(0.5))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:531:29
	makeExtern< void (*)(ImFont *,float) , ImGui::PushFont , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushFont","ImGui::PushFont")
		->args({"font","font_size_base_unscaled"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:532:29
	makeExtern< void (*)() , ImGui::PopFont , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopFont","ImGui::PopFont")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:533:29
	makeExtern< ImFont * (*)() , ImGui::GetFont , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFont","ImGui::GetFont")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:534:29
	makeExtern< float (*)() , ImGui::GetFontSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFontSize","ImGui::GetFontSize")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:535:29
	makeExtern< ImFontBaked * (*)() , ImGui::GetFontBaked , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFontBaked","ImGui::GetFontBaked")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:538:29
	makeExtern< void (*)(int,unsigned int) , ImGui::PushStyleColor , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushStyleColor","ImGui::PushStyleColor")
		->args({"idx","col"})
		->arg_type(0,makeType<ImGuiCol_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:539:29
	makeExtern< void (*)(int,const ImVec4 &) , ImGui::PushStyleColor , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushStyleColor","ImGui::PushStyleColor")
		->args({"idx","col"})
		->arg_type(0,makeType<ImGuiCol_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:540:29
	makeExtern< void (*)(int) , ImGui::PopStyleColor , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopStyleColor","ImGui::PopStyleColor")
		->args({"count"})
		->arg_init(0,make_smart<ExprConstInt>(1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:541:29
	makeExtern< void (*)(int,float) , ImGui::PushStyleVar , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushStyleVar","ImGui::PushStyleVar")
		->args({"idx","val"})
		->arg_type(0,makeType<ImGuiStyleVar_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:542:29
	makeExtern< void (*)(int,const ImVec2 &) , ImGui::PushStyleVar , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushStyleVar","ImGui::PushStyleVar")
		->args({"idx","val"})
		->arg_type(0,makeType<ImGuiStyleVar_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:543:29
	makeExtern< void (*)(int,float) , ImGui::PushStyleVarX , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushStyleVarX","ImGui::PushStyleVarX")
		->args({"idx","val_x"})
		->arg_type(0,makeType<ImGuiStyleVar_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
}
}

