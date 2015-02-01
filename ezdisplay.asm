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
#DEFINE OUT_B	0xF0	; B4..B7
#DEFINE OUT_C	0xFF	; C0..C7

#DEFINE BTX		LATB,1	; B3 used by EZ downloader output
#DEFINE BRX		PORTB,2	; B0 used by EZ downloader input

#DEFINE PRESS0	PORTA,4
#DEFINE PRESS1	PORTA,5

#DEFINE ERRFLAG	USERFLAGS,0
#DEFINE PRESS0F USERFLAGS,1
#DEFINE PRESS1F USERFLAGS,2
#DEFINE CARRIER	USERFLAGS,7

;******************************************************************************
; VARIABLES
;******************************************************************************

	CBLOCK	0X000

	LCDHEX : 16	; lcd chars 0..F

	TEMP : 1	; temp variable
	USERFLAGS : 1

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
	MOVLW 0xB0	; A0..A3 out, A4..A5 in
	MOVWF TRISA
	MOVLW 0x0D	; B1 (BTX), B4..B7 out
	ANDWF TRISB
	CLRF TRISC	; C0..C7 out
	; initialize tick counts for TX/RX
	MOVLW 0x20
	MOVWF WAITUNIT

	; initialise display buffer
	CLRF FSR0H
	MOVLW LCDHEX
	MOVWF FSR0L		; file register <- LCDHEX
	MOVLW 0x7E	; 01111110	; 0
	MOVWF INDF0
	INCF FSR0L
	MOVLW 0x12	; 00010010	; 1
	MOVWF INDF0
	INCF FSR0L
	MOVLW 0xBC	; 10111100	; 2
	MOVWF INDF0
	INCF FSR0L
	MOVLW 0xB6	; 10110110	; 3
	MOVWF INDF0
	INCF FSR0L
	MOVLW 0xD2	; 11010010	; 4
	MOVWF INDF0
	INCF FSR0L
	MOVLW 0xE6	; 11100110	; 5
	MOVWF INDF0
	INCF FSR0L
	MOVLW 0xEE	; 11101110	; 6
	MOVWF INDF0
	INCF FSR0L
	MOVLW 0x32	; 00110010	; 7
	MOVWF INDF0
	INCF FSR0L
 	MOVLW 0xFE	; 11111110	; 8
	MOVWF INDF0
	INCF FSR0L
	MOVLW 0xF6	; 11110110	; 9
	MOVWF INDF0
	INCF FSR0L
	MOVLW 0xFA	; 11111010	; A
	MOVWF INDF0
	INCF FSR0L
	MOVLW 0xCE	; 11001110	; b
	MOVWF INDF0
	INCF FSR0L
	MOVLW 0x6C	; 01101100	; C
	MOVWF INDF0
	INCF FSR0L
	MOVLW 0x9E	; 10011110	; d
	MOVWF INDF0
	INCF FSR0L
	MOVLW 0xEC	; 11101100	; E
	MOVWF INDF0
	INCF FSR0L
	MOVLW 0xE8	; 11101000	; F
	MOVWF INDF0

MAINLOOP
	; user variables and flags
	CLRF USERFLAGS
READCARRIERMAIN		; loop reading first carrier
	BCF ERRFLAG		; clear user error flag
	MOVLW 0x08		; read 8 bits data
	MOVWF BITCNT
	CALL READBYTE
	BTFSC ERRFLAG
	BRA READCARRIERMAIN
DATAREAD			; read 8 bits
	; high quartet on LATC
	MOVFF TOREAD,TEMP
	MOVLW 0xF0
	ANDWF TEMP
	RRNCF TEMP
	RRNCF TEMP
	RRNCF TEMP
	RRNCF TEMP
	MOVLW LCDHEX
	ADDWF TEMP,0
	MOVWF FSR0L		; file register <- LCDHEX + TOREAD high
	MOVFF INDF0,LATC; char on LATC
	; low quartet in TEMP
	MOVFF TOREAD,TEMP
	MOVLW 0x0F
	ANDWF TEMP
	MOVLW LCDHEX
	ADDWF TEMP,0
	MOVWF FSR0L		; file register <- LCDHEX + TOREAD low
	MOVFF INDF0,TEMP; TEMP <- char
	MOVLW 0x0F		; char low on LATB high
	ANDWF TEMP
	RLNCF TEMP
	RLNCF TEMP
	RLNCF TEMP
	RLNCF TEMP
	MOVFF TEMP,LATB
	MOVFF INDF0,TEMP; TEMP <- char
	MOVLW 0xF0		; char high on LATA low
	ANDWF TEMP
	RRNCF TEMP
	RRNCF TEMP
	RRNCF TEMP
	RRNCF TEMP
	MOVFF TEMP,LATA
	; push buttons increment TOREAD
	BTFSC PRESS0
	BRA B0_PRESSED
	BCF PRESS0F
	BTFSC PRESS1
	BRA B1_PRESSED
	BCF PRESS1F
	BRA DATAREAD
B0_PRESSED
	BTFSC PRESS1	; B0 and B1 pressed -> send
	BRA STARTSEND
	BTFSS PRESS0F	; don't inc if BO already pressed
	INCF TOREAD
	BSF PRESS0F
	BRA DATAREAD
B1_PRESSED
	BTFSS PRESS1F	; don't dec if B1 already pressed
	DECF TOREAD
	BSF PRESS1F
	BRA DATAREAD
STARTSEND
	MOVLW 0x20		; send 32 carrier frames
	MOVWF TEMP
SENDCARRIERMAIN
	CALL SENDCARRIER
	DECFSZ TEMP
	BRA SENDCARRIERMAIN
	MOVFF TOREAD,TOSEND
	CALL SENDBYTE
	CALL SENDCARRIER	; send 2 carrier frames to finish
	CALL SENDCARRIER
	BCF ERRFLAG		; clear user error flag
	BRA MAINLOOP
END
