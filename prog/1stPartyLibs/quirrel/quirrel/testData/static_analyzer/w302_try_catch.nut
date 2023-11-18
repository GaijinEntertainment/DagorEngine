local blk = null

function foo() {}

function bar(_x) {}

if (foo()){
  blk = foo()
  try
    blk.load("1111")
  catch(e)
    blk = null
}
else{
  try
    blk = foo()
  catch(e)
    blk = null
}

bar(blk)

