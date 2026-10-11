# examples Code Review Checklist

**Read `REVIEW_COMMON.md` (repo root) first - its contract binds this checklist.**

**A `.das` in this folder tree that a game's `main.das` requires, directly or through another
required file, answers to `games/REVIEW.md` too - wherever the diff puts it.**

**Every file in a folder that holds a `web_shell.html` answers to `dasLLAMA/REVIEW.md` too -
wherever in this tree that folder sits.**

**A diff that drops `require live/audio_live` from a program states, in the PR body, why that
program's audio is meant to die with a reload** - `audio_live` is what carries the audio
thread's stream and channels across a reload, so without it the program creates and finalizes
its own audio system on every reload.

**A diff that adds or changes a draw into a color attachment whose internal format ends in `32F`
disables `GL_BLEND` before that draw.** Blending survives from whatever drew last, and a browser
without `EXT_float_blend` refuses a blended draw into a 32-bit float target with
`GL_INVALID_OPERATION`.
