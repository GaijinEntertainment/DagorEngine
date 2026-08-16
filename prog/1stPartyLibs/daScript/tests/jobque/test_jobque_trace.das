options gen2
require dastest/testing_boost public
require daslib/jobque_boost
require daslib/jobque_profile
require daslib/fio
require daslib/json_boost

// Round-trip of the per-lane event tracer with the jobque_profile registries: arm, register
// categories + markers, run a team dispatch storm with a tagged op and unit markers, save,
// re-read the JSON, and assert the self-describing header + events survived.
// Two worlds by design: with JOBQUE_PROFILING=1 the annotations must land in the trace;
// without it profile_tag/profile_marker compile away, and the same run proves the erasure
// contract — no tags, no unit markers, header and spans intact.

def read_text_file(path : string) : string {
    var text = ""
    fopen(path, "rb") $(f) {
        if (f != null) {
            text = fread(f)
        }
    }
    return text
}

def team_storm(nj, total : int) {
    var results : array<int>
    results |> resize(total)
    var pr = unsafe(addr(results[0]))
    team_parallel_for(0, total, nj) <| @(job_begin : int; job_end : int) {
        for (i in range(job_begin, job_end)) {
            unsafe(pr[i]) = i * 3 + 1
        }
    }
    delete results
}


// parse the saved trace and assert the self-describing header + events — the ON world
// checks the annotations landed, the OFF world checks they were compiled away
def private verify_saved_trace(t : T?; text : string; tokId : int) {
    var jerr = ""
    let root = read_json(text, jerr)
    t |> success(root != null, "trace file parses as JSON: {jerr}")

    // jobqueProfile header
    let prof = root?["jobqueProfile"]
    t |> success(prof != null, "jobqueProfile header present")
    t |> equal(1, prof?["version"] ?? 0)
    t |> success((prof?["lanes"] ?? 0) >= 1, "at least the caller lane")
    var sawSeven = false
    var sawAlpha = false
    if (prof?["categories"] != null) {
        for (cat in prof?["categories"] as _array) {
            let cname = cat?["name"] ?? ""
            if (cname == "seven") {
                sawSeven = true
                t |> equal(7, cat?["id"] ?? 0)
                t |> equal("#112233", cat?["color"] ?? "")
            } elif (cname == "alpha") {
                sawAlpha = true
                t |> equal(1, cat?["id"] ?? 0)
                t |> success(length(cat?["color"] ?? "") == 7, "auto color is #RRGGBB")
            }
        }
    }
    t |> success(sawSeven, "explicit category saved")
    t |> success(sawAlpha, "auto category saved")
    var sawTokenMarker = false
    if (prof?["markers"] != null) {
        for (mk in prof?["markers"] as _array) {
            if ((mk?["name"] ?? "") == "token") {
                sawTokenMarker = true
                t |> equal(tokId, mk?["id"] ?? -1)
            }
        }
    }
    t |> success(sawTokenMarker, "marker kind saved")

    // events: spans exist; both unit markers landed as perfetto instants; any publish
    // recorded during the tagged storm carries tag 7 in stage>>8 (workers may be absent
    // on a 1-core box — then the dispatch inlines and no publish is recorded)
    t |> success(root?["traceEvents"] != null, "traceEvents present")
    var nSpans = 0
    var markerArgs : array<int>
    var nTaggedPublish = 0
    var nUntaggedPublish = 0
    for (ev in root?["traceEvents"] as _array) {
        let ph = ev?["ph"] ?? ""
        if (ph == "X") {
            nSpans++
            if ((ev?["name"] ?? "") == "publish") {
                let packed = ev?["args"]?["stage"] ?? 0
                if ((packed >> 8) == 7) {
                    nTaggedPublish++
                } else {
                    nUntaggedPublish++
                }
            }
        } elif (ph == "i" && (ev?["name"] ?? "") == "token") {
            markerArgs |> push(ev?["args"]?["arg"] ?? -1)
        }
    }
    t |> success(nSpans > 0, "recorded work spans")
    static_if (JOBQUE_PROFILING_ENABLED) {
        t |> equal(0, nUntaggedPublish)
        feint("tagged publishes: {nTaggedPublish}\n")
        sort(markerArgs)
        t |> equal(3, length(markerArgs))
        t |> equal(-7, markerArgs[0])
        t |> equal(41, markerArgs[1])
        t |> equal(42, markerArgs[2])
    } else {
        // annotations compiled away — nothing tagged and no unit markers can exist
        t |> equal(0, nTaggedPublish)
        t |> equal(0, length(markerArgs))
    }
}

[test]
def test_trace_round_trip(t : T?) {
    t |> run("categories, markers, and events round-trip through save") @(t : T?) {
        with_job_que() {
            // registries: explicit id, auto id (idempotent), auto skips used ids
            profile_category(7, "seven", 0x112233u)
            let catAlpha = profile_category("alpha")
            t |> equal(1, catAlpha)
            t |> equal(catAlpha, profile_category("alpha"))
            profile_category(2, "two")
            t |> equal(3, profile_category("beta"))
            let tokId = profile_marker_id("token")
            t |> equal(0, tokId)
            t |> equal(tokId, profile_marker_id("token"))
            t |> equal(1, profile_marker_id("frame"))

            profile_start(8192)
            set_jobque_team_mode(true)
            profile_tag(7)
            team_storm(8, 4096)
            set_jobque_team_mode(false)
            profile_marker(tokId, 41)
            profile_marker("token", 42)
            profile_marker(tokId, -7)   // args are signed — must survive the JSON round-trip
            profile_stop()

            var terr = ""
            let dir = temp_directory(terr)
            t |> success(terr == "", "temp_directory resolves")
            let path = "{dir}/test_jobque_trace_out.json"
            t |> success(profile_save(path), "profile_save succeeds")

            let text = read_text_file(path)
            t |> success(!empty(text), "trace file is not empty")
            verify_saved_trace(t, text, tokId)
        }
    }
}
