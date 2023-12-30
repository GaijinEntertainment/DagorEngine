module safe shared
require ecs
require DagorMath
require math
require daslib/constant_expression


def operator ?[] (t : IntList; idx : int) : int const? const#
  if uint(idx) < uint(length(t))
    unsafe
      return reinterpret<int const? const#> addr(t.[idx])
  unsafe
    return reinterpret<int const? const#> null


def operator ?[] (t : FloatList; idx : int) : float const? const#
  if uint(idx) < uint(length(t))
    unsafe
      return reinterpret<float const? const#> addr(t.[idx])
  unsafe
    return reinterpret<float const? const#> null


def operator ?[] (t : BoolList; idx : int) : bool const? const#
  if uint(idx) < uint(length(t))
    unsafe
      return reinterpret<bool const? const#> addr(t.[idx])
  unsafe
    return reinterpret<bool const? const#> null


def operator ?[] (t : EidList; idx : int) : EntityId const? const#
  if uint(idx) < uint(length(t))
    unsafe
      return reinterpret<EntityId const? const#> addr(t.[idx])
  unsafe
    return reinterpret<EntityId const? const#> null


def operator ?[] (t : Point2List; idx : int) : float2 const? const#
  if uint(idx) < uint(length(t))
    unsafe
      return reinterpret<float2 const? const#> addr(t.[idx])
  unsafe
    return reinterpret<float2 const? const#> null


def operator ?[] (t : Point3List; idx : int) : float3 const? const#
  if uint(idx) < uint(length(t))
    unsafe
      return reinterpret<float3 const? const#> addr(t.[idx])
  unsafe
    return reinterpret<float3 const? const#> null

def operator ?[] (t : Point4List; idx : int) : float4 const? const#
  if uint(idx) < uint(length(t))
    unsafe
      return reinterpret<float4 const? const#> addr(t.[idx])
  unsafe
    return reinterpret<float4 const? const#> null

def operator ?[] (t : Array const; idx : int) : ChildComponent const? const#
  if uint(idx) < uint(length(t))
    unsafe
      return reinterpret<ChildComponent const? const#> addr(t[idx])
  unsafe
    return reinterpret<ChildComponent const? const#> null


def operator ?[] (t : Array const? const; idx : int) : ChildComponent const? const#
  if t != null && uint(idx) < uint(length(*t))
    unsafe
      return reinterpret<ChildComponent const? const#> addr((*t)[idx])
  unsafe
    return reinterpret<ChildComponent const? const#> null


[generic, constant_expression(name=false), unused_argument(name)]
def operator .(t : Object const == const; name : string) : ChildComponent const? const
  static_if typeinfo(is_argument name)
    return t[name]
  else
    return get_child(t, name, ecs_hash(name))


[generic, constant_expression(name=false), unused_argument(name)]
def operator .(var t : Object == const; name : string) : ChildComponent?
  static_if typeinfo(is_argument name)
    return t[name]
  else
    return getRW_child(t, name, ecs_hash(name))


[generic, constant_expression(name=false), unused_argument(name)]
def operator ?.(t : Object const? const == const; name : string) : ChildComponent const? const
  if t != null
    static_if typeinfo(is_argument name)
      return (*t)[name]
    else
      return get_child(*t, name, ecs_hash(name))
  return [[ChildComponent const? const ]]


[generic, constant_expression(name=false), unused_argument(name)]
def operator ?.(var t : Object? == const; name : string) : ChildComponent?
  if t != null
    static_if typeinfo(is_argument name)
      return (*t)[name]
    else
      return getRW_child(*t, name, ecs_hash(name))
  return [[ChildComponent? ]]


[generic, constant_expression(name=false), unused_argument(name)]
def operator ?[] (t : Object const? const; name : string) : ChildComponent const? const
  if t != null
    static_if typeinfo(is_argument name)
      return (*t)[name]
    else
      return get_child(*t, name, ecs_hash(name))
  return [[ChildComponent const? const ]]


[generic, constant_expression(name=false), unused_argument(name)]
def operator ?[] (var t : Object?; name : string) : ChildComponent?
  if t != null
    static_if typeinfo(is_argument name)
      return (*t)[name]
    else
      return getRW_child(*t, name, ecs_hash(name))
  return [[ChildComponent? ]]


