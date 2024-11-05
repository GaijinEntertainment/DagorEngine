if (__name__ == "__analysis__")
  return

//expect:w247

return ::a.b.c.indexof("x") + 6;

//-file:undefined-global
