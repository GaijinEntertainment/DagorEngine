require baseCoreEvent

struct EventComponentChanged : BaseCoreEvent
    [cppType="const_string"] name:string
    flags:uint
    [cppType="component_t"] hash:uint
    [cppType="component_index_t"] index:uint8

struct EventAnyComponentChanged : BaseCoreEvent
    [cppType="const_string"] name:string
    flags:uint
    [cppType="component_t"] hash:uint
    [cppType="component_index_t"] index:uint8
