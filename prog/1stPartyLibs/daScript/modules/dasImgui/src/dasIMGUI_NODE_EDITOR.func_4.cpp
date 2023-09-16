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
void Module_dasIMGUI_NODE_EDITOR::initFunctions_4() {
	makeExtern< bool (*)() , ax::NodeEditor::HasSelectionChanged , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"HasSelectionChanged","ax::NodeEditor::HasSelectionChanged")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< int (*)() , ax::NodeEditor::GetSelectedObjectCount , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetSelectedObjectCount","ax::NodeEditor::GetSelectedObjectCount")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< int (*)(ax::NodeEditor::NodeId *,int) , ax::NodeEditor::GetSelectedNodes , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetSelectedNodes","ax::NodeEditor::GetSelectedNodes")
		->args({"nodes","size"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< int (*)(ax::NodeEditor::LinkId *,int) , ax::NodeEditor::GetSelectedLinks , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetSelectedLinks","ax::NodeEditor::GetSelectedLinks")
		->args({"links","size"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ax::NodeEditor::ClearSelection , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"ClearSelection","ax::NodeEditor::ClearSelection")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(ax::NodeEditor::NodeId,bool) , ax::NodeEditor::SelectNode , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"SelectNode","ax::NodeEditor::SelectNode")
		->args({"nodeId","append"})
		->arg_init(1,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(ax::NodeEditor::LinkId,bool) , ax::NodeEditor::SelectLink , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"SelectLink","ax::NodeEditor::SelectLink")
		->args({"linkId","append"})
		->arg_init(1,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::DeselectNode , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"DeselectNode","ax::NodeEditor::DeselectNode")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(ax::NodeEditor::LinkId) , ax::NodeEditor::DeselectLink , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"DeselectLink","ax::NodeEditor::DeselectLink")
		->args({"linkId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::DeleteNode , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"DeleteNode","ax::NodeEditor::DeleteNode")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(ax::NodeEditor::LinkId) , ax::NodeEditor::DeleteLink , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"DeleteLink","ax::NodeEditor::DeleteLink")
		->args({"linkId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(float) , ax::NodeEditor::NavigateToContent , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"NavigateToContent","ax::NodeEditor::NavigateToContent")
		->args({"duration"})
		->arg_init(0,make_smart<ExprConstFloat>(-1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(bool,float) , ax::NodeEditor::NavigateToSelection , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"NavigateToSelection","ax::NodeEditor::NavigateToSelection")
		->args({"zoomIn","duration"})
		->arg_init(0,make_smart<ExprConstBool>(false))
		->arg_init(1,make_smart<ExprConstFloat>(-1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(ax::NodeEditor::NodeId *) , ax::NodeEditor::ShowNodeContextMenu , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"ShowNodeContextMenu","ax::NodeEditor::ShowNodeContextMenu")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(ax::NodeEditor::PinId *) , ax::NodeEditor::ShowPinContextMenu , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"ShowPinContextMenu","ax::NodeEditor::ShowPinContextMenu")
		->args({"pinId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(ax::NodeEditor::LinkId *) , ax::NodeEditor::ShowLinkContextMenu , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"ShowLinkContextMenu","ax::NodeEditor::ShowLinkContextMenu")
		->args({"linkId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ax::NodeEditor::ShowBackgroundContextMenu , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"ShowBackgroundContextMenu","ax::NodeEditor::ShowBackgroundContextMenu")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(bool) , ax::NodeEditor::EnableShortcuts , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"EnableShortcuts","ax::NodeEditor::EnableShortcuts")
		->args({"enable"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ax::NodeEditor::AreShortcutsEnabled , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"AreShortcutsEnabled","ax::NodeEditor::AreShortcutsEnabled")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ax::NodeEditor::BeginShortcut , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"BeginShortcut","ax::NodeEditor::BeginShortcut")
		->addToModule(*this, SideEffects::worstDefault);
}
}

