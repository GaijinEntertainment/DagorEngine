# lint

lint ships in the SDK as a prebuilt exe - the `DAS_UTILS_SHIPPED_EXES` entry in `utils/CMakeLists.txt`
(repo root); `utils/REVIEW.das` reads this line as the record of that decision.

The lint suite runner: compiles each target file and applies the rule modules
(`daslib/perf_lint.das`, `daslib/style_lint.das`, `daslib/lint.das`) over its AST.

Run:

    daslang utils/lint/main.das -- <files or dirs> --quiet

Exit 2 on any warning - CI's changed-files gate and the nightly full sweep both key on it.
The SDK also ships a prebuilt `bin/lint.exe`.

Five rules are the runner's own, because they are about folders rather than code. Each runs
once per invocation, over a walk of the directory roots the run was given. A directory whose
`.lint_config` carries `[docs] rule_docs_only = true` may hold only rule documents
(`REVIEW*.md`, `ARCHITECTURE*.md`); any other `.md` beside the sources is
**LINT025**. **LINT026**'s reverse direction needs no tag: every `{#anchor}` in any `.md` under
the run's roots must be cited - by an `[arch]` in a `.das` there, a `// <doc>.md#<anchor>`
pointer in a C++ source, or a `<doc>.md#<anchor>` anywhere in any markdown file under the roots
(a rule document, a ledger, an architecture document's routing alike) - and a markdown citation
that resolves to no section is a forward finding like a code one; a markdown citation's path is
joined to the citing file's folder (so `./` and `../` resolve against that folder), then tried at
each ancestor up to the root as a C++ pointer's is, and the folder-tree rule does not bind it. **LINT027** caps each
`REVIEW*.md` / `ARCHITECTURE*.md` at 300 lines in every folder that holds one. Two more read
the checklists' text: **LINT032** reports a `REVIEW*.md` citing a rule by position ("the rule
above", "see below"), and **LINT033** a `REVIEW*.md` naming a path (a backticked token with a
source extension) or a test cell (a backticked `test_<subject>_<claim>`) the tree no longer has -
a path resolves against the checklist's folder, its ancestors and the working directory, a bare
basename anywhere in the repo, a cell as a `def test_*` under the checklist's folder. The
`rule_docs_only` key is a folder property - it never cascades, unlike `[format]`. Fixtures:
`tests/lint025_*`, `tests/lint026_*`, `tests/lint027_*` and `tests/lint033_*`, each driving the
CLI over a planted tree.

Design: the runner stays thin - rules live in the daslib modules (authoring rails:
`skills/internal/perf_lint_authoring.md`, `skills/internal/style_lint_authoring.md`);
suppression policy is `skills/perf_lint.md` / `skills/style_lint.md`. `tests/` here are the
runner's own fixtures, run by `run_utils_tests`.

Every rule id has a fixture whose file name carries the id - `tests/<id>_*.das` here, or a
`tests/lint/` (repo root) file for the rules whose test needs the dastest harness - and a
section in `doc/source/reference/language/lint.rst`; `REVIEW.das` beside this file checks the
triple. Fixtures whose names do not carry the id: LINT019 -> `tests/lint/test_stale_nolint.das`.
