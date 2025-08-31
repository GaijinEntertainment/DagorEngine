require ecs

[es]
def array_iterate_es(info : UpdateStageInfoAct; ecs_array_int_float_point2_1_2_21 : Array)
  assert(ecs_array_int_float_point2_1_2_21.size == 3u)
  verify(*get_int(ecs_array_int_float_point2_1_2_21[0]) == 1)
  verify(*get_float(ecs_array_int_float_point2_1_2_21[1]) == 2.)
  verify(*get_Point2(ecs_array_int_float_point2_1_2_21[2]) == float2(2, 1))
  var i = 0
  for a in ecs_array_int_float_point2_1_2_21
    if (i == 0)
      verify(*get_int(a) == 1)
    elif (i == 1)
      verify(*get_float(a) == 2.)
    elif (i == 3)
      verify(*get_Point2(a) == float2(2, 1))
    ++i
    //print ("{a.getUserType}: int:{get_int(a)} float:{get_float(a)} float2:{get_Point2(a)}")
