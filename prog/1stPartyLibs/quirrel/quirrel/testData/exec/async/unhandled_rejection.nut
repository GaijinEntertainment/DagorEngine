from "async" import Promise

// A manual Promise is rejected and never awaited. When it dies, promise_release
// hook prints a one-line "[sqasync] unhandled promise rejection" diagnostic to
// the VM's error stream. This test relies on testRunner.py redirecting both
// stdout and stderr to the same .out file.

async function main() {
  let p = Promise()
  p.reject("oops")
  // p never gets awaited; goes out of scope when main's frame settles.
  print("body done\n")
}

main()
print("script done\n")
