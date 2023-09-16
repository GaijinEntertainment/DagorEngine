int force_ignore_history = 0;

macro USE_IGNORE_HISTORY(stage)
  (stage) {
    force_ignore_history@i1 = (force_ignore_history);
  }
endmacro