//expect:w287

::idx <- require("sq3_sa_test").idx
::tblObj <- require("sq3_sa_test").tblObj

return (::idx < 0 || ::idx > ::tblObj.childrenCount())
