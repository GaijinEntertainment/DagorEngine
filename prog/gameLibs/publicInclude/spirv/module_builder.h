//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_span.h>
#include <EASTL/bitset.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <spirv/compiler.h>
#include <drv/shadersMetaData/spirv/compiled_meta_data.h>
#include "traits_table.h"

namespace spirv
{
using namespace spv;

class ErrorHandler
{
public:
  ErrorHandler() = default;
  virtual ~ErrorHandler() = default;

  enum class Action
  {
    STOP,
    CONTINUE
  };

  // called if an error is encountered with no possibility to recover from
  virtual void onFatalError(const char *msg) = 0;
  // called if an error is encountered but it is possible to go on
  virtual Action onError(const char *msg) = 0;
  // called if an incorrect behavior is detected that is not considered an error but bad practice
  virtual Action onWarning(const char *msg) = 0;
  // called if something noteworthy happened
  virtual void onMessage(const char *msg) = 0;

  virtual void onFatalError(const eastl::string &msg) = 0;
  virtual Action onError(const eastl::string &msg) = 0;
  virtual Action onWarning(const eastl::string &msg) = 0;
  virtual void onMessage(const eastl::string &msg) = 0;
};

struct Node;
struct NodeId;
struct NodeFunction;
struct NodeVariable;
struct NodeOpVariable;
struct NodeTypedef;
struct NodeOpUndef;
struct NodeConstant;
struct NodeSpecConstant;

struct ExecutionModeBase;

// A module builder instance holds one complete spir-v module
// in its in memory node representation with all its metadata
class ModuleBuilder
{
  struct NodeRefInfo
  {
    NodePointer<NodeId> node;
    eastl::vector<NodePointer<NodeId>> refs;
  };
  struct EntryPoint
  {
    ExecutionModel execModel;
    NodePointer<NodeFunction> function;
    eastl::string name;
    eastl::vector<NodePointer<NodeVariable>> interface;
    eastl::vector<CastableUniquePointer<ExecutionModeBase>> executionModes;
  };
  eastl::bitset<static_cast<unsigned>(Extension::Count)> enabledExtensions = {};
  Id extendedGrammars[static_cast<unsigned>(ExtendedGrammar::Count)] = {};

  Id lastAllocatedId = 0;
  eastl::vector<Id> freeIdSlots;
  eastl::vector<Capability> enabledCaps;
  eastl::vector<eastl::string> moduleProcesses;

  eastl::vector<EntryPoint> entryPoints;

  eastl::vector<CastableUniquePointer<Node>> allNodes;
  eastl::vector<NodePointer<NodeVariable>> globalVariables;
  // undef is special, its a operation that can appear in type/var def block
  // we put them after types and before first global variable
  eastl::vector<NodePointer<NodeOpUndef>> globalUndefs;

  // eastl::vector<NodeRefInfo> nodeRefs;

  template <typename T>
  void registerNodeRef(NodeRefInfo &target, T)
  {}
  template <typename T>
  void registerNodeRef(NodeRefInfo &target, NodePointer<T> ref)
  {
    if (ref && is<NodeId>(ref))
      target.refs.emplace_back(ref);
  }
  template <typename F, typename S, typename... R>
  void registerNodeRefRec(NodeRefInfo &target, F f, S s, R... r)
  {
    registerNodeRef(target, f);
    registerNodeRefRec(target, s, r...);
  }
  template <typename F>
  void registerNodeRefRec(NodeRefInfo &target, F f)
  {
    registerNodeRef(target, f);
  }
  void registerNodeRefRec(NodeRefInfo &target);

  unsigned version = 0x00010000;
  unsigned toolIdent = 0;

  AddressingModel addressingModel = AddressingModel::Logical;
  MemoryModel memoryModel = MemoryModel::GLSL450;

  SourceLanguage sourceLanguage = SourceLanguage::Unknown;
  unsigned sourceLanguageVersion = 0;
  eastl::string sourceString;
  eastl::vector<eastl::string> sourceCodeExtension;

public:
  ModuleBuilder() = default;
  ~ModuleBuilder() = default;

