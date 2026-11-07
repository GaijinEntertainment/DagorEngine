// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ioSys/dag_dataBlock.h>

#include <EASTL/hash_map.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

#include <stdint.h>

namespace texgen
{
// Appends the "texture ..." size, type and wrap comboboxes a texture or particles producer lacks; skips "<graph properties>".
void preprocess_node_descriptor(DataBlock &node_desc);

// A string, int, real, bool or point4 param as text; else "".
eastl::string param_as_string(const DataBlock *blk, const char *name);

enum class DescriptorSource : uint8_t
{
  Base,
  ShaderTemplate,
  Subgraph,
  Pack,
};

// node{} descriptors in the base_nodes.blk form, preprocessed when added. A descriptor pointer stays valid until clear().
class NodeRegistry
{
public:
  // False when the file does not load. errors gets one line per problem:
  // a load failure or error, a node with no templateUid, a replace.
  bool loadBaseNodes(const char *path, eastl::string &errors);
  // True when it replaced the descriptor of the same templateUid; the replace is in place.
  bool addDescriptor(const DataBlock &node, DescriptorSource source);
  // <root_dir>/<folder>/node.blk in folder name order, nativeShader "pack_<folder>" if unset. False: no root_dir or a folder skipped.
  bool loadPacks(const char *root_dir, eastl::string &errors);
  void clear();

  const DataBlock &descriptors() const { return blk; }
  const DataBlock *findByUid(const char *uid) const;
  // For graphs saved before templateUid: the first descriptor with that name:t.
  const DataBlock *findByName(const char *name) const;

private:
  DataBlock blk;
  eastl::vector<DescriptorSource> sources;
  eastl::hash_map<eastl::string, uint32_t> indexByUid;
};
} // namespace texgen
