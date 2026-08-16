options gen2

module _template_structure_class_mod shared public

require daslib/typemacro_boost public

class public BaseCommand {
    executions : int = 0

    def abstract redo() : void
    def abstract undo() : void
}

[template_structure(ValueType)]
class template public ProbeScalarCommand : BaseCommand {
    newValue : ValueType

    def override redo() : void {
        executions++
    }

    def override undo() : void {
        executions--
    }

    def getValue() : ValueType {
        return newValue
    }
}