  ModuleBuilder(const ModuleBuilder &) = default;
  ModuleBuilder &operator=(const ModuleBuilder &) = default;

  ModuleBuilder(ModuleBuilder &&) = default;
  ModuleBuilder &operator=(ModuleBuilder &&) = default;

  // return loaded module version?
  unsigned getModuleVersion() const;
  void setModuleVersion(unsigned v);

  unsigned getToolIdentifcation() const;
  void setToolIdentification(unsigned ident);

  Id getIdBounds() const;
  void setIdBounds(Id ident);

  AddressingModel getAddressingModel() const;
  void setAddressingModel(AddressingModel am);

  MemoryModel getMemoryModel() const;
  void setMemoryModel(MemoryModel mm);

  const eastl::vector<Capability> &getEnabledCapabilities() const;
  const eastl::bitset<static_cast<unsigned>(Extension::Count)> &getEnabledExtensions() const;

  template <typename T>
  void enumerateAllNodes(T u)
  {
    for (auto &&n : allNodes)
    {
      u(NodePointer<Node>{n.get()});
    }
  }

  Id allocateId();
  void freeId(Id ident);
  void addModuleProcess(const char *name, Id len);
  template <typename T>
  void enumerateAllTypes(T u)
  {
    for (auto &&n : allNodes)
    {
      NodePointer<NodeTypedef> type{n.get()};
      if (type)
        u(type);
    }
  }

  template <typename T>
  void enumerateAllConstants(T u)
  {
    for (auto &&n : allNodes)
    {
      NodePointer<NodeConstant> cnst{n.get()};
      if (cnst)
      {
        u(cnst);
        continue;
      }

      NodePointer<NodeSpecConstant> sconst{n.get()};
      if (sconst)
        u(sconst);
    }
  }

  template <typename T>
  void enumerateModuleProcessed(T u)
  {
    for (auto &&p : moduleProcesses)
      u(p.c_str(), p.length());
  }

  void enableCapability(Capability cap);
  // on simplification step some caps can be turned off
  // as they are implicitly enabled with caps that need
  // their functionality
  void disableCapability(Capability cap);

  template <typename T>
  void enumerateEnabledCapabilities(T t)
  {
    for (auto &&c : enabledCaps)
      t(c);
  }

  void enableExtension(Extension ext, const char *, size_t);
  bool isExtensionEnabled(Extension ext) const;
  void disableExtension(Extension ext);
  template <typename T>
  void enumerateEnabledExtensions(T t)
  {
    for (unsigned ext = 0; ext < static_cast<unsigned>(Extension::Count); ++ext)
    {
      if (enabledExtensions.test(ext))
      {
        auto name = extension_id_to_name(static_cast<Extension>(ext));
        t(static_cast<Extension>(ext), name.data(), name.size());
      }
    }
  }
  void loadExtendedGrammar(IdResult ident, ExtendedGrammar egram, const char *, size_t);
  // If the extended grammar is already loaded, then it returns its id, if not then it allocates a
  // id for it and returns the new id
  Id lazyLoadExtendedGrammar(ExtendedGrammar egram, const char *, size_t);
  // returns 0 if grammar is not loaded yet
  Id getExtendedGrammarIdRef(ExtendedGrammar egram) const;
  ExtendedGrammar getExtendedGrammarFromIdRef(Id ident) const;

  template <typename T>
  void enumerateLoadedExtendedGrammars(T t)
  {
    for (unsigned eg = 0; eg < static_cast<unsigned>(ExtendedGrammar::Count); ++eg)
    {
      if (extendedGrammars[eg] != 0)
      {
        auto name = extended_grammar_id_to_name(static_cast<ExtendedGrammar>(eg));
        t(extendedGrammars[eg], static_cast<ExtendedGrammar>(eg), name.data(), name.size());
      }
    }
  }

