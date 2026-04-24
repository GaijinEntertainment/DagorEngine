options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module linked_list shared private

//! Doubly-linked list template.
//!
//! Provides ``TLinkedList``, a spoof template that generates a doubly-linked list class
//! for a given element type. The generated class supports ``add``, ``remove``, and ``each`` (iteration).
//! Usage: ``apply_template(TLinkedList, "MyList", "MyElement")``

require daslib/spoof public

let public TLinkedList = %spoof_template~(ListClass,Foo) //! Spoof template that generates a doubly-linked list class for a given element type.
struct LLNode_%ListClass {
    //! Node of the doubly-linked list. Contains data pointer and prev/next links.
    data : %Foo?                        //! pointer to the stored element
    prev, next : LLNode_%ListClass?     //! links to previous and next nodes
}

class %ListClass {
    //! Doubly-linked list class. Maintains head and tail pointers.
    head, tail : LLNode_%ListClass?     //! head and tail of the list
    def %ListClass {
        head = null
        tail = null
    }

    def add ( var data : %Foo? ) {
        //! Appends an element to the end of the list.
        var node = new LLNode_%ListClass(data=data)
        if (head == null) {
            head = node
            tail = node
        } else {
            tail.next = node
            node.prev = tail
            tail = node
        }
    }

    def remove ( data : %Foo? ) {
        //! Removes the first node whose data matches the given pointer.
        var node = head
        while (node != null) {
            if (node.data == data) {
                if (node.prev != null) {
                    node.prev.next = node.next
                } else {
                    head = node.next
                }
                if (node.next != null) {
                    node.next.prev = node.prev
                } else {
                    tail = node.prev
                }
                return
            }
            node = node.next
        }
    }
    def each : iterator<%Foo?> {
        //! Returns an iterator over all elements in the list, from head to tail.
        return <- generator <%Foo?> () <| $() {
            var node = head
            while (node != null) {
                yield node.data
                node = node.next
            }
            return false
        }
    }
}
%%
