// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameLibs/textureGen/registry/texgenRegistry_api.h>
#include <gameLibs/textureGen/graph/texgenGraph_api.h>

#include <math/dag_Point4.h>
#include <osApiWrappers/dag_direct.h>

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>

#include <stdio.h>
#include <string.h>

namespace texgen
{
namespace
{
// Read the first comma-separated type token out of a pin's `types:t` value. Returns empty
// when the field is missing/empty.
eastl::string first_type_token(const char *types_csv)
{
  if (!types_csv || !*types_csv)
  {
    return {};
  }
  const char *comma = strchr(types_csv, ',');
  return comma ? eastl::string(types_csv, size_t(comma - types_csv)) : eastl::string(types_csv);
}

void push_size_items(DataBlock &items, bool include_width)
{
  if (include_width)
  {
    items.addStr("item", SIZE_SAME_AS_WIDTH);
  }
  for (const char *p : SIZE_PREFIXES)
  {
    items.addStr("item", p);
  }
  for (const char *s : BASE_SIZES)
  {
    items.addStr("item", s);
  }
}

void push_type_items(DataBlock &items)
{
  for (const char *p : TYPE_PREFIXES)
  {
    items.addStr("item", p);
  }
  for (const char *t : BASE_TYPES)
  {
    items.addStr("item", t);
  }
}

void push_wrap_items(DataBlock &items)
{
  for (const char *p : WRAP_PREFIXES)
  {
    items.addStr("item", p);
  }
  for (const char *w : BASE_WRAPS)
  {
    items.addStr("item", w);
  }
}

struct BlkErrors final : DataBlock::IErrorReporterPipe
{
  eastl::string text;
  void reportError(const char *error_text, bool) override { text.append_sprintf(" %s", error_text); }
};

// The reporter turns a bad file into an errors line instead of a fatal or a log line.
bool load_blk(DataBlock &blk, const char *path, eastl::string &errors)
{
  BlkErrors reporter;
  DataBlock::InstallReporterRAII install(&reporter);
  const bool loaded = dblk::load(blk, path, dblk::ReadFlag::ROBUST);
  if (loaded && reporter.text.empty())
  {
    return true;
  }
  eastl::replace(reporter.text.begin(), reporter.text.end(), '\n', ' ');
  reporter.text.rtrim();
  errors.append_sprintf("%s: %s:%s\n", path, loaded ? "loads with errors" : "does not load", reporter.text.c_str());
  return loaded;
}
} // namespace

void preprocess_node_descriptor(DataBlock &node_desc)
{
  // The "<graph properties>" pseudo-node carries the graph-level texture defaults in the JS
  // editor (mainNodes.js GE_preprocessDescription inflates it with texture/heightmap/dir properties). In the
  // C++ port those defaults live on GraphData fields (graphTextureWidth etc.) and are
  // rendered by PropertiesPanel::rebuildForGraph, so injecting per-node properties here
  // would duplicate the controls.
  if (strcmp(node_desc.getStr("name", ""), "<graph properties>") == 0)
  {
    return;
  }

  // Walk output pins to derive outDimension / outParticles, as GE_preprocessDescription. We
  // match the JS behaviour of looking only at types[0] -- a pin tagged "float,texture2D"
  // counts as a float output and contributes nothing.
  int outDimension = 0;
  bool outParticles = false;
  for (uint32_t i = 0; i < node_desc.blockCount(); ++i)
  {
    const DataBlock *pinBlk = node_desc.getBlock(i);
    if (strcmp(pinBlk->getBlockName(), "pin") != 0)
    {
      continue;
    }
    if (strcmp(pinBlk->getStr("role", "in"), "out") != 0)
    {
      continue;
    }
    const eastl::string firstType = first_type_token(pinBlk->getStr("types", ""));
    const PinType pinType = parse_pin_type(firstType.c_str());
    const int pinDim = texture_out_dimension(pinType);
    if (pinType == PinType::Particles)
    {
      outParticles = true;
    }
    if (pinDim > outDimension)
    {
      outDimension = pinDim;
    }
  }

  if (outDimension == 0)
  {
    return;
  }

  // Skip injection for properties the descriptor already declares (e.g. the "raw texture"
  // node in base_nodes.blk hard-codes texture width/height/type/wrap).
  bool haveTexWidth = false;
  bool haveTexHeight = false;
  bool haveTexDepth = false;
  bool haveTexType = false;
  bool haveTexWrap = false;
  for (uint32_t i = 0; i < node_desc.blockCount(); ++i)
  {
    const DataBlock *propBlk = node_desc.getBlock(i);
    if (strcmp(propBlk->getBlockName(), "property") != 0)
    {
      continue;
    }
    const char *propName = propBlk->getStr("name", "");
    if (strcmp(propName, "texture width") == 0)
    {
      haveTexWidth = true;
    }
    else if (strcmp(propName, "texture height") == 0)
    {
      haveTexHeight = true;
    }
    else if (strcmp(propName, "texture depth") == 0)
    {
      haveTexDepth = true;
    }
    else if (strcmp(propName, "texture type") == 0)
    {
      haveTexType = true;
    }
    else if (strcmp(propName, "texture wrap") == 0)
    {
      haveTexWrap = true;
    }
  }

  // Default values mirror GE_preprocessDescription for the non-graph-properties branch:
  // size defaults to SIZE_PREFIXES[0] ("parent size"), height/depth to "width", type to
  // TYPE_PREFIXES[0] ("parent type"), wrap to WRAP_PREFIXES[0] ("parent wrap"). Particles
  // outputs override width to "= 512" and height to "= 1" so they map naturally to a
  // 1D particle buffer rather than inheriting upstream texture dimensions.
  const char *defaultWidth = outParticles ? "= 512" : SIZE_PREFIXES[0];
  const char *defaultHeight = outParticles ? "= 1" : SIZE_SAME_AS_WIDTH;
  const char *defaultDepth = SIZE_SAME_AS_WIDTH;
  const char *defaultType = TYPE_PREFIXES[0];
  const char *defaultWrap = WRAP_PREFIXES[0];

  auto appendCombobox = [&](const char *name, const char *val) -> DataBlock * {
    DataBlock *p = node_desc.addNewBlock("property");
    p->setStr("name", name);
    p->setStr("type", "combobox");
    p->setStr("val", val);
    return p->addNewBlock("items");
  };

  if (!haveTexWidth)
  {
    push_size_items(*appendCombobox("texture width", defaultWidth), /*include_width=*/false);
  }
  if (outDimension >= 2 && !haveTexHeight)
  {
    push_size_items(*appendCombobox("texture height", defaultHeight), /*include_width=*/true);
  }
  if (outDimension >= 3 && !haveTexDepth)
  {
    push_size_items(*appendCombobox("texture depth", defaultDepth), /*include_width=*/true);
  }
  if (!haveTexType)
  {
    push_type_items(*appendCombobox("texture type", defaultType));
  }
  if (!haveTexWrap)
  {
    push_wrap_items(*appendCombobox("texture wrap", defaultWrap));
  }
}

eastl::string param_as_string(const DataBlock *blk, const char *name)
{
  if (!blk || !name)
  {
    return {};
  }
  const int idx = blk->findParam(name);
  if (idx < 0)
  {
    return {};
  }
  char buf[128] = {};
  switch (blk->getParamType(idx))
  {
    case DataBlock::TYPE_STRING: return eastl::string(blk->getStr(idx));
    case DataBlock::TYPE_INT: snprintf(buf, sizeof(buf), "%d", blk->getInt(idx)); return buf;
    case DataBlock::TYPE_REAL: snprintf(buf, sizeof(buf), "%g", blk->getReal(idx)); return buf;
    case DataBlock::TYPE_BOOL: return blk->getBool(idx) ? eastl::string("true") : eastl::string("false");
    case DataBlock::TYPE_POINT4:
    {
      const Point4 p = blk->getPoint4(idx);
      snprintf(buf, sizeof(buf), "%g,%g,%g,%g", p.x, p.y, p.z, p.w);
      return buf;
    }
    default: return {};
  }
}

bool NodeRegistry::loadBaseNodes(const char *path, eastl::string &errors)
{
  DataBlock file;
  if (!load_blk(file, path, errors))
  {
    return false;
  }
  for (uint32_t i = 0; i < file.blockCount(); ++i)
  {
    const DataBlock &node = *file.getBlock(i);
    if (strcmp(node.getBlockName(), "node") != 0)
    {
      continue;
    }
    const char *uid = node.getStr("templateUid", "");
    if (!uid[0])
    {
      errors.append_sprintf("%s: node '%s' has no templateUid\n", path, node.getStr("name", ""));
    }
    if (addDescriptor(node, DescriptorSource::Base))
    {
      errors.append_sprintf("%s: templateUid '%s' replaces an earlier descriptor\n", path, uid);
    }
  }
  return true;
}

bool NodeRegistry::addDescriptor(const DataBlock &node, DescriptorSource source)
{
  const char *uid = node.getStr("templateUid", "");
  const auto found = uid[0] ? indexByUid.find_as(uid) : indexByUid.end();
  const bool replaced = found != indexByUid.end();
  DataBlock *target = nullptr;
  if (replaced)
  {
    target = blk.getBlock(found->second);
    target->setFrom(&node);
    sources[found->second] = source;
  }
  else
  {
    if (uid[0])
    {
      indexByUid.emplace(eastl::string(uid), blk.blockCount());
    }
    target = blk.addNewBlock(&node, "node");
    sources.push_back(source);
  }
  preprocess_node_descriptor(*target);
  return replaced;
}

bool NodeRegistry::loadPacks(const char *root_dir, eastl::string &errors)
{
  if (!dd_dir_exists(root_dir))
  {
    errors.append_sprintf("%s: not a directory\n", root_dir);
    return false;
  }
  eastl::string root(root_dir);
  if (root.back() != '/' && root.back() != '\\')
  {
    root.push_back('/');
  }
  const eastl::string mask = root + "*";
  eastl::vector<eastl::string> folders;
  for (const alefind_t &fs : dd_find_iterator(mask.c_str(), DA_SUBDIR))
  {
    if ((fs.attr & DA_SUBDIR) && fs.name[0] != '.')
    {
      folders.emplace_back(fs.name);
    }
  }
  // The scan order differs between file systems; the palette and the das sheet must not.
  eastl::sort(folders.begin(), folders.end());

  bool allLoaded = true;
  for (const eastl::string &folder : folders)
  {
    const eastl::string path = root + folder + "/node.blk";
    DataBlock file;
    if (!load_blk(file, path.c_str(), errors))
    {
      allLoaded = false;
      continue;
    }
    DataBlock *node = file.blockCount() == 1 ? file.getBlockByName("node") : nullptr;
    if (!node || !node->getStr("templateUid", "")[0])
    {
      errors.append_sprintf("%s: holds no single node{} with a templateUid\n", path.c_str());
      allLoaded = false;
      continue;
    }
    if (!node->paramExists("nativeShader"))
    {
      node->setStr("nativeShader", ("pack_" + folder).c_str());
    }
    if (addDescriptor(*node, DescriptorSource::Pack))
    {
      errors.append_sprintf("%s: templateUid '%s' replaces an earlier descriptor\n", path.c_str(), node->getStr("templateUid"));
    }
  }
  return allLoaded;
}

void NodeRegistry::clear()
{
  blk.reset();
  sources.clear();
  indexByUid.clear();
}

const DataBlock *NodeRegistry::findByUid(const char *uid) const
{
  const auto found = indexByUid.find_as(uid);
  return found != indexByUid.end() ? blk.getBlock(found->second) : nullptr;
}

const DataBlock *NodeRegistry::findByName(const char *name) const
{
  for (uint32_t i = 0; i < blk.blockCount(); ++i)
  {
    if (strcmp(blk.getBlock(i)->getStr("name", ""), name) == 0)
    {
      return blk.getBlock(i);
    }
  }
  return nullptr;
}
} // namespace texgen
