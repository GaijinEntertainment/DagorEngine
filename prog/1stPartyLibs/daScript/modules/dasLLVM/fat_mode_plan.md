# The fat tune mode - one exe, one clone per CPU class

The plan for the third `[tune]` mode. `skills/tune.md` carries the two existing modes;
`ARCHITECTURE_TARGET_FEATURES.md` sec.10 carries the CPU classes and the baseline the mode builds
on. Present-tense mechanism text moves into the architecture docs as each piece lands; this file
keeps the plan, the rules decided up front, and the ledger.

## What it is

A standalone exe built for a **baseline class** (`DAS_JIT_BASELINE=x86-avx2`) that carries, for
every `[tune]` kernel, one clone per CPU class the library ships a profile for, and picks the
clone at startup from cpuid. Kernels are baked; the exe never re-races or tunes them. The
runtime section of the sidecar - the knobs the library applies at load, the Metal twin crowns
among them - is raced inside the exe at first start, because that race needs no rebuild.

## The pieces, in landing order

1. **Baseline class** - `DAS_JIT_BASELINE`, the class table, one truth on every rail. Landed:
   `daslib/llvm_cpu_class.das`, `tests/llvm_jit_baseline.das`.
2. **Fat stamping** in `daslib/llvm_tune.das` - `DAS_TUNE_MODE=fat`, refused outside `-exe`
   and without the baseline. Landed (`ARCHITECTURE_TARGET_FEATURES.md` sec.11): the ship set,
   the per-class clones and companions, the dispatch chain over a runtime class MASK (one bit
   per ladder class, so a class's membership is its own full feature set - a vnni512 box
   without vbmi is not in `x86-vnni512` and takes the avx2 clone), `DAS_TUNE_FAT_CLASS`, the
   `klass` column of `tune_status`. Test: `tests/llvm_tune_fat.das`.
3. **Emitter** - landed: the clone's `target-cpu` / `target-features` attributes, the tier
   gates swapped to the class around its generator, and `noinline` (AArch64's inliner would
   otherwise pull an intrinsic-free clone into the baseline). The dispatch is generated as
   daslang and lowered by the JIT to a load, compares and direct calls; an IR stub was not
   needed.
4. **The runtime section in the exe** - landed (`modules/dasLLAMA/ARCHITECTURE_MEASUREMENT.md`
   sec.2.42): `dasllama/dasllama_fat_start.das` owns the runtime snapshot and the Metal twin
   race the harness now calls too, and registers the first-start hook the box-profile apply
   fires for a missing section; a fat exe races once, writes the sidecar beside itself, and
   the next start reads it. Test: `modules/dasLLAMA/tests/test_fat_start.das`.
5. **Release** - landed: `daspkg release --fat <class>` builds under the baseline and the fat
   mode, mints nothing, ships no sidecar, and refuses a kernel whose baseline clone came from
   no profile (the deps JSON's `fat_unprofiled`). Test: `test_cmd_release_fat` in
   `utils/daspkg/test_daspkg.das`.

## Rules decided up front

- **A shipped exe runs no confirm.** Every confirm the tuner has spawns a daslang child on a
  harness script and a vehicle model: the generator half's end-to-end prefill confirm (fires on
  every box when a crown diverges from the fallback), the kernel half's IQ2_XXS serving confirm
  and the gemma MTP depth confirm (Metal only). None of them ships. On the M5 the twin race
  alone is 14 s; the two Metal confirms were 147 of the family's 161 s.
- **Kernels are never raced outside a mint.** The exe carries no grid, no tuner and no
  `<name>_variants()` rows beyond the reference row; a re-tune is a new profile and a rebuild.
- **The C++ runtime and every `.shared_module` are baseline already**: no CMake in the repo
  passes `/arch:` or `-march`, so MSVC and clang both emit the x86-64 baseline. A build flag
  raising that would silently break every fat exe on the class below it.
- **Classes, not cpus.** The knob names a class; the LLVM cpu is the class row's. A scheduling
  model knob does not exist until a measurement says the generic model costs something.
- **Macs are two classes.** M1 is `arm-neon`, M2 and later are `arm-i8mm`; a Mac exe with
  baseline `arm-neon` runs on M1 and picks the i8mm clone on the rest. SME would be a third
  class, never a per-box choice.

## Later, not this arc

- `[tuned]` loop-hint kernels cloned per class (the auto-vectorizer widens under the attrs).
- A per-box kernel sidecar choosing among the baked clones.
- Chip-keyed shipped runtime sections, if a 14 s first-start race ever proves too slow.
- A vnni512 class without vbmi for the pre-Ice-Lake Xeons, if anyone ships to one.
- Per-box profiles for the shipped classes, after dasllama-server ships: an `arm-sme` class for
  M4/M5, and an AMX crown for `x86-amx` once the tile and the gemv are separate backends -
  `modules/dasLLAMA/followup_general.md` entry 118 carries the numbers and the plan.
