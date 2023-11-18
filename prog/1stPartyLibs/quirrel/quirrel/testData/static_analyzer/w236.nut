//expect:w236
::SCRIPT_STATE_USER_SHIFT <- 2
let function foo(berserkFx, state){ //-declared-never-used
  if (!berserkFx && (state & (1 << ::SCRIPT_STATE_USER_SHIFT + 4)))
    ::print(1)
}