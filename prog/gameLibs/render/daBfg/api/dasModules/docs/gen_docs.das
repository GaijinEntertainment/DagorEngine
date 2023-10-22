require daBfg
require rtti
require fio
require daslib/rst
require daslib/regex
require daslib/regex_boost

[export]
def main
  let dasRoot = get_das_root()
  let dagorRoot = "{dasRoot}/../../.."
  let docsRoot = "{dagorRoot}/prog/engine/_docs/render/docs/source/index/daBFG"
  var mod = get_module("daBfg")
  var groups <- [{DocGroup
    group_by_regex("Top level functions", mod, %regex~(root|registerNode|get_shader_variable_id|fillSlot|resetNode)$%%);
    group_by_regex("Registry manipulation", mod, %regex~(requestRenderPass|orderMeBefore|orderMeAfter|setPriority|multiplex|executionHas|create|read|history|modify|rename|requestState)$%%);
    group_by_regex("Request manipulation", mod, %regex~(depthRo|depthRw|color|texture|buffer|blob|useAs|atStage|bindToShaderVar|handle|view|setFrameBlock|setSceneBlock|setObjectBlock|allowWireFrame|allowVrs|enableOverride)$%%);
    hide_group(group_by_regex("Resource data manipulation", mod, %regex~(setDescription|setResolution|setTextureInfo)$%%));
    hide_group(group_by_regex("Node data manipulation", mod, %regex~(hasStateRequirements|addStateRequirements|hasRenderingRequirements|addRenderingRequirements|setNodeSource|setWireframe|setVrs|setOverride|setDepthReadOnly|getColorAttachments|getDepthAttachment|registerNodeDeclaration)$%%));
    hide_group(group_by_regex("Internal container manipulation", mod, %regex~(set|insert|emplace|getNameId|addNameId|getName|empty|clear|reserve|contains|get)$%%));
    hide_group(group_by_regex("Misc", mod, %regex~(getBufView|getTexView|thisRequest|NodeHandle|unregisterNode|getTracker|using|getRegistry|getResourceProvider|register_external_node)$%%));
  }]
  topic_root = "{dagorRoot}/prog/gameLibs/render/daBfg/api/dasModules/docs/detail"
  document("DaScript module", mod, "{docsRoot}/das.rst", "", groups)
  var rstText = ""
  fopen("{docsRoot}/das.rst", "rb") <| $(f)
    if f != null
      rstText = fread(f)
  fopen("{docsRoot}/das.rst", "wb") <| $(f)
    fwrite(f, "..\n  This is auto generated file. See daBfg/api/dasModules/docs\n")
    if f != null
      fwrite(f, rstText)