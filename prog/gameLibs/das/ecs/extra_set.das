module extra_set shared
require ecs

//wrappers for ecs::set(ecs::ComponentsInitializer, string, das::array<T>)
//             ecs::set(ecs::Object, string, das::array<T>)
//Codegen in end of file

def set(var init : ComponentsInitializer |#; name : string; source : array<bool>)
  using() <| $(var list : BoolList)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<int8>)
  using() <| $(var list : Int8List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<int16>)
  using() <| $(var list : Int16List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<int>)
  using() <| $(var list : IntList)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<int64>)
  using() <| $(var list : Int64List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<uint8>)
  using() <| $(var list : UInt8List)
    for value in source
      list |> push(value)
    set(init, name, list)


def set(var init : ComponentsInitializer |#; name : string; source : array<uint16>)
  using() <| $(var list : UInt16List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<uint>)
  using() <| $(var list : UInt32List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<uint64>)
  using() <| $(var list : UInt64List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<string>)
  using() <| $(var list : StringList)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<EntityId>)
  using() <| $(var list : EidList)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<float>)
  using() <| $(var list : FloatList)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<float2>)
  using() <| $(var list : Point2List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<float3>)
  using() <| $(var list : Point3List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<float4>)
  using() <| $(var list : Point4List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<int2>)
  using() <| $(var list : IPoint2List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<int3>)
  using() <| $(var list : IPoint3List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<int4>)
  using() <| $(var list : IPoint4List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : ComponentsInitializer |#; name : string; source : array<float3x4>)
  using() <| $(var list : TMatrixList)
    for value in source
      list |> push(value)
    set(init, name, list)


def set(var init : Object |#; name : string; source : array<bool>)
  using() <| $(var list : BoolList)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<int8>)
  using() <| $(var list : Int8List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<int16>)
  using() <| $(var list : Int16List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<int>)
  using() <| $(var list : IntList)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<int64>)
  using() <| $(var list : Int64List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<uint8>)
  using() <| $(var list : UInt8List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<uint16>)
  using() <| $(var list : UInt16List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<uint>)
  using() <| $(var list : UInt32List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<uint64>)
  using() <| $(var list : UInt64List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<string>)
  using() <| $(var list : StringList)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<EntityId>)
  using() <| $(var list : EidList)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<float>)
  using() <| $(var list : FloatList)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<float2>)
  using() <| $(var list : Point2List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<float3>)
  using() <| $(var list : Point3List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<float4>)
  using() <| $(var list : Point4List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<int2>)
  using() <| $(var list : IPoint2List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<int3>)
  using() <| $(var list : IPoint3List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<int4>)
  using() <| $(var list : IPoint4List)
    for value in source
      list |> push(value)
    set(init, name, list)

def set(var init : Object |#; name : string; source : array<float3x4>)
  using() <| $(var list : TMatrixList)
    for value in source
      list |> push(value)
    set(init, name, list)

/*
[init]
def gen()
  let types <- {{ "bool" => "BoolList"; "string" => "StringList"; "EntityId" => "EidList";
    "float" => "FloatList"; "float2" => "Point2List"; "float3" => "Point3List"; "float4" => "Point4List";
    "uint8" => "UInt8List"; "uint16" => "UInt16List"; "uint" => "UInt32List"; "uint64" => "UInt64List";
    "int8" => "Int8List"; "int16" => "Int16List"; "int" => "IntList"; "int64" => "Int64List";
    "int2" => "IPoint2List"; "int3" => "IPoint3List"; "int4" => "IPoint4List" }}


  for k, v in keys(types), values(types)
    print(
"def set(var init : ComponentsInitializer | #; name : string; source : array<{k}> | array<{k}>#)
  using() <| $(var list : {v}#)
    for value in source
      list |> push(value)
    set(init, name, list))\n")
  print("\n")
  for k, v in keys(types), values(types)
    print(
"def set(var init : Object | #; name : string; source : array<{k}> | array<{k}>#)
  using() <| $(var list : {v}#)
    for value in source
      list |> push(value)
    set(init, name, list))\n")
 */