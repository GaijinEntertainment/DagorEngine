from "async" import Future

// markHandled acknowledges a faulted future without consuming the value (getValue
// still reads it, getState still "faulted"). No "[sqasync] unhandled" line in the
// golden is the assertion that the ack suppressed the report.

// Acknowledge after the fault.
let f = Future()
f.reject("boom")
print("f: " + f.getState() + " / " + f.getValue() + "\n")
f.markHandled()
print("f after ack: " + f.getState() + " / " + f.getValue() + "\n")

// Pre-acknowledge before the fault.
let g = Future()
g.markHandled()
g.reject("later")
print("g: " + g.getState() + " / " + g.getValue() + "\n")

print("script done\n")
