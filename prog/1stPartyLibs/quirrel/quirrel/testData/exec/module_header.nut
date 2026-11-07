@@"The module header: a docstring, then directives, then imports"
#forbid-root-table
#allow-switch-statement
from "math" import PI
import "string" as str

switch (1) {
  case 1:
    println(PI > 3)
    break
}
println(str.format("%d", 42))
