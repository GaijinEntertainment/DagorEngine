Creates temporary array from the given object.
Important requirements are:
    * object memory is linear
    * each element follows the next one directly, with the stride equal to size of the element
    * object memory does not change within the lifetime of the returned array
