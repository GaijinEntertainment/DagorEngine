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
void Module_dasIMGUI_NODE_EDITOR::initFunctions_1() {
	addCtorAndUsing<ax::NodeEditor::Config>(*this,lib,"Config","ax::NodeEditor::Config");
	addCtorAndUsing<ax::NodeEditor::Style>(*this,lib,"Style","ax::NodeEditor::Style");
	makeExtern< void (*)(ax::NodeEditor::EditorContext *) , ax::NodeEditor::SetCurrentEditor , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"SetCurrentEditor","ax::NodeEditor::SetCurrentEditor")
		->args({"ctx"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ax::NodeEditor::EditorContext * (*)() , ax::NodeEditor::GetCurrentEditor , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetCurrentEditor","ax::NodeEditor::GetCurrentEditor")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ax::NodeEditor::EditorContext * (*)(const ax::NodeEditor::Config *) , ax::NodeEditor::CreateEditor , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"CreateEditor","ax::NodeEditor::CreateEditor")
		->args({"config"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(ax::NodeEditor::EditorContext *) , ax::NodeEditor::DestroyEditor , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"DestroyEditor","ax::NodeEditor::DestroyEditor")
		->args({"ctx"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ax::NodeEditor::Style & (*)() , ax::NodeEditor::GetStyle , SimNode_ExtFuncCallRef , imgui_node_editorTempFn>(lib,"GetStyle","ax::NodeEditor::GetStyle")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< const char * (*)(ax::NodeEditor::StyleColor) , ax::NodeEditor::GetStyleColorName , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"GetStyleColorName","ax::NodeEditor::GetStyleColorName")
		->args({"colorIndex"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(ax::NodeEditor::StyleColor,const ImVec4 &) , ax::NodeEditor::PushStyleColor , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"PushStyleColor","ax::NodeEditor::PushStyleColor")
		->args({"colorIndex","color"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(int) , ax::NodeEditor::PopStyleColor , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"PopStyleColor","ax::NodeEditor::PopStyleColor")
		->args({"count"})
		->arg_init(0,make_smart<ExprConstInt>(1))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(ax::NodeEditor::StyleVar,float) , ax::NodeEditor::PushStyleVar , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"PushStyleVar","ax::NodeEditor::PushStyleVar")
		->args({"varIndex","value"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(ax::NodeEditor::StyleVar,const ImVec2 &) , ax::NodeEditor::PushStyleVar , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"PushStyleVar","ax::NodeEditor::PushStyleVar")
		->args({"varIndex","value"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(ax::NodeEditor::StyleVar,const ImVec4 &) , ax::NodeEditor::PushStyleVar , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"PushStyleVar","ax::NodeEditor::PushStyleVar")
		->args({"varIndex","value"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(int) , ax::NodeEditor::PopStyleVar , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"PopStyleVar","ax::NodeEditor::PopStyleVar")
		->args({"count"})
		->arg_init(0,make_smart<ExprConstInt>(1))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *,const ImVec2 &) , ax::NodeEditor::Begin , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"Begin","ax::NodeEditor::Begin")
		->args({"id","size"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ax::NodeEditor::End , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"End","ax::NodeEditor::End")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(ax::NodeEditor::NodeId) , ax::NodeEditor::BeginNode , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"BeginNode","ax::NodeEditor::BeginNode")
		->args({"id"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(ax::NodeEditor::PinId,ax::NodeEditor::PinKind) , ax::NodeEditor::BeginPin , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"BeginPin","ax::NodeEditor::BeginPin")
		->args({"id","kind"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const ImVec2 &,const ImVec2 &) , ax::NodeEditor::PinRect , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"PinRect","ax::NodeEditor::PinRect")
		->args({"a","b"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const ImVec2 &,const ImVec2 &) , ax::NodeEditor::PinPivotRect , SimNode_ExtFuncCall , imgui_node_editorTempFn>(lib,"PinPivotRect","ax::NodeEditor::PinPivotRect")
		->args({"a","b"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

