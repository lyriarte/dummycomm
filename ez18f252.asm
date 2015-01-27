;******************************************************************************
; MCU TYPE
;******************************************************************************

	LIST	p=18F252


;******************************************************************************
; INCLUDES
;******************************************************************************

	#include "p18F252.inc"


;******************************************************************************
; DEFINES
;******************************************************************************

#DEFINE OUT_A	0x0F	; A0..A3
#DEFINE OUT_C	0xFF	; C0..C7

#DEFINE IN_B	0xF0	; B4..B7

#DEFINE BTX		LATB,1	; B3 used by EZ downloader output
#DEFINE BRX		PORTB,2	; B0 used by EZ downloader input

#DEFINE PRESS0	PORTA,4
#DEFINE PRESS1	PORTA,5

#DEFINE ERRFLAG	USERFLAGS,0
#DEFINE WRITE_B	USERFLAGS,1
#DEFINE CARRIER	USERFLAGS,7

;******************************************************************************
; VARIABLES
;******************************************************************************

	CBLOCK	0X000

	TEMP : 1	; temp variable
	USERFLAGS : 1
	AUTOSTATE : 1 ; current automaton state

	WAITUNIT : 1
	WAIT : 1	; wait routine parameter / BRX inner count
	WAIT0 : 1	; private 2^8 loop / BRX 0 count
	WAIT1 : 1	; private WAITUNIT * 2^8 loop / BRX 1 count

	TOSEND : 1	; sendbyte routine parameter
	TOREAD : 1	; readframe routine in/out parameter

	ROTCNT : 1	; rotation counter
	BITCNT : 1	; bit counter

	COUNT0H : 1
	COUNT0L : 1
	COUNT1H : 1
	COUNT1L : 1

	DATABYTES : 256	; bytes count + max 255 data bytes

	ENDC


;******************************************************************************
;	USER CODE
;******************************************************************************
	ORG	0X0200	; EZ boot loader user code start address
	GOTO START ;

	ORG	0X0208	; high priority interrupt
	RETFIE
	
	ORG	0X0218	; low priority interrupt
	RETFIE


;******************************************************************************
;	ROUTINES
;******************************************************************************
WAITLOOP
	MOVFF WAITUNIT,WAIT1
WAITLOOP1
	SETF WAIT0
WAITLOOP0
	DECFSZ WAIT0
	BRA WAITLOOP0
	DECFSZ WAIT1
	BRA WAITLOOP1
	DECFSZ WAIT
	BRA WAITLOOP
	RETURN

SENDBYTE
	MOVLW 0x08
	MOVWF ROTCNT
SENDBYTELOOP
	RLCF TOSEND
	BC SENDBYTE1
	CALL SEND0
	BRA SENDBYTENEXT
SENDBYTE1
	CALL SEND1
SENDBYTENEXT
	DECFSZ ROTCNT
	BRA SENDBYTELOOP
	RETURN

SENDCARRIER		; 1000
	BSF BTX
	MOVLW 0x01
	MOVWF WAIT
	CALL WAITLOOP
	BCF BTX
	MOVLW 0x03
	MOVWF WAIT
	CALL WAITLOOP
	RETURN

SEND0			; 1100
	BSF BTX
	MOVLW 0x02
	MOVWF WAIT
	CALL WAITLOOP
	BCF BTX
	MOVLW 0x02
	MOVWF WAIT
	CALL WAITLOOP
	RETURN

SEND1			; 1110
	BSF BTX
	MOVLW 0x03
	MOVWF WAIT
	CALL WAITLOOP
	BCF BTX
	MOVLW 0x01
	MOVWF WAIT
	CALL WAITLOOP
	RETURN

GETINPUT	; B7.B6.B5.B4.PRESS1.PRESS0
	MOVFF PORTB,TOSEND
	MOVLW 0xF0
	ANDWF TOSEND
	BTFSC PRESS1
	BSF TOSEND,3
	BTFSC PRESS0
	BSF TOSEND,2
	RETURN

SENDINPUT	; B7.B6.B5.B4.PRESS1.PRESS0.BRX.ERRFLAG
	CALL GETINPUT
	BTFSC BRX
	BSF TOSEND,1
	BTFSC ERRFLAG
	BSF TOSEND,0
	CALL SENDBYTE
	RETURN

BRXCOUNT01
	CLRF WAIT0	; count received 0
	CLRF WAIT1	; count received 1
	SETF WAIT
BRXCOUNT01LOOP
	BTFSC BRX
	BRA BRXCOUNT01READ1
	INCF WAIT0
	BRA BRXCOUNT01NEXT
BRXCOUNT01READ1
	INCF WAIT1
