// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <nodeBasedShaderManager/nodeBasedShaderManager.h>

static DataBlock emptyBlk;

NodeBasedShaderManager::~NodeBasedShaderManager() {}
bool NodeBasedShaderManager::loadFromResources(const String &, bool) { return true; }
void NodeBasedShaderManager::setArrayValue(const char *, const Tab<Point4> &) {}
bool NodeBasedShaderManager::update(const DataBlock &, String &, PLATFORM) { return true; }
const NodeBasedShaderManager::ShaderBinPermutations &NodeBasedShaderManager::getPermutationShadersBin(uint32_t) const
{
  return shaderBin[0][0];
}
void NodeBasedShaderManager::setConstants() {}
void NodeBasedShaderManager::clearAllCachedResources() {}
void NodeBasedShaderManager::getPlatformList(Tab<String> &, PLATFORM) {}
bool NodeBasedShaderManager::compileShaderProgram(const DataBlock &, String &, PLATFORM) { return false; }
void NodeBasedShaderManager::saveToFile(const String &, PLATFORM) const {}
void NodeBasedShaderManager::getShadersBinariesFileNames(const String &, Tab<String> &, PLATFORM) const {}
void NodeBasedShaderManager::enableOptionalGraph(const String &, bool) {}
const DataBlock &NodeBasedShaderManager::getMetadata() const { return emptyBlk; }