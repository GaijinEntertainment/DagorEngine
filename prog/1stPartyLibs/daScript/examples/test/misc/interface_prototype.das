class ITick
    def abstract beforeTick : void
    def abstract tick ( dt:float ) : void
    def abstract afterTick : void

class ILogger
    def abstract log ( message : string ) : void

class Foo
    _i_tick : ITick?
    _i_logger : ILogger?
    def Foo
        pass
    def get_ITick : ITick?
        if _i_tick == null
            _i_tick = new implementation_Foo_ITick(self)
        return _i_tick
    def get_ILogger : ILogger?
        if _i_logger == null
            _i_logger = new implementation_Foo_ILogger(self)
        return _i_logger
    def ITick`tick ( dt:float )
        print("tick {dt}\n")
    def ITick`beforeTick
        print("beforeTick\n")
    def ITick`afterTick
        print("afterTick\n")
    def ILogger`log ( message : string )
        print("log {message}\n")

class implementation_Foo_ITick : ITick
    [[do_not_delete]] _self : Foo?
    def implementation_Foo_ITick ( var that:Foo )
        _self = unsafe(addr(that))
    def override beforeTick : void
        _self->ITick`beforeTick()
    def override afterTick : void
        _self->ITick`afterTick()
    def override tick ( dt:float ) : void
        _self->ITick`tick(dt)

class implementation_Foo_ILogger : ILogger
    [[do_not_delete]] _self : Foo?
    def implementation_Foo_ILogger ( var that:Foo )
        _self = unsafe(addr(that))
    def override log ( message : string ) : void
        _self->ILogger`log(message)


[export]
def main
    print("hello, world\n")
    var f = new Foo()

    f->get_ITick()->beforeTick()
    f->get_ITick()->tick(1.0)
    f->get_ITick()->afterTick()
    f->get_ILogger()->log("hello")

    unsafe
        delete f

options log
