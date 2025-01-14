module common_sounds_events shared
require ecs

[cpp_event(unicast)]
struct CmdSoundIrq
  irq : string
  irqType : uint

[cpp_event(unicast)]
struct CmdSoundVoicefxIrq
  irq : string
  irqType : uint

[cpp_event(unicast)]
struct CmdSoundStepIrq
  objIdx : int
