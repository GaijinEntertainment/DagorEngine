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
void Module_dasIMGUI_NODE_EDITOR::initFunctions_5() {
// from imgui-node-editor/imgui_node_editor.h:378:27
	makeExtern< int (*)(ax::NodeEditor::PinId) , ax::NodeEditor::BreakLinks , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"BreakLinks","ax::NodeEditor::BreakLinks")
		->args({"pinId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:380:28
	makeExtern< void (*)(float) , ax::NodeEditor::NavigateToContent , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"NavigateToContent","ax::NodeEditor::NavigateToContent")
		->args({"duration"})
		->arg_init(0,make_smart<ExprConstFloat>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:381:28
	makeExtern< void (*)(bool,float) , ax::NodeEditor::NavigateToSelection , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"NavigateToSelection","ax::NodeEditor::NavigateToSelection")
		->args({"zoomIn","duration"})
		->arg_init(0,make_smart<ExprConstBool>(false))
		->arg_init(1,make_smart<ExprConstFloat>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:383:28
	makeExtern< bool (*)(ax::NodeEditor::NodeId *) , ax::NodeEditor::ShowNodeContextMenu , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"ShowNodeContextMenu","ax::NodeEditor::ShowNodeContextMenu")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:384:28
	makeExtern< bool (*)(ax::NodeEditor::PinId *) , ax::NodeEditor::ShowPinContextMenu , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"ShowPinContextMenu","ax::NodeEditor::ShowPinContextMenu")
		->args({"pinId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:385:28
	makeExtern< bool (*)(ax::NodeEditor::LinkId *) , ax::NodeEditor::ShowLinkContextMenu , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"ShowLinkContextMenu","ax::NodeEditor::ShowLinkContextMenu")
		->args({"linkId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:386:28
	makeExtern< bool (*)() , ax::NodeEditor::ShowBackgroundContextMenu , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"ShowBackgroundContextMenu","ax::NodeEditor::ShowBackgroundContextMenu")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:388:28
	makeExtern< void (*)(bool) , ax::NodeEditor::EnableShortcuts , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"EnableShortcuts","ax::NodeEditor::EnableShortcuts")
		->args({"enable"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:389:28
	makeExtern< bool (*)() , ax::NodeEditor::AreShortcutsEnabled , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"AreShortcutsEnabled","ax::NodeEditor::AreShortcutsEnabled")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:391:28
	makeExtern< bool (*)() , ax::NodeEditor::BeginShortcut , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"BeginShortcut","ax::NodeEditor::BeginShortcut")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:392:28
	makeExtern< bool (*)() , ax::NodeEditor::AcceptCut , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"AcceptCut","ax::NodeEditor::AcceptCut")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:393:28
	makeExtern< bool (*)() , ax::NodeEditor::AcceptCopy , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"AcceptCopy","ax::NodeEditor::AcceptCopy")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:394:28
	makeExtern< bool (*)() , ax::NodeEditor::AcceptPaste , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"AcceptPaste","ax::NodeEditor::AcceptPaste")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:395:28
	makeExtern< bool (*)() , ax::NodeEditor::AcceptDuplicate , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"AcceptDuplicate","ax::NodeEditor::AcceptDuplicate")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:396:28
	makeExtern< bool (*)() , ax::NodeEditor::AcceptCreateNode , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"AcceptCreateNode","ax::NodeEditor::AcceptCreateNode")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:397:28
	makeExtern< int (*)() , ax::NodeEditor::GetActionContextSize , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetActionContextSize","ax::NodeEditor::GetActionContextSize")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:398:28
	makeExtern< int (*)(ax::NodeEditor::NodeId *,int) , ax::NodeEditor::GetActionContextNodes , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetActionContextNodes","ax::NodeEditor::GetActionContextNodes")
		->args({"nodes","size"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:399:28
	makeExtern< int (*)(ax::NodeEditor::LinkId *,int) , ax::NodeEditor::GetActionContextLinks , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetActionContextLinks","ax::NodeEditor::GetActionContextLinks")
		->args({"links","size"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:400:28
	makeExtern< void (*)() , ax::NodeEditor::EndShortcut , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"EndShortcut","ax::NodeEditor::EndShortcut")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:402:29
	makeExtern< float (*)() , ax::NodeEditor::GetCurrentZoom , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetCurrentZoom","ax::NodeEditor::GetCurrentZoom")
		->addToModule(*this, SideEffects::worstDefault);
}
}

