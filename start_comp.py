import argparse
import sys

from subprocess import *

data_folder = "data/"
bin_folder = "src/"

small_problems = [ "maze_small.bin", "sudoku_small.bin", "array_small.bin", "password_small.bin", "tree_small.bin", "RLE_small.bin" ]
small_problems = [ data_folder + s for s in small_problems ]

test_problems = [ "maze_test.bin", "sudoku_test.bin", "array_test.bin", "password_test.bin", "tree_test.bin", "RLE_test.bin" ]
test_problems = [ data_folder + s for s in test_problems ]

executable = ""
if sys.platform == "linux2":
    executable = 'gnome-terminal -e '

def start_comp(data):  
  Popen([executable + bin_folder + "\"Server " + " ".join(data) + "\""], stderr=PIPE, stdout=PIPE, shell=True)
  Popen([executable + bin_folder + "\"Client localhost\""], stderr=PIPE, stdout=PIPE, shell=True)


if __name__ == "__main__":
  parser = argparse.ArgumentParser("Argument for the competition")
  parser.add_argument("-m", "--mode", help="Mode of the competition: Test or Eval", required=True)
  args = parser.parse_args()

  print "Starting competition in %s mode" % args.mode
  if args.mode == "Test":
    start_comp(small_problems)
  elif args.mode == "Eval":
    start_comp(test_problems)
  else:
    print "Wrong argument on the command line"
