#include <spirv/module_builder.h>
#include "module_nodes.h"

using namespace spirv;

void ModuleBuilder::registerNodeRefRec(NodeRefInfo &target) {}

// return loaded module version?
unsigned ModuleBuilder::getModuleVersion() const { return version; }

void ModuleBuilder::setModuleVersion(unsigned v) { version = v; }

unsigned ModuleBuilder::getToolIdentifcation() const { return toolIdent; }

void ModuleBuilder::setToolIdentification(unsigned ident) { toolIdent = ident; }

Id ModuleBuilder::getIdBounds() const { return lastAllocatedId + 1; }

void ModuleBuilder::setIdBounds(Id ident)
{
  lastAllocatedId = ident - 1;
  freeIdSlots.clear();
}

AddressingModel ModuleBuilder::getAddressingModel() const { return addressingModel; }

void ModuleBuilder::setAddressingModel(AddressingModel am) { addressingModel = am; }

MemoryModel ModuleBuilder::getMemoryModel() const { return MemoryModel::GLSL450; }

void ModuleBuilder::setMemoryModel(MemoryModel mm) { memoryModel = mm; }

const eastl::vector<Capability> &ModuleBuilder::getEnabledCapabilities() const { return enabledCaps; }
const eastl::bitset<static_cast<unsigned>(Extension::Count)> &ModuleBuilder::getEnabledExtensions() const { return enabledExtensions; }

Id ModuleBuilder::allocateId()
{
  if (!freeIdSlots.empty())
  {
    auto result = freeIdSlots.back();
    freeIdSlots.pop_back();
    return result;
  }
  return ++lastAllocatedId;
}

void ModuleBuilder::freeId(Id ident) { freeIdSlots.push_back(ident); }

void ModuleBuilder::addModuleProcess(const char *name, Id len) { moduleProcesses.emplace_back(name, len); }

void ModuleBuilder::enableCapability(Capability cap)
{
  using namespace eastl;
  if (end(enabledCaps) == find(begin(enabledCaps), end(enabledCaps), cap))
    enabledCaps.push_back(cap);
}

// on simplification step some caps can be turned off
// as they are implicitly enabled with caps that need
// their functionality
void ModuleBuilder::disableCapability(Capability cap) { enabledCaps.erase(eastl::find(begin(enabledCaps), end(enabledCaps), cap)); }

void ModuleBuilder::enableExtension(Extension ext, const char *, size_t) { enabledExtensions.set(static_cast<unsigned>(ext)); }

bool ModuleBuilder::isExtensionEnabled(Extension ext) const { return enabledExtensions.test(static_cast<unsigned>(ext)); }

void ModuleBuilder::disableExtension(Extension ext) { enabledExtensions.reset(static_cast<unsigned>(ext)); }

void ModuleBuilder::loadExtendedGrammar(IdResult ident, ExtendedGrammar egram, const char *, size_t)
{
  extendedGrammars[static_cast<unsigned>(egram)] = ident.value;
}
// If the extended grammar is already loaded, then it returns its id, if not then it allocates a id
// for it and returns the new id
Id ModuleBuilder::lazyLoadExtendedGrammar(ExtendedGrammar egram, const char *, size_t)
{
  auto &slot = extendedGrammars[static_cast<unsigned>(egram)];
  if (0 == slot)
    slot = allocateId();
  return slot;
}
// returns 0 if grammar is not loaded yet
Id ModuleBuilder::getExtendedGrammarIdRef(ExtendedGrammar egram) const { return extendedGrammars[static_cast<unsigned>(egram)]; }
ExtendedGrammar ModuleBuilder::getExtendedGrammarFromIdRef(Id ident) const
{
  using namespace eastl;
  return static_cast<ExtendedGrammar>(find(begin(extendedGrammars), end(extendedGrammars), ident) - begin(extendedGrammars));
}

void ModuleBuilder::addEntryPoint(ExecutionModel em, IdRef func, LiteralString name, Multiple<IdRef> interface)
{
  EntryPoint ep;
  ep.execModel = em;
  ep.function = getNode(func);
  ep.name.assign(name.asString(), name.length);
  ep.interface.reserve(interface.count);
  while (!interface.empty())
    ep.interface.push_back(getNode(interface.consume()));
  entryPoints.push_back(eastl::move(ep));
}

void ModuleBuilder::addEntryPoint(ExecutionModel em, NodePointer<NodeFunction> func, eastl::string name,
  eastl::vector<NodePointer<NodeVariable>> interface)
{
  EntryPoint ep;
  ep.execModel = em;
  ep.function = func;
  ep.name = eastl::move(name);
  ep.interface = eastl::move(interface);
  entryPoints.push_back(eastl::move(ep));
}

void ModuleBuilder::addExecutionMode(IdRef target, CastableUniquePointer<ExecutionModeBase> mode)
{
  for (auto &&ep : entryPoints)
    if (ep.function->resultId == target)
      ep.executionModes.push_back(eastl::move(mode));
}

NodePointer<NodeId> ModuleBuilder::autoAssignId(NodePointer<NodeId> id_node)
{
  if (id_node && !id_node->resultId)
    id_node->resultId = allocateId();

  return id_node;
}

