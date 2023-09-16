defer a block of code. For example::

    var a = fopen("filename.txt","r")
    defer <|
        fclose(a)

Will close the file when 'a' is out of scope.
