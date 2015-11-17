# Laser Fence Main Application
# Example command line usage:
#  stdbuf -oL lfcamapp 210 10 240 500 n=10 | cat
#  stdbuf -oL lfcamapp 210 10 240 500 | python lfmainapp.py 999 `discover_ue9_ipaddr.sh` 2>log.txt 1>cam.out
#    where
#      stdbuf -o0 turns off buffering completely for stdout
#      stdbuf -oL sets buffering to line buffering for stdout
#
# 07/29/15 LAJ - Document created

from __future__ import print_function
import argparse
from datetime import datetime
import sys
from time import sleep
import ue9

# Parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument('threshold', type=int, help="Pass/fail threshold")
parser.add_argument('ipaddr', help="LabJack UE9 IP address")
parser.add_argument('--input', type=argparse.FileType('r'), default='-')
args = parser.parse_args()
print("Threshold: {0}".format(args.threshold), file=sys.stderr)
print("UE9 IP address: {0}".format(args.ipaddr), file=sys.stderr)

# Create device object for LabJack UE9
d = ue9.UE9(ethernet=True, ipAddress=args.ipaddr)

# Set conveyor belt to YELLOW
# TODO

# Turn on conveyor belt
# TODO

# Loop over stdin.  Presumably, data input comes from lfcamapp.
for line in args.input():
    print(line, file=sys.stdout)  # Echo lfcamapp's results to stdout

    # Parse the line
    linesplit = line.split(' ')
    counter = int(linesplit[0])
    firstFrameSum = int(linesplit[1])
    secondFrameSum = int(linesplit[2])

    # Compute score
    score = firstFrameSum - secondFrameSum

    # Record the current time
    t = datetime.now()

    # Did the pouch pass or fail?
    if score > args.threshold:
        # FAIL: fibers detected
        print("{0} FAIL {1}".format(counter, unicode(t)), file=sys.stderr)
        # Set light tower to RED
        # TODO
        # Command robot arm to remove bad pad
        # TODO: use some delay, current time, t, to compute when to
        # stop belt and remove stuff...
    else:
        # PASS: no fibers found
        print("{0} PASS {1}".format(counter, unicode(t)), file=sys.stderr)
        # Set light tower to GREEN
        # TODO


    ### PWM

    # Set the time clock to be the system clock with a given divisor
    d.writeRegister(7000, 1)
    d.writeRegister(7002, 15)

    # Enable the timer
    d.writeRegister(50501, 1)

    # Configure the timer for PWM, starting with a duty cycle of 0.0015%
    baseValue = 65535
    d.writeRegister(7100, [0, baseValue])

    # Loop, updating the duty cycle every time
    for i in range(65):
        currentValue = baseValue - (i * 1000)
        dutyCycle = ( float(65536 - currentValue) / 65535 ) * 100
        print("Duty cyle = {0}%".format(dutyCycle), file=sys.stderr)
        d.writeRegister(7200, currentValue)
        sleep(0.3)
    print("Duty cycle = 100%", file=sys.stderr)
    d.writeRegister(7200,0)


# Close the device
d.close



