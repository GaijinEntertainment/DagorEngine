from "params.gen.nut" import *

include_decl_h("StaticVisSphere");
include_decl_h("StdEmitter");
begin_declare_params("EmitterFlowPs");



declare_struct("EmitterFlowPsParams", 3,
[
  { name="fx1", type="ref_slot", slotType="Effects" },

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

  { name="numParts", type="int" },
  { name="seed", type="int" },

  { name="amountScale", type="real", defVal=1 },

  { name="turbulenceScale", type="real", defVal=1 },
  { name="turbulenceTimeScale", type="real", defVal=0.1 },
  { name="turbulenceMultiplier", type="real", defVal=0 },
  { name="windScale", type="real", defVal=0 },
  { name="burstMode", type="bool", defVal=false },
  { name="ltPower", type="real", defVal=0 },
  { name="sunLtPower", type="real", defVal=0.5 },
  { name="ambLtPower", type="real", defVal=0.25 },
]);



end_declare_params("emitterflow_ps", 2, [
  {struct="EmitterFlowPsParams"},
  {struct="StaticVisSphereParams"},
  {struct="StdEmitterParams"},
]);
