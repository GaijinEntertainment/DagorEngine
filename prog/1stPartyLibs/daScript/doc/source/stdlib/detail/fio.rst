.. |typedef-fio-file| replace:: alias for the `FILE const?`; its there since most file functions expect exactly this type

.. |any_annotation-fio-FILE| replace:: Holds system specific `FILE` type.

.. |structure-fio-df_header| replace:: obsolete. header for the `fsave` and `fload` which internally use `binary_save` and `binary_load`.

.. |function-fio-base_name| replace:: equivalent to linux `basename`. Splits path and returns the string up to, but not including, the final '/'.

.. |function-fio-builtin_dir| replace:: iterates through all files in the specified `path`.

.. |function-fio-dir_name| replace:: equivalent to linux `dirname`. Splits path and returns the component preceding the final '/'.  Trailing '/' characters are not counted as part of the pathname.

.. |function-fio-fclose| replace:: equivalent to C `fclose`. Closes file.

.. |function-fio-feof| replace:: equivalent to C `feof`. Returns true if end of file has been reached.

.. |function-fio-fgets| replace:: equivalent to C `fgets`. Reads and returns new string from the line.

.. |function-fio-fmap| replace:: create map view of file, i.e. maps file contents to memory. Data is available as array<uint8> inside the block.

.. |function-fio-fopen| replace:: equivalent to C `fopen`. Opens file in different modes.

.. |function-fio-popen_binary| replace:: opens pipe to command and returns FILE pointer to it, in binary mode.

.. |function-fio-fprint| replace:: same as `print` but outputs to file.

.. |function-fio-fread| replace:: reads data from file.

.. |function-fio-fstat| replace:: equivalent to C `fstat`. Returns information about file, such as file size, timestamp, etc.

.. |function-fio-fstderr| replace:: returns FILE pointer to standard error.

.. |function-fio-fstdin| replace:: returns FILE pointer to standard input.

.. |function-fio-fstdout| replace:: returns FILE pointer to standard output.

.. |function-fio-fwrite| replace:: writes data fo file.

.. |function-fio-mkdir| replace:: makes directory.

.. |function-fio-sleep| replace:: sleeps for specified number of milliseconds.

.. |function-fio-stat| replace:: same as fstat, but file is specified by file name.

.. |function-fio-dir| replace:: iterates through all files in the specified `path`.

.. |function-fio-fload| replace:: obsolete. saves data to file.

.. |function-fio-fsave| replace:: obsolete. loads data from file.

.. |structure_annotation-fio-FILE| replace:: this is C FILE *

.. |structure_annotation-fio-FStat| replace:: `stat` and `fstat` return file information in this structure.

.. |variable-fio-df_magic| replace:: obsolete. magic number for `binary_save` and `binary_load`.

.. |function-fio-fseek| replace:: equivalent to C `fseek`. Rewinds position of the current FILE pointer.

.. |function-fio-ftell| replace:: equivalent to C `ftell`. Returns current FILE pointer position.

.. |function-fio-getchar| replace:: equivalent to C `getchar`. Reads and returns next character from standard input.

.. |function-fio-exit| replace:: equivalent to C `exit`. Terminates program.

.. |function-fio-popen| replace:: equivalent to linux `popen`. Opens pipe to command.

.. |function-fio-get_full_file_name| replace:: returns full name of the file in normalized form.

.. |variable-fio-seek_set| replace:: constant for `fseek` which sets the file pointer to the beginning of the file plus the offset.

.. |variable-fio-seek_cur| replace:: constant for `fseek` which sets the file pointer to the current position of the file plus the offset.

.. |variable-fio-seek_end| replace:: constant for `fseek` which sets the file pointer to the end of the file plus the offset.

.. |function-fio-remove| replace:: deletes file specified by name

.. |function-fio-fflush| replace:: equivalent to C `fflush`. Flushes FILE buffers.

.. |function-fio-get_env_variable| replace:: returns value of the environment variable.

.. |function-fio-sanitize_command_line| replace:: sanitizes command line arguments.

.. |function-fio-rename| replace:: renames file.

.. |function-fio-chdir| replace:: changes current directory.

.. |function-fio-getcwd| replace:: returns current working directory.