require AnimV20
require GeomNodeTree
require DagorDataBlock
options indenting = 2

let TIME_TicksPerSec = 15360.0


// Base class for daScript-defined post blend controllers. A controller is a class that inherits
// this one and is annotated with [anim_post_blend_controller(name="<blockName>")]; the C++ adapter
// (DasAnimPostBlendCtrl) forwards the AnimPostBlendCtrl virtual calls into the methods below.
// Note: only value/pointer fields are allowed (no strings/arrays) - the instance is a flat blob.
class AnimPostBlendCtrlAdapter
  @hidden thisCtrl : AnimPostBlendCtrl?      // this controller (C++ side)
  @hidden srcBlk : DataBlock?                // this controller's blk used in createNode
  // called once at load: allocate param ids and read configuration from `blk`
  def abstract createNode(var graph : AnimationGraph; var blk : DataBlock) : void
  // called once per animchar instance: resolve node indices etc. against `tree`
  def abstract init(var st : IPureAnimStateHolder; tree : GeomNodeTree) : void
  // called every frame: act on the skeleton (`tree`) using state params (`st`) at weight `wt`
  def abstract process(var st : IPureAnimStateHolder; wt : float; var tree : GeomNodeTree;
                       var ctx : AnimPostBlendCtrlContext) : void
  // optional per-frame advance independent of blending weight
  def abstract advance(var st : IPureAnimStateHolder; dt : float) : void
  // reset per-instance state params to their defaults
  def abstract setDefaultState(var st : IPureAnimStateHolder) : void


[generic]
def anim_graph_enqueueState(var anim_graph : AnimationGraph;
                            var state : IAnimStateHolder;
                            state_idx : int;
                            force_dur : float = -1.f;
                            force_speed : float = -1.f)
  anim_graph_enqueueState(anim_graph, state, state_idx, force_dur, force_speed)
