#!/usr/bin/env python3
# Copyright (C) Gaijin Games KFT.  All rights reserved.
"""Statistically compare two daProfiler captures (the `daprofiler-capture` JSON
produced by dap_to_json.py, or .dap files directly) and report which timelines
are significantly slower/faster.

Each capture's per-frame values are treated as independent samples of a
distribution (the two captures are separate runs, so frame i in OLD is unrelated
to frame i in NEW -- we compare distributions, not frame-by-frame). For overall
frame time, GPU frame time, each GPU draw stat, and every CPU/GPU scope matched by
name+thread we compute:

  - Mann-Whitney U (two-sided, tie-corrected normal approx) -- robust to the
    right-skewed, spike-prone shape of frame times. This is the significance test.
  - Welch's t-test (exact p via the regularized incomplete beta) -- secondary.
  - Cliff's delta -- nonparametric effect size (how separated the distributions are).
  - Benjamini-Hochberg across all scope tests -- q-values, so testing ~200 scopes
    does not manufacture false "significant" hits.

A scope is flagged a regression/improvement only when q < alpha AND the mean
per-frame change clears both a relative (%) and an absolute (ms) floor, so noise
and trivial deltas are filtered. Convention: OLD = baseline, NEW = the change.

If scipy is importable it is NOT required; everything here is stdlib.
"""
import sys, os, json, math, argparse, re
from bisect import bisect_right

# ------------------------------------------------------------ CPU/GPU classify
#
# A regression on the CPU side and a regression on the GPU side need opposite
# fixes, so every scope is bucketed:
#   GPU       -- a detailed GPU-timeline event (type == "gpu").
#   GPU-wait  -- a CPU thread parked waiting for the GPU (present / swapchain /
#                latency / fence). A rise here means the GPU got slower, not the CPU.
#   Wait      -- a CPU thread parked on something else (job pool, another thread).
#   CPU       -- actual CPU work.
#
# Two backend-independent work-cycle brackets, summed inclusive for the CPU/GPU split:
# the begin-frame GPU-latency wait and the present. Both wrap backend-specific leaves
# (gpuLatencyWait, waitForFrameStart, DX12_waitForFrameProgress, vulkan_latency_wait,
# latency_slop__present_dx11, ...), so the two brackets alone give a double-count-free
# "CPU blocked on GPU" total; the leaves are matched by pattern only to *tag* scopes,
# never summed. The begin-frame wait is TIME_PROFILE(low_latency) in workCycle.cpp but
# TIME_PROFILE(begin_frame) on daNetGame (drawScene.cpp wraps dagor_start_next_frame); a
# capture has one or the other, so they are alternatives within the first group. Present
# is TIME_D3D_PROFILE_NAME(present,("blockedPresent")).
SPLIT_BRACKET_GROUPS = (("low_latency", "begin_frame"), ("blockedPresent",))
GPU_WAIT_PARENTS = frozenset(n for g in SPLIT_BRACKET_GROUPS for n in g)

# Token-specific (not a bare "wait") so generic job waits -- tpool_wait,
# ecs_pfor_wait, wait_ri_cull_jobs, wait_cascade, wait_*_visibility -- stay CPU.
# present(?!ation): match "blockedPresent"/"CmdPresent" but not the UI script scope
# "decoratorsPresentation". swapchain only with acquire (vulkan_swapchain_*_acquire
# block on the compositor), but NOT vulkan_swapchain_acquire_contention -- that spins on
# a mutex held by the backend thread, so it is thread contention (a Wait), not a GPU
# wait. Bare swapchain scopes like vulkan_swapchain_update are CPU recreation work.
# Readback handled by the exact driver wait name only -- bare "*_gpu_readback" scopes
# are async ops, not blocking waits.
_GPU_WAIT_RE = re.compile(
    r"present(?!ation)|swapchain.*acquire(?!_contention)|acquire.*swapchain|drawable|vsync|"
    r"gpulatency|low_?latency|latency_wait|latency_slop|"
    # not waitforfree/waittovisit: DX12_waitForFree/_waitToVisit block on the command
    # ring's free/committed counters (a CPU-side consumer), so they are thread Wait.
    r"waitforframe(start|progress)|waitforasync|"
    r"pending_frame_gpu_wait|readback_gpu_wait|frame_info_wait|"
    r"wait_prev_frame|nv_low_latency",
    re.I)

# Generic blocking (job pool, another thread, sleeps, lock contention) -- checked only
# after the GPU-wait test, so present/latency waits never fall through to here.
_WAIT_RE = re.compile(r"wait|sleep|idle|stall|spin|contention", re.I)


def classify_kind(name, typ, is_wait=False):
    if typ == 'gpu':
        return 'GPU'
    if name in GPU_WAIT_PARENTS or _GPU_WAIT_RE.search(name):
        return 'GPU-wait'
    # The profiler's explicit IsWait flag (carried in the JSON as "isWait") catches
    # lock waits whose names the regex misses -- critsec, mutex, global_mutex,
    # rwlock_read/write, rpc. The present/latency check above still wins, so a flagged
    # wait that is really a GPU wait stays GPU-wait.
    if is_wait or _WAIT_RE.search(name):
        return 'Wait'
    return 'CPU'


