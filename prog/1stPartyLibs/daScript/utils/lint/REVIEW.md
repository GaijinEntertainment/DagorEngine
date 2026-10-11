# lint Code Review Checklist

**Read `REVIEW_COMMON.md` (repo root) first - its contract binds this checklist.** Architecture doc:
`README.md`.

**A diff that shrinks the set of rule ids `REVIEW.das` (beside this file) scans is a
defect** - whether by editing the gate or by deleting an id's last scannable spelling. The
gate checks that every emitted rule id has a fixture and a `doc/source/reference/language/lint.rst`
(repo root) section, so a shrunk set retires those checks with no warning.

**A diff that makes a file emit a rule id the gate `REVIEW.das` (beside this file) does not
scan adds that file to `RULE_MODULES` in the gate, in the same change.** A file emits an id
when its own code prints it; a fixture asserting on that text does not emit.
