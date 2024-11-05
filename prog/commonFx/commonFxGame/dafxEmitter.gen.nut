from "params.gen.nut" import *

begin_declare_params("DafxEmitter");

declare_struct("DafxEmitterDistanceBased", 1,
[
  { name="elem_limit", type="int", defVal=10 },
  { name="distance", type="real", defVal=1 },
  { name="idle_period", type="real", defVal=0 },
]
);

declare_struct("DafxEmitterParams", 2,
[
  { name="type", type="list", list=["fixed", "burst", "linear", "distance_based"] },
  { name="count", type="int" },
  { name="life", type="real" },
  { name="cycles", type="int" },
  { name="period", type="real" },
  { name="delay", type="real" },
  { name="distance_based", type="DafxEmitterDistanceBased" },
]);



end_module();
