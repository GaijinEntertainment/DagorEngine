require ecs

[es(on_event=EventStart)]
def ballistics_create_es(evt : Event)
  for i in range(0, 100)
    createEntity("ballistics")//everything from template
