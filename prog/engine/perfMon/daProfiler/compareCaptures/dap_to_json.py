#!/usr/bin/env python3
# Copyright (C) Gaijin Games KFT.  All rights reserved.
"""Convert a daProfiler binary capture (.dap) to the compact `daprofiler-capture`
JSON format used by compare_captures.py.

The binary format is the response stream produced by da_profiler::save_dump
(prog/engine/perfMon/daProfiler/daProfilerSaveDump.cpp). A .dap file is one or
more segments, each: an 8-byte DumpHeader (magic 0xC50FC50F, version u16, flags
u16) followed by a standard zlib stream of DataResponse records. Each record is
{version u32, size u32, type u16, application u16==0xC50F} + `size` payload bytes.
Integer encodings (VLQ) match daProfilerDumpUtils.h.

This aggregates per-scope, per-frame inclusive/self/call counts so the output is
small (~hundreds of KB) and ready for statistical comparison. See FORMAT.md.
"""
import sys, os, zlib, struct, json, argparse, bisect
from collections import defaultdict

MAGIC = 0xC50FC50F
APP = 0xC50F
FLAG_ZLIB = 1 << 1

# DataResponse::Type (daProfilerMessageTypes.h)
T_FrameDescBoard = 0
T_EventFrame = 1
T_UniqueName = 6
T_TagsPack = 8
T_CallstackDescBoard = 9
T_CallstackPack = 10
T_UniqueEventsBoard = 15
T_SummaryPack = 258
T_FramesPack = 259

TYPE_CPU = 0
TYPE_GPU = 1
MASK_GPU = 2
U64_MARK = 0xFFFFFFFFFFFFFFFF

# EventDescription flag byte (top byte of colorAndFlags); see dag_daProfilerToken.h.
# The 9 default lock/wait tokens and every DA_PROFILE_WAIT scope set this bit.
DESC_FLAG_IS_WAIT = 1

FORMAT_VERSION = 1

# The CPU/GPU-split bracket scopes (compare_captures.GPU_WAIT_PARENTS): low_latency or
# begin_frame for the begin-frame wait, plus blockedPresent. They are pinned to per-frame
# detail: compare's CPU-vs-GPU attribution needs them with perFrame, so a generic
# rank/threshold cutoff must not demote them to scopesTail.
SPLIT_BRACKETS = ('low_latency', 'begin_frame', 'blockedPresent')


def _san(s):
    """Strip control chars (some names, e.g. the GPU name, embed newlines)."""
    return s.replace('\n', ' ').replace('\r', ' ').replace('\t', ' ').strip()


class Reader:
    """Cursor over a bytes buffer with the daProfiler primitive decoders."""
    __slots__ = ('b', 'o', 'end')

    def __init__(self, b, o=0, end=None):
        self.b = b
        self.o = o
        self.end = len(b) if end is None else end

    def u8(self):
        v = self.b[self.o]; self.o += 1; return v

    def i32(self):
        v = struct.unpack_from('<i', self.b, self.o)[0]; self.o += 4; return v

    def u32(self):
        v = struct.unpack_from('<I', self.b, self.o)[0]; self.o += 4; return v

    def i64(self):
        v = struct.unpack_from('<q', self.b, self.o)[0]; self.o += 8; return v

    def u64(self):
        v = struct.unpack_from('<Q', self.b, self.o)[0]; self.o += 8; return v

    def f32(self):
        v = struct.unpack_from('<f', self.b, self.o)[0]; self.o += 4; return v

    def vlq_u(self):
        b = self.b; o = self.o
        x = b[o]; o += 1; r = x & 0x7F; sh = 7
        while x & 0x80:
            x = b[o]; o += 1; r |= (x & 0x7F) << sh; sh += 7
        self.o = o
        return r

    def vlq_i(self):
        b = self.b; o = self.o
        x = b[o]; o += 1; r = x & 0x3F; neg = x & 0x40; cont = x & 0x80; sh = 6
        while cont:
            x = b[o]; o += 1; cont = x & 0x80; r |= (x & 0x7F) << sh; sh += 7
        self.o = o
        return -r if neg else r

    def sstr(self):
        n = self.vlq_u()
        s = self.b[self.o:self.o + n].decode('utf-8', 'replace')
        self.o += n
        return s


