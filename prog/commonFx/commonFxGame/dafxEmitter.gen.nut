from "params.gen.nut" import *

begin_declare_params("DafxEmitter");



declare_struct("DafxEmitterParams", 1,
[
  { name="type", type="list", list=["fixed", "burst", "linear"] },
  { name="count", type="int" },
  { name="life", type="real" },
  { name="cycles", type="int" },
  { name="period", type="real" },
  { name="delay", type="real" },
]);



end_module();
