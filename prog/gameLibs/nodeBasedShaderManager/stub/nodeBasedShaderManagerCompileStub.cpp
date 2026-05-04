// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <nodeBasedShaderManager/nodeBasedShaderManager.h>

String NodeBasedShaderManager::toolsPath;
String NodeBasedShaderManager::rootPath;

void NodeBasedShaderManager::initCompilation() {}
bool NodeBasedShaderManager::compileScriptedShaders(const String &, const DataBlock &, String &) { return false; }
void NodeBasedShaderManager::getShadersBinariesFileNames(const String &, Tab<String> &, PLATFORM) const {}