class Cap:
    """Accumulators filled across all segments of one file."""
    def __init__(self):
        self.freq = None
        self.meta = {}
        # Keyed by board thread id, not display name: daProfiler reuses names across
        # threads (worker pools), and merging their timelines corrupts self time.
        self.threads = {}            # tid -> {name, mask, isgpu}
        self.cpu_frames = []         # (start, end) ticks
        self.gpu_frames = []
        # src/isWait travel per event (resolved by description index), not via a
        # name->meta map: two descriptions can share a name but differ in file/flags.
        self.events = defaultdict(list)   # tid -> [(start, end, name, isgpu, src, isWait)]
        self.frame_ms = []           # from SummaryPack (cross-check)
        self.frametags = {}          # frame.start tick -> frameNo
        self.gpu_ds = []             # (time, dp, rt, ps, ins, rp, tri)
        self.samples = []            # (tick, tid, [addrs])
        self.symbols = {}            # addr -> (fun, file, line)
        self.summary_seen = False


def parse_board(r, cap, descs, desc_meta, threads):
    r.i32()                          # board
    freq = r.i64()
    r.i64()                          # origin
    r.i32()                          # precision
    r.i64(); r.i64()                 # timeStart, timeEnd

    def read_threads():
        cnt = r.i32(); out = []
        for _ in range(cnt):
            tid = r.i64(); r.i32()   # pid
            name = _san(r.sstr())
            r.i32(); r.i32()         # maxDepth, priority
            mask = r.i32()
            out.append((name, bool(mask & MASK_GPU) or tid == -1, tid, mask))
        return out

    th = read_threads()
    r.i32()                          # fibers
    main_idx = r.i32()
    while True:
        flags = r.u8()
        if flags == 0xFF:
            break
        name = _san(r.sstr()); fil = r.sstr(); line = r.i32(); r.vlq_u()  # color
        descs.append(name)
        desc_meta.append((("%s:%d" % (fil, line)) if fil else "",
                          bool(flags & DESC_FLAG_IS_WAIT)))
    r.i32(); r.i32()                 # mode, proc
    read_threads()                   # threads saved twice
    r.i32(); r.i32()                 # pid, cores

    threads[:] = th
    for (name, isgpu, tid, mask) in th:
        cap.threads.setdefault(tid, {'name': name, 'mask': mask, 'isgpu': isgpu})
    if cap.freq is None:
        cap.freq = freq
        cap.meta['cpuFreqHz'] = freq
        if 0 <= main_idx < len(th):
            cap.meta['mainThread'] = th[main_idx][0]


def parse_summary(r, cap):
    r.i32()                          # board
    af = r.i32()
    ms = [r.f32() for _ in range(af)]
    cap.frame_ms.extend(ms)
    platform = r.sstr(); exe = r.sstr()
    kv = {}
    while True:
        k = r.sstr()
        if k == '':
            break
        kv[k] = r.sstr()
    if not cap.summary_seen:
        cap.summary_seen = True
        cap.meta['platform'] = platform
        cap.meta['executable'] = exe
        if 'CPU' in kv:
            cap.meta['cpu'] = kv['CPU']
        if 'Dump time' in kv:
            cap.meta['dumpTime'] = kv['Dump time']
        if 'Time passed after launch' in kv:
            cap.meta['timeAfterLaunch'] = kv['Time passed after launch']


def parse_frames(r, cap):
    r.i32()                          # board
    while True:
        typ = r.vlq_i()
        if typ < 0:
            break
        r.vlq_u()                    # frame description
        r.i64()                      # thread id
        lst = cap.cpu_frames if typ == TYPE_CPU else cap.gpu_frames
        while True:
            s = r.u64(); e = r.u64()
            if s == U64_MARK and e == U64_MARK:
                break
            lst.append((s, e))


