//expect:w285

::sys <- require("sq3_sa_test").sys


local tubeNameV2 = ::sys.get_arg_value_by_name("userstat_tube_v2") ?? ""
if ((tubeNameV2 ?? "") != "")
  print($"userstat_tube_v2: {tubeNameV2}")
