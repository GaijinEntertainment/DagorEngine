from "params.gen.nut" import *

begin_declare_params("StdEmitter");



declare_struct("StdEmitterParams", 2,
[
  { name="radius", type="real" },
  { name="height", type="real" },
  { name="width", type="real" },
  { name="depth", type="real" },

  { name="isVolumetric", type="bool" },
  { name="isTrail", type="bool" },
  { name="geometry", type="list", list=["sphere", "cylinder", "box"] },
]);



end_module();
