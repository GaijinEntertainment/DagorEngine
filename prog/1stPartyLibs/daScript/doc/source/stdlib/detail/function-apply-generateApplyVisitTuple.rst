def apply`foo : void
self:Foo
block<(name:string;x:field1type)>
arg_field1:block...
invoke()
arg_field1
"name"
self
self.name

    def apply`Foo(self:Foo;arg_field1:block<(name:string,value:field1-type):void>;arg_field2:...)
        if variant_index(self)==0
            invoke(arg_field1,"field1",self.field1)
            return
        if variant_idnex(self)==2
            invoke(arg_field2,"field2",self.field2)
            return
        ...

