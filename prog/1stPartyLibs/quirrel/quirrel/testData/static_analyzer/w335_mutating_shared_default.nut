#allow-delete-operator
//-file:declared-never-used

function mut_array_method(a = []) {
    a.append(1)
}

function mut_table_newslot(t = {}) {
    t.x <- 1
}

function mut_table_assign(t = {}) {
    t.x = 1
}

function mut_table_plus_eq(t = { x = 0 }) {
    t.x += 1
}

function mut_table_slot_plus_eq(t = { x = 0 }) {
    t["x"] += 1
}

function mut_table_inc(t = { x = 0 }) {
    t.x++
}

function mut_table_slot_dec(t = { x = 0 }) {
    t["x"]--
}

function mut_table_delete(t = {}) {
    delete t.x
}

function mut_table_slot(t = {}) {
    t["x"] <- 1
}

function mut_table_slot_assign(t = {}) {
    t["x"] = 1
}

function mut_in_if(t = {}, cond = true) {
    if (cond) {
        t.x = 1
    }
}

function mut_in_inner_block(t = {}) {
    foreach (_ in [1]) {
        t.x <- 1
    }
}

function mut_in_nested_function(t = {}) {
    let f = function() {
        t.x = 1
    }
    f()
}

function mut_in_lambda(t = {}) {
    let f = @() t.append(1)
    f()
}

function nullable_default(a = null) {
    a.append(1)
}

function reassigned_default(a = []) {
    a = []
    a.append(1)
}

function mut_factory_call(a = array(0)) {
    a.append(1)
}

class Pt {
    x = 0
    constructor(v) { this.x = v }
}

function mut_instance_field(p = Pt(1)) {
    p.x = 2
}

function mut_clone_default(t = clone {}) {
    t.x <- 1
}

function scalar_default(x = 1 + 2) {
    x.foo = 1
}

function aliased_default(a = []) {
    let q = a
    q.append(1)
}

function reassigned_in_if(t = {}, cond = true) {
    if (cond) {
        t = {}
        t.x = 1
    }
}

function reassigned_in_nested_function(t = {}) {
    let f = function() {
        t = {}
        t.x = 1
    }
    f()
}