# ------------------------------------------------------------------ statistics


def mean(a):
    return sum(a) / len(a) if a else 0.0


def median(a):
    if not a:
        return 0.0
    s = sorted(a); n = len(s); m = n // 2
    return s[m] if n % 2 else 0.5 * (s[m - 1] + s[m])


def pstdev(a):
    n = len(a)
    if n < 2:
        return 0.0
    m = mean(a)
    return math.sqrt(sum((x - m) ** 2 for x in a) / n)


def percentile(a, p):
    if not a:
        return 0.0
    s = sorted(a); k = max(0, min(len(s) - 1, int(math.ceil(p / 100.0 * len(s))) - 1))
    return s[k]


def norm_sf(z):
    return 0.5 * math.erfc(z / math.sqrt(2.0))


def _rankdata(a):
    n = len(a)
    order = sorted(range(n), key=lambda i: a[i])
    ranks = [0.0] * n
    ties = []
    i = 0
    while i < n:
        j = i
        while j + 1 < n and a[order[j + 1]] == a[order[i]]:
            j += 1
        avg = (i + j + 2) / 2.0          # 1-based average rank of positions i..j
        for k in range(i, j + 1):
            ranks[order[k]] = avg
        if j > i:
            ties.append(j - i + 1)
        i = j + 1
    return ranks, ties


def mann_whitney(new, old):
    """Two-sided MWU of NEW vs OLD. Returns (p, cliffs_delta). Positive delta =>
    NEW tends larger (slower)."""
    n1, n2 = len(new), len(old)
    if n1 < 3 or n2 < 3:
        return (1.0, 0.0)
    ranks, ties = _rankdata(list(new) + list(old))
    r_new = sum(ranks[:n1])
    u_new = r_new - n1 * (n1 + 1) / 2.0
    cliff = 2.0 * u_new / (n1 * n2) - 1.0
    N = n1 + n2
    mu = n1 * n2 / 2.0
    tie = sum(t ** 3 - t for t in ties)
    var = n1 * n2 / 12.0 * ((N + 1) - tie / (N * (N - 1.0)))
    if var <= 0:
        return (1.0, cliff)
    z = (u_new - mu) / math.sqrt(var)
    p = min(1.0, 2.0 * norm_sf(abs(z)))
    return (p, cliff)


def _betacf(a, b, x):
    MAXIT, EPS, FPMIN = 300, 3e-16, 1e-300
    qab, qap, qam = a + b, a + 1.0, a - 1.0
    c = 1.0
    d = 1.0 - qab * x / qap
    if abs(d) < FPMIN:
        d = FPMIN
    d = 1.0 / d
    h = d
    for m in range(1, MAXIT + 1):
        m2 = 2 * m
        aa = m * (b - m) * x / ((qam + m2) * (a + m2))
        d = 1.0 + aa * d
        if abs(d) < FPMIN:
            d = FPMIN
        c = 1.0 + aa / c
        if abs(c) < FPMIN:
            c = FPMIN
        d = 1.0 / d
        h *= d * c
        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2))
        d = 1.0 + aa * d
        if abs(d) < FPMIN:
            d = FPMIN
        c = 1.0 + aa / c
        if abs(c) < FPMIN:
            c = FPMIN
        d = 1.0 / d
        de = d * c
        h *= de
        if abs(de - 1.0) < EPS:
            break
    return h


def betainc(x, a, b):
    """Regularized incomplete beta I_x(a, b)."""
    if x <= 0.0:
        return 0.0
    if x >= 1.0:
        return 1.0
    lbeta = math.lgamma(a + b) - math.lgamma(a) - math.lgamma(b)
    bt = math.exp(lbeta + a * math.log(x) + b * math.log(1.0 - x))
    if x < (a + 1.0) / (a + b + 2.0):
        return bt * _betacf(a, b, x) / a
    return 1.0 - bt * _betacf(b, a, 1.0 - x) / b


def welch_t(new, old):
    """Returns (t, df, p_two_sided)."""
    n1, n2 = len(new), len(old)
    if n1 < 2 or n2 < 2:
        return (0.0, 0.0, 1.0)
    m1, m2 = mean(new), mean(old)
    v1 = pstdev(new) ** 2 * n1 / (n1 - 1)
    v2 = pstdev(old) ** 2 * n2 / (n2 - 1)
    se2 = v1 / n1 + v2 / n2
    if se2 <= 0:
        return (0.0, 0.0, 1.0 if m1 == m2 else 0.0)
    t = (m1 - m2) / math.sqrt(se2)
    df = se2 ** 2 / ((v1 / n1) ** 2 / (n1 - 1) + (v2 / n2) ** 2 / (n2 - 1))
    x = df / (df + t * t)
    p = min(1.0, betainc(x, df / 2.0, 0.5))
    return (t, df, p)