def parse_eventframe(r, cap, descs, desc_meta, threads):
    r.vlq_u()                        # board
    ti = r.vlq_i()
    r.vlq_i()                        # fiber index
    r.vlq_i()                        # type
    if 0 <= ti < len(threads):
        tname, isgpu, tid = threads[ti][0], threads[ti][1], threads[ti][2]
    else:
        tname, isgpu, tid = '?', False, None
    frame = []
    while True:
        depth = r.vlq_i()
        if depth < 0:
            break
        d = r.vlq_u(); s = r.i64(); e = r.i64()
        if 0 <= d < len(descs):
            nm = descs[d]; src, isw = desc_meta[d]
        else:
            nm = '?'; src, isw = '', False
        frame.append((s, e, nm, isgpu, src, isw))
    # The dump writes the synthetic placeholder span as the sole event of an eventless
    # thread's frame. Flag it here, per segment: a multi-segment .dap merges all segments
    # into one event list, so a thread idle in one segment but active in another would
    # otherwise hide the fake span among real events and count it as real scope time.
    is_ph = len(frame) == 1 and frame[0][2] == tname
    ev = cap.events[tid]
    for fe in frame:
        ev.append(fe + (is_ph,))
    r.i64(); r.i64()                 # trailer start, end


def parse_tags(r, cap):
    r.i32(); r.i32()                 # board, thread index
    while True:                      # frame tags
        tick = r.u64()
        if tick == U64_MARK:
            break
        r.vlq_u()                    # description
        val = r.i32()
        cap.frametags.setdefault(tick, val)
    while True:                      # GPU draw-stat tags
        t = r.u64()
        if t == U64_MARK:
            break
        r.vlq_u()                    # LOCKVIB
        dp = r.vlq_u(); rt = r.vlq_u(); ps = r.vlq_u(); ins = r.vlq_u(); rp = r.vlq_u()
        tri = r.i64()
        cap.gpu_ds.append((t, dp, rt, ps, ins, rp, tri))
    while True:                      # string tags (unused here)
        endt = r.u64()
        if endt == U64_MARK:
            break
        r.vlq_u(); r.sstr()


def parse_callstacks(r, cap):
    r.i32()                          # board
    while True:
        cnt = r.u64()
        if cnt == U64_MARK:
            break
        tick = r.i64(); tid = r.i64()
        addrs = [r.u64() for _ in range(cnt)]
        cap.samples.append((tick, tid, addrs))


def parse_symbols(r, cap):
    r.i32()                          # board
    while True:                      # modules
        base = r.u64()
        if base == 0:
            break
        r.u64(); r.sstr()            # size, name
    while True:                      # symbols
        addr = r.u64()
        if addr == 0:
            break
        line = r.i32()
        if line != -1:
            fun = r.sstr(); fil = r.sstr()
            cap.symbols[addr] = (fun, fil, line)


def parse_segment(raw, cap):
    r = Reader(raw)
    descs = []
    desc_meta = []
    threads = []
    n = len(raw)
    while r.o + 12 <= n:
        r.u32()                      # response version
        size = r.u32()
        typ = struct.unpack_from('<H', raw, r.o)[0]
        app = struct.unpack_from('<H', raw, r.o + 2)[0]
        r.o += 4
        body_start = r.o
        body_end = body_start + size
        if app != APP or body_end > n:
            break
        sub = Reader(raw, body_start, body_end)
        if typ == T_FrameDescBoard:
            parse_board(sub, cap, descs, desc_meta, threads)
        elif typ == T_SummaryPack:
            parse_summary(sub, cap)
        elif typ == T_FramesPack:
            parse_frames(sub, cap)
        elif typ == T_EventFrame:
            parse_eventframe(sub, cap, descs, desc_meta, threads)
        elif typ == T_TagsPack:
            parse_tags(sub, cap)
        elif typ == T_UniqueName:
            cap.meta['uniqueName'] = sub.sstr()
        elif typ == T_CallstackPack:
            parse_callstacks(sub, cap)
        elif typ == T_CallstackDescBoard:
            parse_symbols(sub, cap)
        r.o = body_end


def read_file(path, cap):
    data = open(path, 'rb').read()
    p = 0
    n = len(data)
    while p + 8 <= n:
        magic, version, flags = struct.unpack_from('<IHH', data, p)
        if magic != MAGIC:
            break
        p += 8
        if flags & FLAG_ZLIB:
            d = zlib.decompressobj()
            raw = d.decompress(data[p:])
            raw += d.flush()
            used = (n - p) - len(d.unused_data)
            parse_segment(raw, cap)
            p += used
        else:
            parse_segment(data[p:], cap)
            break
    cap.meta['source'] = os.path.basename(path)


