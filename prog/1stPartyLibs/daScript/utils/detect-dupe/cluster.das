options gen2

require strings
require math
require minhash


struct public MemberRef {
    file : string
    name : string
    line : int
    end_line : int
    is_lambda : bool
}


struct public FuncRecord {
    ref : MemberRef
    canonical : string
    sig : array<uint64>
    token_count : int
    cluster_id : int = -1
    // Set true on records that came from the user's "focus set"
    // (e.g. PR files, or new functions vs. a baseline). When any
    // record is flagged, exact_buckets and fuzzy_pairs filter their
    // output to only entries that include at least one candidate.
    // Runtime/derived flag — never serialized in ExportedFunction.
    is_candidate : bool = false
}


struct public Cluster {
    canonical_sample : string
    token_count : int
    members : array<MemberRef>
}


struct public FuzzyPair {
    similarity : float
    a : MemberRef
    b : MemberRef
    canonical_a : string
    canonical_b : string
}


// Build exact-clone clusters by grouping records with identical
// canonical text. Mutates `records[i].cluster_id` so the fuzzy pass
// can skip same-cluster pairs. Skips clusters of size 1.
//
// Bucket on the canonical string itself (not its 64-bit hash) so a
// rare hash collision can't conflate two unrelated functions.
def public exact_buckets(var records : array<FuncRecord>; min_tokens : int;
                         filter_candidates : bool = false) : array<Cluster> {
    var buckets : table<string; array<int>>
    for (i in range(length(records))) {
        if (records[i].token_count < min_tokens) continue
        let key = records[i].canonical
        if (key_exists(buckets, key)) {
            buckets[key] |> push(i)
        } else {
            var lst : array<int>
            lst |> push(i)
            buckets[key] <- lst
        }
    }
    var out : array<Cluster>
    var cluster_idx = 0
    for (idxs in values(buckets)) {
        if (length(idxs) < 2) continue
        var c = Cluster(
            canonical_sample = records[idxs[0]].canonical,
            token_count = records[idxs[0]].token_count)
        c.members |> reserve(length(idxs))
        var cluster_has_candidate = false
        for (i in idxs) {
            // Always assign cluster_id, even for clusters we'll drop —
            // fuzzy_pairs uses cluster_id to skip same-canonical pairs,
            // and that semantics shouldn't depend on the filter.
            records[i].cluster_id = cluster_idx
            c.members |> push(records[i].ref)
            if (records[i].is_candidate) {
                cluster_has_candidate = true
            }
        }
        cluster_idx++
        // Drop clusters with no candidate members ONLY when the caller
        // requested filtering. Auto-detecting "any candidate exists"
        // would silently fall back to legacy mode in baseline runs
        // where genuinely zero records are new (e.g. CI on an unchanged
        // branch), which is the wrong default for a CI gate.
        if (filter_candidates && !cluster_has_candidate) continue
        out |> emplace(c)
    }
    return <- out
}


// All-pairs MinHash comparison. Skips pairs already in the same exact
// cluster (those are reported as exact, not fuzzy). Skips pairs where
// either function has fewer than min_tokens tokens (noise floor).
//
// MinHash works on the SET of shingles. Highly periodic code (e.g. a
// boilerplate block repeated N times) has a tiny shingle set, so 4x
// and 7x repetitions both estimate Jaccard = 1.0 even though they're
// not the same function. Gate the report on a length ratio too.
// Inner-body helper: the per-pair filtering and emit logic, shared by
// the legacy O(N²) loop and the candidate-restricted loop. `lo` and
// `hi` must satisfy lo < hi so the emitted pair's (a, b) ordering
// matches the legacy report.
def fuzzy_emit_if_match(records : array<FuncRecord>; lo : int; hi : int;
                        threshold : float; min_tokens : int;
                        var out : array<FuzzyPair>) {
    // Same canonical text would already be in the exact-cluster set
    // (caught above when both have a cluster_id). Compare on the string
    // itself rather than the 64-bit hash to dodge any theoretical
    // collision.
    if (records[lo].token_count < min_tokens || records[hi].token_count < min_tokens
        || (records[lo].cluster_id != -1 && records[lo].cluster_id == records[hi].cluster_id)
        || records[lo].canonical == records[hi].canonical) return
    let tl = float(records[lo].token_count)
    let th = float(records[hi].token_count)
    let len_ratio = tl < th ? tl / th : th / tl
    if (len_ratio < threshold) return
    let s = jaccard_estimate(records[lo].sig, records[hi].sig)
    // Geometric mean of MinHash similarity and length similarity:
    // periodic code with very different lengths drops out.
    let combined = sqrt(s * len_ratio)
    if (combined >= threshold) {
        var p = FuzzyPair(
            similarity = combined,
            a = records[lo].ref,
            b = records[hi].ref,
            canonical_a = records[lo].canonical,
            canonical_b = records[hi].canonical)
        out |> emplace(p)
    }
}


def public fuzzy_pairs(records : array<FuncRecord>; threshold : float; min_tokens : int;
                       filter_candidates : bool = false) : array<FuzzyPair> {
    var out : array<FuzzyPair>
    out |> reserve(64)
    let n = length(records)
    if (!filter_candidates) {
        for (i in range(n)) {
            for (j in range(i + 1, n)) {
                fuzzy_emit_if_match(records, i, j, threshold, min_tokens, out)
            }
        }
        return <- out
    }
    // Candidate-restricted path: outer iterates flagged records only,
    // inner iterates all. Saves O(N²) → O(K·N) when K = |candidates|
    // << N. Dedup rule: when both endpoints are candidates, only emit
    // during the iteration whose outer index is smaller — `candidate_indices`
    // is built in ascending order, so when records[j].is_candidate &&
    // j < i, the pair was already emitted at iteration j.
    let candidate_indices <- [for (i in range(n)); i; where records[i].is_candidate]
    for (i in candidate_indices) {
        for (j in range(n)) {
            if (j == i || (records[j].is_candidate && j < i)) continue
            let lo = min(i, j)
            let hi = max(i, j)
            fuzzy_emit_if_match(records, lo, hi, threshold, min_tokens, out)
        }
    }
    return <- out
}


def public member_key(m : MemberRef) : string {
    return "{m.file}:{m.line}:{m.name}"
}


// Sort clusters by member count descending; tie-break on token_count
// desc, then canonical_sample, then first-member location. Without
// tie-breakers `values(buckets)` iteration order leaks into the report
// and diff-ing two runs becomes noisy.
def public sort_clusters(var clusters : array<Cluster>) {
    sort(clusters) $(a, b) {
        let na = length(a.members)
        let nb = length(b.members)
        if (na != nb) return na > nb
        if (a.token_count != b.token_count) return a.token_count > b.token_count
        if (a.canonical_sample != b.canonical_sample) return a.canonical_sample < b.canonical_sample
        if (na == 0) return false
        return member_key(a.members[0]) < member_key(b.members[0])
    }
}


// Sort fuzzy pairs by similarity descending; tie-break on (a, b)
// member locations so pair ordering is reproducible across runs.
def public sort_pairs(var pairs : array<FuzzyPair>) {
    sort(pairs) $(a, b) {
        if (a.similarity != b.similarity) return a.similarity > b.similarity
        let ka = member_key(a.a)
        let kb = member_key(b.a)
        if (ka != kb) return ka < kb
        return member_key(a.b) < member_key(b.b)
    }
}