def benjamini_hochberg(pvals):
    n = len(pvals)
    if n == 0:
        return []
    order = sorted(range(n), key=lambda i: pvals[i])
    q = [0.0] * n
    prev = 1.0
    for rank in range(n - 1, -1, -1):
        i = order[rank]
        val = min(prev, pvals[i] * n / (rank + 1))
        q[i] = val
        prev = val
    return q


# ------------------------------------------------------------------ loading


def load_capture(path, conv_kwargs=None):
    if path.lower().endswith('.dap'):
        here = os.path.dirname(os.path.abspath(__file__))
        if here not in sys.path:
            sys.path.insert(0, here)
        import dap_to_json
        return dap_to_json.convert_file(path, **(conv_kwargs or {}))
    with open(path, 'r', encoding='utf-8') as f:
        doc = json.load(f)
    if doc.get('format') != 'daprofiler-capture':
        raise SystemExit("%s is not a daprofiler-capture JSON" % path)
    # series_ms() assumes microseconds; reject a future/hand-written doc that carries the
    # same tag but a different version or unit rather than comparing with wrong units.
    if doc.get('formatVersion') != 1 or doc.get('timeUnit') != 'us':
        raise SystemExit("%s has unsupported daprofiler-capture version/unit "
                         "(formatVersion=%r, timeUnit=%r)" %
                         (path, doc.get('formatVersion'), doc.get('timeUnit')))
    return doc


def series_ms(arr, skip):
    return [v / 1000.0 for v in arr[skip:]]


class Stat:
    """Summary + test result for one comparable series."""
    def __init__(self, label, old, new):
        self.label = label
        self.n_old, self.n_new = len(old), len(new)
        self.mean_old, self.mean_new = mean(old), mean(new)
        self.med_old, self.med_new = median(old), median(new)
        self.p95_old, self.p95_new = percentile(old, 95), percentile(new, 95)
        self.d_mean = self.mean_new - self.mean_old
        self.pct = (100.0 * self.d_mean / self.mean_old) if self.mean_old else (
            math.inf if self.d_mean else 0.0)
        self.p, self.cliff = mann_whitney(new, old)
        _, _, self.welch_p = welch_t(new, old)
        self.q = self.p          # replaced by BH later for scopes


# ------------------------------------------------------------------ compare


