// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/nodeBasedShader.h>
#include <nodeBasedShaderManager/nodeBasedShaderManager.h>

static DataBlock emptyBlk;

NodeBasedShader::NodeBasedShader(NodeBasedShaderType, const String &, const String &, uint32_t) {}
NodeBasedShader::~NodeBasedShader() = default;
void NodeBasedShader::dispatch(int, int, int) const {}
void NodeBasedShader::setArrayValue(const char *, const Tab<Point4> &) {}
bool NodeBasedShader::update(const String &, const DataBlock &, String &) { return true; }
void NodeBasedShader::init(const DataBlock &) {}
void NodeBasedShader::init(const String &, bool) {}
void NodeBasedShader::closeShader() {}
void NodeBasedShader::reset() {}
bool NodeBasedShader::isValid() const { return false; }
void NodeBasedShader::enableOptionalGraph(const String &, bool) {}
PROGRAM *NodeBasedShader::getProgram() { return nullptr; }
const DataBlock &NodeBasedShader::getMetadata() const { return emptyBlk; }

namespace nodebasedshaderutils
{
eastl::vector<String> getAvailableInt() { return {}; }
eastl::vector<String> getAvailableFloat() { return {}; }
eastl::vector<String> getAvailableFloat4() { return {}; }
eastl::vector<String> getAvailableTextures() { return {}; }
} // namespace nodebasedshaderutils

eastl::string node_based_shader_get_resource_name(const char *) { return eastl::string(); }