def operator ??(a : ChildComponent const? const; default_value)
  static_if default_value is type<string>
    let res = get_ecs_string(a)
    return res != null ? string(*res) : default_value
  static_elif default_value is type<int64>
    return get_int64(a) ?? default_value
  static_elif default_value is type<int2>
    return get_IPoint2(a) ?? default_value
  static_elif default_value is type<int3>
    return get_IPoint3(a) ?? default_value
  static_elif default_value is type<int4>
    return get_IPoint4(a) ?? default_value
  static_elif default_value is type<int>
    return get_int(a) ?? default_value
  static_elif default_value is type<uint8>
    return get_uint8(a) ?? default_value
  static_elif default_value is type<uint>
    return get_uint(a) ?? default_value
  static_elif default_value is type<uint16>
    return get_uint16(a) ?? default_value
  static_elif default_value is type<uint64>
    return get_uint64(a) ?? default_value
  static_elif default_value is type<math::float3x4>
    return get_TMatrix(a) ?? default_value
  static_elif default_value is type<DagorMath::E3DCOLOR>
    return get_E3DCOLOR(a) ?? default_value
  static_elif default_value is type<float>
    return get_float(a) ?? default_value
  static_elif default_value is type<float2>
    return get_Point2(a) ?? default_value
  static_elif default_value is type<float3>
    return get_Point3(a) ?? default_value
  static_elif default_value is type<float4>
    return get_Point4(a) ?? default_value
  static_elif default_value is type<ecs::EntityId>
    return get_Eid(a) ?? default_value
  static_elif default_value is type<bool>
    return get_bool(a) ?? default_value
  static_elif default_value is type<int8>
    return get_int8(a) ?? default_value
  static_elif default_value is type<int16>
    return get_int16(a) ?? default_value
  else
    concept_assert(false, "Unsupported type for ?? operator")