def compare(old_doc, new_doc, skip=0, alpha=0.05, min_pct=2.0, min_ms=0.05):
    # Validate --skip-warmup before it reaches any slice: a negative skip keeps only
    # the tail frames, and skip >= frame count yields empty series that the stats
    # helpers silently report as 0 ms / p=1 instead of failing.
    n_old = len(old_doc['frames']['cpuUs'])
    n_new = len(new_doc['frames']['cpuUs'])
    if skip < 0:
        raise SystemExit("--skip-warmup must be >= 0 (got %d)" % skip)
    if skip >= min(n_old, n_new):
        raise SystemExit("--skip-warmup %d drops every frame (captures have %d and %d "
                         "frames)" % (skip, n_old, n_new))
    res = {'old': old_doc['meta'], 'new': new_doc['meta'], 'skip': skip,
           'alpha': alpha, 'min_pct': min_pct, 'min_ms': min_ms}

    # overall frame time
    res['frame'] = Stat('frame CPU time',
                        series_ms(old_doc['frames']['cpuUs'], skip),
                        series_ms(new_doc['frames']['cpuUs'], skip))
    if old_doc['meta'].get('hasGpu') and new_doc['meta'].get('hasGpu'):
        res['gpu_frame'] = Stat('frame GPU time',
                                series_ms(old_doc['frames']['gpuUs'], skip),
                                series_ms(new_doc['frames']['gpuUs'], skip))

    # GPU draw stats (raw per-frame totals, not ms)
    res['gpu_stats'] = []
    og, ng = old_doc.get('gpu'), new_doc.get('gpu')
    if og and ng:
        for key in ('drawPrims', 'tris', 'instructions', 'renderTargets',
                    'programs', 'renderPasses'):
            o = og.get(key, [])[skip:]; n = ng.get(key, [])[skip:]
            if (sum(o) + sum(n)) == 0:
                continue
            res['gpu_stats'].append(Stat(key, o, n))

    # CPU vs GPU attribution: split the frame into time blocked on the GPU (the two
    # backend-independent work-cycle brackets, summed inclusive -- leaves nest inside,
    # so no double count) and the CPU residual. Works with or without a detailed GPU
    # timeline, which is the point: a capture with no GPU thread still shows whether
    # the regression is GPU-bound via the present/latency wait.
    # Require BOTH brackets with per-frame data: if the converter demoted one to
    # scopesTail (below the detail threshold, or --no-perframe), a capture would
    # contribute only one bracket. Summing a different bracket set on each side makes
    # the frame-minus-wait residual asymmetric and misleading, so refuse the split
    # unless both captures carry both brackets per-frame.
    def gpu_wait_series(doc):
        by_name = {s['name']: s for s in doc.get('scopes', [])
                   if s.get('type') != 'gpu' and 'perFrame' in s}
        acc = None
        for group in SPLIT_BRACKET_GROUPS:
            picked = next((by_name[n] for n in group if n in by_name), None)
            if picked is None:       # a bracket group has no per-frame member -> no split
                return None
            arr = picked['perFrame']['inclUs']
            if acc is None:
                acc = list(arr)
            else:
                for i, v in enumerate(arr):
                    acc[i] += v
        return series_ms(acc, skip)

    gw_old, gw_new = gpu_wait_series(old_doc), gpu_wait_series(new_doc)
    if gw_old and gw_new and (sum(gw_old) + sum(gw_new)) > 0:
        res['gpu_wait'] = Stat('CPU blocked on GPU', gw_old, gw_new)
        fr_old = series_ms(old_doc['frames']['cpuUs'], skip)
        fr_new = series_ms(new_doc['frames']['cpuUs'], skip)
        cw_old = [max(0.0, f - w) for f, w in zip(fr_old, gw_old)]
        cw_new = [max(0.0, f - w) for f, w in zip(fr_new, gw_new)]
        res['cpu_work'] = Stat('CPU work (frame minus GPU wait)', cw_old, cw_new)

    # scopes matched by (thread, name). Index both the detailed scopes and the
    # summary-only tail: a tail scope is a real scope, just without per-frame arrays.
    # Omitting the tail makes a scope that crosses the --top-scopes / --min-incl-ms
    # detail threshold between the two captures look newly added or removed.
    # The single GPU timeline's display name carries the adapter string (e.g.
    # "GPU:AMD Radeon-based (PS5) PS5"), which varies by machine/driver/build, so
    # match its scopes under a canonical thread -- otherwise every GPU pass shows
    # up as wholly removed+new and the GPU regression table comes out empty.
    def match_key(s):
        thread = 'GPU' if s.get('type') == 'gpu' else s['thread']
        return (thread, s['name'])
    def index(doc):
        d = {}
        for s in doc.get('scopes', []) + doc.get('scopesTail', []):
            if s.get('idle'):        # synthetic idle-thread placeholder, not real work
                continue
            d[match_key(s)] = s
        return d
    oi, ni = index(old_doc), index(new_doc)
    keys = set(oi) | set(ni)

    matched, new_only, removed, crossing = [], [], [], []
    for k in keys:
        o, n = oi.get(k), ni.get(k)
        if o and n:
            o_pf, n_pf = 'perFrame' in o, 'perFrame' in n
            if o_pf and n_pf:
                so = series_ms(o['perFrame']['inclUs'], skip)
                sn = series_ms(n['perFrame']['inclUs'], skip)
                st = Stat(o['name'], so, sn)
                # Prefer the NEW src: scopes are matched by (thread, name), so if the
                # instrumentation moved between captures the actionable file:line is the
                # change side, not the baseline (fall back to OLD when NEW has none).
                st.thread = o['thread']; st.type = o['type']
                st.src = n.get('src', '') or o.get('src', '')
                st.kind = classify_kind(o['name'], o['type'],
                                        o.get('isWait', False) or n.get('isWait', False))
                st.calls_old = mean(o['perFrame']['calls'][skip:])
                st.calls_new = mean(n['perFrame']['calls'][skip:])
                st.self_old = mean(series_ms(o['perFrame']['selfUs'], skip))
                st.self_new = mean(series_ms(n['perFrame']['selfUs'], skip))
                st.d_self = st.self_new - st.self_old
                matched.append(st)
            elif o_pf or n_pf:
                # Present on both sides but detailed on only one: it crossed the
                # --top-scopes / --min-incl-ms threshold between captures. No per-frame
                # test is possible, but it is neither new nor removed -- report it
                # separately so it is not silently dropped. Summary-only on both sides
                # is below the detail threshold in both, so it is intentionally ignored.
                crossing.append((o, n))
        elif n:
            new_only.append(n)
        else:
            removed.append(o)

    # multiple-comparison correction over the matched scope tests
    qs = benjamini_hochberg([st.p for st in matched])
    for st, q in zip(matched, qs):
        st.q = q

    def significant(st):
        return (st.q < alpha and abs(st.pct) >= min_pct and abs(st.d_mean) >= min_ms)

    # rank by self-time delta: attribute the change to where the cost actually
    # moved, so pass-through containers (self ~= 0) sink below the real work.
    regr = sorted([s for s in matched if significant(s) and s.d_mean > 0],
                  key=lambda s: -s.d_self)
    impr = sorted([s for s in matched if significant(s) and s.d_mean < 0],
                  key=lambda s: s.d_self)

    # When the whole capture drifts (e.g. a slower run), every scope inherits that
    # shift and looks "regressed". Subtract the frame-time drift to surface scopes
    # whose cost changed out of step with the frame -- those are the real shape changes.
    fp = res['frame'].pct if res['frame'].pct != math.inf else 0.0
    for s in matched:
        s.excess = (s.pct - fp) if s.pct != math.inf else math.inf
    dispro = [s for s in matched if significant(s)
              and (s.excess == math.inf or abs(s.excess) >= max(min_pct, 2.0 * min_pct))]
    dispro.sort(key=lambda s: -(abs(s.excess) if s.excess != math.inf else 1e9))
    res['frame_pct'] = fp
    res['disproportionate'] = dispro
    res['matched'] = matched
    res['regressions'] = regr
    res['improvements'] = impr

    # One-sided and threshold-crossing scopes: apply --skip-warmup where per-frame
    # data exists, so a scope that ran only during skipped warmup frames is not
    # reported as new/removed. Summary-only tail scopes have no per-frame arrays, so
    # they fall back to the full-capture total (skip cannot be applied to them).
    def one_sided(s, full_fc):
        pf = s.get('perFrame')
        if pf:
            incl = sum(pf['inclUs'][skip:]); fc = max(1, full_fc - skip)
        else:
            incl = s['inclTotalUs']; fc = full_fc or 1
        s = dict(s); s['_inclPost'] = incl; s['_msPerFrame'] = incl / 1000.0 / fc
        return s

    new_only = [one_sided(s, n_new) for s in new_only]
    removed = [one_sided(s, n_old) for s in removed]
    res['new_only'] = sorted([s for s in new_only if s['_inclPost'] > 0],
                             key=lambda s: -s['_inclPost'])
    res['removed'] = sorted([s for s in removed if s['_inclPost'] > 0],
                            key=lambda s: -s['_inclPost'])

    def crossing_row(o, n):
        co, cn = one_sided(o, n_old), one_sided(n, n_new)
        return {'name': o['name'], 'thread': o['thread'], 'src': o.get('src', ''),
                'oldMsPerFrame': co['_msPerFrame'], 'newMsPerFrame': cn['_msPerFrame'],
                'detailed': 'OLD' if 'perFrame' in o else 'NEW'}
    res['crossing'] = sorted([crossing_row(o, n) for (o, n) in crossing],
                             key=lambda r: -abs(r['newMsPerFrame'] - r['oldMsPerFrame']))
    res['n_tested'] = len(matched)
    return res


