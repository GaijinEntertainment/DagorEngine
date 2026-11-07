# daPCG plan

## Goal

A model program gives a shippable asset: a node tree with animated nodes, collision and LODs
over the meshes of daCSG and daSurf ([roadmap](agent_modelling_roadmap.md)). The first milestone
is the scene producer: the sport bike as a tree that can steer and turn its wheels.

## Milestones

| # | Milestone | State |
|---|---|---|
| M1 | Scene nodes ([scene_nodes_proposal.md](scene_nodes_proposal.md)) | done 2026-10-27, in review |
| M1.1 | Refactor, no behaviour change: the console's model result and .dag writer take a list of nodes (one node today); dagrender's node walk | done |
| M1.2 | Feature: the daPCG scene core and the das module `scene`, core tests; nothing calls them | done |
| M1.3 | Use: the console's `Node` path, `-nodes`, dagrender renderable and `-pose`, test_nodes.das, sheet_sync, the `scene` DSL sheet | done |
| M1.4 | Models: both sport bikes as trees, pose pictures in the csg-fit acceptance | done |
| M2 | A separate collision dag bound by `ref_skeleton` | later |
| M3 | LOD sets from `model(lod : int)`; needs segment counts that follow quality (roadmap section 6) | later |
| M4 | Open surfaces as output parts ([open_surfaces_proposal.md](open_surfaces_proposal.md)) | proposed |

## Tracks and owners

- Orchestrator: design, briefs, review, landing, Gerrit.
- impl-scene (Opus): M1.1 to M1.4.

## Decisions

- 2026-10-25: the owner's decisions 1-5 in [README.md](../README.md).
- 2026-10-26 and 2026-10-27, scene nodes: the answers of proposal section 9 (questions 1-3 as
  recommended; question 4 by the owner: a node is plain, a part that moves calls `n.animated()`).

## Risks

- Parallel node builds: no test runs two daCSG executes at once; the first version builds the
  nodes one after another.
- DagExporter is all static: .dag writes stay serial.
- The motorcycle physics checks its node axes (joltMotorcycle.cpp, loadDesc); a wrong pivot frame
  only logs an error at load in a game, so the console tests the rules on the written tree.
- dsl.md is at 4796 of its 4800 bytes: the `scene` calls get their own sheet.
- A node is executed alone, so a defect that the whole union hid can show.
