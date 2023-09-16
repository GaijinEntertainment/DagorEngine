function canEquipBothItems(_x) {}

function _foo(data, onDropExceptionCb = null) {
    if (onDropExceptionCb != null && !canEquipBothItems(data) && data?.item != null) {
        onDropExceptionCb(data.item)
    }
}