# ------------------------------------------------------------------ reporting


def _fpct(p):
    return "inf" if p == math.inf else ("%+.1f%%" % p)


def _verdict(st, alpha):
    if st.p >= alpha:
        return "no significant change"
    word = "SLOWER" if st.d_mean > 0 else "FASTER"
    return "%s %s (p=%.1e)" % (_fpct(st.pct).lstrip('+'), word, st.p)


def _attribution_verdict(res):
    f = res['frame']; gwt = res.get('gpu_wait'); cw = res.get('cpu_work'); gf = res.get('gpu_frame')
    gpu_line = (" Detailed GPU frame time %+.2f ms (%s)." % (gf.d_mean, _fpct(gf.pct))) if gf else ""
    if f.p >= res['alpha']:
        return "**Attribution:** no significant frame-time change.%s" % gpu_line
    if not (gwt and cw):
        return ("**Attribution:** the present/latency brackets (low_latency or "
                "begin_frame, plus blockedPresent) are not both present with per-frame "
                "data on both captures, so the CPU/GPU split is unavailable.%s" % gpu_line)
    dgw, dcw = gwt.d_mean, cw.d_mean
    side = "GPU-bound" if abs(dgw) >= abs(dcw) else "CPU-bound"
    return ("**Attribution: NEW is %s** -- CPU-blocked-on-GPU %+.2f ms, CPU work "
            "%+.2f ms (of %+.2f ms frame).%s" % (side, dgw, dcw, f.d_mean, gpu_line))