def _innermost_scope(intervals, starts, tick):
    """Deepest event whose [start,end) contains tick."""
    i = bisect.bisect_right(starts, tick) - 1
    while i >= 0:
        s, e, nm = intervals[i]
        if e > tick:
            return nm
        i -= 1
    return None


def build_samples(cap, intervals_by_thread):
    by_scope = {}
    for (tick, tid, addrs) in cap.samples:
        # Samples keep the live OS tid; once a thread is removed the board stores its
        # id as ~tid (tids get recycled), so fall back to that for removed threads.
        iv = intervals_by_thread.get(tid)
        if iv is None:
            iv = intervals_by_thread.get(~tid)
        if not iv:
            continue
        scope = _innermost_scope(iv[0], iv[1], tick)
        if scope is None:
            continue
        leaf = addrs[0] if addrs else 0      # innermost frame (reversed stack)
        sym = cap.symbols.get(leaf)
        # (func, src) tuple key, not a "func|src" string: demangled C++ names contain '|'
        # (operator|, operator|=), which a delimiter split would truncate.
        fkey = (sym[0], "%s:%d" % (sym[1], sym[2])) if sym else ("0x%x" % leaf, "")
        d = by_scope.setdefault(scope, {})
        d[fkey] = d.get(fkey, 0) + 1
    out = {}
    for scope, funcs in by_scope.items():
        total = sum(funcs.values())
        top = sorted(funcs.items(), key=lambda kv: -kv[1])[:12]
        out[scope] = {
            'totalSamples': total,
            'topFunctions': [
                {'func': k[0], 'src': k[1],
                 'samples': v, 'pct': round(100.0 * v / total, 1)}
                for k, v in top
            ],
        }
    return out