  void addEntryPoint(ExecutionModel em, IdRef func, LiteralString name, Multiple<IdRef> interface);
  void addEntryPoint(ExecutionModel em, NodePointer<NodeFunction> func, eastl::string name,
    eastl::vector<NodePointer<NodeVariable>> interface);

  template <typename T, typename... Args>
  void addExecutionMode(NodePointer<NodeFunction> target, Args &&...args)
  {
    for (auto &&ep : entryPoints)
      if (ep.function == target)
        ep.executionModes.emplace_back(new T(eastl::forward<Args>(args)...));
  }

  void addExecutionMode(IdRef target, CastableUniquePointer<ExecutionModeBase> mode);

  template <typename T>
  void enumerateEntryPoints(T t)
  {
    for (auto &&ep : entryPoints)
      t(ep.function, ep.execModel, ep.name, ep.interface);
  }

  template <typename T>
  void enumerateExecutionModes(T t)
  {
    for (auto &&ep : entryPoints)
      for (auto &&em : ep.executionModes)
        t(ep.function, em.get());
  }

  template <typename P, typename T>
  void enumerateAllInstructionsWithProperty(T t)
  {
    for (auto &&node : allNodes)
    {
      NodePointer<NodeId> idNode{node.get()};
      if (!idNode)
        continue;

      for (auto &&prop : idNode->properties)
        if (is<P>(prop))
          t(idNode, as<P>(prop));
    }
  }

  template <typename T>
  void enumerateAllInstructionsWithAnyProperty(T t)
  {
    for (auto &&node : allNodes)
    {
      NodePointer<NodeId> idNode{node.get()};
      if (!idNode)
        continue;
      if (idNode->properties.empty())
        continue;

      t(idNode);
    }
  }

  NodePointer<NodeId> autoAssignId(NodePointer<NodeId> id_node);
  void registerIdNode(NodePointer<NodeId> id_node);
  void registerTypeNode(NodePointer<NodeTypedef> type_node);

  template <typename... Args>
  void registerNodeRefs(NodePointer<NodeId> node, Args &&...args)
  {
    if (!node)
      return;

    // NodeRefInfo info;
    // info.node = node;
    // registerNodeRefRec(info, args...);
    // nodeRefs.push_back(info);
  }

  template <typename T, typename... Args>
  auto newNode(Args &&...args)
  {
    auto newNode = NodePointer<T>(new T(args...));
    allNodes.emplace_back(newNode.get());
    // reuse newly casted result to avoid repeated cast calls
    auto newIdNode = autoAssignId(newNode);
    if (newIdNode)
    {
      registerIdNode(newIdNode);
      registerTypeNode(newIdNode);
    }
    registerNodeRefs(newNode, args...);
    return newNode;
  }

  NodePointer<NodeTypedef> getType(IdResultType id);
  NodePointer<NodeId> getNode(IdResult id);
  NodePointer<NodeId> getNode(IdRef id);
  NodePointer<NodeId> getNode(IdScope id);
  NodePointer<NodeId> getNode(IdMemorySemantics id);
  NodePointer<NodeId> getNode(Id id);

  template <typename T>
  void enumerateAllFunctions(T u)
  {
    for (auto &&node : allNodes)
    {
      NodePointer<NodeFunction> funcNode{node.get()};
      if (!funcNode)
        continue;
      u(funcNode);
    }
  }

  void addGlobalVariable(NodePointer<NodeVariable> var);

  template <typename T>
  void enumerateAllGlobalVariables(T u)
  {
    for (auto &&node : globalVariables)
      u(node);
  }

  void addGlobalUndef(NodePointer<NodeOpUndef> udf);

  template <typename T>
  void enumerateAllGlobalUndefs(T u)
  {
    for (auto &&node : globalUndefs)
      u(node);
  }

