from "dagor.math" import Color3, Point2, Point3

from "params.gen.nut" import *

include_decl_h("StaticVisSphere");
include_decl_h("StdEmitter");
begin_declare_params("CompoundPs");



declare_struct("CompoundPsParams", 8,
[
  { name="fx1", type="ref_slot", slotType="Effects" },
  { name="fx1_offset", type="Point3", defVal=Point3(0,0,0) },
  { name="fx1_scale",  type="Point3", defVal=Point3(1,1,1) },
  { name="fx1_rot",    type="Point3", defVal=Point3(0,0,0) },
  { name="fx1_rot_speed", type="Point3", defVal=Point3(0,0,0) },
  { name="fx1_delay", type="real", defVal=0 },
  { name="fx1_emitter_lifetime", type="real", defVal=-1 },
  { name="fx2", type="ref_slot", slotType="Effects" },
  { name="fx2_offset", type="Point3", defVal=Point3(0,0,0) },
  { name="fx2_scale",  type="Point3", defVal=Point3(1,1,1) },
  { name="fx2_rot",    type="Point3", defVal=Point3(0,0,0) },
  { name="fx2_rot_speed", type="Point3", defVal=Point3(0,0,0) },
  { name="fx2_delay", type="real", defVal=0 },
  { name="fx2_emitter_lifetime", type="real", defVal=-1 },
  { name="fx3", type="ref_slot", slotType="Effects" },
  { name="fx3_offset", type="Point3", defVal=Point3(0,0,0) },
  { name="fx3_scale",  type="Point3", defVal=Point3(1,1,1) },
  { name="fx3_rot",    type="Point3", defVal=Point3(0,0,0) },
  { name="fx3_rot_speed", type="Point3", defVal=Point3(0,0,0) },
  { name="fx3_delay", type="real", defVal=0 },
  { name="fx3_emitter_lifetime", type="real", defVal=-1 },
  { name="fx4", type="ref_slot", slotType="Effects" },
  { name="fx4_offset", type="Point3", defVal=Point3(0,0,0) },
  { name="fx4_scale",  type="Point3", defVal=Point3(1,1,1) },
  { name="fx4_rot",    type="Point3", defVal=Point3(0,0,0) },
  { name="fx4_rot_speed", type="Point3", defVal=Point3(0,0,0) },
  { name="fx4_delay", type="real", defVal=0 },
  { name="fx4_emitter_lifetime", type="real", defVal=-1 },
  { name="fx5", type="ref_slot", slotType="Effects" },
  { name="fx5_offset", type="Point3", defVal=Point3(0,0,0) },
  { name="fx5_scale",  type="Point3", defVal=Point3(1,1,1) },
  { name="fx5_rot",    type="Point3", defVal=Point3(0,0,0) },
  { name="fx5_rot_speed", type="Point3", defVal=Point3(0,0,0) },
  { name="fx5_delay", type="real", defVal=0 },
  { name="fx5_emitter_lifetime", type="real", defVal=-1 },
  { name="fx6", type="ref_slot", slotType="Effects" },
  { name="fx6_offset", type="Point3", defVal=Point3(0,0,0) },
  { name="fx6_scale",  type="Point3", defVal=Point3(1,1,1) },
  { name="fx6_rot",    type="Point3", defVal=Point3(0,0,0) },
  { name="fx6_rot_speed", type="Point3", defVal=Point3(0,0,0) },
  { name="fx6_delay", type="real", defVal=0 },
  { name="fx6_emitter_lifetime", type="real", defVal=-1 },
  { name="fx7", type="ref_slot", slotType="Effects" },
  { name="fx7_offset", type="Point3", defVal=Point3(0,0,0) },
  { name="fx7_scale",  type="Point3", defVal=Point3(1,1,1) },
  { name="fx7_rot",    type="Point3", defVal=Point3(0,0,0) },
  { name="fx7_rot_speed", type="Point3", defVal=Point3(0,0,0) },
  { name="fx7_delay", type="real", defVal=0 },
  { name="fx7_emitter_lifetime", type="real", defVal=-1 },
  { name="fx8", type="ref_slot", slotType="Effects" },
  { name="fx8_offset", type="Point3", defVal=Point3(0,0,0) },
  { name="fx8_scale",  type="Point3", defVal=Point3(1,1,1) },
  { name="fx8_rot",    type="Point3", defVal=Point3(0,0,0) },
  { name="fx8_rot_speed", type="Point3", defVal=Point3(0,0,0) },
  { name="fx8_delay", type="real", defVal=0 },
  { name="fx8_emitter_lifetime", type="real", defVal=-1 },

  { name="useCommonEmitter", type="bool", defVal=true },
  { name="ltPower", type="real", defVal=0 },
  { name="sunLtPower", type="real", defVal=0.5 },
  { name="ambLtPower", type="real", defVal=0.25 },
]);



end_declare_params("compound_ps", 2, [
  {struct="CompoundPsParams"},
  {struct="StaticVisSphereParams"},
  {struct="StdEmitterParams"},
]);
