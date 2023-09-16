//require baseCoreEvent//not working!
struct BaseCoreEvent
    [skipCpp=true] vtbl__:uint?
    [skipCpp=true] eventName:string
    [skipCpp=true] eventType:uint
/*
struct SomeEvent
    pos,vel:float3

struct TestEvent
    i :int
    i2:int2
    i3:int3
    i4:int4
    ui:uint
    f :float
    f2:float2
    f3:float3
    f4:float4
    s:string
    m3x4:float3x4
    m4x4:float4x4
    ev:SomeEvent
    ai:int[2]
    aai:int[3][4]
*/


[comment (sValue="hello,world",iValue=5,fValue=2.0,bValue=true)]
struct Dummy {}

