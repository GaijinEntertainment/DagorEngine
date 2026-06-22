// foreach over userdata carrying a plain table delegate (no _nexti) must
// not reinterpret the userdata as an instance

from "test.native" import ud_with_delegate

let ud = ud_with_delegate()
let entries = []
foreach (k, v in ud)
  entries.append($"{k}={v}")
entries.sort()
local joined = ""
foreach (i, e in entries)
  joined += (i ? "," : "") + e
println($"ud foreach: {joined}")

println("PASSED")
