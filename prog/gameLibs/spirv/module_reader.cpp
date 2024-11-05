// auto generated, do not modify!
#include "module_nodes.h"
#include <spirv/module_builder.h>
#include "module_decoder.h"
using namespace spirv;
struct PorpertyCloneVisitor
{
  template <typename T>
  CastableUniquePointer<Property> operator()(const T *p)
  {
    return p->clone();
  }
};
struct PorpertyCloneWithMemberIndexVisitor
{
  LiteralInteger newMemberIndex;
  template <typename T>
  CastableUniquePointer<Property> operator()(const T *p)
  {
    return p->cloneWithMemberIndexOverride(newMemberIndex);
  }
};
struct ExectionModeDef
{
  IdRef target;
  CastableUniquePointer<ExecutionModeBase> mode;
};
// entry points forward reference functions and its interface, so need to store for later resolve. Reader result types are ok, as the
// source lives longer that this struct
struct EntryPointDef
{
  ExecutionModel executionModel;
  IdRef function;
  LiteralString name;
  Multiple<IdRef> interface;
};
// selection merge property needed to be resolved later as it has forward refs
struct SelectionMergeDef
{
  NodePointer<NodeBlock> target;
  IdRef mergeBlock;
  SelectionControlMask controlMask;
};
// loop merge property needed to be resolved later as it has forward refs
struct LoopMergeDef
{
  NodePointer<NodeBlock> target;
  IdRef mergeBlock;
  IdRef continueBlock;
  LoopControlMask controlMask;
  LiteralInteger dependencyLength;
};
// generic property definitions, properties (decoartions) are all using forward refs, so need to store it for later resolve
struct PropertyTargetResolveInfo
{
  IdRef target;
  CastableUniquePointer<Property> property;
};
// some properties them self forward reference some objects, need to record them for later resolve
struct PropertyReferenceResolveInfo
{
  NodePointer<NodeId> *target;
  IdRef ref;
};
struct ReaderContext
{
  ModuleBuilder &moduleBuilder;
  eastl::vector<EntryPointDef> &entryPoints;
  NodePointer<NodeOpFunction> &currentFunction;
  eastl::vector<IdRef> &forwardProperties;
  eastl::vector<SelectionMergeDef> &selectionMerges;
  eastl::vector<LoopMergeDef> &loopMerges;
  NodePointer<NodeBlock> &currentBlock;
  eastl::vector<PropertyTargetResolveInfo> &propertyTargetResolves;
  eastl::vector<PropertyReferenceResolveInfo> &propertyReferenceResolves;
  eastl::vector<ExectionModeDef> &executionModes;
  NodeBlock &specConstGrabBlock;
  unsigned &sourcePosition;
  template <typename T>
  void finalize(T on_error)
  {
    for (auto &&entry : entryPoints)
    {
      moduleBuilder.addEntryPoint(entry.executionModel, entry.function, entry.name, entry.interface);
    }
    for (auto &&prop : forwardProperties)
    {
      moduleBuilder.getNode(prop)->properties.emplace_back(new PropertyForwardDeclaration);
    }
    for (auto &&smerge : selectionMerges)
    {
      auto prop = new PropertySelectionMerge;
      prop->mergeBlock = moduleBuilder.getNode(smerge.mergeBlock);
      prop->controlMask = smerge.controlMask;
      smerge.target->properties.emplace_back(prop);
    }
    for (auto &&lmerge : loopMerges)
    {
      auto prop = new PorpertyLoopMerge;
      prop->mergeBlock = moduleBuilder.getNode(lmerge.mergeBlock);
      prop->continueBlock = moduleBuilder.getNode(lmerge.continueBlock);
      prop->controlMask = lmerge.controlMask;
      prop->dependencyLength = lmerge.dependencyLength;
      lmerge.target->properties.emplace_back(prop);
    }
    for (auto &&prop : propertyReferenceResolves)
    {
      *prop.target = moduleBuilder.getNode(prop.ref);
    }
    for (auto &&prop : propertyTargetResolves)
    {
      moduleBuilder.getNode(prop.target)->properties.push_back(eastl::move(prop.property));
    }
    for (auto &&em : executionModes)
    {
      moduleBuilder.addExecutionMode(em.target, eastl::move(em.mode));
    }
  }
  bool operator()(const FileHeader &header)
  {
    // handle own endian only
    if (header.magic != MagicNumber)
      return false;
    moduleBuilder.setModuleVersion(header.version);
    moduleBuilder.setToolIdentification(header.toolId);
    moduleBuilder.setIdBounds(header.idBounds);
    return true;
  }
  void setAt(unsigned pos) { sourcePosition = pos; }
  unsigned getContextDependentSize(IdResultType type_id) { return moduleBuilder.getTypeSizeBits(moduleBuilder.getType(type_id)); }
  void onNop(Op)
  {
    auto node = moduleBuilder.newNode<NodeOpNop>();
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUndef(Op, IdResult id_result, IdResultType id_result_type)
  {
    auto node = moduleBuilder.newNode<NodeOpUndef>(id_result.value, moduleBuilder.getType(id_result_type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
    else
      moduleBuilder.addGlobalUndef(node);
  }
  void onSourceContinued(Op, LiteralString continued_source) { moduleBuilder.continueSourceString(continued_source.asStringObj()); }
  void onSource(Op, SourceLanguage source_language, LiteralInteger version, Optional<IdRef> file, Optional<LiteralString> source)
  {
    moduleBuilder.setSourceLanguage(source_language);
    moduleBuilder.setSourceLanguageVersion(version.value);
    if (file.valid)
      moduleBuilder.setSourceFile(file.value);
    if (source.valid)
      moduleBuilder.setSourceString(source.value.asStringObj());
  }
  void onSourceExtension(Op, LiteralString extension) { moduleBuilder.addSourceCodeExtension(extension.asStringObj()); }
  void onName(Op, IdRef target, LiteralString name)
  {
    CastableUniquePointer<PropertyName> nameProperty{new PropertyName};
    nameProperty->name = name.asStringObj();
    PropertyTargetResolveInfo info{target, eastl::move(nameProperty)};
    propertyTargetResolves.push_back(eastl::move(info));
  }
  void onMemberName(Op, IdRef type, LiteralInteger member, LiteralString name)
  {
    CastableUniquePointer<PropertyName> nameProperty{new PropertyName};
    nameProperty->memberIndex = member.value;
    nameProperty->name = name.asStringObj();
    PropertyTargetResolveInfo info{type, eastl::move(nameProperty)};
    propertyTargetResolves.push_back(eastl::move(info));
  }
  void onString(Op, IdResult id_result, LiteralString string)
  {
    auto node = moduleBuilder.newNode<NodeOpString>(id_result.value, string.asStringObj());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onLine(Op, IdRef file, LiteralInteger line, LiteralInteger column)
  {
    auto node = moduleBuilder.newNode<NodeOpLine>(moduleBuilder.getNode(file), line, column);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void enableExtension(Extension ext, const char *name, size_t length) { moduleBuilder.enableExtension(ext, name, length); }
  void loadExtendedGrammar(IdResult id, ExtendedGrammar egram, const char *name, size_t length)
  {
    moduleBuilder.loadExtendedGrammar(id, egram, name, length);
  }
  ExtendedGrammar getExtendedGrammarIdentifierFromRefId(IdRef id) { return moduleBuilder.getExtendedGrammarFromIdRef(id.value); }
  void onMemoryModel(Op, AddressingModel addressing_model, MemoryModel memory_model)
  {
    moduleBuilder.setAddressingModel(addressing_model);
    moduleBuilder.setMemoryModel(memory_model);
  }
  void onEntryPoint(Op, ExecutionModel execution_model, IdRef entry_point, LiteralString name, Multiple<IdRef> interface)
  {
    EntryPointDef entryPoint;
    entryPoint.executionModel = execution_model;
    entryPoint.function = entry_point;
    entryPoint.name = name;
    entryPoint.interface = interface;
    entryPoints.push_back(entryPoint);
  }
  void onExecutionMode(Op, IdRef entry_point, TypeTraits<ExecutionMode>::ReadType mode)
  {
    ExectionModeDef info;
    info.target = entry_point;
    switch (mode.value)
    {
      case ExecutionMode::Invocations:
        info.mode = new ExecutionModeInvocations(mode.data.Invocations.numberOfInvocationInvocations);
        break;
      case ExecutionMode::SpacingEqual: info.mode = new ExecutionModeSpacingEqual; break;
      case ExecutionMode::SpacingFractionalEven: info.mode = new ExecutionModeSpacingFractionalEven; break;
      case ExecutionMode::SpacingFractionalOdd: info.mode = new ExecutionModeSpacingFractionalOdd; break;
      case ExecutionMode::VertexOrderCw: info.mode = new ExecutionModeVertexOrderCw; break;
      case ExecutionMode::VertexOrderCcw: info.mode = new ExecutionModeVertexOrderCcw; break;
      case ExecutionMode::PixelCenterInteger: info.mode = new ExecutionModePixelCenterInteger; break;
      case ExecutionMode::OriginUpperLeft: info.mode = new ExecutionModeOriginUpperLeft; break;
      case ExecutionMode::OriginLowerLeft: info.mode = new ExecutionModeOriginLowerLeft; break;
      case ExecutionMode::EarlyFragmentTests: info.mode = new ExecutionModeEarlyFragmentTests; break;
      case ExecutionMode::PointMode: info.mode = new ExecutionModePointMode; break;
      case ExecutionMode::Xfb: info.mode = new ExecutionModeXfb; break;
      case ExecutionMode::DepthReplacing: info.mode = new ExecutionModeDepthReplacing; break;
      case ExecutionMode::DepthGreater: info.mode = new ExecutionModeDepthGreater; break;
      case ExecutionMode::DepthLess: info.mode = new ExecutionModeDepthLess; break;
      case ExecutionMode::DepthUnchanged: info.mode = new ExecutionModeDepthUnchanged; break;
      case ExecutionMode::LocalSize:
        info.mode = new ExecutionModeLocalSize(mode.data.LocalSize.xSize, mode.data.LocalSize.ySize, mode.data.LocalSize.zSize);
        break;
      case ExecutionMode::LocalSizeHint:
        info.mode =
          new ExecutionModeLocalSizeHint(mode.data.LocalSizeHint.xSize, mode.data.LocalSizeHint.ySize, mode.data.LocalSizeHint.zSize);
        break;
      case ExecutionMode::InputPoints: info.mode = new ExecutionModeInputPoints; break;
      case ExecutionMode::InputLines: info.mode = new ExecutionModeInputLines; break;
      case ExecutionMode::InputLinesAdjacency: info.mode = new ExecutionModeInputLinesAdjacency; break;
      case ExecutionMode::Triangles: info.mode = new ExecutionModeTriangles; break;
      case ExecutionMode::InputTrianglesAdjacency: info.mode = new ExecutionModeInputTrianglesAdjacency; break;
      case ExecutionMode::Quads: info.mode = new ExecutionModeQuads; break;
      case ExecutionMode::Isolines: info.mode = new ExecutionModeIsolines; break;
      case ExecutionMode::OutputVertices: info.mode = new ExecutionModeOutputVertices(mode.data.OutputVertices.vertexCount); break;
      case ExecutionMode::OutputPoints: info.mode = new ExecutionModeOutputPoints; break;
      case ExecutionMode::OutputLineStrip: info.mode = new ExecutionModeOutputLineStrip; break;
      case ExecutionMode::OutputTriangleStrip: info.mode = new ExecutionModeOutputTriangleStrip; break;
      case ExecutionMode::VecTypeHint: info.mode = new ExecutionModeVecTypeHint(mode.data.VecTypeHint.vectorType); break;
      case ExecutionMode::ContractionOff: info.mode = new ExecutionModeContractionOff; break;
      case ExecutionMode::Initializer: info.mode = new ExecutionModeInitializer; break;
      case ExecutionMode::Finalizer: info.mode = new ExecutionModeFinalizer; break;
      case ExecutionMode::SubgroupSize: info.mode = new ExecutionModeSubgroupSize(mode.data.SubgroupSize.subgroupSize); break;
      case ExecutionMode::SubgroupsPerWorkgroup:
        info.mode = new ExecutionModeSubgroupsPerWorkgroup(mode.data.SubgroupsPerWorkgroup.subgroupsPerWorkgroup);
        break;
      case ExecutionMode::SubgroupsPerWorkgroupId:
      {
        auto emode = new ExecutionModeSubgroupsPerWorkgroupId;
        PropertyReferenceResolveInfo refInfo;
        refInfo.target = &emode->subgroupsPerWorkgroup;
        refInfo.ref = mode.data.SubgroupsPerWorkgroupId.subgroupsPerWorkgroup;
        propertyReferenceResolves.push_back(refInfo);
        info.mode = emode;
      };
      break;
      case ExecutionMode::LocalSizeId:
      {
        auto emode = new ExecutionModeLocalSizeId;
        PropertyReferenceResolveInfo refInfo;
        refInfo.target = &emode->xSize;
        refInfo.ref = mode.data.LocalSizeId.xSize;
        propertyReferenceResolves.push_back(refInfo);
        refInfo.target = &emode->ySize;
        refInfo.ref = mode.data.LocalSizeId.ySize;
        propertyReferenceResolves.push_back(refInfo);
        refInfo.target = &emode->zSize;
        refInfo.ref = mode.data.LocalSizeId.zSize;
        propertyReferenceResolves.push_back(refInfo);
        info.mode = emode;
      };
      break;
      case ExecutionMode::LocalSizeHintId:
      {
        auto emode = new ExecutionModeLocalSizeHintId;
        PropertyReferenceResolveInfo refInfo;
        refInfo.target = &emode->xSizeHint;
        refInfo.ref = mode.data.LocalSizeHintId.xSizeHint;
        propertyReferenceResolves.push_back(refInfo);
        refInfo.target = &emode->ySizeHint;
        refInfo.ref = mode.data.LocalSizeHintId.ySizeHint;
        propertyReferenceResolves.push_back(refInfo);
        refInfo.target = &emode->zSizeHint;
        refInfo.ref = mode.data.LocalSizeHintId.zSizeHint;
        propertyReferenceResolves.push_back(refInfo);
        info.mode = emode;
      };
      break;
      case ExecutionMode::SubgroupUniformControlFlowKHR: info.mode = new ExecutionModeSubgroupUniformControlFlowKHR; break;
      case ExecutionMode::PostDepthCoverage: info.mode = new ExecutionModePostDepthCoverage; break;
      case ExecutionMode::DenormPreserve: info.mode = new ExecutionModeDenormPreserve(mode.data.DenormPreserve.targetWidth); break;
      case ExecutionMode::DenormFlushToZero:
        info.mode = new ExecutionModeDenormFlushToZero(mode.data.DenormFlushToZero.targetWidth);
        break;
      case ExecutionMode::SignedZeroInfNanPreserve:
        info.mode = new ExecutionModeSignedZeroInfNanPreserve(mode.data.SignedZeroInfNanPreserve.targetWidth);
        break;
      case ExecutionMode::RoundingModeRTE: info.mode = new ExecutionModeRoundingModeRTE(mode.data.RoundingModeRTE.targetWidth); break;
      case ExecutionMode::RoundingModeRTZ: info.mode = new ExecutionModeRoundingModeRTZ(mode.data.RoundingModeRTZ.targetWidth); break;
      case ExecutionMode::StencilRefReplacingEXT: info.mode = new ExecutionModeStencilRefReplacingEXT; break;
      case ExecutionMode::OutputLinesNV: info.mode = new ExecutionModeOutputLinesNV; break;
      case ExecutionMode::OutputPrimitivesNV:
        info.mode = new ExecutionModeOutputPrimitivesNV(mode.data.OutputPrimitivesNV.primitiveCount);
        break;
      case ExecutionMode::DerivativeGroupQuadsNV: info.mode = new ExecutionModeDerivativeGroupQuadsNV; break;
      case ExecutionMode::DerivativeGroupLinearNV: info.mode = new ExecutionModeDerivativeGroupLinearNV; break;
      case ExecutionMode::OutputTrianglesNV: info.mode = new ExecutionModeOutputTrianglesNV; break;
      case ExecutionMode::PixelInterlockOrderedEXT: info.mode = new ExecutionModePixelInterlockOrderedEXT; break;
      case ExecutionMode::PixelInterlockUnorderedEXT: info.mode = new ExecutionModePixelInterlockUnorderedEXT; break;
      case ExecutionMode::SampleInterlockOrderedEXT: info.mode = new ExecutionModeSampleInterlockOrderedEXT; break;
      case ExecutionMode::SampleInterlockUnorderedEXT: info.mode = new ExecutionModeSampleInterlockUnorderedEXT; break;
      case ExecutionMode::ShadingRateInterlockOrderedEXT: info.mode = new ExecutionModeShadingRateInterlockOrderedEXT; break;
      case ExecutionMode::ShadingRateInterlockUnorderedEXT: info.mode = new ExecutionModeShadingRateInterlockUnorderedEXT; break;
      case ExecutionMode::SharedLocalMemorySizeINTEL:
        info.mode = new ExecutionModeSharedLocalMemorySizeINTEL(mode.data.SharedLocalMemorySizeINTEL.size);
        break;
      case ExecutionMode::RoundingModeRTPINTEL:
        info.mode = new ExecutionModeRoundingModeRTPINTEL(mode.data.RoundingModeRTPINTEL.targetWidth);
        break;
      case ExecutionMode::RoundingModeRTNINTEL:
        info.mode = new ExecutionModeRoundingModeRTNINTEL(mode.data.RoundingModeRTNINTEL.targetWidth);
        break;
      case ExecutionMode::FloatingPointModeALTINTEL:
        info.mode = new ExecutionModeFloatingPointModeALTINTEL(mode.data.FloatingPointModeALTINTEL.targetWidth);
        break;
      case ExecutionMode::FloatingPointModeIEEEINTEL:
        info.mode = new ExecutionModeFloatingPointModeIEEEINTEL(mode.data.FloatingPointModeIEEEINTEL.targetWidth);
        break;
      case ExecutionMode::MaxWorkgroupSizeINTEL:
        info.mode = new ExecutionModeMaxWorkgroupSizeINTEL(mode.data.MaxWorkgroupSizeINTEL.max_x_size,
          mode.data.MaxWorkgroupSizeINTEL.max_y_size, mode.data.MaxWorkgroupSizeINTEL.max_z_size);
        break;
      case ExecutionMode::MaxWorkDimINTEL:
        info.mode = new ExecutionModeMaxWorkDimINTEL(mode.data.MaxWorkDimINTEL.max_dimensions);
        break;
      case ExecutionMode::NoGlobalOffsetINTEL: info.mode = new ExecutionModeNoGlobalOffsetINTEL; break;
      case ExecutionMode::NumSIMDWorkitemsINTEL:
        info.mode = new ExecutionModeNumSIMDWorkitemsINTEL(mode.data.NumSIMDWorkitemsINTEL.vector_width);
        break;
      case ExecutionMode::SchedulerTargetFmaxMhzINTEL:
        info.mode = new ExecutionModeSchedulerTargetFmaxMhzINTEL(mode.data.SchedulerTargetFmaxMhzINTEL.target_fmax);
        break;
    }
    executionModes.push_back(eastl::move(info));
  }
  void onCapability(Op, Capability capability) { moduleBuilder.enableCapability(capability); }
  void onTypeVoid(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeVoid>(id_result.value); }
  void onTypeBool(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeBool>(id_result.value); }
  void onTypeInt(Op, IdResult id_result, LiteralInteger width, LiteralInteger signedness)
  {
    moduleBuilder.newNode<NodeOpTypeInt>(id_result.value, width, signedness);
  }
  void onTypeFloat(Op, IdResult id_result, LiteralInteger width) { moduleBuilder.newNode<NodeOpTypeFloat>(id_result.value, width); }
  void onTypeVector(Op, IdResult id_result, IdRef component_type, LiteralInteger component_count)
  {
    moduleBuilder.newNode<NodeOpTypeVector>(id_result.value, moduleBuilder.getNode(component_type), component_count);
  }
  void onTypeMatrix(Op, IdResult id_result, IdRef column_type, LiteralInteger column_count)
  {
    moduleBuilder.newNode<NodeOpTypeMatrix>(id_result.value, moduleBuilder.getNode(column_type), column_count);
  }
  void onTypeImage(Op, IdResult id_result, IdRef sampled_type, Dim dim, LiteralInteger depth, LiteralInteger arrayed,
    LiteralInteger m_s, LiteralInteger sampled, ImageFormat image_format, Optional<AccessQualifier> access_qualifier)
  {
    eastl::optional<AccessQualifier> access_qualifierOpt;
    if (access_qualifier.valid)
    {
      access_qualifierOpt = access_qualifier.value;
    }
    moduleBuilder.newNode<NodeOpTypeImage>(id_result.value, moduleBuilder.getNode(sampled_type), dim, depth, arrayed, m_s, sampled,
      image_format, access_qualifierOpt);
  }
  void onTypeSampler(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeSampler>(id_result.value); }
  void onTypeSampledImage(Op, IdResult id_result, IdRef image_type)
  {
    moduleBuilder.newNode<NodeOpTypeSampledImage>(id_result.value, moduleBuilder.getNode(image_type));
  }
  void onTypeArray(Op, IdResult id_result, IdRef element_type, IdRef length)
  {
    moduleBuilder.newNode<NodeOpTypeArray>(id_result.value, moduleBuilder.getNode(element_type), moduleBuilder.getNode(length));
  }
  void onTypeRuntimeArray(Op, IdResult id_result, IdRef element_type)
  {
    moduleBuilder.newNode<NodeOpTypeRuntimeArray>(id_result.value, moduleBuilder.getNode(element_type));
  }
  void onTypeStruct(Op, IdResult id_result, Multiple<IdRef> param_1)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<NodePointer<NodeId>> param_1Var;
    while (!param_1.empty())
    {
      param_1Var.push_back(moduleBuilder.getNode(param_1.consume()));
    }
    moduleBuilder.newNode<NodeOpTypeStruct>(id_result.value, param_1Var.data(), param_1Var.size());
  }
  void onTypeOpaque(Op, IdResult id_result, LiteralString the_name_of_the_opaque_type)
  {
    moduleBuilder.newNode<NodeOpTypeOpaque>(id_result.value, the_name_of_the_opaque_type.asStringObj());
  }
  void onTypePointer(Op, IdResult id_result, StorageClass storage_class, IdRef type)
  {
    moduleBuilder.newNode<NodeOpTypePointer>(id_result.value, storage_class, moduleBuilder.getNode(type));
  }
  void onTypeFunction(Op, IdResult id_result, IdRef return_type, Multiple<IdRef> param_2)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<NodePointer<NodeId>> param_2Var;
    while (!param_2.empty())
    {
      param_2Var.push_back(moduleBuilder.getNode(param_2.consume()));
    }
    moduleBuilder.newNode<NodeOpTypeFunction>(id_result.value, moduleBuilder.getNode(return_type), param_2Var.data(),
      param_2Var.size());
  }
  void onTypeEvent(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeEvent>(id_result.value); }
  void onTypeDeviceEvent(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeDeviceEvent>(id_result.value); }
  void onTypeReserveId(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeReserveId>(id_result.value); }
  void onTypeQueue(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeQueue>(id_result.value); }
  void onTypePipe(Op, IdResult id_result, AccessQualifier qualifier)
  {
    moduleBuilder.newNode<NodeOpTypePipe>(id_result.value, qualifier);
  }
  void onTypeForwardPointer(Op, IdRef pointer_type, StorageClass storage_class)
  {
    // forward def is translated into a property of the target op, so we can later write the correct instructions
    // no need to store the storage class, has to be the same as the target pointer
    forwardProperties.push_back(pointer_type);
  }
  void onConstantTrue(Op, IdResult id_result, IdResultType id_result_type)
  {
    auto node = moduleBuilder.newNode<NodeOpConstantTrue>(id_result.value, moduleBuilder.getType(id_result_type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConstantFalse(Op, IdResult id_result, IdResultType id_result_type)
  {
    auto node = moduleBuilder.newNode<NodeOpConstantFalse>(id_result.value, moduleBuilder.getType(id_result_type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConstant(Op, IdResult id_result, IdResultType id_result_type, LiteralContextDependentNumber value)
  {
    auto node = moduleBuilder.newNode<NodeOpConstant>(id_result.value, moduleBuilder.getType(id_result_type), value);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConstantComposite(Op, IdResult id_result, IdResultType id_result_type, Multiple<IdRef> constituents)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<NodePointer<NodeId>> constituentsVar;
    while (!constituents.empty())
    {
      constituentsVar.push_back(moduleBuilder.getNode(constituents.consume()));
    }
    auto node = moduleBuilder.newNode<NodeOpConstantComposite>(id_result.value, moduleBuilder.getType(id_result_type),
      constituentsVar.data(), constituentsVar.size());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConstantSampler(Op, IdResult id_result, IdResultType id_result_type, SamplerAddressingMode sampler_addressing_mode,
    LiteralInteger param, SamplerFilterMode sampler_filter_mode)
  {
    auto node = moduleBuilder.newNode<NodeOpConstantSampler>(id_result.value, moduleBuilder.getType(id_result_type),
      sampler_addressing_mode, param, sampler_filter_mode);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConstantNull(Op, IdResult id_result, IdResultType id_result_type)
  {
    auto node = moduleBuilder.newNode<NodeOpConstantNull>(id_result.value, moduleBuilder.getType(id_result_type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecConstantTrue(Op, IdResult id_result, IdResultType id_result_type)
  {
    auto node = moduleBuilder.newNode<NodeOpSpecConstantTrue>(id_result.value, moduleBuilder.getType(id_result_type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecConstantFalse(Op, IdResult id_result, IdResultType id_result_type)
  {
    auto node = moduleBuilder.newNode<NodeOpSpecConstantFalse>(id_result.value, moduleBuilder.getType(id_result_type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecConstant(Op, IdResult id_result, IdResultType id_result_type, LiteralContextDependentNumber value)
  {
    auto node = moduleBuilder.newNode<NodeOpSpecConstant>(id_result.value, moduleBuilder.getType(id_result_type), value);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecConstantComposite(Op, IdResult id_result, IdResultType id_result_type, Multiple<IdRef> constituents)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<NodePointer<NodeId>> constituentsVar;
    while (!constituents.empty())
    {
      constituentsVar.push_back(moduleBuilder.getNode(constituents.consume()));
    }
    auto node = moduleBuilder.newNode<NodeOpSpecConstantComposite>(id_result.value, moduleBuilder.getType(id_result_type),
      constituentsVar.data(), constituentsVar.size());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecAccessChain(Op op, IdResult id, IdResultType type, IdRef base, Multiple<IdRef> indexes)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onAccessChain(op, id, type, base, indexes);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecInBoundsAccessChain(Op op, IdResult id, IdResultType type, IdRef base, Multiple<IdRef> indexes)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onInBoundsAccessChain(op, id, type, base, indexes);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecPtrAccessChain(Op op, IdResult id, IdResultType type, IdRef base, IdRef element, Multiple<IdRef> indexes)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onPtrAccessChain(op, id, type, base, element, indexes);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecInBoundsPtrAccessChain(Op op, IdResult id, IdResultType type, IdRef base, IdRef element, Multiple<IdRef> indexes)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onInBoundsPtrAccessChain(op, id, type, base, element, indexes);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecVectorShuffle(Op op, IdResult id, IdResultType type, IdRef vector_1, IdRef vector_2, Multiple<LiteralInteger> components)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onVectorShuffle(op, id, type, vector_1, vector_2, components);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecCompositeExtract(Op op, IdResult id, IdResultType type, IdRef composite, Multiple<LiteralInteger> indexes)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onCompositeExtract(op, id, type, composite, indexes);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecCompositeInsert(Op op, IdResult id, IdResultType type, IdRef object, IdRef composite, Multiple<LiteralInteger> indexes)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onCompositeInsert(op, id, type, object, composite, indexes);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecConvertFToU(Op op, IdResult id, IdResultType type, IdRef float_value)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onConvertFToU(op, id, type, float_value);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecConvertFToS(Op op, IdResult id, IdResultType type, IdRef float_value)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onConvertFToS(op, id, type, float_value);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecConvertSToF(Op op, IdResult id, IdResultType type, IdRef signed_value)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onConvertSToF(op, id, type, signed_value);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecConvertUToF(Op op, IdResult id, IdResultType type, IdRef unsigned_value)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onConvertUToF(op, id, type, unsigned_value);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecUConvert(Op op, IdResult id, IdResultType type, IdRef unsigned_value)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onUConvert(op, id, type, unsigned_value);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecSConvert(Op op, IdResult id, IdResultType type, IdRef signed_value)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onSConvert(op, id, type, signed_value);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecFConvert(Op op, IdResult id, IdResultType type, IdRef float_value)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onFConvert(op, id, type, float_value);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecQuantizeToF16(Op op, IdResult id, IdResultType type, IdRef value)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onQuantizeToF16(op, id, type, value);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecConvertPtrToU(Op op, IdResult id, IdResultType type, IdRef pointer)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onConvertPtrToU(op, id, type, pointer);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecConvertUToPtr(Op op, IdResult id, IdResultType type, IdRef integer_value)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onConvertUToPtr(op, id, type, integer_value);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecPtrCastToGeneric(Op op, IdResult id, IdResultType type, IdRef pointer)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onPtrCastToGeneric(op, id, type, pointer);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecGenericCastToPtr(Op op, IdResult id, IdResultType type, IdRef pointer)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onGenericCastToPtr(op, id, type, pointer);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecBitcast(Op op, IdResult id, IdResultType type, IdRef operand)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onBitcast(op, id, type, operand);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecSNegate(Op op, IdResult id, IdResultType type, IdRef operand)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onSNegate(op, id, type, operand);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecFNegate(Op op, IdResult id, IdResultType type, IdRef operand)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onFNegate(op, id, type, operand);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecIAdd(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onIAdd(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecFAdd(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onFAdd(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecISub(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onISub(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecFSub(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onFSub(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecIMul(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onIMul(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecFMul(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onFMul(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecUDiv(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onUDiv(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecSDiv(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onSDiv(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecFDiv(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onFDiv(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecUMod(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onUMod(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecSRem(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onSRem(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecSMod(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onSMod(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecFRem(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onFRem(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecFMod(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onFMod(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecLogicalEqual(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onLogicalEqual(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecLogicalNotEqual(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onLogicalNotEqual(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecLogicalOr(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onLogicalOr(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecLogicalAnd(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onLogicalAnd(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecLogicalNot(Op op, IdResult id, IdResultType type, IdRef operand)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onLogicalNot(op, id, type, operand);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecSelect(Op op, IdResult id, IdResultType type, IdRef condition, IdRef object_1, IdRef object_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onSelect(op, id, type, condition, object_1, object_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecIEqual(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onIEqual(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecINotEqual(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onINotEqual(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecUGreaterThan(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onUGreaterThan(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecSGreaterThan(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onSGreaterThan(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecUGreaterThanEqual(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onUGreaterThanEqual(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecSGreaterThanEqual(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onSGreaterThanEqual(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecULessThan(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onULessThan(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecSLessThan(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onSLessThan(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecULessThanEqual(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onULessThanEqual(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecSLessThanEqual(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onSLessThanEqual(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecShiftRightLogical(Op op, IdResult id, IdResultType type, IdRef base, IdRef shift)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onShiftRightLogical(op, id, type, base, shift);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecShiftRightArithmetic(Op op, IdResult id, IdResultType type, IdRef base, IdRef shift)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onShiftRightArithmetic(op, id, type, base, shift);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecShiftLeftLogical(Op op, IdResult id, IdResultType type, IdRef base, IdRef shift)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onShiftLeftLogical(op, id, type, base, shift);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecBitwiseOr(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onBitwiseOr(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecBitwiseXor(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onBitwiseXor(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecBitwiseAnd(Op op, IdResult id, IdResultType type, IdRef operand_1, IdRef operand_2)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onBitwiseAnd(op, id, type, operand_1, operand_2);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecNot(Op op, IdResult id, IdResultType type, IdRef operand)
  {
    // do not add node to current block, use static block to grab the node
    auto blockTemp = currentBlock;
    currentBlock = &specConstGrabBlock;
    onNot(op, id, type, operand);
    auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());
    specConstGrabBlock.instructions.pop_back();
    currentBlock = blockTemp;
    auto node = moduleBuilder.newNode<NodeOpSpecConstantOp>(specOp->resultId, specOp->resultType, specOp);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFunction(Op, IdResult id_result, IdResultType id_result_type, FunctionControlMask function_control, IdRef function_type)
  {
    currentFunction = moduleBuilder.newNode<NodeOpFunction>(id_result.value, moduleBuilder.getType(id_result_type), function_control,
      moduleBuilder.getNode(function_type));
  }
  void onFunctionParameter(Op, IdResult id_result, IdResultType id_result_type)
  {
    currentFunction->parameters.push_back(
      moduleBuilder.newNode<NodeOpFunctionParameter>(id_result.value, moduleBuilder.getType(id_result_type)));
  }
  void onFunctionEnd(Op) { currentFunction.reset(); }
  void onFunctionCall(Op, IdResult id_result, IdResultType id_result_type, IdRef function, Multiple<IdRef> param_3)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<NodePointer<NodeId>> param_3Var;
    while (!param_3.empty())
    {
      param_3Var.push_back(moduleBuilder.getNode(param_3.consume()));
    }
    auto node = moduleBuilder.newNode<NodeOpFunctionCall>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(function), param_3Var.data(), param_3Var.size());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onVariable(Op, IdResult id_result, IdResultType id_result_type, StorageClass storage_class, Optional<IdRef> initializer)
  {
    eastl::optional<NodePointer<NodeId>> initializerOpt;
    if (initializer.valid)
    {
      initializerOpt = moduleBuilder.getNode(initializer.value);
    }
    auto node =
      moduleBuilder.newNode<NodeOpVariable>(id_result.value, moduleBuilder.getType(id_result_type), storage_class, initializerOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
    else
      moduleBuilder.addGlobalVariable(node);
  }
  void onImageTexelPointer(Op, IdResult id_result, IdResultType id_result_type, IdRef image, IdRef coordinate, IdRef sample)
  {
    auto node = moduleBuilder.newNode<NodeOpImageTexelPointer>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image), moduleBuilder.getNode(coordinate), moduleBuilder.getNode(sample));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onLoad(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer,
    Optional<TypeTraits<MemoryAccessMask>::ReadType> memory_access)
  {
    eastl::optional<MemoryAccessMask> memory_accessVal;
    eastl::optional<LiteralInteger> memory_access_Aligned_first;
    NodePointer<NodeOperation> memory_access_MakePointerAvailable_first;
    NodePointer<NodeOperation> memory_access_MakePointerAvailableKHR_first;
    NodePointer<NodeOperation> memory_access_MakePointerVisible_first;
    NodePointer<NodeOperation> memory_access_MakePointerVisibleKHR_first;
    if (memory_access.valid)
    {
      memory_accessVal = memory_access.value.value;
      if ((memory_access.value.value & MemoryAccessMask::Aligned) != MemoryAccessMask::MaskNone)
      {
        memory_access_Aligned_first = memory_access.value.data.Aligned.first;
      }
      if ((memory_access.value.value & MemoryAccessMask::MakePointerAvailable) != MemoryAccessMask::MaskNone)
      {
        memory_access_MakePointerAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access.value.data.MakePointerAvailable.first));
      }
      if ((memory_access.value.value & MemoryAccessMask::MakePointerAvailableKHR) != MemoryAccessMask::MaskNone)
      {
        memory_access_MakePointerAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access.value.data.MakePointerAvailableKHR.first));
      }
      if ((memory_access.value.value & MemoryAccessMask::MakePointerVisible) != MemoryAccessMask::MaskNone)
      {
        memory_access_MakePointerVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access.value.data.MakePointerVisible.first));
      }
      if ((memory_access.value.value & MemoryAccessMask::MakePointerVisibleKHR) != MemoryAccessMask::MaskNone)
      {
        memory_access_MakePointerVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access.value.data.MakePointerVisibleKHR.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpLoad>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), memory_accessVal, memory_access_Aligned_first, memory_access_MakePointerAvailable_first,
      memory_access_MakePointerAvailableKHR_first, memory_access_MakePointerVisible_first, memory_access_MakePointerVisibleKHR_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onStore(Op, IdRef pointer, IdRef object, Optional<TypeTraits<MemoryAccessMask>::ReadType> memory_access)
  {
    eastl::optional<MemoryAccessMask> memory_accessVal;
    eastl::optional<LiteralInteger> memory_access_Aligned_first;
    NodePointer<NodeOperation> memory_access_MakePointerAvailable_first;
    NodePointer<NodeOperation> memory_access_MakePointerAvailableKHR_first;
    NodePointer<NodeOperation> memory_access_MakePointerVisible_first;
    NodePointer<NodeOperation> memory_access_MakePointerVisibleKHR_first;
    if (memory_access.valid)
    {
      memory_accessVal = memory_access.value.value;
      if ((memory_access.value.value & MemoryAccessMask::Aligned) != MemoryAccessMask::MaskNone)
      {
        memory_access_Aligned_first = memory_access.value.data.Aligned.first;
      }
      if ((memory_access.value.value & MemoryAccessMask::MakePointerAvailable) != MemoryAccessMask::MaskNone)
      {
        memory_access_MakePointerAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access.value.data.MakePointerAvailable.first));
      }
      if ((memory_access.value.value & MemoryAccessMask::MakePointerAvailableKHR) != MemoryAccessMask::MaskNone)
      {
        memory_access_MakePointerAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access.value.data.MakePointerAvailableKHR.first));
      }
      if ((memory_access.value.value & MemoryAccessMask::MakePointerVisible) != MemoryAccessMask::MaskNone)
      {
        memory_access_MakePointerVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access.value.data.MakePointerVisible.first));
      }
      if ((memory_access.value.value & MemoryAccessMask::MakePointerVisibleKHR) != MemoryAccessMask::MaskNone)
      {
        memory_access_MakePointerVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access.value.data.MakePointerVisibleKHR.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpStore>(moduleBuilder.getNode(pointer), moduleBuilder.getNode(object), memory_accessVal,
      memory_access_Aligned_first, memory_access_MakePointerAvailable_first, memory_access_MakePointerAvailableKHR_first,
      memory_access_MakePointerVisible_first, memory_access_MakePointerVisibleKHR_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onCopyMemory(Op, IdRef target, IdRef source, Optional<TypeTraits<MemoryAccessMask>::ReadType> memory_access0,
    Optional<TypeTraits<MemoryAccessMask>::ReadType> memory_access1)
  {
    eastl::optional<MemoryAccessMask> memory_access0Val;
    eastl::optional<LiteralInteger> memory_access0_Aligned_first;
    NodePointer<NodeOperation> memory_access0_MakePointerAvailable_first;
    NodePointer<NodeOperation> memory_access0_MakePointerAvailableKHR_first;
    NodePointer<NodeOperation> memory_access0_MakePointerVisible_first;
    NodePointer<NodeOperation> memory_access0_MakePointerVisibleKHR_first;
    if (memory_access0.valid)
    {
      memory_access0Val = memory_access0.value.value;
      if ((memory_access0.value.value & MemoryAccessMask::Aligned) != MemoryAccessMask::MaskNone)
      {
        memory_access0_Aligned_first = memory_access0.value.data.Aligned.first;
      }
      if ((memory_access0.value.value & MemoryAccessMask::MakePointerAvailable) != MemoryAccessMask::MaskNone)
      {
        memory_access0_MakePointerAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access0.value.data.MakePointerAvailable.first));
      }
      if ((memory_access0.value.value & MemoryAccessMask::MakePointerAvailableKHR) != MemoryAccessMask::MaskNone)
      {
        memory_access0_MakePointerAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access0.value.data.MakePointerAvailableKHR.first));
      }
      if ((memory_access0.value.value & MemoryAccessMask::MakePointerVisible) != MemoryAccessMask::MaskNone)
      {
        memory_access0_MakePointerVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access0.value.data.MakePointerVisible.first));
      }
      if ((memory_access0.value.value & MemoryAccessMask::MakePointerVisibleKHR) != MemoryAccessMask::MaskNone)
      {
        memory_access0_MakePointerVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access0.value.data.MakePointerVisibleKHR.first));
      }
    }
    eastl::optional<MemoryAccessMask> memory_access1Val;
    eastl::optional<LiteralInteger> memory_access1_Aligned_first;
    NodePointer<NodeOperation> memory_access1_MakePointerAvailable_first;
    NodePointer<NodeOperation> memory_access1_MakePointerAvailableKHR_first;
    NodePointer<NodeOperation> memory_access1_MakePointerVisible_first;
    NodePointer<NodeOperation> memory_access1_MakePointerVisibleKHR_first;
    if (memory_access1.valid)
    {
      memory_access1Val = memory_access1.value.value;
      if ((memory_access1.value.value & MemoryAccessMask::Aligned) != MemoryAccessMask::MaskNone)
      {
        memory_access1_Aligned_first = memory_access1.value.data.Aligned.first;
      }
      if ((memory_access1.value.value & MemoryAccessMask::MakePointerAvailable) != MemoryAccessMask::MaskNone)
      {
        memory_access1_MakePointerAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access1.value.data.MakePointerAvailable.first));
      }
      if ((memory_access1.value.value & MemoryAccessMask::MakePointerAvailableKHR) != MemoryAccessMask::MaskNone)
      {
        memory_access1_MakePointerAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access1.value.data.MakePointerAvailableKHR.first));
      }
      if ((memory_access1.value.value & MemoryAccessMask::MakePointerVisible) != MemoryAccessMask::MaskNone)
      {
        memory_access1_MakePointerVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access1.value.data.MakePointerVisible.first));
      }
      if ((memory_access1.value.value & MemoryAccessMask::MakePointerVisibleKHR) != MemoryAccessMask::MaskNone)
      {
        memory_access1_MakePointerVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access1.value.data.MakePointerVisibleKHR.first));
      }
    }
    auto node =
      moduleBuilder.newNode<NodeOpCopyMemory>(moduleBuilder.getNode(target), moduleBuilder.getNode(source), memory_access0Val,
        memory_access0_Aligned_first, memory_access0_MakePointerAvailable_first, memory_access0_MakePointerAvailableKHR_first,
        memory_access0_MakePointerVisible_first, memory_access0_MakePointerVisibleKHR_first, memory_access1Val,
        memory_access1_Aligned_first, memory_access1_MakePointerAvailable_first, memory_access1_MakePointerAvailableKHR_first,
        memory_access1_MakePointerVisible_first, memory_access1_MakePointerVisibleKHR_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onCopyMemorySized(Op, IdRef target, IdRef source, IdRef size, Optional<TypeTraits<MemoryAccessMask>::ReadType> memory_access0,
    Optional<TypeTraits<MemoryAccessMask>::ReadType> memory_access1)
  {
    eastl::optional<MemoryAccessMask> memory_access0Val;
    eastl::optional<LiteralInteger> memory_access0_Aligned_first;
    NodePointer<NodeOperation> memory_access0_MakePointerAvailable_first;
    NodePointer<NodeOperation> memory_access0_MakePointerAvailableKHR_first;
    NodePointer<NodeOperation> memory_access0_MakePointerVisible_first;
    NodePointer<NodeOperation> memory_access0_MakePointerVisibleKHR_first;
    if (memory_access0.valid)
    {
      memory_access0Val = memory_access0.value.value;
      if ((memory_access0.value.value & MemoryAccessMask::Aligned) != MemoryAccessMask::MaskNone)
      {
        memory_access0_Aligned_first = memory_access0.value.data.Aligned.first;
      }
      if ((memory_access0.value.value & MemoryAccessMask::MakePointerAvailable) != MemoryAccessMask::MaskNone)
      {
        memory_access0_MakePointerAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access0.value.data.MakePointerAvailable.first));
      }
      if ((memory_access0.value.value & MemoryAccessMask::MakePointerAvailableKHR) != MemoryAccessMask::MaskNone)
      {
        memory_access0_MakePointerAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access0.value.data.MakePointerAvailableKHR.first));
      }
      if ((memory_access0.value.value & MemoryAccessMask::MakePointerVisible) != MemoryAccessMask::MaskNone)
      {
        memory_access0_MakePointerVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access0.value.data.MakePointerVisible.first));
      }
      if ((memory_access0.value.value & MemoryAccessMask::MakePointerVisibleKHR) != MemoryAccessMask::MaskNone)
      {
        memory_access0_MakePointerVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access0.value.data.MakePointerVisibleKHR.first));
      }
    }
    eastl::optional<MemoryAccessMask> memory_access1Val;
    eastl::optional<LiteralInteger> memory_access1_Aligned_first;
    NodePointer<NodeOperation> memory_access1_MakePointerAvailable_first;
    NodePointer<NodeOperation> memory_access1_MakePointerAvailableKHR_first;
    NodePointer<NodeOperation> memory_access1_MakePointerVisible_first;
    NodePointer<NodeOperation> memory_access1_MakePointerVisibleKHR_first;
    if (memory_access1.valid)
    {
      memory_access1Val = memory_access1.value.value;
      if ((memory_access1.value.value & MemoryAccessMask::Aligned) != MemoryAccessMask::MaskNone)
      {
        memory_access1_Aligned_first = memory_access1.value.data.Aligned.first;
      }
      if ((memory_access1.value.value & MemoryAccessMask::MakePointerAvailable) != MemoryAccessMask::MaskNone)
      {
        memory_access1_MakePointerAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access1.value.data.MakePointerAvailable.first));
      }
      if ((memory_access1.value.value & MemoryAccessMask::MakePointerAvailableKHR) != MemoryAccessMask::MaskNone)
      {
        memory_access1_MakePointerAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access1.value.data.MakePointerAvailableKHR.first));
      }
      if ((memory_access1.value.value & MemoryAccessMask::MakePointerVisible) != MemoryAccessMask::MaskNone)
      {
        memory_access1_MakePointerVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access1.value.data.MakePointerVisible.first));
      }
      if ((memory_access1.value.value & MemoryAccessMask::MakePointerVisibleKHR) != MemoryAccessMask::MaskNone)
      {
        memory_access1_MakePointerVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access1.value.data.MakePointerVisibleKHR.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpCopyMemorySized>(moduleBuilder.getNode(target), moduleBuilder.getNode(source),
      moduleBuilder.getNode(size), memory_access0Val, memory_access0_Aligned_first, memory_access0_MakePointerAvailable_first,
      memory_access0_MakePointerAvailableKHR_first, memory_access0_MakePointerVisible_first,
      memory_access0_MakePointerVisibleKHR_first, memory_access1Val, memory_access1_Aligned_first,
      memory_access1_MakePointerAvailable_first, memory_access1_MakePointerAvailableKHR_first, memory_access1_MakePointerVisible_first,
      memory_access1_MakePointerVisibleKHR_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAccessChain(Op, IdResult id_result, IdResultType id_result_type, IdRef base, Multiple<IdRef> indexes)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<NodePointer<NodeId>> indexesVar;
    while (!indexes.empty())
    {
      indexesVar.push_back(moduleBuilder.getNode(indexes.consume()));
    }
    auto node = moduleBuilder.newNode<NodeOpAccessChain>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(base), indexesVar.data(), indexesVar.size());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onInBoundsAccessChain(Op, IdResult id_result, IdResultType id_result_type, IdRef base, Multiple<IdRef> indexes)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<NodePointer<NodeId>> indexesVar;
    while (!indexes.empty())
    {
      indexesVar.push_back(moduleBuilder.getNode(indexes.consume()));
    }
    auto node = moduleBuilder.newNode<NodeOpInBoundsAccessChain>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(base), indexesVar.data(), indexesVar.size());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onPtrAccessChain(Op, IdResult id_result, IdResultType id_result_type, IdRef base, IdRef element, Multiple<IdRef> indexes)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<NodePointer<NodeId>> indexesVar;
    while (!indexes.empty())
    {
      indexesVar.push_back(moduleBuilder.getNode(indexes.consume()));
    }
    auto node = moduleBuilder.newNode<NodeOpPtrAccessChain>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(base), moduleBuilder.getNode(element), indexesVar.data(), indexesVar.size());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArrayLength(Op, IdResult id_result, IdResultType id_result_type, IdRef structure, LiteralInteger array_member)
  {
    auto node = moduleBuilder.newNode<NodeOpArrayLength>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(structure), array_member);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGenericPtrMemSemantics(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer)
  {
    auto node = moduleBuilder.newNode<NodeOpGenericPtrMemSemantics>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onInBoundsPtrAccessChain(Op, IdResult id_result, IdResultType id_result_type, IdRef base, IdRef element,
    Multiple<IdRef> indexes)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<NodePointer<NodeId>> indexesVar;
    while (!indexes.empty())
    {
      indexesVar.push_back(moduleBuilder.getNode(indexes.consume()));
    }
    auto node = moduleBuilder.newNode<NodeOpInBoundsPtrAccessChain>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(base), moduleBuilder.getNode(element), indexesVar.data(), indexesVar.size());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onDecorate(Op, IdRef target, TypeTraits<Decoration>::ReadType decoration)
  {
    CastableUniquePointer<Property> propPtr;
    switch (decoration.value)
    {
      case Decoration::RelaxedPrecision:
      {
        auto newProp = new PropertyRelaxedPrecision;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::SpecId:
      {
        auto newProp = new PropertySpecId;
        propPtr.reset(newProp);
        newProp->specializationConstantId = decoration.data.SpecId.specializationConstantId;
        break;
      }
      case Decoration::Block:
      {
        auto newProp = new PropertyBlock;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BufferBlock:
      {
        auto newProp = new PropertyBufferBlock;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::RowMajor:
      {
        auto newProp = new PropertyRowMajor;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ColMajor:
      {
        auto newProp = new PropertyColMajor;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ArrayStride:
      {
        auto newProp = new PropertyArrayStride;
        propPtr.reset(newProp);
        newProp->arrayStride = decoration.data.ArrayStride.arrayStride;
        break;
      }
      case Decoration::MatrixStride:
      {
        auto newProp = new PropertyMatrixStride;
        propPtr.reset(newProp);
        newProp->matrixStride = decoration.data.MatrixStride.matrixStride;
        break;
      }
      case Decoration::GLSLShared:
      {
        auto newProp = new PropertyGLSLShared;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::GLSLPacked:
      {
        auto newProp = new PropertyGLSLPacked;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::CPacked:
      {
        auto newProp = new PropertyCPacked;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BuiltIn:
      {
        auto newProp = new PropertyBuiltIn;
        propPtr.reset(newProp);
        newProp->builtIn = decoration.data.BuiltInData.first;
        break;
      }
      case Decoration::NoPerspective:
      {
        auto newProp = new PropertyNoPerspective;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Flat:
      {
        auto newProp = new PropertyFlat;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Patch:
      {
        auto newProp = new PropertyPatch;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Centroid:
      {
        auto newProp = new PropertyCentroid;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Sample:
      {
        auto newProp = new PropertySample;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Invariant:
      {
        auto newProp = new PropertyInvariant;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Restrict:
      {
        auto newProp = new PropertyRestrict;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Aliased:
      {
        auto newProp = new PropertyAliased;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Volatile:
      {
        auto newProp = new PropertyVolatile;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Constant:
      {
        auto newProp = new PropertyConstant;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Coherent:
      {
        auto newProp = new PropertyCoherent;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::NonWritable:
      {
        auto newProp = new PropertyNonWritable;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::NonReadable:
      {
        auto newProp = new PropertyNonReadable;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Uniform:
      {
        auto newProp = new PropertyUniform;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::UniformId:
      {
        auto newProp = new PropertyUniformId;
        propPtr.reset(newProp);
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->execution), IdRef{decoration.data.UniformId.execution.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::SaturatedConversion:
      {
        auto newProp = new PropertySaturatedConversion;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Stream:
      {
        auto newProp = new PropertyStream;
        propPtr.reset(newProp);
        newProp->streamNumber = decoration.data.Stream.streamNumber;
        break;
      }
      case Decoration::Location:
      {
        auto newProp = new PropertyLocation;
        propPtr.reset(newProp);
        newProp->location = decoration.data.Location.location;
        break;
      }
      case Decoration::Component:
      {
        auto newProp = new PropertyComponent;
        propPtr.reset(newProp);
        newProp->component = decoration.data.Component.component;
        break;
      }
      case Decoration::Index:
      {
        auto newProp = new PropertyIndex;
        propPtr.reset(newProp);
        newProp->index = decoration.data.Index.index;
        break;
      }
      case Decoration::Binding:
      {
        auto newProp = new PropertyBinding;
        propPtr.reset(newProp);
        newProp->bindingPoint = decoration.data.Binding.bindingPoint;
        break;
      }
      case Decoration::DescriptorSet:
      {
        auto newProp = new PropertyDescriptorSet;
        propPtr.reset(newProp);
        newProp->descriptorSet = decoration.data.DescriptorSet.descriptorSet;
        break;
      }
      case Decoration::Offset:
      {
        auto newProp = new PropertyOffset;
        propPtr.reset(newProp);
        newProp->byteOffset = decoration.data.Offset.byteOffset;
        break;
      }
      case Decoration::XfbBuffer:
      {
        auto newProp = new PropertyXfbBuffer;
        propPtr.reset(newProp);
        newProp->xfbBufferNumber = decoration.data.XfbBuffer.xfbBufferNumber;
        break;
      }
      case Decoration::XfbStride:
      {
        auto newProp = new PropertyXfbStride;
        propPtr.reset(newProp);
        newProp->xfbStride = decoration.data.XfbStride.xfbStride;
        break;
      }
      case Decoration::FuncParamAttr:
      {
        auto newProp = new PropertyFuncParamAttr;
        propPtr.reset(newProp);
        newProp->functionParameterAttribute = decoration.data.FuncParamAttr.functionParameterAttribute;
        break;
      }
      case Decoration::FPRoundingMode:
      {
        auto newProp = new PropertyFPRoundingMode;
        propPtr.reset(newProp);
        newProp->floatingPointRoundingMode = decoration.data.FPRoundingMode.floatingPointRoundingMode;
        break;
      }
      case Decoration::FPFastMathMode:
      {
        auto newProp = new PropertyFPFastMathMode;
        propPtr.reset(newProp);
        newProp->fastMathMode = decoration.data.FPFastMathMode.fastMathMode;
        break;
      }
      case Decoration::LinkageAttributes:
      {
        auto newProp = new PropertyLinkageAttributes;
        propPtr.reset(newProp);
        newProp->name = decoration.data.LinkageAttributes.name.asStringObj();
        newProp->linkageType = decoration.data.LinkageAttributes.linkageType;
        break;
      }
      case Decoration::NoContraction:
      {
        auto newProp = new PropertyNoContraction;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::InputAttachmentIndex:
      {
        auto newProp = new PropertyInputAttachmentIndex;
        propPtr.reset(newProp);
        newProp->attachmentIndex = decoration.data.InputAttachmentIndex.attachmentIndex;
        break;
      }
      case Decoration::Alignment:
      {
        auto newProp = new PropertyAlignment;
        propPtr.reset(newProp);
        newProp->alignment = decoration.data.Alignment.alignment;
        break;
      }
      case Decoration::MaxByteOffset:
      {
        auto newProp = new PropertyMaxByteOffset;
        propPtr.reset(newProp);
        newProp->maxByteOffset = decoration.data.MaxByteOffset.maxByteOffset;
        break;
      }
      case Decoration::AlignmentId:
      {
        auto newProp = new PropertyAlignmentId;
        propPtr.reset(newProp);
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->alignment), IdRef{decoration.data.AlignmentId.alignment.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::MaxByteOffsetId:
      {
        auto newProp = new PropertyMaxByteOffsetId;
        propPtr.reset(newProp);
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->maxByteOffset),
            IdRef{decoration.data.MaxByteOffsetId.maxByteOffset.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::NoSignedWrap:
      {
        auto newProp = new PropertyNoSignedWrap;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::NoUnsignedWrap:
      {
        auto newProp = new PropertyNoUnsignedWrap;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ExplicitInterpAMD:
      {
        auto newProp = new PropertyExplicitInterpAMD;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::OverrideCoverageNV:
      {
        auto newProp = new PropertyOverrideCoverageNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PassthroughNV:
      {
        auto newProp = new PropertyPassthroughNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ViewportRelativeNV:
      {
        auto newProp = new PropertyViewportRelativeNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::SecondaryViewportRelativeNV:
      {
        auto newProp = new PropertySecondaryViewportRelativeNV;
        propPtr.reset(newProp);
        newProp->offset = decoration.data.SecondaryViewportRelativeNV.offset;
        break;
      }
      case Decoration::PerPrimitiveNV:
      {
        auto newProp = new PropertyPerPrimitiveNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PerViewNV:
      {
        auto newProp = new PropertyPerViewNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PerTaskNV:
      {
        auto newProp = new PropertyPerTaskNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PerVertexKHR:
      {
        auto newProp = new PropertyPerVertexKHR;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::NonUniform:
      {
        auto newProp = new PropertyNonUniform;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::RestrictPointer:
      {
        auto newProp = new PropertyRestrictPointer;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::AliasedPointer:
      {
        auto newProp = new PropertyAliasedPointer;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BindlessSamplerNV:
      {
        auto newProp = new PropertyBindlessSamplerNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BindlessImageNV:
      {
        auto newProp = new PropertyBindlessImageNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BoundSamplerNV:
      {
        auto newProp = new PropertyBoundSamplerNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BoundImageNV:
      {
        auto newProp = new PropertyBoundImageNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::SIMTCallINTEL:
      {
        auto newProp = new PropertySIMTCallINTEL;
        propPtr.reset(newProp);
        newProp->n = decoration.data.SIMTCallINTEL.n;
        break;
      }
      case Decoration::ReferencedIndirectlyINTEL:
      {
        auto newProp = new PropertyReferencedIndirectlyINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ClobberINTEL:
      {
        auto newProp = new PropertyClobberINTEL;
        propPtr.reset(newProp);
        newProp->_register = decoration.data.ClobberINTEL._register.asStringObj();
        break;
      }
      case Decoration::SideEffectsINTEL:
      {
        auto newProp = new PropertySideEffectsINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::VectorComputeVariableINTEL:
      {
        auto newProp = new PropertyVectorComputeVariableINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::FuncParamIOKindINTEL:
      {
        auto newProp = new PropertyFuncParamIOKindINTEL;
        propPtr.reset(newProp);
        newProp->kind = decoration.data.FuncParamIOKindINTEL.kind;
        break;
      }
      case Decoration::VectorComputeFunctionINTEL:
      {
        auto newProp = new PropertyVectorComputeFunctionINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::StackCallINTEL:
      {
        auto newProp = new PropertyStackCallINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::GlobalVariableOffsetINTEL:
      {
        auto newProp = new PropertyGlobalVariableOffsetINTEL;
        propPtr.reset(newProp);
        newProp->offset = decoration.data.GlobalVariableOffsetINTEL.offset;
        break;
      }
      case Decoration::CounterBuffer:
      {
        auto newProp = new PropertyCounterBuffer;
        propPtr.reset(newProp);
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->counterBuffer), IdRef{decoration.data.CounterBuffer.counterBuffer.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::UserSemantic:
      {
        auto newProp = new PropertyUserSemantic;
        propPtr.reset(newProp);
        newProp->semantic = decoration.data.UserSemantic.semantic.asStringObj();
        break;
      }
      case Decoration::UserTypeGOOGLE:
      {
        auto newProp = new PropertyUserTypeGOOGLE;
        propPtr.reset(newProp);
        newProp->userType = decoration.data.UserTypeGOOGLE.userType.asStringObj();
        break;
      }
      case Decoration::FunctionRoundingModeINTEL:
      {
        auto newProp = new PropertyFunctionRoundingModeINTEL;
        propPtr.reset(newProp);
        newProp->targetWidth = decoration.data.FunctionRoundingModeINTEL.targetWidth;
        newProp->fpRoundingMode = decoration.data.FunctionRoundingModeINTEL.fpRoundingMode;
        break;
      }
      case Decoration::FunctionDenormModeINTEL:
      {
        auto newProp = new PropertyFunctionDenormModeINTEL;
        propPtr.reset(newProp);
        newProp->targetWidth = decoration.data.FunctionDenormModeINTEL.targetWidth;
        newProp->fpDenormMode = decoration.data.FunctionDenormModeINTEL.fpDenormMode;
        break;
      }
      case Decoration::RegisterINTEL:
      {
        auto newProp = new PropertyRegisterINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::MemoryINTEL:
      {
        auto newProp = new PropertyMemoryINTEL;
        propPtr.reset(newProp);
        newProp->memoryType = decoration.data.MemoryINTEL.memoryType.asStringObj();
        break;
      }
      case Decoration::NumbanksINTEL:
      {
        auto newProp = new PropertyNumbanksINTEL;
        propPtr.reset(newProp);
        newProp->banks = decoration.data.NumbanksINTEL.banks;
        break;
      }
      case Decoration::BankwidthINTEL:
      {
        auto newProp = new PropertyBankwidthINTEL;
        propPtr.reset(newProp);
        newProp->bankWidth = decoration.data.BankwidthINTEL.bankWidth;
        break;
      }
      case Decoration::MaxPrivateCopiesINTEL:
      {
        auto newProp = new PropertyMaxPrivateCopiesINTEL;
        propPtr.reset(newProp);
        newProp->maximumCopies = decoration.data.MaxPrivateCopiesINTEL.maximumCopies;
        break;
      }
      case Decoration::SinglepumpINTEL:
      {
        auto newProp = new PropertySinglepumpINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::DoublepumpINTEL:
      {
        auto newProp = new PropertyDoublepumpINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::MaxReplicatesINTEL:
      {
        auto newProp = new PropertyMaxReplicatesINTEL;
        propPtr.reset(newProp);
        newProp->maximumReplicates = decoration.data.MaxReplicatesINTEL.maximumReplicates;
        break;
      }
      case Decoration::SimpleDualPortINTEL:
      {
        auto newProp = new PropertySimpleDualPortINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::MergeINTEL:
      {
        auto newProp = new PropertyMergeINTEL;
        propPtr.reset(newProp);
        newProp->mergeKey = decoration.data.MergeINTEL.mergeKey.asStringObj();
        newProp->mergeType = decoration.data.MergeINTEL.mergeType.asStringObj();
        break;
      }
      case Decoration::BankBitsINTEL:
      {
        auto newProp = new PropertyBankBitsINTEL;
        propPtr.reset(newProp);
        newProp->bankBits = decoration.data.BankBitsINTEL.bankBits;
        break;
      }
      case Decoration::ForcePow2DepthINTEL:
      {
        auto newProp = new PropertyForcePow2DepthINTEL;
        propPtr.reset(newProp);
        newProp->forceKey = decoration.data.ForcePow2DepthINTEL.forceKey;
        break;
      }
      case Decoration::BurstCoalesceINTEL:
      {
        auto newProp = new PropertyBurstCoalesceINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::CacheSizeINTEL:
      {
        auto newProp = new PropertyCacheSizeINTEL;
        propPtr.reset(newProp);
        newProp->cacheSizeInBytes = decoration.data.CacheSizeINTEL.cacheSizeInBytes;
        break;
      }
      case Decoration::DontStaticallyCoalesceINTEL:
      {
        auto newProp = new PropertyDontStaticallyCoalesceINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PrefetchINTEL:
      {
        auto newProp = new PropertyPrefetchINTEL;
        propPtr.reset(newProp);
        newProp->prefetcherSizeInBytes = decoration.data.PrefetchINTEL.prefetcherSizeInBytes;
        break;
      }
      case Decoration::StallEnableINTEL:
      {
        auto newProp = new PropertyStallEnableINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::FuseLoopsInFunctionINTEL:
      {
        auto newProp = new PropertyFuseLoopsInFunctionINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BufferLocationINTEL:
      {
        auto newProp = new PropertyBufferLocationINTEL;
        propPtr.reset(newProp);
        newProp->bufferLocationId = decoration.data.BufferLocationINTEL.bufferLocationId;
        break;
      }
      case Decoration::IOPipeStorageINTEL:
      {
        auto newProp = new PropertyIOPipeStorageINTEL;
        propPtr.reset(newProp);
        newProp->ioPipeId = decoration.data.IOPipeStorageINTEL.ioPipeId;
        break;
      }
      case Decoration::FunctionFloatingPointModeINTEL:
      {
        auto newProp = new PropertyFunctionFloatingPointModeINTEL;
        propPtr.reset(newProp);
        newProp->targetWidth = decoration.data.FunctionFloatingPointModeINTEL.targetWidth;
        newProp->fpOperationMode = decoration.data.FunctionFloatingPointModeINTEL.fpOperationMode;
        break;
      }
      case Decoration::SingleElementVectorINTEL:
      {
        auto newProp = new PropertySingleElementVectorINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::VectorComputeCallableFunctionINTEL:
      {
        auto newProp = new PropertyVectorComputeCallableFunctionINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::MediaBlockIOINTEL:
      {
        auto newProp = new PropertyMediaBlockIOINTEL;
        propPtr.reset(newProp);
        break;
      }
    }
    PropertyTargetResolveInfo targetResolve //
      {target, eastl::move(propPtr)};
    propertyTargetResolves.push_back(eastl::move(targetResolve));
  }
  void onMemberDecorate(Op, IdRef structure_type, LiteralInteger member, TypeTraits<Decoration>::ReadType decoration)
  {
    CastableUniquePointer<Property> propPtr;
    switch (decoration.value)
    {
      case Decoration::RelaxedPrecision:
      {
        auto newProp = new PropertyRelaxedPrecision;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::SpecId:
      {
        auto newProp = new PropertySpecId;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->specializationConstantId = decoration.data.SpecId.specializationConstantId;
        break;
      }
      case Decoration::Block:
      {
        auto newProp = new PropertyBlock;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BufferBlock:
      {
        auto newProp = new PropertyBufferBlock;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::RowMajor:
      {
        auto newProp = new PropertyRowMajor;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::ColMajor:
      {
        auto newProp = new PropertyColMajor;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::ArrayStride:
      {
        auto newProp = new PropertyArrayStride;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->arrayStride = decoration.data.ArrayStride.arrayStride;
        break;
      }
      case Decoration::MatrixStride:
      {
        auto newProp = new PropertyMatrixStride;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->matrixStride = decoration.data.MatrixStride.matrixStride;
        break;
      }
      case Decoration::GLSLShared:
      {
        auto newProp = new PropertyGLSLShared;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::GLSLPacked:
      {
        auto newProp = new PropertyGLSLPacked;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::CPacked:
      {
        auto newProp = new PropertyCPacked;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BuiltIn:
      {
        auto newProp = new PropertyBuiltIn;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->builtIn = decoration.data.BuiltInData.first;
        break;
      }
      case Decoration::NoPerspective:
      {
        auto newProp = new PropertyNoPerspective;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Flat:
      {
        auto newProp = new PropertyFlat;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Patch:
      {
        auto newProp = new PropertyPatch;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Centroid:
      {
        auto newProp = new PropertyCentroid;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Sample:
      {
        auto newProp = new PropertySample;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Invariant:
      {
        auto newProp = new PropertyInvariant;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Restrict:
      {
        auto newProp = new PropertyRestrict;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Aliased:
      {
        auto newProp = new PropertyAliased;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Volatile:
      {
        auto newProp = new PropertyVolatile;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Constant:
      {
        auto newProp = new PropertyConstant;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Coherent:
      {
        auto newProp = new PropertyCoherent;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::NonWritable:
      {
        auto newProp = new PropertyNonWritable;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::NonReadable:
      {
        auto newProp = new PropertyNonReadable;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Uniform:
      {
        auto newProp = new PropertyUniform;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::UniformId:
      {
        auto newProp = new PropertyUniformId;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->execution), IdRef{decoration.data.UniformId.execution.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::SaturatedConversion:
      {
        auto newProp = new PropertySaturatedConversion;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Stream:
      {
        auto newProp = new PropertyStream;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->streamNumber = decoration.data.Stream.streamNumber;
        break;
      }
      case Decoration::Location:
      {
        auto newProp = new PropertyLocation;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->location = decoration.data.Location.location;
        break;
      }
      case Decoration::Component:
      {
        auto newProp = new PropertyComponent;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->component = decoration.data.Component.component;
        break;
      }
      case Decoration::Index:
      {
        auto newProp = new PropertyIndex;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->index = decoration.data.Index.index;
        break;
      }
      case Decoration::Binding:
      {
        auto newProp = new PropertyBinding;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->bindingPoint = decoration.data.Binding.bindingPoint;
        break;
      }
      case Decoration::DescriptorSet:
      {
        auto newProp = new PropertyDescriptorSet;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->descriptorSet = decoration.data.DescriptorSet.descriptorSet;
        break;
      }
      case Decoration::Offset:
      {
        auto newProp = new PropertyOffset;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->byteOffset = decoration.data.Offset.byteOffset;
        break;
      }
      case Decoration::XfbBuffer:
      {
        auto newProp = new PropertyXfbBuffer;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->xfbBufferNumber = decoration.data.XfbBuffer.xfbBufferNumber;
        break;
      }
      case Decoration::XfbStride:
      {
        auto newProp = new PropertyXfbStride;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->xfbStride = decoration.data.XfbStride.xfbStride;
        break;
      }
      case Decoration::FuncParamAttr:
      {
        auto newProp = new PropertyFuncParamAttr;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->functionParameterAttribute = decoration.data.FuncParamAttr.functionParameterAttribute;
        break;
      }
      case Decoration::FPRoundingMode:
      {
        auto newProp = new PropertyFPRoundingMode;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->floatingPointRoundingMode = decoration.data.FPRoundingMode.floatingPointRoundingMode;
        break;
      }
      case Decoration::FPFastMathMode:
      {
        auto newProp = new PropertyFPFastMathMode;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->fastMathMode = decoration.data.FPFastMathMode.fastMathMode;
        break;
      }
      case Decoration::LinkageAttributes:
      {
        auto newProp = new PropertyLinkageAttributes;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->name = decoration.data.LinkageAttributes.name.asStringObj();
        newProp->linkageType = decoration.data.LinkageAttributes.linkageType;
        break;
      }
      case Decoration::NoContraction:
      {
        auto newProp = new PropertyNoContraction;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::InputAttachmentIndex:
      {
        auto newProp = new PropertyInputAttachmentIndex;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->attachmentIndex = decoration.data.InputAttachmentIndex.attachmentIndex;
        break;
      }
      case Decoration::Alignment:
      {
        auto newProp = new PropertyAlignment;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->alignment = decoration.data.Alignment.alignment;
        break;
      }
      case Decoration::MaxByteOffset:
      {
        auto newProp = new PropertyMaxByteOffset;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->maxByteOffset = decoration.data.MaxByteOffset.maxByteOffset;
        break;
      }
      case Decoration::AlignmentId:
      {
        auto newProp = new PropertyAlignmentId;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->alignment), IdRef{decoration.data.AlignmentId.alignment.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::MaxByteOffsetId:
      {
        auto newProp = new PropertyMaxByteOffsetId;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->maxByteOffset),
            IdRef{decoration.data.MaxByteOffsetId.maxByteOffset.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::NoSignedWrap:
      {
        auto newProp = new PropertyNoSignedWrap;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::NoUnsignedWrap:
      {
        auto newProp = new PropertyNoUnsignedWrap;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::ExplicitInterpAMD:
      {
        auto newProp = new PropertyExplicitInterpAMD;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::OverrideCoverageNV:
      {
        auto newProp = new PropertyOverrideCoverageNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::PassthroughNV:
      {
        auto newProp = new PropertyPassthroughNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::ViewportRelativeNV:
      {
        auto newProp = new PropertyViewportRelativeNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::SecondaryViewportRelativeNV:
      {
        auto newProp = new PropertySecondaryViewportRelativeNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->offset = decoration.data.SecondaryViewportRelativeNV.offset;
        break;
      }
      case Decoration::PerPrimitiveNV:
      {
        auto newProp = new PropertyPerPrimitiveNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::PerViewNV:
      {
        auto newProp = new PropertyPerViewNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::PerTaskNV:
      {
        auto newProp = new PropertyPerTaskNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::PerVertexKHR:
      {
        auto newProp = new PropertyPerVertexKHR;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::NonUniform:
      {
        auto newProp = new PropertyNonUniform;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::RestrictPointer:
      {
        auto newProp = new PropertyRestrictPointer;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::AliasedPointer:
      {
        auto newProp = new PropertyAliasedPointer;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BindlessSamplerNV:
      {
        auto newProp = new PropertyBindlessSamplerNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BindlessImageNV:
      {
        auto newProp = new PropertyBindlessImageNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BoundSamplerNV:
      {
        auto newProp = new PropertyBoundSamplerNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BoundImageNV:
      {
        auto newProp = new PropertyBoundImageNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::SIMTCallINTEL:
      {
        auto newProp = new PropertySIMTCallINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->n = decoration.data.SIMTCallINTEL.n;
        break;
      }
      case Decoration::ReferencedIndirectlyINTEL:
      {
        auto newProp = new PropertyReferencedIndirectlyINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::ClobberINTEL:
      {
        auto newProp = new PropertyClobberINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->_register = decoration.data.ClobberINTEL._register.asStringObj();
        break;
      }
      case Decoration::SideEffectsINTEL:
      {
        auto newProp = new PropertySideEffectsINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::VectorComputeVariableINTEL:
      {
        auto newProp = new PropertyVectorComputeVariableINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::FuncParamIOKindINTEL:
      {
        auto newProp = new PropertyFuncParamIOKindINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->kind = decoration.data.FuncParamIOKindINTEL.kind;
        break;
      }
      case Decoration::VectorComputeFunctionINTEL:
      {
        auto newProp = new PropertyVectorComputeFunctionINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::StackCallINTEL:
      {
        auto newProp = new PropertyStackCallINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::GlobalVariableOffsetINTEL:
      {
        auto newProp = new PropertyGlobalVariableOffsetINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->offset = decoration.data.GlobalVariableOffsetINTEL.offset;
        break;
      }
      case Decoration::CounterBuffer:
      {
        auto newProp = new PropertyCounterBuffer;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->counterBuffer), IdRef{decoration.data.CounterBuffer.counterBuffer.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::UserSemantic:
      {
        auto newProp = new PropertyUserSemantic;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->semantic = decoration.data.UserSemantic.semantic.asStringObj();
        break;
      }
      case Decoration::UserTypeGOOGLE:
      {
        auto newProp = new PropertyUserTypeGOOGLE;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->userType = decoration.data.UserTypeGOOGLE.userType.asStringObj();
        break;
      }
      case Decoration::FunctionRoundingModeINTEL:
      {
        auto newProp = new PropertyFunctionRoundingModeINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->targetWidth = decoration.data.FunctionRoundingModeINTEL.targetWidth;
        newProp->fpRoundingMode = decoration.data.FunctionRoundingModeINTEL.fpRoundingMode;
        break;
      }
      case Decoration::FunctionDenormModeINTEL:
      {
        auto newProp = new PropertyFunctionDenormModeINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->targetWidth = decoration.data.FunctionDenormModeINTEL.targetWidth;
        newProp->fpDenormMode = decoration.data.FunctionDenormModeINTEL.fpDenormMode;
        break;
      }
      case Decoration::RegisterINTEL:
      {
        auto newProp = new PropertyRegisterINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::MemoryINTEL:
      {
        auto newProp = new PropertyMemoryINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->memoryType = decoration.data.MemoryINTEL.memoryType.asStringObj();
        break;
      }
      case Decoration::NumbanksINTEL:
      {
        auto newProp = new PropertyNumbanksINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->banks = decoration.data.NumbanksINTEL.banks;
        break;
      }
      case Decoration::BankwidthINTEL:
      {
        auto newProp = new PropertyBankwidthINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->bankWidth = decoration.data.BankwidthINTEL.bankWidth;
        break;
      }
      case Decoration::MaxPrivateCopiesINTEL:
      {
        auto newProp = new PropertyMaxPrivateCopiesINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->maximumCopies = decoration.data.MaxPrivateCopiesINTEL.maximumCopies;
        break;
      }
      case Decoration::SinglepumpINTEL:
      {
        auto newProp = new PropertySinglepumpINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::DoublepumpINTEL:
      {
        auto newProp = new PropertyDoublepumpINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::MaxReplicatesINTEL:
      {
        auto newProp = new PropertyMaxReplicatesINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->maximumReplicates = decoration.data.MaxReplicatesINTEL.maximumReplicates;
        break;
      }
      case Decoration::SimpleDualPortINTEL:
      {
        auto newProp = new PropertySimpleDualPortINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::MergeINTEL:
      {
        auto newProp = new PropertyMergeINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->mergeKey = decoration.data.MergeINTEL.mergeKey.asStringObj();
        newProp->mergeType = decoration.data.MergeINTEL.mergeType.asStringObj();
        break;
      }
      case Decoration::BankBitsINTEL:
      {
        auto newProp = new PropertyBankBitsINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->bankBits = decoration.data.BankBitsINTEL.bankBits;
        break;
      }
      case Decoration::ForcePow2DepthINTEL:
      {
        auto newProp = new PropertyForcePow2DepthINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->forceKey = decoration.data.ForcePow2DepthINTEL.forceKey;
        break;
      }
      case Decoration::BurstCoalesceINTEL:
      {
        auto newProp = new PropertyBurstCoalesceINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::CacheSizeINTEL:
      {
        auto newProp = new PropertyCacheSizeINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->cacheSizeInBytes = decoration.data.CacheSizeINTEL.cacheSizeInBytes;
        break;
      }
      case Decoration::DontStaticallyCoalesceINTEL:
      {
        auto newProp = new PropertyDontStaticallyCoalesceINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::PrefetchINTEL:
      {
        auto newProp = new PropertyPrefetchINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->prefetcherSizeInBytes = decoration.data.PrefetchINTEL.prefetcherSizeInBytes;
        break;
      }
      case Decoration::StallEnableINTEL:
      {
        auto newProp = new PropertyStallEnableINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::FuseLoopsInFunctionINTEL:
      {
        auto newProp = new PropertyFuseLoopsInFunctionINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BufferLocationINTEL:
      {
        auto newProp = new PropertyBufferLocationINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->bufferLocationId = decoration.data.BufferLocationINTEL.bufferLocationId;
        break;
      }
      case Decoration::IOPipeStorageINTEL:
      {
        auto newProp = new PropertyIOPipeStorageINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->ioPipeId = decoration.data.IOPipeStorageINTEL.ioPipeId;
        break;
      }
      case Decoration::FunctionFloatingPointModeINTEL:
      {
        auto newProp = new PropertyFunctionFloatingPointModeINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->targetWidth = decoration.data.FunctionFloatingPointModeINTEL.targetWidth;
        newProp->fpOperationMode = decoration.data.FunctionFloatingPointModeINTEL.fpOperationMode;
        break;
      }
      case Decoration::SingleElementVectorINTEL:
      {
        auto newProp = new PropertySingleElementVectorINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::VectorComputeCallableFunctionINTEL:
      {
        auto newProp = new PropertyVectorComputeCallableFunctionINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::MediaBlockIOINTEL:
      {
        auto newProp = new PropertyMediaBlockIOINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
    }
    PropertyTargetResolveInfo targetResolve //
      {structure_type, eastl::move(propPtr)};
    propertyTargetResolves.push_back(eastl::move(targetResolve));
  }
  void onDecorationGroup(Op, IdResult id_result)
  {
    auto node = moduleBuilder.newNode<NodeOpDecorationGroup>(id_result.value);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupDecorate(Op, IdRef decoration_group, Multiple<IdRef> targets)
  {
    // need to store new infos into separate vectors and append them later to not invalidate iterators
    eastl::vector<PropertyTargetResolveInfo> targetsToAdd;
    eastl::vector<PropertyReferenceResolveInfo> refsToAdd;
    for (auto &&prop : propertyTargetResolves)
    {
      if (prop.target == decoration_group)
      {
        while (targets.count)
        {
          auto newTargetValue = targets.consume();
          auto newProp = visitProperty(prop.property.get(), PorpertyCloneVisitor{});
          PropertyTargetResolveInfo targetResolve{newTargetValue, eastl::move(newProp)};
          if (is<PropertyUniformId>(newProp))
          {
            PropertyReferenceResolveInfo refResolve //
              {reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyUniformId>(newProp)->execution)};
            auto cmp = reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyUniformId>(prop.property)->execution);
            for (auto &&ref : propertyReferenceResolves)
            {
              if (ref.target == cmp)
              {
                refResolve.ref = ref.ref;
                refsToAdd.push_back(refResolve);
                break;
              }
            }
          }
          else if (is<PropertyAlignmentId>(newProp))
          {
            PropertyReferenceResolveInfo refResolve //
              {reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyAlignmentId>(newProp)->alignment)};
            auto cmp = reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyAlignmentId>(prop.property)->alignment);
            for (auto &&ref : propertyReferenceResolves)
            {
              if (ref.target == cmp)
              {
                refResolve.ref = ref.ref;
                refsToAdd.push_back(refResolve);
                break;
              }
            }
          }
          else if (is<PropertyMaxByteOffsetId>(newProp))
          {
            PropertyReferenceResolveInfo refResolve //
              {reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyMaxByteOffsetId>(newProp)->maxByteOffset)};
            auto cmp = reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyMaxByteOffsetId>(prop.property)->maxByteOffset);
            for (auto &&ref : propertyReferenceResolves)
            {
              if (ref.target == cmp)
              {
                refResolve.ref = ref.ref;
                refsToAdd.push_back(refResolve);
                break;
              }
            }
          }
          else if (is<PropertyCounterBuffer>(newProp))
          {
            PropertyReferenceResolveInfo refResolve //
              {reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyCounterBuffer>(newProp)->counterBuffer)};
            auto cmp = reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyCounterBuffer>(prop.property)->counterBuffer);
            for (auto &&ref : propertyReferenceResolves)
            {
              if (ref.target == cmp)
              {
                refResolve.ref = ref.ref;
                refsToAdd.push_back(refResolve);
                break;
              }
            }
          }
          else if (is<PropertyHlslCounterBufferGOOGLE>(newProp))
          {
            PropertyReferenceResolveInfo refResolve //
              {reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyHlslCounterBufferGOOGLE>(newProp)->counterBuffer)};
            auto cmp = reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyHlslCounterBufferGOOGLE>(prop.property)->counterBuffer);
            for (auto &&ref : propertyReferenceResolves)
            {
              if (ref.target == cmp)
              {
                refResolve.ref = ref.ref;
                refsToAdd.push_back(refResolve);
                break;
              }
            }
          }
          targetsToAdd.push_back(eastl::move(targetResolve));
        }
      }
    }
    propertyReferenceResolves.insert(propertyReferenceResolves.end(), refsToAdd.begin(), refsToAdd.end());
    for (auto &&ta : targetsToAdd)
      propertyTargetResolves.push_back(eastl::move(ta));
  }
  void onGroupMemberDecorate(Op, IdRef decoration_group, Multiple<PairIdRefLiteralInteger> targets)
  {
    // need to store new infos into separate vectors and append them later to not invalidate iterators
    eastl::vector<PropertyTargetResolveInfo> targetsToAdd;
    eastl::vector<PropertyReferenceResolveInfo> refsToAdd;
    for (auto &&prop : propertyTargetResolves)
    {
      if (prop.target == decoration_group)
      {
        while (targets.count)
        {
          auto newTargetValue = targets.consume();
          auto newProp = visitProperty(prop.property.get(), PorpertyCloneWithMemberIndexVisitor{newTargetValue.second});
          PropertyTargetResolveInfo targetResolve{newTargetValue.first, eastl::move(newProp)};
          if (is<PropertyUniformId>(newProp))
          {
            PropertyReferenceResolveInfo refResolve //
              {reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyUniformId>(newProp)->execution)};
            auto cmp = reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyUniformId>(prop.property)->execution);
            for (auto &&ref : propertyReferenceResolves)
            {
              if (ref.target == cmp)
              {
                refResolve.ref = ref.ref;
                refsToAdd.push_back(refResolve);
                break;
              }
            }
          }
          else if (is<PropertyAlignmentId>(newProp))
          {
            PropertyReferenceResolveInfo refResolve //
              {reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyAlignmentId>(newProp)->alignment)};
            auto cmp = reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyAlignmentId>(prop.property)->alignment);
            for (auto &&ref : propertyReferenceResolves)
            {
              if (ref.target == cmp)
              {
                refResolve.ref = ref.ref;
                refsToAdd.push_back(refResolve);
                break;
              }
            }
          }
          else if (is<PropertyMaxByteOffsetId>(newProp))
          {
            PropertyReferenceResolveInfo refResolve //
              {reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyMaxByteOffsetId>(newProp)->maxByteOffset)};
            auto cmp = reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyMaxByteOffsetId>(prop.property)->maxByteOffset);
            for (auto &&ref : propertyReferenceResolves)
            {
              if (ref.target == cmp)
              {
                refResolve.ref = ref.ref;
                refsToAdd.push_back(refResolve);
                break;
              }
            }
          }
          else if (is<PropertyCounterBuffer>(newProp))
          {
            PropertyReferenceResolveInfo refResolve //
              {reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyCounterBuffer>(newProp)->counterBuffer)};
            auto cmp = reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyCounterBuffer>(prop.property)->counterBuffer);
            for (auto &&ref : propertyReferenceResolves)
            {
              if (ref.target == cmp)
              {
                refResolve.ref = ref.ref;
                refsToAdd.push_back(refResolve);
                break;
              }
            }
          }
          else if (is<PropertyHlslCounterBufferGOOGLE>(newProp))
          {
            PropertyReferenceResolveInfo refResolve //
              {reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyHlslCounterBufferGOOGLE>(newProp)->counterBuffer)};
            auto cmp = reinterpret_cast<NodePointer<NodeId> *>(&as<PropertyHlslCounterBufferGOOGLE>(prop.property)->counterBuffer);
            for (auto &&ref : propertyReferenceResolves)
            {
              if (ref.target == cmp)
              {
                refResolve.ref = ref.ref;
                refsToAdd.push_back(refResolve);
                break;
              }
            }
          }
          targetsToAdd.push_back(eastl::move(targetResolve));
        }
      }
    }
    propertyReferenceResolves.insert(propertyReferenceResolves.end(), refsToAdd.begin(), refsToAdd.end());
    for (auto &&ta : targetsToAdd)
      propertyTargetResolves.push_back(eastl::move(ta));
  }
  void onVectorExtractDynamic(Op, IdResult id_result, IdResultType id_result_type, IdRef vector, IdRef index)
  {
    auto node = moduleBuilder.newNode<NodeOpVectorExtractDynamic>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector), moduleBuilder.getNode(index));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onVectorInsertDynamic(Op, IdResult id_result, IdResultType id_result_type, IdRef vector, IdRef component, IdRef index)
  {
    auto node = moduleBuilder.newNode<NodeOpVectorInsertDynamic>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector), moduleBuilder.getNode(component), moduleBuilder.getNode(index));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onVectorShuffle(Op, IdResult id_result, IdResultType id_result_type, IdRef vector_1, IdRef vector_2,
    Multiple<LiteralInteger> components)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<LiteralInteger> componentsVar;
    while (!components.empty())
    {
      componentsVar.push_back(components.consume());
    }
    auto node = moduleBuilder.newNode<NodeOpVectorShuffle>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector_1), moduleBuilder.getNode(vector_2), componentsVar.data(), componentsVar.size());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onCompositeConstruct(Op, IdResult id_result, IdResultType id_result_type, Multiple<IdRef> constituents)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<NodePointer<NodeId>> constituentsVar;
    while (!constituents.empty())
    {
      constituentsVar.push_back(moduleBuilder.getNode(constituents.consume()));
    }
    auto node = moduleBuilder.newNode<NodeOpCompositeConstruct>(id_result.value, moduleBuilder.getType(id_result_type),
      constituentsVar.data(), constituentsVar.size());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onCompositeExtract(Op, IdResult id_result, IdResultType id_result_type, IdRef composite, Multiple<LiteralInteger> indexes)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<LiteralInteger> indexesVar;
    while (!indexes.empty())
    {
      indexesVar.push_back(indexes.consume());
    }
    auto node = moduleBuilder.newNode<NodeOpCompositeExtract>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(composite), indexesVar.data(), indexesVar.size());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onCompositeInsert(Op, IdResult id_result, IdResultType id_result_type, IdRef object, IdRef composite,
    Multiple<LiteralInteger> indexes)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<LiteralInteger> indexesVar;
    while (!indexes.empty())
    {
      indexesVar.push_back(indexes.consume());
    }
    auto node = moduleBuilder.newNode<NodeOpCompositeInsert>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(object), moduleBuilder.getNode(composite), indexesVar.data(), indexesVar.size());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onCopyObject(Op, IdResult id_result, IdResultType id_result_type, IdRef operand)
  {
    auto node =
      moduleBuilder.newNode<NodeOpCopyObject>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(operand));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onTranspose(Op, IdResult id_result, IdResultType id_result_type, IdRef matrix)
  {
    auto node =
      moduleBuilder.newNode<NodeOpTranspose>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(matrix));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSampledImage(Op, IdResult id_result, IdResultType id_result_type, IdRef image, IdRef sampler)
  {
    auto node = moduleBuilder.newNode<NodeOpSampledImage>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image), moduleBuilder.getNode(sampler));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSampleImplicitLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate,
    Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageSampleImplicitLod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), image_operandsVal, image_operands_Bias_first,
      image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second, image_operands_ConstOffset_first,
      image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first, image_operands_MinLod_first,
      image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first, image_operands_MakeTexelVisible_first,
      image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSampleExplicitLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate,
    TypeTraits<ImageOperandsMask>::ReadType image_operands)
  {
    ImageOperandsMask image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    image_operandsVal = image_operands.value;
    if ((image_operands.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
    {
      image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Bias.first));
    }
    if ((image_operands.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
    {
      image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Lod.first));
    }
    if ((image_operands.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
    {
      image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Grad.first));
      image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Grad.second));
    }
    if ((image_operands.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
    {
      image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.ConstOffset.first));
    }
    if ((image_operands.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
    {
      image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Offset.first));
    }
    if ((image_operands.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
    {
      image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.ConstOffsets.first));
    }
    if ((image_operands.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
    {
      image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Sample.first));
    }
    if ((image_operands.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
    {
      image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.MinLod.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelAvailable_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelAvailable.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelAvailableKHR_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelAvailableKHR.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelVisible_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelVisible.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelVisibleKHR_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelVisibleKHR.first));
    }
    if ((image_operands.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
    {
      image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Offsets.first));
    }
    auto node = moduleBuilder.newNode<NodeOpImageSampleExplicitLod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), image_operandsVal, image_operands_Bias_first,
      image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second, image_operands_ConstOffset_first,
      image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first, image_operands_MinLod_first,
      image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first, image_operands_MakeTexelVisible_first,
      image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSampleDrefImplicitLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate,
    IdRef d_ref, Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageSampleDrefImplicitLod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), moduleBuilder.getNode(d_ref), image_operandsVal,
      image_operands_Bias_first, image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second,
      image_operands_ConstOffset_first, image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first,
      image_operands_MinLod_first, image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first,
      image_operands_MakeTexelVisible_first, image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSampleDrefExplicitLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate,
    IdRef d_ref, TypeTraits<ImageOperandsMask>::ReadType image_operands)
  {
    ImageOperandsMask image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    image_operandsVal = image_operands.value;
    if ((image_operands.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
    {
      image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Bias.first));
    }
    if ((image_operands.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
    {
      image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Lod.first));
    }
    if ((image_operands.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
    {
      image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Grad.first));
      image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Grad.second));
    }
    if ((image_operands.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
    {
      image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.ConstOffset.first));
    }
    if ((image_operands.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
    {
      image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Offset.first));
    }
    if ((image_operands.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
    {
      image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.ConstOffsets.first));
    }
    if ((image_operands.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
    {
      image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Sample.first));
    }
    if ((image_operands.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
    {
      image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.MinLod.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelAvailable_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelAvailable.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelAvailableKHR_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelAvailableKHR.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelVisible_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelVisible.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelVisibleKHR_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelVisibleKHR.first));
    }
    if ((image_operands.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
    {
      image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Offsets.first));
    }
    auto node = moduleBuilder.newNode<NodeOpImageSampleDrefExplicitLod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), moduleBuilder.getNode(d_ref), image_operandsVal,
      image_operands_Bias_first, image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second,
      image_operands_ConstOffset_first, image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first,
      image_operands_MinLod_first, image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first,
      image_operands_MakeTexelVisible_first, image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSampleProjImplicitLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate,
    Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageSampleProjImplicitLod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), image_operandsVal, image_operands_Bias_first,
      image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second, image_operands_ConstOffset_first,
      image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first, image_operands_MinLod_first,
      image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first, image_operands_MakeTexelVisible_first,
      image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSampleProjExplicitLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate,
    TypeTraits<ImageOperandsMask>::ReadType image_operands)
  {
    ImageOperandsMask image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    image_operandsVal = image_operands.value;
    if ((image_operands.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
    {
      image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Bias.first));
    }
    if ((image_operands.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
    {
      image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Lod.first));
    }
    if ((image_operands.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
    {
      image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Grad.first));
      image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Grad.second));
    }
    if ((image_operands.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
    {
      image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.ConstOffset.first));
    }
    if ((image_operands.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
    {
      image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Offset.first));
    }
    if ((image_operands.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
    {
      image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.ConstOffsets.first));
    }
    if ((image_operands.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
    {
      image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Sample.first));
    }
    if ((image_operands.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
    {
      image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.MinLod.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelAvailable_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelAvailable.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelAvailableKHR_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelAvailableKHR.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelVisible_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelVisible.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelVisibleKHR_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelVisibleKHR.first));
    }
    if ((image_operands.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
    {
      image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Offsets.first));
    }
    auto node = moduleBuilder.newNode<NodeOpImageSampleProjExplicitLod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), image_operandsVal, image_operands_Bias_first,
      image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second, image_operands_ConstOffset_first,
      image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first, image_operands_MinLod_first,
      image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first, image_operands_MakeTexelVisible_first,
      image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSampleProjDrefImplicitLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate,
    IdRef d_ref, Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageSampleProjDrefImplicitLod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), moduleBuilder.getNode(d_ref), image_operandsVal,
      image_operands_Bias_first, image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second,
      image_operands_ConstOffset_first, image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first,
      image_operands_MinLod_first, image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first,
      image_operands_MakeTexelVisible_first, image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSampleProjDrefExplicitLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate,
    IdRef d_ref, TypeTraits<ImageOperandsMask>::ReadType image_operands)
  {
    ImageOperandsMask image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    image_operandsVal = image_operands.value;
    if ((image_operands.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
    {
      image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Bias.first));
    }
    if ((image_operands.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
    {
      image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Lod.first));
    }
    if ((image_operands.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
    {
      image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Grad.first));
      image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Grad.second));
    }
    if ((image_operands.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
    {
      image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.ConstOffset.first));
    }
    if ((image_operands.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
    {
      image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Offset.first));
    }
    if ((image_operands.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
    {
      image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.ConstOffsets.first));
    }
    if ((image_operands.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
    {
      image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Sample.first));
    }
    if ((image_operands.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
    {
      image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.MinLod.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelAvailable_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelAvailable.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelAvailableKHR_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelAvailableKHR.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelVisible_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelVisible.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelVisibleKHR_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelVisibleKHR.first));
    }
    if ((image_operands.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
    {
      image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Offsets.first));
    }
    auto node = moduleBuilder.newNode<NodeOpImageSampleProjDrefExplicitLod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), moduleBuilder.getNode(d_ref), image_operandsVal,
      image_operands_Bias_first, image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second,
      image_operands_ConstOffset_first, image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first,
      image_operands_MinLod_first, image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first,
      image_operands_MakeTexelVisible_first, image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageFetch(Op, IdResult id_result, IdResultType id_result_type, IdRef image, IdRef coordinate,
    Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageFetch>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image), moduleBuilder.getNode(coordinate), image_operandsVal, image_operands_Bias_first,
      image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second, image_operands_ConstOffset_first,
      image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first, image_operands_MinLod_first,
      image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first, image_operands_MakeTexelVisible_first,
      image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageGather(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate, IdRef component,
    Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageGather>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), moduleBuilder.getNode(component), image_operandsVal,
      image_operands_Bias_first, image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second,
      image_operands_ConstOffset_first, image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first,
      image_operands_MinLod_first, image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first,
      image_operands_MakeTexelVisible_first, image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageDrefGather(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate, IdRef d_ref,
    Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageDrefGather>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), moduleBuilder.getNode(d_ref), image_operandsVal,
      image_operands_Bias_first, image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second,
      image_operands_ConstOffset_first, image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first,
      image_operands_MinLod_first, image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first,
      image_operands_MakeTexelVisible_first, image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageRead(Op, IdResult id_result, IdResultType id_result_type, IdRef image, IdRef coordinate,
    Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageRead>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image), moduleBuilder.getNode(coordinate), image_operandsVal, image_operands_Bias_first,
      image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second, image_operands_ConstOffset_first,
      image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first, image_operands_MinLod_first,
      image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first, image_operands_MakeTexelVisible_first,
      image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageWrite(Op, IdRef image, IdRef coordinate, IdRef texel, Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageWrite>(moduleBuilder.getNode(image), moduleBuilder.getNode(coordinate),
      moduleBuilder.getNode(texel), image_operandsVal, image_operands_Bias_first, image_operands_Lod_first, image_operands_Grad_first,
      image_operands_Grad_second, image_operands_ConstOffset_first, image_operands_Offset_first, image_operands_ConstOffsets_first,
      image_operands_Sample_first, image_operands_MinLod_first, image_operands_MakeTexelAvailable_first,
      image_operands_MakeTexelAvailableKHR_first, image_operands_MakeTexelVisible_first, image_operands_MakeTexelVisibleKHR_first,
      image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImage(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image)
  {
    auto node =
      moduleBuilder.newNode<NodeOpImage>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(sampled_image));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageQueryFormat(Op, IdResult id_result, IdResultType id_result_type, IdRef image)
  {
    auto node = moduleBuilder.newNode<NodeOpImageQueryFormat>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageQueryOrder(Op, IdResult id_result, IdResultType id_result_type, IdRef image)
  {
    auto node = moduleBuilder.newNode<NodeOpImageQueryOrder>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageQuerySizeLod(Op, IdResult id_result, IdResultType id_result_type, IdRef image, IdRef level_of_detail)
  {
    auto node = moduleBuilder.newNode<NodeOpImageQuerySizeLod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image), moduleBuilder.getNode(level_of_detail));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageQuerySize(Op, IdResult id_result, IdResultType id_result_type, IdRef image)
  {
    auto node = moduleBuilder.newNode<NodeOpImageQuerySize>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageQueryLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate)
  {
    auto node = moduleBuilder.newNode<NodeOpImageQueryLod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageQueryLevels(Op, IdResult id_result, IdResultType id_result_type, IdRef image)
  {
    auto node = moduleBuilder.newNode<NodeOpImageQueryLevels>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageQuerySamples(Op, IdResult id_result, IdResultType id_result_type, IdRef image)
  {
    auto node = moduleBuilder.newNode<NodeOpImageQuerySamples>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConvertFToU(Op, IdResult id_result, IdResultType id_result_type, IdRef float_value)
  {
    auto node = moduleBuilder.newNode<NodeOpConvertFToU>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(float_value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConvertFToS(Op, IdResult id_result, IdResultType id_result_type, IdRef float_value)
  {
    auto node = moduleBuilder.newNode<NodeOpConvertFToS>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(float_value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConvertSToF(Op, IdResult id_result, IdResultType id_result_type, IdRef signed_value)
  {
    auto node = moduleBuilder.newNode<NodeOpConvertSToF>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(signed_value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConvertUToF(Op, IdResult id_result, IdResultType id_result_type, IdRef unsigned_value)
  {
    auto node = moduleBuilder.newNode<NodeOpConvertUToF>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(unsigned_value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUConvert(Op, IdResult id_result, IdResultType id_result_type, IdRef unsigned_value)
  {
    auto node = moduleBuilder.newNode<NodeOpUConvert>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(unsigned_value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSConvert(Op, IdResult id_result, IdResultType id_result_type, IdRef signed_value)
  {
    auto node = moduleBuilder.newNode<NodeOpSConvert>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(signed_value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFConvert(Op, IdResult id_result, IdResultType id_result_type, IdRef float_value)
  {
    auto node = moduleBuilder.newNode<NodeOpFConvert>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(float_value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onQuantizeToF16(Op, IdResult id_result, IdResultType id_result_type, IdRef value)
  {
    auto node =
      moduleBuilder.newNode<NodeOpQuantizeToF16>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConvertPtrToU(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer)
  {
    auto node = moduleBuilder.newNode<NodeOpConvertPtrToU>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSatConvertSToU(Op, IdResult id_result, IdResultType id_result_type, IdRef signed_value)
  {
    auto node = moduleBuilder.newNode<NodeOpSatConvertSToU>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(signed_value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSatConvertUToS(Op, IdResult id_result, IdResultType id_result_type, IdRef unsigned_value)
  {
    auto node = moduleBuilder.newNode<NodeOpSatConvertUToS>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(unsigned_value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConvertUToPtr(Op, IdResult id_result, IdResultType id_result_type, IdRef integer_value)
  {
    auto node = moduleBuilder.newNode<NodeOpConvertUToPtr>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(integer_value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onPtrCastToGeneric(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer)
  {
    auto node = moduleBuilder.newNode<NodeOpPtrCastToGeneric>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGenericCastToPtr(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer)
  {
    auto node = moduleBuilder.newNode<NodeOpGenericCastToPtr>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGenericCastToPtrExplicit(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, StorageClass storage)
  {
    auto node = moduleBuilder.newNode<NodeOpGenericCastToPtrExplicit>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), storage);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onBitcast(Op, IdResult id_result, IdResultType id_result_type, IdRef operand)
  {
    auto node =
      moduleBuilder.newNode<NodeOpBitcast>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(operand));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSNegate(Op, IdResult id_result, IdResultType id_result_type, IdRef operand)
  {
    auto node =
      moduleBuilder.newNode<NodeOpSNegate>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(operand));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFNegate(Op, IdResult id_result, IdResultType id_result_type, IdRef operand)
  {
    auto node =
      moduleBuilder.newNode<NodeOpFNegate>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(operand));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIAdd(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpIAdd>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFAdd(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFAdd>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onISub(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpISub>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFSub(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFSub>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIMul(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpIMul>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFMul(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFMul>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUDiv(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpUDiv>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSDiv(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpSDiv>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFDiv(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFDiv>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUMod(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpUMod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSRem(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpSRem>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSMod(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpSMod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFRem(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFRem>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFMod(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFMod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onVectorTimesScalar(Op, IdResult id_result, IdResultType id_result_type, IdRef vector, IdRef scalar)
  {
    auto node = moduleBuilder.newNode<NodeOpVectorTimesScalar>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector), moduleBuilder.getNode(scalar));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onMatrixTimesScalar(Op, IdResult id_result, IdResultType id_result_type, IdRef matrix, IdRef scalar)
  {
    auto node = moduleBuilder.newNode<NodeOpMatrixTimesScalar>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(matrix), moduleBuilder.getNode(scalar));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onVectorTimesMatrix(Op, IdResult id_result, IdResultType id_result_type, IdRef vector, IdRef matrix)
  {
    auto node = moduleBuilder.newNode<NodeOpVectorTimesMatrix>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector), moduleBuilder.getNode(matrix));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onMatrixTimesVector(Op, IdResult id_result, IdResultType id_result_type, IdRef matrix, IdRef vector)
  {
    auto node = moduleBuilder.newNode<NodeOpMatrixTimesVector>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(matrix), moduleBuilder.getNode(vector));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onMatrixTimesMatrix(Op, IdResult id_result, IdResultType id_result_type, IdRef left_matrix, IdRef right_matrix)
  {
    auto node = moduleBuilder.newNode<NodeOpMatrixTimesMatrix>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(left_matrix), moduleBuilder.getNode(right_matrix));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onOuterProduct(Op, IdResult id_result, IdResultType id_result_type, IdRef vector_1, IdRef vector_2)
  {
    auto node = moduleBuilder.newNode<NodeOpOuterProduct>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector_1), moduleBuilder.getNode(vector_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onDot(Op, IdResult id_result, IdResultType id_result_type, IdRef vector_1, IdRef vector_2)
  {
    auto node = moduleBuilder.newNode<NodeOpDot>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector_1), moduleBuilder.getNode(vector_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIAddCarry(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpIAddCarry>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onISubBorrow(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpISubBorrow>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUMulExtended(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpUMulExtended>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSMulExtended(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpSMulExtended>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAny(Op, IdResult id_result, IdResultType id_result_type, IdRef vector)
  {
    auto node =
      moduleBuilder.newNode<NodeOpAny>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(vector));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAll(Op, IdResult id_result, IdResultType id_result_type, IdRef vector)
  {
    auto node =
      moduleBuilder.newNode<NodeOpAll>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(vector));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIsNan(Op, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpIsNan>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIsInf(Op, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpIsInf>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIsFinite(Op, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpIsFinite>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIsNormal(Op, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpIsNormal>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSignBitSet(Op, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpSignBitSet>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onLessOrGreater(Op, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y)
  {
    auto node = moduleBuilder.newNode<NodeOpLessOrGreater>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onOrdered(Op, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y)
  {
    auto node = moduleBuilder.newNode<NodeOpOrdered>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x),
      moduleBuilder.getNode(y));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUnordered(Op, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y)
  {
    auto node = moduleBuilder.newNode<NodeOpUnordered>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onLogicalEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpLogicalEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onLogicalNotEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpLogicalNotEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onLogicalOr(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpLogicalOr>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onLogicalAnd(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpLogicalAnd>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onLogicalNot(Op, IdResult id_result, IdResultType id_result_type, IdRef operand)
  {
    auto node =
      moduleBuilder.newNode<NodeOpLogicalNot>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(operand));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSelect(Op, IdResult id_result, IdResultType id_result_type, IdRef condition, IdRef object_1, IdRef object_2)
  {
    auto node = moduleBuilder.newNode<NodeOpSelect>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(condition), moduleBuilder.getNode(object_1), moduleBuilder.getNode(object_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpIEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onINotEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpINotEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUGreaterThan(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpUGreaterThan>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSGreaterThan(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpSGreaterThan>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUGreaterThanEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpUGreaterThanEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSGreaterThanEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpSGreaterThanEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onULessThan(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpULessThan>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSLessThan(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpSLessThan>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onULessThanEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpULessThanEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSLessThanEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpSLessThanEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFOrdEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFOrdEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFUnordEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFUnordEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFOrdNotEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFOrdNotEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFUnordNotEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFUnordNotEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFOrdLessThan(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFOrdLessThan>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFUnordLessThan(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFUnordLessThan>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFOrdGreaterThan(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFOrdGreaterThan>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFUnordGreaterThan(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFUnordGreaterThan>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFOrdLessThanEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFOrdLessThanEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFUnordLessThanEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFUnordLessThanEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFOrdGreaterThanEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFOrdGreaterThanEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFUnordGreaterThanEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpFUnordGreaterThanEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onShiftRightLogical(Op, IdResult id_result, IdResultType id_result_type, IdRef base, IdRef shift)
  {
    auto node = moduleBuilder.newNode<NodeOpShiftRightLogical>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(base), moduleBuilder.getNode(shift));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onShiftRightArithmetic(Op, IdResult id_result, IdResultType id_result_type, IdRef base, IdRef shift)
  {
    auto node = moduleBuilder.newNode<NodeOpShiftRightArithmetic>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(base), moduleBuilder.getNode(shift));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onShiftLeftLogical(Op, IdResult id_result, IdResultType id_result_type, IdRef base, IdRef shift)
  {
    auto node = moduleBuilder.newNode<NodeOpShiftLeftLogical>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(base), moduleBuilder.getNode(shift));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onBitwiseOr(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpBitwiseOr>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onBitwiseXor(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpBitwiseXor>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onBitwiseAnd(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpBitwiseAnd>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onNot(Op, IdResult id_result, IdResultType id_result_type, IdRef operand)
  {
    auto node =
      moduleBuilder.newNode<NodeOpNot>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(operand));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onBitFieldInsert(Op, IdResult id_result, IdResultType id_result_type, IdRef base, IdRef insert, IdRef offset, IdRef count)
  {
    auto node = moduleBuilder.newNode<NodeOpBitFieldInsert>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(base), moduleBuilder.getNode(insert), moduleBuilder.getNode(offset), moduleBuilder.getNode(count));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onBitFieldSExtract(Op, IdResult id_result, IdResultType id_result_type, IdRef base, IdRef offset, IdRef count)
  {
    auto node = moduleBuilder.newNode<NodeOpBitFieldSExtract>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(base), moduleBuilder.getNode(offset), moduleBuilder.getNode(count));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onBitFieldUExtract(Op, IdResult id_result, IdResultType id_result_type, IdRef base, IdRef offset, IdRef count)
  {
    auto node = moduleBuilder.newNode<NodeOpBitFieldUExtract>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(base), moduleBuilder.getNode(offset), moduleBuilder.getNode(count));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onBitReverse(Op, IdResult id_result, IdResultType id_result_type, IdRef base)
  {
    auto node =
      moduleBuilder.newNode<NodeOpBitReverse>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(base));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onBitCount(Op, IdResult id_result, IdResultType id_result_type, IdRef base)
  {
    auto node =
      moduleBuilder.newNode<NodeOpBitCount>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(base));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onDPdx(Op, IdResult id_result, IdResultType id_result_type, IdRef p)
  {
    auto node = moduleBuilder.newNode<NodeOpDPdx>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(p));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onDPdy(Op, IdResult id_result, IdResultType id_result_type, IdRef p)
  {
    auto node = moduleBuilder.newNode<NodeOpDPdy>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(p));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFwidth(Op, IdResult id_result, IdResultType id_result_type, IdRef p)
  {
    auto node = moduleBuilder.newNode<NodeOpFwidth>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(p));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onDPdxFine(Op, IdResult id_result, IdResultType id_result_type, IdRef p)
  {
    auto node =
      moduleBuilder.newNode<NodeOpDPdxFine>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(p));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onDPdyFine(Op, IdResult id_result, IdResultType id_result_type, IdRef p)
  {
    auto node =
      moduleBuilder.newNode<NodeOpDPdyFine>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(p));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFwidthFine(Op, IdResult id_result, IdResultType id_result_type, IdRef p)
  {
    auto node =
      moduleBuilder.newNode<NodeOpFwidthFine>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(p));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onDPdxCoarse(Op, IdResult id_result, IdResultType id_result_type, IdRef p)
  {
    auto node =
      moduleBuilder.newNode<NodeOpDPdxCoarse>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(p));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onDPdyCoarse(Op, IdResult id_result, IdResultType id_result_type, IdRef p)
  {
    auto node =
      moduleBuilder.newNode<NodeOpDPdyCoarse>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(p));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFwidthCoarse(Op, IdResult id_result, IdResultType id_result_type, IdRef p)
  {
    auto node =
      moduleBuilder.newNode<NodeOpFwidthCoarse>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(p));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onEmitVertex(Op)
  {
    auto node = moduleBuilder.newNode<NodeOpEmitVertex>();
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onEndPrimitive(Op)
  {
    auto node = moduleBuilder.newNode<NodeOpEndPrimitive>();
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onEmitStreamVertex(Op, IdRef stream)
  {
    auto node = moduleBuilder.newNode<NodeOpEmitStreamVertex>(moduleBuilder.getNode(stream));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onEndStreamPrimitive(Op, IdRef stream)
  {
    auto node = moduleBuilder.newNode<NodeOpEndStreamPrimitive>(moduleBuilder.getNode(stream));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onControlBarrier(Op, IdScope execution, IdScope memory, IdMemorySemantics semantics)
  {
    auto node = moduleBuilder.newNode<NodeOpControlBarrier>(moduleBuilder.getNode(execution), moduleBuilder.getNode(memory),
      moduleBuilder.getNode(semantics));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onMemoryBarrier(Op, IdScope memory, IdMemorySemantics semantics)
  {
    auto node = moduleBuilder.newNode<NodeOpMemoryBarrier>(moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicLoad(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory, IdMemorySemantics semantics)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicLoad>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicStore(Op, IdRef pointer, IdScope memory, IdMemorySemantics semantics, IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicStore>(moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory),
      moduleBuilder.getNode(semantics), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicExchange(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory,
    IdMemorySemantics semantics, IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicExchange>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicCompareExchange(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory,
    IdMemorySemantics equal, IdMemorySemantics unequal, IdRef value, IdRef comparator)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicCompareExchange>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(equal), moduleBuilder.getNode(unequal),
      moduleBuilder.getNode(value), moduleBuilder.getNode(comparator));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicCompareExchangeWeak(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory,
    IdMemorySemantics equal, IdMemorySemantics unequal, IdRef value, IdRef comparator)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicCompareExchangeWeak>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(equal), moduleBuilder.getNode(unequal),
      moduleBuilder.getNode(value), moduleBuilder.getNode(comparator));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicIIncrement(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory,
    IdMemorySemantics semantics)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicIIncrement>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicIDecrement(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory,
    IdMemorySemantics semantics)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicIDecrement>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicIAdd(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory, IdMemorySemantics semantics,
    IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicIAdd>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicISub(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory, IdMemorySemantics semantics,
    IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicISub>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicSMin(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory, IdMemorySemantics semantics,
    IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicSMin>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicUMin(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory, IdMemorySemantics semantics,
    IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicUMin>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicSMax(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory, IdMemorySemantics semantics,
    IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicSMax>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicUMax(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory, IdMemorySemantics semantics,
    IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicUMax>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicAnd(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory, IdMemorySemantics semantics,
    IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicAnd>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicOr(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory, IdMemorySemantics semantics,
    IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicOr>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicXor(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory, IdMemorySemantics semantics,
    IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicXor>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onPhi(Op, IdResult id_result, IdResultType id_result_type, Multiple<PairIdRefIdRef> variable_parent)
  {
    auto node = moduleBuilder.newNode<NodeOpPhi>(id_result.value, moduleBuilder.getType(id_result_type), nullptr, 0);
    node->sources.resize(variable_parent.count);
    for (int i = 0; variable_parent.count; ++i)
    {
      auto sourceInfo = variable_parent.consume();
      auto &ref = node->sources[i];
      PropertyReferenceResolveInfo info;
      info.target = reinterpret_cast<NodePointer<NodeId> *>(&ref.source);
      info.ref = sourceInfo.first;
      propertyReferenceResolves.push_back(info);
      info.target = reinterpret_cast<NodePointer<NodeId> *>(&ref.block);
      info.ref = sourceInfo.second;
      propertyReferenceResolves.push_back(info);
    }
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onLoopMerge(Op, IdRef merge_block, IdRef continue_target, TypeTraits<LoopControlMask>::ReadType loop_control)
  {
    LoopMergeDef newLMerge;
    newLMerge.target = currentBlock;
    newLMerge.mergeBlock = merge_block;
    newLMerge.continueBlock = continue_target;
    newLMerge.controlMask = loop_control.value;
    newLMerge.dependencyLength = loop_control.data.DependencyLength.first;
    loopMerges.push_back(newLMerge);
  }
  void onSelectionMerge(Op, IdRef merge_block, SelectionControlMask selection_control)
  {
    SelectionMergeDef newSMerge;
    newSMerge.target = currentBlock;
    newSMerge.mergeBlock = merge_block;
    newSMerge.controlMask = selection_control;
    selectionMerges.push_back(newSMerge);
  }
  void onLabel(Op, IdResult id_result)
  {
    currentBlock = moduleBuilder.newNode<NodeOpLabel>(id_result.value);
    currentFunction->body.push_back(currentBlock);
  }
  void onBranch(Op, IdRef target_label)
  {
    auto node = moduleBuilder.newNode<NodeOpBranch>(NodePointer<NodeId>{});
    PropertyReferenceResolveInfo info;
    info.target = reinterpret_cast<NodePointer<NodeId> *>(&node->targetLabel);
    info.ref = target_label;
    propertyReferenceResolves.push_back(info);
    currentBlock->exit = node;
    currentBlock.reset();
  }
  void onBranchConditional(Op, IdRef condition, IdRef true_label, IdRef false_label, Multiple<LiteralInteger> branch_weights)
  {
    eastl::vector<LiteralInteger> branch_weightsVar;
    while (!branch_weights.empty())
    {
      branch_weightsVar.push_back(branch_weights.consume());
    }
    auto node = moduleBuilder.newNode<NodeOpBranchConditional>(moduleBuilder.getNode(condition), NodePointer<NodeId>{},
      NodePointer<NodeId>{}, branch_weightsVar.data(), branch_weightsVar.size());
    PropertyReferenceResolveInfo info;
    info.target = reinterpret_cast<NodePointer<NodeId> *>(&node->trueLabel);
    info.ref = true_label;
    propertyReferenceResolves.push_back(info);
    info.target = reinterpret_cast<NodePointer<NodeId> *>(&node->falseLabel);
    info.ref = false_label;
    propertyReferenceResolves.push_back(info);
    currentBlock->exit = node;
    currentBlock.reset();
  }
  void onSwitch(Op, IdRef selector, IdRef _default, Multiple<PairLiteralIntegerIdRef> target)
  {
    auto node = moduleBuilder.newNode<NodeOpSwitch>(moduleBuilder.getNode(selector), NodePointer<NodeId>{}, nullptr, 0);
    PropertyReferenceResolveInfo info;
    info.target = reinterpret_cast<NodePointer<NodeId> *>(&node->defaultBlock);
    info.ref = _default;
    propertyReferenceResolves.push_back(info);
    node->cases.resize(target.count);
    for (int i = 0; target.count; ++i)
    {
      auto sourceInfo = target.consume();
      auto &ref = node->cases[i];
      ref.value = sourceInfo.first;
      info.target = reinterpret_cast<NodePointer<NodeId> *>(&ref.block);
      info.ref = sourceInfo.second;
      propertyReferenceResolves.push_back(info);
    }
    currentBlock->exit = node;
    currentBlock.reset();
  }
  void onKill(Op)
  {
    auto node = moduleBuilder.newNode<NodeOpKill>();
    currentBlock->exit = node;
    currentBlock.reset();
  }
  void onReturn(Op)
  {
    auto node = moduleBuilder.newNode<NodeOpReturn>();
    currentBlock->exit = node;
    currentBlock.reset();
  }
  void onReturnValue(Op, IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpReturnValue>(moduleBuilder.getNode(value));
    currentBlock->exit = node;
    currentBlock.reset();
  }
  void onUnreachable(Op)
  {
    auto node = moduleBuilder.newNode<NodeOpUnreachable>();
    currentBlock->exit = node;
    currentBlock.reset();
  }
  void onLifetimeStart(Op, IdRef pointer, LiteralInteger size)
  {
    auto node = moduleBuilder.newNode<NodeOpLifetimeStart>(moduleBuilder.getNode(pointer), size);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onLifetimeStop(Op, IdRef pointer, LiteralInteger size)
  {
    auto node = moduleBuilder.newNode<NodeOpLifetimeStop>(moduleBuilder.getNode(pointer), size);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupAsyncCopy(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef destination, IdRef source,
    IdRef num_elements, IdRef stride, IdRef event)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupAsyncCopy>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(destination), moduleBuilder.getNode(source),
      moduleBuilder.getNode(num_elements), moduleBuilder.getNode(stride), moduleBuilder.getNode(event));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupWaitEvents(Op, IdScope execution, IdRef num_events, IdRef events_list)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupWaitEvents>(moduleBuilder.getNode(execution), moduleBuilder.getNode(num_events),
      moduleBuilder.getNode(events_list));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupAll(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef predicate)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupAll>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(predicate));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupAny(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef predicate)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupAny>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(predicate));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupBroadcast(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef value, IdRef local_id)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupBroadcast>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(value), moduleBuilder.getNode(local_id));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupIAdd(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupIAdd>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupFAdd(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupFAdd>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupFMin(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupFMin>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupUMin(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupUMin>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupSMin(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupSMin>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupFMax(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupFMax>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupUMax(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupUMax>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupSMax(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupSMax>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onReadPipe(Op, IdResult id_result, IdResultType id_result_type, IdRef pipe, IdRef pointer, IdRef packet_size,
    IdRef packet_alignment)
  {
    auto node =
      moduleBuilder.newNode<NodeOpReadPipe>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(pipe),
        moduleBuilder.getNode(pointer), moduleBuilder.getNode(packet_size), moduleBuilder.getNode(packet_alignment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onWritePipe(Op, IdResult id_result, IdResultType id_result_type, IdRef pipe, IdRef pointer, IdRef packet_size,
    IdRef packet_alignment)
  {
    auto node =
      moduleBuilder.newNode<NodeOpWritePipe>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(pipe),
        moduleBuilder.getNode(pointer), moduleBuilder.getNode(packet_size), moduleBuilder.getNode(packet_alignment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onReservedReadPipe(Op, IdResult id_result, IdResultType id_result_type, IdRef pipe, IdRef reserve_id, IdRef index,
    IdRef pointer, IdRef packet_size, IdRef packet_alignment)
  {
    auto node = moduleBuilder.newNode<NodeOpReservedReadPipe>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pipe), moduleBuilder.getNode(reserve_id), moduleBuilder.getNode(index), moduleBuilder.getNode(pointer),
      moduleBuilder.getNode(packet_size), moduleBuilder.getNode(packet_alignment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onReservedWritePipe(Op, IdResult id_result, IdResultType id_result_type, IdRef pipe, IdRef reserve_id, IdRef index,
    IdRef pointer, IdRef packet_size, IdRef packet_alignment)
  {
    auto node = moduleBuilder.newNode<NodeOpReservedWritePipe>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pipe), moduleBuilder.getNode(reserve_id), moduleBuilder.getNode(index), moduleBuilder.getNode(pointer),
      moduleBuilder.getNode(packet_size), moduleBuilder.getNode(packet_alignment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onReserveReadPipePackets(Op, IdResult id_result, IdResultType id_result_type, IdRef pipe, IdRef num_packets, IdRef packet_size,
    IdRef packet_alignment)
  {
    auto node = moduleBuilder.newNode<NodeOpReserveReadPipePackets>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pipe), moduleBuilder.getNode(num_packets), moduleBuilder.getNode(packet_size),
      moduleBuilder.getNode(packet_alignment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onReserveWritePipePackets(Op, IdResult id_result, IdResultType id_result_type, IdRef pipe, IdRef num_packets, IdRef packet_size,
    IdRef packet_alignment)
  {
    auto node = moduleBuilder.newNode<NodeOpReserveWritePipePackets>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pipe), moduleBuilder.getNode(num_packets), moduleBuilder.getNode(packet_size),
      moduleBuilder.getNode(packet_alignment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onCommitReadPipe(Op, IdRef pipe, IdRef reserve_id, IdRef packet_size, IdRef packet_alignment)
  {
    auto node = moduleBuilder.newNode<NodeOpCommitReadPipe>(moduleBuilder.getNode(pipe), moduleBuilder.getNode(reserve_id),
      moduleBuilder.getNode(packet_size), moduleBuilder.getNode(packet_alignment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onCommitWritePipe(Op, IdRef pipe, IdRef reserve_id, IdRef packet_size, IdRef packet_alignment)
  {
    auto node = moduleBuilder.newNode<NodeOpCommitWritePipe>(moduleBuilder.getNode(pipe), moduleBuilder.getNode(reserve_id),
      moduleBuilder.getNode(packet_size), moduleBuilder.getNode(packet_alignment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIsValidReserveId(Op, IdResult id_result, IdResultType id_result_type, IdRef reserve_id)
  {
    auto node = moduleBuilder.newNode<NodeOpIsValidReserveId>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(reserve_id));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGetNumPipePackets(Op, IdResult id_result, IdResultType id_result_type, IdRef pipe, IdRef packet_size, IdRef packet_alignment)
  {
    auto node = moduleBuilder.newNode<NodeOpGetNumPipePackets>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pipe), moduleBuilder.getNode(packet_size), moduleBuilder.getNode(packet_alignment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGetMaxPipePackets(Op, IdResult id_result, IdResultType id_result_type, IdRef pipe, IdRef packet_size, IdRef packet_alignment)
  {
    auto node = moduleBuilder.newNode<NodeOpGetMaxPipePackets>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pipe), moduleBuilder.getNode(packet_size), moduleBuilder.getNode(packet_alignment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupReserveReadPipePackets(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef pipe,
    IdRef num_packets, IdRef packet_size, IdRef packet_alignment)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupReserveReadPipePackets>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(pipe), moduleBuilder.getNode(num_packets),
      moduleBuilder.getNode(packet_size), moduleBuilder.getNode(packet_alignment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupReserveWritePipePackets(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef pipe,
    IdRef num_packets, IdRef packet_size, IdRef packet_alignment)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupReserveWritePipePackets>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(pipe), moduleBuilder.getNode(num_packets),
      moduleBuilder.getNode(packet_size), moduleBuilder.getNode(packet_alignment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupCommitReadPipe(Op, IdScope execution, IdRef pipe, IdRef reserve_id, IdRef packet_size, IdRef packet_alignment)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupCommitReadPipe>(moduleBuilder.getNode(execution), moduleBuilder.getNode(pipe),
      moduleBuilder.getNode(reserve_id), moduleBuilder.getNode(packet_size), moduleBuilder.getNode(packet_alignment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupCommitWritePipe(Op, IdScope execution, IdRef pipe, IdRef reserve_id, IdRef packet_size, IdRef packet_alignment)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupCommitWritePipe>(moduleBuilder.getNode(execution), moduleBuilder.getNode(pipe),
      moduleBuilder.getNode(reserve_id), moduleBuilder.getNode(packet_size), moduleBuilder.getNode(packet_alignment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onEnqueueMarker(Op, IdResult id_result, IdResultType id_result_type, IdRef queue, IdRef num_events, IdRef wait_events,
    IdRef ret_event)
  {
    auto node =
      moduleBuilder.newNode<NodeOpEnqueueMarker>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(queue),
        moduleBuilder.getNode(num_events), moduleBuilder.getNode(wait_events), moduleBuilder.getNode(ret_event));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onEnqueueKernel(Op, IdResult id_result, IdResultType id_result_type, IdRef queue, IdRef flags, IdRef n_d_range,
    IdRef num_events, IdRef wait_events, IdRef ret_event, IdRef invoke, IdRef param, IdRef param_size, IdRef param_align,
    Multiple<IdRef> local_size)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<NodePointer<NodeId>> local_sizeVar;
    while (!local_size.empty())
    {
      local_sizeVar.push_back(moduleBuilder.getNode(local_size.consume()));
    }
    auto node = moduleBuilder.newNode<NodeOpEnqueueKernel>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(queue), moduleBuilder.getNode(flags), moduleBuilder.getNode(n_d_range), moduleBuilder.getNode(num_events),
      moduleBuilder.getNode(wait_events), moduleBuilder.getNode(ret_event), moduleBuilder.getNode(invoke),
      moduleBuilder.getNode(param), moduleBuilder.getNode(param_size), moduleBuilder.getNode(param_align), local_sizeVar.data(),
      local_sizeVar.size());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGetKernelNDrangeSubGroupCount(Op, IdResult id_result, IdResultType id_result_type, IdRef n_d_range, IdRef invoke, IdRef param,
    IdRef param_size, IdRef param_align)
  {
    auto node = moduleBuilder.newNode<NodeOpGetKernelNDrangeSubGroupCount>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(n_d_range), moduleBuilder.getNode(invoke), moduleBuilder.getNode(param), moduleBuilder.getNode(param_size),
      moduleBuilder.getNode(param_align));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGetKernelNDrangeMaxSubGroupSize(Op, IdResult id_result, IdResultType id_result_type, IdRef n_d_range, IdRef invoke,
    IdRef param, IdRef param_size, IdRef param_align)
  {
    auto node = moduleBuilder.newNode<NodeOpGetKernelNDrangeMaxSubGroupSize>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(n_d_range), moduleBuilder.getNode(invoke), moduleBuilder.getNode(param), moduleBuilder.getNode(param_size),
      moduleBuilder.getNode(param_align));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGetKernelWorkGroupSize(Op, IdResult id_result, IdResultType id_result_type, IdRef invoke, IdRef param, IdRef param_size,
    IdRef param_align)
  {
    auto node = moduleBuilder.newNode<NodeOpGetKernelWorkGroupSize>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(invoke), moduleBuilder.getNode(param), moduleBuilder.getNode(param_size),
      moduleBuilder.getNode(param_align));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGetKernelPreferredWorkGroupSizeMultiple(Op, IdResult id_result, IdResultType id_result_type, IdRef invoke, IdRef param,
    IdRef param_size, IdRef param_align)
  {
    auto node = moduleBuilder.newNode<NodeOpGetKernelPreferredWorkGroupSizeMultiple>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(invoke), moduleBuilder.getNode(param),
      moduleBuilder.getNode(param_size), moduleBuilder.getNode(param_align));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRetainEvent(Op, IdRef event)
  {
    auto node = moduleBuilder.newNode<NodeOpRetainEvent>(moduleBuilder.getNode(event));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onReleaseEvent(Op, IdRef event)
  {
    auto node = moduleBuilder.newNode<NodeOpReleaseEvent>(moduleBuilder.getNode(event));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onCreateUserEvent(Op, IdResult id_result, IdResultType id_result_type)
  {
    auto node = moduleBuilder.newNode<NodeOpCreateUserEvent>(id_result.value, moduleBuilder.getType(id_result_type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIsValidEvent(Op, IdResult id_result, IdResultType id_result_type, IdRef event)
  {
    auto node =
      moduleBuilder.newNode<NodeOpIsValidEvent>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(event));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSetUserEventStatus(Op, IdRef event, IdRef status)
  {
    auto node = moduleBuilder.newNode<NodeOpSetUserEventStatus>(moduleBuilder.getNode(event), moduleBuilder.getNode(status));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onCaptureEventProfilingInfo(Op, IdRef event, IdRef profiling_info, IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpCaptureEventProfilingInfo>(moduleBuilder.getNode(event),
      moduleBuilder.getNode(profiling_info), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGetDefaultQueue(Op, IdResult id_result, IdResultType id_result_type)
  {
    auto node = moduleBuilder.newNode<NodeOpGetDefaultQueue>(id_result.value, moduleBuilder.getType(id_result_type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onBuildNDRange(Op, IdResult id_result, IdResultType id_result_type, IdRef global_work_size, IdRef local_work_size,
    IdRef global_work_offset)
  {
    auto node = moduleBuilder.newNode<NodeOpBuildNDRange>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(global_work_size), moduleBuilder.getNode(local_work_size), moduleBuilder.getNode(global_work_offset));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSparseSampleImplicitLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate,
    Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageSparseSampleImplicitLod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), image_operandsVal, image_operands_Bias_first,
      image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second, image_operands_ConstOffset_first,
      image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first, image_operands_MinLod_first,
      image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first, image_operands_MakeTexelVisible_first,
      image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSparseSampleExplicitLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate,
    TypeTraits<ImageOperandsMask>::ReadType image_operands)
  {
    ImageOperandsMask image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    image_operandsVal = image_operands.value;
    if ((image_operands.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
    {
      image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Bias.first));
    }
    if ((image_operands.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
    {
      image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Lod.first));
    }
    if ((image_operands.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
    {
      image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Grad.first));
      image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Grad.second));
    }
    if ((image_operands.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
    {
      image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.ConstOffset.first));
    }
    if ((image_operands.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
    {
      image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Offset.first));
    }
    if ((image_operands.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
    {
      image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.ConstOffsets.first));
    }
    if ((image_operands.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
    {
      image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Sample.first));
    }
    if ((image_operands.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
    {
      image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.MinLod.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelAvailable_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelAvailable.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelAvailableKHR_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelAvailableKHR.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelVisible_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelVisible.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelVisibleKHR_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelVisibleKHR.first));
    }
    if ((image_operands.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
    {
      image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Offsets.first));
    }
    auto node = moduleBuilder.newNode<NodeOpImageSparseSampleExplicitLod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), image_operandsVal, image_operands_Bias_first,
      image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second, image_operands_ConstOffset_first,
      image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first, image_operands_MinLod_first,
      image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first, image_operands_MakeTexelVisible_first,
      image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSparseSampleDrefImplicitLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate,
    IdRef d_ref, Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageSparseSampleDrefImplicitLod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), moduleBuilder.getNode(d_ref), image_operandsVal,
      image_operands_Bias_first, image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second,
      image_operands_ConstOffset_first, image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first,
      image_operands_MinLod_first, image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first,
      image_operands_MakeTexelVisible_first, image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSparseSampleDrefExplicitLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate,
    IdRef d_ref, TypeTraits<ImageOperandsMask>::ReadType image_operands)
  {
    ImageOperandsMask image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    image_operandsVal = image_operands.value;
    if ((image_operands.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
    {
      image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Bias.first));
    }
    if ((image_operands.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
    {
      image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Lod.first));
    }
    if ((image_operands.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
    {
      image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Grad.first));
      image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Grad.second));
    }
    if ((image_operands.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
    {
      image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.ConstOffset.first));
    }
    if ((image_operands.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
    {
      image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Offset.first));
    }
    if ((image_operands.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
    {
      image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.ConstOffsets.first));
    }
    if ((image_operands.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
    {
      image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Sample.first));
    }
    if ((image_operands.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
    {
      image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.MinLod.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelAvailable_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelAvailable.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelAvailableKHR_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelAvailableKHR.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelVisible_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelVisible.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelVisibleKHR_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelVisibleKHR.first));
    }
    if ((image_operands.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
    {
      image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Offsets.first));
    }
    auto node = moduleBuilder.newNode<NodeOpImageSparseSampleDrefExplicitLod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), moduleBuilder.getNode(d_ref), image_operandsVal,
      image_operands_Bias_first, image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second,
      image_operands_ConstOffset_first, image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first,
      image_operands_MinLod_first, image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first,
      image_operands_MakeTexelVisible_first, image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSparseSampleProjImplicitLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate,
    Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageSparseSampleProjImplicitLod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), image_operandsVal, image_operands_Bias_first,
      image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second, image_operands_ConstOffset_first,
      image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first, image_operands_MinLod_first,
      image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first, image_operands_MakeTexelVisible_first,
      image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSparseSampleProjExplicitLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate,
    TypeTraits<ImageOperandsMask>::ReadType image_operands)
  {
    ImageOperandsMask image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    image_operandsVal = image_operands.value;
    if ((image_operands.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
    {
      image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Bias.first));
    }
    if ((image_operands.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
    {
      image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Lod.first));
    }
    if ((image_operands.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
    {
      image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Grad.first));
      image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Grad.second));
    }
    if ((image_operands.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
    {
      image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.ConstOffset.first));
    }
    if ((image_operands.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
    {
      image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Offset.first));
    }
    if ((image_operands.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
    {
      image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.ConstOffsets.first));
    }
    if ((image_operands.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
    {
      image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Sample.first));
    }
    if ((image_operands.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
    {
      image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.MinLod.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelAvailable_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelAvailable.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelAvailableKHR_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelAvailableKHR.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelVisible_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelVisible.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelVisibleKHR_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelVisibleKHR.first));
    }
    if ((image_operands.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
    {
      image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Offsets.first));
    }
    auto node = moduleBuilder.newNode<NodeOpImageSparseSampleProjExplicitLod>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), image_operandsVal, image_operands_Bias_first,
      image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second, image_operands_ConstOffset_first,
      image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first, image_operands_MinLod_first,
      image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first, image_operands_MakeTexelVisible_first,
      image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSparseSampleProjDrefImplicitLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image,
    IdRef coordinate, IdRef d_ref, Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node =
      moduleBuilder.newNode<NodeOpImageSparseSampleProjDrefImplicitLod>(id_result.value, moduleBuilder.getType(id_result_type),
        moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), moduleBuilder.getNode(d_ref), image_operandsVal,
        image_operands_Bias_first, image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second,
        image_operands_ConstOffset_first, image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first,
        image_operands_MinLod_first, image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first,
        image_operands_MakeTexelVisible_first, image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSparseSampleProjDrefExplicitLod(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image,
    IdRef coordinate, IdRef d_ref, TypeTraits<ImageOperandsMask>::ReadType image_operands)
  {
    ImageOperandsMask image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    image_operandsVal = image_operands.value;
    if ((image_operands.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
    {
      image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Bias.first));
    }
    if ((image_operands.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
    {
      image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Lod.first));
    }
    if ((image_operands.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
    {
      image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Grad.first));
      image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Grad.second));
    }
    if ((image_operands.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
    {
      image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.ConstOffset.first));
    }
    if ((image_operands.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
    {
      image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Offset.first));
    }
    if ((image_operands.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
    {
      image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.ConstOffsets.first));
    }
    if ((image_operands.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
    {
      image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Sample.first));
    }
    if ((image_operands.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
    {
      image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.MinLod.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelAvailable_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelAvailable.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelAvailableKHR_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelAvailableKHR.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelVisible_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelVisible.first));
    }
    if ((image_operands.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
    {
      image_operands_MakeTexelVisibleKHR_first =
        NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.data.MakeTexelVisibleKHR.first));
    }
    if ((image_operands.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
    {
      image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.data.Offsets.first));
    }
    auto node =
      moduleBuilder.newNode<NodeOpImageSparseSampleProjDrefExplicitLod>(id_result.value, moduleBuilder.getType(id_result_type),
        moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), moduleBuilder.getNode(d_ref), image_operandsVal,
        image_operands_Bias_first, image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second,
        image_operands_ConstOffset_first, image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first,
        image_operands_MinLod_first, image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first,
        image_operands_MakeTexelVisible_first, image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSparseFetch(Op, IdResult id_result, IdResultType id_result_type, IdRef image, IdRef coordinate,
    Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageSparseFetch>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image), moduleBuilder.getNode(coordinate), image_operandsVal, image_operands_Bias_first,
      image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second, image_operands_ConstOffset_first,
      image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first, image_operands_MinLod_first,
      image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first, image_operands_MakeTexelVisible_first,
      image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSparseGather(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate, IdRef component,
    Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageSparseGather>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), moduleBuilder.getNode(component), image_operandsVal,
      image_operands_Bias_first, image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second,
      image_operands_ConstOffset_first, image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first,
      image_operands_MinLod_first, image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first,
      image_operands_MakeTexelVisible_first, image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSparseDrefGather(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate, IdRef d_ref,
    Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageSparseDrefGather>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), moduleBuilder.getNode(d_ref), image_operandsVal,
      image_operands_Bias_first, image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second,
      image_operands_ConstOffset_first, image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first,
      image_operands_MinLod_first, image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first,
      image_operands_MakeTexelVisible_first, image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSparseTexelsResident(Op, IdResult id_result, IdResultType id_result_type, IdRef resident_code)
  {
    auto node = moduleBuilder.newNode<NodeOpImageSparseTexelsResident>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(resident_code));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onNoLine(Op)
  {
    auto node = moduleBuilder.newNode<NodeOpNoLine>();
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicFlagTestAndSet(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory,
    IdMemorySemantics semantics)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicFlagTestAndSet>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicFlagClear(Op, IdRef pointer, IdScope memory, IdMemorySemantics semantics)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicFlagClear>(moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory),
      moduleBuilder.getNode(semantics));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSparseRead(Op, IdResult id_result, IdResultType id_result_type, IdRef image, IdRef coordinate,
    Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageSparseRead>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image), moduleBuilder.getNode(coordinate), image_operandsVal, image_operands_Bias_first,
      image_operands_Lod_first, image_operands_Grad_first, image_operands_Grad_second, image_operands_ConstOffset_first,
      image_operands_Offset_first, image_operands_ConstOffsets_first, image_operands_Sample_first, image_operands_MinLod_first,
      image_operands_MakeTexelAvailable_first, image_operands_MakeTexelAvailableKHR_first, image_operands_MakeTexelVisible_first,
      image_operands_MakeTexelVisibleKHR_first, image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSizeOf(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer)
  {
    auto node =
      moduleBuilder.newNode<NodeOpSizeOf>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(pointer));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onTypePipeStorage(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypePipeStorage>(id_result.value); }
  void onConstantPipeStorage(Op, IdResult id_result, IdResultType id_result_type, LiteralInteger packet_size,
    LiteralInteger packet_alignment, LiteralInteger capacity)
  {
    auto node = moduleBuilder.newNode<NodeOpConstantPipeStorage>(id_result.value, moduleBuilder.getType(id_result_type), packet_size,
      packet_alignment, capacity);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onCreatePipeFromPipeStorage(Op, IdResult id_result, IdResultType id_result_type, IdRef pipe_storage)
  {
    auto node = moduleBuilder.newNode<NodeOpCreatePipeFromPipeStorage>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pipe_storage));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGetKernelLocalSizeForSubgroupCount(Op, IdResult id_result, IdResultType id_result_type, IdRef subgroup_count, IdRef invoke,
    IdRef param, IdRef param_size, IdRef param_align)
  {
    auto node = moduleBuilder.newNode<NodeOpGetKernelLocalSizeForSubgroupCount>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(subgroup_count), moduleBuilder.getNode(invoke), moduleBuilder.getNode(param),
      moduleBuilder.getNode(param_size), moduleBuilder.getNode(param_align));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGetKernelMaxNumSubgroups(Op, IdResult id_result, IdResultType id_result_type, IdRef invoke, IdRef param, IdRef param_size,
    IdRef param_align)
  {
    auto node = moduleBuilder.newNode<NodeOpGetKernelMaxNumSubgroups>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(invoke), moduleBuilder.getNode(param), moduleBuilder.getNode(param_size),
      moduleBuilder.getNode(param_align));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onTypeNamedBarrier(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeNamedBarrier>(id_result.value); }
  void onNamedBarrierInitialize(Op, IdResult id_result, IdResultType id_result_type, IdRef subgroup_count)
  {
    auto node = moduleBuilder.newNode<NodeOpNamedBarrierInitialize>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(subgroup_count));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onMemoryNamedBarrier(Op, IdRef named_barrier, IdScope memory, IdMemorySemantics semantics)
  {
    auto node = moduleBuilder.newNode<NodeOpMemoryNamedBarrier>(moduleBuilder.getNode(named_barrier), moduleBuilder.getNode(memory),
      moduleBuilder.getNode(semantics));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onModuleProcessed(Op, LiteralString process) { moduleBuilder.addModuleProcess(process.asString(), process.length); }
  void onExecutionModeId(Op, IdRef entry_point, TypeTraits<ExecutionMode>::ReadType mode)
  {
    ExectionModeDef info;
    info.target = entry_point;
    switch (mode.value)
    {
      case ExecutionMode::Invocations:
        info.mode = new ExecutionModeInvocations(mode.data.Invocations.numberOfInvocationInvocations);
        break;
      case ExecutionMode::SpacingEqual: info.mode = new ExecutionModeSpacingEqual; break;
      case ExecutionMode::SpacingFractionalEven: info.mode = new ExecutionModeSpacingFractionalEven; break;
      case ExecutionMode::SpacingFractionalOdd: info.mode = new ExecutionModeSpacingFractionalOdd; break;
      case ExecutionMode::VertexOrderCw: info.mode = new ExecutionModeVertexOrderCw; break;
      case ExecutionMode::VertexOrderCcw: info.mode = new ExecutionModeVertexOrderCcw; break;
      case ExecutionMode::PixelCenterInteger: info.mode = new ExecutionModePixelCenterInteger; break;
      case ExecutionMode::OriginUpperLeft: info.mode = new ExecutionModeOriginUpperLeft; break;
      case ExecutionMode::OriginLowerLeft: info.mode = new ExecutionModeOriginLowerLeft; break;
      case ExecutionMode::EarlyFragmentTests: info.mode = new ExecutionModeEarlyFragmentTests; break;
      case ExecutionMode::PointMode: info.mode = new ExecutionModePointMode; break;
      case ExecutionMode::Xfb: info.mode = new ExecutionModeXfb; break;
      case ExecutionMode::DepthReplacing: info.mode = new ExecutionModeDepthReplacing; break;
      case ExecutionMode::DepthGreater: info.mode = new ExecutionModeDepthGreater; break;
      case ExecutionMode::DepthLess: info.mode = new ExecutionModeDepthLess; break;
      case ExecutionMode::DepthUnchanged: info.mode = new ExecutionModeDepthUnchanged; break;
      case ExecutionMode::LocalSize:
        info.mode = new ExecutionModeLocalSize(mode.data.LocalSize.xSize, mode.data.LocalSize.ySize, mode.data.LocalSize.zSize);
        break;
      case ExecutionMode::LocalSizeHint:
        info.mode =
          new ExecutionModeLocalSizeHint(mode.data.LocalSizeHint.xSize, mode.data.LocalSizeHint.ySize, mode.data.LocalSizeHint.zSize);
        break;
      case ExecutionMode::InputPoints: info.mode = new ExecutionModeInputPoints; break;
      case ExecutionMode::InputLines: info.mode = new ExecutionModeInputLines; break;
      case ExecutionMode::InputLinesAdjacency: info.mode = new ExecutionModeInputLinesAdjacency; break;
      case ExecutionMode::Triangles: info.mode = new ExecutionModeTriangles; break;
      case ExecutionMode::InputTrianglesAdjacency: info.mode = new ExecutionModeInputTrianglesAdjacency; break;
      case ExecutionMode::Quads: info.mode = new ExecutionModeQuads; break;
      case ExecutionMode::Isolines: info.mode = new ExecutionModeIsolines; break;
      case ExecutionMode::OutputVertices: info.mode = new ExecutionModeOutputVertices(mode.data.OutputVertices.vertexCount); break;
      case ExecutionMode::OutputPoints: info.mode = new ExecutionModeOutputPoints; break;
      case ExecutionMode::OutputLineStrip: info.mode = new ExecutionModeOutputLineStrip; break;
      case ExecutionMode::OutputTriangleStrip: info.mode = new ExecutionModeOutputTriangleStrip; break;
      case ExecutionMode::VecTypeHint: info.mode = new ExecutionModeVecTypeHint(mode.data.VecTypeHint.vectorType); break;
      case ExecutionMode::ContractionOff: info.mode = new ExecutionModeContractionOff; break;
      case ExecutionMode::Initializer: info.mode = new ExecutionModeInitializer; break;
      case ExecutionMode::Finalizer: info.mode = new ExecutionModeFinalizer; break;
      case ExecutionMode::SubgroupSize: info.mode = new ExecutionModeSubgroupSize(mode.data.SubgroupSize.subgroupSize); break;
      case ExecutionMode::SubgroupsPerWorkgroup:
        info.mode = new ExecutionModeSubgroupsPerWorkgroup(mode.data.SubgroupsPerWorkgroup.subgroupsPerWorkgroup);
        break;
      case ExecutionMode::SubgroupsPerWorkgroupId:
      {
        auto emode = new ExecutionModeSubgroupsPerWorkgroupId;
        PropertyReferenceResolveInfo refInfo;
        refInfo.target = &emode->subgroupsPerWorkgroup;
        refInfo.ref = mode.data.SubgroupsPerWorkgroupId.subgroupsPerWorkgroup;
        propertyReferenceResolves.push_back(refInfo);
        info.mode = emode;
      };
      break;
      case ExecutionMode::LocalSizeId:
      {
        auto emode = new ExecutionModeLocalSizeId;
        PropertyReferenceResolveInfo refInfo;
        refInfo.target = &emode->xSize;
        refInfo.ref = mode.data.LocalSizeId.xSize;
        propertyReferenceResolves.push_back(refInfo);
        refInfo.target = &emode->ySize;
        refInfo.ref = mode.data.LocalSizeId.ySize;
        propertyReferenceResolves.push_back(refInfo);
        refInfo.target = &emode->zSize;
        refInfo.ref = mode.data.LocalSizeId.zSize;
        propertyReferenceResolves.push_back(refInfo);
        info.mode = emode;
      };
      break;
      case ExecutionMode::LocalSizeHintId:
      {
        auto emode = new ExecutionModeLocalSizeHintId;
        PropertyReferenceResolveInfo refInfo;
        refInfo.target = &emode->xSizeHint;
        refInfo.ref = mode.data.LocalSizeHintId.xSizeHint;
        propertyReferenceResolves.push_back(refInfo);
        refInfo.target = &emode->ySizeHint;
        refInfo.ref = mode.data.LocalSizeHintId.ySizeHint;
        propertyReferenceResolves.push_back(refInfo);
        refInfo.target = &emode->zSizeHint;
        refInfo.ref = mode.data.LocalSizeHintId.zSizeHint;
        propertyReferenceResolves.push_back(refInfo);
        info.mode = emode;
      };
      break;
      case ExecutionMode::SubgroupUniformControlFlowKHR: info.mode = new ExecutionModeSubgroupUniformControlFlowKHR; break;
      case ExecutionMode::PostDepthCoverage: info.mode = new ExecutionModePostDepthCoverage; break;
      case ExecutionMode::DenormPreserve: info.mode = new ExecutionModeDenormPreserve(mode.data.DenormPreserve.targetWidth); break;
      case ExecutionMode::DenormFlushToZero:
        info.mode = new ExecutionModeDenormFlushToZero(mode.data.DenormFlushToZero.targetWidth);
        break;
      case ExecutionMode::SignedZeroInfNanPreserve:
        info.mode = new ExecutionModeSignedZeroInfNanPreserve(mode.data.SignedZeroInfNanPreserve.targetWidth);
        break;
      case ExecutionMode::RoundingModeRTE: info.mode = new ExecutionModeRoundingModeRTE(mode.data.RoundingModeRTE.targetWidth); break;
      case ExecutionMode::RoundingModeRTZ: info.mode = new ExecutionModeRoundingModeRTZ(mode.data.RoundingModeRTZ.targetWidth); break;
      case ExecutionMode::StencilRefReplacingEXT: info.mode = new ExecutionModeStencilRefReplacingEXT; break;
      case ExecutionMode::OutputLinesNV: info.mode = new ExecutionModeOutputLinesNV; break;
      case ExecutionMode::OutputPrimitivesNV:
        info.mode = new ExecutionModeOutputPrimitivesNV(mode.data.OutputPrimitivesNV.primitiveCount);
        break;
      case ExecutionMode::DerivativeGroupQuadsNV: info.mode = new ExecutionModeDerivativeGroupQuadsNV; break;
      case ExecutionMode::DerivativeGroupLinearNV: info.mode = new ExecutionModeDerivativeGroupLinearNV; break;
      case ExecutionMode::OutputTrianglesNV: info.mode = new ExecutionModeOutputTrianglesNV; break;
      case ExecutionMode::PixelInterlockOrderedEXT: info.mode = new ExecutionModePixelInterlockOrderedEXT; break;
      case ExecutionMode::PixelInterlockUnorderedEXT: info.mode = new ExecutionModePixelInterlockUnorderedEXT; break;
      case ExecutionMode::SampleInterlockOrderedEXT: info.mode = new ExecutionModeSampleInterlockOrderedEXT; break;
      case ExecutionMode::SampleInterlockUnorderedEXT: info.mode = new ExecutionModeSampleInterlockUnorderedEXT; break;
      case ExecutionMode::ShadingRateInterlockOrderedEXT: info.mode = new ExecutionModeShadingRateInterlockOrderedEXT; break;
      case ExecutionMode::ShadingRateInterlockUnorderedEXT: info.mode = new ExecutionModeShadingRateInterlockUnorderedEXT; break;
      case ExecutionMode::SharedLocalMemorySizeINTEL:
        info.mode = new ExecutionModeSharedLocalMemorySizeINTEL(mode.data.SharedLocalMemorySizeINTEL.size);
        break;
      case ExecutionMode::RoundingModeRTPINTEL:
        info.mode = new ExecutionModeRoundingModeRTPINTEL(mode.data.RoundingModeRTPINTEL.targetWidth);
        break;
      case ExecutionMode::RoundingModeRTNINTEL:
        info.mode = new ExecutionModeRoundingModeRTNINTEL(mode.data.RoundingModeRTNINTEL.targetWidth);
        break;
      case ExecutionMode::FloatingPointModeALTINTEL:
        info.mode = new ExecutionModeFloatingPointModeALTINTEL(mode.data.FloatingPointModeALTINTEL.targetWidth);
        break;
      case ExecutionMode::FloatingPointModeIEEEINTEL:
        info.mode = new ExecutionModeFloatingPointModeIEEEINTEL(mode.data.FloatingPointModeIEEEINTEL.targetWidth);
        break;
      case ExecutionMode::MaxWorkgroupSizeINTEL:
        info.mode = new ExecutionModeMaxWorkgroupSizeINTEL(mode.data.MaxWorkgroupSizeINTEL.max_x_size,
          mode.data.MaxWorkgroupSizeINTEL.max_y_size, mode.data.MaxWorkgroupSizeINTEL.max_z_size);
        break;
      case ExecutionMode::MaxWorkDimINTEL:
        info.mode = new ExecutionModeMaxWorkDimINTEL(mode.data.MaxWorkDimINTEL.max_dimensions);
        break;
      case ExecutionMode::NoGlobalOffsetINTEL: info.mode = new ExecutionModeNoGlobalOffsetINTEL; break;
      case ExecutionMode::NumSIMDWorkitemsINTEL:
        info.mode = new ExecutionModeNumSIMDWorkitemsINTEL(mode.data.NumSIMDWorkitemsINTEL.vector_width);
        break;
      case ExecutionMode::SchedulerTargetFmaxMhzINTEL:
        info.mode = new ExecutionModeSchedulerTargetFmaxMhzINTEL(mode.data.SchedulerTargetFmaxMhzINTEL.target_fmax);
        break;
    }
    executionModes.push_back(eastl::move(info));
  }
  void onDecorateId(Op, IdRef target, TypeTraits<Decoration>::ReadType decoration)
  {
    CastableUniquePointer<Property> propPtr;
    switch (decoration.value)
    {
      case Decoration::RelaxedPrecision:
      {
        auto newProp = new PropertyRelaxedPrecision;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::SpecId:
      {
        auto newProp = new PropertySpecId;
        propPtr.reset(newProp);
        newProp->specializationConstantId = decoration.data.SpecId.specializationConstantId;
        break;
      }
      case Decoration::Block:
      {
        auto newProp = new PropertyBlock;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BufferBlock:
      {
        auto newProp = new PropertyBufferBlock;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::RowMajor:
      {
        auto newProp = new PropertyRowMajor;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ColMajor:
      {
        auto newProp = new PropertyColMajor;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ArrayStride:
      {
        auto newProp = new PropertyArrayStride;
        propPtr.reset(newProp);
        newProp->arrayStride = decoration.data.ArrayStride.arrayStride;
        break;
      }
      case Decoration::MatrixStride:
      {
        auto newProp = new PropertyMatrixStride;
        propPtr.reset(newProp);
        newProp->matrixStride = decoration.data.MatrixStride.matrixStride;
        break;
      }
      case Decoration::GLSLShared:
      {
        auto newProp = new PropertyGLSLShared;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::GLSLPacked:
      {
        auto newProp = new PropertyGLSLPacked;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::CPacked:
      {
        auto newProp = new PropertyCPacked;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BuiltIn:
      {
        auto newProp = new PropertyBuiltIn;
        propPtr.reset(newProp);
        newProp->builtIn = decoration.data.BuiltInData.first;
        break;
      }
      case Decoration::NoPerspective:
      {
        auto newProp = new PropertyNoPerspective;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Flat:
      {
        auto newProp = new PropertyFlat;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Patch:
      {
        auto newProp = new PropertyPatch;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Centroid:
      {
        auto newProp = new PropertyCentroid;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Sample:
      {
        auto newProp = new PropertySample;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Invariant:
      {
        auto newProp = new PropertyInvariant;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Restrict:
      {
        auto newProp = new PropertyRestrict;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Aliased:
      {
        auto newProp = new PropertyAliased;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Volatile:
      {
        auto newProp = new PropertyVolatile;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Constant:
      {
        auto newProp = new PropertyConstant;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Coherent:
      {
        auto newProp = new PropertyCoherent;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::NonWritable:
      {
        auto newProp = new PropertyNonWritable;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::NonReadable:
      {
        auto newProp = new PropertyNonReadable;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Uniform:
      {
        auto newProp = new PropertyUniform;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::UniformId:
      {
        auto newProp = new PropertyUniformId;
        propPtr.reset(newProp);
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->execution), IdRef{decoration.data.UniformId.execution.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::SaturatedConversion:
      {
        auto newProp = new PropertySaturatedConversion;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Stream:
      {
        auto newProp = new PropertyStream;
        propPtr.reset(newProp);
        newProp->streamNumber = decoration.data.Stream.streamNumber;
        break;
      }
      case Decoration::Location:
      {
        auto newProp = new PropertyLocation;
        propPtr.reset(newProp);
        newProp->location = decoration.data.Location.location;
        break;
      }
      case Decoration::Component:
      {
        auto newProp = new PropertyComponent;
        propPtr.reset(newProp);
        newProp->component = decoration.data.Component.component;
        break;
      }
      case Decoration::Index:
      {
        auto newProp = new PropertyIndex;
        propPtr.reset(newProp);
        newProp->index = decoration.data.Index.index;
        break;
      }
      case Decoration::Binding:
      {
        auto newProp = new PropertyBinding;
        propPtr.reset(newProp);
        newProp->bindingPoint = decoration.data.Binding.bindingPoint;
        break;
      }
      case Decoration::DescriptorSet:
      {
        auto newProp = new PropertyDescriptorSet;
        propPtr.reset(newProp);
        newProp->descriptorSet = decoration.data.DescriptorSet.descriptorSet;
        break;
      }
      case Decoration::Offset:
      {
        auto newProp = new PropertyOffset;
        propPtr.reset(newProp);
        newProp->byteOffset = decoration.data.Offset.byteOffset;
        break;
      }
      case Decoration::XfbBuffer:
      {
        auto newProp = new PropertyXfbBuffer;
        propPtr.reset(newProp);
        newProp->xfbBufferNumber = decoration.data.XfbBuffer.xfbBufferNumber;
        break;
      }
      case Decoration::XfbStride:
      {
        auto newProp = new PropertyXfbStride;
        propPtr.reset(newProp);
        newProp->xfbStride = decoration.data.XfbStride.xfbStride;
        break;
      }
      case Decoration::FuncParamAttr:
      {
        auto newProp = new PropertyFuncParamAttr;
        propPtr.reset(newProp);
        newProp->functionParameterAttribute = decoration.data.FuncParamAttr.functionParameterAttribute;
        break;
      }
      case Decoration::FPRoundingMode:
      {
        auto newProp = new PropertyFPRoundingMode;
        propPtr.reset(newProp);
        newProp->floatingPointRoundingMode = decoration.data.FPRoundingMode.floatingPointRoundingMode;
        break;
      }
      case Decoration::FPFastMathMode:
      {
        auto newProp = new PropertyFPFastMathMode;
        propPtr.reset(newProp);
        newProp->fastMathMode = decoration.data.FPFastMathMode.fastMathMode;
        break;
      }
      case Decoration::LinkageAttributes:
      {
        auto newProp = new PropertyLinkageAttributes;
        propPtr.reset(newProp);
        newProp->name = decoration.data.LinkageAttributes.name.asStringObj();
        newProp->linkageType = decoration.data.LinkageAttributes.linkageType;
        break;
      }
      case Decoration::NoContraction:
      {
        auto newProp = new PropertyNoContraction;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::InputAttachmentIndex:
      {
        auto newProp = new PropertyInputAttachmentIndex;
        propPtr.reset(newProp);
        newProp->attachmentIndex = decoration.data.InputAttachmentIndex.attachmentIndex;
        break;
      }
      case Decoration::Alignment:
      {
        auto newProp = new PropertyAlignment;
        propPtr.reset(newProp);
        newProp->alignment = decoration.data.Alignment.alignment;
        break;
      }
      case Decoration::MaxByteOffset:
      {
        auto newProp = new PropertyMaxByteOffset;
        propPtr.reset(newProp);
        newProp->maxByteOffset = decoration.data.MaxByteOffset.maxByteOffset;
        break;
      }
      case Decoration::AlignmentId:
      {
        auto newProp = new PropertyAlignmentId;
        propPtr.reset(newProp);
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->alignment), IdRef{decoration.data.AlignmentId.alignment.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::MaxByteOffsetId:
      {
        auto newProp = new PropertyMaxByteOffsetId;
        propPtr.reset(newProp);
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->maxByteOffset),
            IdRef{decoration.data.MaxByteOffsetId.maxByteOffset.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::NoSignedWrap:
      {
        auto newProp = new PropertyNoSignedWrap;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::NoUnsignedWrap:
      {
        auto newProp = new PropertyNoUnsignedWrap;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ExplicitInterpAMD:
      {
        auto newProp = new PropertyExplicitInterpAMD;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::OverrideCoverageNV:
      {
        auto newProp = new PropertyOverrideCoverageNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PassthroughNV:
      {
        auto newProp = new PropertyPassthroughNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ViewportRelativeNV:
      {
        auto newProp = new PropertyViewportRelativeNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::SecondaryViewportRelativeNV:
      {
        auto newProp = new PropertySecondaryViewportRelativeNV;
        propPtr.reset(newProp);
        newProp->offset = decoration.data.SecondaryViewportRelativeNV.offset;
        break;
      }
      case Decoration::PerPrimitiveNV:
      {
        auto newProp = new PropertyPerPrimitiveNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PerViewNV:
      {
        auto newProp = new PropertyPerViewNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PerTaskNV:
      {
        auto newProp = new PropertyPerTaskNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PerVertexKHR:
      {
        auto newProp = new PropertyPerVertexKHR;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::NonUniform:
      {
        auto newProp = new PropertyNonUniform;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::RestrictPointer:
      {
        auto newProp = new PropertyRestrictPointer;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::AliasedPointer:
      {
        auto newProp = new PropertyAliasedPointer;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BindlessSamplerNV:
      {
        auto newProp = new PropertyBindlessSamplerNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BindlessImageNV:
      {
        auto newProp = new PropertyBindlessImageNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BoundSamplerNV:
      {
        auto newProp = new PropertyBoundSamplerNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BoundImageNV:
      {
        auto newProp = new PropertyBoundImageNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::SIMTCallINTEL:
      {
        auto newProp = new PropertySIMTCallINTEL;
        propPtr.reset(newProp);
        newProp->n = decoration.data.SIMTCallINTEL.n;
        break;
      }
      case Decoration::ReferencedIndirectlyINTEL:
      {
        auto newProp = new PropertyReferencedIndirectlyINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ClobberINTEL:
      {
        auto newProp = new PropertyClobberINTEL;
        propPtr.reset(newProp);
        newProp->_register = decoration.data.ClobberINTEL._register.asStringObj();
        break;
      }
      case Decoration::SideEffectsINTEL:
      {
        auto newProp = new PropertySideEffectsINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::VectorComputeVariableINTEL:
      {
        auto newProp = new PropertyVectorComputeVariableINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::FuncParamIOKindINTEL:
      {
        auto newProp = new PropertyFuncParamIOKindINTEL;
        propPtr.reset(newProp);
        newProp->kind = decoration.data.FuncParamIOKindINTEL.kind;
        break;
      }
      case Decoration::VectorComputeFunctionINTEL:
      {
        auto newProp = new PropertyVectorComputeFunctionINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::StackCallINTEL:
      {
        auto newProp = new PropertyStackCallINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::GlobalVariableOffsetINTEL:
      {
        auto newProp = new PropertyGlobalVariableOffsetINTEL;
        propPtr.reset(newProp);
        newProp->offset = decoration.data.GlobalVariableOffsetINTEL.offset;
        break;
      }
      case Decoration::CounterBuffer:
      {
        auto newProp = new PropertyCounterBuffer;
        propPtr.reset(newProp);
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->counterBuffer), IdRef{decoration.data.CounterBuffer.counterBuffer.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::UserSemantic:
      {
        auto newProp = new PropertyUserSemantic;
        propPtr.reset(newProp);
        newProp->semantic = decoration.data.UserSemantic.semantic.asStringObj();
        break;
      }
      case Decoration::UserTypeGOOGLE:
      {
        auto newProp = new PropertyUserTypeGOOGLE;
        propPtr.reset(newProp);
        newProp->userType = decoration.data.UserTypeGOOGLE.userType.asStringObj();
        break;
      }
      case Decoration::FunctionRoundingModeINTEL:
      {
        auto newProp = new PropertyFunctionRoundingModeINTEL;
        propPtr.reset(newProp);
        newProp->targetWidth = decoration.data.FunctionRoundingModeINTEL.targetWidth;
        newProp->fpRoundingMode = decoration.data.FunctionRoundingModeINTEL.fpRoundingMode;
        break;
      }
      case Decoration::FunctionDenormModeINTEL:
      {
        auto newProp = new PropertyFunctionDenormModeINTEL;
        propPtr.reset(newProp);
        newProp->targetWidth = decoration.data.FunctionDenormModeINTEL.targetWidth;
        newProp->fpDenormMode = decoration.data.FunctionDenormModeINTEL.fpDenormMode;
        break;
      }
      case Decoration::RegisterINTEL:
      {
        auto newProp = new PropertyRegisterINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::MemoryINTEL:
      {
        auto newProp = new PropertyMemoryINTEL;
        propPtr.reset(newProp);
        newProp->memoryType = decoration.data.MemoryINTEL.memoryType.asStringObj();
        break;
      }
      case Decoration::NumbanksINTEL:
      {
        auto newProp = new PropertyNumbanksINTEL;
        propPtr.reset(newProp);
        newProp->banks = decoration.data.NumbanksINTEL.banks;
        break;
      }
      case Decoration::BankwidthINTEL:
      {
        auto newProp = new PropertyBankwidthINTEL;
        propPtr.reset(newProp);
        newProp->bankWidth = decoration.data.BankwidthINTEL.bankWidth;
        break;
      }
      case Decoration::MaxPrivateCopiesINTEL:
      {
        auto newProp = new PropertyMaxPrivateCopiesINTEL;
        propPtr.reset(newProp);
        newProp->maximumCopies = decoration.data.MaxPrivateCopiesINTEL.maximumCopies;
        break;
      }
      case Decoration::SinglepumpINTEL:
      {
        auto newProp = new PropertySinglepumpINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::DoublepumpINTEL:
      {
        auto newProp = new PropertyDoublepumpINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::MaxReplicatesINTEL:
      {
        auto newProp = new PropertyMaxReplicatesINTEL;
        propPtr.reset(newProp);
        newProp->maximumReplicates = decoration.data.MaxReplicatesINTEL.maximumReplicates;
        break;
      }
      case Decoration::SimpleDualPortINTEL:
      {
        auto newProp = new PropertySimpleDualPortINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::MergeINTEL:
      {
        auto newProp = new PropertyMergeINTEL;
        propPtr.reset(newProp);
        newProp->mergeKey = decoration.data.MergeINTEL.mergeKey.asStringObj();
        newProp->mergeType = decoration.data.MergeINTEL.mergeType.asStringObj();
        break;
      }
      case Decoration::BankBitsINTEL:
      {
        auto newProp = new PropertyBankBitsINTEL;
        propPtr.reset(newProp);
        newProp->bankBits = decoration.data.BankBitsINTEL.bankBits;
        break;
      }
      case Decoration::ForcePow2DepthINTEL:
      {
        auto newProp = new PropertyForcePow2DepthINTEL;
        propPtr.reset(newProp);
        newProp->forceKey = decoration.data.ForcePow2DepthINTEL.forceKey;
        break;
      }
      case Decoration::BurstCoalesceINTEL:
      {
        auto newProp = new PropertyBurstCoalesceINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::CacheSizeINTEL:
      {
        auto newProp = new PropertyCacheSizeINTEL;
        propPtr.reset(newProp);
        newProp->cacheSizeInBytes = decoration.data.CacheSizeINTEL.cacheSizeInBytes;
        break;
      }
      case Decoration::DontStaticallyCoalesceINTEL:
      {
        auto newProp = new PropertyDontStaticallyCoalesceINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PrefetchINTEL:
      {
        auto newProp = new PropertyPrefetchINTEL;
        propPtr.reset(newProp);
        newProp->prefetcherSizeInBytes = decoration.data.PrefetchINTEL.prefetcherSizeInBytes;
        break;
      }
      case Decoration::StallEnableINTEL:
      {
        auto newProp = new PropertyStallEnableINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::FuseLoopsInFunctionINTEL:
      {
        auto newProp = new PropertyFuseLoopsInFunctionINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BufferLocationINTEL:
      {
        auto newProp = new PropertyBufferLocationINTEL;
        propPtr.reset(newProp);
        newProp->bufferLocationId = decoration.data.BufferLocationINTEL.bufferLocationId;
        break;
      }
      case Decoration::IOPipeStorageINTEL:
      {
        auto newProp = new PropertyIOPipeStorageINTEL;
        propPtr.reset(newProp);
        newProp->ioPipeId = decoration.data.IOPipeStorageINTEL.ioPipeId;
        break;
      }
      case Decoration::FunctionFloatingPointModeINTEL:
      {
        auto newProp = new PropertyFunctionFloatingPointModeINTEL;
        propPtr.reset(newProp);
        newProp->targetWidth = decoration.data.FunctionFloatingPointModeINTEL.targetWidth;
        newProp->fpOperationMode = decoration.data.FunctionFloatingPointModeINTEL.fpOperationMode;
        break;
      }
      case Decoration::SingleElementVectorINTEL:
      {
        auto newProp = new PropertySingleElementVectorINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::VectorComputeCallableFunctionINTEL:
      {
        auto newProp = new PropertyVectorComputeCallableFunctionINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::MediaBlockIOINTEL:
      {
        auto newProp = new PropertyMediaBlockIOINTEL;
        propPtr.reset(newProp);
        break;
      }
    }
    PropertyTargetResolveInfo targetResolve //
      {target, eastl::move(propPtr)};
    propertyTargetResolves.push_back(eastl::move(targetResolve));
  }
  void onGroupNonUniformElect(Op, IdResult id_result, IdResultType id_result_type, IdScope execution)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformElect>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformAll(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef predicate)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformAll>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(predicate));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformAny(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef predicate)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformAny>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(predicate));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformAllEqual(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformAllEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformBroadcast(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef value, IdRef id)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformBroadcast>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(value), moduleBuilder.getNode(id));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformBroadcastFirst(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformBroadcastFirst>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformBallot(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef predicate)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformBallot>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(predicate));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformInverseBallot(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformInverseBallot>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformBallotBitExtract(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef value,
    IdRef index)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformBallotBitExtract>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(value), moduleBuilder.getNode(index));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformBallotBitCount(Op, IdResult id_result, IdResultType id_result_type, IdScope execution,
    GroupOperation operation, IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformBallotBitCount>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformBallotFindLSB(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformBallotFindLSB>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformBallotFindMSB(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformBallotFindMSB>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformShuffle(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef value, IdRef id)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformShuffle>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(value), moduleBuilder.getNode(id));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformShuffleXor(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef value, IdRef mask)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformShuffleXor>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(value), moduleBuilder.getNode(mask));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformShuffleUp(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef value, IdRef delta)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformShuffleUp>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(value), moduleBuilder.getNode(delta));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformShuffleDown(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef value, IdRef delta)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformShuffleDown>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(value), moduleBuilder.getNode(delta));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformIAdd(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef value, Optional<IdRef> cluster_size)
  {
    eastl::optional<NodePointer<NodeId>> cluster_sizeOpt;
    if (cluster_size.valid)
    {
      cluster_sizeOpt = moduleBuilder.getNode(cluster_size.value);
    }
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformIAdd>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value), cluster_sizeOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformFAdd(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef value, Optional<IdRef> cluster_size)
  {
    eastl::optional<NodePointer<NodeId>> cluster_sizeOpt;
    if (cluster_size.valid)
    {
      cluster_sizeOpt = moduleBuilder.getNode(cluster_size.value);
    }
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformFAdd>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value), cluster_sizeOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformIMul(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef value, Optional<IdRef> cluster_size)
  {
    eastl::optional<NodePointer<NodeId>> cluster_sizeOpt;
    if (cluster_size.valid)
    {
      cluster_sizeOpt = moduleBuilder.getNode(cluster_size.value);
    }
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformIMul>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value), cluster_sizeOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformFMul(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef value, Optional<IdRef> cluster_size)
  {
    eastl::optional<NodePointer<NodeId>> cluster_sizeOpt;
    if (cluster_size.valid)
    {
      cluster_sizeOpt = moduleBuilder.getNode(cluster_size.value);
    }
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformFMul>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value), cluster_sizeOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformSMin(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef value, Optional<IdRef> cluster_size)
  {
    eastl::optional<NodePointer<NodeId>> cluster_sizeOpt;
    if (cluster_size.valid)
    {
      cluster_sizeOpt = moduleBuilder.getNode(cluster_size.value);
    }
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformSMin>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value), cluster_sizeOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformUMin(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef value, Optional<IdRef> cluster_size)
  {
    eastl::optional<NodePointer<NodeId>> cluster_sizeOpt;
    if (cluster_size.valid)
    {
      cluster_sizeOpt = moduleBuilder.getNode(cluster_size.value);
    }
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformUMin>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value), cluster_sizeOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformFMin(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef value, Optional<IdRef> cluster_size)
  {
    eastl::optional<NodePointer<NodeId>> cluster_sizeOpt;
    if (cluster_size.valid)
    {
      cluster_sizeOpt = moduleBuilder.getNode(cluster_size.value);
    }
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformFMin>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value), cluster_sizeOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformSMax(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef value, Optional<IdRef> cluster_size)
  {
    eastl::optional<NodePointer<NodeId>> cluster_sizeOpt;
    if (cluster_size.valid)
    {
      cluster_sizeOpt = moduleBuilder.getNode(cluster_size.value);
    }
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformSMax>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value), cluster_sizeOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformUMax(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef value, Optional<IdRef> cluster_size)
  {
    eastl::optional<NodePointer<NodeId>> cluster_sizeOpt;
    if (cluster_size.valid)
    {
      cluster_sizeOpt = moduleBuilder.getNode(cluster_size.value);
    }
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformUMax>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value), cluster_sizeOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformFMax(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef value, Optional<IdRef> cluster_size)
  {
    eastl::optional<NodePointer<NodeId>> cluster_sizeOpt;
    if (cluster_size.valid)
    {
      cluster_sizeOpt = moduleBuilder.getNode(cluster_size.value);
    }
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformFMax>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value), cluster_sizeOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformBitwiseAnd(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef value, Optional<IdRef> cluster_size)
  {
    eastl::optional<NodePointer<NodeId>> cluster_sizeOpt;
    if (cluster_size.valid)
    {
      cluster_sizeOpt = moduleBuilder.getNode(cluster_size.value);
    }
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformBitwiseAnd>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value), cluster_sizeOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformBitwiseOr(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef value, Optional<IdRef> cluster_size)
  {
    eastl::optional<NodePointer<NodeId>> cluster_sizeOpt;
    if (cluster_size.valid)
    {
      cluster_sizeOpt = moduleBuilder.getNode(cluster_size.value);
    }
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformBitwiseOr>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value), cluster_sizeOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformBitwiseXor(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef value, Optional<IdRef> cluster_size)
  {
    eastl::optional<NodePointer<NodeId>> cluster_sizeOpt;
    if (cluster_size.valid)
    {
      cluster_sizeOpt = moduleBuilder.getNode(cluster_size.value);
    }
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformBitwiseXor>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value), cluster_sizeOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformLogicalAnd(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef value, Optional<IdRef> cluster_size)
  {
    eastl::optional<NodePointer<NodeId>> cluster_sizeOpt;
    if (cluster_size.valid)
    {
      cluster_sizeOpt = moduleBuilder.getNode(cluster_size.value);
    }
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformLogicalAnd>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value), cluster_sizeOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformLogicalOr(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef value, Optional<IdRef> cluster_size)
  {
    eastl::optional<NodePointer<NodeId>> cluster_sizeOpt;
    if (cluster_size.valid)
    {
      cluster_sizeOpt = moduleBuilder.getNode(cluster_size.value);
    }
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformLogicalOr>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value), cluster_sizeOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformLogicalXor(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef value, Optional<IdRef> cluster_size)
  {
    eastl::optional<NodePointer<NodeId>> cluster_sizeOpt;
    if (cluster_size.valid)
    {
      cluster_sizeOpt = moduleBuilder.getNode(cluster_size.value);
    }
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformLogicalXor>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(value), cluster_sizeOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformQuadBroadcast(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef value, IdRef index)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformQuadBroadcast>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(value), moduleBuilder.getNode(index));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformQuadSwap(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, IdRef value, IdRef direction)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformQuadSwap>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(value), moduleBuilder.getNode(direction));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onCopyLogical(Op, IdResult id_result, IdResultType id_result_type, IdRef operand)
  {
    auto node =
      moduleBuilder.newNode<NodeOpCopyLogical>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(operand));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onPtrEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpPtrEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onPtrNotEqual(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpPtrNotEqual>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onPtrDiff(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpPtrDiff>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onTerminateInvocation(Op)
  {
    auto node = moduleBuilder.newNode<NodeOpTerminateInvocation>();
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupBallotKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef predicate)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupBallotKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(predicate));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupFirstInvocationKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupFirstInvocationKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAllKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef predicate)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAllKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(predicate));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAnyKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef predicate)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAnyKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(predicate));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAllEqualKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef predicate)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAllEqualKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(predicate));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupReadInvocationKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef value, IdRef index)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupReadInvocationKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(value), moduleBuilder.getNode(index));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onTraceRayKHR(Op, IdRef accel, IdRef ray_flags, IdRef cull_mask, IdRef s_b_t_offset, IdRef s_b_t_stride, IdRef miss_index,
    IdRef ray_origin, IdRef ray_tmin, IdRef ray_direction, IdRef ray_tmax, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpTraceRayKHR>(moduleBuilder.getNode(accel), moduleBuilder.getNode(ray_flags),
      moduleBuilder.getNode(cull_mask), moduleBuilder.getNode(s_b_t_offset), moduleBuilder.getNode(s_b_t_stride),
      moduleBuilder.getNode(miss_index), moduleBuilder.getNode(ray_origin), moduleBuilder.getNode(ray_tmin),
      moduleBuilder.getNode(ray_direction), moduleBuilder.getNode(ray_tmax), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onExecuteCallableKHR(Op, IdRef s_b_t_index, IdRef callable_data)
  {
    auto node =
      moduleBuilder.newNode<NodeOpExecuteCallableKHR>(moduleBuilder.getNode(s_b_t_index), moduleBuilder.getNode(callable_data));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConvertUToAccelerationStructureKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef accel)
  {
    auto node = moduleBuilder.newNode<NodeOpConvertUToAccelerationStructureKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(accel));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIgnoreIntersectionKHR(Op)
  {
    auto node = moduleBuilder.newNode<NodeOpIgnoreIntersectionKHR>();
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onTerminateRayKHR(Op)
  {
    auto node = moduleBuilder.newNode<NodeOpTerminateRayKHR>();
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSDot(Op, IdResult id_result, IdResultType id_result_type, IdRef vector_1, IdRef vector_2,
    Optional<PackedVectorFormat> packed_vector_format)
  {
    eastl::optional<PackedVectorFormat> packed_vector_formatOpt;
    if (packed_vector_format.valid)
    {
      packed_vector_formatOpt = packed_vector_format.value;
    }
    auto node = moduleBuilder.newNode<NodeOpSDot>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector_1), moduleBuilder.getNode(vector_2), packed_vector_formatOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSDotKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef vector_1, IdRef vector_2,
    Optional<PackedVectorFormat> packed_vector_format)
  {
    eastl::optional<PackedVectorFormat> packed_vector_formatOpt;
    if (packed_vector_format.valid)
    {
      packed_vector_formatOpt = packed_vector_format.value;
    }
    auto node = moduleBuilder.newNode<NodeOpSDotKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector_1), moduleBuilder.getNode(vector_2), packed_vector_formatOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUDot(Op, IdResult id_result, IdResultType id_result_type, IdRef vector_1, IdRef vector_2,
    Optional<PackedVectorFormat> packed_vector_format)
  {
    eastl::optional<PackedVectorFormat> packed_vector_formatOpt;
    if (packed_vector_format.valid)
    {
      packed_vector_formatOpt = packed_vector_format.value;
    }
    auto node = moduleBuilder.newNode<NodeOpUDot>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector_1), moduleBuilder.getNode(vector_2), packed_vector_formatOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUDotKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef vector_1, IdRef vector_2,
    Optional<PackedVectorFormat> packed_vector_format)
  {
    eastl::optional<PackedVectorFormat> packed_vector_formatOpt;
    if (packed_vector_format.valid)
    {
      packed_vector_formatOpt = packed_vector_format.value;
    }
    auto node = moduleBuilder.newNode<NodeOpUDotKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector_1), moduleBuilder.getNode(vector_2), packed_vector_formatOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSUDot(Op, IdResult id_result, IdResultType id_result_type, IdRef vector_1, IdRef vector_2,
    Optional<PackedVectorFormat> packed_vector_format)
  {
    eastl::optional<PackedVectorFormat> packed_vector_formatOpt;
    if (packed_vector_format.valid)
    {
      packed_vector_formatOpt = packed_vector_format.value;
    }
    auto node = moduleBuilder.newNode<NodeOpSUDot>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector_1), moduleBuilder.getNode(vector_2), packed_vector_formatOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSUDotKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef vector_1, IdRef vector_2,
    Optional<PackedVectorFormat> packed_vector_format)
  {
    eastl::optional<PackedVectorFormat> packed_vector_formatOpt;
    if (packed_vector_format.valid)
    {
      packed_vector_formatOpt = packed_vector_format.value;
    }
    auto node = moduleBuilder.newNode<NodeOpSUDotKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector_1), moduleBuilder.getNode(vector_2), packed_vector_formatOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSDotAccSat(Op, IdResult id_result, IdResultType id_result_type, IdRef vector_1, IdRef vector_2, IdRef accumulator,
    Optional<PackedVectorFormat> packed_vector_format)
  {
    eastl::optional<PackedVectorFormat> packed_vector_formatOpt;
    if (packed_vector_format.valid)
    {
      packed_vector_formatOpt = packed_vector_format.value;
    }
    auto node = moduleBuilder.newNode<NodeOpSDotAccSat>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector_1), moduleBuilder.getNode(vector_2), moduleBuilder.getNode(accumulator), packed_vector_formatOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSDotAccSatKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef vector_1, IdRef vector_2, IdRef accumulator,
    Optional<PackedVectorFormat> packed_vector_format)
  {
    eastl::optional<PackedVectorFormat> packed_vector_formatOpt;
    if (packed_vector_format.valid)
    {
      packed_vector_formatOpt = packed_vector_format.value;
    }
    auto node = moduleBuilder.newNode<NodeOpSDotAccSatKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector_1), moduleBuilder.getNode(vector_2), moduleBuilder.getNode(accumulator), packed_vector_formatOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUDotAccSat(Op, IdResult id_result, IdResultType id_result_type, IdRef vector_1, IdRef vector_2, IdRef accumulator,
    Optional<PackedVectorFormat> packed_vector_format)
  {
    eastl::optional<PackedVectorFormat> packed_vector_formatOpt;
    if (packed_vector_format.valid)
    {
      packed_vector_formatOpt = packed_vector_format.value;
    }
    auto node = moduleBuilder.newNode<NodeOpUDotAccSat>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector_1), moduleBuilder.getNode(vector_2), moduleBuilder.getNode(accumulator), packed_vector_formatOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUDotAccSatKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef vector_1, IdRef vector_2, IdRef accumulator,
    Optional<PackedVectorFormat> packed_vector_format)
  {
    eastl::optional<PackedVectorFormat> packed_vector_formatOpt;
    if (packed_vector_format.valid)
    {
      packed_vector_formatOpt = packed_vector_format.value;
    }
    auto node = moduleBuilder.newNode<NodeOpUDotAccSatKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector_1), moduleBuilder.getNode(vector_2), moduleBuilder.getNode(accumulator), packed_vector_formatOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSUDotAccSat(Op, IdResult id_result, IdResultType id_result_type, IdRef vector_1, IdRef vector_2, IdRef accumulator,
    Optional<PackedVectorFormat> packed_vector_format)
  {
    eastl::optional<PackedVectorFormat> packed_vector_formatOpt;
    if (packed_vector_format.valid)
    {
      packed_vector_formatOpt = packed_vector_format.value;
    }
    auto node = moduleBuilder.newNode<NodeOpSUDotAccSat>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector_1), moduleBuilder.getNode(vector_2), moduleBuilder.getNode(accumulator), packed_vector_formatOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSUDotAccSatKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef vector_1, IdRef vector_2, IdRef accumulator,
    Optional<PackedVectorFormat> packed_vector_format)
  {
    eastl::optional<PackedVectorFormat> packed_vector_formatOpt;
    if (packed_vector_format.valid)
    {
      packed_vector_formatOpt = packed_vector_format.value;
    }
    auto node = moduleBuilder.newNode<NodeOpSUDotAccSatKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(vector_1), moduleBuilder.getNode(vector_2), moduleBuilder.getNode(accumulator), packed_vector_formatOpt);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onTypeRayQueryKHR(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeRayQueryKHR>(id_result.value); }
  void onRayQueryInitializeKHR(Op, IdRef ray_query, IdRef accel, IdRef ray_flags, IdRef cull_mask, IdRef ray_origin, IdRef ray_t_min,
    IdRef ray_direction, IdRef ray_t_max)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryInitializeKHR>(moduleBuilder.getNode(ray_query), moduleBuilder.getNode(accel),
      moduleBuilder.getNode(ray_flags), moduleBuilder.getNode(cull_mask), moduleBuilder.getNode(ray_origin),
      moduleBuilder.getNode(ray_t_min), moduleBuilder.getNode(ray_direction), moduleBuilder.getNode(ray_t_max));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryTerminateKHR(Op, IdRef ray_query)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryTerminateKHR>(moduleBuilder.getNode(ray_query));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGenerateIntersectionKHR(Op, IdRef ray_query, IdRef hit_t)
  {
    auto node =
      moduleBuilder.newNode<NodeOpRayQueryGenerateIntersectionKHR>(moduleBuilder.getNode(ray_query), moduleBuilder.getNode(hit_t));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryConfirmIntersectionKHR(Op, IdRef ray_query)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryConfirmIntersectionKHR>(moduleBuilder.getNode(ray_query));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryProceedKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryProceedKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(ray_query));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetIntersectionTypeKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query, IdRef intersection)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetIntersectionTypeKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(ray_query), moduleBuilder.getNode(intersection));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupIAddNonUniformAMD(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupIAddNonUniformAMD>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupFAddNonUniformAMD(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupFAddNonUniformAMD>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupFMinNonUniformAMD(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupFMinNonUniformAMD>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupUMinNonUniformAMD(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupUMinNonUniformAMD>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupSMinNonUniformAMD(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupSMinNonUniformAMD>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupFMaxNonUniformAMD(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupFMaxNonUniformAMD>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupUMaxNonUniformAMD(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupUMaxNonUniformAMD>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupSMaxNonUniformAMD(Op, IdResult id_result, IdResultType id_result_type, IdScope execution, GroupOperation operation,
    IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupSMaxNonUniformAMD>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(execution), operation, moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFragmentMaskFetchAMD(Op, IdResult id_result, IdResultType id_result_type, IdRef image, IdRef coordinate)
  {
    auto node = moduleBuilder.newNode<NodeOpFragmentMaskFetchAMD>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image), moduleBuilder.getNode(coordinate));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFragmentFetchAMD(Op, IdResult id_result, IdResultType id_result_type, IdRef image, IdRef coordinate, IdRef fragment_index)
  {
    auto node = moduleBuilder.newNode<NodeOpFragmentFetchAMD>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image), moduleBuilder.getNode(coordinate), moduleBuilder.getNode(fragment_index));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onReadClockKHR(Op, IdResult id_result, IdResultType id_result_type, IdScope scope)
  {
    auto node =
      moduleBuilder.newNode<NodeOpReadClockKHR>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(scope));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onImageSampleFootprintNV(Op, IdResult id_result, IdResultType id_result_type, IdRef sampled_image, IdRef coordinate,
    IdRef granularity, IdRef coarse, Optional<TypeTraits<ImageOperandsMask>::ReadType> image_operands)
  {
    eastl::optional<ImageOperandsMask> image_operandsVal;
    NodePointer<NodeId> image_operands_Bias_first;
    NodePointer<NodeId> image_operands_Lod_first;
    NodePointer<NodeId> image_operands_Grad_first;
    NodePointer<NodeId> image_operands_Grad_second;
    NodePointer<NodeId> image_operands_ConstOffset_first;
    NodePointer<NodeId> image_operands_Offset_first;
    NodePointer<NodeId> image_operands_ConstOffsets_first;
    NodePointer<NodeId> image_operands_Sample_first;
    NodePointer<NodeId> image_operands_MinLod_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailable_first;
    NodePointer<NodeOperation> image_operands_MakeTexelAvailableKHR_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisible_first;
    NodePointer<NodeOperation> image_operands_MakeTexelVisibleKHR_first;
    NodePointer<NodeId> image_operands_Offsets_first;
    if (image_operands.valid)
    {
      image_operandsVal = image_operands.value.value;
      if ((image_operands.value.value & ImageOperandsMask::Bias) != ImageOperandsMask::MaskNone)
      {
        image_operands_Bias_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Bias.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Lod) != ImageOperandsMask::MaskNone)
      {
        image_operands_Lod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Lod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Grad) != ImageOperandsMask::MaskNone)
      {
        image_operands_Grad_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.first));
        image_operands_Grad_second = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Grad.second));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffset) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offset) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offset_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offset.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::ConstOffsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_ConstOffsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.ConstOffsets.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Sample) != ImageOperandsMask::MaskNone)
      {
        image_operands_Sample_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Sample.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MinLod) != ImageOperandsMask::MaskNone)
      {
        image_operands_MinLod_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.MinLod.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailable) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailable.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelAvailableKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelAvailableKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisible) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisible.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::MakeTexelVisibleKHR) != ImageOperandsMask::MaskNone)
      {
        image_operands_MakeTexelVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(image_operands.value.data.MakeTexelVisibleKHR.first));
      }
      if ((image_operands.value.value & ImageOperandsMask::Offsets) != ImageOperandsMask::MaskNone)
      {
        image_operands_Offsets_first = NodePointer<NodeId>(moduleBuilder.getNode(image_operands.value.data.Offsets.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpImageSampleFootprintNV>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(sampled_image), moduleBuilder.getNode(coordinate), moduleBuilder.getNode(granularity),
      moduleBuilder.getNode(coarse), image_operandsVal, image_operands_Bias_first, image_operands_Lod_first, image_operands_Grad_first,
      image_operands_Grad_second, image_operands_ConstOffset_first, image_operands_Offset_first, image_operands_ConstOffsets_first,
      image_operands_Sample_first, image_operands_MinLod_first, image_operands_MakeTexelAvailable_first,
      image_operands_MakeTexelAvailableKHR_first, image_operands_MakeTexelVisible_first, image_operands_MakeTexelVisibleKHR_first,
      image_operands_Offsets_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGroupNonUniformPartitionNV(Op, IdResult id_result, IdResultType id_result_type, IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpGroupNonUniformPartitionNV>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onWritePackedPrimitiveIndices4x8NV(Op, IdRef index_offset, IdRef packed_indices)
  {
    auto node = moduleBuilder.newNode<NodeOpWritePackedPrimitiveIndices4x8NV>(moduleBuilder.getNode(index_offset),
      moduleBuilder.getNode(packed_indices));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onReportIntersectionNV(Op, IdResult id_result, IdResultType id_result_type, IdRef hit, IdRef hit_kind)
  {
    auto node = moduleBuilder.newNode<NodeOpReportIntersectionNV>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(hit), moduleBuilder.getNode(hit_kind));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onReportIntersectionKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef hit, IdRef hit_kind)
  {
    auto node = moduleBuilder.newNode<NodeOpReportIntersectionKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(hit), moduleBuilder.getNode(hit_kind));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIgnoreIntersectionNV(Op)
  {
    auto node = moduleBuilder.newNode<NodeOpIgnoreIntersectionNV>();
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onTerminateRayNV(Op)
  {
    auto node = moduleBuilder.newNode<NodeOpTerminateRayNV>();
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onTraceNV(Op, IdRef accel, IdRef ray_flags, IdRef cull_mask, IdRef s_b_t_offset, IdRef s_b_t_stride, IdRef miss_index,
    IdRef ray_origin, IdRef ray_tmin, IdRef ray_direction, IdRef ray_tmax, IdRef payload_id)
  {
    auto node = moduleBuilder.newNode<NodeOpTraceNV>(moduleBuilder.getNode(accel), moduleBuilder.getNode(ray_flags),
      moduleBuilder.getNode(cull_mask), moduleBuilder.getNode(s_b_t_offset), moduleBuilder.getNode(s_b_t_stride),
      moduleBuilder.getNode(miss_index), moduleBuilder.getNode(ray_origin), moduleBuilder.getNode(ray_tmin),
      moduleBuilder.getNode(ray_direction), moduleBuilder.getNode(ray_tmax), moduleBuilder.getNode(payload_id));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onTraceMotionNV(Op, IdRef accel, IdRef ray_flags, IdRef cull_mask, IdRef s_b_t_offset, IdRef s_b_t_stride, IdRef miss_index,
    IdRef ray_origin, IdRef ray_tmin, IdRef ray_direction, IdRef ray_tmax, IdRef time, IdRef payload_id)
  {
    auto node = moduleBuilder.newNode<NodeOpTraceMotionNV>(moduleBuilder.getNode(accel), moduleBuilder.getNode(ray_flags),
      moduleBuilder.getNode(cull_mask), moduleBuilder.getNode(s_b_t_offset), moduleBuilder.getNode(s_b_t_stride),
      moduleBuilder.getNode(miss_index), moduleBuilder.getNode(ray_origin), moduleBuilder.getNode(ray_tmin),
      moduleBuilder.getNode(ray_direction), moduleBuilder.getNode(ray_tmax), moduleBuilder.getNode(time),
      moduleBuilder.getNode(payload_id));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onTraceRayMotionNV(Op, IdRef accel, IdRef ray_flags, IdRef cull_mask, IdRef s_b_t_offset, IdRef s_b_t_stride, IdRef miss_index,
    IdRef ray_origin, IdRef ray_tmin, IdRef ray_direction, IdRef ray_tmax, IdRef time, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpTraceRayMotionNV>(moduleBuilder.getNode(accel), moduleBuilder.getNode(ray_flags),
      moduleBuilder.getNode(cull_mask), moduleBuilder.getNode(s_b_t_offset), moduleBuilder.getNode(s_b_t_stride),
      moduleBuilder.getNode(miss_index), moduleBuilder.getNode(ray_origin), moduleBuilder.getNode(ray_tmin),
      moduleBuilder.getNode(ray_direction), moduleBuilder.getNode(ray_tmax), moduleBuilder.getNode(time),
      moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onTypeAccelerationStructureNV(Op, IdResult id_result)
  {
    moduleBuilder.newNode<NodeOpTypeAccelerationStructureNV>(id_result.value);
  }
  void onTypeAccelerationStructureKHR(Op, IdResult id_result)
  {
    moduleBuilder.newNode<NodeOpTypeAccelerationStructureKHR>(id_result.value);
  }
  void onExecuteCallableNV(Op, IdRef s_b_t_index, IdRef callable_data_id)
  {
    auto node =
      moduleBuilder.newNode<NodeOpExecuteCallableNV>(moduleBuilder.getNode(s_b_t_index), moduleBuilder.getNode(callable_data_id));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onTypeCooperativeMatrixNV(Op, IdResult id_result, IdRef component_type, IdScope execution, IdRef rows, IdRef columns)
  {
    moduleBuilder.newNode<NodeOpTypeCooperativeMatrixNV>(id_result.value, moduleBuilder.getNode(component_type),
      moduleBuilder.getNode(execution), moduleBuilder.getNode(rows), moduleBuilder.getNode(columns));
  }
  void onCooperativeMatrixLoadNV(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdRef stride, IdRef column_major,
    Optional<TypeTraits<MemoryAccessMask>::ReadType> memory_access)
  {
    eastl::optional<MemoryAccessMask> memory_accessVal;
    eastl::optional<LiteralInteger> memory_access_Aligned_first;
    NodePointer<NodeOperation> memory_access_MakePointerAvailable_first;
    NodePointer<NodeOperation> memory_access_MakePointerAvailableKHR_first;
    NodePointer<NodeOperation> memory_access_MakePointerVisible_first;
    NodePointer<NodeOperation> memory_access_MakePointerVisibleKHR_first;
    if (memory_access.valid)
    {
      memory_accessVal = memory_access.value.value;
      if ((memory_access.value.value & MemoryAccessMask::Aligned) != MemoryAccessMask::MaskNone)
      {
        memory_access_Aligned_first = memory_access.value.data.Aligned.first;
      }
      if ((memory_access.value.value & MemoryAccessMask::MakePointerAvailable) != MemoryAccessMask::MaskNone)
      {
        memory_access_MakePointerAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access.value.data.MakePointerAvailable.first));
      }
      if ((memory_access.value.value & MemoryAccessMask::MakePointerAvailableKHR) != MemoryAccessMask::MaskNone)
      {
        memory_access_MakePointerAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access.value.data.MakePointerAvailableKHR.first));
      }
      if ((memory_access.value.value & MemoryAccessMask::MakePointerVisible) != MemoryAccessMask::MaskNone)
      {
        memory_access_MakePointerVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access.value.data.MakePointerVisible.first));
      }
      if ((memory_access.value.value & MemoryAccessMask::MakePointerVisibleKHR) != MemoryAccessMask::MaskNone)
      {
        memory_access_MakePointerVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access.value.data.MakePointerVisibleKHR.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpCooperativeMatrixLoadNV>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(stride), moduleBuilder.getNode(column_major), memory_accessVal,
      memory_access_Aligned_first, memory_access_MakePointerAvailable_first, memory_access_MakePointerAvailableKHR_first,
      memory_access_MakePointerVisible_first, memory_access_MakePointerVisibleKHR_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onCooperativeMatrixStoreNV(Op, IdRef pointer, IdRef object, IdRef stride, IdRef column_major,
    Optional<TypeTraits<MemoryAccessMask>::ReadType> memory_access)
  {
    eastl::optional<MemoryAccessMask> memory_accessVal;
    eastl::optional<LiteralInteger> memory_access_Aligned_first;
    NodePointer<NodeOperation> memory_access_MakePointerAvailable_first;
    NodePointer<NodeOperation> memory_access_MakePointerAvailableKHR_first;
    NodePointer<NodeOperation> memory_access_MakePointerVisible_first;
    NodePointer<NodeOperation> memory_access_MakePointerVisibleKHR_first;
    if (memory_access.valid)
    {
      memory_accessVal = memory_access.value.value;
      if ((memory_access.value.value & MemoryAccessMask::Aligned) != MemoryAccessMask::MaskNone)
      {
        memory_access_Aligned_first = memory_access.value.data.Aligned.first;
      }
      if ((memory_access.value.value & MemoryAccessMask::MakePointerAvailable) != MemoryAccessMask::MaskNone)
      {
        memory_access_MakePointerAvailable_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access.value.data.MakePointerAvailable.first));
      }
      if ((memory_access.value.value & MemoryAccessMask::MakePointerAvailableKHR) != MemoryAccessMask::MaskNone)
      {
        memory_access_MakePointerAvailableKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access.value.data.MakePointerAvailableKHR.first));
      }
      if ((memory_access.value.value & MemoryAccessMask::MakePointerVisible) != MemoryAccessMask::MaskNone)
      {
        memory_access_MakePointerVisible_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access.value.data.MakePointerVisible.first));
      }
      if ((memory_access.value.value & MemoryAccessMask::MakePointerVisibleKHR) != MemoryAccessMask::MaskNone)
      {
        memory_access_MakePointerVisibleKHR_first =
          NodePointer<NodeOperation>(moduleBuilder.getNode(memory_access.value.data.MakePointerVisibleKHR.first));
      }
    }
    auto node = moduleBuilder.newNode<NodeOpCooperativeMatrixStoreNV>(moduleBuilder.getNode(pointer), moduleBuilder.getNode(object),
      moduleBuilder.getNode(stride), moduleBuilder.getNode(column_major), memory_accessVal, memory_access_Aligned_first,
      memory_access_MakePointerAvailable_first, memory_access_MakePointerAvailableKHR_first, memory_access_MakePointerVisible_first,
      memory_access_MakePointerVisibleKHR_first);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onCooperativeMatrixMulAddNV(Op, IdResult id_result, IdResultType id_result_type, IdRef a, IdRef b, IdRef c)
  {
    auto node = moduleBuilder.newNode<NodeOpCooperativeMatrixMulAddNV>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), moduleBuilder.getNode(b), moduleBuilder.getNode(c));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onCooperativeMatrixLengthNV(Op, IdResult id_result, IdResultType id_result_type, IdRef type)
  {
    auto node = moduleBuilder.newNode<NodeOpCooperativeMatrixLengthNV>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onBeginInvocationInterlockEXT(Op)
  {
    auto node = moduleBuilder.newNode<NodeOpBeginInvocationInterlockEXT>();
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onEndInvocationInterlockEXT(Op)
  {
    auto node = moduleBuilder.newNode<NodeOpEndInvocationInterlockEXT>();
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onDemoteToHelperInvocation(Op)
  {
    auto node = moduleBuilder.newNode<NodeOpDemoteToHelperInvocation>();
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onDemoteToHelperInvocationEXT(Op)
  {
    auto node = moduleBuilder.newNode<NodeOpDemoteToHelperInvocationEXT>();
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIsHelperInvocationEXT(Op, IdResult id_result, IdResultType id_result_type)
  {
    auto node = moduleBuilder.newNode<NodeOpIsHelperInvocationEXT>(id_result.value, moduleBuilder.getType(id_result_type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConvertUToImageNV(Op, IdResult id_result, IdResultType id_result_type, IdRef operand)
  {
    auto node = moduleBuilder.newNode<NodeOpConvertUToImageNV>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConvertUToSamplerNV(Op, IdResult id_result, IdResultType id_result_type, IdRef operand)
  {
    auto node = moduleBuilder.newNode<NodeOpConvertUToSamplerNV>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConvertImageToUNV(Op, IdResult id_result, IdResultType id_result_type, IdRef operand)
  {
    auto node = moduleBuilder.newNode<NodeOpConvertImageToUNV>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConvertSamplerToUNV(Op, IdResult id_result, IdResultType id_result_type, IdRef operand)
  {
    auto node = moduleBuilder.newNode<NodeOpConvertSamplerToUNV>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConvertUToSampledImageNV(Op, IdResult id_result, IdResultType id_result_type, IdRef operand)
  {
    auto node = moduleBuilder.newNode<NodeOpConvertUToSampledImageNV>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConvertSampledImageToUNV(Op, IdResult id_result, IdResultType id_result_type, IdRef operand)
  {
    auto node = moduleBuilder.newNode<NodeOpConvertSampledImageToUNV>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSamplerImageAddressingModeNV(Op, LiteralInteger bit_width) {}
  void onSubgroupShuffleINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef data, IdRef invocation_id)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupShuffleINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(data), moduleBuilder.getNode(invocation_id));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupShuffleDownINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef current, IdRef next, IdRef delta)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupShuffleDownINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(current), moduleBuilder.getNode(next), moduleBuilder.getNode(delta));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupShuffleUpINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef previous, IdRef current, IdRef delta)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupShuffleUpINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(previous), moduleBuilder.getNode(current), moduleBuilder.getNode(delta));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupShuffleXorINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef data, IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupShuffleXorINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(data), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupBlockReadINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef ptr)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupBlockReadINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(ptr));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupBlockWriteINTEL(Op, IdRef ptr, IdRef data)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupBlockWriteINTEL>(moduleBuilder.getNode(ptr), moduleBuilder.getNode(data));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupImageBlockReadINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef image, IdRef coordinate)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupImageBlockReadINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image), moduleBuilder.getNode(coordinate));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupImageBlockWriteINTEL(Op, IdRef image, IdRef coordinate, IdRef data)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupImageBlockWriteINTEL>(moduleBuilder.getNode(image),
      moduleBuilder.getNode(coordinate), moduleBuilder.getNode(data));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupImageMediaBlockReadINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef image, IdRef coordinate,
    IdRef width, IdRef height)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupImageMediaBlockReadINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image), moduleBuilder.getNode(coordinate), moduleBuilder.getNode(width), moduleBuilder.getNode(height));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupImageMediaBlockWriteINTEL(Op, IdRef image, IdRef coordinate, IdRef width, IdRef height, IdRef data)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupImageMediaBlockWriteINTEL>(moduleBuilder.getNode(image),
      moduleBuilder.getNode(coordinate), moduleBuilder.getNode(width), moduleBuilder.getNode(height), moduleBuilder.getNode(data));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUCountLeadingZerosINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef operand)
  {
    auto node = moduleBuilder.newNode<NodeOpUCountLeadingZerosINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUCountTrailingZerosINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef operand)
  {
    auto node = moduleBuilder.newNode<NodeOpUCountTrailingZerosINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAbsISubINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpAbsISubINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAbsUSubINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpAbsUSubINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIAddSatINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpIAddSatINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUAddSatINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpUAddSatINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIAverageINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpIAverageINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUAverageINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpUAverageINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIAverageRoundedINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpIAverageRoundedINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUAverageRoundedINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpUAverageRoundedINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onISubSatINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpISubSatINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUSubSatINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpUSubSatINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onIMul32x16INTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpIMul32x16INTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onUMul32x16INTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef operand_1, IdRef operand_2)
  {
    auto node = moduleBuilder.newNode<NodeOpUMul32x16INTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(operand_1), moduleBuilder.getNode(operand_2));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onConstantFunctionPointerINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef function)
  {
    auto node = moduleBuilder.newNode<NodeOpConstantFunctionPointerINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(function));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFunctionPointerCallINTEL(Op, IdResult id_result, IdResultType id_result_type, Multiple<IdRef> operand_1)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<NodePointer<NodeId>> operand_1Var;
    while (!operand_1.empty())
    {
      operand_1Var.push_back(moduleBuilder.getNode(operand_1.consume()));
    }
    auto node = moduleBuilder.newNode<NodeOpFunctionPointerCallINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      operand_1Var.data(), operand_1Var.size());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAsmTargetINTEL(Op, IdResult id_result, IdResultType id_result_type, LiteralString asm_target)
  {
    auto node =
      moduleBuilder.newNode<NodeOpAsmTargetINTEL>(id_result.value, moduleBuilder.getType(id_result_type), asm_target.asStringObj());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAsmINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef asm_type, IdRef target, LiteralString asm_instructions,
    LiteralString constraints)
  {
    auto node = moduleBuilder.newNode<NodeOpAsmINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(asm_type), moduleBuilder.getNode(target), asm_instructions.asStringObj(), constraints.asStringObj());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAsmCallINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef not_reserved_asm, Multiple<IdRef> argument_0)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<NodePointer<NodeId>> argument_0Var;
    while (!argument_0.empty())
    {
      argument_0Var.push_back(moduleBuilder.getNode(argument_0.consume()));
    }
    auto node = moduleBuilder.newNode<NodeOpAsmCallINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(not_reserved_asm), argument_0Var.data(), argument_0Var.size());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicFMinEXT(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory, IdMemorySemantics semantics,
    IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicFMinEXT>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicFMaxEXT(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory, IdMemorySemantics semantics,
    IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicFMaxEXT>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAssumeTrueKHR(Op, IdRef condition)
  {
    auto node = moduleBuilder.newNode<NodeOpAssumeTrueKHR>(moduleBuilder.getNode(condition));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onExpectKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef value, IdRef expected_value)
  {
    auto node = moduleBuilder.newNode<NodeOpExpectKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(value), moduleBuilder.getNode(expected_value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onDecorateString(Op, IdRef target, TypeTraits<Decoration>::ReadType decoration)
  {
    CastableUniquePointer<Property> propPtr;
    switch (decoration.value)
    {
      case Decoration::RelaxedPrecision:
      {
        auto newProp = new PropertyRelaxedPrecision;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::SpecId:
      {
        auto newProp = new PropertySpecId;
        propPtr.reset(newProp);
        newProp->specializationConstantId = decoration.data.SpecId.specializationConstantId;
        break;
      }
      case Decoration::Block:
      {
        auto newProp = new PropertyBlock;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BufferBlock:
      {
        auto newProp = new PropertyBufferBlock;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::RowMajor:
      {
        auto newProp = new PropertyRowMajor;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ColMajor:
      {
        auto newProp = new PropertyColMajor;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ArrayStride:
      {
        auto newProp = new PropertyArrayStride;
        propPtr.reset(newProp);
        newProp->arrayStride = decoration.data.ArrayStride.arrayStride;
        break;
      }
      case Decoration::MatrixStride:
      {
        auto newProp = new PropertyMatrixStride;
        propPtr.reset(newProp);
        newProp->matrixStride = decoration.data.MatrixStride.matrixStride;
        break;
      }
      case Decoration::GLSLShared:
      {
        auto newProp = new PropertyGLSLShared;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::GLSLPacked:
      {
        auto newProp = new PropertyGLSLPacked;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::CPacked:
      {
        auto newProp = new PropertyCPacked;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BuiltIn:
      {
        auto newProp = new PropertyBuiltIn;
        propPtr.reset(newProp);
        newProp->builtIn = decoration.data.BuiltInData.first;
        break;
      }
      case Decoration::NoPerspective:
      {
        auto newProp = new PropertyNoPerspective;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Flat:
      {
        auto newProp = new PropertyFlat;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Patch:
      {
        auto newProp = new PropertyPatch;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Centroid:
      {
        auto newProp = new PropertyCentroid;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Sample:
      {
        auto newProp = new PropertySample;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Invariant:
      {
        auto newProp = new PropertyInvariant;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Restrict:
      {
        auto newProp = new PropertyRestrict;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Aliased:
      {
        auto newProp = new PropertyAliased;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Volatile:
      {
        auto newProp = new PropertyVolatile;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Constant:
      {
        auto newProp = new PropertyConstant;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Coherent:
      {
        auto newProp = new PropertyCoherent;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::NonWritable:
      {
        auto newProp = new PropertyNonWritable;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::NonReadable:
      {
        auto newProp = new PropertyNonReadable;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Uniform:
      {
        auto newProp = new PropertyUniform;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::UniformId:
      {
        auto newProp = new PropertyUniformId;
        propPtr.reset(newProp);
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->execution), IdRef{decoration.data.UniformId.execution.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::SaturatedConversion:
      {
        auto newProp = new PropertySaturatedConversion;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Stream:
      {
        auto newProp = new PropertyStream;
        propPtr.reset(newProp);
        newProp->streamNumber = decoration.data.Stream.streamNumber;
        break;
      }
      case Decoration::Location:
      {
        auto newProp = new PropertyLocation;
        propPtr.reset(newProp);
        newProp->location = decoration.data.Location.location;
        break;
      }
      case Decoration::Component:
      {
        auto newProp = new PropertyComponent;
        propPtr.reset(newProp);
        newProp->component = decoration.data.Component.component;
        break;
      }
      case Decoration::Index:
      {
        auto newProp = new PropertyIndex;
        propPtr.reset(newProp);
        newProp->index = decoration.data.Index.index;
        break;
      }
      case Decoration::Binding:
      {
        auto newProp = new PropertyBinding;
        propPtr.reset(newProp);
        newProp->bindingPoint = decoration.data.Binding.bindingPoint;
        break;
      }
      case Decoration::DescriptorSet:
      {
        auto newProp = new PropertyDescriptorSet;
        propPtr.reset(newProp);
        newProp->descriptorSet = decoration.data.DescriptorSet.descriptorSet;
        break;
      }
      case Decoration::Offset:
      {
        auto newProp = new PropertyOffset;
        propPtr.reset(newProp);
        newProp->byteOffset = decoration.data.Offset.byteOffset;
        break;
      }
      case Decoration::XfbBuffer:
      {
        auto newProp = new PropertyXfbBuffer;
        propPtr.reset(newProp);
        newProp->xfbBufferNumber = decoration.data.XfbBuffer.xfbBufferNumber;
        break;
      }
      case Decoration::XfbStride:
      {
        auto newProp = new PropertyXfbStride;
        propPtr.reset(newProp);
        newProp->xfbStride = decoration.data.XfbStride.xfbStride;
        break;
      }
      case Decoration::FuncParamAttr:
      {
        auto newProp = new PropertyFuncParamAttr;
        propPtr.reset(newProp);
        newProp->functionParameterAttribute = decoration.data.FuncParamAttr.functionParameterAttribute;
        break;
      }
      case Decoration::FPRoundingMode:
      {
        auto newProp = new PropertyFPRoundingMode;
        propPtr.reset(newProp);
        newProp->floatingPointRoundingMode = decoration.data.FPRoundingMode.floatingPointRoundingMode;
        break;
      }
      case Decoration::FPFastMathMode:
      {
        auto newProp = new PropertyFPFastMathMode;
        propPtr.reset(newProp);
        newProp->fastMathMode = decoration.data.FPFastMathMode.fastMathMode;
        break;
      }
      case Decoration::LinkageAttributes:
      {
        auto newProp = new PropertyLinkageAttributes;
        propPtr.reset(newProp);
        newProp->name = decoration.data.LinkageAttributes.name.asStringObj();
        newProp->linkageType = decoration.data.LinkageAttributes.linkageType;
        break;
      }
      case Decoration::NoContraction:
      {
        auto newProp = new PropertyNoContraction;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::InputAttachmentIndex:
      {
        auto newProp = new PropertyInputAttachmentIndex;
        propPtr.reset(newProp);
        newProp->attachmentIndex = decoration.data.InputAttachmentIndex.attachmentIndex;
        break;
      }
      case Decoration::Alignment:
      {
        auto newProp = new PropertyAlignment;
        propPtr.reset(newProp);
        newProp->alignment = decoration.data.Alignment.alignment;
        break;
      }
      case Decoration::MaxByteOffset:
      {
        auto newProp = new PropertyMaxByteOffset;
        propPtr.reset(newProp);
        newProp->maxByteOffset = decoration.data.MaxByteOffset.maxByteOffset;
        break;
      }
      case Decoration::AlignmentId:
      {
        auto newProp = new PropertyAlignmentId;
        propPtr.reset(newProp);
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->alignment), IdRef{decoration.data.AlignmentId.alignment.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::MaxByteOffsetId:
      {
        auto newProp = new PropertyMaxByteOffsetId;
        propPtr.reset(newProp);
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->maxByteOffset),
            IdRef{decoration.data.MaxByteOffsetId.maxByteOffset.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::NoSignedWrap:
      {
        auto newProp = new PropertyNoSignedWrap;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::NoUnsignedWrap:
      {
        auto newProp = new PropertyNoUnsignedWrap;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ExplicitInterpAMD:
      {
        auto newProp = new PropertyExplicitInterpAMD;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::OverrideCoverageNV:
      {
        auto newProp = new PropertyOverrideCoverageNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PassthroughNV:
      {
        auto newProp = new PropertyPassthroughNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ViewportRelativeNV:
      {
        auto newProp = new PropertyViewportRelativeNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::SecondaryViewportRelativeNV:
      {
        auto newProp = new PropertySecondaryViewportRelativeNV;
        propPtr.reset(newProp);
        newProp->offset = decoration.data.SecondaryViewportRelativeNV.offset;
        break;
      }
      case Decoration::PerPrimitiveNV:
      {
        auto newProp = new PropertyPerPrimitiveNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PerViewNV:
      {
        auto newProp = new PropertyPerViewNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PerTaskNV:
      {
        auto newProp = new PropertyPerTaskNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PerVertexKHR:
      {
        auto newProp = new PropertyPerVertexKHR;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::NonUniform:
      {
        auto newProp = new PropertyNonUniform;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::RestrictPointer:
      {
        auto newProp = new PropertyRestrictPointer;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::AliasedPointer:
      {
        auto newProp = new PropertyAliasedPointer;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BindlessSamplerNV:
      {
        auto newProp = new PropertyBindlessSamplerNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BindlessImageNV:
      {
        auto newProp = new PropertyBindlessImageNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BoundSamplerNV:
      {
        auto newProp = new PropertyBoundSamplerNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BoundImageNV:
      {
        auto newProp = new PropertyBoundImageNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::SIMTCallINTEL:
      {
        auto newProp = new PropertySIMTCallINTEL;
        propPtr.reset(newProp);
        newProp->n = decoration.data.SIMTCallINTEL.n;
        break;
      }
      case Decoration::ReferencedIndirectlyINTEL:
      {
        auto newProp = new PropertyReferencedIndirectlyINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ClobberINTEL:
      {
        auto newProp = new PropertyClobberINTEL;
        propPtr.reset(newProp);
        newProp->_register = decoration.data.ClobberINTEL._register.asStringObj();
        break;
      }
      case Decoration::SideEffectsINTEL:
      {
        auto newProp = new PropertySideEffectsINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::VectorComputeVariableINTEL:
      {
        auto newProp = new PropertyVectorComputeVariableINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::FuncParamIOKindINTEL:
      {
        auto newProp = new PropertyFuncParamIOKindINTEL;
        propPtr.reset(newProp);
        newProp->kind = decoration.data.FuncParamIOKindINTEL.kind;
        break;
      }
      case Decoration::VectorComputeFunctionINTEL:
      {
        auto newProp = new PropertyVectorComputeFunctionINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::StackCallINTEL:
      {
        auto newProp = new PropertyStackCallINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::GlobalVariableOffsetINTEL:
      {
        auto newProp = new PropertyGlobalVariableOffsetINTEL;
        propPtr.reset(newProp);
        newProp->offset = decoration.data.GlobalVariableOffsetINTEL.offset;
        break;
      }
      case Decoration::CounterBuffer:
      {
        auto newProp = new PropertyCounterBuffer;
        propPtr.reset(newProp);
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->counterBuffer), IdRef{decoration.data.CounterBuffer.counterBuffer.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::UserSemantic:
      {
        auto newProp = new PropertyUserSemantic;
        propPtr.reset(newProp);
        newProp->semantic = decoration.data.UserSemantic.semantic.asStringObj();
        break;
      }
      case Decoration::UserTypeGOOGLE:
      {
        auto newProp = new PropertyUserTypeGOOGLE;
        propPtr.reset(newProp);
        newProp->userType = decoration.data.UserTypeGOOGLE.userType.asStringObj();
        break;
      }
      case Decoration::FunctionRoundingModeINTEL:
      {
        auto newProp = new PropertyFunctionRoundingModeINTEL;
        propPtr.reset(newProp);
        newProp->targetWidth = decoration.data.FunctionRoundingModeINTEL.targetWidth;
        newProp->fpRoundingMode = decoration.data.FunctionRoundingModeINTEL.fpRoundingMode;
        break;
      }
      case Decoration::FunctionDenormModeINTEL:
      {
        auto newProp = new PropertyFunctionDenormModeINTEL;
        propPtr.reset(newProp);
        newProp->targetWidth = decoration.data.FunctionDenormModeINTEL.targetWidth;
        newProp->fpDenormMode = decoration.data.FunctionDenormModeINTEL.fpDenormMode;
        break;
      }
      case Decoration::RegisterINTEL:
      {
        auto newProp = new PropertyRegisterINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::MemoryINTEL:
      {
        auto newProp = new PropertyMemoryINTEL;
        propPtr.reset(newProp);
        newProp->memoryType = decoration.data.MemoryINTEL.memoryType.asStringObj();
        break;
      }
      case Decoration::NumbanksINTEL:
      {
        auto newProp = new PropertyNumbanksINTEL;
        propPtr.reset(newProp);
        newProp->banks = decoration.data.NumbanksINTEL.banks;
        break;
      }
      case Decoration::BankwidthINTEL:
      {
        auto newProp = new PropertyBankwidthINTEL;
        propPtr.reset(newProp);
        newProp->bankWidth = decoration.data.BankwidthINTEL.bankWidth;
        break;
      }
      case Decoration::MaxPrivateCopiesINTEL:
      {
        auto newProp = new PropertyMaxPrivateCopiesINTEL;
        propPtr.reset(newProp);
        newProp->maximumCopies = decoration.data.MaxPrivateCopiesINTEL.maximumCopies;
        break;
      }
      case Decoration::SinglepumpINTEL:
      {
        auto newProp = new PropertySinglepumpINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::DoublepumpINTEL:
      {
        auto newProp = new PropertyDoublepumpINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::MaxReplicatesINTEL:
      {
        auto newProp = new PropertyMaxReplicatesINTEL;
        propPtr.reset(newProp);
        newProp->maximumReplicates = decoration.data.MaxReplicatesINTEL.maximumReplicates;
        break;
      }
      case Decoration::SimpleDualPortINTEL:
      {
        auto newProp = new PropertySimpleDualPortINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::MergeINTEL:
      {
        auto newProp = new PropertyMergeINTEL;
        propPtr.reset(newProp);
        newProp->mergeKey = decoration.data.MergeINTEL.mergeKey.asStringObj();
        newProp->mergeType = decoration.data.MergeINTEL.mergeType.asStringObj();
        break;
      }
      case Decoration::BankBitsINTEL:
      {
        auto newProp = new PropertyBankBitsINTEL;
        propPtr.reset(newProp);
        newProp->bankBits = decoration.data.BankBitsINTEL.bankBits;
        break;
      }
      case Decoration::ForcePow2DepthINTEL:
      {
        auto newProp = new PropertyForcePow2DepthINTEL;
        propPtr.reset(newProp);
        newProp->forceKey = decoration.data.ForcePow2DepthINTEL.forceKey;
        break;
      }
      case Decoration::BurstCoalesceINTEL:
      {
        auto newProp = new PropertyBurstCoalesceINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::CacheSizeINTEL:
      {
        auto newProp = new PropertyCacheSizeINTEL;
        propPtr.reset(newProp);
        newProp->cacheSizeInBytes = decoration.data.CacheSizeINTEL.cacheSizeInBytes;
        break;
      }
      case Decoration::DontStaticallyCoalesceINTEL:
      {
        auto newProp = new PropertyDontStaticallyCoalesceINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PrefetchINTEL:
      {
        auto newProp = new PropertyPrefetchINTEL;
        propPtr.reset(newProp);
        newProp->prefetcherSizeInBytes = decoration.data.PrefetchINTEL.prefetcherSizeInBytes;
        break;
      }
      case Decoration::StallEnableINTEL:
      {
        auto newProp = new PropertyStallEnableINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::FuseLoopsInFunctionINTEL:
      {
        auto newProp = new PropertyFuseLoopsInFunctionINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BufferLocationINTEL:
      {
        auto newProp = new PropertyBufferLocationINTEL;
        propPtr.reset(newProp);
        newProp->bufferLocationId = decoration.data.BufferLocationINTEL.bufferLocationId;
        break;
      }
      case Decoration::IOPipeStorageINTEL:
      {
        auto newProp = new PropertyIOPipeStorageINTEL;
        propPtr.reset(newProp);
        newProp->ioPipeId = decoration.data.IOPipeStorageINTEL.ioPipeId;
        break;
      }
      case Decoration::FunctionFloatingPointModeINTEL:
      {
        auto newProp = new PropertyFunctionFloatingPointModeINTEL;
        propPtr.reset(newProp);
        newProp->targetWidth = decoration.data.FunctionFloatingPointModeINTEL.targetWidth;
        newProp->fpOperationMode = decoration.data.FunctionFloatingPointModeINTEL.fpOperationMode;
        break;
      }
      case Decoration::SingleElementVectorINTEL:
      {
        auto newProp = new PropertySingleElementVectorINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::VectorComputeCallableFunctionINTEL:
      {
        auto newProp = new PropertyVectorComputeCallableFunctionINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::MediaBlockIOINTEL:
      {
        auto newProp = new PropertyMediaBlockIOINTEL;
        propPtr.reset(newProp);
        break;
      }
    }
    PropertyTargetResolveInfo targetResolve //
      {target, eastl::move(propPtr)};
    propertyTargetResolves.push_back(eastl::move(targetResolve));
  }
  void onDecorateStringGOOGLE(Op, IdRef target, TypeTraits<Decoration>::ReadType decoration)
  {
    CastableUniquePointer<Property> propPtr;
    switch (decoration.value)
    {
      case Decoration::RelaxedPrecision:
      {
        auto newProp = new PropertyRelaxedPrecision;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::SpecId:
      {
        auto newProp = new PropertySpecId;
        propPtr.reset(newProp);
        newProp->specializationConstantId = decoration.data.SpecId.specializationConstantId;
        break;
      }
      case Decoration::Block:
      {
        auto newProp = new PropertyBlock;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BufferBlock:
      {
        auto newProp = new PropertyBufferBlock;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::RowMajor:
      {
        auto newProp = new PropertyRowMajor;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ColMajor:
      {
        auto newProp = new PropertyColMajor;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ArrayStride:
      {
        auto newProp = new PropertyArrayStride;
        propPtr.reset(newProp);
        newProp->arrayStride = decoration.data.ArrayStride.arrayStride;
        break;
      }
      case Decoration::MatrixStride:
      {
        auto newProp = new PropertyMatrixStride;
        propPtr.reset(newProp);
        newProp->matrixStride = decoration.data.MatrixStride.matrixStride;
        break;
      }
      case Decoration::GLSLShared:
      {
        auto newProp = new PropertyGLSLShared;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::GLSLPacked:
      {
        auto newProp = new PropertyGLSLPacked;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::CPacked:
      {
        auto newProp = new PropertyCPacked;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BuiltIn:
      {
        auto newProp = new PropertyBuiltIn;
        propPtr.reset(newProp);
        newProp->builtIn = decoration.data.BuiltInData.first;
        break;
      }
      case Decoration::NoPerspective:
      {
        auto newProp = new PropertyNoPerspective;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Flat:
      {
        auto newProp = new PropertyFlat;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Patch:
      {
        auto newProp = new PropertyPatch;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Centroid:
      {
        auto newProp = new PropertyCentroid;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Sample:
      {
        auto newProp = new PropertySample;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Invariant:
      {
        auto newProp = new PropertyInvariant;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Restrict:
      {
        auto newProp = new PropertyRestrict;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Aliased:
      {
        auto newProp = new PropertyAliased;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Volatile:
      {
        auto newProp = new PropertyVolatile;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Constant:
      {
        auto newProp = new PropertyConstant;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Coherent:
      {
        auto newProp = new PropertyCoherent;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::NonWritable:
      {
        auto newProp = new PropertyNonWritable;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::NonReadable:
      {
        auto newProp = new PropertyNonReadable;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Uniform:
      {
        auto newProp = new PropertyUniform;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::UniformId:
      {
        auto newProp = new PropertyUniformId;
        propPtr.reset(newProp);
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->execution), IdRef{decoration.data.UniformId.execution.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::SaturatedConversion:
      {
        auto newProp = new PropertySaturatedConversion;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::Stream:
      {
        auto newProp = new PropertyStream;
        propPtr.reset(newProp);
        newProp->streamNumber = decoration.data.Stream.streamNumber;
        break;
      }
      case Decoration::Location:
      {
        auto newProp = new PropertyLocation;
        propPtr.reset(newProp);
        newProp->location = decoration.data.Location.location;
        break;
      }
      case Decoration::Component:
      {
        auto newProp = new PropertyComponent;
        propPtr.reset(newProp);
        newProp->component = decoration.data.Component.component;
        break;
      }
      case Decoration::Index:
      {
        auto newProp = new PropertyIndex;
        propPtr.reset(newProp);
        newProp->index = decoration.data.Index.index;
        break;
      }
      case Decoration::Binding:
      {
        auto newProp = new PropertyBinding;
        propPtr.reset(newProp);
        newProp->bindingPoint = decoration.data.Binding.bindingPoint;
        break;
      }
      case Decoration::DescriptorSet:
      {
        auto newProp = new PropertyDescriptorSet;
        propPtr.reset(newProp);
        newProp->descriptorSet = decoration.data.DescriptorSet.descriptorSet;
        break;
      }
      case Decoration::Offset:
      {
        auto newProp = new PropertyOffset;
        propPtr.reset(newProp);
        newProp->byteOffset = decoration.data.Offset.byteOffset;
        break;
      }
      case Decoration::XfbBuffer:
      {
        auto newProp = new PropertyXfbBuffer;
        propPtr.reset(newProp);
        newProp->xfbBufferNumber = decoration.data.XfbBuffer.xfbBufferNumber;
        break;
      }
      case Decoration::XfbStride:
      {
        auto newProp = new PropertyXfbStride;
        propPtr.reset(newProp);
        newProp->xfbStride = decoration.data.XfbStride.xfbStride;
        break;
      }
      case Decoration::FuncParamAttr:
      {
        auto newProp = new PropertyFuncParamAttr;
        propPtr.reset(newProp);
        newProp->functionParameterAttribute = decoration.data.FuncParamAttr.functionParameterAttribute;
        break;
      }
      case Decoration::FPRoundingMode:
      {
        auto newProp = new PropertyFPRoundingMode;
        propPtr.reset(newProp);
        newProp->floatingPointRoundingMode = decoration.data.FPRoundingMode.floatingPointRoundingMode;
        break;
      }
      case Decoration::FPFastMathMode:
      {
        auto newProp = new PropertyFPFastMathMode;
        propPtr.reset(newProp);
        newProp->fastMathMode = decoration.data.FPFastMathMode.fastMathMode;
        break;
      }
      case Decoration::LinkageAttributes:
      {
        auto newProp = new PropertyLinkageAttributes;
        propPtr.reset(newProp);
        newProp->name = decoration.data.LinkageAttributes.name.asStringObj();
        newProp->linkageType = decoration.data.LinkageAttributes.linkageType;
        break;
      }
      case Decoration::NoContraction:
      {
        auto newProp = new PropertyNoContraction;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::InputAttachmentIndex:
      {
        auto newProp = new PropertyInputAttachmentIndex;
        propPtr.reset(newProp);
        newProp->attachmentIndex = decoration.data.InputAttachmentIndex.attachmentIndex;
        break;
      }
      case Decoration::Alignment:
      {
        auto newProp = new PropertyAlignment;
        propPtr.reset(newProp);
        newProp->alignment = decoration.data.Alignment.alignment;
        break;
      }
      case Decoration::MaxByteOffset:
      {
        auto newProp = new PropertyMaxByteOffset;
        propPtr.reset(newProp);
        newProp->maxByteOffset = decoration.data.MaxByteOffset.maxByteOffset;
        break;
      }
      case Decoration::AlignmentId:
      {
        auto newProp = new PropertyAlignmentId;
        propPtr.reset(newProp);
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->alignment), IdRef{decoration.data.AlignmentId.alignment.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::MaxByteOffsetId:
      {
        auto newProp = new PropertyMaxByteOffsetId;
        propPtr.reset(newProp);
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->maxByteOffset),
            IdRef{decoration.data.MaxByteOffsetId.maxByteOffset.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::NoSignedWrap:
      {
        auto newProp = new PropertyNoSignedWrap;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::NoUnsignedWrap:
      {
        auto newProp = new PropertyNoUnsignedWrap;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ExplicitInterpAMD:
      {
        auto newProp = new PropertyExplicitInterpAMD;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::OverrideCoverageNV:
      {
        auto newProp = new PropertyOverrideCoverageNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PassthroughNV:
      {
        auto newProp = new PropertyPassthroughNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ViewportRelativeNV:
      {
        auto newProp = new PropertyViewportRelativeNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::SecondaryViewportRelativeNV:
      {
        auto newProp = new PropertySecondaryViewportRelativeNV;
        propPtr.reset(newProp);
        newProp->offset = decoration.data.SecondaryViewportRelativeNV.offset;
        break;
      }
      case Decoration::PerPrimitiveNV:
      {
        auto newProp = new PropertyPerPrimitiveNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PerViewNV:
      {
        auto newProp = new PropertyPerViewNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PerTaskNV:
      {
        auto newProp = new PropertyPerTaskNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PerVertexKHR:
      {
        auto newProp = new PropertyPerVertexKHR;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::NonUniform:
      {
        auto newProp = new PropertyNonUniform;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::RestrictPointer:
      {
        auto newProp = new PropertyRestrictPointer;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::AliasedPointer:
      {
        auto newProp = new PropertyAliasedPointer;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BindlessSamplerNV:
      {
        auto newProp = new PropertyBindlessSamplerNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BindlessImageNV:
      {
        auto newProp = new PropertyBindlessImageNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BoundSamplerNV:
      {
        auto newProp = new PropertyBoundSamplerNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BoundImageNV:
      {
        auto newProp = new PropertyBoundImageNV;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::SIMTCallINTEL:
      {
        auto newProp = new PropertySIMTCallINTEL;
        propPtr.reset(newProp);
        newProp->n = decoration.data.SIMTCallINTEL.n;
        break;
      }
      case Decoration::ReferencedIndirectlyINTEL:
      {
        auto newProp = new PropertyReferencedIndirectlyINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::ClobberINTEL:
      {
        auto newProp = new PropertyClobberINTEL;
        propPtr.reset(newProp);
        newProp->_register = decoration.data.ClobberINTEL._register.asStringObj();
        break;
      }
      case Decoration::SideEffectsINTEL:
      {
        auto newProp = new PropertySideEffectsINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::VectorComputeVariableINTEL:
      {
        auto newProp = new PropertyVectorComputeVariableINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::FuncParamIOKindINTEL:
      {
        auto newProp = new PropertyFuncParamIOKindINTEL;
        propPtr.reset(newProp);
        newProp->kind = decoration.data.FuncParamIOKindINTEL.kind;
        break;
      }
      case Decoration::VectorComputeFunctionINTEL:
      {
        auto newProp = new PropertyVectorComputeFunctionINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::StackCallINTEL:
      {
        auto newProp = new PropertyStackCallINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::GlobalVariableOffsetINTEL:
      {
        auto newProp = new PropertyGlobalVariableOffsetINTEL;
        propPtr.reset(newProp);
        newProp->offset = decoration.data.GlobalVariableOffsetINTEL.offset;
        break;
      }
      case Decoration::CounterBuffer:
      {
        auto newProp = new PropertyCounterBuffer;
        propPtr.reset(newProp);
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->counterBuffer), IdRef{decoration.data.CounterBuffer.counterBuffer.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::UserSemantic:
      {
        auto newProp = new PropertyUserSemantic;
        propPtr.reset(newProp);
        newProp->semantic = decoration.data.UserSemantic.semantic.asStringObj();
        break;
      }
      case Decoration::UserTypeGOOGLE:
      {
        auto newProp = new PropertyUserTypeGOOGLE;
        propPtr.reset(newProp);
        newProp->userType = decoration.data.UserTypeGOOGLE.userType.asStringObj();
        break;
      }
      case Decoration::FunctionRoundingModeINTEL:
      {
        auto newProp = new PropertyFunctionRoundingModeINTEL;
        propPtr.reset(newProp);
        newProp->targetWidth = decoration.data.FunctionRoundingModeINTEL.targetWidth;
        newProp->fpRoundingMode = decoration.data.FunctionRoundingModeINTEL.fpRoundingMode;
        break;
      }
      case Decoration::FunctionDenormModeINTEL:
      {
        auto newProp = new PropertyFunctionDenormModeINTEL;
        propPtr.reset(newProp);
        newProp->targetWidth = decoration.data.FunctionDenormModeINTEL.targetWidth;
        newProp->fpDenormMode = decoration.data.FunctionDenormModeINTEL.fpDenormMode;
        break;
      }
      case Decoration::RegisterINTEL:
      {
        auto newProp = new PropertyRegisterINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::MemoryINTEL:
      {
        auto newProp = new PropertyMemoryINTEL;
        propPtr.reset(newProp);
        newProp->memoryType = decoration.data.MemoryINTEL.memoryType.asStringObj();
        break;
      }
      case Decoration::NumbanksINTEL:
      {
        auto newProp = new PropertyNumbanksINTEL;
        propPtr.reset(newProp);
        newProp->banks = decoration.data.NumbanksINTEL.banks;
        break;
      }
      case Decoration::BankwidthINTEL:
      {
        auto newProp = new PropertyBankwidthINTEL;
        propPtr.reset(newProp);
        newProp->bankWidth = decoration.data.BankwidthINTEL.bankWidth;
        break;
      }
      case Decoration::MaxPrivateCopiesINTEL:
      {
        auto newProp = new PropertyMaxPrivateCopiesINTEL;
        propPtr.reset(newProp);
        newProp->maximumCopies = decoration.data.MaxPrivateCopiesINTEL.maximumCopies;
        break;
      }
      case Decoration::SinglepumpINTEL:
      {
        auto newProp = new PropertySinglepumpINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::DoublepumpINTEL:
      {
        auto newProp = new PropertyDoublepumpINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::MaxReplicatesINTEL:
      {
        auto newProp = new PropertyMaxReplicatesINTEL;
        propPtr.reset(newProp);
        newProp->maximumReplicates = decoration.data.MaxReplicatesINTEL.maximumReplicates;
        break;
      }
      case Decoration::SimpleDualPortINTEL:
      {
        auto newProp = new PropertySimpleDualPortINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::MergeINTEL:
      {
        auto newProp = new PropertyMergeINTEL;
        propPtr.reset(newProp);
        newProp->mergeKey = decoration.data.MergeINTEL.mergeKey.asStringObj();
        newProp->mergeType = decoration.data.MergeINTEL.mergeType.asStringObj();
        break;
      }
      case Decoration::BankBitsINTEL:
      {
        auto newProp = new PropertyBankBitsINTEL;
        propPtr.reset(newProp);
        newProp->bankBits = decoration.data.BankBitsINTEL.bankBits;
        break;
      }
      case Decoration::ForcePow2DepthINTEL:
      {
        auto newProp = new PropertyForcePow2DepthINTEL;
        propPtr.reset(newProp);
        newProp->forceKey = decoration.data.ForcePow2DepthINTEL.forceKey;
        break;
      }
      case Decoration::BurstCoalesceINTEL:
      {
        auto newProp = new PropertyBurstCoalesceINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::CacheSizeINTEL:
      {
        auto newProp = new PropertyCacheSizeINTEL;
        propPtr.reset(newProp);
        newProp->cacheSizeInBytes = decoration.data.CacheSizeINTEL.cacheSizeInBytes;
        break;
      }
      case Decoration::DontStaticallyCoalesceINTEL:
      {
        auto newProp = new PropertyDontStaticallyCoalesceINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::PrefetchINTEL:
      {
        auto newProp = new PropertyPrefetchINTEL;
        propPtr.reset(newProp);
        newProp->prefetcherSizeInBytes = decoration.data.PrefetchINTEL.prefetcherSizeInBytes;
        break;
      }
      case Decoration::StallEnableINTEL:
      {
        auto newProp = new PropertyStallEnableINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::FuseLoopsInFunctionINTEL:
      {
        auto newProp = new PropertyFuseLoopsInFunctionINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::BufferLocationINTEL:
      {
        auto newProp = new PropertyBufferLocationINTEL;
        propPtr.reset(newProp);
        newProp->bufferLocationId = decoration.data.BufferLocationINTEL.bufferLocationId;
        break;
      }
      case Decoration::IOPipeStorageINTEL:
      {
        auto newProp = new PropertyIOPipeStorageINTEL;
        propPtr.reset(newProp);
        newProp->ioPipeId = decoration.data.IOPipeStorageINTEL.ioPipeId;
        break;
      }
      case Decoration::FunctionFloatingPointModeINTEL:
      {
        auto newProp = new PropertyFunctionFloatingPointModeINTEL;
        propPtr.reset(newProp);
        newProp->targetWidth = decoration.data.FunctionFloatingPointModeINTEL.targetWidth;
        newProp->fpOperationMode = decoration.data.FunctionFloatingPointModeINTEL.fpOperationMode;
        break;
      }
      case Decoration::SingleElementVectorINTEL:
      {
        auto newProp = new PropertySingleElementVectorINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::VectorComputeCallableFunctionINTEL:
      {
        auto newProp = new PropertyVectorComputeCallableFunctionINTEL;
        propPtr.reset(newProp);
        break;
      }
      case Decoration::MediaBlockIOINTEL:
      {
        auto newProp = new PropertyMediaBlockIOINTEL;
        propPtr.reset(newProp);
        break;
      }
    }
    PropertyTargetResolveInfo targetResolve //
      {target, eastl::move(propPtr)};
    propertyTargetResolves.push_back(eastl::move(targetResolve));
  }
  void onMemberDecorateString(Op, IdRef struct_type, LiteralInteger member, TypeTraits<Decoration>::ReadType decoration)
  {
    CastableUniquePointer<Property> propPtr;
    switch (decoration.value)
    {
      case Decoration::RelaxedPrecision:
      {
        auto newProp = new PropertyRelaxedPrecision;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::SpecId:
      {
        auto newProp = new PropertySpecId;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->specializationConstantId = decoration.data.SpecId.specializationConstantId;
        break;
      }
      case Decoration::Block:
      {
        auto newProp = new PropertyBlock;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BufferBlock:
      {
        auto newProp = new PropertyBufferBlock;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::RowMajor:
      {
        auto newProp = new PropertyRowMajor;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::ColMajor:
      {
        auto newProp = new PropertyColMajor;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::ArrayStride:
      {
        auto newProp = new PropertyArrayStride;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->arrayStride = decoration.data.ArrayStride.arrayStride;
        break;
      }
      case Decoration::MatrixStride:
      {
        auto newProp = new PropertyMatrixStride;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->matrixStride = decoration.data.MatrixStride.matrixStride;
        break;
      }
      case Decoration::GLSLShared:
      {
        auto newProp = new PropertyGLSLShared;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::GLSLPacked:
      {
        auto newProp = new PropertyGLSLPacked;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::CPacked:
      {
        auto newProp = new PropertyCPacked;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BuiltIn:
      {
        auto newProp = new PropertyBuiltIn;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->builtIn = decoration.data.BuiltInData.first;
        break;
      }
      case Decoration::NoPerspective:
      {
        auto newProp = new PropertyNoPerspective;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Flat:
      {
        auto newProp = new PropertyFlat;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Patch:
      {
        auto newProp = new PropertyPatch;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Centroid:
      {
        auto newProp = new PropertyCentroid;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Sample:
      {
        auto newProp = new PropertySample;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Invariant:
      {
        auto newProp = new PropertyInvariant;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Restrict:
      {
        auto newProp = new PropertyRestrict;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Aliased:
      {
        auto newProp = new PropertyAliased;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Volatile:
      {
        auto newProp = new PropertyVolatile;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Constant:
      {
        auto newProp = new PropertyConstant;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Coherent:
      {
        auto newProp = new PropertyCoherent;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::NonWritable:
      {
        auto newProp = new PropertyNonWritable;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::NonReadable:
      {
        auto newProp = new PropertyNonReadable;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Uniform:
      {
        auto newProp = new PropertyUniform;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::UniformId:
      {
        auto newProp = new PropertyUniformId;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->execution), IdRef{decoration.data.UniformId.execution.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::SaturatedConversion:
      {
        auto newProp = new PropertySaturatedConversion;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Stream:
      {
        auto newProp = new PropertyStream;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->streamNumber = decoration.data.Stream.streamNumber;
        break;
      }
      case Decoration::Location:
      {
        auto newProp = new PropertyLocation;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->location = decoration.data.Location.location;
        break;
      }
      case Decoration::Component:
      {
        auto newProp = new PropertyComponent;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->component = decoration.data.Component.component;
        break;
      }
      case Decoration::Index:
      {
        auto newProp = new PropertyIndex;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->index = decoration.data.Index.index;
        break;
      }
      case Decoration::Binding:
      {
        auto newProp = new PropertyBinding;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->bindingPoint = decoration.data.Binding.bindingPoint;
        break;
      }
      case Decoration::DescriptorSet:
      {
        auto newProp = new PropertyDescriptorSet;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->descriptorSet = decoration.data.DescriptorSet.descriptorSet;
        break;
      }
      case Decoration::Offset:
      {
        auto newProp = new PropertyOffset;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->byteOffset = decoration.data.Offset.byteOffset;
        break;
      }
      case Decoration::XfbBuffer:
      {
        auto newProp = new PropertyXfbBuffer;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->xfbBufferNumber = decoration.data.XfbBuffer.xfbBufferNumber;
        break;
      }
      case Decoration::XfbStride:
      {
        auto newProp = new PropertyXfbStride;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->xfbStride = decoration.data.XfbStride.xfbStride;
        break;
      }
      case Decoration::FuncParamAttr:
      {
        auto newProp = new PropertyFuncParamAttr;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->functionParameterAttribute = decoration.data.FuncParamAttr.functionParameterAttribute;
        break;
      }
      case Decoration::FPRoundingMode:
      {
        auto newProp = new PropertyFPRoundingMode;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->floatingPointRoundingMode = decoration.data.FPRoundingMode.floatingPointRoundingMode;
        break;
      }
      case Decoration::FPFastMathMode:
      {
        auto newProp = new PropertyFPFastMathMode;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->fastMathMode = decoration.data.FPFastMathMode.fastMathMode;
        break;
      }
      case Decoration::LinkageAttributes:
      {
        auto newProp = new PropertyLinkageAttributes;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->name = decoration.data.LinkageAttributes.name.asStringObj();
        newProp->linkageType = decoration.data.LinkageAttributes.linkageType;
        break;
      }
      case Decoration::NoContraction:
      {
        auto newProp = new PropertyNoContraction;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::InputAttachmentIndex:
      {
        auto newProp = new PropertyInputAttachmentIndex;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->attachmentIndex = decoration.data.InputAttachmentIndex.attachmentIndex;
        break;
      }
      case Decoration::Alignment:
      {
        auto newProp = new PropertyAlignment;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->alignment = decoration.data.Alignment.alignment;
        break;
      }
      case Decoration::MaxByteOffset:
      {
        auto newProp = new PropertyMaxByteOffset;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->maxByteOffset = decoration.data.MaxByteOffset.maxByteOffset;
        break;
      }
      case Decoration::AlignmentId:
      {
        auto newProp = new PropertyAlignmentId;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->alignment), IdRef{decoration.data.AlignmentId.alignment.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::MaxByteOffsetId:
      {
        auto newProp = new PropertyMaxByteOffsetId;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->maxByteOffset),
            IdRef{decoration.data.MaxByteOffsetId.maxByteOffset.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::NoSignedWrap:
      {
        auto newProp = new PropertyNoSignedWrap;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::NoUnsignedWrap:
      {
        auto newProp = new PropertyNoUnsignedWrap;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::ExplicitInterpAMD:
      {
        auto newProp = new PropertyExplicitInterpAMD;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::OverrideCoverageNV:
      {
        auto newProp = new PropertyOverrideCoverageNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::PassthroughNV:
      {
        auto newProp = new PropertyPassthroughNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::ViewportRelativeNV:
      {
        auto newProp = new PropertyViewportRelativeNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::SecondaryViewportRelativeNV:
      {
        auto newProp = new PropertySecondaryViewportRelativeNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->offset = decoration.data.SecondaryViewportRelativeNV.offset;
        break;
      }
      case Decoration::PerPrimitiveNV:
      {
        auto newProp = new PropertyPerPrimitiveNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::PerViewNV:
      {
        auto newProp = new PropertyPerViewNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::PerTaskNV:
      {
        auto newProp = new PropertyPerTaskNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::PerVertexKHR:
      {
        auto newProp = new PropertyPerVertexKHR;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::NonUniform:
      {
        auto newProp = new PropertyNonUniform;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::RestrictPointer:
      {
        auto newProp = new PropertyRestrictPointer;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::AliasedPointer:
      {
        auto newProp = new PropertyAliasedPointer;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BindlessSamplerNV:
      {
        auto newProp = new PropertyBindlessSamplerNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BindlessImageNV:
      {
        auto newProp = new PropertyBindlessImageNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BoundSamplerNV:
      {
        auto newProp = new PropertyBoundSamplerNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BoundImageNV:
      {
        auto newProp = new PropertyBoundImageNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::SIMTCallINTEL:
      {
        auto newProp = new PropertySIMTCallINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->n = decoration.data.SIMTCallINTEL.n;
        break;
      }
      case Decoration::ReferencedIndirectlyINTEL:
      {
        auto newProp = new PropertyReferencedIndirectlyINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::ClobberINTEL:
      {
        auto newProp = new PropertyClobberINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->_register = decoration.data.ClobberINTEL._register.asStringObj();
        break;
      }
      case Decoration::SideEffectsINTEL:
      {
        auto newProp = new PropertySideEffectsINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::VectorComputeVariableINTEL:
      {
        auto newProp = new PropertyVectorComputeVariableINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::FuncParamIOKindINTEL:
      {
        auto newProp = new PropertyFuncParamIOKindINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->kind = decoration.data.FuncParamIOKindINTEL.kind;
        break;
      }
      case Decoration::VectorComputeFunctionINTEL:
      {
        auto newProp = new PropertyVectorComputeFunctionINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::StackCallINTEL:
      {
        auto newProp = new PropertyStackCallINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::GlobalVariableOffsetINTEL:
      {
        auto newProp = new PropertyGlobalVariableOffsetINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->offset = decoration.data.GlobalVariableOffsetINTEL.offset;
        break;
      }
      case Decoration::CounterBuffer:
      {
        auto newProp = new PropertyCounterBuffer;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->counterBuffer), IdRef{decoration.data.CounterBuffer.counterBuffer.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::UserSemantic:
      {
        auto newProp = new PropertyUserSemantic;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->semantic = decoration.data.UserSemantic.semantic.asStringObj();
        break;
      }
      case Decoration::UserTypeGOOGLE:
      {
        auto newProp = new PropertyUserTypeGOOGLE;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->userType = decoration.data.UserTypeGOOGLE.userType.asStringObj();
        break;
      }
      case Decoration::FunctionRoundingModeINTEL:
      {
        auto newProp = new PropertyFunctionRoundingModeINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->targetWidth = decoration.data.FunctionRoundingModeINTEL.targetWidth;
        newProp->fpRoundingMode = decoration.data.FunctionRoundingModeINTEL.fpRoundingMode;
        break;
      }
      case Decoration::FunctionDenormModeINTEL:
      {
        auto newProp = new PropertyFunctionDenormModeINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->targetWidth = decoration.data.FunctionDenormModeINTEL.targetWidth;
        newProp->fpDenormMode = decoration.data.FunctionDenormModeINTEL.fpDenormMode;
        break;
      }
      case Decoration::RegisterINTEL:
      {
        auto newProp = new PropertyRegisterINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::MemoryINTEL:
      {
        auto newProp = new PropertyMemoryINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->memoryType = decoration.data.MemoryINTEL.memoryType.asStringObj();
        break;
      }
      case Decoration::NumbanksINTEL:
      {
        auto newProp = new PropertyNumbanksINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->banks = decoration.data.NumbanksINTEL.banks;
        break;
      }
      case Decoration::BankwidthINTEL:
      {
        auto newProp = new PropertyBankwidthINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->bankWidth = decoration.data.BankwidthINTEL.bankWidth;
        break;
      }
      case Decoration::MaxPrivateCopiesINTEL:
      {
        auto newProp = new PropertyMaxPrivateCopiesINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->maximumCopies = decoration.data.MaxPrivateCopiesINTEL.maximumCopies;
        break;
      }
      case Decoration::SinglepumpINTEL:
      {
        auto newProp = new PropertySinglepumpINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::DoublepumpINTEL:
      {
        auto newProp = new PropertyDoublepumpINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::MaxReplicatesINTEL:
      {
        auto newProp = new PropertyMaxReplicatesINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->maximumReplicates = decoration.data.MaxReplicatesINTEL.maximumReplicates;
        break;
      }
      case Decoration::SimpleDualPortINTEL:
      {
        auto newProp = new PropertySimpleDualPortINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::MergeINTEL:
      {
        auto newProp = new PropertyMergeINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->mergeKey = decoration.data.MergeINTEL.mergeKey.asStringObj();
        newProp->mergeType = decoration.data.MergeINTEL.mergeType.asStringObj();
        break;
      }
      case Decoration::BankBitsINTEL:
      {
        auto newProp = new PropertyBankBitsINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->bankBits = decoration.data.BankBitsINTEL.bankBits;
        break;
      }
      case Decoration::ForcePow2DepthINTEL:
      {
        auto newProp = new PropertyForcePow2DepthINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->forceKey = decoration.data.ForcePow2DepthINTEL.forceKey;
        break;
      }
      case Decoration::BurstCoalesceINTEL:
      {
        auto newProp = new PropertyBurstCoalesceINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::CacheSizeINTEL:
      {
        auto newProp = new PropertyCacheSizeINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->cacheSizeInBytes = decoration.data.CacheSizeINTEL.cacheSizeInBytes;
        break;
      }
      case Decoration::DontStaticallyCoalesceINTEL:
      {
        auto newProp = new PropertyDontStaticallyCoalesceINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::PrefetchINTEL:
      {
        auto newProp = new PropertyPrefetchINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->prefetcherSizeInBytes = decoration.data.PrefetchINTEL.prefetcherSizeInBytes;
        break;
      }
      case Decoration::StallEnableINTEL:
      {
        auto newProp = new PropertyStallEnableINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::FuseLoopsInFunctionINTEL:
      {
        auto newProp = new PropertyFuseLoopsInFunctionINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BufferLocationINTEL:
      {
        auto newProp = new PropertyBufferLocationINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->bufferLocationId = decoration.data.BufferLocationINTEL.bufferLocationId;
        break;
      }
      case Decoration::IOPipeStorageINTEL:
      {
        auto newProp = new PropertyIOPipeStorageINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->ioPipeId = decoration.data.IOPipeStorageINTEL.ioPipeId;
        break;
      }
      case Decoration::FunctionFloatingPointModeINTEL:
      {
        auto newProp = new PropertyFunctionFloatingPointModeINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->targetWidth = decoration.data.FunctionFloatingPointModeINTEL.targetWidth;
        newProp->fpOperationMode = decoration.data.FunctionFloatingPointModeINTEL.fpOperationMode;
        break;
      }
      case Decoration::SingleElementVectorINTEL:
      {
        auto newProp = new PropertySingleElementVectorINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::VectorComputeCallableFunctionINTEL:
      {
        auto newProp = new PropertyVectorComputeCallableFunctionINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::MediaBlockIOINTEL:
      {
        auto newProp = new PropertyMediaBlockIOINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
    }
    PropertyTargetResolveInfo targetResolve //
      {struct_type, eastl::move(propPtr)};
    propertyTargetResolves.push_back(eastl::move(targetResolve));
  }
  void onMemberDecorateStringGOOGLE(Op, IdRef struct_type, LiteralInteger member, TypeTraits<Decoration>::ReadType decoration)
  {
    CastableUniquePointer<Property> propPtr;
    switch (decoration.value)
    {
      case Decoration::RelaxedPrecision:
      {
        auto newProp = new PropertyRelaxedPrecision;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::SpecId:
      {
        auto newProp = new PropertySpecId;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->specializationConstantId = decoration.data.SpecId.specializationConstantId;
        break;
      }
      case Decoration::Block:
      {
        auto newProp = new PropertyBlock;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BufferBlock:
      {
        auto newProp = new PropertyBufferBlock;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::RowMajor:
      {
        auto newProp = new PropertyRowMajor;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::ColMajor:
      {
        auto newProp = new PropertyColMajor;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::ArrayStride:
      {
        auto newProp = new PropertyArrayStride;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->arrayStride = decoration.data.ArrayStride.arrayStride;
        break;
      }
      case Decoration::MatrixStride:
      {
        auto newProp = new PropertyMatrixStride;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->matrixStride = decoration.data.MatrixStride.matrixStride;
        break;
      }
      case Decoration::GLSLShared:
      {
        auto newProp = new PropertyGLSLShared;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::GLSLPacked:
      {
        auto newProp = new PropertyGLSLPacked;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::CPacked:
      {
        auto newProp = new PropertyCPacked;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BuiltIn:
      {
        auto newProp = new PropertyBuiltIn;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->builtIn = decoration.data.BuiltInData.first;
        break;
      }
      case Decoration::NoPerspective:
      {
        auto newProp = new PropertyNoPerspective;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Flat:
      {
        auto newProp = new PropertyFlat;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Patch:
      {
        auto newProp = new PropertyPatch;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Centroid:
      {
        auto newProp = new PropertyCentroid;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Sample:
      {
        auto newProp = new PropertySample;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Invariant:
      {
        auto newProp = new PropertyInvariant;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Restrict:
      {
        auto newProp = new PropertyRestrict;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Aliased:
      {
        auto newProp = new PropertyAliased;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Volatile:
      {
        auto newProp = new PropertyVolatile;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Constant:
      {
        auto newProp = new PropertyConstant;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Coherent:
      {
        auto newProp = new PropertyCoherent;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::NonWritable:
      {
        auto newProp = new PropertyNonWritable;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::NonReadable:
      {
        auto newProp = new PropertyNonReadable;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Uniform:
      {
        auto newProp = new PropertyUniform;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::UniformId:
      {
        auto newProp = new PropertyUniformId;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->execution), IdRef{decoration.data.UniformId.execution.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::SaturatedConversion:
      {
        auto newProp = new PropertySaturatedConversion;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::Stream:
      {
        auto newProp = new PropertyStream;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->streamNumber = decoration.data.Stream.streamNumber;
        break;
      }
      case Decoration::Location:
      {
        auto newProp = new PropertyLocation;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->location = decoration.data.Location.location;
        break;
      }
      case Decoration::Component:
      {
        auto newProp = new PropertyComponent;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->component = decoration.data.Component.component;
        break;
      }
      case Decoration::Index:
      {
        auto newProp = new PropertyIndex;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->index = decoration.data.Index.index;
        break;
      }
      case Decoration::Binding:
      {
        auto newProp = new PropertyBinding;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->bindingPoint = decoration.data.Binding.bindingPoint;
        break;
      }
      case Decoration::DescriptorSet:
      {
        auto newProp = new PropertyDescriptorSet;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->descriptorSet = decoration.data.DescriptorSet.descriptorSet;
        break;
      }
      case Decoration::Offset:
      {
        auto newProp = new PropertyOffset;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->byteOffset = decoration.data.Offset.byteOffset;
        break;
      }
      case Decoration::XfbBuffer:
      {
        auto newProp = new PropertyXfbBuffer;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->xfbBufferNumber = decoration.data.XfbBuffer.xfbBufferNumber;
        break;
      }
      case Decoration::XfbStride:
      {
        auto newProp = new PropertyXfbStride;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->xfbStride = decoration.data.XfbStride.xfbStride;
        break;
      }
      case Decoration::FuncParamAttr:
      {
        auto newProp = new PropertyFuncParamAttr;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->functionParameterAttribute = decoration.data.FuncParamAttr.functionParameterAttribute;
        break;
      }
      case Decoration::FPRoundingMode:
      {
        auto newProp = new PropertyFPRoundingMode;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->floatingPointRoundingMode = decoration.data.FPRoundingMode.floatingPointRoundingMode;
        break;
      }
      case Decoration::FPFastMathMode:
      {
        auto newProp = new PropertyFPFastMathMode;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->fastMathMode = decoration.data.FPFastMathMode.fastMathMode;
        break;
      }
      case Decoration::LinkageAttributes:
      {
        auto newProp = new PropertyLinkageAttributes;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->name = decoration.data.LinkageAttributes.name.asStringObj();
        newProp->linkageType = decoration.data.LinkageAttributes.linkageType;
        break;
      }
      case Decoration::NoContraction:
      {
        auto newProp = new PropertyNoContraction;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::InputAttachmentIndex:
      {
        auto newProp = new PropertyInputAttachmentIndex;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->attachmentIndex = decoration.data.InputAttachmentIndex.attachmentIndex;
        break;
      }
      case Decoration::Alignment:
      {
        auto newProp = new PropertyAlignment;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->alignment = decoration.data.Alignment.alignment;
        break;
      }
      case Decoration::MaxByteOffset:
      {
        auto newProp = new PropertyMaxByteOffset;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->maxByteOffset = decoration.data.MaxByteOffset.maxByteOffset;
        break;
      }
      case Decoration::AlignmentId:
      {
        auto newProp = new PropertyAlignmentId;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->alignment), IdRef{decoration.data.AlignmentId.alignment.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::MaxByteOffsetId:
      {
        auto newProp = new PropertyMaxByteOffsetId;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->maxByteOffset),
            IdRef{decoration.data.MaxByteOffsetId.maxByteOffset.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::NoSignedWrap:
      {
        auto newProp = new PropertyNoSignedWrap;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::NoUnsignedWrap:
      {
        auto newProp = new PropertyNoUnsignedWrap;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::ExplicitInterpAMD:
      {
        auto newProp = new PropertyExplicitInterpAMD;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::OverrideCoverageNV:
      {
        auto newProp = new PropertyOverrideCoverageNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::PassthroughNV:
      {
        auto newProp = new PropertyPassthroughNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::ViewportRelativeNV:
      {
        auto newProp = new PropertyViewportRelativeNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::SecondaryViewportRelativeNV:
      {
        auto newProp = new PropertySecondaryViewportRelativeNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->offset = decoration.data.SecondaryViewportRelativeNV.offset;
        break;
      }
      case Decoration::PerPrimitiveNV:
      {
        auto newProp = new PropertyPerPrimitiveNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::PerViewNV:
      {
        auto newProp = new PropertyPerViewNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::PerTaskNV:
      {
        auto newProp = new PropertyPerTaskNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::PerVertexKHR:
      {
        auto newProp = new PropertyPerVertexKHR;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::NonUniform:
      {
        auto newProp = new PropertyNonUniform;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::RestrictPointer:
      {
        auto newProp = new PropertyRestrictPointer;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::AliasedPointer:
      {
        auto newProp = new PropertyAliasedPointer;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BindlessSamplerNV:
      {
        auto newProp = new PropertyBindlessSamplerNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BindlessImageNV:
      {
        auto newProp = new PropertyBindlessImageNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BoundSamplerNV:
      {
        auto newProp = new PropertyBoundSamplerNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BoundImageNV:
      {
        auto newProp = new PropertyBoundImageNV;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::SIMTCallINTEL:
      {
        auto newProp = new PropertySIMTCallINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->n = decoration.data.SIMTCallINTEL.n;
        break;
      }
      case Decoration::ReferencedIndirectlyINTEL:
      {
        auto newProp = new PropertyReferencedIndirectlyINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::ClobberINTEL:
      {
        auto newProp = new PropertyClobberINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->_register = decoration.data.ClobberINTEL._register.asStringObj();
        break;
      }
      case Decoration::SideEffectsINTEL:
      {
        auto newProp = new PropertySideEffectsINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::VectorComputeVariableINTEL:
      {
        auto newProp = new PropertyVectorComputeVariableINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::FuncParamIOKindINTEL:
      {
        auto newProp = new PropertyFuncParamIOKindINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->kind = decoration.data.FuncParamIOKindINTEL.kind;
        break;
      }
      case Decoration::VectorComputeFunctionINTEL:
      {
        auto newProp = new PropertyVectorComputeFunctionINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::StackCallINTEL:
      {
        auto newProp = new PropertyStackCallINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::GlobalVariableOffsetINTEL:
      {
        auto newProp = new PropertyGlobalVariableOffsetINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->offset = decoration.data.GlobalVariableOffsetINTEL.offset;
        break;
      }
      case Decoration::CounterBuffer:
      {
        auto newProp = new PropertyCounterBuffer;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        PropertyReferenceResolveInfo refResolve //
          {reinterpret_cast<NodePointer<NodeId> *>(&newProp->counterBuffer), IdRef{decoration.data.CounterBuffer.counterBuffer.value}};
        propertyReferenceResolves.push_back(refResolve);
        break;
      }
      case Decoration::UserSemantic:
      {
        auto newProp = new PropertyUserSemantic;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->semantic = decoration.data.UserSemantic.semantic.asStringObj();
        break;
      }
      case Decoration::UserTypeGOOGLE:
      {
        auto newProp = new PropertyUserTypeGOOGLE;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->userType = decoration.data.UserTypeGOOGLE.userType.asStringObj();
        break;
      }
      case Decoration::FunctionRoundingModeINTEL:
      {
        auto newProp = new PropertyFunctionRoundingModeINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->targetWidth = decoration.data.FunctionRoundingModeINTEL.targetWidth;
        newProp->fpRoundingMode = decoration.data.FunctionRoundingModeINTEL.fpRoundingMode;
        break;
      }
      case Decoration::FunctionDenormModeINTEL:
      {
        auto newProp = new PropertyFunctionDenormModeINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->targetWidth = decoration.data.FunctionDenormModeINTEL.targetWidth;
        newProp->fpDenormMode = decoration.data.FunctionDenormModeINTEL.fpDenormMode;
        break;
      }
      case Decoration::RegisterINTEL:
      {
        auto newProp = new PropertyRegisterINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::MemoryINTEL:
      {
        auto newProp = new PropertyMemoryINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->memoryType = decoration.data.MemoryINTEL.memoryType.asStringObj();
        break;
      }
      case Decoration::NumbanksINTEL:
      {
        auto newProp = new PropertyNumbanksINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->banks = decoration.data.NumbanksINTEL.banks;
        break;
      }
      case Decoration::BankwidthINTEL:
      {
        auto newProp = new PropertyBankwidthINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->bankWidth = decoration.data.BankwidthINTEL.bankWidth;
        break;
      }
      case Decoration::MaxPrivateCopiesINTEL:
      {
        auto newProp = new PropertyMaxPrivateCopiesINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->maximumCopies = decoration.data.MaxPrivateCopiesINTEL.maximumCopies;
        break;
      }
      case Decoration::SinglepumpINTEL:
      {
        auto newProp = new PropertySinglepumpINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::DoublepumpINTEL:
      {
        auto newProp = new PropertyDoublepumpINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::MaxReplicatesINTEL:
      {
        auto newProp = new PropertyMaxReplicatesINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->maximumReplicates = decoration.data.MaxReplicatesINTEL.maximumReplicates;
        break;
      }
      case Decoration::SimpleDualPortINTEL:
      {
        auto newProp = new PropertySimpleDualPortINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::MergeINTEL:
      {
        auto newProp = new PropertyMergeINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->mergeKey = decoration.data.MergeINTEL.mergeKey.asStringObj();
        newProp->mergeType = decoration.data.MergeINTEL.mergeType.asStringObj();
        break;
      }
      case Decoration::BankBitsINTEL:
      {
        auto newProp = new PropertyBankBitsINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->bankBits = decoration.data.BankBitsINTEL.bankBits;
        break;
      }
      case Decoration::ForcePow2DepthINTEL:
      {
        auto newProp = new PropertyForcePow2DepthINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->forceKey = decoration.data.ForcePow2DepthINTEL.forceKey;
        break;
      }
      case Decoration::BurstCoalesceINTEL:
      {
        auto newProp = new PropertyBurstCoalesceINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::CacheSizeINTEL:
      {
        auto newProp = new PropertyCacheSizeINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->cacheSizeInBytes = decoration.data.CacheSizeINTEL.cacheSizeInBytes;
        break;
      }
      case Decoration::DontStaticallyCoalesceINTEL:
      {
        auto newProp = new PropertyDontStaticallyCoalesceINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::PrefetchINTEL:
      {
        auto newProp = new PropertyPrefetchINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->prefetcherSizeInBytes = decoration.data.PrefetchINTEL.prefetcherSizeInBytes;
        break;
      }
      case Decoration::StallEnableINTEL:
      {
        auto newProp = new PropertyStallEnableINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::FuseLoopsInFunctionINTEL:
      {
        auto newProp = new PropertyFuseLoopsInFunctionINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::BufferLocationINTEL:
      {
        auto newProp = new PropertyBufferLocationINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->bufferLocationId = decoration.data.BufferLocationINTEL.bufferLocationId;
        break;
      }
      case Decoration::IOPipeStorageINTEL:
      {
        auto newProp = new PropertyIOPipeStorageINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->ioPipeId = decoration.data.IOPipeStorageINTEL.ioPipeId;
        break;
      }
      case Decoration::FunctionFloatingPointModeINTEL:
      {
        auto newProp = new PropertyFunctionFloatingPointModeINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        newProp->targetWidth = decoration.data.FunctionFloatingPointModeINTEL.targetWidth;
        newProp->fpOperationMode = decoration.data.FunctionFloatingPointModeINTEL.fpOperationMode;
        break;
      }
      case Decoration::SingleElementVectorINTEL:
      {
        auto newProp = new PropertySingleElementVectorINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::VectorComputeCallableFunctionINTEL:
      {
        auto newProp = new PropertyVectorComputeCallableFunctionINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
      case Decoration::MediaBlockIOINTEL:
      {
        auto newProp = new PropertyMediaBlockIOINTEL;
        propPtr.reset(newProp);
        newProp->memberIndex = member.value;
        break;
      }
    }
    PropertyTargetResolveInfo targetResolve //
      {struct_type, eastl::move(propPtr)};
    propertyTargetResolves.push_back(eastl::move(targetResolve));
  }
  void onVmeImageINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef image_type, IdRef sampler)
  {
    auto node = moduleBuilder.newNode<NodeOpVmeImageINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(image_type), moduleBuilder.getNode(sampler));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onTypeVmeImageINTEL(Op, IdResult id_result, IdRef image_type)
  {
    moduleBuilder.newNode<NodeOpTypeVmeImageINTEL>(id_result.value, moduleBuilder.getNode(image_type));
  }
  void onTypeAvcImePayloadINTEL(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeAvcImePayloadINTEL>(id_result.value); }
  void onTypeAvcRefPayloadINTEL(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeAvcRefPayloadINTEL>(id_result.value); }
  void onTypeAvcSicPayloadINTEL(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeAvcSicPayloadINTEL>(id_result.value); }
  void onTypeAvcMcePayloadINTEL(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeAvcMcePayloadINTEL>(id_result.value); }
  void onTypeAvcMceResultINTEL(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeAvcMceResultINTEL>(id_result.value); }
  void onTypeAvcImeResultINTEL(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeAvcImeResultINTEL>(id_result.value); }
  void onTypeAvcImeResultSingleReferenceStreamoutINTEL(Op, IdResult id_result)
  {
    moduleBuilder.newNode<NodeOpTypeAvcImeResultSingleReferenceStreamoutINTEL>(id_result.value);
  }
  void onTypeAvcImeResultDualReferenceStreamoutINTEL(Op, IdResult id_result)
  {
    moduleBuilder.newNode<NodeOpTypeAvcImeResultDualReferenceStreamoutINTEL>(id_result.value);
  }
  void onTypeAvcImeSingleReferenceStreaminINTEL(Op, IdResult id_result)
  {
    moduleBuilder.newNode<NodeOpTypeAvcImeSingleReferenceStreaminINTEL>(id_result.value);
  }
  void onTypeAvcImeDualReferenceStreaminINTEL(Op, IdResult id_result)
  {
    moduleBuilder.newNode<NodeOpTypeAvcImeDualReferenceStreaminINTEL>(id_result.value);
  }
  void onTypeAvcRefResultINTEL(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeAvcRefResultINTEL>(id_result.value); }
  void onTypeAvcSicResultINTEL(Op, IdResult id_result) { moduleBuilder.newNode<NodeOpTypeAvcSicResultINTEL>(id_result.value); }
  void onSubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef slice_type, IdRef qp)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(slice_type), moduleBuilder.getNode(qp));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef reference_base_penalty, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(reference_base_penalty), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetDefaultInterShapePenaltyINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef slice_type,
    IdRef qp)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetDefaultInterShapePenaltyINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(slice_type), moduleBuilder.getNode(qp));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceSetInterShapePenaltyINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef packed_shape_penalty,
    IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceSetInterShapePenaltyINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(packed_shape_penalty), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef slice_type,
    IdRef qp)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(slice_type), moduleBuilder.getNode(qp));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceSetInterDirectionPenaltyINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef direction_cost,
    IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceSetInterDirectionPenaltyINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(direction_cost), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef slice_type,
    IdRef qp)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(slice_type), moduleBuilder.getNode(qp));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef slice_type,
    IdRef qp)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(slice_type), moduleBuilder.getNode(qp));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL(Op, IdResult id_result, IdResultType id_result_type)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL(Op, IdResult id_result, IdResultType id_result_type)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL(Op, IdResult id_result, IdResultType id_result_type)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceSetMotionVectorCostFunctionINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef packed_cost_center_delta, IdRef packed_cost_table, IdRef cost_precision, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceSetMotionVectorCostFunctionINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(packed_cost_center_delta), moduleBuilder.getNode(packed_cost_table),
      moduleBuilder.getNode(cost_precision), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef slice_type,
    IdRef qp)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(slice_type), moduleBuilder.getNode(qp));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL(Op, IdResult id_result, IdResultType id_result_type)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL(Op, IdResult id_result, IdResultType id_result_type)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceSetAcOnlyHaarINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceSetAcOnlyHaarINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef source_field_polarity, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(source_field_polarity), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef reference_field_polarity, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(reference_field_polarity), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef forward_reference_field_polarity, IdRef backward_reference_field_polarity, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(forward_reference_field_polarity),
      moduleBuilder.getNode(backward_reference_field_polarity), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceConvertToImePayloadINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceConvertToImePayloadINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceConvertToImeResultINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceConvertToImeResultINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceConvertToRefPayloadINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceConvertToRefPayloadINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceConvertToRefResultINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceConvertToRefResultINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceConvertToSicPayloadINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceConvertToSicPayloadINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceConvertToSicResultINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceConvertToSicResultINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetMotionVectorsINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetMotionVectorsINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetInterDistortionsINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetInterDistortionsINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetBestInterDistortionsINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetBestInterDistortionsINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetInterMajorShapeINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetInterMajorShapeINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetInterMinorShapeINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetInterMinorShapeINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetInterDirectionsINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetInterDirectionsINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetInterMotionVectorCountINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetInterMotionVectorCountINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetInterReferenceIdsINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetInterReferenceIdsINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef packed_reference_ids, IdRef packed_reference_parameter_field_polarities, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(packed_reference_ids),
      moduleBuilder.getNode(packed_reference_parameter_field_polarities), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeInitializeINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_coord, IdRef partition_mask,
    IdRef s_a_d_adjustment)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeInitializeINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(src_coord), moduleBuilder.getNode(partition_mask), moduleBuilder.getNode(s_a_d_adjustment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeSetSingleReferenceINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef ref_offset,
    IdRef search_window_config, IdRef payload)
  {
    auto node =
      moduleBuilder.newNode<NodeOpSubgroupAvcImeSetSingleReferenceINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
        moduleBuilder.getNode(ref_offset), moduleBuilder.getNode(search_window_config), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeSetDualReferenceINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef fwd_ref_offset,
    IdRef bwd_ref_offset, IdRef id_search_window_config, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeSetDualReferenceINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(fwd_ref_offset), moduleBuilder.getNode(bwd_ref_offset),
      moduleBuilder.getNode(id_search_window_config), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeRefWindowSizeINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef search_window_config,
    IdRef dual_ref)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeRefWindowSizeINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(search_window_config), moduleBuilder.getNode(dual_ref));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeAdjustRefOffsetINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef ref_offset, IdRef src_coord,
    IdRef ref_window_size, IdRef image_size)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeAdjustRefOffsetINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(ref_offset), moduleBuilder.getNode(src_coord), moduleBuilder.getNode(ref_window_size),
      moduleBuilder.getNode(image_size));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeConvertToMcePayloadINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeConvertToMcePayloadINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeSetMaxMotionVectorCountINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef max_motion_vector_count,
    IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeSetMaxMotionVectorCountINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(max_motion_vector_count), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeSetUnidirectionalMixDisableINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeSetUnidirectionalMixDisableINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeSetEarlySearchTerminationThresholdINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef threshold,
    IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeSetEarlySearchTerminationThresholdINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(threshold), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeSetWeightedSadINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef packed_sad_weights,
    IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeSetWeightedSadINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(packed_sad_weights), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeEvaluateWithSingleReferenceINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_image,
    IdRef ref_image, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeEvaluateWithSingleReferenceINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(src_image), moduleBuilder.getNode(ref_image),
      moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeEvaluateWithDualReferenceINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_image,
    IdRef fwd_ref_image, IdRef bwd_ref_image, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeEvaluateWithDualReferenceINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(src_image), moduleBuilder.getNode(fwd_ref_image),
      moduleBuilder.getNode(bwd_ref_image), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_image,
    IdRef ref_image, IdRef payload, IdRef streamin_components)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(src_image), moduleBuilder.getNode(ref_image),
      moduleBuilder.getNode(payload), moduleBuilder.getNode(streamin_components));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_image,
    IdRef fwd_ref_image, IdRef bwd_ref_image, IdRef payload, IdRef streamin_components)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(src_image), moduleBuilder.getNode(fwd_ref_image),
      moduleBuilder.getNode(bwd_ref_image), moduleBuilder.getNode(payload), moduleBuilder.getNode(streamin_components));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_image,
    IdRef ref_image, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(src_image), moduleBuilder.getNode(ref_image),
      moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_image,
    IdRef fwd_ref_image, IdRef bwd_ref_image, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(src_image), moduleBuilder.getNode(fwd_ref_image),
      moduleBuilder.getNode(bwd_ref_image), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef src_image, IdRef ref_image, IdRef payload, IdRef streamin_components)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(src_image), moduleBuilder.getNode(ref_image),
      moduleBuilder.getNode(payload), moduleBuilder.getNode(streamin_components));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_image,
    IdRef fwd_ref_image, IdRef bwd_ref_image, IdRef payload, IdRef streamin_components)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(src_image), moduleBuilder.getNode(fwd_ref_image),
      moduleBuilder.getNode(bwd_ref_image), moduleBuilder.getNode(payload), moduleBuilder.getNode(streamin_components));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeConvertToMceResultINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeConvertToMceResultINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeGetSingleReferenceStreaminINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeGetSingleReferenceStreaminINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeGetDualReferenceStreaminINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeGetDualReferenceStreaminINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeStripSingleReferenceStreamoutINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeStripSingleReferenceStreamoutINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeStripDualReferenceStreamoutINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeStripDualReferenceStreamoutINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef payload, IdRef major_shape)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload), moduleBuilder.getNode(major_shape));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef payload, IdRef major_shape)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload), moduleBuilder.getNode(major_shape));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef payload, IdRef major_shape)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload), moduleBuilder.getNode(major_shape));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef payload, IdRef major_shape, IdRef direction)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload), moduleBuilder.getNode(major_shape),
      moduleBuilder.getNode(direction));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef payload, IdRef major_shape, IdRef direction)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload), moduleBuilder.getNode(major_shape),
      moduleBuilder.getNode(direction));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef payload, IdRef major_shape, IdRef direction)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload), moduleBuilder.getNode(major_shape),
      moduleBuilder.getNode(direction));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeGetBorderReachedINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef image_select, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeGetBorderReachedINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(image_select), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeGetTruncatedSearchIndicationINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeGetTruncatedSearchIndicationINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcFmeInitializeINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_coord, IdRef motion_vectors,
    IdRef major_shapes, IdRef minor_shapes, IdRef direction, IdRef pixel_resolution, IdRef sad_adjustment)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcFmeInitializeINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(src_coord), moduleBuilder.getNode(motion_vectors), moduleBuilder.getNode(major_shapes),
      moduleBuilder.getNode(minor_shapes), moduleBuilder.getNode(direction), moduleBuilder.getNode(pixel_resolution),
      moduleBuilder.getNode(sad_adjustment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcBmeInitializeINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_coord, IdRef motion_vectors,
    IdRef major_shapes, IdRef minor_shapes, IdRef direction, IdRef pixel_resolution, IdRef bidirectional_weight, IdRef sad_adjustment)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcBmeInitializeINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(src_coord), moduleBuilder.getNode(motion_vectors), moduleBuilder.getNode(major_shapes),
      moduleBuilder.getNode(minor_shapes), moduleBuilder.getNode(direction), moduleBuilder.getNode(pixel_resolution),
      moduleBuilder.getNode(bidirectional_weight), moduleBuilder.getNode(sad_adjustment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcRefConvertToMcePayloadINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcRefConvertToMcePayloadINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcRefSetBidirectionalMixDisableINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcRefSetBidirectionalMixDisableINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcRefSetBilinearFilterEnableINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcRefSetBilinearFilterEnableINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcRefEvaluateWithSingleReferenceINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_image,
    IdRef ref_image, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcRefEvaluateWithSingleReferenceINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(src_image), moduleBuilder.getNode(ref_image),
      moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcRefEvaluateWithDualReferenceINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_image,
    IdRef fwd_ref_image, IdRef bwd_ref_image, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcRefEvaluateWithDualReferenceINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(src_image), moduleBuilder.getNode(fwd_ref_image),
      moduleBuilder.getNode(bwd_ref_image), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcRefEvaluateWithMultiReferenceINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_image,
    IdRef packed_reference_ids, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcRefEvaluateWithMultiReferenceINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(src_image), moduleBuilder.getNode(packed_reference_ids),
      moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_image,
    IdRef packed_reference_ids, IdRef packed_reference_field_polarities, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(src_image), moduleBuilder.getNode(packed_reference_ids),
      moduleBuilder.getNode(packed_reference_field_polarities), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcRefConvertToMceResultINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcRefConvertToMceResultINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicInitializeINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_coord)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicInitializeINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(src_coord));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicConfigureSkcINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef skip_block_partition_type,
    IdRef skip_motion_vector_mask, IdRef motion_vectors, IdRef bidirectional_weight, IdRef sad_adjustment, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicConfigureSkcINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(skip_block_partition_type), moduleBuilder.getNode(skip_motion_vector_mask),
      moduleBuilder.getNode(motion_vectors), moduleBuilder.getNode(bidirectional_weight), moduleBuilder.getNode(sad_adjustment),
      moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicConfigureIpeLumaINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef luma_intra_partition_mask,
    IdRef intra_neighbour_availabilty, IdRef left_edge_luma_pixels, IdRef upper_left_corner_luma_pixel, IdRef upper_edge_luma_pixels,
    IdRef upper_right_edge_luma_pixels, IdRef sad_adjustment, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicConfigureIpeLumaINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(luma_intra_partition_mask),
      moduleBuilder.getNode(intra_neighbour_availabilty), moduleBuilder.getNode(left_edge_luma_pixels),
      moduleBuilder.getNode(upper_left_corner_luma_pixel), moduleBuilder.getNode(upper_edge_luma_pixels),
      moduleBuilder.getNode(upper_right_edge_luma_pixels), moduleBuilder.getNode(sad_adjustment), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicConfigureIpeLumaChromaINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef luma_intra_partition_mask, IdRef intra_neighbour_availabilty, IdRef left_edge_luma_pixels,
    IdRef upper_left_corner_luma_pixel, IdRef upper_edge_luma_pixels, IdRef upper_right_edge_luma_pixels,
    IdRef left_edge_chroma_pixels, IdRef upper_left_corner_chroma_pixel, IdRef upper_edge_chroma_pixels, IdRef sad_adjustment,
    IdRef payload)
  {
    auto node =
      moduleBuilder.newNode<NodeOpSubgroupAvcSicConfigureIpeLumaChromaINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
        moduleBuilder.getNode(luma_intra_partition_mask), moduleBuilder.getNode(intra_neighbour_availabilty),
        moduleBuilder.getNode(left_edge_luma_pixels), moduleBuilder.getNode(upper_left_corner_luma_pixel),
        moduleBuilder.getNode(upper_edge_luma_pixels), moduleBuilder.getNode(upper_right_edge_luma_pixels),
        moduleBuilder.getNode(left_edge_chroma_pixels), moduleBuilder.getNode(upper_left_corner_chroma_pixel),
        moduleBuilder.getNode(upper_edge_chroma_pixels), moduleBuilder.getNode(sad_adjustment), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicGetMotionVectorMaskINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef skip_block_partition_type,
    IdRef direction)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicGetMotionVectorMaskINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(skip_block_partition_type), moduleBuilder.getNode(direction));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicConvertToMcePayloadINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicConvertToMcePayloadINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicSetIntraLumaShapePenaltyINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef packed_shape_penalty,
    IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicSetIntraLumaShapePenaltyINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(packed_shape_penalty), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicSetIntraLumaModeCostFunctionINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef luma_mode_penalty,
    IdRef luma_packed_neighbor_modes, IdRef luma_packed_non_dc_penalty, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicSetIntraLumaModeCostFunctionINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(luma_mode_penalty),
      moduleBuilder.getNode(luma_packed_neighbor_modes), moduleBuilder.getNode(luma_packed_non_dc_penalty),
      moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicSetIntraChromaModeCostFunctionINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef chroma_mode_base_penalty, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicSetIntraChromaModeCostFunctionINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(chroma_mode_base_penalty), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicSetBilinearFilterEnableINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicSetBilinearFilterEnableINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicSetSkcForwardTransformEnableINTEL(Op, IdResult id_result, IdResultType id_result_type,
    IdRef packed_sad_coefficients, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicSetSkcForwardTransformEnableINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(packed_sad_coefficients), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicSetBlockBasedRawSkipSadINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef block_based_skip_type,
    IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicSetBlockBasedRawSkipSadINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(block_based_skip_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicEvaluateIpeINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_image, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicEvaluateIpeINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(src_image), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicEvaluateWithSingleReferenceINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_image,
    IdRef ref_image, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicEvaluateWithSingleReferenceINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(src_image), moduleBuilder.getNode(ref_image),
      moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicEvaluateWithDualReferenceINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_image,
    IdRef fwd_ref_image, IdRef bwd_ref_image, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicEvaluateWithDualReferenceINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(src_image), moduleBuilder.getNode(fwd_ref_image),
      moduleBuilder.getNode(bwd_ref_image), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicEvaluateWithMultiReferenceINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_image,
    IdRef packed_reference_ids, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicEvaluateWithMultiReferenceINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(src_image), moduleBuilder.getNode(packed_reference_ids),
      moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef src_image,
    IdRef packed_reference_ids, IdRef packed_reference_field_polarities, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(src_image), moduleBuilder.getNode(packed_reference_ids),
      moduleBuilder.getNode(packed_reference_field_polarities), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicConvertToMceResultINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicConvertToMceResultINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicGetIpeLumaShapeINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicGetIpeLumaShapeINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicGetBestIpeLumaDistortionINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicGetBestIpeLumaDistortionINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicGetBestIpeChromaDistortionINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicGetBestIpeChromaDistortionINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicGetPackedIpeLumaModesINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicGetPackedIpeLumaModesINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicGetIpeChromaModeINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicGetIpeChromaModeINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSubgroupAvcSicGetInterRawSadsINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef payload)
  {
    auto node = moduleBuilder.newNode<NodeOpSubgroupAvcSicGetInterRawSadsINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(payload));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onVariableLengthArrayINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef length)
  {
    auto node = moduleBuilder.newNode<NodeOpVariableLengthArrayINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(length));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSaveMemoryINTEL(Op, IdResult id_result, IdResultType id_result_type)
  {
    auto node = moduleBuilder.newNode<NodeOpSaveMemoryINTEL>(id_result.value, moduleBuilder.getType(id_result_type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRestoreMemoryINTEL(Op, IdRef ptr)
  {
    auto node = moduleBuilder.newNode<NodeOpRestoreMemoryINTEL>(moduleBuilder.getNode(ptr));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatSinCosPiINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1,
    LiteralInteger mout, LiteralInteger from_sign, LiteralInteger enable_subnormals, LiteralInteger rounding_mode,
    LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatSinCosPiINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, from_sign, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatCastINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatCastINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatCastFromIntINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger mout,
    LiteralInteger from_sign, LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatCastFromIntINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), mout, from_sign, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatCastToIntINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatCastToIntINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatAddINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, IdRef b,
    LiteralInteger m2, LiteralInteger mout, LiteralInteger enable_subnormals, LiteralInteger rounding_mode,
    LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatAddINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, moduleBuilder.getNode(b), m2, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatSubINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, IdRef b,
    LiteralInteger m2, LiteralInteger mout, LiteralInteger enable_subnormals, LiteralInteger rounding_mode,
    LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatSubINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, moduleBuilder.getNode(b), m2, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatMulINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, IdRef b,
    LiteralInteger m2, LiteralInteger mout, LiteralInteger enable_subnormals, LiteralInteger rounding_mode,
    LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatMulINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, moduleBuilder.getNode(b), m2, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatDivINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, IdRef b,
    LiteralInteger m2, LiteralInteger mout, LiteralInteger enable_subnormals, LiteralInteger rounding_mode,
    LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatDivINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, moduleBuilder.getNode(b), m2, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatGTINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, IdRef b,
    LiteralInteger m2)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatGTINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, moduleBuilder.getNode(b), m2);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatGEINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, IdRef b,
    LiteralInteger m2)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatGEINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, moduleBuilder.getNode(b), m2);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatLTINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, IdRef b,
    LiteralInteger m2)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatLTINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, moduleBuilder.getNode(b), m2);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatLEINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, IdRef b,
    LiteralInteger m2)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatLEINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, moduleBuilder.getNode(b), m2);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatEQINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, IdRef b,
    LiteralInteger m2)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatEQINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, moduleBuilder.getNode(b), m2);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatRecipINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatRecipINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatRSqrtINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatRSqrtINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatCbrtINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatCbrtINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatHypotINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, IdRef b,
    LiteralInteger m2, LiteralInteger mout, LiteralInteger enable_subnormals, LiteralInteger rounding_mode,
    LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatHypotINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, moduleBuilder.getNode(b), m2, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatSqrtINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatSqrtINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatLogINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatLogINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatLog2INTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatLog2INTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatLog10INTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatLog10INTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatLog1pINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatLog1pINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatExpINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatExpINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatExp2INTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatExp2INTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatExp10INTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatExp10INTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatExpm1INTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatExpm1INTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatSinINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatSinINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatCosINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatCosINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatSinCosINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1,
    LiteralInteger mout, LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatSinCosINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatSinPiINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatSinPiINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatCosPiINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatCosPiINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatASinINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatASinINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatASinPiINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1,
    LiteralInteger mout, LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatASinPiINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatACosINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatACosINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatACosPiINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1,
    LiteralInteger mout, LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatACosPiINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatATanINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, LiteralInteger mout,
    LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatATanINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatATanPiINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1,
    LiteralInteger mout, LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatATanPiINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatATan2INTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, IdRef b,
    LiteralInteger m2, LiteralInteger mout, LiteralInteger enable_subnormals, LiteralInteger rounding_mode,
    LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatATan2INTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, moduleBuilder.getNode(b), m2, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatPowINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, IdRef b,
    LiteralInteger m2, LiteralInteger mout, LiteralInteger enable_subnormals, LiteralInteger rounding_mode,
    LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatPowINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, moduleBuilder.getNode(b), m2, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatPowRINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, IdRef b,
    LiteralInteger m2, LiteralInteger mout, LiteralInteger enable_subnormals, LiteralInteger rounding_mode,
    LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatPowRINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, moduleBuilder.getNode(b), m2, mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onArbitraryFloatPowNINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef a, LiteralInteger m1, IdRef b,
    LiteralInteger mout, LiteralInteger enable_subnormals, LiteralInteger rounding_mode, LiteralInteger rounding_accuracy)
  {
    auto node = moduleBuilder.newNode<NodeOpArbitraryFloatPowNINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), m1, moduleBuilder.getNode(b), mout, enable_subnormals, rounding_mode, rounding_accuracy);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onLoopControlINTEL(Op, Multiple<LiteralInteger> loop_control_parameters) {}
  void onFixedSqrtINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef input_type, IdRef input, LiteralInteger s,
    LiteralInteger i, LiteralInteger r_i, LiteralInteger q, LiteralInteger o)
  {
    auto node = moduleBuilder.newNode<NodeOpFixedSqrtINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(input_type), moduleBuilder.getNode(input), s, i, r_i, q, o);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFixedRecipINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef input_type, IdRef input, LiteralInteger s,
    LiteralInteger i, LiteralInteger r_i, LiteralInteger q, LiteralInteger o)
  {
    auto node = moduleBuilder.newNode<NodeOpFixedRecipINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(input_type), moduleBuilder.getNode(input), s, i, r_i, q, o);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFixedRsqrtINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef input_type, IdRef input, LiteralInteger s,
    LiteralInteger i, LiteralInteger r_i, LiteralInteger q, LiteralInteger o)
  {
    auto node = moduleBuilder.newNode<NodeOpFixedRsqrtINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(input_type), moduleBuilder.getNode(input), s, i, r_i, q, o);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFixedSinINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef input_type, IdRef input, LiteralInteger s,
    LiteralInteger i, LiteralInteger r_i, LiteralInteger q, LiteralInteger o)
  {
    auto node = moduleBuilder.newNode<NodeOpFixedSinINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(input_type), moduleBuilder.getNode(input), s, i, r_i, q, o);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFixedCosINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef input_type, IdRef input, LiteralInteger s,
    LiteralInteger i, LiteralInteger r_i, LiteralInteger q, LiteralInteger o)
  {
    auto node = moduleBuilder.newNode<NodeOpFixedCosINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(input_type), moduleBuilder.getNode(input), s, i, r_i, q, o);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFixedSinCosINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef input_type, IdRef input, LiteralInteger s,
    LiteralInteger i, LiteralInteger r_i, LiteralInteger q, LiteralInteger o)
  {
    auto node = moduleBuilder.newNode<NodeOpFixedSinCosINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(input_type), moduleBuilder.getNode(input), s, i, r_i, q, o);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFixedSinPiINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef input_type, IdRef input, LiteralInteger s,
    LiteralInteger i, LiteralInteger r_i, LiteralInteger q, LiteralInteger o)
  {
    auto node = moduleBuilder.newNode<NodeOpFixedSinPiINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(input_type), moduleBuilder.getNode(input), s, i, r_i, q, o);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFixedCosPiINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef input_type, IdRef input, LiteralInteger s,
    LiteralInteger i, LiteralInteger r_i, LiteralInteger q, LiteralInteger o)
  {
    auto node = moduleBuilder.newNode<NodeOpFixedCosPiINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(input_type), moduleBuilder.getNode(input), s, i, r_i, q, o);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFixedSinCosPiINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef input_type, IdRef input, LiteralInteger s,
    LiteralInteger i, LiteralInteger r_i, LiteralInteger q, LiteralInteger o)
  {
    auto node = moduleBuilder.newNode<NodeOpFixedSinCosPiINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(input_type), moduleBuilder.getNode(input), s, i, r_i, q, o);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFixedLogINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef input_type, IdRef input, LiteralInteger s,
    LiteralInteger i, LiteralInteger r_i, LiteralInteger q, LiteralInteger o)
  {
    auto node = moduleBuilder.newNode<NodeOpFixedLogINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(input_type), moduleBuilder.getNode(input), s, i, r_i, q, o);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFixedExpINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef input_type, IdRef input, LiteralInteger s,
    LiteralInteger i, LiteralInteger r_i, LiteralInteger q, LiteralInteger o)
  {
    auto node = moduleBuilder.newNode<NodeOpFixedExpINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(input_type), moduleBuilder.getNode(input), s, i, r_i, q, o);
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onPtrCastToCrossWorkgroupINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer)
  {
    auto node = moduleBuilder.newNode<NodeOpPtrCastToCrossWorkgroupINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onCrossWorkgroupCastToPtrINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer)
  {
    auto node = moduleBuilder.newNode<NodeOpCrossWorkgroupCastToPtrINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onReadPipeBlockingINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef packet_size, IdRef packet_alignment)
  {
    auto node = moduleBuilder.newNode<NodeOpReadPipeBlockingINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(packet_size), moduleBuilder.getNode(packet_alignment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onWritePipeBlockingINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef packet_size, IdRef packet_alignment)
  {
    auto node = moduleBuilder.newNode<NodeOpWritePipeBlockingINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(packet_size), moduleBuilder.getNode(packet_alignment));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onFPGARegINTEL(Op, IdResult id_result, IdResultType id_result_type, IdRef result, IdRef input)
  {
    auto node = moduleBuilder.newNode<NodeOpFPGARegINTEL>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(result), moduleBuilder.getNode(input));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetRayTMinKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetRayTMinKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(ray_query));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetRayFlagsKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetRayFlagsKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(ray_query));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetIntersectionTKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query, IdRef intersection)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetIntersectionTKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(ray_query), moduleBuilder.getNode(intersection));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetIntersectionInstanceCustomIndexKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query,
    IdRef intersection)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetIntersectionInstanceCustomIndexKHR>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(ray_query), moduleBuilder.getNode(intersection));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetIntersectionInstanceIdKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query, IdRef intersection)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetIntersectionInstanceIdKHR>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(ray_query), moduleBuilder.getNode(intersection));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetKHR(Op, IdResult id_result, IdResultType id_result_type,
    IdRef ray_query, IdRef intersection)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetKHR>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(ray_query), moduleBuilder.getNode(intersection));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetIntersectionGeometryIndexKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query,
    IdRef intersection)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetIntersectionGeometryIndexKHR>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(ray_query), moduleBuilder.getNode(intersection));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetIntersectionPrimitiveIndexKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query,
    IdRef intersection)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetIntersectionPrimitiveIndexKHR>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(ray_query), moduleBuilder.getNode(intersection));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetIntersectionBarycentricsKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query,
    IdRef intersection)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetIntersectionBarycentricsKHR>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(ray_query), moduleBuilder.getNode(intersection));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetIntersectionFrontFaceKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query, IdRef intersection)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetIntersectionFrontFaceKHR>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(ray_query), moduleBuilder.getNode(intersection));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetIntersectionCandidateAABBOpaqueKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetIntersectionCandidateAABBOpaqueKHR>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(ray_query));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetIntersectionObjectRayDirectionKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query,
    IdRef intersection)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetIntersectionObjectRayDirectionKHR>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(ray_query), moduleBuilder.getNode(intersection));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetIntersectionObjectRayOriginKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query,
    IdRef intersection)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetIntersectionObjectRayOriginKHR>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(ray_query), moduleBuilder.getNode(intersection));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetWorldRayDirectionKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetWorldRayDirectionKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(ray_query));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetWorldRayOriginKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetWorldRayOriginKHR>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(ray_query));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetIntersectionObjectToWorldKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query,
    IdRef intersection)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetIntersectionObjectToWorldKHR>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(ray_query), moduleBuilder.getNode(intersection));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onRayQueryGetIntersectionWorldToObjectKHR(Op, IdResult id_result, IdResultType id_result_type, IdRef ray_query,
    IdRef intersection)
  {
    auto node = moduleBuilder.newNode<NodeOpRayQueryGetIntersectionWorldToObjectKHR>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(ray_query), moduleBuilder.getNode(intersection));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAtomicFAddEXT(Op, IdResult id_result, IdResultType id_result_type, IdRef pointer, IdScope memory, IdMemorySemantics semantics,
    IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpAtomicFAddEXT>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(pointer), moduleBuilder.getNode(memory), moduleBuilder.getNode(semantics), moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onTypeBufferSurfaceINTEL(Op, IdResult id_result, AccessQualifier access_qualifier)
  {
    moduleBuilder.newNode<NodeOpTypeBufferSurfaceINTEL>(id_result.value, access_qualifier);
  }
  void onTypeStructContinuedINTEL(Op, Multiple<IdRef> param_0)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<NodePointer<NodeId>> param_0Var;
    while (!param_0.empty())
    {
      param_0Var.push_back(moduleBuilder.getNode(param_0.consume()));
    }
    moduleBuilder.newNode<NodeOpTypeStructContinuedINTEL>(param_0Var.data(), param_0Var.size());
  }
  void onConstantCompositeContinuedINTEL(Op, Multiple<IdRef> constituents)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<NodePointer<NodeId>> constituentsVar;
    while (!constituents.empty())
    {
      constituentsVar.push_back(moduleBuilder.getNode(constituents.consume()));
    }
    auto node = moduleBuilder.newNode<NodeOpConstantCompositeContinuedINTEL>(constituentsVar.data(), constituentsVar.size());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onSpecConstantCompositeContinuedINTEL(Op, Multiple<IdRef> constituents)
  {
    // FIXME: use vector directly in constructor
    eastl::vector<NodePointer<NodeId>> constituentsVar;
    while (!constituents.empty())
    {
      constituentsVar.push_back(moduleBuilder.getNode(constituents.consume()));
    }
    auto node = moduleBuilder.newNode<NodeOpSpecConstantCompositeContinuedINTEL>(constituentsVar.data(), constituentsVar.size());
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Round(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Round>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450RoundEven(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450RoundEven>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Trunc(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Trunc>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450FAbs(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450FAbs>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450SAbs(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450SAbs>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450FSign(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450FSign>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450SSign(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450SSign>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Floor(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Floor>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Ceil(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Ceil>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Fract(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Fract>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Radians(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef degrees)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450Radians>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(degrees));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Degrees(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef radians)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450Degrees>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(radians));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Sin(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Sin>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Cos(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Cos>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Tan(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Tan>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Asin(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Asin>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Acos(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Acos>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Atan(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef y_over_x)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450Atan>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(y_over_x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Sinh(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Sinh>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Cosh(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Cosh>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Tanh(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Tanh>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Asinh(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Asinh>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Acosh(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Acosh>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Atanh(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Atanh>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Atan2(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef y, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450Atan2>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(y), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Pow(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450Pow>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Exp(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Exp>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Log(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Log>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Exp2(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Exp2>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Log2(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Log2>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Sqrt(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Sqrt>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450InverseSqrt(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450InverseSqrt>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Determinant(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450Determinant>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450MatrixInverse(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450MatrixInverse>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Modf(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef i)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450Modf>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(i));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450ModfStruct(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450ModfStruct>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450FMin(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450FMin>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450UMin(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450UMin>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450SMin(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450SMin>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450FMax(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450FMax>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450UMax(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450UMax>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450SMax(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450SMax>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450FClamp(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef min_val, IdRef max_val)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450FClamp>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(min_val), moduleBuilder.getNode(max_val));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450UClamp(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef min_val, IdRef max_val)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450UClamp>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(min_val), moduleBuilder.getNode(max_val));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450SClamp(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef min_val, IdRef max_val)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450SClamp>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(min_val), moduleBuilder.getNode(max_val));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450FMix(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y, IdRef a)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450FMix>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y), moduleBuilder.getNode(a));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450IMix(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y, IdRef a)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450IMix>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y), moduleBuilder.getNode(a));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Step(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef edge, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450Step>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(edge), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450SmoothStep(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef edge0, IdRef edge1, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450SmoothStep>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(edge0), moduleBuilder.getNode(edge1), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Fma(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef a, IdRef b, IdRef c)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450Fma>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(a), moduleBuilder.getNode(b), moduleBuilder.getNode(c));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Frexp(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef exp)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450Frexp>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(exp));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450FrexpStruct(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450FrexpStruct>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Ldexp(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef exp)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450Ldexp>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(exp));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450PackSnorm4x8(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef v)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450PackSnorm4x8>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(v));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450PackUnorm4x8(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef v)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450PackUnorm4x8>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(v));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450PackSnorm2x16(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef v)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450PackSnorm2x16>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(v));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450PackUnorm2x16(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef v)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450PackUnorm2x16>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(v));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450PackHalf2x16(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef v)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450PackHalf2x16>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(v));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450PackDouble2x32(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef v)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450PackDouble2x32>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(v));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450UnpackSnorm2x16(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef p)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450UnpackSnorm2x16>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(p));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450UnpackUnorm2x16(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef p)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450UnpackUnorm2x16>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(p));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450UnpackHalf2x16(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef v)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450UnpackHalf2x16>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(v));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450UnpackSnorm4x8(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef p)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450UnpackSnorm4x8>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(p));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450UnpackUnorm4x8(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef p)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450UnpackUnorm4x8>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(p));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450UnpackDouble2x32(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef v)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450UnpackDouble2x32>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(v));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Length(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node =
      moduleBuilder.newNode<NodeOpGLSLstd450Length>(id_result.value, moduleBuilder.getType(id_result_type), moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Distance(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef p0, IdRef p1)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450Distance>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(p0), moduleBuilder.getNode(p1));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Cross(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450Cross>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Normalize(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450Normalize>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450FaceForward(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef n, IdRef i, IdRef nref)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450FaceForward>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(n), moduleBuilder.getNode(i), moduleBuilder.getNode(nref));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Reflect(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef i, IdRef n)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450Reflect>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(i), moduleBuilder.getNode(n));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450Refract(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef i, IdRef n, IdRef eta)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450Refract>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(i), moduleBuilder.getNode(n), moduleBuilder.getNode(eta));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450FindILsb(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450FindILsb>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450FindSMsb(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450FindSMsb>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450FindUMsb(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef value)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450FindUMsb>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(value));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450InterpolateAtCentroid(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef interpolant)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450InterpolateAtCentroid>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(interpolant));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450InterpolateAtSample(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef interpolant, IdRef sample)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450InterpolateAtSample>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(interpolant), moduleBuilder.getNode(sample));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450InterpolateAtOffset(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef interpolant, IdRef offset)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450InterpolateAtOffset>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(interpolant), moduleBuilder.getNode(offset));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450NMin(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450NMin>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450NMax(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450NMax>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onGLSLstd450NClamp(GLSLstd450, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef min_val, IdRef max_val)
  {
    auto node = moduleBuilder.newNode<NodeOpGLSLstd450NClamp>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(min_val), moduleBuilder.getNode(max_val));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDGcnShaderCubeFaceIndex(AMDGcnShader, IdResult id_result, IdResultType id_result_type, IdRef p)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDGcnShaderCubeFaceIndex>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(p));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDGcnShaderCubeFaceCoord(AMDGcnShader, IdResult id_result, IdResultType id_result_type, IdRef p)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDGcnShaderCubeFaceCoord>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(p));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDGcnShaderTime(AMDGcnShader, IdResult id_result, IdResultType id_result_type)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDGcnShaderTime>(id_result.value, moduleBuilder.getType(id_result_type));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDShaderBallotSwizzleInvocations(AMDShaderBallot, IdResult id_result, IdResultType id_result_type, IdRef data, IdRef offset)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDShaderBallotSwizzleInvocations>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(data), moduleBuilder.getNode(offset));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDShaderBallotSwizzleInvocationsMasked(AMDShaderBallot, IdResult id_result, IdResultType id_result_type, IdRef data,
    IdRef mask)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDShaderBallotSwizzleInvocationsMasked>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(data), moduleBuilder.getNode(mask));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDShaderBallotWriteInvocation(AMDShaderBallot, IdResult id_result, IdResultType id_result_type, IdRef input_value,
    IdRef write_value, IdRef invocation_index)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDShaderBallotWriteInvocation>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(input_value), moduleBuilder.getNode(write_value), moduleBuilder.getNode(invocation_index));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDShaderBallotMbcnt(AMDShaderBallot, IdResult id_result, IdResultType id_result_type, IdRef mask)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDShaderBallotMbcnt>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(mask));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDShaderTrinaryMinmaxFMin3(AMDShaderTrinaryMinmax, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y,
    IdRef z)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDShaderTrinaryMinmaxFMin3>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y), moduleBuilder.getNode(z));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDShaderTrinaryMinmaxUMin3(AMDShaderTrinaryMinmax, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y,
    IdRef z)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDShaderTrinaryMinmaxUMin3>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y), moduleBuilder.getNode(z));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDShaderTrinaryMinmaxSMin3(AMDShaderTrinaryMinmax, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y,
    IdRef z)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDShaderTrinaryMinmaxSMin3>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y), moduleBuilder.getNode(z));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDShaderTrinaryMinmaxFMax3(AMDShaderTrinaryMinmax, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y,
    IdRef z)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDShaderTrinaryMinmaxFMax3>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y), moduleBuilder.getNode(z));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDShaderTrinaryMinmaxUMax3(AMDShaderTrinaryMinmax, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y,
    IdRef z)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDShaderTrinaryMinmaxUMax3>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y), moduleBuilder.getNode(z));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDShaderTrinaryMinmaxSMax3(AMDShaderTrinaryMinmax, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y,
    IdRef z)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDShaderTrinaryMinmaxSMax3>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y), moduleBuilder.getNode(z));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDShaderTrinaryMinmaxFMid3(AMDShaderTrinaryMinmax, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y,
    IdRef z)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDShaderTrinaryMinmaxFMid3>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y), moduleBuilder.getNode(z));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDShaderTrinaryMinmaxUMid3(AMDShaderTrinaryMinmax, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y,
    IdRef z)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDShaderTrinaryMinmaxUMid3>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y), moduleBuilder.getNode(z));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDShaderTrinaryMinmaxSMid3(AMDShaderTrinaryMinmax, IdResult id_result, IdResultType id_result_type, IdRef x, IdRef y,
    IdRef z)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDShaderTrinaryMinmaxSMid3>(id_result.value, moduleBuilder.getType(id_result_type),
      moduleBuilder.getNode(x), moduleBuilder.getNode(y), moduleBuilder.getNode(z));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  void onAMDShaderExplicitVertexParameterInterpolateAtVertex(AMDShaderExplicitVertexParameter, IdResult id_result,
    IdResultType id_result_type, IdRef interpolant, IdRef vertex_idx)
  {
    auto node = moduleBuilder.newNode<NodeOpAMDShaderExplicitVertexParameterInterpolateAtVertex>(id_result.value,
      moduleBuilder.getType(id_result_type), moduleBuilder.getNode(interpolant), moduleBuilder.getNode(vertex_idx));
    if (currentBlock)
      currentBlock->instructions.push_back(node);
  }
  // 752 instructions handled
};
struct ErrorRecorder
{
  ReaderResult &target;
  bool operator()(const char *msg)
  {
    target.errorLog.emplace_back(msg);
    return true;
  }
  bool operator()(const char *op_name, const char *msg)
  {
    target.errorLog.emplace_back(op_name);
    target.errorLog.back().append(" : ");
    target.errorLog.back().append(msg);
    return true;
  }
};
ReaderResult spirv::read(ModuleBuilder &builder, const unsigned *words, unsigned word_count)
{
  eastl::vector<EntryPointDef> entryPoints;
  NodePointer<NodeOpFunction> currentFunction;
  eastl::vector<IdRef> forwardProperties;
  eastl::vector<SelectionMergeDef> selectionMerges;
  eastl::vector<LoopMergeDef> loopMerges;
  NodePointer<NodeBlock> currentBlock;
  eastl::vector<PropertyTargetResolveInfo> propertyTargetResolves;
  eastl::vector<PropertyReferenceResolveInfo> propertyReferenceResolves;
  eastl::vector<ExectionModeDef> executionModes;
  NodeBlock specConstGrabBlock;
  unsigned sourcePosition = 0;
  ReaderResult resultInfo;
  ReaderContext ctx //
    {builder, entryPoints, currentFunction, forwardProperties, selectionMerges, loopMerges, currentBlock, propertyTargetResolves,
      propertyReferenceResolves, executionModes, specConstGrabBlock, sourcePosition};
  ErrorRecorder onError{resultInfo};
  if (load(words, word_count, ctx, ctx, onError))
    ctx.finalize(onError);
  return resultInfo;
}
