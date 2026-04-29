if (__name__ == "__analysis__")
  return

let sec = 10000
let s = sec > 0
let r = ::external_data //-undefined-global

if (r.isVisible() != s) // doesn't warn
    print("d")

if (r.foo() != s) // warns
    print("d")
