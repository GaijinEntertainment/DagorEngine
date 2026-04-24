options gen2
options no_aot

// Tutorial macro module: capture macro that audits captured fields.
//
// Provides:
//   [audited]            — structure annotation (tag) marking a struct for audit
//   CaptureAuditMacro    — [capture_macro] with three hooks:
//     captureExpression   — wraps capture value in audit_on_capture() (per variable)
//     captureFunction     — appends audit_after_invoke() to per-call finally
//     releaseFunction     — appends audit_on_finalize() to the finalizer (destruction)
//   audit_on_capture()   — runtime helper: prints a message and returns the captured value
//   audit_after_invoke() — runtime helper: prints a message after each lambda invocation
//   audit_on_finalize()  — runtime helper: prints a message when a lambda is destroyed

module capture_macro_mod

require daslib/ast
require daslib/ast_boost

// ---- [audited] tag annotation -----------------------------------------------
// Mark a struct with [audited] to opt in to capture/release logging.
// The annotation itself does nothing — the capture macro checks for it.

[structure_macro(name=audited)]
class AuditedAnnotation : AstStructureAnnotation {
    def override apply(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        return true  // no-op tag
    }
}

// ---- Runtime helpers --------------------------------------------------------

def audit_on_capture(value : auto(T); field_name : string) : T {
    //! Prints a capture-audit message and returns the value unchanged.
    print("[audit] captured '{field_name}'\n")
    return value
}

def audit_after_invoke(field_name : string) {
    //! Prints a post-invocation audit message.
    //! Called from the lambda function's finalList (runs after each invocation,
    //! NOT on destruction — see captureFunction below).
    print("[audit] after-call: '{field_name}' still captured\n")
}

def audit_on_finalize(field_name : string) {
    //! Prints a release-audit message when the lambda is destroyed.
    //! Called from the lambda FINALIZER body (runs once on destruction,
    //! after the user-written finally{} block but before field cleanup).
    print("[audit] releasing '{field_name}'\n")
}

// ---- Type check helper ------------------------------------------------------

[macro_function]
def private is_audited(typ : TypeDeclPtr) : bool {
    if (!typ.isStructure || typ.structType == null) {
        return false
    }
    for (ann in typ.structType.annotations) {
        if (ann.annotation.name == "audited") {
            return true
        }
    }
    return false
}

// ---- Capture macro ----------------------------------------------------------

[capture_macro(name=capture_audit)]
class CaptureAuditMacro : AstCaptureMacro {
    //! Audits capture and release of [audited] struct fields in lambdas.
    //!
    //! captureExpression — wraps the capture expression in audit_on_capture(),
    //!   which prints a message and passes the value through.
    //! captureFunction — appends audit_after_invoke() calls to the lambda
    //!   function's body finalList for each [audited] field.  This code runs
    //!   after EACH invocation (per-call finally), not on destruction.
    //! releaseFunction — appends audit_on_finalize() calls to the lambda
    //!   FINALIZER body for each [audited] field.  This code runs once on
    //!   destruction (after the user-written finally{} but before field cleanup).

    def override captureExpression(prog : Program?; mod : Module?; expr : ExpressionPtr; etype : TypeDeclPtr) : ExpressionPtr {
        if (is_in_completion()) {
            return <- default<ExpressionPtr>
        }
        if (!is_audited(etype)) {
            return <- default<ExpressionPtr>
        }
        // Determine the field name from the expression (typically an ExprVar)
        var field_name = "unknown"
        if (expr is ExprVar) {
            field_name = string((expr as ExprVar).name)
        }
        // Wrap: capture_macro_mod::audit_on_capture(original_expr, "field_name")
        var inscope pCall <- new ExprCall(at = expr.at, name := "capture_macro_mod::audit_on_capture")
        pCall.arguments |> emplace_new <| clone_expression(expr)
        pCall.arguments |> emplace_new <| new ExprConstString(at = expr.at, value := field_name)
        return <- pCall
    }

    def override captureFunction(prog : Program?; mod : Module?; var lcs : Structure?; var fun : FunctionPtr) : void {
        // Generators call finally on every iteration — skip them
        if (fun.flags._generator) {
            return
        }
        for (fld in lcs.fields) {
            if (!is_audited(fld._type)) {
                continue
            }
            // Append: capture_macro_mod::audit_after_invoke("field_name")
            // NOTE: this adds to the lambda FUNCTION's body finalList, so it
            // runs after each invocation — like a per-call finally.  This is
            // different from the user-written finally{} block on the lambda
            // literal, which goes into the FINALIZER and runs on destruction.
            {
                var inscope pCall <- new ExprCall(at = fld.at, name := "capture_macro_mod::audit_after_invoke")
                pCall.arguments |> emplace_new <| new ExprConstString(at = fld.at, value := string(fld.name))
                (fun.body as ExprBlock).finalList |> emplace(pCall)
            }
        }
    }

    def override releaseFunction(prog : Program?; mod : Module?; var lcs : Structure?; var fun : FunctionPtr) : void {
        // Called once per lambda FINALIZER function.  Code appended here
        // runs on DESTRUCTION — after the user-written finally{} block but
        // before the compiler-generated field cleanup (delete *__this).
        for (fld in lcs.fields) {
            if (!is_audited(fld._type)) {
                continue
            }
            {
                var inscope pCall <- new ExprCall(at = fld.at, name := "capture_macro_mod::audit_on_finalize")
                pCall.arguments |> emplace_new <| new ExprConstString(at = fld.at, value := string(fld.name))
                (fun.body as ExprBlock).list |> emplace(pCall)
            }
        }
    }
}
