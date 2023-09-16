from "params.gen.nut" import *

begin_declare_params("StdFxShaderParams");



declare_struct("StdFxShaderParams", 6,
[
  { name="texture", type="ref_slot", slotType="Texture" },
  { name="shader", type="list", list=["AlphaBlend", "Additive", "AddSmooth"] },
  { name="shaderType", type="list", list=["SimpleShading", "TwoSidedShading", "TranslucencyShading"] },
  { name="sorted", type="bool" },
  { name="useColorMult", type="bool", defVal=false },
  { name="distortion", type="bool", defVal=false },
  { name="softnessDistance", type="real", defVal=0 },
  { name="waterProjection", type="bool", defVal=false },
  { name="waterProjectionOnly", type="bool", defVal=false }
]);



end_module();
