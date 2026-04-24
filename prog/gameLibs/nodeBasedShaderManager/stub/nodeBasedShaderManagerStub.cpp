// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <nodeBasedShaderManager/nodeBasedShaderManager.h>

static DataBlock emptyBlk;

NodeBasedShaderManager::~NodeBasedShaderManager() {}
bool NodeBasedShaderManager::loadFromResources(const String &, bool) { return true; }
bool NodeBasedShaderManager::update(const String &, const DataBlock &, String &) { return true; }
void NodeBasedShaderManager::getShadersBinariesFileNames(const String &, Tab<String> &, PLATFORM) const {}
void NodeBasedShaderManager::enableOptionalGraph(const String &, bool) {}
String NodeBasedShaderManager::buildScriptedShaderName(char const *) { return {}; };
char const *NodeBasedShaderManager::getShaderBlockName() const { return nullptr; }
void NodeBasedShaderManager::setShadervars(int) {}
