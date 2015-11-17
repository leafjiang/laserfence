# Test with
# echo "123 456 789" | python argtest.py 10

import argparse

parser = argparse.ArgumentParser()
parser.add_argument('val', type=int, help="int value")
parser.add_argument('--input', type=argparse.FileType('r'), default='-')

args = parser.parse_args()

print "val = ", args.val

for line in args.input:
    print "stdin:", line
