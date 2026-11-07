# daPCG

daPCG is the home of procedural content generation in Dagor: model programs and generators that
make trees, rocks, buildings and other assets. An asset is not always a solid: a tree has leaf
cards and a house has window panes. daPCG builds on daCSG (mesh CSG, `prog/gameLibs/daCSG`) and
daSurf (B-spline surfaces, `prog/gameLibs/daSurf`). Its first code is the scene producer of the
[scene nodes proposal](_doc/scene_nodes_proposal.md): `scene/` (the node tree, pivot frames and
build on daCSG, `scene_api.h`, tested in `unittests/daPCG`) and `das/` (the `scene` das module).
Then `material/` (the .dag material table of a model: the rows, their rules and the script join,
`material_api.h`, tested in `unittests/daPCG`) and the `material` das module in `das/`,
with its sheet [material.md](_doc/material.md).

## The owner's decisions of 2026-10-25

1. daSurf is a general surface modeller, not only the body modeller of daCSG: it must not be
   limited to closed solids. Only daCSG boolean operands must be closed solids.
2. The agreed order: first a model output of parts (solids through daCSG, open surfaces joined
   with no boolean, instanced); later a daCSG clip of an open surface by a solid.
3. The open surfaces design is proposed, not decided: its DSL shape and its five questions
   (section 8) wait for the owner. It lands as new commits on top of the daCSG and daSurf stack
   (a refactor, the feature, the console and a tree model), after the daSurf review loop.
4. From the research (Procedura, libfive): take the checks and the organic-leaf design, write all
   of it again under our float rules, and copy no code (libfive is MPL 2.0, its Studio GPL).
5. libfive is the reference for our future dual-contouring SDF modelling tool, for performance,
   memory and correctness. It is a reference only: a test or bench harness links it (MPL 2.0
   allows linking), and our library never copies it. The comparison is on geometry, not bytes,
   since libfive's vertex order depends on its threads. The measures: time and peak memory on
   the same fields at the same cell size; the distance of the mesh to the field; sharp edges
   kept; volume; triangle count; validity (closed, no self-intersection). libfive's output can
   fail validity (research section 3), so there "at least as good" means that ours stays valid.
   libfive needs Eigen and Boost interval, lockfree and container; `prog/3rdPartyLibs/boost` has
   only date_time, filesystem, system and thread, and the repo has no Eigen.

Context, not decisions:

- The csg tools use daCSG, not carve. Until the clip lands, daCSG refuses dagCSG SLICE_IN and
  SLICE_OUT of an open A.
- The manifold kernel keeps its refusal of some box grids, and the exact kernel takes them as the
  fallback (deferred by the lead on 2026-10-23).

## Open questions

The five questions of the open surfaces proposal (section 8), for the product owner; the open
questions of the roadmap (section 12); the libfive harness of decision 5: a libfive checkout path
outside the repo (dev and bench only), or the Eigen and Boost parts vendored for tests only.

## Docs

| Doc | What it holds |
|---|---|
| [Plan](_doc/PLAN.md) | Milestones with their state, owners, decisions, risks. |
| [Scene nodes proposal](_doc/scene_nodes_proposal.md) | The scene producer: a model returns a tree of named nodes with pivots and collision keys, the moving ones animated, written as one multi-node .dag, so the sport bike can steer and turn its wheels. Decided 2026-10-26; the owner changed question 4 on 2026-10-27. |
| [Open surfaces proposal](_doc/open_surfaces_proposal.md) | Open surfaces (leaf cards, panes, open sheets) as output parts next to the daCSG solid: the `Model` value, the host side, validation, the later clip. Proposed; five questions open. |
| [Research: Procedura and libfive](_doc/research_procedura_libfive.md) | What an agent pipeline over OpenSCAD (Procedura) and an f-rep kernel with octree dual contouring (libfive) teach us: ideas ranked by value against cost, what not to copy, next steps. |
| [Agent modelling roadmap](_doc/agent_modelling_roadmap.md) | The daCSG and daSurf roadmap for agent modelling: the scene producer, look tools, kits, grammars, LODs, the organic library, the phases. A copy of `prog/gameLibs/daCSG/roadmap.md` of the dacsg worktree (untracked), 2026-10-25. |
| [Deferred work](_doc/deferred.md) | The features that wait for a later phase. |
| HTML page, `_doc/index.html` | README.md and every `_doc/*.md` on one page with a side navigation; not in git. Built, not written by hand: run `python build_html.py` in this folder after a doc changes (Python 3, standard library only). |
