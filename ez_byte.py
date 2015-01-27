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
value = int(sys.argv[1])

if value >255:
	print("Error: illegal input")
	exit(1)

print("00000010" + hextable[value/16] + hextable[value%16][4:] + hextable[value%16][:4])


