// A failed constant call gives the cause of the failure in the compile error
from "math" import sqrt

const function [pure] throwing() {
  throw "boom"
}

const function [pure] throwingTable() {
  throw { code = 1 }
}

foreach (src in ["const E = sqrt(\"a\")", "const E = throwing()", "const E = throwingTable()"]) {
  try {
    compilestring(src, "failing", { sqrt, throwing, throwingTable })
    println("compiled")
  }
  catch (e) {
    println(e)
  }
}
