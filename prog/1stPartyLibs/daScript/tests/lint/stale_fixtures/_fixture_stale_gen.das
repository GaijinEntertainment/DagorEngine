options gen2
// Generic-body directives are consumed only when a CONSUMER instantiates the
// generic - and the consumer sorts AFTER this file. Pins the end-of-run stale
// scan: the per-file scan would call both directives stale (order bug).

def public gen_push(var dst : array<auto(TT)>; src : array<TT>) {
    for (s in src) { // nolint:PERF022
        dst |> push(s) // nolint:PERF006
    }
}
