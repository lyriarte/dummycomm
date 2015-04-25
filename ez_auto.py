#!/usr/bin/python

import os,sys,time,json

def strbin(num,l):
	str=bin(num)[2:]
	while len(str) < l:
		str = '0' + str
	return str

def strtrans(trans, offset):
	if trans.get('delay'):
		str = strbin(trans.get('delay'),7) + '1'
	else:
		str = trans.get('inB') + trans.get('inA') + '00'
	str = str + strbin(offset[trans.get('target')],8)
	return str

def strstate(state):
	str = state.get('outC')
	ntrans = 0
	if state.get('transitions'):
		ntrans = len(state.get('transitions'))
	str = str + strbin(ntrans,4) + state.get('outA')
	return str


#### #### #### ####

usage = "Usage: ez_auto.py <automaton.json> [--int]"

if len(sys.argv) < 2 :
	print usage
	exit(1)


filename = sys.argv[1]
file = open(filename,'r')
autotext = file.read()
autodict = json.loads(autotext )

autosize = 0
offset = []
for state in autodict:
	offset.append(autosize)
	autosize = autosize + 2
	if state.get('transitions'):
		autosize = autosize + len(state.get('transitions')) * 2

autobin = strbin(autosize,8)
for state in autodict:
	autobin = autobin + strstate(state)
	if state.get('transitions'):
		for trans in state.get('transitions'):
			autobin = autobin + strtrans(trans,offset)

if len(sys.argv) < 3 :
	print autobin
else:
	ibyte = 0
	while ibyte + 8 <= len(autobin):
		print int(autobin[ibyte:ibyte+8],2)
		ibyte = ibyte + 8
