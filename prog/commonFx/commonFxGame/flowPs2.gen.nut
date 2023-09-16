from "dagor.math" import Color3, Point2, Point3

from "params.gen.nut" import *

include_decl_h("StaticVisSphere");
include_decl_h("StdEmitter");
begin_declare_params("FlowPs2");



declare_struct("FlowPsColor2", 4,
[
  { name="color", type="E3DCOLOR" },
  { name="color2", type="E3DCOLOR" },
  { name="scale", type="real", defVal=1 },
  { name="fakeHdrBrightness", type="real", defVal=1 },
  { name="rFunc", type="cubic_curve", color=Color3(255,   0,   0) },
  { name="gFunc", type="cubic_curve", color=Color3(  0, 255,   0) },
  { name="bFunc", type="cubic_curve", color=Color3(  0,   0, 255) },
  { name="aFunc", type="cubic_curve", color=Color3(255, 255, 255) },
  { name="specFunc", type="cubic_curve", color=Color3(255, 255, 255) },
]);


declare_struct("FlowPsSize2", 1,
[
  { name="radius", type="real" },
  { name="lengthDt", type="real" },
  { name="sizeFunc", type="cubic_curve" },
]);


declare_struct("FlowPsParams2", 14,
[
  { name="framesX", type="int" },
  { name="framesY", type="int" },

  { name="color", type="FlowPsColor2" },

  { name="size", type="FlowPsSize2" },

  { name="life", type="real" },
  { name="randomLife", type="real" },
  { name="randomVel", type="real" },
  { name="normalVel", type="real" },
  { name="startVel", type="Point3" },
  { name="gravity", type="real" },
  { name="viscosity", type="real" },

  { name="randomRot", type="real" },

  { name="rotSpeed", type="real" },
  { name="rotViscosity", type="real" },

  { name="randomPhase", type="real" },
  { name="animSpeed", type="real", defVal=1.0 },

  { name="numParts", type="int" },
  { name="seed", type="int" },

  { name="texture", type="ref_slot", slotType="Texture" },
  { name="normal", type="ref_slot", slotType="Normal" },

  { name="shader", type="list", list=["AlphaBlend", "Additive", "AddSmooth", "AlphaTest", "PreMultAlpha", "GbufferAlphaTest", "AlphaTestRefraction", "GbufferPatch"] },
  { name="shaderType", type="list", list=["SimpleShading", "TwoSidedShading", "TranslucencyShading"] },

  { name="sorted", type="bool" },

  { name="useColorMult", type="bool", defVal=true },
  { name="colorMultMode", type="list", list=["for_system", "for_emitter"] },

  { name="burstMode", type="bool", defVal=false },

  { name="amountScale", type="real", defVal=1 },

  { name="turbulenceScale", type="real", defVal=1 },
  { name="turbulenceTimeScale", type="real", defVal=0.1 },
  { name="turbulenceMultiplier", type="real", defVal=0 },
  { name="windScale", type="real", defVal=0 },
  { name="ltPower", type="real", defVal=0 },
  { name="sunLtPower", type="real", defVal=0.5 },
  { name="ambLtPower", type="real", defVal=0.25 },
  { name="distortion", type="bool", defVal=false },
  { name="softnessDistance", type="real", defVal=0 },
  { name="waterProjection", type="bool", defVal=false },
  { name="waterProjectionOnly", type="bool", defVal=false },
  { name="airFriction", type="bool", defVal=false },
  { name="simulateInLocalSpace", type="bool", defVal=false },
  { name="animatedFlipbook", type="bool", defVal=false }
]);



end_declare_params("flow_ps_2", 2, [
  {struct="FlowPsParams2"},
  {struct="StaticVisSphereParams"},
  {struct="StdEmitterParams"},
]);
