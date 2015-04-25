#!/usr/bin/python

import sys

#### 0..F bin strings 
hextable=[
"01111110",
"00010010",
"10111100",
"10110110",
"11010010",
"11100110",
"11101110",
"00110010",
"11111110",
"11110110",
"11111010",
"11001110",
"01101100",
"10011110",
"11101100",
"11101000"
]

#### #### #### ####

usage = "Usage: ez_byte.py <0..255> [--int]"

if len(sys.argv) < 2 :
	print usage
	exit(1)

value = int(sys.argv[1])

if value >255:
	print("Error: illegal input")
	exit(1)

strbin = "00000010" + hextable[value/16] + hextable[value%16][4:] + hextable[value%16][:4]

if len(sys.argv) < 3 :
	print strbin
else:
	ibyte = 0
	while ibyte + 8 <= len(strbin):
		print int(strbin[ibyte:ibyte+8],2)
		ibyte = ibyte + 8
