// this file is generated via Daslang automatic C++ binder
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
void Module_dasIMGUI_NODE_EDITOR::initFunctions_6() {
// from imgui-node-editor/imgui_node_editor.h:404:30
	makeExtern< ax::NodeEditor::NodeId (*)() , ax::NodeEditor::GetHoveredNode , SimNode_ExtFuncCallAndCopyOrMove , imgui_node_editorTempFn>(lib,"GetHoveredNode","ax::NodeEditor::GetHoveredNode")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:405:29
	makeExtern< ax::NodeEditor::PinId (*)() , ax::NodeEditor::GetHoveredPin , SimNode_ExtFuncCallAndCopyOrMove , imgui_node_editorTempFn>(lib,"GetHoveredPin","ax::NodeEditor::GetHoveredPin")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:406:30
	makeExtern< ax::NodeEditor::LinkId (*)() , ax::NodeEditor::GetHoveredLink , SimNode_ExtFuncCallAndCopyOrMove , imgui_node_editorTempFn>(lib,"GetHoveredLink","ax::NodeEditor::GetHoveredLink")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:407:30
	makeExtern< ax::NodeEditor::NodeId (*)() , ax::NodeEditor::GetDoubleClickedNode , SimNode_ExtFuncCallAndCopyOrMove , imgui_node_editorTempFn>(lib,"GetDoubleClickedNode","ax::NodeEditor::GetDoubleClickedNode")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:408:29
	makeExtern< ax::NodeEditor::PinId (*)() , ax::NodeEditor::GetDoubleClickedPin , SimNode_ExtFuncCallAndCopyOrMove , imgui_node_editorTempFn>(lib,"GetDoubleClickedPin","ax::NodeEditor::GetDoubleClickedPin")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:409:30
	makeExtern< ax::NodeEditor::LinkId (*)() , ax::NodeEditor::GetDoubleClickedLink , SimNode_ExtFuncCallAndCopyOrMove , imgui_node_editorTempFn>(lib,"GetDoubleClickedLink","ax::NodeEditor::GetDoubleClickedLink")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:410:28
	makeExtern< bool (*)() , ax::NodeEditor::IsBackgroundClicked , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"IsBackgroundClicked","ax::NodeEditor::IsBackgroundClicked")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:411:28
	makeExtern< bool (*)() , ax::NodeEditor::IsBackgroundDoubleClicked , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"IsBackgroundDoubleClicked","ax::NodeEditor::IsBackgroundDoubleClicked")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:412:40
	makeExtern< int (*)() , ax::NodeEditor::GetBackgroundClickButtonIndex , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetBackgroundClickButtonIndex","ax::NodeEditor::GetBackgroundClickButtonIndex")
		->res_type(makeType<ImGuiMouseButton_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:413:40
	makeExtern< int (*)() , ax::NodeEditor::GetBackgroundDoubleClickButtonIndex , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetBackgroundDoubleClickButtonIndex","ax::NodeEditor::GetBackgroundDoubleClickButtonIndex")
		->res_type(makeType<ImGuiMouseButton_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:415:28
	makeExtern< bool (*)(ax::NodeEditor::LinkId,ax::NodeEditor::PinId *,ax::NodeEditor::PinId *) , ax::NodeEditor::GetLinkPins , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetLinkPins","ax::NodeEditor::GetLinkPins")
		->args({"linkId","startPinId","endPinId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:417:28
	makeExtern< bool (*)(ax::NodeEditor::PinId) , ax::NodeEditor::PinHadAnyLinks , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"PinHadAnyLinks","ax::NodeEditor::PinHadAnyLinks")
		->args({"pinId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:419:30
	makeExtern< ImVec2 (*)() , ax::NodeEditor::GetScreenSize , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetScreenSize","ax::NodeEditor::GetScreenSize")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:420:30
	makeExtern< ImVec2 (*)(const ImVec2 &) , ax::NodeEditor::ScreenToCanvas , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"ScreenToCanvas","ax::NodeEditor::ScreenToCanvas")
		->args({"pos"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:421:30
	makeExtern< ImVec2 (*)(const ImVec2 &) , ax::NodeEditor::CanvasToScreen , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"CanvasToScreen","ax::NodeEditor::CanvasToScreen")
		->args({"pos"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:423:27
	makeExtern< int (*)() , ax::NodeEditor::GetNodeCount , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetNodeCount","ax::NodeEditor::GetNodeCount")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:424:27
	makeExtern< int (*)(ax::NodeEditor::NodeId *,int) , ax::NodeEditor::GetOrderedNodeIds , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetOrderedNodeIds","ax::NodeEditor::GetOrderedNodeIds")
		->args({"nodes","size"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

