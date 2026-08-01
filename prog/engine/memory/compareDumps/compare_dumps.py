#!/usr/bin/env python3
# Copyright (C) Gaijin Games KFT.  All rights reserved.
"""Compare two Dagor memory flame-graph dumps: where allocated bytes and live
allocation (pointer) counts grew and where they were saved.

Inputs are saved Memory Flame Graph pages (.html with the embedded JSON tree;
produced by the /memory_map webui page, see
webui/plugins/dagor/memmap.cpp) or the raw JSON tree itself.

Convention: the first argument is OLD (the baseline), the second is NEW (the
change under test). A positive delta means NEW allocates more.

Pure stdlib, Python 3.6+.
"""

import argparse
import json
import re
import sys

# ------------------------------- input --------------------------------------

EMBED_TAG_RE = re.compile(r'<script[^>]*\bid=["\']embeddedData["\'][^>]*>')


def load_tree(path):
    with open(path, 'r', encoding='utf-8', errors='replace') as f:
        text = f.read()
    if text.lstrip()[:1] == '{':
        return parse_json_tree(text)
    m = EMBED_TAG_RE.search(text)
    if not m:
        raise SystemExit(path + ': no embedded memory tree found (expected a saved '
                                'Memory Flame Graph .html or the raw .json tree)')
    end = text.find('</script', m.end())
    if end < 0:
        raise SystemExit(path + ': unterminated embeddedData script tag')
    return parse_json_tree(text[m.end():end])


def parse_json_tree(text):
    try:
        return json.loads(text)
    except RecursionError:
        return _json_loads_iterative(text)


def _json_loads_iterative(s):
    # stdlib json recurses once per nesting level and very deep dumps
    # (recursive script stacks) overflow it; this parser keeps its own stack
    # for containers and defers scalars to the public raw_decode API.
    scalar = json.JSONDecoder().raw_decode
    ws = ' \t\r\n'
    n = len(s)
    root = []
    stack = [root]  # open containers, innermost last
    keys = [None]   # pending dict key per open container

    def put(v):
        c = stack[-1]
        if type(c) is list:
            c.append(v)
        else:
            c[keys[-1]] = v

    def read_key(i):
        while s[i] in ws:
            i += 1
        if s[i] != '"':
            raise ValueError('expected key at offset %d' % i)
        k, i = scalar(s, i)
        keys[-1] = k
        while s[i] in ws:
            i += 1
        if s[i] != ':':
            raise ValueError('expected ":" at offset %d' % i)
        return i + 1

    i = 0
    expect_value = True
    while True:
        while i < n and s[i] in ws:
            i += 1
        if i >= n:
            break
        c = s[i]
        if expect_value:
            if c == '{':
                d = {}
                put(d)
                stack.append(d)
                keys.append(None)
                i += 1
                while s[i] in ws:
                    i += 1
                if s[i] == '}':
                    stack.pop()
                    keys.pop()
                    i += 1
                    expect_value = False
                else:
                    i = read_key(i)
            elif c == '[':
                v = []
                put(v)
                stack.append(v)
                keys.append(None)
                i += 1
                while s[i] in ws:
                    i += 1
                if s[i] == ']':
                    stack.pop()
                    keys.pop()
                    i += 1
                    expect_value = False
            else:
                v, i = scalar(s, i)  # strings, numbers, true/false/null
                put(v)
                expect_value = False
        else:
            if c == ',':
                i += 1
                if type(stack[-1]) is dict:
                    i = read_key(i)
                expect_value = True
            elif c in '}]':
                if len(stack) <= 1:
                    raise ValueError('unbalanced JSON at offset %d' % i)
                stack.pop()
                keys.pop()
                i += 1
            else:
                raise ValueError('bad JSON at offset %d' % i)
    if len(stack) != 1 or len(root) != 1:
        raise ValueError('unbalanced JSON')
    return root[0]


# ---------------------------- aggregation -----------------------------------

# symbolized frames end in "(713)  +46" (dbghelp: line + offset) or " + 0x1a2b"
# (unix dladdr: module offset, possibly with a doubled 0x from "0x%p"); both
# shift between builds, so strip them by default
SYM_TAIL_RE = re.compile(r'\s*(?:\([0-9]+\))?\s*\+\s*(?:0[xX])*[0-9a-fA-F]+\s*$')


def normalize_name(name):
    return SYM_TAIL_RE.sub('', name)


