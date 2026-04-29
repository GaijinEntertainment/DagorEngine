Enumeration with all possible side effects of expression or function.
No side effects.
Function is unsafe.
[sideeffects] annotation to indicate side effects.
Function may modify external state.
Access to external state.
Function may modify argument values.
Function may modify argument values and external state.
Function has all sideeffects, except for a user scenario. This is to bind functions, whith unknown sideeffects.
Function may access global state (variables and such).
Function is using 'invoke', so we don't know any additional side effects.
Mask for all sideefects, which can be inferred from the code.
