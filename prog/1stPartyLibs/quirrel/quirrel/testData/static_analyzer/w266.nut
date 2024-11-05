if (__name__ == "__analysis__")
  return

//expect:w266

{
  ::x++
} while (::x)
::x--

//-file:undefined-global
