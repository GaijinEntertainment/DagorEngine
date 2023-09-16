require DagorFindFiles

[generic]
def find_files_in_folder(dir_path, file_suffix_to_match : string; result : block<(list : array<string> const#) : void>) : int
  return find_files_in_folder(dir_path, file_suffix_to_match, /*vromfs*/true, /*realfs*/true, /*subdirs*/false, result)

[generic]
def find_files_in_vroms(dir_path, file_suffix_to_match : string; result : block<(list : array<string> const#) : void>) : int
  return find_files_in_folder(dir_path, file_suffix_to_match, /*vromfs*/true, /*realfs*/false, /*subdirs*/false, result)
