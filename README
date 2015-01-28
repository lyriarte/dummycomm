dummycomm README
================


### 4-ticks transmission frames

carrier : 1000
zero: 1100
one: 1110


### Automaton format

# first byte: automaton size

states:
  byte 1: latC output
  byte 2 high quartet: nb transitions
  byte 2 low quartet: latA low quartet output

transitions:
  byte 1: b0=1: b7..b1 delay
  byte 1: b0=0: b7..b4 latB high quartet input
                b3..b2 latA,4 latA,5
                b1=0
  byte 2: target state offset

# first byte / automaton size=2 / single state
  byte 2 high quartet: latbB high quartet output

# first byte / automaton size=0 / final state / error: 
  send output vector: b7..b4 latB high quartet
                      b3..b2 latA,4 latA,5
                      b1=0
                      b0=0 / 1 on error


#### LED hex display

# automaton size=2
  byte 1: high quartet display on latC
  byte 2: low quartet display quartets swap on latB high / latA low

# arduino dummycomm automaton size=1
  expand byte1 into quartets display in byte1 + byte 2
  send 2 bytes automaton