class FrameTable(object):
    """Interns (parent_id, name) pairs. Shared by both dumps so equal stacks
    get equal site ids and can be diffed directly."""

    def __init__(self):
        self.ids = {}
        self.parent = []
        self.name = []

    def intern(self, parent, name):
        key = (parent, name)
        fid = self.ids.get(key)
        if fid is None:
            fid = len(self.name)
            self.ids[key] = fid
            self.parent.append(parent)
            self.name.append(name)
        return fid

    def stack(self, fid):
        out = []
        while fid >= 0:
            out.append(self.name[fid])
            fid = self.parent[fid]
        out.reverse()
        if len(out) > 1 and out[0].upper() == 'ROOT':
            out = out[1:]
        return out


def collect_sites(tree, frames, raw_names):
    """-> ({site_id: [self_bytes, self_ptrs]}, [total_bytes, total_ptrs])"""
    sites = {}
    totals = [0, 0]
    todo = [(tree, -1)]
    while todo:
        node, parent = todo.pop()
        name = node.get('n') or node.get('uid') or '?'
        if not raw_names:
            name = normalize_name(name)
        fid = frames.intern(parent, sys.intern(name))
        sa = int(node.get('sa') or 0)
        sp = int(node.get('sp') or 0)
        if sa or sp:
            e = sites.get(fid)
            if e is None:
                sites[fid] = [sa, sp]
            else:
                e[0] += sa
                e[1] += sp
            totals[0] += sa
            totals[1] += sp
        children = node.get('children')
        if children:
            for c in children:
                todo.append((c, fid))
    return sites, totals


def build_rows(old_sites, new_sites, frames, group_by_frame):
    """One row per compared unit (frame name or exact stack), with old/new
    self bytes and pointer counts. In frame mode 'fid' is the site with the
    largest contribution, kept for caller-context display."""
    rows = []
    union = set(old_sites)
    union.update(new_sites)
    if group_by_frame:
        groups = {}
        for fid in union:
            ob, op = old_sites.get(fid, (0, 0))
            nb, np_ = new_sites.get(fid, (0, 0))
            score = (abs(nb - ob), abs(np_ - op))
            e = groups.get(frames.name[fid])
            if e is None:
                groups[frames.name[fid]] = [ob, nb, op, np_, fid, score]
            else:
                e[0] += ob
                e[1] += nb
                e[2] += op
                e[3] += np_
                if score > e[5]:
                    e[4] = fid
                    e[5] = score
        for name, (ob, nb, op, np_, dom, _score) in groups.items():
            rows.append({'name': name, 'fid': dom,
                         'ob': ob, 'nb': nb, 'op': op, 'np': np_})
    else:
        for fid in union:
            ob, op = old_sites.get(fid, (0, 0))
            nb, np_ = new_sites.get(fid, (0, 0))
            rows.append({'name': frames.name[fid], 'fid': fid,
                         'ob': ob, 'nb': nb, 'op': op, 'np': np_})
    for r in rows:
        r['db'] = r['nb'] - r['ob']
        r['dp'] = r['np'] - r['op']
        if r['ob'] == 0 and r['op'] == 0:
            r['status'] = 'new'
        elif r['nb'] == 0 and r['np'] == 0:
            r['status'] = 'gone'
        else:
            r['status'] = 'changed'
    return rows


SUSPECT_BYTES = 1 << 31


def find_suspects(sites, frames, label):
    out = []
    for fid, (b, p) in sites.items():
        if b >= SUSPECT_BYTES:
            out.append({'dump': label, 'frame': frames.name[fid],
                        'stack': frames.stack(fid), 'bytes': b, 'ptrs': p})
    out.sort(key=lambda r: -r['bytes'])
    return out


# ---------------------------- formatting ------------------------------------

def fmt_bytes(v, sign=False):
    a = abs(v)
    for unit, div in (('GB', 1 << 30), ('MB', 1 << 20), ('KB', 1 << 10)):
        if a >= div:
            return ('%+.2f %s' if sign else '%.2f %s') % (v / float(div), unit)
    return ('%+d B' if sign else '%d B') % v


def fmt_int(v, sign=False):
    return format(v, '+,d' if sign else ',d')


def fmt_pct(delta, base):
    if base <= 0:
        return 'n/a'
    return '%+.2f%%' % (100.0 * delta / base)


def md_escape(s, maxlen):
    s = s.replace('|', '\\|')
    if len(s) > maxlen:
        s = s[:maxlen - 3] + '...'
    return s


