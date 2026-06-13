// BUG: sqbaselib.cpp base_newthread() accepts native closures via its typemask
// but unconditionally does _closure(func)->_function => reads garbage / crashes.
print("calling newthread(print)...\n")
local err = null
try {
  local t = newthread(print)
  print("newthread returned: " + typeof(t) + "\n")
} catch (e) {
  err = e
}
print("error: " + err + "\n")
print("SURVIVED (no crash)\n")
