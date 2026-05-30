// Phase 0: unhandled async faults route through the VM's installed error
// handler at the pump-tick boundary, the same surface a synchronous
// unhandled throw reaches. A caught fault must NOT reach the handler.

from "debug" import seterrorhandler

seterrorhandler(function(err) {
  print("[handler] " + (err instanceof Error ? err.message : err.tostring()) + "\n")
})

async function unhandled() {
  throw Error("from unhandled")
}

async function caughtBody() {
  throw Error("from caught")
}

async function consumer() {
  try {
    await caughtBody()
  } catch (e) {
    print("script caught: " + e.message + "\n")
  }
}

unhandled()                // fire-and-forget: handler must receive Error("from unhandled")
consumer()                 // awaits + catches: handler must NOT receive anything from caughtBody
print("script done\n")
