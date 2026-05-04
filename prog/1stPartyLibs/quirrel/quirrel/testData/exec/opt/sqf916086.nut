//#disable-optimizer
// seed = 916086
try{
local count = 0
let to_log = function(id, s) { let tp = typeof(s); print($"{id}: "); print(tp == "integer" || tp == "float" || tp == "string" ? s : tp); print("\n"); if (count++ > 1000) print["stop"]; }
{
  local v0 = -111
  v0 = -3
  if (v0 >= v0)
    to_log("1", 8)
}} catch (q) {print(q)}
print("===\n")