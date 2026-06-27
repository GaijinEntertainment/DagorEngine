from "async" import Future

// Script-side reject() captures the call site: an abandoned rejected future
// reports an ERROR TRACE pointing at where reject() was called, not just the
// reason value (native future_throw, with no Quirrel stack, has no trace).

function rejectAt(f) {
    f.reject("boom")
}

let f = Future()
rejectAt(f)
// never awaited / markHandled -> the unhandled sweep reports it with the trace.

print("script done\n")
