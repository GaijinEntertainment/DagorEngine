// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "jsonDumpCommon.h"

namespace dafg::jsondump
{

extern Writer w;


static void print_res_request(const ResourceRequest &res_req)
{
  OBJECT_SCOPED

  w.Key("usage");
  {
    OBJECT_SCOPED

    w.Key("type");
    write_string(w, detail::decode_flag_names(res_req.usage.type));

    w.Key("access");
    write_string(w, detail::decode_flag_names(res_req.usage.access));

    w.Key("stage");
    write_string(w, detail::decode_flag_names(res_req.usage.stage));
  }

  w.Key("slotRequest");
  w.Bool(res_req.slotRequest);

  w.Key("optional");
  w.Bool(res_req.optional);

  w.Key("usageDeclared");
  w.Bool(res_req.usageDeclared);

  w.Key("subtypeTag");
  w.Uint64(eastl::to_underlying(res_req.subtypeTag));
}

static void print_virt_subres_ref(const VirtualSubresourceRef &att, const InternalRegistry &registry)
{
  OBJECT_SCOPED

  if (att.nameId == ResNameId::Invalid)
    return;

  w.Key("nameId");
  w.String(get_name(registry, att.nameId));

  w.Key("mipLevel");
  w.Uint(att.mipLevel);

  w.Key("layer");
  w.Uint(att.layer);
}

static void print_bindings_map(const BindingsMap &map, const InternalRegistry &registry)
{
  OBJECT_SCOPED

  for (const auto &[index, binding] : map)
  {
    w.Key(String(0, "%d", index));
    {
      OBJECT_SCOPED

      w.Key("type");
      write_string(w, detail::get_enum_name(binding.type));

      w.Key("resource");
      w.String(get_name(registry, binding.resource));

      w.Key("history");
      w.Bool(binding.history);

      w.Key("reset");
      w.Bool(binding.reset);

      w.Key("optional");
      w.Bool(binding.optional);
    }
  }
}


static void print_u32_blob_param(const auto &param, const InternalRegistry &registry)
{
  OBJECT_SCOPED

  w.Key("variant");
  eastl::visit(
    [&](const auto &variant) {
      using T = eastl::decay_t<decltype(variant)>;

      if constexpr (eastl::is_same_v<T, uint32_t>)
      {
        w.String("uint32_t");
        const uint32_t value = variant;

        w.Key("value");
        w.Uint(value);
      }

      else if constexpr (eastl::is_same_v<T, BlobArg>)
      {
        w.String("BlobArg");
        const BlobArg &blobArg = variant;

        w.Key("name");
        w.String(get_name(registry, blobArg.res));
      }

      else
      {
        w.String("<unknown>");
      }
    },
    param);
}

static void print_mono_u32_blob_param(const auto &param, const InternalRegistry &registry)
{
  OBJECT_SCOPED

  w.Key("variant");
  eastl::visit(
    [&](const auto &variant) {
      using T = eastl::decay_t<decltype(variant)>;

      if constexpr (eastl::is_same_v<T, eastl::monostate>)
      {
        w.String("<null>");
      }

      else if constexpr (eastl::is_same_v<T, uint32_t>)
      {
        w.String("uint32_t");
        const uint32_t value = variant;

        w.Key("value");
        w.Uint(value);
      }

      else if constexpr (eastl::is_same_v<T, BlobArg>)
      {
        w.String("BlobArg");
        const BlobArg &blobArg = variant;

        w.Key("name");
        w.String(get_name(registry, blobArg.res));
      }

      else
      {
        w.String("<unknown>");
      }
    },
    param);
};


static void print_dispatch_direct_args_param(const auto &param, const InternalRegistry &registry)
{
  OBJECT_SCOPED

  w.Key("variant");
  eastl::visit(
    [&](const auto &variant) {
      using T = eastl::decay_t<decltype(variant)>;

      if constexpr (eastl::is_same_v<T, uint32_t>)
      {
        w.String("uint32_t");
        const uint32_t value = variant;

        w.Key("value");
        w.Uint(value);
      }

      else if constexpr (eastl::is_same_v<T, BlobArg>)
      {
        w.String("BlobArg");
        const BlobArg &blobArg = variant;

        w.Key("name");
        w.String(get_name(registry, blobArg.res));
      }

      else if constexpr (eastl::is_same_v<T, DispatchAutoResolutionArg>)
      {
        w.String("DispatchAutoResolutionArg");
        const DispatchAutoResolutionArg &dispAutoResArg = variant;

        w.Key("name");
        w.String(get_name(registry, dispAutoResArg.autoResTypeId));

        w.Key("multiplier");
        w.Double(dispAutoResArg.multiplier);
      }

      else if constexpr (eastl::is_same_v<T, DispatchTextureResolutionArg>)
      {
        w.String("DispatchTextureResolutionArg");
        const DispatchTextureResolutionArg &dispTexResArg = variant;

        w.Key("name");
        w.String(get_name(registry, dispTexResArg.res));
      }

      else
      {
        w.String("<unknown>");
      }
    },
    param);
}

static void print_dispatch_direct_args(const DispatchRequirements::DirectArgs &direct_args, const InternalRegistry &registry)
{
  w.Key("x");
  print_dispatch_direct_args_param(direct_args.x, registry);

  w.Key("y");
  print_dispatch_direct_args_param(direct_args.y, registry);

  w.Key("z");
  print_dispatch_direct_args_param(direct_args.z, registry);
}


static void print_dispatch_indirect_args(const DispatchRequirements::IndirectArgs &args, const InternalRegistry &registry)
{
  w.Key("name");
  w.String(get_name(registry, args.res));

  w.Key("offset");
  print_u32_blob_param(args.offset, registry);
};

static void print_dispatch_mesh_indirect_args(const DispatchRequirements::MeshIndirectArgs &args, const InternalRegistry &registry)
{
  print_dispatch_indirect_args(args, registry);

  w.Key("stride");
  print_mono_u32_blob_param(args.stride, registry);

  w.Key("count");
  print_mono_u32_blob_param(args.count, registry);
};

static void print_dispatch_mesh_indirect_count_args(const DispatchRequirements::MeshIndirectCountArgs &args,
  const InternalRegistry &registry)
{
  print_dispatch_mesh_indirect_args(args, registry);

  w.Key("countRes");
  w.String(get_name(registry, args.countRes));

  w.Key("countOffset");
  print_u32_blob_param(args.countOffset, registry);

  w.Key("maxCount");
  print_mono_u32_blob_param(args.maxCount, registry);
};

static void print_dispatch_threads(const DispatchRequirements::Threads &args, const InternalRegistry &registry)
{
  print_dispatch_direct_args(args, registry);
};

static void print_dispatch_mesh_threads(const DispatchRequirements::MeshThreads &args, const InternalRegistry &registry)
{
  print_dispatch_threads(args, registry);
};

static void print_dispatch_groups(const DispatchRequirements::Groups &args, const InternalRegistry &registry)
{
  print_dispatch_direct_args(args, registry);
};

static void print_dispatch_mesh_groups(const DispatchRequirements::MeshGroups &args, const InternalRegistry &registry)
{
  print_dispatch_groups(args, registry);
};


static void print_draw_draw_args(const DrawRequirements::DrawArgs &args)
{
  w.Key("primitive");
  write_string(w, detail::get_enum_name(args.primitive));
};

static void print_draw_direct_args(const DrawRequirements::DirectArgs &args, const InternalRegistry &registry)
{
  print_draw_draw_args(args);

  w.Key("primitiveCount");
  print_mono_u32_blob_param(args.primitiveCount, registry);

  w.Key("instanceCount");
  print_u32_blob_param(args.instanceCount, registry);

  w.Key("startInstance");
  print_u32_blob_param(args.startInstance, registry);
};

static void print_draw_direct_non_indexed_args(const DrawRequirements::DirectNonIndexedArgs &args, const InternalRegistry &registry)
{
  print_draw_direct_args(args, registry);

  w.Key("startVertex");
  print_u32_blob_param(args.startVertex, registry);
};

static void print_draw_direct_indexed_args(const DrawRequirements::DirectIndexedArgs &args, const InternalRegistry &registry)
{
  print_draw_direct_args(args, registry);

  w.Key("startIndex");
  print_u32_blob_param(args.startIndex, registry);

  w.Key("baseVertex");
  print_u32_blob_param(args.baseVertex, registry);
};

static void print_draw_indirect_args(const DrawRequirements::IndirectArgs &args, const InternalRegistry &registry)
{
  print_draw_draw_args(args);

  w.Key("buffer");
  w.String(get_name(registry, args.buffer));

  w.Key("offset");
  print_u32_blob_param(args.offset, registry);
};

static void print_draw_indirect_non_indexed_args(const DrawRequirements::IndirectNonIndexedArgs &args,
  const InternalRegistry &registry)
{
  print_draw_indirect_args(args, registry);
};

static void print_draw_indirect_indexed_args(const DrawRequirements::IndirectIndexedArgs &args, const InternalRegistry &registry)
{
  print_draw_indirect_args(args, registry);
};

static void print_draw_multi_indirect_args(const DrawRequirements::MultiIndirectArgs &args, const InternalRegistry &registry)
{
  print_draw_indirect_args(args, registry);

  w.Key("stride");
  print_mono_u32_blob_param(args.stride, registry);

  w.Key("drawCount");
  print_mono_u32_blob_param(args.drawCount, registry);
};

static void print_draw_multi_indirect_non_indexed_args(const DrawRequirements::MultiIndirectNonIndexedArgs &args,
  const InternalRegistry &registry)
{
  print_draw_multi_indirect_args(args, registry);
};

static void print_draw_multi_indirect_indexed_args(const DrawRequirements::MultiIndirectIndexedArgs &args,
  const InternalRegistry &registry)
{
  print_draw_multi_indirect_args(args, registry);
};


void dump_registry_to_json(const InternalRegistry &registry)
{
  w.Key("internalRegistry");
  {
    OBJECT_SCOPED

    w.Key("nodes");
    {
      OBJECT_SCOPED
      for (const auto &[nodeNameId, nodeData] : registry.nodes.enumerate())
      {
        w.Key(get_name(registry, nodeNameId));
        {
          OBJECT_SCOPED

          if (!nodeData.declare)
            continue;

          w.Key("priority");
          w.Int(nodeData.priority);

          w.Key("multiplexingMode");
          write_string(w, nodeData.multiplexingMode ? detail::decode_flag_names(*nodeData.multiplexingMode) : "");

          w.Key("sideEffect");
          write_string(w, detail::get_enum_name(nodeData.sideEffect));

          w.Key("enabled");
          w.Bool(nodeData.enabled);

          w.Key("allowAsyncPipelines");
          w.Bool(nodeData.allowAsyncPipelines);

          w.Key("hasCustomExecution");
          w.Bool(nodeData.hasCustomExecution);

          w.Key("generation");
          w.Uint(nodeData.generation);

          w.Key("indexSource");
          {
            OBJECT_SCOPED
            if (nodeData.indexSource)
            {
              w.Key("buffer");
              w.String(get_name(registry, nodeData.indexSource->buffer));
            }
          }

          w.Key("stateRequirements");
          {
            OBJECT_SCOPED
            if (nodeData.stateRequirements)
            {
              w.Key("supportsWireframe");
              w.Bool(nodeData.stateRequirements->supportsWireframe);

              w.Key("vrsState");
              {
                OBJECT_SCOPED

                w.Key("rateX");
                w.Uint(nodeData.stateRequirements->vrsState.rateX);

                w.Key("rateY");
                w.Uint(nodeData.stateRequirements->vrsState.rateY);

                w.Key("vertexCombiner");
                write_string(w, detail::get_enum_name(nodeData.stateRequirements->vrsState.vertexCombiner));

                w.Key("pixelCombiner");
                write_string(w, detail::get_enum_name(nodeData.stateRequirements->vrsState.pixelCombiner));
              }

              w.Key("pipelineStateOverride");
              {
                OBJECT_SCOPED
                if (nodeData.stateRequirements->pipelineStateOverride)
                  print_shaders_override_state(*nodeData.stateRequirements->pipelineStateOverride);
              }
            }
          }

          w.Key("precedingNodeIds");
          {
            ARRAY_SCOPED
            for (const NodeNameId nodeNameId : nodeData.precedingNodeIds)
              w.String(get_name(registry, nodeNameId));
          }

          w.Key("followingNodeIds");
          {
            ARRAY_SCOPED
            for (const NodeNameId nodeNameId : nodeData.followingNodeIds)
              w.String(get_name(registry, nodeNameId));
          }

          w.Key("createdResources");
          {
            ARRAY_SCOPED
            for (const ResNameId resNameId : nodeData.createdResources)
              w.String(get_name(registry, resNameId));
          }

          w.Key("modifiedResources");
          {
            ARRAY_SCOPED
            for (const ResNameId resNameId : nodeData.modifiedResources)
              w.String(get_name(registry, resNameId));
          }

          w.Key("readResources");
          {
            ARRAY_SCOPED
            for (const ResNameId resNameId : nodeData.readResources)
              w.String(get_name(registry, resNameId));
          }

          w.Key("renamedResources");
          {
            OBJECT_SCOPED
            for (const auto [toResNameId, fromResNameId] : nodeData.renamedResources)
            {
              w.Key(get_name(registry, fromResNameId));
              w.String(get_name(registry, toResNameId));
            }
          }

          w.Key("resourceRequests");
          {
            OBJECT_SCOPED
            for (const auto &[resNameId, resourceRequest] : nodeData.resourceRequests)
            {
              w.Key(get_name(registry, resNameId));
              print_res_request(resourceRequest);
            }
          }

          w.Key("historyResourceReadRequests");
          {
            OBJECT_SCOPED
            for (const auto &[resNameId, resourceRequest] : nodeData.historyResourceReadRequests)
            {
              w.Key(get_name(registry, resNameId));
              print_res_request(resourceRequest);
            }
          }

          w.Key("renderingRequirements");
          {
            OBJECT_SCOPED
            if (nodeData.renderingRequirements)
            {
              w.Key("colorAttachments");
              {
                ARRAY_SCOPED
                for (const auto colorAtt : nodeData.renderingRequirements->colorAttachments)
                  print_virt_subres_ref(colorAtt, registry);
              }

              w.Key("depthAttachment");
              print_virt_subres_ref(nodeData.renderingRequirements->depthAttachment, registry);

              w.Key("depthReadOnly");
              w.Bool(nodeData.renderingRequirements->depthReadOnly);

              w.Key("depthReadTestOnly");
              w.Bool(nodeData.renderingRequirements->depthReadTestOnly);

              w.Key("resolves");
              {
                OBJECT_SCOPED
                for (const auto [fromResNameId, toResNameId] : nodeData.renderingRequirements->resolves)
                {
                  w.Key(get_name(registry, fromResNameId));
                  w.String(get_name(registry, toResNameId));
                }
              }

              w.Key("vrsRateAttachment");
              print_virt_subres_ref(nodeData.renderingRequirements->vrsRateAttachment, registry);
            }
          }

          w.Key("executeRequirements");
          {
            OBJECT_SCOPED

            w.Key("variant");
            eastl::visit(
              [&](const auto &exec_reqs) {
                using T = eastl::decay_t<decltype(exec_reqs)>;

                if constexpr (eastl::is_same_v<T, eastl::monostate>)
                {
                  w.String("<null>");
                }

                else if constexpr (eastl::is_same_v<T, DispatchRequirements>)
                {
                  w.String("DispatchRequirements");
                  const DispatchRequirements &dispReqs = exec_reqs;

                  w.Key("shaderName");
                  w.String(get_name(registry, dispReqs.shaderId));

                  w.Key("arguments");
                  {
                    OBJECT_SCOPED

                    w.Key("variant");
                    eastl::visit(
                      [&](const auto &args) {
                        using T = eastl::decay_t<decltype(args)>;

                        if constexpr (eastl::is_same_v<T, DispatchRequirements::Threads>)
                        {
                          w.String("Threads");
                          print_dispatch_threads(args, registry);
                        }

                        else if constexpr (eastl::is_same_v<T, DispatchRequirements::MeshThreads>)
                        {
                          w.String("MeshThreads");
                          print_dispatch_mesh_threads(args, registry);
                        }

                        else if constexpr (eastl::is_same_v<T, DispatchRequirements::Groups>)
                        {
                          w.String("Groups");
                          print_dispatch_groups(args, registry);
                        }

                        else if constexpr (eastl::is_same_v<T, DispatchRequirements::MeshGroups>)
                        {
                          w.String("MeshGroups");
                          print_dispatch_mesh_groups(args, registry);
                        }

                        else if constexpr (eastl::is_same_v<T, DispatchRequirements::IndirectArgs>)
                        {
                          w.String("IndirectArgs");
                          print_dispatch_indirect_args(args, registry);
                        }

                        else if constexpr (eastl::is_same_v<T, DispatchRequirements::MeshIndirectArgs>)
                        {
                          w.String("MeshIndirectArgs");
                          print_dispatch_mesh_indirect_args(args, registry);
                        }

                        else if constexpr (eastl::is_same_v<T, DispatchRequirements::MeshIndirectCountArgs>)
                        {
                          w.String("MeshIndirectCountArgs");
                          print_dispatch_mesh_indirect_count_args(args, registry);
                        }

                        else
                        {
                          w.String("<unknown>");
                        }
                      },
                      dispReqs.arguments);
                  }
                }

                else if constexpr (eastl::is_same_v<T, DrawRequirements>)
                {
                  w.String("DrawRequirements");
                  const DrawRequirements &drawReqs = exec_reqs;

                  w.Key("shaderName");
                  w.String(get_name(registry, drawReqs.shaderId));

                  w.Key("arguments");
                  {
                    OBJECT_SCOPED

                    w.Key("variant");
                    eastl::visit(
                      [&](const auto &args) {
                        using T = eastl::decay_t<decltype(args)>;

                        if constexpr (eastl::is_same_v<T, DrawRequirements::DirectNonIndexedArgs>)
                        {
                          w.String("DirectNonIndexedArgs");
                          print_draw_direct_non_indexed_args(args, registry);
                        }

                        else if constexpr (eastl::is_same_v<T, DrawRequirements::DirectIndexedArgs>)
                        {
                          w.String("DirectIndexedArgs");
                          print_draw_direct_indexed_args(args, registry);
                        }

                        else if constexpr (eastl::is_same_v<T, DrawRequirements::IndirectNonIndexedArgs>)
                        {
                          w.String("IndirectNonIndexedArgs");
                          print_draw_indirect_non_indexed_args(args, registry);
                        }

                        else if constexpr (eastl::is_same_v<T, DrawRequirements::IndirectIndexedArgs>)
                        {
                          w.String("IndirectIndexedArgs");
                          print_draw_indirect_indexed_args(args, registry);
                        }

                        else if constexpr (eastl::is_same_v<T, DrawRequirements::MultiIndirectNonIndexedArgs>)
                        {
                          w.String("MultiIndirectNonIndexedArgs");
                          print_draw_multi_indirect_non_indexed_args(args, registry);
                        }

                        else if constexpr (eastl::is_same_v<T, DrawRequirements::MultiIndirectIndexedArgs>)
                        {
                          w.String("MultiIndirectIndexedArgs");
                          print_draw_multi_indirect_indexed_args(args, registry);
                        }

                        else
                        {
                          w.String("<unknown>");
                        }
                      },
                      drawReqs.arguments);
                  }
                }

                else if constexpr (eastl::is_same_v<T, CopyRequirements>)
                {
                  w.String("CopyRequirements");
                  const CopyRequirements &copyReqs = exec_reqs;

                  w.Key("source");
                  w.String(get_name(registry, copyReqs.source));

                  w.Key("sourceIsHistory");
                  w.Bool(copyReqs.sourceIsHistory);

                  w.Key("destination");
                  w.String(get_name(registry, copyReqs.destination));
                }

                else
                {
                  w.String("<unknown>");
                }
              },
              nodeData.executeRequirements);
          }

          w.Key("vertexSources");
          {
            OBJECT_SCOPED
            for (uint32_t i = 0; i < nodeData.vertexSources.size(); ++i)
            {
              w.Key(String(0, "%d", i));
              {
                OBJECT_SCOPED
                if (nodeData.vertexSources[i])
                {
                  w.Key("buffer");
                  w.String(get_name(registry, nodeData.vertexSources[i]->buffer));

                  w.Key("stride");
                  w.Uint(nodeData.vertexSources[i]->stride);
                }
              }
            }
          }

          w.Key("bindings");
          print_bindings_map(nodeData.bindings, registry);

          w.Key("shaderBlockLayers");
          {
            OBJECT_SCOPED

            w.Key("objectLayer");
            w.Int(nodeData.shaderBlockLayers.objectLayer);

            w.Key("frameLayer");
            w.Int(nodeData.shaderBlockLayers.frameLayer);

            w.Key("sceneLayer");
            w.Int(nodeData.shaderBlockLayers.sceneLayer);
          }

          w.Key("refinedBlockBindings");
          {
            OBJECT_SCOPED
            for (const auto &[refBlockNameId, bindingsMap] : nodeData.refinedBlockBindings)
            {
              w.Key(get_name(registry, refBlockNameId));
              print_bindings_map(bindingsMap, registry);
            }
          }

          w.Key("usedRefinedBlock");
          {
            OBJECT_SCOPED
            if (nodeData.usedRefinedBlock)
            {
              w.Key("name");
              w.String(get_name(registry, *nodeData.usedRefinedBlock));
            }
          }

          w.Key("nodeSource");
          write_string(w, nodeData.nodeSource);
        }
      }
    }

    w.Key("resources");
    {
      OBJECT_SCOPED
      for (const auto &[resNameId, resData] : registry.resources.enumerate())
      {
        w.Key(get_name(registry, resNameId));
        {
          OBJECT_SCOPED

          w.Key("history");
          write_string(w, detail::get_enum_name(resData.history));

          w.Key("createdResData");
          {
            OBJECT_SCOPED
            if (resData.createdResData.has_value())
            {
              w.Key("creationInfo");
              {
                OBJECT_SCOPED

                w.Key("variant");
                eastl::visit(
                  [&](const auto &creation_info) {
                    using T = eastl::decay_t<decltype(creation_info)>;

                    if constexpr (eastl::is_same_v<T, eastl::monostate>)
                    {
                      w.String("<null>");
                    }

                    else if constexpr (eastl::is_same_v<T, Texture2dCreateInfo>)
                    {
                      w.String("Texture2dCreateInfo");
                      const Texture2dCreateInfo &tex2dCreateInfo = creation_info;

                      w.Key("creationFlags");
                      w.Uint(tex2dCreateInfo.creationFlags);

                      w.Key("resolution");
                      {
                        OBJECT_SCOPED

                        w.Key("variant");
                        eastl::visit(
                          [&](const auto &resolution) {
                            using T = eastl::decay_t<decltype(resolution)>;

                            if constexpr (eastl::is_same_v<T, IPoint2>)
                            {
                              w.String("IPoint2");
                              const IPoint2 &p2 = resolution;

                              w.Key("x");
                              w.Int(p2.x);

                              w.Key("y");
                              w.Int(p2.y);
                            }

                            else if constexpr (eastl::is_same_v<T, AutoResolutionRequest<2>>)
                            {
                              w.String("AutoResolutionRequest<2>");
                              const AutoResolutionRequest<2> &arr = resolution;

                              w.Key("name");
                              w.String(get_name(registry, arr.autoResTypeId));

                              w.Key("multiplier");
                              w.Double(arr.multiplier);
                            }

                            else
                            {
                              w.String("<unknown>");
                            }
                          },
                          tex2dCreateInfo.resolution);
                      }

                      w.Key("mipLevels");
                      w.Uint(tex2dCreateInfo.mipLevels);

                      w.Key("arrayLayers");
                      w.Uint(tex2dCreateInfo.arrayLayers);
                    }

                    else if constexpr (eastl::is_same_v<T, Texture3dCreateInfo>)
                    {
                      w.String("Texture3dCreateInfo");
                      const Texture3dCreateInfo &tex3dCreateInfo = creation_info;

                      w.Key("creationFlags");
                      w.Uint(tex3dCreateInfo.creationFlags);

                      w.Key("resolution");
                      {
                        OBJECT_SCOPED

                        w.Key("variant");
                        eastl::visit(
                          [&](const auto &resolution) {
                            using T = eastl::decay_t<decltype(resolution)>;

                            if constexpr (eastl::is_same_v<T, IPoint3>)
                            {
                              w.String("IPoint3");
                              const IPoint3 &p3 = resolution;

                              w.Key("x");
                              w.Int(p3.x);

                              w.Key("y");
                              w.Int(p3.y);

                              w.Key("z");
                              w.Int(p3.z);
                            }

                            else if constexpr (eastl::is_same_v<T, AutoResolutionRequest<3>>)
                            {
                              w.String("AutoResolutionRequest<3>");
                              const AutoResolutionRequest<3> &arr = resolution;

                              w.Key("name");
                              w.String(get_name(registry, arr.autoResTypeId));

                              w.Key("multiplier");
                              w.Double(arr.multiplier);
                            }

                            else
                            {
                              w.String("<unknown>");
                            }
                          },
                          tex3dCreateInfo.resolution);
                      }

                      w.Key("mipLevels");
                      w.Uint(tex3dCreateInfo.mipLevels);
                    }

                    else if constexpr (eastl::is_same_v<T, BufferCreateInfo>)
                    {
                      w.String("BufferCreateInfo");
                      const BufferCreateInfo &bufferCreateInfo = creation_info;

                      w.Key("elementSize");
                      w.Uint(bufferCreateInfo.elementSize);

                      w.Key("elementCount");
                      w.Uint(bufferCreateInfo.elementCount);

                      w.Key("flags");
                      w.Uint(bufferCreateInfo.flags);

                      w.Key("format");
                      w.Uint(bufferCreateInfo.format);
                    }

                    else if constexpr (eastl::is_same_v<T, BlobDescription>)
                    {
                      w.String("BlobDescription");
                      const BlobDescription &blobDescr = creation_info;

                      w.Key("typeTag");
                      w.Uint64(eastl::to_underlying(blobDescr.typeTag));

                      w.Key("ctorOverride");
                      w.Bool(bool(blobDescr.ctorOverride));
                    }

                    else if constexpr (eastl::is_same_v<T, ExternalResourceProvider>)
                    {
                      w.String("ExternalResourceProvider");
                    }

                    else if constexpr (eastl::is_same_v<T, DriverDeferredTexture>)
                    {
                      w.String("DriverDeferredTexture");
                    }

                    else
                    {
                      w.String("<unknown>");
                    }
                  },
                  resData.createdResData->creationInfo);
              }

              w.Key("resolution");
              {
                OBJECT_SCOPED
                if (resData.createdResData->resolution.has_value())
                {
                  w.Key("name");
                  w.String(get_name(registry, resData.createdResData->resolution->id));

                  w.Key("multiplier");
                  w.Double(resData.createdResData->resolution->multiplier);
                }
              }

              w.Key("clearValue");
              {
                OBJECT_SCOPED

                w.Key("variant");
                eastl::visit(
                  [&](const auto &clear_value) {
                    using T = eastl::decay_t<decltype(clear_value)>;

                    if constexpr (eastl::is_same_v<T, eastl::monostate>)
                    {
                      w.String("<null>");
                    }

                    else if constexpr (eastl::is_same_v<T, ResourceClearValue>)
                    {
                      w.String("ResourceClearValue");
                      const ResourceClearValue &clearVal = clear_value;

                      w.Key("asUint");
                      {
                        ARRAY_SCOPED
                        w.Uint(clearVal.asUint[0]);
                        w.Uint(clearVal.asUint[1]);
                        w.Uint(clearVal.asUint[2]);
                        w.Uint(clearVal.asUint[3]);
                      }
                    }

                    else if constexpr (eastl::is_same_v<T, DynamicParameter>)
                    {
                      w.String("DynamicParameter");
                      const DynamicParameter &dynClearVal = clear_value;

                      w.Key("name");
                      w.String(get_name(registry, dynClearVal.resource));
                    }

                    else
                    {
                      w.String("<unknown>");
                    }
                  },
                  resData.createdResData->clearValue);
              }

              w.Key("clearFlags");
              write_string(w, detail::decode_flag_names(resData.createdResData->clearFlags));

              w.Key("type");
              write_string(w, detail::get_enum_name(resData.createdResData->type));
            }
          }
        }
      }
    }

    w.Key("autoResTypes");
    {
      OBJECT_SCOPED
      for (const auto &[autoResNameId, autoResData] : registry.autoResTypes.enumerate())
      {
        w.Key(get_name(registry, autoResNameId));
        {
          OBJECT_SCOPED

          w.Key("values");
          {
            OBJECT_SCOPED

            w.Key("variant");
            eastl::visit(
              [&](const auto &values) {
                using T = eastl::decay_t<decltype(values)>;

                if constexpr (eastl::is_same_v<T, eastl::monostate>)
                {
                  w.String("<null>");
                }

                else if constexpr (eastl::is_same_v<T, ResolutionValues<IPoint2>>)
                {
                  w.String("ResolutionValues<IPoint2>");
                  const ResolutionValues<IPoint2> &vals = values;

                  w.Key("staticResolution");
                  {
                    OBJECT_SCOPED

                    w.Key("x");
                    w.Int(vals.staticResolution.x);

                    w.Key("y");
                    w.Int(vals.staticResolution.y);
                  }

                  w.Key("dynamicResolution");
                  {
                    OBJECT_SCOPED

                    w.Key("x");
                    w.Int(vals.dynamicResolution.x);

                    w.Key("y");
                    w.Int(vals.dynamicResolution.y);
                  }

                  w.Key("lastAppliedDynamicResolution");
                  {
                    OBJECT_SCOPED

                    w.Key("x");
                    w.Int(vals.lastAppliedDynamicResolution.x);

                    w.Key("y");
                    w.Int(vals.lastAppliedDynamicResolution.y);
                  }
                }

                else if constexpr (eastl::is_same_v<T, ResolutionValues<IPoint3>>)
                {
                  w.String("ResolutionValues<IPoint3>");
                  const ResolutionValues<IPoint3> &vals = values;

                  w.Key("staticResolution");
                  {
                    OBJECT_SCOPED

                    w.Key("x");
                    w.Int(vals.staticResolution.x);

                    w.Key("y");
                    w.Int(vals.staticResolution.y);

                    w.Key("z");
                    w.Int(vals.staticResolution.z);
                  }

                  w.Key("dynamicResolution");
                  {
                    OBJECT_SCOPED

                    w.Key("x");
                    w.Int(vals.dynamicResolution.x);

                    w.Key("y");
                    w.Int(vals.dynamicResolution.y);

                    w.Key("z");
                    w.Int(vals.dynamicResolution.z);
                  }

                  w.Key("lastAppliedDynamicResolution");
                  {
                    OBJECT_SCOPED

                    w.Key("x");
                    w.Int(vals.lastAppliedDynamicResolution.x);

                    w.Key("y");
                    w.Int(vals.lastAppliedDynamicResolution.y);

                    w.Key("z");
                    w.Int(vals.lastAppliedDynamicResolution.z);
                  }
                }

                else
                {
                  w.String("<unknown>");
                }
              },
              autoResData.values);
          }

          w.Key("dynamicResolutionCountdown");
          w.Uint(autoResData.dynamicResolutionCountdown);
        }
      }
    }

    w.Key("autoResTypesChanged");
    {
      OBJECT_SCOPED
      for (const auto &[autoResNameId, value] : registry.autoResTypesChanged.enumerate())
      {
        w.Key(get_name(registry, autoResNameId));
        w.Bool(value);
      }
    }

    w.Key("resourceSlots");
    {
      OBJECT_SCOPED
      for (const auto &[resNameId, slotDataOpt] : registry.resourceSlots.enumerate())
      {
        if (!slotDataOpt.has_value())
          continue;

        w.Key(get_name(registry, resNameId));
        {
          OBJECT_SCOPED

          w.Key("contents");
          w.String(get_name(registry, slotDataOpt->contents));

          w.Key("prevContents");
          w.String(get_name(registry, slotDataOpt->prevContents));
        }
      }
    }

    w.Key("sinkExternalResources");
    {
      ARRAY_SCOPED
      for (const auto resNameId : registry.sinkExternalResources)
        w.String(get_name(registry, resNameId));
    }

    w.Key("defaultMultiplexingMode");
    write_string(w, detail::decode_flag_names(registry.defaultMultiplexingMode));

    w.Key("defaultHistoryMultiplexingMode");
    write_string(w, detail::decode_flag_names(registry.defaultHistoryMultiplexingMode));
  }
}

} // namespace dafg::jsondump