  // TODO: this can be auto generated
  // TODO: incomplete
  unsigned getTypeSizeBits(NodePointer<NodeTypedef> type);
  void continueSourceString(const eastl::string &value);
  void setSourceLanguage(SourceLanguage sl);
  SourceLanguage getSourceLanguage() const;
  void setSourceLanguageVersion(unsigned ver);
  unsigned getSourceLanguageVersion() const;
  void setSourceString(eastl::string str);
  const eastl::string &getSourceString() const;
  void setSourceFile(IdRef name_ref);
  IdRef getSourceFile() const;
  void addSourceCodeExtension(eastl::string str);

  template <typename U>
  void enumerateSourceExtensions(U v)
  {
    for (auto &&ext : sourceCodeExtension)
      v(ext);
  }

  template <typename T, typename U>
  void enumerateNodeType(U v)
  {
    for (auto &&node : allNodes)
      if (is<T>(node))
        v(as<T>(node));
  }

  template <typename T, typename U>
  void enumerateConsumersOf(NodePointer<T> node, U visitor)
  {
    for (auto &&vn : allNodes)
      visitNode(vn.get(), [&](auto n) //
        {
          n->visitRefs([&](auto &r) //
            {
              if (as<Node>(r) == as<Node>(node))
                visitor(make_node_pointer(n), r);
            });
          return true;
        });
  }

  template <typename T>
  void removeNode(NodePointer<T> node)
  {
    enumerateConsumersOf(node, [=](auto block_node, auto &) //
      {
        if (is<NodeOpLabel>(block_node))
        {
          auto block = as<NodeOpLabel>(block_node);
          block->instructions.erase(eastl::find_if(begin(block->instructions), end(block->instructions),
            [=](auto br) //
            { return as<Node>(br).get() == as<Node>(node).get(); }));
        }
      });
    if (is<NodeVariable>(node))
    {
      globalVariables.erase(eastl::find_if(begin(globalVariables), end(globalVariables),
        [=](auto &&br) //
        { return as<Node>(br).get() == as<Node>(node).get(); }));
    }
    allNodes.erase(eastl::find_if(begin(allNodes), end(allNodes),
      [=](auto &br) //
      { return as<Node>(br) == as<Node>(node).get(); }));
  }

  // updates a node ref
  // t_node - node that is modified
  // t_ref - reference of the node pointer of t_node that is supposed to be updated
  // to_ref - new value for t_ref
  template <typename T, typename U, typename V>
  void updateRef(NodePointer<T> t_node, NodePointer<U> &t_ref, NodePointer<V> to_ref)
  {
    /*
    auto refInfoRef = eastl::find_if(begin(nodeRefs), end(nodeRefs), [t_node](auto &info) { return
    info.node == t_node; }); if (refInfoRef != end(nodeRefs))
    {
      auto ref = eastl::find(begin(refInfoRef->refs), end(refInfoRef->refs), t_ref);
      if (ref != end(refInfoRef->refs))
      {
        *ref = to_ref;
      }
      else
      {
        refInfoRef->refs.push_back(to_ref);
      }
    }*/
    t_ref = to_ref;
  }
};

struct WordWriter
{
  static constexpr unsigned max_string_length = 0xFFFF;
  eastl::vector<unsigned> words;
  size_t wordsToWrite = 0;
  size_t wordsWritten = 0;
  unsigned stringWritePos = 0;
  bool hadError = false;
  ErrorHandler *errorHandler;
  void beginHeader()
  {
    wordsToWrite = 5;
    if (hadError)
      return;
    wordsWritten = 0;
  }
  void endWrite()
  {
    if (wordsToWrite != wordsWritten)
    {
      if (errorHandler)
      {
        errorHandler->onFatalError("WordWriter: expected to write " + eastl::to_string(wordsToWrite) + " words, but only " +
                                   eastl::to_string(wordsWritten) + " where actually written!");
      }
      hadError = true;
      words.clear();
    }
    wordsToWrite = 0;
    wordsWritten = 0;
  }
  void beginInstruction(Op opcode, size_t extra_words)
  {
    if (hadError)
      return;
    wordsToWrite = extra_words + 1;
    wordsWritten = 0;
    words.reserve(words.size() + wordsToWrite);
    writeWord(static_cast<Id>(opcode) | static_cast<Id>(wordsToWrite << WordCountShift));
  }
  void writeWord(unsigned w)
  {
    if (hadError)
      return;
    words.push_back(w);
    ++wordsWritten;
  }
  void writeWords(const unsigned *w, size_t count)
  {
    if (hadError)
      return;
    words.insert(end(words), w, w + count);
    wordsWritten += count;
  }
  static size_t calculateStringWords(size_t chars)
  {
    // requires extra null so sub length by one
    if (chars > max_string_length)
      return (max_string_length + sizeof(unsigned) - 1) / sizeof(unsigned);
    // include null
    return (chars + sizeof(unsigned)) / sizeof(unsigned);
  }
  const char *writeChars(const char *str)
  {
    union
    {
      unsigned word;
      char chars[4];
    };

    word = 0;

    unsigned i = 0;
    for (; i < 4 && str[i] && stringWritePos < max_string_length; ++i, ++stringWritePos)
      chars[i] = str[i];

    writeWord(word);
    if (i < 4 && stringWritePos < max_string_length)
      return nullptr;
    return str + i;
  }
  const char *writeString(const char *str)
  {
    stringWritePos = 0;
    while (str && stringWritePos < max_string_length)
      str = writeChars(str);
    return str;
  }

