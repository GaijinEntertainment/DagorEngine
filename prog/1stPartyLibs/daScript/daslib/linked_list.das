options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module linked_list shared private

require daslib/spoof public

let public TLinkedList = %spoof_template~(ListClass,Foo)
struct LLNode_%ListClass
    data : %Foo?
    prev, next : LLNode_%ListClass?

class %ListClass
    head, tail : LLNode_%ListClass?
    def %ListClass
        head = null
        tail = null
    def add ( data : %Foo? )
        var node = new [[LLNode_%ListClass data=data]]
        if head == null
            head = node
            tail = node
        else
            tail.next = node
            node.prev = tail
            tail = node
    def remove ( data : %Foo? )
        var node = head
        while node != null
            if node.data == data
                if node.prev != null
                    node.prev.next = node.next
                else
                    head = node.next
                if node.next != null
                    node.next.prev = node.prev
                else
                    tail = node.prev
                return
            node = node.next
    def each : iterator<%Foo?>
        return <- generator <%Foo?> () <|
            var node = head
            while node != null
                yield node.data
                node = node.next
            return false
%%
