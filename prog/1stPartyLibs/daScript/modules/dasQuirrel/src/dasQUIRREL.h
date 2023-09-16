// this file is generated via daScript automatic C++ binder
// all user modifications will be lost after this file is re-generated

#pragma once
namespace das {
class Module_dasQUIRREL : public Module {
public:
	Module_dasQUIRREL();
protected:
virtual bool initDependencies() override;
	void initMain ();
	void initBind ();
	virtual ModuleAotType aotRequire ( TextWriter & tw ) const override;
	#include "dasQUIRREL.func.decl.inc"
public:
	ModuleLibrary lib;
	bool initialized = false;
};
}