void ModuleBuilder::registerIdNode(NodePointer<NodeId> id_node)
{
  if (!id_node)
    return;
}

void ModuleBuilder::registerTypeNode(NodePointer<NodeTypedef> type_node)
{
  if (!type_node)
    return;
}

NodePointer<NodeTypedef> ModuleBuilder::getType(IdResultType id)
{
  for (auto &&node : allNodes)
  {
    NodePointer<NodeTypedef> type{node.get()};
    if (!type)
      continue;
    if (type->resultId == id.value)
      return type;
  }
  return {};
}

NodePointer<NodeId> ModuleBuilder::getNode(IdResult id)
{
  for (auto &&node : allNodes)
  {
    NodePointer<NodeId> idNode{node.get()};
    if (!idNode)
      continue;
    if (idNode->resultId == id)
      return idNode;
  }
  return {};
}

NodePointer<NodeId> ModuleBuilder::getNode(IdRef id)
{
  for (auto &&node : allNodes)
  {
    NodePointer<NodeId> idNode{node.get()};
    if (!idNode)
      continue;
    if (idNode->resultId == id)
      return idNode;
  }
  return {};
}

NodePointer<NodeId> ModuleBuilder::getNode(IdScope id)
{
  for (auto &&node : allNodes)
  {
    NodePointer<NodeId> idNode{node.get()};
    if (!idNode)
      continue;
    if (idNode->resultId == id)
      return idNode;
  }
  return {};
}

NodePointer<NodeId> ModuleBuilder::getNode(IdMemorySemantics id)
{
  for (auto &&node : allNodes)
  {
    NodePointer<NodeId> idNode{node.get()};
    if (!idNode)
      continue;
    if (idNode->resultId == id)
      return idNode;
  }
  return {};
}

NodePointer<NodeId> ModuleBuilder::getNode(Id id)
{
  for (auto &&node : allNodes)
  {
    NodePointer<NodeId> idNode{node.get()};
    if (!idNode)
      continue;
    if (idNode->resultId == id)
      return idNode;
  }
  return {};
}

void ModuleBuilder::addGlobalVariable(NodePointer<NodeVariable> var) { globalVariables.push_back(var); }

void ModuleBuilder::addGlobalUndef(NodePointer<NodeOpUndef> udf) { globalUndefs.push_back(udf); }

// TODO: this can be auto generated
// TODO: incomplete
unsigned ModuleBuilder::getTypeSizeBits(NodePointer<NodeTypedef> type)
{
  if (type)
  {
    switch (type->opCode)
    {
      case Op::OpTypeInt: return as<NodeOpTypeInt>(type)->width.value;
      case Op::OpTypeFloat: return as<NodeOpTypeFloat>(type)->width.value; break;
    }
  }

  return 0;
}

void ModuleBuilder::continueSourceString(const eastl::string &value) { sourceString += value; }

void ModuleBuilder::setSourceLanguage(SourceLanguage sl) { sourceLanguage = sl; }

SourceLanguage ModuleBuilder::getSourceLanguage() const { return sourceLanguage; }

void ModuleBuilder::setSourceLanguageVersion(unsigned ver) { sourceLanguageVersion = ver; }

unsigned ModuleBuilder::getSourceLanguageVersion() const { return sourceLanguageVersion; }

void ModuleBuilder::setSourceString(eastl::string str) { sourceString = eastl::move(str); }

const eastl::string &ModuleBuilder::getSourceString() const { return sourceString; }

void ModuleBuilder::setSourceFile(IdRef name_ref)
{
  // TODO: handle this correctly
}

IdRef ModuleBuilder::getSourceFile() const { return {}; }

void ModuleBuilder::addSourceCodeExtension(eastl::string str) { sourceCodeExtension.emplace_back(eastl::move(str)); }

// utils
BufferKind spirv::get_buffer_kind(NodePointer<NodeOpVariable> var)
{
  auto ptrType = as<NodeOpTypePointer>(var->resultType);
  auto valueType = as<NodeTypedef>(ptrType->type);

  bool isUniformBlock = false;
  bool isStorageBlock = false;
  bool isReadOnly = false;
  // have to look at the properties to figure out what we got
  for (auto &&prop : valueType->properties)
  {
    if (is<PropertyBlock>(prop))
    {
      isUniformBlock = true;
      break;
    }
    else if (is<PropertyBufferBlock>(prop))
    {
      isStorageBlock = true;
    }
    else if (is<PropertyNonWritable>(prop))
    {
      isReadOnly = true;
    }
  }

  if (isUniformBlock)
    return BufferKind::Uniform;
  else if (isStorageBlock)
    return isReadOnly ? BufferKind::ReadOnlyStorage : BufferKind::Storage;
  return BufferKind::Err;
}

void spirv::re_index(ModuleBuilder &builder)
{
  Id lastId = 1;
  // first give all extensions their ids
  builder.enumerateLoadedExtendedGrammars([&](auto, auto gram_id, auto name_str, auto name_len) //
    { builder.loadExtendedGrammar(lastId++, gram_id, name_str, name_len); });
  builder.enumerateNodeType<NodeId>([&](auto node) { node->resultId = lastId++; });
  builder.setIdBounds(lastId);
}