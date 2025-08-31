from "params.gen.nut" import *

begin_declare_params("LightfxShadow");

declare_struct("LightfxShadowParams", 2,
[
  { name="enabled", type="bool", defVal=false },
  { name="is_dynamic_light", type="bool", defVal=false },
  { name="shadows_for_dynamic_objects", type="bool", defVal=false },
  { name="shadows_for_gpu_objects", type="bool", defVal=false },
  { name="sdf_shadows", type="bool", defVal=false },
  { name="quality", type="int", defVal=0 },
  { name="shrink", type="int", defVal=0 },
  { name="priority", type="int", defVal=0 },
]);

end_module();
