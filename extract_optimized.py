#read result from verbose and extract optimized ll

import sys

input_file_name = sys.argv[1]
output_file_name = sys.argv[2]

input_file = open(input_file_name,"r")
output_file = open(output_file_name,"w")

verbose = input_file.readlines()


start = 6
while verbose[start] != "----------------------------------------\n":
    output_file.write(verbose[start])
    start += 1