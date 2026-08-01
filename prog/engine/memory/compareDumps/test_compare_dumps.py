#!/usr/bin/env python3
# Copyright (C) Gaijin Games KFT.  All rights reserved.
"""Self-tests for compare_dumps.py, mainly the iterative JSON fallback that
only runs when stdlib json.loads overflows on a deeply nested dump.
Run: python test_compare_dumps.py
"""

import json
import sys
import unittest
from unittest import mock

import compare_dumps as cd

# deep enough to stress the iterative parser; whether stdlib json.loads
# overflows at this depth varies by CPython version (C recursion budget),
# so no test may depend on it raising
DEEP = sys.getrecursionlimit() * 2


def deep_dump_text(depth):
    # same shape as a /memory_map tree: one child chain, self values per node
    parts = []
    for d in range(depth):
        parts.append('{"n":"f%d","sa":%d,"sp":1,"children":[' % (d, d))
    parts.append('{"n":"leaf","sa":7,"sp":2}')
    parts.append(']}' * depth)
    return ''.join(parts)


class TestIterativeJson(unittest.TestCase):
    def test_matches_stdlib_on_shallow_docs(self):
        docs = [
            '{}', '[]', '""', '0', '-0', '42', '-17', '3.5', '-2.5e-3', '1e10',
            'true', 'false', 'null',
            '{"a": 1, "b": [2, 3.5, "x"], "c": {"d": null, "e": [true, false]}}',
            '[1, [2, [3, [4, {}]]], {"k": []}]',
            r'"esc \" \\ \/ \b \f \n \r \t \u0416 end"',
            ' \t\r\n [ 1 , { " k " : " v " } ] \n ',
            '{"dup": 1, "dup": 2}',
        ]
        for doc in docs:
            self.assertEqual(cd._json_loads_iterative(doc), json.loads(doc), doc)

    def test_rejects_malformed(self):
        docs = ['', '[1,]', '{"a"}', '{"a" 1}', '{1: 2}', '[1 2]', '[[1]',
                '[1]]', '1 2', 'nul', '{,}']
        for doc in docs:
            with self.assertRaises(ValueError, msg=doc):
                cd._json_loads_iterative(doc)

    def test_deep_object_chain(self):
        text = deep_dump_text(DEEP)
        node = cd._json_loads_iterative(text)
        d = 0
        while 'children' in node:
            self.assertEqual(node['n'], 'f%d' % d)
            self.assertEqual(node['sa'], d)
            node = node['children'][0]
            d += 1
        self.assertEqual(d, DEEP)
        self.assertEqual(node, {'n': 'leaf', 'sa': 7, 'sp': 2})

    def test_deep_array(self):
        v = cd._json_loads_iterative('[' * DEEP + '9' + ']' * DEEP)
        d = 0
        while isinstance(v, list):
            self.assertEqual(len(v), 1)
            v = v[0]
            d += 1
        self.assertEqual(d, DEEP)
        self.assertEqual(v, 9)

    def test_parse_json_tree_falls_back(self):
        text = deep_dump_text(3)
        with mock.patch.object(cd.json, 'loads', side_effect=RecursionError):
            tree = cd.parse_json_tree(text)
        self.assertEqual(tree, json.loads(text))

    def test_parse_json_tree_handles_deep_dump(self):
        tree = cd.parse_json_tree(deep_dump_text(DEEP))
        self.assertEqual(tree['n'], 'f0')

    def test_collect_sites_survives_deep_tree(self):
        frames = cd.FrameTable()
        tree = cd.parse_json_tree(deep_dump_text(DEEP))
        sites, totals = cd.collect_sites(tree, frames, False)
        self.assertEqual(totals[0], sum(range(DEEP)) + 7)
        self.assertEqual(totals[1], DEEP + 2)
        self.assertEqual(len(sites), DEEP + 1)  # every chain node plus the leaf


class TestNameNormalization(unittest.TestCase):
    def test_strips_windows_dbghelp_tail(self):
        self.assertEqual(cd.normalize_name('Foo::bar(713)  +46'), 'Foo::bar')

    def test_strips_unix_offset_tail(self):
        self.assertEqual(cd.normalize_name('Foo::bar(int) + 0x7f1234'), 'Foo::bar(int)')
        self.assertEqual(cd.normalize_name('module + 0x0x1a2b'), 'module')

    def test_keeps_plain_names(self):
        self.assertEqual(cd.normalize_name('dag::Vector<int>::resize'),
                         'dag::Vector<int>::resize')


class TestEmbedTag(unittest.TestCase):
    def test_matches_both_quote_styles(self):
        self.assertTrue(cd.EMBED_TAG_RE.search(
            '<script id="embeddedData" type="application/json">'))
        self.assertTrue(cd.EMBED_TAG_RE.search(
            "<script type='text/plain' id='embeddedData'>"))


if __name__ == '__main__':
    unittest.main()
