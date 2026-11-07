from "string" import format, split_by_chars
from "system" import getenv
import "math" as math

if (__name__ == "__analysis__")
  return

let f = @(v) v
let a = [1]
let s = "abc"
let tbl = { a = 1 }
let own = { map = @(_) null }
let late = {}
late.map <- @(_) null
const C = [1]

"abc".len();                  //warning:discarded-nodiscard
"abc".contains("a");          //warning:discarded-nodiscard
([1, 2]).map(f);              //warning:discarded-nodiscard
({}).keys();                  //warning:discarded-nodiscard
({ a = 1 }).keys();           //warning:discarded-nodiscard
(const { a = 1 }).keys();     //warning:discarded-nodiscard
({ map = @(_) null }).map(f);
({ keys = 1 }).$keys()        //warning:discarded-nodiscard
late.$map(f)                  //warning:discarded-nodiscard
a.map(f)                      //warning:discarded-nodiscard
C.map(f)                      //warning:discarded-nodiscard
s.toupper();                  //warning:discarded-nodiscard
(1.5).tointeger();            //warning:discarded-nodiscard
(1).tostring()                //warning:discarded-nodiscard
true.tostring()               //warning:discarded-nodiscard
array(2).map(f)               //warning:discarded-nodiscard
format("%d", 1).len()         //warning:discarded-nodiscard
split_by_chars("a,b", ",").map(f) //warning:discarded-nodiscard
newthread(f).getstatus()      //warning:discarded-nodiscard
function mk(): string { return "a" }
mk().strip()
let fs = format("%d", 1)
fs.len()                      //warning:discarded-nodiscard
math.sqrt(4.0).tointeger()    //warning:discarded-nodiscard
s?.len()                      //warning:discarded-nodiscard
format?("%d", 1).len()        //warning:named-like-should-return

a.append(1)
a.each(f);
({ [s] = 1 }).keys();
({ ["a"] = 1 }).keys();
({ ...tbl }).keys()
getenv("PATH").strip()
own.map(f)
late.map(f)
class K { function [nodiscard] map(_) { return null } }
K().map(f)
tbl.values()
tbl.keys().map(f)
getroottable().keys()
$"{s}".len()                  //warning:named-like-should-return
let _ = a.map(f)
local x = []
x = x.filter(f)
x.map(f)

function unknown(p) {
  p.map(f)
  p.join(",")
}
unknown(a)

"abc".len() //-discarded-nodiscard
$"x={s}"                      //warning:discarded-nodiscard
