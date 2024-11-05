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
void Module_dasIMGUI_NODE_EDITOR::initFunctions_4() {
// from imgui-node-editor/imgui_node_editor.h:354:28
	makeExtern< void (*)() , ax::NodeEditor::Suspend , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"Suspend","ax::NodeEditor::Suspend")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:355:28
	makeExtern< void (*)() , ax::NodeEditor::Resume , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"Resume","ax::NodeEditor::Resume")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:356:28
	makeExtern< bool (*)() , ax::NodeEditor::IsSuspended , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"IsSuspended","ax::NodeEditor::IsSuspended")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:358:28
	makeExtern< bool (*)() , ax::NodeEditor::IsActive , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"IsActive","ax::NodeEditor::IsActive")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:360:28
	makeExtern< bool (*)() , ax::NodeEditor::HasSelectionChanged , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"HasSelectionChanged","ax::NodeEditor::HasSelectionChanged")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:361:28
	makeExtern< int (*)() , ax::NodeEditor::GetSelectedObjectCount , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetSelectedObjectCount","ax::NodeEditor::GetSelectedObjectCount")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:362:28
	makeExtern< int (*)(ax::NodeEditor::NodeId *,int) , ax::NodeEditor::GetSelectedNodes , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetSelectedNodes","ax::NodeEditor::GetSelectedNodes")
		->args({"nodes","size"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:363:28
	makeExtern< int (*)(ax::NodeEditor::LinkId *,int) , ax::NodeEditor::GetSelectedLinks , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetSelectedLinks","ax::NodeEditor::GetSelectedLinks")
		->args({"links","size"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:364:28
	makeExtern< bool (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::IsNodeSelected , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"IsNodeSelected","ax::NodeEditor::IsNodeSelected")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:365:28
	makeExtern< bool (*)(ax::NodeEditor::LinkId) , ax::NodeEditor::IsLinkSelected , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"IsLinkSelected","ax::NodeEditor::IsLinkSelected")
		->args({"linkId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:366:28
	makeExtern< void (*)() , ax::NodeEditor::ClearSelection , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"ClearSelection","ax::NodeEditor::ClearSelection")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:367:28
	makeExtern< void (*)(ax::NodeEditor::NodeId,bool) , ax::NodeEditor::SelectNode , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"SelectNode","ax::NodeEditor::SelectNode")
		->args({"nodeId","append"})
		->arg_init(1,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:368:28
	makeExtern< void (*)(ax::NodeEditor::LinkId,bool) , ax::NodeEditor::SelectLink , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"SelectLink","ax::NodeEditor::SelectLink")
		->args({"linkId","append"})
		->arg_init(1,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:369:28
	makeExtern< void (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::DeselectNode , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"DeselectNode","ax::NodeEditor::DeselectNode")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:370:28
	makeExtern< void (*)(ax::NodeEditor::LinkId) , ax::NodeEditor::DeselectLink , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"DeselectLink","ax::NodeEditor::DeselectLink")
		->args({"linkId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:372:28
	makeExtern< bool (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::DeleteNode , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"DeleteNode","ax::NodeEditor::DeleteNode")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:373:28
	makeExtern< bool (*)(ax::NodeEditor::LinkId) , ax::NodeEditor::DeleteLink , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"DeleteLink","ax::NodeEditor::DeleteLink")
		->args({"linkId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:375:28
	makeExtern< bool (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::HasAnyLinks , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"HasAnyLinks","ax::NodeEditor::HasAnyLinks")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:376:28
	makeExtern< bool (*)(ax::NodeEditor::PinId) , ax::NodeEditor::HasAnyLinks , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"HasAnyLinks","ax::NodeEditor::HasAnyLinks")
		->args({"pinId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:377:27
	makeExtern< int (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::BreakLinks , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"BreakLinks","ax::NodeEditor::BreakLinks")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