  auto beginSpecConstantOpMode()
  {
    // return the pos where the next op will be placed so that we know where to edit stuff
    return words.size();
  }

  void endSpecConstantOpMode(size_t inst_index)
  {
    // current length of the spec op without any extra (usually 3)
    auto instLen = words[inst_index] >> WordCountShift;
    auto specOpCodeIndex = inst_index + instLen;
    // length of the op of the spec op including opcode
    auto specOpLength = words[specOpCodeIndex] >> WordCountShift;
    // only adds the spec op opcode and shuffles the result type and id to the front
    auto fullLen = 1 + specOpLength;
    // remove stuff what spec op already has written (type and id)
    auto toRemove = (instLen + specOpLength) - fullLen;
    // first mask of len, only plain opcode allowed
    words[specOpCodeIndex] &= 0xFFFF;
    // replace opcode length of spec op
    words[inst_index] &= 0xFFFF;
    words[inst_index] |= fullLen << WordCountShift;
    // remove return type and id from the operation, they are already defined by the spec op
    words.erase(words.begin() + specOpCodeIndex + 1, words.begin() + specOpCodeIndex + 1 + toRemove);
  }
};

struct WriteConfig
{
  bool writeNames;
  bool writeSourceExtensions;
  bool writeLines;
  bool writeDebugGrammar;
};

struct ReaderResult
{
  eastl::vector<eastl::string> errorLog;
  eastl::vector<eastl::string> warningLog;

