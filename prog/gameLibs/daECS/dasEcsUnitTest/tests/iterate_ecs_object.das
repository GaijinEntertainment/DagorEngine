require ecs

[es]
def object_iterate_es(info : UpdateStageInfoAct; ecs_object_int_float_point2_1_2_21 : Object)
  assert(ecs_object_int_float_point2_1_2_21.size == 3u)
  verify(*get_int(*(ecs_object_int_float_point2_1_2_21["int"])) == 1)
  verify(*get_float(*(ecs_object_int_float_point2_1_2_21["float"])) == 2.)
  verify(*get_Point2(*(ecs_object_int_float_point2_1_2_21["point2"])) == float2(2, 1))
  var missed = 0
  for a in ecs_object_int_float_point2_1_2_21
    //print ("key=\"{a.key}\" {a.value.getUserType} int= {get_int(a.value)} float = {get_float(a.value)}")
    if (a.key == "int")
      verify(*get_int(a.value) == 1)
    elif (a.key == "float")
      verify(*get_float(a.value) == 2.)
      verify(a.key == "float")
    elif (a.key == "point2")
      verify(*get_Point2(a.value) == float2(2, 1))
    else
      ++missed
  assert(missed == 0)
