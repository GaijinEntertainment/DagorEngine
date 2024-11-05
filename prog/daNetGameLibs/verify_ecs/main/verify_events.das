module verify_events shared
require ecs

[event(broadcast)]
struct VerifyEcsSystems {}

[event(broadcast)]
struct VerifyEcsTemplates {}
