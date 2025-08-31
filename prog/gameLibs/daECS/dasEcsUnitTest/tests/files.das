require ecs
require DagorFiles

struct FooBar
  x : int
  y : float

[es(on_event=EventStart)]
def file_test_es(evt : Event)
  print("running file_test script\n")
  // puts and gets
  print("saving ftest.txt\n")
  df_open("ftest.txt", DF_WRITE) <| $(file)
    if file != null
      df_write(file, "hello,world!\n")
    else
      panic("can't open ftest.txt for writing")
  print("loading ftest.txt\n")
  df_open("ftest.txt", DF_READ) <| $(file)
    if file != null
      let s = df_gets(file, 1024)
      assert(s == "hello,world!\n")
    else
      panic("can't open ftest.txt for reading")
  // read and write
  print("saving ftest.bin\n")
  df_open("ftest.bin", DF_WRITE) <| $(file)
    if file != null
      df_write(file, FooBar(x = 1, y = 1.0))
    else
      panic("can't open ftest.bin for writing")
  print("loading ftest.bin\n")
  df_open("ftest.bin", DF_READ) <| $(file)
    if file != null
      var tfb : FooBar
      df_read(file, tfb)
      assert(tfb.x == 1 && tfb.y == 1.0)
      let pos = df_tell(file)
      assert(pos == typeinfo sizeof(type<FooBar>))
    else
      panic("can't open ftest.bin_save for reading")
  // save and load
  //this part is not working!
  print("saving ftest.bin_save\n")
  df_open("ftest.bin_save", DF_WRITE) <| $(file)
    if file != null
      df_save(file, FooBar(x = 1, y = 1.0))
    else
      panic("can't open ftest.bin for writing")
  print("loading ftest.bin_save\n")
  df_open("ftest.bin_save", DF_READ) <| $(file)
    if file != null
      var tfb : FooBar
      df_load(file, tfb)
      assert(tfb.x == 1 && tfb.y == 1.0)
    else
      panic("can't open ftest.bin_save for reading")
