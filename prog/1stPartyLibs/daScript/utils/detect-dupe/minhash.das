options gen2

// MinHash signature length and shingle width. K=64 keeps the all-pairs
// dot product cheap; N=5 is the usual choice for code-token shingles.
let public K_HASHES = 64
let public N_GRAM = 5

let SHINGLE_SEED : uint64 = 0xcbf29ce484222325ul
let MAX_U64 : uint64 = 0xFFFFFFFFFFFFFFFFul


// Combine n consecutive token hashes into one shingle hash. `hash_combine64` is the builtin
// wyhash fold, so this stays sequence-order-sensitive without a hand-rolled mixer.
def shingle_hash(token_hashes : array<uint64>; start, n : int) : uint64 {
    var sh = SHINGLE_SEED
    for (j in range(n)) {
        sh = hash_combine64(sh, token_hashes[start + j])
    }
    return sh
}


// Apply the k-th hash function of the MinHash family to a base shingle hash.
def hash_with_seed(sh : uint64; k : int) : uint64 {
    return hash_combine64(sh, uint64(k))
}


// Build a K-slot MinHash signature for a token list. Pads short token
// lists so functions with fewer tokens than N_GRAM still get a stable
// signature (one shingle covering all tokens).
def public minhash_signature(tokens : array<string>) : array<uint64> {
    var sig : array<uint64>
    sig |> resize(K_HASHES)
    for (k in range(K_HASHES)) {
        sig[k] = MAX_U64
    }

    var token_hashes : array<uint64>
    token_hashes |> reserve(length(tokens))
    for (t in tokens) {
        token_hashes |> push(hash(t))
    }
    let pad_hash = hash("<PAD>")
    while (length(token_hashes) < N_GRAM) {
        token_hashes |> push(pad_hash)
    }

    let limit = length(token_hashes) - N_GRAM + 1
    for (i in range(limit)) {
        let sh = shingle_hash(token_hashes, i, N_GRAM)
        for (k in range(K_HASHES)) {
            let cand = hash_with_seed(sh, k)
            if (cand < sig[k]) {
                sig[k] = cand
            }
        }
    }
    return <- sig
}


// Estimate Jaccard similarity from two MinHash signatures: fraction of
// matching slots. With K=64, resolution is ~1.5%.
def public jaccard_estimate(sig_a, sig_b : array<uint64>) : float {
    if (length(sig_a) != length(sig_b) || empty(sig_a)) return 0.0f
    var matches = 0
    for (a, b in sig_a, sig_b) {
        if (a == b) {
            matches++
        }
    }
    return float(matches) / float(length(sig_a))
}