def finalize(cap, top_scopes, min_incl_ms, want_perframe, scope_filter):
    freq = cap.freq or 1
    to_us = 1e6 / freq
    cpu_frames = sorted(cap.cpu_frames)
    frame_count = len(cpu_frames)
    if frame_count == 0:
        raise SystemExit("no CPU frames found in capture (corrupt or empty .dap)")
    cpu_starts = [f[0] for f in cpu_frames]
    cmax = frame_count - 1

    def bucket(x):
        i = bisect.bisect_right(cpu_starts, x) - 1
        if i < 0:
            return 0
        return cmax if i > cmax else i

    scopes = {}

    # Scopes are keyed by (thread display name, scope name) on purpose, so two threads
    # that share a display name (anonymous worker pools) aggregate into one scope and
    # one src/isWait slot. compare_captures matches scopes across two runs by this key;
    # identically-named threads have no identity stable across runs (tids differ,
    # creation order is not fixed), so keying by tid would pair arbitrary workers
    # between runs and manufacture spurious diffs. The merge gives the comparable
    # per-pool aggregate. Self time is still correct: each thread's interval stack is
    # reconstructed separately (events are keyed by board tid) before this aggregation.
    def get(tname, nm, isgpu, idle):
        key = (tname, nm, idle)
        s = scopes.get(key)
        if s is None:
            s = {'incl': [0] * frame_count, 'self': [0] * frame_count,
                 'calls': [0] * frame_count, 'isgpu': isgpu, 'thread': tname, 'name': nm,
                 'src': '', 'isWait': False, 'idle': idle}
            scopes[key] = s
        return s

    keep_intervals = bool(cap.samples)
    intervals_by_thread = {}
    gpu_ticks = [0] * frame_count
    gpu_root_starts = set()      # start ticks of depth-0 GPU events (carry frame totals)
    has_gpu = False

    # GPU events are saved in CPU ticks, so every thread buckets on the one CPU-frame axis.
    # A placeholder event (flagged is_ph at parse, per segment) is the synthetic span of
    # an eventless thread. It is not dropped but routed to a separate idle scope record
    # (keyed apart in get()), so it is marked rather than lost yet never mixes its fake
    # time into a real scope -- even when the same thread is idle in one segment and busy
    # in another. The idle record is summary-only and compare_captures skips it.
    for tid, evlist in cap.events.items():
        display = cap.threads.get(tid, {}).get('name', '?')
        evlist.sort(key=lambda e: (e[0], -e[1]))
        if keep_intervals:           # placeholder spans are not real work -> no samples
            iv = [(s, e, nm) for (s, e, nm, _g, _src, _w, _ph) in evlist if not _ph]
            intervals_by_thread[tid] = (iv, [x[0] for x in iv])
        stack = []   # [end, childsum, name, isgpu, start, dur, src, isWait, is_ph]

        def flush(node, _disp=display):
            e, childsum, nm, isgpu, s, dur, src, isw, is_ph = node
            self_ticks = dur - childsum
            if self_ticks < 0:
                self_ticks = 0
            sc = get(_disp, nm, isgpu, is_ph)
            if src and not sc['src']:    # keyed by the contributing description, not name
                sc['src'] = src
            if isw:
                sc['isWait'] = True
            fi = bucket(s)
            sc['incl'][fi] += dur
            sc['self'][fi] += self_ticks
            sc['calls'][fi] += 1

        for (s, e, nm, isgpu, src, isw, is_ph) in evlist:
            dur = e - s
            if dur < 0:
                dur = 0
            while stack and stack[-1][0] <= s:
                flush(stack.pop())
            if isgpu:
                has_gpu = True
                if not stack:        # depth-0 GPU event spans that frame's GPU work
                    gpu_ticks[bucket(s)] += dur
                    gpu_root_starts.add(s)
            if stack:
                stack[-1][1] += dur
            stack.append([e, 0, nm, isgpu, s, dur, src, isw, is_ph])
        while stack:
            flush(stack.pop())

    # overall per-frame series
    cpu_us = [int(round((e - s) * to_us)) for (s, e) in cpu_frames]
    frame_no = [cap.frametags.get(s) for (s, e) in cpu_frames]
    frames = {'cpuUs': cpu_us}
    if any(v is not None for v in frame_no):
        frames['frameNo'] = frame_no
    if has_gpu:
        frames['gpuUs'] = [int(round(v * to_us)) for v in gpu_ticks]

    # GPU draw stats: each event's ds is a per-event delta, so a nested scope's
    # counts are already included in its ancestors. Only depth-0 (root) events span
    # a whole frame, and their counts are the frame totals; summing all depths would
    # multiply-count. ds-tag time equals the event start exactly (same from2to1 +
    # counter), so root tags are the ones whose time is a depth-0 event start.
    gpu_out = None
    if has_gpu and cap.gpu_ds:
        dp = [0] * frame_count; rt = [0] * frame_count; ps = [0] * frame_count
        ins = [0] * frame_count; rp = [0] * frame_count; tri = [0] * frame_count
        for (t, a, b, c, d, e_, f) in cap.gpu_ds:
            if t not in gpu_root_starts:
                continue
            fi = bucket(t)
            dp[fi] += a; rt[fi] += b; ps[fi] += c; ins[fi] += d; rp[fi] += e_; tri[fi] += f
        gpu_out = {'drawPrims': dp, 'renderTargets': rt, 'programs': ps,
                   'instructions': ins, 'renderPasses': rp, 'tris': tri}

    # Apply --scope before ranking, so a matching scope outside the global top-N is
    # ranked among the kept scopes and still gets per-frame detail (the point of a
    # drill-down). Idle placeholders are excluded from the ranking (their full-capture
    # span would otherwise top it and steal detail slots) and always emitted summary-only.
    def kept(s):
        return scope_filter is None or scope_filter.lower() in s['name'].lower()
    real = sorted((s for s in scopes.values() if not s['idle'] and kept(s)),
                  key=lambda s: -sum(s['incl']))
    idle = [s for s in scopes.values() if s['idle'] and kept(s)]
    out_scopes = []
    out_tail = []

    def emit(s, rank):
        incl_us = [int(round(v * to_us)) for v in s['incl']]
        self_us = [int(round(v * to_us)) for v in s['self']]
        calls = s['calls']
        incl_total = sum(incl_us)
        rec = {
            'name': s['name'],
            'src': s['src'],
            'thread': s['thread'],
            'type': 'gpu' if s['isgpu'] else 'cpu',
            'callsTotal': sum(calls),
            'inclTotalUs': incl_total,
            'selfTotalUs': sum(self_us),
        }
        if s['isWait']:
            rec['isWait'] = True
        if s['idle']:
            rec['idle'] = True
        # pin the CPU/GPU-split brackets to detailed output, else the direct-.dap
        # attribution goes dark when they fall below the generic rank/threshold cutoff.
        detail = (not s['idle']) and want_perframe and (
            s['name'] in SPLIT_BRACKETS
            or (rank < top_scopes and (incl_total / 1e3) >= min_incl_ms))
        if detail:
            rec['perFrame'] = {'calls': calls, 'inclUs': incl_us, 'selfUs': self_us}
            out_scopes.append(rec)
        else:
            out_tail.append(rec)

    for rank, s in enumerate(real):
        emit(s, rank)
    for s in idle:
        emit(s, top_scopes)   # a rank that never qualifies for detail

    samples_out = None
    if cap.samples:
        try:
            by_scope = build_samples(cap, intervals_by_thread)
            if scope_filter is not None:   # --scope filters scopes; keep samples in step
                flt = scope_filter.lower()
                by_scope = {k: v for k, v in by_scope.items() if flt in k.lower()}
            samples_out = {'byScope': by_scope}
        except Exception as ex:    # sampling path is best-effort
            sys.stderr.write("warning: sample attribution failed: %s\n" % ex)

    meta = dict(cap.meta)
    meta['frameCount'] = frame_count
    meta['hasGpu'] = has_gpu
    meta['hasSamples'] = samples_out is not None
    if cap.frame_ms:
        meta['summaryMeanMs'] = round(sum(cap.frame_ms) / len(cap.frame_ms), 3)

    doc = {
        'format': 'daprofiler-capture',
        'formatVersion': FORMAT_VERSION,
        'timeUnit': 'us',
        'meta': meta,
        'threads': [{'name': i['name'], 'tid': tid, 'mask': i['mask']}
                    for tid, i in cap.threads.items()],
        'frames': frames,
        'scopes': out_scopes,
        'scopesTail': out_tail,
    }
    if gpu_out is not None:
        doc['gpu'] = gpu_out
    if samples_out is not None:
        doc['samples'] = samples_out
    return doc