def operator ??(a : ChildComponent const? const#; default_value)
  static_if default_value is type<string>
    let res = get_ecs_string(a)
    return res != null ? string(*res) : default_value
  static_elif default_value is type<int64>
    return get_int64(a) ?? default_value
  static_elif default_value is type<int2>
    return get_IPoint2(a) ?? default_value
  static_elif default_value is type<int3>
    return get_IPoint3(a) ?? default_value
  static_elif default_value is type<int4>
    return get_IPoint4(a) ?? default_value
  static_elif default_value is type<int>
    return get_int(a) ?? default_value
  static_elif default_value is type<uint8>
    return get_uint8(a) ?? default_value
  static_elif default_value is type<uint>
    return get_uint(a) ?? default_value
  static_elif default_value is type<uint16>
    return get_uint16(a) ?? default_value
  static_elif default_value is type<uint64>
    return get_uint64(a) ?? default_value
  static_elif default_value is type<math::float3x4>
    return get_TMatrix(a) ?? default_value
  static_elif default_value is type<DagorMath::E3DCOLOR>
    return get_E3DCOLOR(a) ?? default_value
  static_elif default_value is type<float>
    return get_float(a) ?? default_value
  static_elif default_value is type<float2>
    return get_Point2(a) ?? default_value
  static_elif default_value is type<float3>
    return get_Point3(a) ?? default_value
  static_elif default_value is type<float4>
    return get_Point4(a) ?? default_value
  static_elif default_value is type<ecs::EntityId>
    return get_Eid(a) ?? default_value
  static_elif default_value is type<bool>
    return get_bool(a) ?? default_value
  static_elif default_value is type<int8>
    return get_int8(a) ?? default_value
  static_elif default_value is type<int16>
    return get_int16(a) ?? default_value
  else
    concept_assert(false, "Unsupported type for ?? operator")


def operator ??(a : ChildComponent const; default_value)
  static_if default_value is type<string>
    let res = get_ecs_string(a)
    return res != null ? string(*res) : default_value
  static_elif default_value is type<int64>
    return get_int64(a) ?? default_value
  static_elif default_value is type<int2>
    return get_IPoint2(a) ?? default_value
  static_elif default_value is type<int3>
    return get_IPoint3(a) ?? default_value
  static_elif default_value is type<int4>
    return get_IPoint4(a) ?? default_value
  static_elif default_value is type<int>
    return get_int(a) ?? default_value
  static_elif default_value is type<uint8>
    return get_uint8(a) ?? default_value
  static_elif default_value is type<uint>
    return get_uint(a) ?? default_value
  static_elif default_value is type<uint16>
    return get_uint16(a) ?? default_value
  static_elif default_value is type<uint64>
    return get_uint64(a) ?? default_value
  static_elif default_value is type<math::float3x4>
    return get_TMatrix(a) ?? default_value
  static_elif default_value is type<DagorMath::E3DCOLOR>
    return get_E3DCOLOR(a) ?? default_value
  static_elif default_value is type<float>
    return get_float(a) ?? default_value
  static_elif default_value is type<float2>
    return get_Point2(a) ?? default_value
  static_elif default_value is type<float3>
    return get_Point3(a) ?? default_value
  static_elif default_value is type<float4>
    return get_Point4(a) ?? default_value
  static_elif default_value is type<ecs::EntityId>
    return get_Eid(a) ?? default_value
  static_elif default_value is type<bool>
    return get_bool(a) ?? default_value
  static_elif default_value is type<int8>
    return get_int8(a) ?? default_value
  static_elif default_value is type<int16>
    return get_int16(a) ?? default_value
  else
    concept_assert(false, "Unsupported type for ?? operator")


def operator ??(a : ChildComponent const#; default_value)
  static_if default_value is type<string>
    let res = get_ecs_string(a)
    return res != null ? string(*res) : default_value
  static_elif default_value is type<int64>
    return get_int64(a) ?? default_value
  static_elif default_value is type<int2>
    return get_IPoint2(a) ?? default_value
  static_elif default_value is type<int3>
    return get_IPoint3(a) ?? default_value
  static_elif default_value is type<int4>
    return get_IPoint4(a) ?? default_value
  static_elif default_value is type<int>
    return get_int(a) ?? default_value
  static_elif default_value is type<uint8>
    return get_uint8(a) ?? default_value
  static_elif default_value is type<uint>
    return get_uint(a) ?? default_value
  static_elif default_value is type<uint16>
    return get_uint16(a) ?? default_value
  static_elif default_value is type<uint64>
    return get_uint64(a) ?? default_value
  static_elif default_value is type<math::float3x4>
    return get_TMatrix(a) ?? default_value
  static_elif default_value is type<DagorMath::E3DCOLOR>
    return get_E3DCOLOR(a) ?? default_value
  static_elif default_value is type<float>
    return get_float(a) ?? default_value
  static_elif default_value is type<float2>
    return get_Point2(a) ?? default_value
  static_elif default_value is type<float3>
    return get_Point3(a) ?? default_value
  static_elif default_value is type<float4>
    return get_Point4(a) ?? default_value
  static_elif default_value is type<ecs::EntityId>
    return get_Eid(a) ?? default_value
  static_elif default_value is type<bool>
    return get_bool(a) ?? default_value
  static_elif default_value is type<int8>
    return get_int8(a) ?? default_value
  static_elif default_value is type<int16>
    return get_int16(a) ?? default_value
  else
    concept_assert(false, "Unsupported type for ?? operator")


[generic, constant_expression(name), unused_argument(child, name)]
def operator ?as(child : ChildComponent const? const == const; name : string)
  static_if !typeinfo(is_argument name)
    static_if name == "Object"
      return get_ecs_object(child)
    static_elif name == "Array"
      return get_ecs_array(child)
    static_elif name == "IntList"
      return get_ecs_IntList(child)
    static_elif name == "FloatList"
      return get_ecs_FloatList(child)
    static_elif name == "int4"
      return get_IPoint4(child)
    static_elif name == "int2"
      return get_IPoint2(child)
    static_elif name == "int8"
      return get_int8(child)
    static_elif name == "int16"
      return get_int16(child)
    static_elif name == "int"
      return get_int(child)
    static_elif name == "uint8"
      return get_uint8(child)
    static_elif name == "uint"
      return get_uint(child)
    static_elif name == "uint16"
      return get_uint16(child)
    static_elif name == "uint64"
      return get_uint64(child)
    static_elif name == "string"
      return get_ecs_string(child)
    static_elif name == "math::float3x4"
      return get_TMatrix(child)
    static_elif name == "DagorMath::E3DCOLOR"
      return get_E3DCOLOR(child)
    static_elif name == "float"
      return get_float(child)
    static_elif name == "float2"
      return get_Point2(child)
    static_elif name == "float3"
      return get_Point3(child)
    static_elif name == "float4"
      return get_Point4(child)
    static_elif name == "ecs::EntityId"
      return get_Eid(child)
    static_elif name == "bool"
      return get_bool(child)
    static_elif name == "int3"
      return get_IPoint3(child)
    static_elif name == "int64"
      return get_int64(child)
    else
      concept_assert(false, "Unsupported type for ?as operator")


[generic, constant_expression(name), unused_argument(child, name)]
def operator ?as(child : ChildComponent const? const# == const; name : string)
  static_if !typeinfo(is_argument name)
    static_if name == "Object"
      return get_ecs_object(child)
    static_elif name == "Array"
      return get_ecs_array(child)
    static_elif name == "IntList"
      return get_ecs_IntList(child)
    static_elif name == "FloatList"
      return get_ecs_FloatList(child)
    static_elif name == "int4"
      return get_IPoint4(child)
    static_elif name == "int2"
      return get_IPoint2(child)
    static_elif name == "int8"
      return get_int8(child)
    static_elif name == "int16"
      return get_int16(child)
    static_elif name == "int"
      return get_int(child)
    static_elif name == "uint8"
      return get_uint8(child)
    static_elif name == "uint"
      return get_uint(child)
    static_elif name == "uint16"
      return get_uint16(child)
    static_elif name == "uint64"
      return get_uint64(child)
    static_elif name == "string"
      return get_ecs_string(child)
    static_elif name == "math::float3x4"
      return get_TMatrix(child)
    static_elif name == "DagorMath::E3DCOLOR"
      return get_E3DCOLOR(child)
    static_elif name == "float"
      return get_float(child)
    static_elif name == "float2"
      return get_Point2(child)
    static_elif name == "float3"
      return get_Point3(child)
    static_elif name == "float4"
      return get_Point4(child)
    static_elif name == "ecs::EntityId"
      return get_Eid(child)
    static_elif name == "bool"
      return get_bool(child)
    static_elif name == "int3"
      return get_IPoint3(child)
    static_elif name == "int64"
      return get_int64(child)
    else
      concept_assert(false, "Unsupported type for ?as operator")


[generic, constant_expression(name), unused_argument(child, name)]
def operator as(child : ChildComponent const == const; name : string)
  static_if !typeinfo(is_argument name)
    static_if name == "Object"
      return get_ecs_object(child)
    static_elif name == "Array"
      return get_ecs_array(child)
    static_elif name == "IntList"
      return get_ecs_IntList(child)
    static_elif name == "FloatList"
      return get_ecs_FloatList(child)
    static_elif name == "int4"
      return get_IPoint4(child)
    static_elif name == "int2"
      return get_IPoint2(child)
    static_elif name == "int8"
      return get_int8(child)
    static_elif name == "int16"
      return get_int16(child)
    static_elif name == "int"
      return get_int(child)
    static_elif name == "uint8"
      return get_uint8(child)
    static_elif name == "uint"
      return get_uint(child)
    static_elif name == "uint16"
      return get_uint16(child)
    static_elif name == "uint64"
      return get_uint64(child)
    static_elif name == "string"
      return get_ecs_string(child)
    static_elif name == "math::float3x4"
      return get_TMatrix(child)
    static_elif name == "DagorMath::E3DCOLOR"
      return get_E3DCOLOR(child)
    static_elif name == "float"
      return get_float(child)
    static_elif name == "float2"
      return get_Point2(child)
    static_elif name == "float3"
      return get_Point3(child)
    static_elif name == "float4"
      return get_Point4(child)
    static_elif name == "ecs::EntityId"
      return get_Eid(child)
    static_elif name == "bool"
      return get_bool(child)
    static_elif name == "int3"
      return get_IPoint3(child)
    static_elif name == "int64"
      return get_int64(child)
    else
      concept_assert(false, "Unsupported type for as operator")


[generic, constant_expression(name), unused_argument(child, name)]
def operator as(child : ChildComponent const# == const; name : string)
  static_if !typeinfo(is_argument name)
    static_if name == "Object"
      return get_ecs_object(child)
    static_elif name == "Array"
      return get_ecs_array(child)
    static_elif name == "IntList"
      return get_ecs_IntList(child)
    static_elif name == "FloatList"
      return get_ecs_FloatList(child)
    static_elif name == "int4"
      return get_IPoint4(child)
    static_elif name == "int2"
      return get_IPoint2(child)
    static_elif name == "int8"
      return get_int8(child)
    static_elif name == "int16"
      return get_int16(child)
    static_elif name == "int"
      return get_int(child)
    static_elif name == "uint8"
      return get_uint8(child)
    static_elif name == "uint"
      return get_uint(child)
    static_elif name == "uint16"
      return get_uint16(child)
    static_elif name == "uint64"
      return get_uint64(child)
    static_elif name == "string"
      return get_ecs_string(child)
    static_elif name == "math::float3x4"
      return get_TMatrix(child)
    static_elif name == "DagorMath::E3DCOLOR"
      return get_E3DCOLOR(child)
    static_elif name == "float"
      return get_float(child)
    static_elif name == "float2"
      return get_Point2(child)
    static_elif name == "float3"
      return get_Point3(child)
    static_elif name == "float4"
      return get_Point4(child)
    static_elif name == "ecs::EntityId"
      return get_Eid(child)
    static_elif name == "bool"
      return get_bool(child)
    static_elif name == "int3"
      return get_IPoint3(child)
    static_elif name == "int64"
      return get_int64(child)
    else
      concept_assert(false, "Unsupported type for as operator")


[generic, constant_expression(name), unused_argument(child, name)]
def operator ?as(var child : ChildComponent? == const; name : string)
  static_if !typeinfo(is_argument name)
    static_if name == "Object"
      return getRW_ecs_object(child)
    static_elif name == "Array"
      return getRW_ecs_array(child)
    static_elif name == "IntList"
      return getRW_ecs_IntList(child)
    static_elif name == "FloatList"
      return getRW_ecs_FloatList(child)
    static_elif name == "int4"
      return getRW_IPoint4(child)
    static_elif name == "int2"
      return getRW_IPoint2(child)
    static_elif name == "int8"
      return getRW_int8(child)
    static_elif name == "int16"
      return getRW_int16(child)
    static_elif name == "int"
      return getRW_int(child)
    static_elif name == "uint8"
      return getRW_uint8(child)
    static_elif name == "uint"
      return getRW_uint(child)
    static_elif name == "uint16"
      return getRW_uint16(child)
    static_elif name == "uint64"
      return getRW_uint64(child)
    static_elif name == "string"
      return getRW_ecs_string(child)
    static_elif name == "math::float3x4"
      return getRW_TMatrix(child)
    static_elif name == "DagorMath::E3DCOLOR"
      return getRW_E3DCOLOR(child)
    static_elif name == "float"
      return getRW_float(child)
    static_elif name == "float2"
      return getRW_Point2(child)
    static_elif name == "float3"
      return getRW_Point3(child)
    static_elif name == "float4"
      return getRW_Point4(child)
    static_elif name == "ecs::EntityId"
      return getRW_Eid(child)
    static_elif name == "bool"
      return getRW_bool(child)
    static_elif name == "int3"
      return getRW_IPoint3(child)
    static_elif name == "int64"
      return getRW_int64(child)
    else
      concept_assert(false, "Unsupported type for ?as operator")


[generic, constant_expression(name), unused_argument(child, name)]
def operator ?as(var child : ChildComponent?# == const; name : string)
  static_if !typeinfo(is_argument name)
    static_if name == "Object"
      return getRW_ecs_object(child)
    static_elif name == "Array"
      return getRW_ecs_array(child)
    static_elif name == "IntList"
      return getRW_ecs_IntList(child)
    static_elif name == "FloatList"
      return getRW_ecs_FloatList(child)
    static_elif name == "int4"
      return getRW_IPoint4(child)
    static_elif name == "int2"
      return getRW_IPoint2(child)
    static_elif name == "int8"
      return getRW_int8(child)
    static_elif name == "int16"
      return getRW_int16(child)
    static_elif name == "int"
      return getRW_int(child)
    static_elif name == "uint8"
      return getRW_uint8(child)
    static_elif name == "uint"
      return getRW_uint(child)
    static_elif name == "uint16"
      return getRW_uint16(child)
    static_elif name == "uint64"
      return getRW_uint64(child)
    static_elif name == "string"
      return getRW_ecs_string(child)
    static_elif name == "math::float3x4"
      return getRW_TMatrix(child)
    static_elif name == "DagorMath::E3DCOLOR"
      return getRW_E3DCOLOR(child)
    static_elif name == "float"
      return getRW_float(child)
    static_elif name == "float2"
      return getRW_Point2(child)
    static_elif name == "float3"
      return getRW_Point3(child)
    static_elif name == "float4"
      return getRW_Point4(child)
    static_elif name == "ecs::EntityId"
      return getRW_Eid(child)
    static_elif name == "bool"
      return getRW_bool(child)
    static_elif name == "int3"
      return getRW_IPoint3(child)
    static_elif name == "int64"
      return getRW_int64(child)
    else
      concept_assert(false, "Unsupported type for ?as operator")


[generic, constant_expression(name), unused_argument(child, name)]
def operator as(var child : ChildComponent == const; name : string)
  static_if !typeinfo(is_argument name)
    static_if name == "Object"
      return getRW_ecs_object(child)
    static_elif name == "Array"
      return getRW_ecs_array(child)
    static_elif name == "IntList"
      return getRW_ecs_IntList(child)
    static_elif name == "FloatList"
      return getRW_ecs_FloatList(child)
    static_elif name == "int4"
      return getRW_IPoint4(child)
    static_elif name == "int2"
      return getRW_IPoint2(child)
    static_elif name == "int8"
      return getRW_int8(child)
    static_elif name == "int16"
      return getRW_int16(child)
    static_elif name == "int"
      return getRW_int(child)
    static_elif name == "uint8"
      return getRW_uint8(child)
    static_elif name == "uint"
      return getRW_uint(child)
    static_elif name == "uint16"
      return getRW_uint16(child)
    static_elif name == "uint64"
      return getRW_uint64(child)
    static_elif name == "string"
      return getRW_ecs_string(child)
    static_elif name == "math::float3x4"
      return getRW_TMatrix(child)
    static_elif name == "DagorMath::E3DCOLOR"
      return getRW_E3DCOLOR(child)
    static_elif name == "float"
      return getRW_float(child)
    static_elif name == "float2"
      return getRW_Point2(child)
    static_elif name == "float3"
      return getRW_Point3(child)
    static_elif name == "float4"
      return getRW_Point4(child)
    static_elif name == "ecs::EntityId"
      return getRW_Eid(child)
    static_elif name == "bool"
      return getRW_bool(child)
    static_elif name == "int3"
      return getRW_IPoint3(child)
    static_elif name == "int64"
      return getRW_int64(child)
    else
      concept_assert(false, "Unsupported type for as operator")


[generic, constant_expression(name), unused_argument(child, name)]
def operator as(var child : ChildComponent# == const; name : string)
  static_if !typeinfo(is_argument name)
    static_if name == "Object"
      return getRW_ecs_object(child)
    static_elif name == "Array"
      return getRW_ecs_array(child)
    static_elif name == "IntList"
      return getRW_ecs_IntList(child)
    static_elif name == "FloatList"
      return getRW_ecs_FloatList(child)
    static_elif name == "int4"
      return getRW_IPoint4(child)
    static_elif name == "int2"
      return getRW_IPoint2(child)
    static_elif name == "int8"
      return getRW_int8(child)
    static_elif name == "int16"
      return getRW_int16(child)
    static_elif name == "int"
      return getRW_int(child)
    static_elif name == "uint8"
      return getRW_uint8(child)
    static_elif name == "uint"
      return getRW_uint(child)
    static_elif name == "uint16"
      return getRW_uint16(child)
    static_elif name == "uint64"
      return getRW_uint64(child)
    static_elif name == "string"
      return getRW_ecs_string(child)
    static_elif name == "math::float3x4"
      return getRW_TMatrix(child)
    static_elif name == "DagorMath::E3DCOLOR"
      return getRW_E3DCOLOR(child)
    static_elif name == "float"
      return getRW_float(child)
    static_elif name == "float2"
      return getRW_Point2(child)
    static_elif name == "float3"
      return getRW_Point3(child)
    static_elif name == "float4"
      return getRW_Point4(child)
    static_elif name == "ecs::EntityId"
      return getRW_Eid(child)
    static_elif name == "bool"
      return getRW_bool(child)
    static_elif name == "int3"
      return getRW_IPoint3(child)
    static_elif name == "int64"
      return getRW_int64(child)
    else
      concept_assert(false, "Unsupported type for as operator")

/*
[init]
def gen()
  let typesWithoutString <- {{ "bool" => "bool";
    "float" => "float";
    "float2" => "Point2"; "float3" => "Point3"; "float4" => "Point4";
    "uint8" => "uint8"; "uint16" => "uint16"; "uint" => "uint"; "uint64" => "uint64";
    "int8" => "int8"; "int16" => "int16"; "int" => "int"; "int64" => "int64";
    "int2" => "IPoint2"; "int3" => "IPoint3"; "int4" => "IPoint4";
    "math::float3x4" => "TMatrix"; "ecs::EntityId" => "Eid";
    "DagorMath::E3DCOLOR" => "E3DCOLOR" }}

  var allTypes := typesWithoutString
  allTypes["string"] = "ecs_string"


  print("def operator ??(a : ChildComponent const? const; default_value)
  static_if default_value is type<string>
    let res = get_ecs_string(a)
    return res != null ? string(*res) : default_value")
  for dasType, cType in keys(typesWithoutString), values(typesWithoutString)
    print("  static_elif default_value is type<{dasType}>
    return get_{cType}(a) ?? default_value")
  print("  else
    concept_assert(false, \"Unsupported type for ?? operator\")")

  print("\n")
  print("def operator ??(a : ChildComponent const? const#; default_value)
  static_if default_value is type<string>
    let res = get_ecs_string(a)
    return res != null ? string(*res) : default_value")
  for dasType, cType in keys(typesWithoutString), values(typesWithoutString)
    print("  static_elif default_value is type<{dasType}>
    return get_{cType}(a) ?? default_value")
  print("  else
    concept_assert(false, \"Unsupported type for ?? operator\")")

  print("\n")
  print("def operator ??(a : ChildComponent const; default_value)
  static_if default_value is type<string>
    let res = get_ecs_string(a)
    return res != null ? string(*res) : default_value")
  for dasType, cType in keys(typesWithoutString), values(typesWithoutString)
    print("  static_elif default_value is type<{dasType}>
    return get_{cType}(a) ?? default_value")
  print("  else
    concept_assert(false, \"Unsupported type for ?? operator\")")

  print("\n")
  print("def operator ??(a : ChildComponent const#; default_value)
  static_if default_value is type<string>
    let res = get_ecs_string(a)
    return res != null ? string(*res) : default_value")
  for dasType, cType in keys(typesWithoutString), values(typesWithoutString)
    print("  static_elif default_value is type<{dasType}>
    return get_{cType}(a) ?? default_value")
  print("  else
    concept_assert(false, \"Unsupported type for ?? operator\")")

  print("\n")
  print("[generic, constant_expression(name), unused_argument(child, name)]
def operator ?as(child : ChildComponent const? const ==const; name : string)
  static_if !typeinfo(is_argument name)
    static_if name == \"Object\"
      return get_ecs_object(child)
    static_elif name == \"Array\"
      return get_ecs_array(child)
    static_elif name == \"IntList\"
      return get_ecs_IntList(child)
    static_elif name == \"FloatList\"
      return get_ecs_FloatList(child)")

  for dasType, cType in keys(allTypes), values(allTypes)
    print("    static_elif name == \"{dasType}\"
      return get_{cType}(child)")

  print("    else
      concept_assert(false, \"Unsupported type for ?as operator\")")

  print("\n")
  print("[generic, constant_expression(name), unused_argument(child, name)]
def operator ?as(child : ChildComponent const? const# ==const; name : string)
  static_if !typeinfo(is_argument name)
    static_if name == \"Object\"
      return get_ecs_object(child)
    static_elif name == \"Array\"
      return get_ecs_array(child)
    static_elif name == \"IntList\"
      return get_ecs_IntList(child)
    static_elif name == \"FloatList\"
      return get_ecs_FloatList(child)")

  for dasType, cType in keys(allTypes), values(allTypes)
    print("    static_elif name == \"{dasType}\"
      return get_{cType}(child)")

  print("    else
      concept_assert(false, \"Unsupported type for ?as operator\")")

  print("\n")
  print("[generic, constant_expression(name), unused_argument(child, name)]
def operator as(child : ChildComponent const ==const; name : string)
  static_if !typeinfo(is_argument name)
    static_if name == \"Object\"
      return get_ecs_object(child)
    static_elif name == \"Array\"
      return get_ecs_array(child)
    static_elif name == \"IntList\"
      return get_ecs_IntList(child)
    static_elif name == \"FloatList\"
      return get_ecs_FloatList(child)")

  for dasType, cType in keys(allTypes), values(allTypes)
    print("    static_elif name == \"{dasType}\"
      return get_{cType}(child)")

  print("    else
      concept_assert(false, \"Unsupported type for as operator\")")

  print("\n")
  print("[generic, constant_expression(name), unused_argument(child, name)]
def operator as(child : ChildComponent const# ==const; name : string)
  static_if !typeinfo(is_argument name)
    static_if name == \"Object\"
      return get_ecs_object(child)
    static_elif name == \"Array\"
      return get_ecs_array(child)
    static_elif name == \"IntList\"
      return get_ecs_IntList(child)
    static_elif name == \"FloatList\"
      return get_ecs_FloatList(child)")

  for dasType, cType in keys(allTypes), values(allTypes)
    print("    static_elif name == \"{dasType}\"
      return get_{cType}(child)")

  print("    else
      concept_assert(false, \"Unsupported type for as operator\")")

  print("\n")
  print("[generic, constant_expression(name), unused_argument(child, name)]
def operator ?as(var child : ChildComponent? ==const; name : string)
  static_if !typeinfo(is_argument name)
    static_if name == \"Object\"
      return getRW_ecs_object(child)
    static_elif name == \"Array\"
      return getRW_ecs_array(child)
    static_elif name == \"IntList\"
      return getRW_ecs_IntList(child)
    static_elif name == \"FloatList\"
      return getRW_ecs_FloatList(child)")

  for dasType, cType in keys(allTypes), values(allTypes)
    print("    static_elif name == \"{dasType}\"
      return getRW_{cType}(child)")

  print("    else
      concept_assert(false, \"Unsupported type for ?as operator\")")

  print("\n")
  print("[generic, constant_expression(name), unused_argument(child, name)]
def operator ?as(var child : ChildComponent?# ==const; name : string)
  static_if !typeinfo(is_argument name)
    static_if name == \"Object\"
      return getRW_ecs_object(child)
    static_elif name == \"Array\"
      return getRW_ecs_array(child)
    static_elif name == \"IntList\"
      return getRW_ecs_IntList(child)
    static_elif name == \"FloatList\"
      return getRW_ecs_FloatList(child)")

  for dasType, cType in keys(allTypes), values(allTypes)
    print("    static_elif name == \"{dasType}\"
      return getRW_{cType}(child)")

  print("    else
      concept_assert(false, \"Unsupported type for ?as operator\")")

  print("\n")
  print("[generic, constant_expression(name), unused_argument(child, name)]
def operator as(var child : ChildComponent ==const; name : string)
  static_if !typeinfo(is_argument name)
    static_if name == \"Object\"
      return getRW_ecs_object(child)
    static_elif name == \"Array\"
      return getRW_ecs_array(child)
    static_elif name == \"IntList\"
      return getRW_ecs_IntList(child)
    static_elif name == \"FloatList\"
      return getRW_ecs_FloatList(child)")

  for dasType, cType in keys(allTypes), values(allTypes)
    print("    static_elif name == \"{dasType}\"
      return getRW_{cType}(child)")

  print("    else
      concept_assert(false, \"Unsupported type for as operator\")")

  print("\n")
  print("[generic, constant_expression(name), unused_argument(child, name)]
def operator as(var child : ChildComponent# ==const; name : string)
  static_if !typeinfo(is_argument name)
    static_if name == \"Object\"
      return getRW_ecs_object(child)
    static_elif name == \"Array\"
      return getRW_ecs_array(child)
    static_elif name == \"IntList\"
      return getRW_ecs_IntList(child)
    static_elif name == \"FloatList\"
      return getRW_ecs_FloatList(child)")

  for dasType, cType in keys(allTypes), values(allTypes)
    print("    static_elif name == \"{dasType}\"
      return getRW_{cType}(child)")

  print("    else
      concept_assert(false, \"Unsupported type for as operator\")")
*/