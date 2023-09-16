//expect:w242

local ev = {
  victim = {bar = null}
  a = !victim.bar
}

print(ev.a)