BRXCOUNT01NEXT
	DECFSZ WAIT
	BRA BRXCOUNT01LOOP
	RETURN

BRXCOUNT0
	CLRF COUNT0H
	CLRF COUNT0L
BRXCOUNT0LOOP
	CALL BRXCOUNT01
	MOVF WAIT1,0	; WAIT0 + WAIT1 = 255
	CPFSGT WAIT0	; check that WAIT0 > WAIT1
	RETURN			; reading ones, BRX now set
	INFSNZ COUNT0L	; still reading zeroes
	INCF COUNT0H
	BRA BRXCOUNT0LOOP

BRXCOUNT1
	CLRF COUNT1H
	CLRF COUNT1L
BRXCOUNT1LOOP
	CALL BRXCOUNT01
	MOVF WAIT0,0	; WAIT0 + WAIT1 = 255
	CPFSGT WAIT1	; check that WAIT1 > WAIT0
	RETURN			; reading zeroes, BRX now clear
	INFSNZ COUNT1L	; still reading ones
	INCF COUNT1H
	BRA BRXCOUNT1LOOP

COUNTERSDIV2
	BCF COUNT0L,0
	RRNCF COUNT0L
	BCF COUNT1L,0
	RRNCF COUNT1L
	BCF STATUS,C
	RRCF COUNT0H
	BTFSC STATUS,C
	BSF COUNT0L,7
	BCF STATUS,C
	RRCF COUNT1H
	BTFSC STATUS,C
	BSF COUNT1L,7
	RETURN

READFRAME
; shift all counters to 7 lower bits
	CALL COUNTERSDIV2
	TSTFSZ COUNT0H
	BRA READFRAME
	TSTFSZ COUNT1H
	BRA READFRAME
	BTFSC COUNT0L,7
	BRA READFRAME
	BTFSC COUNT1L,7
	BRA READFRAME
; clear carrier flag
	BCF CARRIER
; count0 < count1 ?
	MOVF COUNT0L,0	; W <- count0
	CPFSGT COUNT1L
	BRA COUNT0GTCOUNT1	; count0 > count1
; count0 < count1 < 2.count0 ?
	RLCF WREG		; W <- 2.count0
	CPFSGT COUNT1L
	BRA READ0FRAME		; count0 < count1 < 2.count0
	BRA READ1FRAME		; count1 > 2.count0
COUNT0GTCOUNT1
; count1 < count0 < 2.count1 ?
	MOVF COUNT1L,0	; W <- count1
	RLCF WREG		; W <- 2.count1
	CPFSLT COUNT0L
	BRA READCARRIER	; count0 > 2.count1
; 0 : count1 < count0 < 2.count1 : 1100
; 0 : count0 < count1 < 2.count0 : 1100
READ0FRAME
	BCF TOREAD,0
	RETURN
; 1 : 2.count0 < count1 : 1110
READ1FRAME
	BSF TOREAD,0
	RETURN
; carrier : 2.count1 < count0 : 1000
READCARRIER
	BSF CARRIER
	RETURN

READBYTE
; read BITCNT bits into TOREAD
	CLRF TOREAD		; result. read at most 8 bits
READBYTELOOP
	CALL BRXCOUNT1
	CALL BRXCOUNT0
	CALL READFRAME	; TOREAD,0
	BTFSS CARRIER	; read a carrier ?
	BRA	READBIT		; read a bit
	TSTFSZ BITCNT	; carrier ok if no bits left to read
	BSF ERRFLAG		; otherwise it's an error
	RETURN
READBIT
	DCFSNZ BITCNT	; last bit to read ?
	RETURN
	RLCF TOREAD		; rotate left
	BRA READBYTELOOP; next bit in TOREAD,0 overwrites carry


;******************************************************************************
;	MAIN LOOP
;******************************************************************************
START
	CLRF LATA
	CLRF LATB
	CLRF LATC
RESTART
	MOVLW 0xB0	; A0..A3 out, A4..A5 in
	MOVWF TRISA
	BCF TRISB,1	; B1 (BTX) out
	CLRF TRISC	; C0..C7 out
	; user variables and flags
	CLRF USERFLAGS
	; initialize tick counts for TX/RX
	MOVLW 0x7F
	MOVWF WAITUNIT
MAINLOOP
	CLRF FSR0H		; initialise input buffer
	CLRF FSR0L
	MOVLW DATABYTES
	MOVWF FSR0L		; file register <- DATABYTES
	CLRF INDF0		; DATABYTES+0 <- 0
