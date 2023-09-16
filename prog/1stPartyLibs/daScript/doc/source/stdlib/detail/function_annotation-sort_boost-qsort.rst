Implements `qsort` macro. I'ts `qsort(value,block)`.
For the regular array<> or dim it's replaced with `sort(value,block)`.
For the handled types like das`vector its replaced with `sort(temp_array(value),block)`.
