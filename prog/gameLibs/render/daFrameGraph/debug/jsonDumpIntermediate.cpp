// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "jsonDumpCommon.h"

namespace dafg::jsondump
{

extern Writer w;


static void print_subres_ref(const intermediate::SubresourceRef &att, const intermediate::Graph &graph)
{
  w.Key("resource");
  w.String(get_name(graph, att.resource));

  w.Key("mipLevel");
  w.Uint(att.mipLevel);

  w.Key("layer");
  w.Uint(att.layer);
}


void dump_ir_graph_to_json(const InternalRegistry &registry, const intermediate::Graph &graph)
{
  w.Key("intermediateGraph");
  {
    OBJECT_SCOPED

    w.Key("nodes");
    {
      OBJECT_SCOPED
      for (const auto &[nodeIdx, node] : graph.nodes.enumerate())
      {
        w.Key(get_name(graph, nodeIdx));
        {
          OBJECT_SCOPED

          w.Key("priority");
          w.Int(node.priority);

          w.Key("resourceRequests");
          {
            ARRAY_SCOPED
            for (const auto &resRequest : node.resourceRequests)
            {
              OBJECT_SCOPED

              w.Key("resource");
              w.String(get_name(graph, resRequest.resource));

              w.Key("usage");
              {
                OBJECT_SCOPED

                w.Key("type");
                write_string(w, detail::decode_flag_names(resRequest.usage.type));

                w.Key("access");
                write_string(w, detail::decode_flag_names(resRequest.usage.access));

                w.Key("stage");
                write_string(w, detail::decode_flag_names(resRequest.usage.stage));
              }

              w.Key("fromLastFrame");
              w.Bool(resRequest.fromLastFrame);
            }
          }

          w.Key("supressedRequests");
          {
            ARRAY_SCOPED
            for (const auto suppReqIdx : node.supressedRequests)
              w.String(get_name(graph, suppReqIdx));
          }

          w.Key("predecessors");
          {
            ARRAY_SCOPED
            for (const auto predNodeIdx : node.predecessors)
              w.String(get_name(graph, predNodeIdx));
          }

          w.Key("frontendNode");
          {
            OBJECT_SCOPED
            if (node.frontendNode)
            {
              w.Key("name");
              w.String(get_name(registry, *node.frontendNode));
            }
          }

          w.Key("multiplexingIndex");
          w.Uint(node.multiplexingIndex);

          w.Key("hasSideEffects");
          w.Bool(node.hasSideEffects);
        }
      }
    }

    w.Key("nodeStates");
    {
      OBJECT_SCOPED
      for (const auto &[nodeIdx, nodeState] : graph.nodeStates.enumerate())
      {
        w.Key(get_name(graph, nodeIdx));
        {
          OBJECT_SCOPED

          w.Key("asyncPipelines");
          w.Bool(nodeState.asyncPipelines);

          w.Key("wire");
          w.Bool(nodeState.wire);

          w.Key("vrs");
          {
            OBJECT_SCOPED
            print_intermediate_vrs_state(nodeState.vrs);
          }

          w.Key("pass");
          {
            OBJECT_SCOPED
            if (nodeState.pass)
            {
              w.Key("colorAttachments");
              {
                ARRAY_SCOPED
                for (const auto &colorAtt : nodeState.pass->colorAttachments)
                {
                  OBJECT_SCOPED
                  if (colorAtt)
                    print_subres_ref(*colorAtt, graph);
                }
              }

              w.Key("depthAttachment");
              {
                OBJECT_SCOPED
                if (nodeState.pass->depthAttachment)
                  print_subres_ref(*nodeState.pass->depthAttachment, graph);
              }

              w.Key("depthReadOnly");
              w.Bool(nodeState.pass->depthReadOnly);

              w.Key("resolves");
              {
                OBJECT_SCOPED
                for (const auto [fromResIdx, toResIdx] : nodeState.pass->resolves)
                {
                  w.Key(get_name(graph, fromResIdx));
                  w.String(get_name(graph, toResIdx));
                }
              }

              w.Key("isLegacyPass");
              w.Bool(nodeState.pass->isLegacyPass);

              w.Key("vrsRateAttachment");
              {
                OBJECT_SCOPED
                if (nodeState.pass->vrsRateAttachment)
                  print_subres_ref(*nodeState.pass->vrsRateAttachment, graph);
              }
            }
          }

          w.Key("shaderOverrides");
          {
            OBJECT_SCOPED
            if (nodeState.shaderOverrides)
              print_shaders_override_state(*nodeState.shaderOverrides);
          }

          w.Key("vertexSources");
          {
            OBJECT_SCOPED
            for (uint32_t i = 0; i < nodeState.vertexSources.size(); ++i)
            {
              w.Key(String(0, "%d", i));
              {
                OBJECT_SCOPED
                if (nodeState.vertexSources[i])
                  print_intermediate_vertex_source(*nodeState.vertexSources[i], graph);
              }
            }
          }

          w.Key("indexSource");
          {
            OBJECT_SCOPED
            if (nodeState.indexSource)
              print_intermediate_index_source(*nodeState.indexSource, graph);
          }

          w.Key("bindings");
          {
            OBJECT_SCOPED
            for (const auto &[index, binding] : nodeState.bindings)
            {
              w.Key(String(0, "%d", index));
              {
                OBJECT_SCOPED

                w.Key("type");
                write_string(w, detail::get_enum_name(binding.type));

                w.Key("resource");
                if (binding.resource)
                  w.String(get_name(graph, *binding.resource));
                else
                  w.String("");

                w.Key("history");
                w.Bool(binding.history);

                w.Key("reset");
                w.Bool(binding.reset);

                w.Key("optional");
                w.Bool(binding.optional);
              }
            }
          }

          w.Key("shaderBlockLayers");
          {
            OBJECT_SCOPED

            w.Key("objectLayer");
            w.Int(nodeState.shaderBlockLayers.objectLayer);

            w.Key("frameLayer");
            w.Int(nodeState.shaderBlockLayers.frameLayer);

            w.Key("sceneLayer");
            w.Int(nodeState.shaderBlockLayers.sceneLayer);
          }
        }
      }
    }

    w.Key("resources");
    {
      OBJECT_SCOPED
      for (const auto &[resourceIdx, resource] : graph.resources.enumerate())
      {
        w.Key(get_name(graph, resourceIdx));
        {
          OBJECT_SCOPED

          w.Key("resource");
          {
            OBJECT_SCOPED

            w.Key("variant");
            eastl::visit(
              [&](const auto &resource) {
                using T = eastl::decay_t<decltype(resource)>;

                if constexpr (std::is_same_v<T, eastl::monostate>)
                {
                  w.String("<null>");
                }

                else if constexpr (std::is_same_v<T, intermediate::ScheduledResource>)
                {
                  w.String("ScheduledResource");
                  const intermediate::ScheduledResource &scheduledRes = resource;

                  w.Key("description");
                  {
                    OBJECT_SCOPED

                    w.Key("variant");
                    eastl::visit(
                      [&](const auto &description) {
                        using T = eastl::decay_t<decltype(description)>;

                        if constexpr (std::is_same_v<T, intermediate::GpuDescription>)
                        {
                          w.String("GpuDescription");
                          const ResourceDescription &resDescr = description;

                          w.Key("desc");
                          write_string(w, resDescr.toDebugString());
                        }

                        else if constexpr (std::is_same_v<T, intermediate::BlobDescription>)
                        {
                          w.String("BlobDescription");
                          const intermediate::BlobDescription &blobDescr = description;

                          w.Key("typeTag");
                          w.Uint64(eastl::to_underlying(blobDescr.typeTag));

                          w.Key("size");
                          w.Uint64(blobDescr.size);

                          w.Key("alignment");
                          w.Uint64(blobDescr.alignment);
                        }

                        else
                        {
                          w.String("<unknown>");
                        }
                      },
                      scheduledRes.description);
                  }

                  w.Key("resourceType");
                  write_string(w, detail::get_enum_name(scheduledRes.resourceType));

                  w.Key("resolutionType");
                  {
                    OBJECT_SCOPED
                    if (scheduledRes.resolutionType)
                    {
                      w.Key("name");
                      w.String(get_name(registry, scheduledRes.resolutionType->id));

                      w.Key("multiplier");
                      w.Double(scheduledRes.resolutionType->multiplier);
                    }
                  }

                  w.Key("clearStage");
                  write_string(w, detail::get_enum_name(scheduledRes.clearStage));

                  w.Key("clearValue");
                  {
                    OBJECT_SCOPED

                    w.Key("variant");
                    eastl::visit(
                      [&](const auto &clear_value) {
                        using T = eastl::decay_t<decltype(clear_value)>;

                        if constexpr (std::is_same_v<T, ResourceClearValue>)
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

                        else if constexpr (std::is_same_v<T, intermediate::DynamicParameter>)
                        {
                          w.String("DynamicParameter");
                          const intermediate::DynamicParameter &dynParam = clear_value;

                          w.Key("resource");
                          w.String(get_name(graph, dynParam.resource));
                        }

                        else
                        {
                          w.String("<unknown>");
                        }
                      },
                      scheduledRes.clearValue);
                  }

                  w.Key("clearFlags");
                  write_string(w, detail::decode_flag_names(scheduledRes.clearFlags));

                  w.Key("history");
                  write_string(w, detail::get_enum_name(scheduledRes.history));

                  w.Key("autoMipCount");
                  w.Bool(scheduledRes.autoMipCount);
                }

                else if constexpr (std::is_same_v<T, intermediate::ExternalResource>)
                {
                  w.String("ExternalResource");
                  const intermediate::ExternalResource &extRes = resource;

                  w.Key("info");
                  {
                    OBJECT_SCOPED

                    w.Key("variant");
                    eastl::visit(
                      [&](const auto &info) {
                        using T = eastl::decay_t<decltype(info)>;

                        if constexpr (std::is_same_v<T, TextureInfo>)
                        {
                          w.String("TextureInfo");
                          const TextureInfo &texInfo = info;

                          w.Key("w");
                          w.Uint(texInfo.w);

                          w.Key("h");
                          w.Uint(texInfo.h);

                          w.Key("d");
                          w.Uint(texInfo.d);

                          w.Key("a");
                          w.Uint(texInfo.a);

                          w.Key("mipLevels");
                          w.Uint(texInfo.mipLevels);

                          w.Key("type");
                          write_string(w, detail::get_enum_name(texInfo.type));

                          w.Key("isCommitted");
                          w.Uint(texInfo.isCommitted);

                          w.Key("cflg");
                          w.String(String(0, "%#08X", texInfo.cflg));
                        }

                        else if constexpr (std::is_same_v<T, intermediate::BufferInfo>)
                        {
                          w.String("BufferInfo");
                          const intermediate::BufferInfo &bufInfo = info;

                          w.Key("flags");
                          w.String(String(0, "%#08X", bufInfo.flags));
                        }

                        else
                        {
                          w.String("<unknown>");
                        }
                      },
                      extRes.info);
                  }
                }

                else if constexpr (std::is_same_v<T, DriverDeferredTexture>)
                {
                  w.String("DriverDeferredTexture");
                }

                else
                {
                  w.String("<unknown>");
                }
              },
              resource.resource);
          }

          w.Key("frontendResources");
          {
            ARRAY_SCOPED
            for (const auto frontResNameId : resource.frontendResources)
              w.String(get_name(registry, frontResNameId));
          }

          w.Key("multiplexingIndex");
          w.Uint(resource.multiplexingIndex);
        }
      }
    }
  }
}

} // namespace dafg::jsondump