def format_md(res):
    om, nm = res['old'], res['new']
    L = []
    L.append("# daProfiler capture comparison\n")
    L.append("- **OLD (baseline):** `%s` -- %d frames%s" %
             (om.get('source', '?'), om.get('frameCount', 0),
              (", " + om['dumpTime']) if om.get('dumpTime') else ""))
    L.append("- **NEW (change):**  `%s` -- %d frames%s" %
             (nm.get('source', '?'), nm.get('frameCount', 0),
              (", " + nm['dumpTime']) if nm.get('dumpTime') else ""))
    plat = nm.get('platform', '?'); cpu = nm.get('cpu', '')
    L.append("- platform `%s`%s, exe `%s`" %
             (plat, (", cpu `%s`" % cpu) if cpu else "", nm.get('executable', '?')))
    if res['skip']:
        L.append("- skipped first %d warmup frame(s) per capture" % res['skip'])
    L.append("- tested %d matched scopes; gates: q<%.3g, |d-mean|>=%.2g%% and >=%.3g ms\n"
             % (res['n_tested'], res['alpha'], res['min_pct'], res['min_ms']))

    f = res['frame']
    L.append("## Overall frame time\n")
    L.append("| metric | OLD | NEW | delta |")
    L.append("|---|--:|--:|--:|")
    L.append("| mean ms | %.2f | %.2f | %+.2f (%s) |" %
             (f.mean_old, f.mean_new, f.d_mean, _fpct(f.pct)))
    L.append("| median ms | %.2f | %.2f | %+.2f |" % (f.med_old, f.med_new, f.med_new - f.med_old))
    L.append("| p95 ms | %.2f | %.2f | %+.2f |" % (f.p95_old, f.p95_new, f.p95_new - f.p95_old))
    L.append("")
    L.append("**Verdict: NEW is %s** (Mann-Whitney p=%.2e, Welch p=%.2e, Cliff's d=%.2f).\n"
             % (_verdict(f, res['alpha']), f.p, f.welch_p, f.cliff))

    # CPU vs GPU attribution -- the headline answer to "which side regressed".
    gwt = res.get('gpu_wait'); cw = res.get('cpu_work'); gf = res.get('gpu_frame')
    L.append("## CPU vs GPU attribution\n")
    L.append("_Frame time splits into the main thread blocked on the GPU "
             "(`low_latency` begin-frame wait + `blockedPresent` present) and the CPU "
             "residual. A rise in 'CPU blocked on GPU' means the GPU got slower, not "
             "the CPU. This split works even without a detailed GPU timeline._\n")
    L.append("| measure | OLD ms | NEW ms | delta | p |")
    L.append("|---|--:|--:|--:|--:|")
    L.append("| frame (total) | %.2f | %.2f | %+.2f (%s) | %.1e |" %
             (f.mean_old, f.mean_new, f.d_mean, _fpct(f.pct), f.p))
    if gf:
        L.append("| GPU frame time (detailed) | %.2f | %.2f | %+.2f (%s) | %.1e |" %
                 (gf.mean_old, gf.mean_new, gf.d_mean, _fpct(gf.pct), gf.p))
    if gwt:
        L.append("| CPU blocked on GPU | %.2f | %.2f | %+.2f (%s) | %.1e |" %
                 (gwt.mean_old, gwt.mean_new, gwt.d_mean, _fpct(gwt.pct), gwt.p))
    if cw:
        L.append("| CPU work (residual) | %.2f | %.2f | %+.2f (%s) | %.1e |" %
                 (cw.mean_old, cw.mean_new, cw.d_mean, _fpct(cw.pct), cw.p))
    L.append("")
    L.append(_attribution_verdict(res) + "\n")

    if res['gpu_stats']:
        L.append("## GPU draw stats (per-frame totals, rasterization only)\n")
        L.append("_These count rasterization (draws, triangles, instructions, ...). "
                 "Compute / ray-tracing work (e.g. `bvh_rt`) is not counted here, so "
                 "equal draw stats do NOT imply equal GPU workload._\n")
        L.append("| counter | OLD mean | NEW mean | delta | p |")
        L.append("|---|--:|--:|--:|--:|")
        for s in res['gpu_stats']:
            L.append("| %s | %.0f | %.0f | %s | %.1e |" %
                     (s.label, s.mean_old, s.mean_new, _fpct(s.pct), s.p))
        L.append("")

    # A GPU-timeline regression and a CPU-thread regression need opposite fixes, so
    # the scopes are reported in separate GPU and CPU sections (the attribution above
    # already says which side drives the frame). On the GPU side every row is a GPU
    # event, so `kind` is dropped; on the CPU side `kind` splits real work from waits.
    def gpu_scope_table(rows, header):
        L.append("## %s (%d)\n" % (header, len(rows)))
        if not rows:
            L.append("_none_\n"); return
        L.append("_Ranked by self-time change (where the cost actually moved); "
                 "pass-through containers whose time lives in children sink to the "
                 "bottom. `incl` is the inclusive subtree time for context._\n")
        L.append("| scope | self ms/f | d_self | incl ms/f | d_incl% | d_calls | q | cliff | src |")
        L.append("|---|--:|--:|--:|--:|--:|--:|--:|---|")
        for s in rows:
            L.append("| %s | %.2f->%.2f | %+.3f | %.2f->%.2f | %s | %+.2f | %.1e | %.2f | %s |" %
                     (s.label, s.self_old, s.self_new, s.d_self, s.mean_old, s.mean_new,
                      _fpct(s.pct), s.calls_new - s.calls_old, s.q, s.cliff, s.src))
        L.append("")

    def cpu_scope_table(rows, header):
        kc = {}
        for s in rows:
            kc[getattr(s, 'kind', 'CPU')] = kc.get(getattr(s, 'kind', 'CPU'), 0) + 1
        tag = (" -- " + ", ".join("%d %s" % (kc[k], k) for k in
               ('CPU', 'GPU-wait', 'Wait') if k in kc)) if rows else ""
        L.append("## %s (%d%s)\n" % (header, len(rows), tag))
        if not rows:
            L.append("_none_\n"); return
        L.append("_Ranked by self-time change (where the cost actually moved). `kind`: "
                 "CPU = CPU work, GPU-wait = main thread blocked on the GPU "
                 "(present/latency), Wait = blocked on other work (job pool, another "
                 "thread). `incl` is the inclusive subtree time for context._\n")
        L.append("| scope | kind | thread | self ms/f | d_self | incl ms/f | d_incl% | d_calls | q | cliff | src |")
        L.append("|---|---|---|--:|--:|--:|--:|--:|--:|--:|---|")
        for s in rows:
            L.append("| %s | %s | %s | %.2f->%.2f | %+.3f | %.2f->%.2f | %s | %+.2f | %.1e | %.2f | %s |" %
                     (s.label, getattr(s, 'kind', 'CPU'), s.thread[:14], s.self_old,
                      s.self_new, s.d_self, s.mean_old, s.mean_new, _fpct(s.pct),
                      s.calls_new - s.calls_old, s.q, s.cliff, s.src))
        L.append("")

    gpu_scope_table([s for s in res['regressions'] if s.type == 'gpu'],
                    "GPU regressions (NEW slower, by ms/frame)")
    cpu_scope_table([s for s in res['regressions'] if s.type != 'gpu'],
                    "CPU regressions (NEW slower, by ms/frame)")
    gpu_scope_table([s for s in res['improvements'] if s.type == 'gpu'],
                    "GPU improvements (NEW faster, by ms/frame)")
    cpu_scope_table([s for s in res['improvements'] if s.type != 'gpu'],
                    "CPU improvements (NEW faster, by ms/frame)")

    # Secondary lens only. A scope that *caused* the frame change moves in step with
    # the frame, so "in step" must never be read as "not real" -- the tables above are
    # the answer. This view helps only when you independently suspect a uniform
    # run-to-run drift (e.g. GPU clock/thermal): then scopes that moved by MORE than
    # the frame stand out as the least drift-explainable.
    dp = res['disproportionate']
    L.append("## Moved more than the frame as a whole (%d)\n" % len(dp))
    L.append("_Secondary: only meaningful if you separately suspect a uniform "
             "run-to-run drift. Do NOT treat the regressions above as noise just "
             "because they track the frame -- a scope that caused the regression "
             "tracks it by definition._\n")
    if not dp:
        L.append("_none changed materially more than the frame_\n")
    else:
        L.append("| scope | kind | thread | d_incl% | vs frame | OLD ms/f | NEW ms/f | q | cliff | src |")
        L.append("|---|---|---|--:|--:|--:|--:|--:|--:|---|")
        for s in dp[:20]:
            ex = "inf" if s.excess == math.inf else ("%+.1fpp" % s.excess)
            L.append("| %s | %s | %s | %s | %s | %.3f | %.3f | %.1e | %.2f | %s |" %
                     (s.label, getattr(s, 'kind', 'CPU'), s.thread[:14], _fpct(s.pct),
                      ex, s.mean_old, s.mean_new, s.q, s.cliff, s.src))
        L.append("")

    def list_only(rows, header, side):
        L.append("## %s (%d)\n" % (header, len(rows)))
        if not rows:
            L.append("_none_\n"); return
        L.append("| scope | thread | ms/f (%s) | calls | src |" % side)
        L.append("|---|---|--:|--:|---|")
        for s in rows[:25]:
            L.append("| %s | %s | %.3f | %d | %s |" %
                     (s['name'], s['thread'][:14], s['_msPerFrame'],
                      s['callsTotal'], s.get('src', '')))
        L.append("")

    list_only(res['new_only'], "New scopes (only in NEW)", 'NEW')
    list_only(res['removed'], "Removed scopes (only in OLD)", 'OLD')

    cx = res.get('crossing', [])
    if cx:
        L.append("## Threshold-crossing scopes (detailed on one side only) (%d)\n" % len(cx))
        L.append("_Present in both captures but with per-frame detail on only one side, "
                 "so no per-frame test is possible. Raise --top-scopes or lower "
                 "--min-incl-ms on both captures to compare these directly. The OLD and "
                 "NEW ms/f are not directly comparable: the summary-only side is a "
                 "full-capture average (warmup not skipped)._\n")
        L.append("| scope | thread | OLD ms/f | NEW ms/f | detailed | src |")
        L.append("|---|---|--:|--:|:-:|---|")
        for r in cx[:25]:
            L.append("| %s | %s | %.3f | %.3f | %s | %s |" %
                     (r['name'], r['thread'][:14], r['oldMsPerFrame'],
                      r['newMsPerFrame'], r['detailed'], r['src']))
        L.append("")

    L.append("## Method & caveats\n")
    L.append("- Significance is Mann-Whitney U (two-sided, tie-corrected) on per-frame "
             "values; q-values are Benjamini-Hochberg across all scope tests.")
    L.append("- Effect magnitude is the mean per-frame change (ms/f); scope tables are "
             "ranked by self-time change (where the cost actually moved).")
    L.append("- Cliff's d (column `cliff`) is the effect size: |d|>0.33 medium, >0.47 large.")
    L.append("- A significant absolute-ms change is a real observed difference to "
             "investigate. The tool cannot tell whether a proportional slowdown is a "
             "code regression or external drift (GPU clock/thermal) -- that needs "
             "repeat runs, not statistics on one pair. Do not dismiss findings as drift.")
    L.append("- GPU draw stats are rasterization counters only; compute / ray-tracing "
             "work is invisible to them, so equal draw stats are not equal GPU workload.")
    L.append("- Two separate runs: frame counts differ and single-run variance is real. "
             "For firmer conclusions capture several runs per side (or drop warmup "
             "frames with --skip-warmup).")
    return "\n".join(L)


