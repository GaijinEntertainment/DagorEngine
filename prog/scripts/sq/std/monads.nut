const ASSERT_MSG = "provided value should be Monad"
/*

  todo:
    unify as pssible interfaces for Task and Maybe\Either
*/


/*
function to cceck interfaces, useful for design and tests
local Foo = class {get = @(_) null}

checkInterface(Foo, [{name = "get", params = ["v"]}])

*/
function checkInterface(klass, methods){
  let failedMethods = []
  foreach (method in methods){
    if (type(method) == "string") {
      if (method not in klass)
        failedMethods.append([method, "not presented"])
    }
    else {
      let {name, isVargved=false, params = null} = method
      if (name not in klass) {
        failedMethods.append([name, "not presented"])
        continue
      }
      local {varargs, parameters} = klass[name].getfuncinfos()
      if (isVargved && varargs<1){
        failedMethods.append([name, "should be vargved"])
      }
      if (params == null)
        continue
      parameters = parameters.slice(1)
      if (params.len() < parameters.len()) {
        failedMethods.append([name, "too many arguments"])
        continue
      }
      foreach(i, p in params) {
        let pp = parameters?[i]
        if (pp != p)
          failedMethods.append([name, $"incorrect argument in method: got '{p}' expected '{pp}'"])
      }
    }
  }
  if (failedMethods.len()==0)
    return true
  foreach(f in failedMethods){
    println($"incorrect method '{f[0]}', error: {f[1]}")
  }
  return false
}
/*
  monad has interface with
  of, flatMap, map
*/
let class Monad {
  //of, pure :: a -> M a
  static function of(_){
    throw("pure method needs to be implemented")
  }
  //flatMap :: # M a -> (a -> M b) -> M b
  function flatMap(_){
    throw("flatMap method needs to be implemented")
  }
  isMonad = @() true
  //map :: # M a -> (a -> b) -> M b
  function map(f){
    let next = this.flatMap(@(x) this.of(f(x)))
    assert(next?.isMonad(), ASSERT_MSG)
    return next
  }
  function pipe(...){
    local next = this
    foreach (f in vargv){
      let fn = f
      next = this.flatMap(@(x) this.of(fn(x)))
      assert(next?.isMonad(), ASSERT_MSG)
    }
    return next
  }
}
/*
Maybe has few base methods:
flatMap which is core of any monad - return new Monad of same type transformed by provided function which should return Monad
map lets us get Maybe from Maybe with function that transforms value to another value
filter lets us change Some into None if a condition is not met
orSome - (value) get monad value or just another value
orElse - (Monad) get monad (if is Some) or else new passed monad
*/

local Some
local none
local _Maybe
local _None

_Maybe = class (Monad) {
  constructor(_) {
    throw("do not call directly")
  }
  static function of(a) {
    return (a==null || a==none || a?.isNone())
     ? none
     : (a instanceof Some || a?.isSome())
       ? Some(a.get())
       : Some(a)
  }
  filter = @(fn) this.isNone() ? none : _Maybe.of(fn(this._value) ? this._value : null)
  //nothing :: a -> nothing a
  static nothing = @(...) none
  //some :: a -> Some a
  static some = @(a) Some(a)
  function flatMap(fn){
    if (this.isNone())
      return none
    let next = fn(this._value)
    assert(next instanceof Monad || next?.isMonad(), ASSERT_MSG)
    return next
  }
  static isMaybe = @() true
  function cata(onNoneFn, onSomeFn){
    let next = this.isNone() ? onNoneFn(null) : onSomeFn(this._value)
    assert (next instanceof Monad || next?.isMonad(), ASSERT_MSG)
    return next
  }

  orElse = @(other) this.isNone() ? (assert (other instanceof Monad || other?.isMonad(), ASSERT_MSG) ?? other) : this
  orSome = @(other) this.isNone() ? _Maybe.of(other) : this
  getOrElse = @(other) this.isNone()  ? other : this._value
  get =  @() this.isNone() ? null : this._value
  isNone = @() false
  isSome = @() false
}

/*
 * None class wraps null values and prevents errors
 * that otherwise occur when mapping unexpected undefined or null
 * values
 * except constructor, _tostirng and isNone - everything else are optimizations
 */
_None = class (_Maybe) {
  constructor(){}
  static get = @() null
  static map = @(_fn) none
  static filter = @(...) none
  static orElse = @(other) other
  static orSome = @(other) Some(other)
  static getOrElse = @(other) other
  static isNone = @() true
  static _tostring = @() "[object Maybe.None]"
}

/**
 * Monad Some used for valid values
 */
Some = class (_Maybe) {
  _value = null
  constructor (value) {
    this._value = value
  }
  get = @() this._value
  getOrElse = @(_) this._value
  orElse = @(_) this
  orSome = @(_) this
  map = @(fn) _Maybe.of(fn(this._value))
  filter = @(fn) _Maybe.of(fn(this._value) ? this._value : null)
  isSome = @() true
  _tostring = @() $"[object Maybe.Some] (value: {this._value})"
}

none = _None()
let None = @() none
function Maybe(a){
  if (a==null)
    return none
  else
    return _Maybe.of(a)
}


local Either
local Right
local Left

