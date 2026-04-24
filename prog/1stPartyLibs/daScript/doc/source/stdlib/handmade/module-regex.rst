The REGEX module implements regular expression matching and searching.
It provides ``regex_compile`` for building patterns, ``regex_match`` for
full-string matching, ``regex_search`` for finding the first match anywhere,
``regex_foreach`` for iterating all matches, ``regex_replace`` for substitution
(both block-based and template-string forms), ``regex_split`` for splitting strings,
``regex_match_all`` for collecting all match ranges, ``regex_group`` for
capturing groups by index, and ``regex_group_by_name`` for named group lookup.

See :ref:`tutorial_regex` for a hands-on tutorial.

Supported syntax:

- ``.`` — any character except newline (use ``dot_all=true`` to also match ``\n``)
- ``^`` — beginning of string (or offset position)
- ``$`` — end of string
- ``+`` — one or more (greedy)
- ``*`` — zero or more (greedy)
- ``?`` — zero or one (greedy)
- ``+?`` — one or more (lazy)
- ``*?`` — zero or more (lazy)
- ``??`` — zero or one (lazy)
- ``{n}`` — exactly *n* repetitions
- ``{n,}`` — *n* or more (greedy)
- ``{n,m}`` — between *n* and *m* (greedy)
- ``{n}?`` ``{n,}?`` ``{n,m}?`` — counted repetitions (lazy)
- ``(...)`` — capturing group
- ``(?:...)`` — non-capturing group
- ``(?P<name>...)`` — named capturing group
- ``(?=...)`` — positive lookahead assertion
- ``(?!...)`` — negative lookahead assertion
- ``|`` — alternation
- ``[abc]``, ``[a-z]``, ``[^abc]`` — character sets (negated with ``^``)
- ``\w`` ``\W`` — word / non-word characters
- ``\d`` ``\D`` — digit / non-digit characters
- ``\s`` ``\S`` — whitespace / non-whitespace characters
- ``\b`` ``\B`` — word boundary / non-boundary assertions
- ``\t`` ``\n`` ``\r`` ``\f`` ``\v`` — whitespace escapes
- ``\xHH`` — hexadecimal character escape
- ``\.`` ``\+`` ``\*`` ``\(`` ``\)`` ``\[`` ``\]`` ``\|`` ``\\`` ``\^`` ``\{`` ``\}`` — escaped metacharacters

Flags:

- ``case_insensitive=true`` — ASCII case-insensitive matching (pass to ``regex_compile``)
- ``dot_all=true`` — ``.`` also matches ``\n`` (pass to ``regex_compile``)

Template-string replacement:

``regex_replace(re, str, replacement)`` replaces matches using a template string.
Supported references: ``$0`` or ``$&`` for the whole match, ``$1``–``$9`` for
numbered groups, ``${name}`` for named groups, ``$$`` for a literal ``$``.

The engine is ASCII-only (256-bit ``CharSet``). Matching is anchored — ``regex_match`` tests from
position 0 (or the given offset) and does NOT search; use ``regex_search`` to find the first
occurrence, or ``regex_foreach`` / ``regex_match_all`` to find all occurrences.

See also :doc:`regex_boost` for compile-time regex construction via the ``%regex~`` reader macro.

All functions and symbols are in "regex" module, use require to get access to it.

.. code-block:: das

    require daslib/regex

Example:

.. code-block:: das

    require daslib/regex
        require strings

        [export]
        def main() {
            var re <- regex_compile("[0-9]+")
            let m = regex_match(re, "123abc")
            print("match length = {m}\n")
            let text = "age 25, height 180"
            regex_foreach(re, text) $(r) {
                print("found: {slice(text, r.x, r.y)}\n")
                return true
            }
        }
        // output:
        // match length = 3
        // found: 25
        // found: 180
