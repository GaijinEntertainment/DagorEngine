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
void Module_dasIMGUI_NODE_EDITOR::initFunctions_3() {
	makeExtern< bool (*)() , ax::NodeEditor::AcceptNewItem , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"AcceptNewItem","ax::NodeEditor::AcceptNewItem")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const ImVec4 &,float) , ax::NodeEditor::AcceptNewItem , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"AcceptNewItem","ax::NodeEditor::AcceptNewItem")
		->args({"color","thickness"})
		->arg_init(1,make_smart<ExprConstFloat>(1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ax::NodeEditor::RejectNewItem , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"RejectNewItem","ax::NodeEditor::RejectNewItem")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const ImVec4 &,float) , ax::NodeEditor::RejectNewItem , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"RejectNewItem","ax::NodeEditor::RejectNewItem")
		->args({"color","thickness"})
		->arg_init(1,make_smart<ExprConstFloat>(1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ax::NodeEditor::EndCreate , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"EndCreate","ax::NodeEditor::EndCreate")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ax::NodeEditor::BeginDelete , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"BeginDelete","ax::NodeEditor::BeginDelete")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(ax::NodeEditor::LinkId *,ax::NodeEditor::PinId *,ax::NodeEditor::PinId *) , ax::NodeEditor::QueryDeletedLink , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"QueryDeletedLink","ax::NodeEditor::QueryDeletedLink")
		->args({"linkId","startId","endId"})
		->arg_init(1,make_smart<ExprConstPtr>())
		->arg_init(2,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(ax::NodeEditor::NodeId *) , ax::NodeEditor::QueryDeletedNode , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"QueryDeletedNode","ax::NodeEditor::QueryDeletedNode")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ax::NodeEditor::AcceptDeletedItem , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"AcceptDeletedItem","ax::NodeEditor::AcceptDeletedItem")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ax::NodeEditor::RejectDeletedItem , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"RejectDeletedItem","ax::NodeEditor::RejectDeletedItem")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ax::NodeEditor::EndDelete , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"EndDelete","ax::NodeEditor::EndDelete")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(ax::NodeEditor::NodeId,const ImVec2 &) , ax::NodeEditor::SetNodePosition , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"SetNodePosition","ax::NodeEditor::SetNodePosition")
		->args({"nodeId","editorPosition"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::GetNodePosition , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetNodePosition","ax::NodeEditor::GetNodePosition")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::GetNodeSize , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetNodeSize","ax::NodeEditor::GetNodeSize")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::CenterNodeOnScreen , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"CenterNodeOnScreen","ax::NodeEditor::CenterNodeOnScreen")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::RestoreNodeState , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"RestoreNodeState","ax::NodeEditor::RestoreNodeState")
		->args({"nodeId"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ax::NodeEditor::Suspend , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"Suspend","ax::NodeEditor::Suspend")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ax::NodeEditor::Resume , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"Resume","ax::NodeEditor::Resume")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ax::NodeEditor::IsSuspended , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"IsSuspended","ax::NodeEditor::IsSuspended")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ax::NodeEditor::IsActive , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"IsActive","ax::NodeEditor::IsActive")
		->addToModule(*this, SideEffects::worstDefault);
}
}

