from "dagor.math" import Color3, Point2, Point3

from "params.gen.nut" import *

include_decl_h("StaticVisSphere");
include_decl_h("StdEmitter");
include_decl_h("StdFxShaderParams");
begin_declare_params("AnimPlanesFx");


declare_extern_struct("StdFxShaderParams");


declare_struct("AnimPlaneCurveTime", 1,
[
  { name="timeScale", type="real", defVal=1 },
  { name="minTimeOffset", type="real" },
  { name="maxTimeOffset", type="real" },

  { name="outType", type="list", list=["repeat", "constant", "offset", "extrapolate"] },
]);


declare_struct("AnimPlaneScalar", 1,
[
  { name="scale", type="real", defVal=1 },
  { name="func", type="cubic_curve" },

  { name="time", type="AnimPlaneCurveTime" },
]);


declare_struct("AnimPlaneNormScalar", 1,
[
  { name="func", type="cubic_curve" },
  { name="time", type="AnimPlaneCurveTime" },
]);


declare_struct("AnimPlaneRgb", 1,
[
  { name="rFunc", type="cubic_curve", color=Color3(255,   0,   0) },
  { name="gFunc", type="cubic_curve", color=Color3(  0, 255,   0) },
  { name="bFunc", type="cubic_curve", color=Color3(  0,   0, 255) },

  { name="time", type="AnimPlaneCurveTime" },
]);


declare_struct("AnimPlaneColor", 2,
[
  { name="color", type="E3DCOLOR" },
  { name="fakeHdrBrightness", type="real", defVal=1 },

  { name="alpha", type="AnimPlaneNormScalar" },
  { name="brightness", type="AnimPlaneNormScalar" },

  { name="rgb", type="AnimPlaneRgb" },
]);


declare_struct("AnimPlaneTc", 1,
[
  { name="uOffset", type="real", defVal=0 },
  { name="vOffset", type="real", defVal=0 },

  { name="uSize", type="real", defVal=1024 },
  { name="vSize", type="real", defVal=1024 },
]);


declare_struct("AnimPlaneLife", 3,
[
  { name="life", type="real", defVal=1 },
  { name="lifeVar", type="real", defVal=0 },

  { name="delayBefore", type="real", defVal=0 },
  { name="delayBeforeVar", type="real", defVal=0 },

  { name="repeated", type="bool", defVal=true },
  { name="canRepeatWithoutEmitter", type="bool", defVal=true },

  { name="repeatDelay", type="real", defVal=0 },
  { name="repeatDelayVar", type="real", defVal=0 },
]);


declare_struct("AnimPlaneParams", 5,
[
  { name="life", type="AnimPlaneLife" },

  { name="color", type="AnimPlaneColor" },
  { name="size", type="AnimPlaneScalar" },
  { name="scaleX", type="AnimPlaneScalar" },
  { name="scaleY", type="AnimPlaneScalar" },
  { name="rotationX", type="AnimPlaneScalar" },
  { name="rotationY", type="AnimPlaneScalar" },
  { name="roll", type="AnimPlaneScalar" },
  { name="posX", type="AnimPlaneScalar" },
  { name="posY", type="AnimPlaneScalar" },
  { name="posZ", type="AnimPlaneScalar" },

  { name="tc", type="AnimPlaneTc" },

  { name="posFromEmitter", type="bool" },
  { name="dirFromEmitter", type="bool" },
  { name="dirFromEmitterRandom", type="bool", defVal=true },
  { name="moveWithEmitter", type="bool" },

  { name="rollOffset", type="real" },
  { name="rollOffsetVar", type="real" },
  { name="randomRollSign", type="bool" },

  { name="posOffset", type="Point3" },

  { name="facingType", type="list", list=["Facing", "AxisFacing", "Free"] },

  { name="viewZOffset", type="real", defVal=0 },
]);


declare_struct("AnimPlanesFxParams", 1,
[
  { name="seed", type="int" },
  { name="shader", type="StdFxShaderParams" },
  { name="ltPower", type="real", defVal=0 },
  { name="sunLtPower", type="real", defVal=0.5 },
  { name="ambLtPower", type="real", defVal=0.25 },

  { name="array", type="dyn_array", elemType="AnimPlaneParams" },
]);



end_declare_params("anim_planes", 2, [
  {struct="AnimPlanesFxParams"},
  {struct="StaticVisSphereParams"},
  {struct="StdEmitterParams"},
]);
