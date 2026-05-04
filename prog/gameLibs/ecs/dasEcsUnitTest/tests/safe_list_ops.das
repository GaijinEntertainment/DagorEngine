options indenting = 2
options no_global_variables = false
require ecs
require %daslibEcs/safe
require DasEcsUnitTest


[es(on_event=EventStart)]
def create_safe_list_test_entity(evt : Event)
  createEntitySync("testSafeListOps")


// Test ?[] (safe indexing) on all list types
[es(on_appear)]
def test_safe_list_indexing(evt : Event;
                            test_int_list : IntList;
                            test_ipoint2_list : IPoint2List;
                            test_ipoint3_list : IPoint3List;
                            test_ipoint4_list : IPoint4List;
                            test_float_list : FloatList;
                            test_point2_list : Point2List;
                            test_point3_list : Point3List;
                            test_point4_list : Point4List;
                            test_string_list : StringList;
                            test_eid_list : EidList;
                            test_bool_list : BoolList)
  // IntList: [10, 20]
  verify(test_int_list?[0] != null)
  verify(*test_int_list?[0] == 10)
  verify(*test_int_list?[1] == 20)
  verify(test_int_list?[2] == null)
  verify(test_int_list?[-1] == null)

  // IPoint2List: [int2(1,2)]
  verify(test_ipoint2_list?[0] != null)
  verify(*test_ipoint2_list?[0] == int2(1, 2))
  verify(test_ipoint2_list?[1] == null)
  verify(test_ipoint2_list?[-1] == null)

  // IPoint3List: [int3(1,2,3)]
  verify(test_ipoint3_list?[0] != null)
  verify(*test_ipoint3_list?[0] == int3(1, 2, 3))
  verify(test_ipoint3_list?[1] == null)
  verify(test_ipoint3_list?[-1] == null)

  // IPoint4List: [int4(1,2,3,4)]
  verify(test_ipoint4_list?[0] != null)
  verify(*test_ipoint4_list?[0] == int4(1, 2, 3, 4))
  verify(test_ipoint4_list?[1] == null)
  verify(test_ipoint4_list?[-1] == null)

  // FloatList: [1.5, 2.5]
  verify(test_float_list?[0] != null)
  verify(*test_float_list?[0] == 1.5)
  verify(*test_float_list?[1] == 2.5)
  verify(test_float_list?[2] == null)
  verify(test_float_list?[-1] == null)

  // Point2List: [float2(1,2)]
  verify(test_point2_list?[0] != null)
  verify(*test_point2_list?[0] == float2(1.0, 2.0))
  verify(test_point2_list?[1] == null)
  verify(test_point2_list?[-1] == null)

  // Point3List: [float3(1,2,3)]
  verify(test_point3_list?[0] != null)
  verify(*test_point3_list?[0] == float3(1.0, 2.0, 3.0))
  verify(test_point3_list?[1] == null)
  verify(test_point3_list?[-1] == null)

  // Point4List: [float4(1,2,3,4)]
  verify(test_point4_list?[0] != null)
  verify(*test_point4_list?[0] == float4(1.0, 2.0, 3.0, 4.0))
  verify(test_point4_list?[1] == null)
  verify(test_point4_list?[-1] == null)

  // StringList: ["hello", "world"]
  verify(test_string_list?[0] != null)
  verify(*test_string_list?[0] == "hello")
  verify(*test_string_list?[1] == "world")
  verify(test_string_list?[2] == null)
  verify(test_string_list?[-1] == null)

  // EidList: [] (empty)
  verify(test_eid_list?[0] == null)
  verify(test_eid_list?[-1] == null)

  // BoolList: [true, false]
  verify(test_bool_list?[0] != null)
  verify(*test_bool_list?[0] == true)
  verify(*test_bool_list?[1] == false)
  verify(test_bool_list?[2] == null)
  verify(test_bool_list?[-1] == null)


// Test ?as (read-only cast from ChildComponent) for all list types via Object
[es(on_appear)]
def test_safe_list_cast_ops(evt : Event;
                            test_object_with_lists : Object)
  let int_list = test_object_with_lists["int_list"] ?as IntList
  verify(int_list != null)
  verify(length(*int_list) == 1)

  let ipoint2_list = test_object_with_lists["ipoint2_list"] ?as IPoint2List
  verify(ipoint2_list != null)
  verify(length(*ipoint2_list) == 1)

  let ipoint3_list = test_object_with_lists["ipoint3_list"] ?as IPoint3List
  verify(ipoint3_list != null)
  verify(length(*ipoint3_list) == 1)

  let ipoint4_list = test_object_with_lists["ipoint4_list"] ?as IPoint4List
  verify(ipoint4_list != null)
  verify(length(*ipoint4_list) == 1)

  let float_list = test_object_with_lists["float_list"] ?as FloatList
  verify(float_list != null)
  verify(length(*float_list) == 1)

  let point2_list = test_object_with_lists["point2_list"] ?as Point2List
  verify(point2_list != null)
  verify(length(*point2_list) == 1)

  let point3_list = test_object_with_lists["point3_list"] ?as Point3List
  verify(point3_list != null)
  verify(length(*point3_list) == 1)

  let point4_list = test_object_with_lists["point4_list"] ?as Point4List
  verify(point4_list != null)
  verify(length(*point4_list) == 1)

  let string_list = test_object_with_lists["string_list"] ?as StringList
  verify(string_list != null)
  verify(length(*string_list) == 1)

  let eid_list = test_object_with_lists["eid_list"] ?as EidList
  verify(eid_list != null)
  verify(length(*eid_list) == 0)

  let bool_list = test_object_with_lists["bool_list"] ?as BoolList
  verify(bool_list != null)
  verify(length(*bool_list) == 1)

  // wrong type returns null
  verify(test_object_with_lists["int_list"] ?as FloatList == null)
  verify(test_object_with_lists["float_list"] ?as IntList == null)
