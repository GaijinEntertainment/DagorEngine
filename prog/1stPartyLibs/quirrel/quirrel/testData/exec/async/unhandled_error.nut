// A task-future faults via throw and is never awaited. The pump-tick sweep
// routes the unhandled error through the VM's installed error handler -- the
// same surface a synchronous unhandled throw reaches -- producing "AN ERROR
// HAS OCCURRED" from sqstd_aux_printerror. This test relies on testRunner.py
// redirecting both stdout and stderr to the same .out file.

async function failing() {
  throw "oops"
}

async function main() {
  failing()  // fire-and-forget, never awaited
  print("body done\n")
}

main()
print("script done\n")
