// this file is generated via daScript automatic C++ binder
// all user modifications will be lost after this file is re-generated

#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/bind_enum.h"
#include "dasIMGUI_NODE_EDITOR.h"
#include "need_dasIMGUI_NODE_EDITOR.h"
namespace das {
#include "dasIMGUI_NODE_EDITOR.func.aot.decl.inc"
void Module_dasIMGUI_NODE_EDITOR::initFunctions_2() {
	makeExtern< void (*)(const ImVec2 &) , ax::NodeEditor::PinPivotSize , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"PinPivotSize","ax::NodeEditor::PinPivotSize")
		->args({"size"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const ImVec2 &) , ax::NodeEditor::PinPivotScale , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"PinPivotScale","ax::NodeEditor::PinPivotScale")
		->args({"scale"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const ImVec2 &) , ax::NodeEditor::PinPivotAlignment , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"PinPivotAlignment","ax::NodeEditor::PinPivotAlignment")
		->args({"alignment"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ax::NodeEditor::EndPin , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"EndPin","ax::NodeEditor::EndPin")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const ImVec2 &) , ax::NodeEditor::Group , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"Group","ax::NodeEditor::Group")
		->args({"size"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ax::NodeEditor::EndNode , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"EndNode","ax::NodeEditor::EndNode")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::BeginGroupHint , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"BeginGroupHint","ax::NodeEditor::BeginGroupHint")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)() , ax::NodeEditor::GetGroupMin , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetGroupMin","ax::NodeEditor::GetGroupMin")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)() , ax::NodeEditor::GetGroupMax , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetGroupMax","ax::NodeEditor::GetGroupMax")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImDrawList * (*)() , ax::NodeEditor::GetHintForegroundDrawList , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetHintForegroundDrawList","ax::NodeEditor::GetHintForegroundDrawList")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImDrawList * (*)() , ax::NodeEditor::GetHintBackgroundDrawList , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetHintBackgroundDrawList","ax::NodeEditor::GetHintBackgroundDrawList")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ax::NodeEditor::EndGroupHint , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"EndGroupHint","ax::NodeEditor::EndGroupHint")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImDrawList * (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::GetNodeBackgroundDrawList , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetNodeBackgroundDrawList","ax::NodeEditor::GetNodeBackgroundDrawList")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(ax::NodeEditor::LinkId,ax::NodeEditor::PinId,ax::NodeEditor::PinId,const ImVec4 &,float) , ax::NodeEditor::Link , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"Link","ax::NodeEditor::Link")
		->args({"id","startPinId","endPinId","color","thickness"})
		->arg_init(4,make_smart<ExprConstFloat>(1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(ax::NodeEditor::LinkId) , ax::NodeEditor::Flow , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"Flow","ax::NodeEditor::Flow")
		->args({"linkId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const ImVec4 &,float) , ax::NodeEditor::BeginCreate , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"BeginCreate","ax::NodeEditor::BeginCreate")
		->args({"color","thickness"})
		->arg_init(1,make_smart<ExprConstFloat>(1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(ax::NodeEditor::PinId *,ax::NodeEditor::PinId *) , ax::NodeEditor::QueryNewLink , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"QueryNewLink","ax::NodeEditor::QueryNewLink")
		->args({"startId","endId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(ax::NodeEditor::PinId *,ax::NodeEditor::PinId *,const ImVec4 &,float) , ax::NodeEditor::QueryNewLink , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"QueryNewLink","ax::NodeEditor::QueryNewLink")
		->args({"startId","endId","color","thickness"})
		->arg_init(3,make_smart<ExprConstFloat>(1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(ax::NodeEditor::PinId *) , ax::NodeEditor::QueryNewNode , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"QueryNewNode","ax::NodeEditor::QueryNewNode")
		->args({"pinId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(ax::NodeEditor::PinId *,const ImVec4 &,float) , ax::NodeEditor::QueryNewNode , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"QueryNewNode","ax::NodeEditor::QueryNewNode")
		->args({"pinId","color","thickness"})
		->arg_init(2,make_smart<ExprConstFloat>(1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
}
}

