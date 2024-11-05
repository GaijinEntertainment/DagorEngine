// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <nodeBasedShaderManager/nodeBasedShaderManager.h>

void NodeBasedShaderManager::initCompilation() {}
bool NodeBasedShaderManager::compileShaderProgram(const DataBlock &, String &, PLATFORM) { return false; }
void NodeBasedShaderManager::saveToFile(const String &, PLATFORM) const {}
void NodeBasedShaderManager::getShadersBinariesFileNames(const String &, Tab<String> &, PLATFORM) const {}