def result_to_json(res):
    def stat(s):
        d = {'label': s.label, 'meanOld': s.mean_old, 'meanNew': s.mean_new,
             'deltaMean': s.d_mean, 'pct': (None if s.pct == math.inf else s.pct),
             'medOld': s.med_old, 'medNew': s.med_new, 'p': s.p, 'q': s.q,
             'welchP': s.welch_p, 'cliff': s.cliff}
        if hasattr(s, 'd_self'):
            d['selfOld'] = s.self_old; d['selfNew'] = s.self_new; d['deltaSelf'] = s.d_self
        for k in ('thread', 'type', 'kind', 'src'):
            if hasattr(s, k):
                d[k] = getattr(s, k)
        return d

    def one_sided_json(s):
        return {'name': s['name'], 'thread': s['thread'], 'src': s.get('src', ''),
                'inclTotalUs': s['_inclPost'], 'msPerFrame': s['_msPerFrame']}
    return {
        'old': res['old'], 'new': res['new'],
        'frame': stat(res['frame']),
        'gpuFrame': stat(res['gpu_frame']) if 'gpu_frame' in res else None,
        'gpuWait': stat(res['gpu_wait']) if 'gpu_wait' in res else None,
        'cpuWork': stat(res['cpu_work']) if 'cpu_work' in res else None,
        'attribution': _attribution_verdict(res).replace('**', ''),
        'gpuStats': [stat(s) for s in res['gpu_stats']],
        'framePct': res.get('frame_pct'),
        'disproportionate': [stat(s) for s in res['disproportionate']],
        'regressions': [stat(s) for s in res['regressions']],
        'improvements': [stat(s) for s in res['improvements']],
        # inclTotalUs / msPerFrame are post --skip-warmup (== full capture when skip=0)
        # so the machine-readable output excludes the same warmup frames as the report.
        'newScopes': [one_sided_json(s) for s in res['new_only']],
        'removedScopes': [one_sided_json(s) for s in res['removed']],
        'crossingScopes': res.get('crossing', []),
    }


