from "dagor.math" import Color3, Point2, Point3

from "params.gen.nut" import *

include_decl_h("StdFxShaderParams");
include_decl_h("StaticVisSphere");
include_decl_h("StdEmitter");
begin_declare_params("TrailFlowFx");


declare_extern_struct("StdFxShaderParams");



declare_struct("TrailFlowFxColor", 2,
[
  { name="color", type="E3DCOLOR" },
  { name="scale", type="real", defVal=1 },
  { name="fakeHdrBrightness", type="real", defVal=1 },
  { name="rFunc", type="cubic_curve", color=Color3(255,   0,   0) },
  { name="gFunc", type="cubic_curve", color=Color3(  0, 255,   0) },
  { name="bFunc", type="cubic_curve", color=Color3(  0,   0, 255) },
  { name="aFunc", type="cubic_curve", color=Color3(255, 255, 255) },
]);


declare_struct("TrailFlowFxSize", 1,
[
  { name="radius", type="real", defVal=0.1, },
  { name="sizeFunc", type="cubic_curve" },
]);


declare_struct("TrailFlowFxParams", 4,
[
  { name="numTrails", type="int", defVal=10 },
  { name="numSegs", type="int", defVal=50 },
  { name="timeLength", type="real", defVal=1.0, },
  { name="numFrames", type="int", defVal=1 },

  { name="tile", type="real", defVal=1 },
  { name="scrollSpeed", type="real", defVal=0 },

  { name="color", type="TrailFlowFxColor" },

  { name="size", type="TrailFlowFxSize" },

  { name="life", type="real", defVal=1.0 },
  { name="randomLife", type="real" },
  { name="emitterVelK", type="real", defVal=1.0 },
  { name="turbVel", type="real", defVal=1.0 },
  { name="turbFreq", type="real", defVal=10.0 },
  { name="normalVel", type="real" },
  { name="startVel", type="Point3" },
  { name="gravity", type="real" },
  { name="viscosity", type="real" },

  { name="seed", type="int" },

  { name="shader", type="StdFxShaderParams" },

  { name="burstMode", type="bool", defVal=false },

  { name="amountScale", type="real", defVal=1 },
  { name="ltPower", type="real", defVal=0 },
  { name="sunLtPower", type="real", defVal=0.5 },
  { name="ambLtPower", type="real", defVal=0.25 },
]);



end_declare_params("trail_flow", 2, [
  {struct="TrailFlowFxParams"},
  {struct="StaticVisSphereParams"},
  {struct="StdEmitterParams"},
]);
