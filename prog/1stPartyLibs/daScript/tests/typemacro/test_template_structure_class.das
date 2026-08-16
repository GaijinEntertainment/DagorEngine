options gen2

require dastest/testing_boost public
require _template_structure_class_mod

typedef ProbeIntCommand = $ProbeScalarCommand<int>
typedef ProbeFloatCommand = $ProbeScalarCommand<float>

[test]
def test_template_structure_class(t : T?) {
    unsafe {
        var inscope intCommand <- new ProbeIntCommand()
        intCommand.newValue = 42
        intCommand->redo()
        t |> equal(intCommand->getValue(), 42)
        t |> equal(intCommand.executions, 1)
        intCommand->undo()
        t |> equal(intCommand.executions, 0)

        var inscope floatCommand <- new ProbeFloatCommand()
        floatCommand.newValue = 1.5
        floatCommand->redo()
        t |> equal(floatCommand->getValue(), 1.5)
        t |> equal(floatCommand.executions, 1)
    }
}