def main(argv=None):
    ap = argparse.ArgumentParser(
        description="Statistically compare two daProfiler captures (.json or .dap).")
    ap.add_argument('old', help="baseline capture (.json or .dap)")
    ap.add_argument('new', help="changed capture (.json or .dap)")
    ap.add_argument('--skip-warmup', type=int, default=0,
                    help="drop the first N frames of each capture (default 0)")
    ap.add_argument('--alpha', type=float, default=0.05, help="significance level (q) gate")
    ap.add_argument('--min-change-pct', type=float, default=2.0,
                    help="minimum mean per-frame change (percent) to flag a scope")
    ap.add_argument('--min-delta-ms', type=float, default=0.05,
                    help="minimum mean per-frame change (ms) to flag a scope")
    ap.add_argument('--format', choices=('md', 'json', 'both'), default='md')
    ap.add_argument('--out', default=None, help="write report here (default: stdout for md)")
    ap.add_argument('--json-out', default=None, help="also write machine-readable JSON here")
    args = ap.parse_args(argv)

    # Reports are ASCII, but a stray non-ASCII scope name must not crash a cp1252
    # console; prefer utf-8 stdout where the runtime allows it.
    try:
        sys.stdout.reconfigure(encoding='utf-8')
    except Exception:
        pass

    old_doc = load_capture(args.old)
    new_doc = load_capture(args.new)
    res = compare(old_doc, new_doc, args.skip_warmup, args.alpha,
                  args.min_change_pct, args.min_delta_ms)

    if args.format in ('md', 'both'):
        md = format_md(res)
        if args.out:
            with open(args.out, 'w', encoding='utf-8') as f:
                f.write(md)
            sys.stderr.write("wrote %s\n" % args.out)
        else:
            sys.stdout.write(md + "\n")
    if args.format in ('json', 'both') or args.json_out:
        if args.format == 'json':
            # json-only: --out IS the json destination; do not append a second .json.
            jpath = args.json_out or args.out
        elif args.format == 'both':
            jpath = args.json_out or (args.out + '.json' if args.out else None)
        else:                          # md format with --json-out alongside
            jpath = args.json_out
        data = json.dumps(result_to_json(res), indent=1)
        if jpath:
            with open(jpath, 'w', encoding='utf-8') as f:
                f.write(data)
            sys.stderr.write("wrote %s\n" % jpath)
        elif args.format in ('json', 'both'):
            # no file given: 'both'/'json' print JSON to stdout rather than silently
            # dropping the requested machine-readable output.
            sys.stdout.write(data + "\n")

    # terse stdout headline (even when md went to a file)
    f = res['frame']
    split = ""
    if 'gpu_wait' in res and 'cpu_work' in res:
        split = " | GPU-wait %+.2f ms, CPU work %+.2f ms" % (
            res['gpu_wait'].d_mean, res['cpu_work'].d_mean)
    sys.stderr.write("\nframe: NEW is %s%s | regressions=%d (out-of-step=%d) improvements=%d "
                     "new=%d removed=%d\n"
                     % (_verdict(f, args.alpha), split, len(res['regressions']),
                        len(res['disproportionate']), len(res['improvements']),
                        len(res['new_only']), len(res['removed'])))
    return 0


if __name__ == '__main__':
    sys.exit(main())