Either = class (Monad) {
  //pure :: a -> Either a
  static function of(value){
    return Right(value)
  }
  constructor(...){
    throw("Either can't be created, use Left or Right")
  }
  isEither = @() true
  //flatMap :: # Either a -> (a -> Either b) -> Either b
  function flatMap(f){
    if (this.isLeft())
      return this
    else {
      let next = f(this._value)
      assert(next instanceof Monad || next?.isMonad(), ASSERT_MSG)
      return next
    }
  }
  function flatMapTry(f){
    try{
      if (this.isLeft())
        return this
      else {
        let next = f(this._value)
        assert(next instanceof Monad || next?.isMonad(), ASSERT_MSG)
        return next
      }
    }
    catch(e)
      return Left(e)
  }
  isLeft = @() false
  isRight = @() false
  function fromNullable(a){
    if (a?.isLeft() || a?.isRight())
      return a
    return a?.isNone() || a == null ? Left(a) : Right(a)
  }
  function map(fn){
    if (this.isLeft())
      return this
    let next = fn(this._value)
    return next instanceof Either  || next?.isMonad() ? next : Either.of(next)
  }
   // the same as map, but if fn throw returns Left with error
  function tryMap(fn){
   try{
     if (this.isLeft())
       return this
     let next = fn(this._value)
     return next instanceof Either  || next?.isMonad() ? next : Either.of(next)
   }
   catch(e)
     return Left(e)
  }
  left = @(a) Left(a)
  right = @(a) Right(a)
  filter = @(fn) Either.fromNullable(fn(this._value) ? this._value : null)
  function tryFilter(fn) {
    try{
      return Either.fromNullable(fn(this._value) ? this._value : null)
    }
    catch(e)
      return Left(e)
  }
  get = @() this._value
  getOrElse = @(other) this.isLeft() ? other : this._value
  function orElse(fn) {
    if (this.isLeft()) {
      let next = fn(this._value)
      assert(next instanceof Either || next?.isEither(), ASSERT_MSG)
    }
    return this
  }
  function tryOrElse(fn) {
    try{
      if (this.isLeft()) {
        let next = fn(this._value)
        assert(next instanceof Either || next?.isEither(), ASSERT_MSG)
      }
      return this
    }
    catch(e)
      return Left(e)
  }
  function getOrElseThrow(a) {
    if (this.isLeft())
      throw(a)
    return this._value
  }
  function orThrowValue() {
    if (this.isLeft())
      throw(this._value)
  }
  _tostring = @() this.isLeft() ? $"[object Either.Left] (value: {this._value})" : $"[object Either.Right] (value: {this._value})"
  toObject = @() {value = this._value, isLeft=this.isLeft()}
  function effect(onLeft, onRight){
    if (this.isLeft())
      onLeft(this._value)
    onRight(this._value)
  }
  function cata(onLeft, onRight) {
    let next = this.isLeft() ? onLeft(this._value) : onRight(this._value)
    assert(next instanceof Either || next?.isEither(), ASSERT_MSG)
    return next
  }
  function ofTry(fn){
    try{
      return Either.of(fn())
    }
    catch(e)
      return Left(e)
  }
  function tryCata(onLeft, onRight) {
    try{
      let next = this.isLeft() ? onLeft(this._value) : onRight(this._value)
      assert(next instanceof Either || next?.isEither(), ASSERT_MSG)
      return next
    }
    catch(e){
      return Left(e)
    }
  }
}

Left = class (Either) {
  _value = null
  isLeft = @() true
  constructor(v){
    this._value = v
  }
  flatMap = @(_) this
}

Right = class (Either) {
  _value = null
  isRight = @() true
  constructor(v){
    this._value = v
  }
  function flatMap(f) {
    let next = f(this.value)
    assert(next instanceof Either || next?.isEither(), ASSERT_MSG)
    return next
  }
}

local Identity

Identity = class (Monad) {
  _value = null
  constructor(v){
    this._value = v
  }
  map = @(fn) Identity.of(fn(this._value))

  function flatMap(fn){
    let next = fn(this._value)
    assert(next instanceof Monad || next?.isMonad(), ASSERT_MSG)
    return next
  }

  function pipe(...){
    local next = this._value
    foreach(fn in vargv)
      next = fn(next)
    return Identity.of(next)
  }

  function flatPipe(...){
    local next = this
    foreach(fn in vargv) {
      next = fn(next)
      assert(next instanceof Monad || next?.isMonad(), ASSERT_MSG)
    }
    return next
  }
  of = @(v) Identity(v)
  call = @(fn) fn(this._value)
  ofFn = @(fn) Identity(fn())
  get = @() this._value
}

/*
based https://medium.com/swlh/what-the-fork-c250065df17d
  fork -> exec
  chan -> flatMap

more to read:
  https://github.com/jklmli/monapt/blob/master/src/future/future.ts
  https://medium.com/hackernoon/from-callback-to-future-functor-monad-6c86d9c16cb5
  https://dev.to/avaq/fluture-a-functional-alternative-to-promises-21b

*/
local Task
Task = class (Monad) {
  exec = null
  constructor(fn){
    this.exec = fn
  }
  function map(fn) {
    let f = this.exec
    return Task(@(errFn, okFn)
      f(errFn, @(x) okFn(fn(x)))
    )
  }
  function flatMap(fn){
    let f = this.exec
    return Task(@(errFn, okFn) f(errFn, @(x) fn(x).exec(errFn, okFn)))
  }
  isTask = @() true
  get = @() this.flatMap(@(x) x)
  of = @(x) Task(@(_, ok) ok(x))
}


if (__name__ == "__main__"){
  println("testing Maybe Monad")
  let foo = Maybe(2)
    .map(@(_) null).orSome(10).orElse(2).map(@(...) null).orElse(Some(-2))
    .flatMap(@(v) Some(v+1)).get()
  if (foo!=-1){
    throw("FAILED Maybe check")
  }
}


return {
  _None
  None
  none
  Maybe
  _Maybe
  Some
  Identity
  Monad
  Either
  Left
  Right
  Task
  checkInterface
}
