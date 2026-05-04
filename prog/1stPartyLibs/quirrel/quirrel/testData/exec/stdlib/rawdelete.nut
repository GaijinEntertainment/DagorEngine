for (local size = 1; size < 17; size++) {
    for (local iter = 0; iter < 2000; iter++) {
        //println("============")
        local seed = size + iter * 21
        local t = {}
        local indices = []
        for (local i = 0; i < size; i++) {
            seed++
            seed *= 198375
            indices.append(seed)
            t[seed] <- seed
        }

        //println($"before delete indices.len() = {indices.len()}")

        for (local i = 0; i < iter % size; i++) {
            if (iter & 1) {
                t.$rawdelete(indices[0])
                indices.remove(0)
            }
            else {
                t.$rawdelete(indices[indices.len() - 1])
                indices.remove(indices.len() - 1)
            }
        }
        //println($"after delete indices.len() = {indices.len()}")
        //println($"before ins table.len = {t.len()}")

        t.x <- "abc"

        foreach (_, v in indices)
            assert(t?[v] != null)

        //println($"after ins table.len = {t.len()}")
        //println(1 + size - iter % size)
        assert(t.len() == 1 + size - iter % size)

        assert(t.x == "abc")
    }
}

println("ok")