options gen2
options with_module_is_unsafe = true

expect 31037

require ../language/_with_module_helper.das

[export]
def main {
    with (module _with_module_helper) {     // 31037: policy demands unsafe around user-written module-with
        let x = secret_scale
    }
}
