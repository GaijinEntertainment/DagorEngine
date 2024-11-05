options no_aot
require ecs
require %danetlibs.verify_ecs.main.verify_events
require DagorConsole


[console_cmd(name="ecs.verify_systems_templates")]
def verify_ecs_console_command()
  broadcastEvent([[VerifyEcsSystems]])
  broadcastEvent([[VerifyEcsTemplates]])