READCARRIERMAIN		; loop reading first carrier
	BCF ERRFLAG		; clear user error flag
	MOVLW 0x08		; read 8 bits for size
	MOVWF BITCNT
	CALL READBYTE
	BTFSC ERRFLAG
	BRA READCARRIERMAIN
DATAREAD			; read 8 bits
	MOVFF TOREAD,INDF0	; DATABYTES+0 <- byte count
	MOVFF INDF0,TEMP	; TEMP <- bytes to read
	TSTFSZ TEMP
	BRA DATAREADSTART
	BRA STARTSEND	; no bytes to read
DATAREADSTART
	MOVLW 0xF0		; B4..B7 in
	IORWF TRISB
	MOVLW 0x02
	CPFSEQ TEMP		; if just 2 bytes to read...
	BRA DATAREADMAIN
	BSF WRITE_B		; then using also B for output
	MOVLW 0x0D		; B1 (BTX), B4..B7 out
	ANDWF TRISB
DATAREADMAIN
	MOVLW 0x08		; read one byte
	MOVWF BITCNT
	CALL READBYTE
	BTFSC ERRFLAG	; send output on error
	BRA STARTSEND
	INCF FSR0L		; inc DATABYTES offset
	MOVFF TOREAD,INDF0	; store read byte
	DECFSZ TEMP		; dec bytes to read
	BRA DATAREADMAIN	
AUTOMAININIT
	CLRF LATA
	CLRF LATB
	CLRF LATC
	MOVLW DATABYTES
	MOVWF FSR0L		; file register <- DATABYTES
AUTOMAINLOOP
	MOVFF FSR0L,AUTOSTATE	; current state for automaton loop
AUTOMAINOUTLATC		; state first byte on LATC
	INCF FSR0L		; offset from DATABYTES + 1 for data size
	MOVFF INDF0,LATC
	INCF FSR0L
	MOVFF INDF0,TEMP
AUTOMAINOUTLATA		; state second byte trans count / LATA
	MOVLW 0x0F
	ANDWF TEMP
	MOVFF TEMP,LATA
AUTOMAINTRANSCOUNT
	MOVFF INDF0,TEMP
	MOVLW 0xF0
	ANDWF TEMP		; use high quartet for transition count 
	BTFSS WRITE_B	; or for output on LATB
	BRA AUTOMAINTRANSTEST
	MOVF TEMP,0		; W <- high quartet
	IORWF LATB		; set LATB high quartet
	BRA AUTOMAINFINISH	; displayed 2 bytes, done
AUTOMAINTRANSTEST
	RRNCF TEMP
	RRNCF TEMP
	RRNCF TEMP
	RRNCF TEMP
	TSTFSZ TEMP		; final state has no transition
	BRA AUTOMAINNEXTTRANS
	BRA AUTOMAINFINISH
AUTOMAINNEXTTRANS
	INCF FSR0L		; transition condition
	BTFSS INDF0,0	; if condition b0 is set...
	BRA AUTOMAINCHECKTRANS
	MOVFF INDF0,WAIT; ...then it's a delay...
	CALL WAITLOOP	; ...and unconditional jump
	BRA AUTOMAINGOTOTRANS
AUTOMAINCHECKTRANS
	CALL GETINPUT
	MOVF TOSEND,0
	CPFSEQ INDF0	; compare condition and input
	BRA AUTOMAINDECTRANS
AUTOMAINGOTOTRANS
	INCF FSR0L		; condition verified, get transition offset...
	MOVLW DATABYTES	; ... from DATABYTES
	ADDWF INDF0,0	; WREG <- DATABYTES + transition offset
	MOVWF FSR0L		; at DATABYTES + transition offset...
	BRA AUTOMAINLOOP; ...  + 1 is the transition target on LATC
AUTOMAINDECTRANS
	INCF FSR0L		; skip transition offset
	DECFSZ TEMP		; try next transition if any
	BRA AUTOMAINNEXTTRANS
AUTOMAINENDTRANS	; no valid transition
	MOVFF AUTOSTATE,FSR0L
	BRA AUTOMAINLOOP; loop on current state
AUTOMAINFINISH
STARTSEND
	MOVLW 0x3F		; send 63 carrier frames
	MOVWF TEMP
SENDCARRIERMAIN
	CALL SENDCARRIER
	DECFSZ TEMP
	BRA SENDCARRIERMAIN
	CALL SENDINPUT	; send B7.B6.B5.B4.PRESS1.PRESS0.BRX.ERRFLAG
	CALL SENDCARRIER	; send 2 carrier frames to finish
	CALL SENDCARRIER
	BCF ERRFLAG		; clear user error flag
	BTFSS WRITE_B
	BRA MAINLOOP
	BRA RESTART
END
