# modules/dasLLVM/tests - the module-owned suite

Contract: `ARCHITECTURE_COMMON.md` (repo root). The suite proves the JIT and exe backends on
the host it runs on - emitted IR shapes, built artifacts, tune sidecars, child processes spawned
from the daslang binary dastest itself runs under; the run command, the build gate and the
fixture-name suffixes are in `README.md` beside this file. This document states what a test's
child leaves in the tree and why.

## 1. What a test's child leaves beside the tree's descriptors

A daslang child a test here spawns runs the module scan over this tree before it compiles
anything, and the scan keeps a manifest, `.das_module.manifest`, beside every `.das_module`
descriptor it compiles, replaying it on later starts. Those sidecars are the scan's steady
state and are gitignored: they are the one write a child makes outside the directory its test
created, and they stay.
