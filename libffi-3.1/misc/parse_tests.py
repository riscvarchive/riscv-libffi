#!/bin/python

import sys

def print_help():
    print "Usage: python " + sys.argv[0] + " <test result file>"

if len(sys.argv) != 2:
    exit(1)

summary = ""
passed = []
failed = []
fname = sys.argv[1]
f = open(fname, 'r')
for l in f.readlines():
    if l.startswith("PASS"):
        z = l.split()
        if not z[1] in passed: 
            passed.append(z[1])
    elif l.startswith("FAIL"):
        z = l.split()
        if not z[1] in failed: 
            failed.append(z[1])
    elif l.startswith("#"):
        summary += l

i = 0
while i < len(passed):
    if passed[i] in failed:
        del passed[i]
        continue
    i += 1

p_num = len(passed)
f_num = len(failed)
total = p_num + f_num

print "---TEST FILES PASSED---"
for t in passed: print t
print
print "---TEST FILES FAILED---"
for t in failed: print t
print
print "------TEST SUMMARY------"
print "Passed: " + str(p_num) + " (" + str(round(float(p_num)/total*100, 1)) + "%)"
print "Failed: " + str(f_num) + " (" + str(round(float(f_num)/total*100, 1)) + "%)"
print "Total:  " + str(total)
print 
print summary