  bool hadError() const { return !errorLog.empty(); }
};

eastl::vector<unsigned> write(ModuleBuilder &builder, const WriteConfig &cfg, ErrorHandler &e_handler);
ReaderResult read(ModuleBuilder &builder, const unsigned *words, unsigned word_count);

// Redistributes ids for each instruction that needs a id
void re_index(ModuleBuilder &builder);

// utility stuff
enum class BufferKind
{
  Uniform,
  ReadOnlyStorage,
  Storage,
  Err
};

// This tries to figure out which kind of uniform buffer variable a variable is.
BufferKind get_buffer_kind(NodePointer<NodeOpVariable> var);

// finds first property with type T in the property list of the given node.
// This is sufficient in most cases, but if you have a node that can have
// properties that can have a member index, better use find_base_property
// or find_member_property.
template <typename T, typename U>
inline T *find_property(NodePointer<U> node)
{
  auto ref = eastl::find_if(begin(node->properties), end(node->properties),
    [](auto &prop) //
    { return is<T>(prop); });
  if (ref != end(node->properties))
    return as<T>(*ref);
  return nullptr;
}

// finds first property with type T that has no member index.
template <typename T, typename U>
inline T *find_base_property(NodePointer<U> node)
{
  auto ref = eastl::find_if(begin(node->properties), end(node->properties),
    [](auto &prop) //
    { return is<T>(prop) && !as<T>(prop)->memberIndex; });
  if (ref != end(node->properties))
    return as<T>(*ref);
  return nullptr;
}

// finds first property with type T that has a member index that matches the given member index.
template <typename T, typename U>
inline T *find_member_property(NodePointer<U> node, size_t member_index)
{
  auto ref = eastl::find_if(begin(node->properties), end(node->properties),
    [member_index](auto &prop) //
    { return is<T>(prop) && (as<T>(prop)->memberIndex && *as<T>(prop)->memberIndex == member_index); });
  if (ref != end(node->properties))
    return as<T>(*ref);
  return nullptr;
}

struct SemanticInfo
{
  const char *name;
  unsigned location;
};
// This pass translates HLSL semantic names into Vulkan layout location indices.
// The user has to provide two tables, one for input and one for output mapping.
// If a semantic table is empty then only the semantic property of interface
// variables are dropped.
void resolveSemantics(ModuleBuilder &builder, const SemanticInfo *input_semantics, unsigned input_smentics_count,
  const SemanticInfo *output_smenaitcs, unsigned output_smenaitcs_count, ErrorHandler &e_handler);

enum class AtomicResolveMode
{
  // Basically keeps everything as is (one buf for data one buf for counter), just makes sure
  // that the counter and the data buffer end up being at the same slot
  SeparateBuffer,
  // outdated need too much patching
  Embed,
  // PushConstantIndex,
  // DedicatedUniformBufferIndex
};
// this pass finds atomic buffers and complains if they are used
void resolveAtomicBuffers(ModuleBuilder &builder, AtomicResolveMode mode, ErrorHandler &e_handler);

// Generates the ShaderHeader out of the given SPIR-V module, it also compact the interface uniform
// variable binding indices into a set without empty slots.
ShaderHeader compileHeader(ModuleBuilder &builder, CompileFlags flags, ErrorHandler &e_handler);

ComputeShaderInfo resolveComputeShaderInfo(ModuleBuilder &builder);

struct EntryPointRename
{
  const char *from;
  const char *to;
};
// This pass renames all entry points of the module either by the entry of the renaming list
// or to the default name.
void renameEntryPoints(ModuleBuilder &builder, const EntryPointRename *list = nullptr, unsigned list_len = 0,
  const char *default_name = "main");

// This pass duplicates buffer's pointer types. Old Adreno drivers do not work correctly if types are reused.
void make_buffer_pointer_type_unique_per_buffer(ModuleBuilder &builder);

// Pass that fixes currently blocking bugs of DXC
// Currently this ensures that the implicitly create global const
// buffer for register(c#) variables is assigned to b0. There
// where some cases where it was not.
void fixDXCBugs(ModuleBuilder &builder, ErrorHandler &e_handler);

enum class StructureValidationRules
{
  DEFAULT,
  RELAXED,
  SCALAR,
  VULKAN_1_0 = DEFAULT,
  VULKAN_1_1 = RELAXED
};
// DXC produces structure layouts for uniforms in the same way as for DirectX,
// this does not always matches those rules for Vulkan.
// This pass checks if the layout really matches the Vulkan layout for the selected
// rules.
bool validateStructureLayouts(ModuleBuilder &builder, StructureValidationRules rules, ErrorHandler &e_handler);

// DXC produces reflection, that will crash on some devices due to lack of extensions support
// clean it up by hand
void cleanupReflectionLeftouts(ModuleBuilder &builder, ErrorHandler &e_handler);

} // namespace spirv

// THINGS TODO:
// - node has a link to its block
// - blocks have a link to their function
// - have a global block for everything global (has no function link)