def convert_file(path, top_scopes=1024, min_incl_ms=0.0, want_perframe=True,
                 scope_filter=None):
    cap = Cap()
    read_file(path, cap)
    return finalize(cap, top_scopes, min_incl_ms, want_perframe, scope_filter)


def main(argv=None):
    ap = argparse.ArgumentParser(description="Convert a daProfiler .dap to compact JSON.")
    ap.add_argument('input', help="source .dap file")
    ap.add_argument('output', nargs='?', help="dest .json (default: <input>.json)")
    ap.add_argument('--top-scopes', type=int, default=1024,
                    help="emit per-frame detail for the N costliest scopes (default 1024)")
    ap.add_argument('--min-incl-ms', type=float, default=0.0,
                    help="per-frame detail only for scopes whose total inclusive time >= this (ms)")
    ap.add_argument('--no-perframe', action='store_true',
                    help="omit per-frame arrays entirely (summary only; smallest)")
    ap.add_argument('--scope', default=None,
                    help="keep only scopes whose name contains this substring (drill-down)")
    ap.add_argument('--compact', action='store_true', help="no pretty indentation")
    args = ap.parse_args(argv)

    doc = convert_file(args.input, args.top_scopes, args.min_incl_ms,
                       not args.no_perframe, args.scope)
    out = args.output or (args.input + '.json')
    sep = (',', ':') if args.compact else (',', ': ')
    with open(out, 'w', encoding='utf-8') as f:
        json.dump(doc, f, separators=sep, indent=(None if args.compact else 1))
    m = doc['meta']
    sys.stderr.write("wrote %s  frames=%d scopes=%d(+%d tail) gpu=%s samples=%s  %.0f KB\n"
                     % (out, m['frameCount'], len(doc['scopes']), len(doc['scopesTail']),
                        m['hasGpu'], m['hasSamples'], os.path.getsize(out) / 1024.0))
    return 0


if __name__ == '__main__':
    sys.exit(main())
