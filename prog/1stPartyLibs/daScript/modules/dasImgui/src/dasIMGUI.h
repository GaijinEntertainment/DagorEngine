// this file is generated via Daslang automatic C++ binder
// all user modifications will be lost after this file is re-generated

#pragma once
#include "cb_dasIMGUI.h"
namespace das {
class Module_dasIMGUI : public Module {
public:
	Module_dasIMGUI();
protected:
virtual bool initDependencies() override;
	void initMain ();
	void initAotAlias ();
	virtual ModuleAotType aotRequire ( TextWriter & tw ) const override;
	#include "dasIMGUI.func.decl.inc"
public:
	ModuleLibrary lib;
	bool initialized = false;
};
}

