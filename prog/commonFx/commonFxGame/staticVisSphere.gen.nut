from "params.gen.nut" import *

begin_declare_params("StaticVisSphere");



declare_struct("StaticVisSphereParams", 1,
[
  { name="center", type="Point3" },
  { name="radius", type="real", defVal=10 },
]);



end_module();
