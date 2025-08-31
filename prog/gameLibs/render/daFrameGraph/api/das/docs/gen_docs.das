require daFrameGraph.fg_ecs
require daFrameGraph.resource_slot_ecs
require rtti
require fio
require daslib/rst
require daslib/regex
require daslib/regex_boost
require DagorSystem

def document_module(name, title, rst_path : string; make_groups : block<(mod : Module?) : array<DocGroup>>)
  var mod = get_module(name)
  var groups <- make_groups |> invoke(mod)
  document(title, mod, rst_path, "", groups)
  var rstText = ""
  fopen(rst_path, "rb") <| $(f)
    if f != null
      rstText = fread(f)
  fopen(rst_path, "wb") <| $(f)
    fwrite(f, "..\n  This is auto generated file. See daFrameGraph/api/das/docs\n")
    if f != null
      fwrite(f, rstText)

[export]
def main
  let dasRoot = get_das_root()
  let dagorRoot = "{dasRoot}/../../.."
  let docsRoot = "{dagorRoot}/_docs/source/dagor-render/index/daFrameGraph"
  topic_root = "{dagorRoot}/prog/gameLibs/render/daFrameGraph/api/das/docs/detail"
  document_module("daFrameGraph", "FrameGraph in das", "{docsRoot}/das.rst") <| $(mod)
    return <- array<DocGroup>(
      group_by_regex("Top level functions", mod, %regex~(root|registerNode|fillSlot|/)$%%),
      group_by_regex("Registry manipulation", mod, %regex~(requestRenderPass|orderMeBefore|orderMeAfter|setPriority|multiplex|executionHas|create|read|history|modify|rename|requestState|getResolution|get)$%%),
      group_by_regex("Request manipulation", mod, %regex~(depthRo|depthRw|color|texture|buffer|blob|useAs|atStage|bindToShaderVar|handle|view|setFrameBlock|setSceneBlock|setObjectBlock|allowWireFrame|allowVrs|enableOverride|modifyRequest|optional)$%%))
  document_module("fg_ecs", "FrameGraph in das+ecs", "{docsRoot}/fg_ecs.rst") <| $(mod)
    return <- array<DocGroup>(
      hide_group(group_by_regex("Helpers", mod, %regex~(.*)$%%)))
  document_module("daFgCore", "FgCore das module", "{docsRoot}/daFgCore.rst") <| $(mod)
    return <- array<DocGroup>(
      group_by_regex("Top level functions", mod, %regex~(root|registerNode|get_shader_variable_id|fill_slot|resetNode)$%%),
      hide_group(group_by_regex("Resource data manipulation", mod, %regex~(setDescription|setResolution|setTextureInfo)$%%)),
      hide_group(group_by_regex("Node data manipulation", mod, %regex~(setNodeSource|registerNodeDeclaration)$%%)),
      hide_group(group_by_regex("Ecs integration", mod, %regex~(set)$%%)),
      hide_group(group_by_regex("Misc", mod, %regex~(getBufView|getTexView|getResolution|NodeHandle|unregisterNode|getTracker|using|getRegistry|getResourceProvider|register_external_node|is_daframegraph_runtime_initialized)$%%)))

  // Resource slot
  let resSlotRoot = "{dagorRoot}/_docs/source/dagor-render/index/resourceSlot"

  document_module("resource_slot_ecs", "Resource slot in das+ecs", "{resSlotRoot}/resource_slot_ecs.rst") <| $(mod)
    return <- array<DocGroup>(
      hide_group(group_by_regex("Helpers", mod, %regex~(.*)$%%)))

  document_module("resource_slot", "Resource slot in das", "{resSlotRoot}/resource_slot_das.rst") <| $(mod)
    return <- array<DocGroup>(
      group_by_regex("Register access", mod, %regex~(registerAccess)$%%))

  document_module("ResourceSlotCore", "ResourceSlotCore das module", "{resSlotRoot}/ResourceSlotCore.rst") <| $(mod)
    return <- array<DocGroup>(
      group_by_regex("Actions", mod, %regex~(Create|Update|Read)$%%),
      group_by_regex("State operations", mod, %regex~(resourceToReadFrom|resourceToCreateFor)$%%),
      group_by_regex("Handle methods", mod, %regex~(NodeHandleWithSlotsAccess|reset)$%%),
      hide_group(group_by_regex("Helpers", mod, %regex~(SlotAction|using|register_access|create|update|read|prepare_access)$%%)))
