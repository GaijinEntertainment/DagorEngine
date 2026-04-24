import sys

# $(lic_target) $(lic_exe_name) $(lic_file_list)
targetFile = sys.argv[1]
print("Joining licenses files into {}".format(targetFile))

with open(file=targetFile, mode="wt", encoding="utf-8-sig") as sum_f:
  sum_f.write("{} uses libraries:\n".format(sys.argv[2]))
  for lic in sys.argv[3:]:
    prevDir = lic.rsplit("/", 2)[1]
    sum_f.write("\n==== {}\n\n".format(prevDir))
    with open(file=lic, mode="rt", encoding="utf-8-sig", errors="ignore") as lic_f:
      sum_f.write(lic_f.read())
