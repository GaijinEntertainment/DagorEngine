from "async" import Promise

// A named async function rejects (throws) and its task-promise is
// fire-and-forgotten from a named script function. When the task-promise
// dies unawaited, the enriched diagnostic must report: the async fn
// identity + its definition line, the rejection reason, and the launch
// site (caller source:line + caller name). This also probes that v->ci
// at task-promise creation is the caller frame, not the async frame.

async function failing() {
  throw "boom"
}

function launcher() {
  failing()  // fire-and-forget: the returned task-promise is discarded
}

launcher()
print("script done\n")