def caller_chain(frames, fid, k):
    if k <= 0:
        return ''
    callers = frames.stack(fid)[:-1]
    callers.reverse()
    chain = ' <- '.join(callers[:k])
    if len(callers) > k:
        chain += ' <- ...'
    return chain


def md_table(lines, header, aligns, rows):
    lines.append('| ' + ' | '.join(header) + ' |')
    lines.append('|' + '|'.join('---:' if a == 'r' else ':---' for a in aligns) + '|')
    for r in rows:
        lines.append('| ' + ' | '.join(r) + ' |')


def row_name_md(r):
    name = md_escape(r['name'], 72)
    if r['status'] != 'changed':
        name += ' **[%s]**' % r['status']
    return name


def render_md(ctx):
    a = ctx['args']
    L = []
    L.append('# Memory dump comparison')
    L.append('')
    L.append('- OLD: `%s` -- %s (%s B), %s pointers' %
             (a.old, fmt_bytes(ctx['old_total'][0]), fmt_int(ctx['old_total'][0]),
              fmt_int(ctx['old_total'][1])))
    L.append('- NEW: `%s` -- %s (%s B), %s pointers' %
             (a.new, fmt_bytes(ctx['new_total'][0]), fmt_int(ctx['new_total'][0]),
              fmt_int(ctx['new_total'][1])))
    L.append('- grouping: by %s%s; thresholds: %s / %d ptrs; top %d rows shown per section'
             % (a.group_by, '' if a.raw_names else ' (line/offset stripped from names)',
                fmt_bytes(a.min_bytes), a.min_ptrs, a.top))
    L.append('')
    L.append('## Totals')
    L.append('')
    db = ctx['new_total'][0] - ctx['old_total'][0]
    dp = ctx['new_total'][1] - ctx['old_total'][1]
    md_table(L, ['metric', 'OLD', 'NEW', 'delta', 'rel'], 'lrrrr', [
        ['allocated bytes', fmt_bytes(ctx['old_total'][0]), fmt_bytes(ctx['new_total'][0]),
         fmt_bytes(db, sign=True), fmt_pct(db, ctx['old_total'][0])],
        ['live pointers', fmt_int(ctx['old_total'][1]), fmt_int(ctx['new_total'][1]),
         fmt_int(dp, sign=True), fmt_pct(dp, ctx['old_total'][1])],
    ])
    L.append('')
    bt = ctx['below_threshold']
    L.append('Net bytes %s = grown %s over %d %ss + saved %s over %d %ss '
             '+ below-threshold remainder %s over %d %ss.' %
             (fmt_bytes(db, sign=True),
              fmt_bytes(sum(r['db'] for r in ctx['grown']), sign=True), len(ctx['grown']), a.group_by,
              fmt_bytes(sum(r['db'] for r in ctx['saved']), sign=True), len(ctx['saved']), a.group_by,
              fmt_bytes(bt['bytes'], sign=True), bt['byteSites'], a.group_by))
    L.append('Net pointers %s = grown %s over %d %ss + saved %s over %d %ss '
             '+ below-threshold remainder %s over %d %ss.' %
             (fmt_int(dp, sign=True),
              fmt_int(sum(r['dp'] for r in ctx['ptrs_grown']), sign=True), len(ctx['ptrs_grown']), a.group_by,
              fmt_int(sum(r['dp'] for r in ctx['ptrs_saved']), sign=True), len(ctx['ptrs_saved']), a.group_by,
              fmt_int(bt['pointers'], sign=True), bt['ptrSites'], a.group_by))

    if ctx['suspects']:
        L.append('')
        L.append('## Suspicious single-site values')
        L.append('')
        L.append('Single call sites accounting >= 2 GB. A value just below a multiple of')
        L.append('4 GB usually means a wrapped 32-bit size counter that inflates that')
        L.append("dump's total; verify before trusting the totals above.")
        L.append('')
        md_table(L, ['dump', 'self bytes', 'raw', 'ptrs', 'frame'], 'lrrrl',
                 [[s['dump'], fmt_bytes(s['bytes']), fmt_int(s['bytes']),
                   fmt_int(s['ptrs']), md_escape(s['frame'], 72)]
                  for s in ctx['suspects'][:20]])

    def byte_section(title, rows):
        L.append('')
        L.append('## ' + title)
        L.append('')
        if not rows:
            L.append('(none above threshold)')
            return
        md_table(L, ['delta', 'old', 'new', 'd ptrs', a.group_by, 'called from'], 'rrrrll',
                 [[fmt_bytes(r['db'], sign=True), fmt_bytes(r['ob']), fmt_bytes(r['nb']),
                   fmt_int(r['dp'], sign=True), row_name_md(r),
                   md_escape(caller_chain(ctx['frames'], r['fid'], a.callers), 140)]
                  for r in rows[:a.top]])
        if len(rows) > a.top:
            L.append('')
            L.append('... and %d more above threshold (raise --top or use --json-out for the full list).'
                     % (len(rows) - a.top))

    def ptr_section(title, rows):
        L.append('')
        L.append('## ' + title)
        L.append('')
        if not rows:
            L.append('(none above threshold)')
            return
        md_table(L, ['d ptrs', 'old ptrs', 'new ptrs', 'd bytes', a.group_by, 'called from'], 'rrrrll',
                 [[fmt_int(r['dp'], sign=True), fmt_int(r['op']), fmt_int(r['np']),
                   fmt_bytes(r['db'], sign=True), row_name_md(r),
                   md_escape(caller_chain(ctx['frames'], r['fid'], a.callers), 140)]
                  for r in rows[:a.top]])
        if len(rows) > a.top:
            L.append('')
            L.append('... and %d more above threshold (raise --top or use --json-out for the full list).'
                     % (len(rows) - a.top))

    byte_section('Memory grown (NEW allocates more)', ctx['grown'])
    byte_section('Memory saved (NEW allocates less)', ctx['saved'])
    ptr_section('Pointer count grown', ctx['ptrs_grown'])
    ptr_section('Pointer count saved', ctx['ptrs_saved'])
    L.append('')
    return '\n'.join(L)


