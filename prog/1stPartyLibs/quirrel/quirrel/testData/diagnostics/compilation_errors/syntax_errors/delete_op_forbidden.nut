// EXPECT_ERROR: "Usage of 'delete' operator is forbidden"
#forbid-delete-operator
let t = {a = 1}
delete t.a
