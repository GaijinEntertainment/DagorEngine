# vmperf: measure a VM or compiler change net of code layout

Code layout alone changes the speed of the Quirrel VM by up to 17% on short
loops, and any change in the program can change the layout. Thus one "before"
and one "after" build do not show the effect of a change. Measure each version
in several layouts and compare the means. `vmperf.py` does this.

Use it for every Quirrel change that you think makes the VM or the compiled
code faster or slower: the VM, the compiler, the code generator, the
optimizer.

## Cause

`SQVM::Execute` is a `switch` on the opcode. Clang makes from it one shared
dispatch block: it reads the opcode, checks the range, reads the handler
address from a jump table and jumps there. Each handler jumps back to this
block at its end. The block is 29-36 bytes, and it runs for each bytecode
instruction.

When the block crosses a 64-byte line, short loops (primes, life, mandel,
closure_storm) become 8-17% slower on Raptor Lake P-cores. Probably the CPU
fetches code in 64-byte blocks, and two blocks for each pass cost more. A
32-byte boundary has no effect. On other CPUs the effect is different.

The position of the block follows the address of Execute. That address
follows the size of all code that the linker puts before it: the compiler,
the lexer, other VM functions. Functions start at multiples of 16 bytes, so
Execute has four possible positions in its 64-byte line. With the current
code, the block crosses the line in two of them. Thus an unrelated change can
move a build from a fast layout to a slow one, or back. A change in Execute
also moves the block from the inside, and it can make the block longer: when
clang has too few registers, it reloads the table address in each dispatch.

The same code (master, October 2026) with Execute moved by 0, 16, 32 and 48
bytes, ms; at 32 and 48 the block crosses the line:

| shift         |   0 |  16 |  32 |  48 |
| ------------- | --: | --: | --: | --: |
| primes        | 108 | 109 | 123 | 125 |
| life          |  47 |  48 |  49 |  51 |
| closure_storm |  70 |  68 |  72 |  74 |
| darg          | 116 | 112 | 120 | 115 |

The handlers and the functions after Execute move with the block, so even
darg changes by 7%. A layout can hide a smaller effect, or show it with the
wrong sign.

Example: the commit that removed static memoization. Before it, darg was the
only workload that used memos, so the others had no reason to change. The
jam builds before and after it showed primes and life 14% slower,
closure_storm 10% slower. `vmperf.py compare` of the commit and its parent
gives layout means of +1.6%, +2.2% and -0.6%, and darg +4.6%. In the jam
build, Execute had moved by 16 bytes, and the block crossed into the next
line by 4 bytes. A binary patch
that moved only the block back put primes and life within 4% of "before".

E-cores have a different design, and layout has a different effect on them:
the same build pair showed primes 20% faster on an E-core. Windows can move a
process between core types during a run, so an unpinned run mixes the
results of two different CPUs.

## How to measure

- Machine: power from the wall, the maximum performance mode, no load.
- Core: pin the process to one P-core. The other logical CPU of that core
  must stay idle.
- Length: a timed rep of 200 ms or more. `clock()` steps by 1 ms, so the
  error is then 0.5% or less. Repeat a short workload in the rep.
- Layouts: build each version four times, with Execute aligned to 64 bytes
  and K = 0, 16, 32, 48 NOP bytes after its prologue. Only the layout
  changes. The NOPs run once for each call of Execute, which costs nothing
  measurable.
- Order: run all builds in turn, 3-5 rounds, so that heat and background
  tasks affect all builds equally. Keep the fastest round of each build:
  noise only adds time.
- Compare the mean of the four layouts of each version. Do not use the best
  layout: each layout is a real build that a game can get. The spread
  between the layouts shows how much a workload depends on layout.
- Control: workloads that the change cannot affect must have equal means
  within 2-3%. If they do not, the measurement is wrong, not the change. A
  difference below 2-3% is neither a gain nor a loss.
- A workload that runs mostly outside the VM gets a wider tolerance: in f2s,
  Execute takes about 4% of the time, and the shift moves only Execute and
  the code after it.
- Report the layout means in the commit message. If the plain build gets a
  slow layout, write this too: that build goes into the games.

`-falign-loops=64` does not remove the effect: primes stops depending on
layout, but life and closure_storm become 6-9% slower.

## vmperf.py

Windows only. It needs git and the jam rel build of `prog/tools/consoleSq`:
run `jam -sConfig=rel` there after you update the tree. The tool compiles
only the objects that include Quirrel headers, from the sources of the
revision (`quirrel/quirrel` and `gameLibs/quirrel/quirrelHost`); all other
code links from the jam output. The plain build of the sources that jam
built has the same code (`.text`) as jam's csq.exe.

    python vmperf.py compare                   HEAD against the working tree
    python vmperf.py compare HEAD~1 HEAD       two revisions
    python vmperf.py compare HEAD wt --bench all --rounds 5
    python vmperf.py show <result.json> --all  the table again, and every build
    python vmperf.py dispatch HEAD wt          where the dispatch block lies
    python vmperf.py profile HEAD wt --bench primes,life
    python vmperf.py cores                     P- and E-cores of this machine

`compare` takes revisions (`wt` is the working tree) and paths of csq.exe
files. It builds each revision in five variants: `nat`, the plain build, and
`k0` `k16` `k32` `k48`, the layout shifts. Builds go to
`_output/quirrel-vmperf/<sha>/` and stay there while their sources and the
jam output stay the same (about 260 MB each; delete them when you are done). Then it sets for
each workload how many calls make a 200 ms rep, runs `--rounds` (default 4)
interleaved rounds on a P-core, prints the table and writes the result to
`_output/quirrel-vmperf/results/`. Default workloads take about 15 minutes.
Do not use the machine during the run. The first round is often a few
percent slower (heat from the build); the fastest round is kept.

The workloads are those of `../quirrel/`; `profile.nut` here takes the place
of their timing helper (`VMPERF_INNER` calls in each of 5 reps). A workload
that computes a different result in some build gets a warning.

How to read the table:

- `layout mean`, `delta`: the result.
- `spread`: (slowest / fastest layout - 1) for each version.
- `plain build`: the build that jam makes from the same sources. One
  number, with the layout that this build got.
- First lines: the bytes of its 64-byte line that the dispatch block takes
  in each build. `CROSSES` marks a slow layout.

Before you trust a result, do an A/A test: `compare HEAD wt` on a clean tree
compares two equal builds. Its deltas show the noise of this machine; on a
Raptor Lake laptop they stay within about 1%.

`dispatch` alone is the quick check when there is no time for a full run. If
both builds have the block on the same side (both cross or both do not), a
plain comparison is more trustworthy, but the layout of the other code still
gives some percent. `--listing` writes the code of Execute next to each
csq.exe, with offsets from its start, to diff two builds.

`profile` runs a sampling profiler (`sprof.cpp`, built on first use, no
admin rights) and prints the share of time per function and per source line
of `SQVM::Execute<false>`. A line shows its text and its number in the first
build. A revision means its plain build here.

Limits: the revisions must have the same source files as the tree that jam
built. For a change that adds or removes a file, build each version with
jam and give `compare` the csq.exe paths (plain layout only).

Files: `vmperf.py` the commands, `build.py` the builds and layouts,
`timing.py` pinning and timing, `image.py` symbols and the dispatch block,
`profiler.py` and `sprof.cpp` the profiler, `profile.nut` the timing helper.