def row_to_json(r, frames):
    return {
        'frame': r['name'],
        'status': r['status'],
        'oldBytes': r['ob'], 'newBytes': r['nb'], 'deltaBytes': r['db'],
        'oldPtrs': r['op'], 'newPtrs': r['np'], 'deltaPtrs': r['dp'],
        'stack': frames.stack(r['fid']),
    }


def render_json(ctx):
    a = ctx['args']
    frames = ctx['frames']
    db = ctx['new_total'][0] - ctx['old_total'][0]
    dp = ctx['new_total'][1] - ctx['old_total'][1]

    def pct(delta, base):
        return round(100.0 * delta / base, 4) if base > 0 else None

    out = {
        'old': {'file': a.old, 'bytes': ctx['old_total'][0], 'pointers': ctx['old_total'][1]},
        'new': {'file': a.new, 'bytes': ctx['new_total'][0], 'pointers': ctx['new_total'][1]},
        'delta': {'bytes': db, 'bytesPct': pct(db, ctx['old_total'][0]),
                  'pointers': dp, 'pointersPct': pct(dp, ctx['old_total'][1])},
        'groupBy': a.group_by,
        'normalizedNames': not a.raw_names,
        'thresholds': {'minBytes': a.min_bytes, 'minPtrs': a.min_ptrs},
        'bytesGrown': [row_to_json(r, frames) for r in ctx['grown']],
        'bytesSaved': [row_to_json(r, frames) for r in ctx['saved']],
        'ptrsGrown': [row_to_json(r, frames) for r in ctx['ptrs_grown']],
        'ptrsSaved': [row_to_json(r, frames) for r in ctx['ptrs_saved']],
        'belowThreshold': ctx['below_threshold'],
        'suspects': ctx['suspects'],
    }
    return json.dumps(out, indent=2)


# ------------------------------- main ---------------------------------------

def parse_size(s):
    m = re.match(r'^([0-9]+(?:\.[0-9]+)?)\s*([kKmMgG]?)[bB]?$', s.strip())
    if not m:
        raise argparse.ArgumentTypeError('bad size %r (use e.g. 4096, 64k, 2m, 1g)' % s)
    mul = {'': 1, 'k': 1 << 10, 'm': 1 << 20, 'g': 1 << 30}[m.group(2).lower()]
    return int(float(m.group(1)) * mul)


