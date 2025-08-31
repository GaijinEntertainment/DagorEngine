options gen2
require daslib/coroutines

struct Tree {
    data : int
    left, right : Tree?
}

let tree = new  Tree(
    data = 5,
    left = new Tree(uninitialized
        data = 1
    ),
    right = new Tree(uninitialized
        data = 7,
        right = new Tree(uninitialized
            data = 10
         )
    )
);

def each_async(tree : Tree?) : iterator<int> {
    return <- generator<int>() <| $() {
        if (tree.left != null) {
            yeild_from <| each_async(tree.left)
        }
        yield tree.data
        if (tree.right != null) {
            yeild_from <| each_async(tree.right)
        }
        return false
    }
}

[export]
def main {
    for (t in each_async(tree)) {
        print("{t}\n")
    }
}



