// this file is generated via daScript automatic C++ binder
// all user modifications will be lost after this file is re-generated

#pragma once
#include "cb_dasIMGUI.h"
#include "cb_dasIMGUI_NODE_EDITOR.h"
#include "need_dasIMGUI.h"
namespace das {
class Module_dasIMGUI_NODE_EDITOR : public Module {
public:
	Module_dasIMGUI_NODE_EDITOR();
protected:
virtual bool initDependencies() override;
	void initMain ();
	void initAotAlias ();
	virtual ModuleAotType aotRequire ( TextWriter & tw ) const override;
	#include "dasIMGUI_NODE_EDITOR.func.decl.inc"
public:
	ModuleLibrary lib;
	bool initialized = false;
};
}

