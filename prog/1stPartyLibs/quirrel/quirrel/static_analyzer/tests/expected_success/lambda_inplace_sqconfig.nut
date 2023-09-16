::calls_lambda_inplace <- require("sq3_sa_test").calls_lambda_inplace

local tab = []

foreach (v in [1, 2, 3])
  ::calls_lambda_inplace(@() v)

return tab

