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
void Module_dasIMGUI_NODE_EDITOR::initFunctions_3() {
// from imgui-node-editor/imgui_node_editor.h:330:28
	makeExtern< bool (*)(ax::NodeEditor::PinId *,const ImVec4 &,float) , ax::NodeEditor::QueryNewNode , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"QueryNewNode","ax::NodeEditor::QueryNewNode")
		->args({"pinId","color","thickness"})
		->arg_init(2,make_smart<ExprConstFloat>(1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:331:28
	makeExtern< bool (*)() , ax::NodeEditor::AcceptNewItem , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"AcceptNewItem","ax::NodeEditor::AcceptNewItem")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:332:28
	makeExtern< bool (*)(const ImVec4 &,float) , ax::NodeEditor::AcceptNewItem , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"AcceptNewItem","ax::NodeEditor::AcceptNewItem")
		->args({"color","thickness"})
		->arg_init(1,make_smart<ExprConstFloat>(1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:333:28
	makeExtern< void (*)() , ax::NodeEditor::RejectNewItem , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"RejectNewItem","ax::NodeEditor::RejectNewItem")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:334:28
	makeExtern< void (*)(const ImVec4 &,float) , ax::NodeEditor::RejectNewItem , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"RejectNewItem","ax::NodeEditor::RejectNewItem")
		->args({"color","thickness"})
		->arg_init(1,make_smart<ExprConstFloat>(1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:335:28
	makeExtern< void (*)() , ax::NodeEditor::EndCreate , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"EndCreate","ax::NodeEditor::EndCreate")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:337:28
	makeExtern< bool (*)() , ax::NodeEditor::BeginDelete , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"BeginDelete","ax::NodeEditor::BeginDelete")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:338:28
	makeExtern< bool (*)(ax::NodeEditor::LinkId *,ax::NodeEditor::PinId *,ax::NodeEditor::PinId *) , ax::NodeEditor::QueryDeletedLink , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"QueryDeletedLink","ax::NodeEditor::QueryDeletedLink")
		->args({"linkId","startId","endId"})
		->arg_init(1,make_smart<ExprConstPtr>())
		->arg_init(2,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:339:28
	makeExtern< bool (*)(ax::NodeEditor::NodeId *) , ax::NodeEditor::QueryDeletedNode , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"QueryDeletedNode","ax::NodeEditor::QueryDeletedNode")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:340:28
	makeExtern< bool (*)(bool) , ax::NodeEditor::AcceptDeletedItem , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"AcceptDeletedItem","ax::NodeEditor::AcceptDeletedItem")
		->args({"deleteDependencies"})
		->arg_init(0,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:341:28
	makeExtern< void (*)() , ax::NodeEditor::RejectDeletedItem , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"RejectDeletedItem","ax::NodeEditor::RejectDeletedItem")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:342:28
	makeExtern< void (*)() , ax::NodeEditor::EndDelete , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"EndDelete","ax::NodeEditor::EndDelete")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:344:28
	makeExtern< void (*)(ax::NodeEditor::NodeId,const ImVec2 &) , ax::NodeEditor::SetNodePosition , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"SetNodePosition","ax::NodeEditor::SetNodePosition")
		->args({"nodeId","editorPosition"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:345:28
	makeExtern< void (*)(ax::NodeEditor::NodeId,const ImVec2 &) , ax::NodeEditor::SetGroupSize , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"SetGroupSize","ax::NodeEditor::SetGroupSize")
		->args({"nodeId","size"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:346:30
	makeExtern< ImVec2 (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::GetNodePosition , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetNodePosition","ax::NodeEditor::GetNodePosition")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:347:30
	makeExtern< ImVec2 (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::GetNodeSize , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetNodeSize","ax::NodeEditor::GetNodeSize")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:348:28
	makeExtern< void (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::CenterNodeOnScreen , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"CenterNodeOnScreen","ax::NodeEditor::CenterNodeOnScreen")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:349:28
	makeExtern< void (*)(ax::NodeEditor::NodeId,float) , ax::NodeEditor::SetNodeZPosition , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"SetNodeZPosition","ax::NodeEditor::SetNodeZPosition")
		->args({"nodeId","z"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:350:29
	makeExtern< float (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::GetNodeZPosition , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetNodeZPosition","ax::NodeEditor::GetNodeZPosition")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui-node-editor/imgui_node_editor.h:352:28
	makeExtern< void (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::RestoreNodeState , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"RestoreNodeState","ax::NodeEditor::RestoreNodeState")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

