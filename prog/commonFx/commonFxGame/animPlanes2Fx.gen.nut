from "dagor.math" import Color3, Point2, Point3

from "params.gen.nut" import *

include_decl_h("StaticVisSphere");
include_decl_h("StdEmitter");
include_decl_h("StdFxShaderParams");
begin_declare_params("AnimPlanes2Fx");


declare_extern_struct("StdFxShaderParams");


declare_struct("AnimPlane2CurveTime", 1,
[
  { name="timeScale", type="real", defVal=1 },
  { name="minTimeOffset", type="real" },
  { name="maxTimeOffset", type="real" },

  { name="outType", type="list", list=["repeat", "constant", "offset", "extrapolate"] },
]);


declare_struct("AnimPlane2Scalar", 1,
[
  { name="scale", type="real", defVal=1 },
  { name="func", type="cubic_curve" },

  { name="time", type="AnimPlane2CurveTime" },
]);


declare_struct("AnimPlane2NormScalar", 1,
[
  { name="func", type="cubic_curve" },
  { name="time", type="AnimPlane2CurveTime" },
]);


declare_struct("AnimPlane2Rgb", 1,
[
  { name="rFunc", type="cubic_curve", color=Color3(255,   0,   0) },
  { name="gFunc", type="cubic_curve", color=Color3(  0, 255,   0) },
  { name="bFunc", type="cubic_curve", color=Color3(  0,   0, 255) },

  { name="time", type="AnimPlane2CurveTime" },
]);


declare_struct("AnimPlane2Color", 2,
[
  { name="color", type="E3DCOLOR" },
  { name="fakeHdrBrightness", type="real", defVal=1 },

  { name="alpha", type="AnimPlane2NormScalar" },
  { name="brightness", type="AnimPlane2NormScalar" },

  { name="rgb", type="AnimPlane2Rgb" },
]);


declare_struct("AnimPlane2Tc", 1,
[
  { name="uOffset", type="real", defVal=0 },
  { name="vOffset", type="real", defVal=0 },

  { name="uSize", type="real", defVal=1024 },
  { name="vSize", type="real", defVal=1024 },
]);


declare_struct("AnimPlane2Life", 2,
[
  { name="life", type="real", defVal=1 },
  { name="lifeVar", type="real", defVal=0 },

  { name="delayBefore", type="real", defVal=0 },
  { name="delayBeforeVar", type="real", defVal=0 },

  { name="repeated", type="bool", defVal=true },

  { name="repeatDelay", type="real", defVal=0 },
  { name="repeatDelayVar", type="real", defVal=0 },
]);


declare_struct("AnimPlane2Params", 4,
[
  { name="life", type="AnimPlane2Life" },

  { name="color", type="AnimPlane2Color" },
  { name="size", type="AnimPlane2Scalar" },
  { name="scaleX", type="AnimPlane2Scalar" },
  { name="scaleY", type="AnimPlane2Scalar" },
  { name="rotationX", type="AnimPlane2Scalar" },
  { name="rotationY", type="AnimPlane2Scalar" },
  { name="roll", type="AnimPlane2Scalar" },
  { name="posX", type="AnimPlane2Scalar" },
  { name="posY", type="AnimPlane2Scalar" },
  { name="posZ", type="AnimPlane2Scalar" },

  { name="tc", type="AnimPlane2Tc" },

  { name="posFromEmitter", type="bool" },
  { name="dirFromEmitter", type="bool" },
  { name="moveWithEmitter", type="bool" },

  { name="rollOffset", type="real" },
  { name="rollOffsetVar", type="real" },
  { name="randomRollSign", type="bool" },

  { name="posOffset", type="Point3" },

  { name="facingType", type="list", list=["Facing", "AxisFacing", "Free"] },

  { name="viewZOffset", type="real", defVal=0 },

  { name="framesX", type="int" },
  { name="framesY", type="int" },
  { name="animSpeed", type="real", defVal=1.0 },
]);


declare_struct("AnimPlanes2FxParams", 1,
[
  { name="seed", type="int" },
  { name="shader", type="StdFxShaderParams" },
  { name="ltPower", type="real", defVal=0 },
  { name="sunLtPower", type="real", defVal=0.5 },
  { name="ambLtPower", type="real", defVal=0.25 },

  { name="array", type="dyn_array", elemType="AnimPlane2Params" },
]);



end_declare_params("anim_planes_2", 2, [
  {struct="AnimPlanes2FxParams"},
  {struct="StaticVisSphereParams"},
  {struct="StdEmitterParams"},
]);
