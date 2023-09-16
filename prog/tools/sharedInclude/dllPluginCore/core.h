#ifndef __GAIJIN_DLL_PLUGIN_CORE__
#define __GAIJIN_DLL_PLUGIN_CORE__
#pragma once


#include <EditorCore/ec_IEditorCore.h>


extern IDagorRender *dagRender;
extern IDagorGeom *dagGeom;
extern IDagorConsole *dagConsole;
extern IDagorTools *dagTools;
extern IDagorScene *dagScene;

extern IEditorCore *editorCore;


void init_dll_plugin_core(IEditorCore &core_iface);

String make_full_start_path(const char *rel_path);

#endif //__GAIJIN_DLL_PLUGIN_CORE__