def main(argv=None):
    ap = argparse.ArgumentParser(
        description='Compare two Dagor memory flame dumps (saved Memory Flame Graph '
                    '.html or raw .json tree). First argument is OLD (baseline), '
                    'second is NEW; positive deltas mean NEW allocates more.')
    ap.add_argument('old', help='baseline dump (.html or .json)')
    ap.add_argument('new', help='dump under test (.html or .json)')
    ap.add_argument('--group-by', choices=('frame', 'stack'), default='frame',
                    help='aggregate self-allocations per frame name (default) or per '
                         'exact call stack')
    ap.add_argument('--top', type=int, default=40,
                    help='rows per section in the markdown report (default 40)')
    ap.add_argument('--min-bytes', type=parse_size, default=16 << 10, metavar='N',
                    help='minimum |byte delta| to report a row (default 16k; suffixes k/m/g)')
    ap.add_argument('--min-ptrs', type=int, default=16, metavar='N',
                    help='minimum |pointer-count delta| to report a row (default 16)')
    ap.add_argument('--callers', type=int, default=3, metavar='N',
                    help='caller frames of context shown per row (default 3)')
    ap.add_argument('--raw-names', action='store_true',
                    help='keep "(line) +offset" in frame names; by default they are '
                         'stripped so dumps from different builds still match')
    ap.add_argument('--format', choices=('md', 'json', 'both'), default='md',
                    help='what to write to stdout (default md)')
    ap.add_argument('--out', metavar='FILE',
                    help='write the markdown report here instead of stdout (always '
                         'markdown regardless of --format; use --json-out for JSON)')
    ap.add_argument('--json-out', metavar='FILE', help='also write the machine-readable JSON here')
    args = ap.parse_args(argv)
    args.min_bytes = max(1, args.min_bytes)
    args.min_ptrs = max(1, args.min_ptrs)

    frames = FrameTable()
    old_sites, old_total = collect_sites(load_tree(args.old), frames, args.raw_names)
    new_sites, new_total = collect_sites(load_tree(args.new), frames, args.raw_names)

    rows = build_rows(old_sites, new_sites, frames, args.group_by == 'frame')
    grown = sorted((r for r in rows if r['db'] >= args.min_bytes), key=lambda r: -r['db'])
    saved = sorted((r for r in rows if r['db'] <= -args.min_bytes), key=lambda r: r['db'])
    ptrs_grown = sorted((r for r in rows if r['dp'] >= args.min_ptrs), key=lambda r: -r['dp'])
    ptrs_saved = sorted((r for r in rows if r['dp'] <= -args.min_ptrs), key=lambda r: r['dp'])

    reported_b = sum(r['db'] for r in grown) + sum(r['db'] for r in saved)
    reported_p = sum(r['dp'] for r in ptrs_grown) + sum(r['dp'] for r in ptrs_saved)
    below = {
        'bytes': (new_total[0] - old_total[0]) - reported_b,
        'byteSites': sum(1 for r in rows if r['db'] and abs(r['db']) < args.min_bytes),
        'pointers': (new_total[1] - old_total[1]) - reported_p,
        'ptrSites': sum(1 for r in rows if r['dp'] and abs(r['dp']) < args.min_ptrs),
    }
    suspects = find_suspects(old_sites, frames, 'OLD') + find_suspects(new_sites, frames, 'NEW')

    ctx = {
        'args': args, 'frames': frames,
        'old_total': old_total, 'new_total': new_total,
        'grown': grown, 'saved': saved,
        'ptrs_grown': ptrs_grown, 'ptrs_saved': ptrs_saved,
        'below_threshold': below, 'suspects': suspects,
    }

    if args.out or args.format in ('md', 'both'):
        report = render_md(ctx)
        if args.out:
            with open(args.out, 'w', encoding='utf-8') as f:
                f.write(report)
        if args.format in ('md', 'both') and not args.out:
            print(report)
    if args.json_out or args.format in ('json', 'both'):
        js = render_json(ctx)
        if args.json_out:
            with open(args.json_out, 'w', encoding='utf-8') as f:
                f.write(js + '\n')
        if args.format in ('json', 'both') and not args.json_out:
            print(js)

    db = new_total[0] - old_total[0]
    dp = new_total[1] - old_total[1]
    verdict = ('NET bytes %s (%s), pointers %s (%s); grown %d %ss %s, saved %d %ss %s'
               % (fmt_bytes(db, sign=True), fmt_pct(db, old_total[0]),
                  fmt_int(dp, sign=True), fmt_pct(dp, old_total[1]),
                  len(grown), args.group_by, fmt_bytes(sum(r['db'] for r in grown), sign=True),
                  len(saved), args.group_by, fmt_bytes(sum(r['db'] for r in saved), sign=True)))
    if suspects:
        verdict += '; WARNING: %d suspect >=2GB single site(s), totals may be bogus' % len(suspects)
    print(verdict, file=sys.stderr)
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except BrokenPipeError:
        sys.exit(0)
