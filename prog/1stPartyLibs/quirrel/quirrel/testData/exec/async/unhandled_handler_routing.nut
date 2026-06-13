// Phase 0: unhandled async faults route through the VM's installed error
// handler at the pump-tick boundary, the same surface a synchronous
// unhandled throw reaches. A caught fault must NOT reach the handler. The
// thrown value reaches both the handler and a script catch verbatim (bare).

from "debug" import seterrorhandler

seterrorhandler(function(err) {
  print("[handler] " + err + "\n")
})

async function unhandled() {
  throw "from unhandled"
}

async function caughtBody() {
  throw "from caught"
}

async function consumer() {
  try {
    await caughtBody()
  } catch (e) {
    print("script caught: " + e + "\n")
  }
}

unhandled()                // fire-and-forget: handler must receive "from unhandled"
consumer()                 // awaits + catches: handler must NOT receive anything from caughtBody
print("script done\n")
