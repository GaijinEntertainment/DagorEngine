This macro is used to implement `is json_value` and `as json_value` runtime checks.
It essencially substitutes `value as name` with `value.value as name` and `value is name` with `value.value is name`.
