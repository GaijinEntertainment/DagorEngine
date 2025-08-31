options no_aot
require ecs
require ecs.verify_common
require DagorConsole


[console_cmd(name="ecs.verify_systems_templates")]
def verify_ecs_console_command()
  broadcastEvent(VerifyEcsSystems())
  broadcastEvent(VerifyEcsTemplates())
