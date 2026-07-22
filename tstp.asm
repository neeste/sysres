; tstp.asm - test TB Pinnacle/Fiji soundcard
;
; local variables
;
SCLF1	EQU	1
SCLF2	EQU	2
;
; I/O register equates
;
SRCR    EQU  $FFC0    ; sample rate control register
CLKE    EQU  $FFC2    ; sample clock enable ???
VOLC    EQU  $FFC4    ; volume control register
;
PBCR    EQU  $FFE0    ; port B control register
PCCR    EQU  $FFE1    ; port C control register
PCDD    EQU  $FFE3    ; port C data direction register
HOCR    EQU  $FFE8    ; host control register
HOSR    EQU  $FFE9    ; host status register
HOTR    EQU  $FFEB    ; host receive data register
SSCA    EQU  $FFEC    ; SSI control register A
SSCB    EQU  $FFED    ; SSI control register B
SSSR    EQU  $FFEE    ; SSI status/time slot register
STRD    EQU  $FFEF    ; serial transmit/receive data register
PLLC    EQU  $FFFD    ; PLL control register
PABC    EQU  $FFFE    ; port A bus control register
IPRI    EQU  $FFFF    ; interrupt priority register
;
        ORG P:$0000
; interrupt vector [00] - hardware reset
        JMP <START                              ; RESET
;
        ORG P:$0024
; interrupt vector [12] - host command 'STOP'
        BCLR #$02,Y:<$04                        ; clear play bit
        NOP
; interrupt vector [13] - host command 'PLAY'
        BSET #$02,Y:<$04                        ; set play bit
        NOP
; interrupt vector [14] - host command 'RATE'
        JSR >SETRATE 
; interrupt vector [15] - host command 'NPTS'
        JSR >SETNPTS
; interrupt vector [16] - host command 'SCALE1'
        JSR >SCALE1                                     ; call scale1()
; interrupt vector [17] - host command 'SCALE2'
        JSR >SCALE2                                     ; call scale2()
;
        ORG P:$0034
; interrupt vector [1A] - host command 'SETPOT'
        JSR >SETPOT
; interrupt vector [1B] - host command 'BNK1EMP'
        BCLR #$0D,Y:<$04  
        NOP               
; interrupt vector [1C] - host command 'BNK1FUL'
        BSET #$0D,Y:<$04  
        NOP               
; interrupt vector [1D] - host command 'BNK2EMP'
        BCLR #$0F,Y:<$04  
        NOP               
; interrupt vector [1E] - host command 'BNK2FUL'
        BSET #$0F,Y:<$04
        NOP
;
        ORG P:$0040
;
; interrupt routine [00] - hardware reset
;
START   ORI #$03,MR                             ; mask interrupts
        CLR A                                   ; put zero in reg A
        MOVEP #>$0004,X:<<PABC                  ; set mem wait states
        MOVEP #>$0001,X:<<PBCR                  ; Port-B set periph
        MOVEP #>$0004,X:<<HOCR                  ; Host cmd interrupt enable
        MOVEP A,X:<<PCCR                        ; Port-C clear control reg
        MOVEP #>$0008,X:<<PCDD                  ; Port-C cfg pin3 output
        MOVEP #>$FFFFE7,X:<<PCCR                ; Port-C periph except pin3,4
        MOVEP A,X:<<STRD                        ; SSI clear data reg
        MOVEP #>$6100,X:<<SSCA                  ; SSI 2wrd/frm, 24bit/wrd
        MOVEP #>$FA00,X:<<SSCB                  ; SSI T/R enbl, net.mode
                                                ; cont.clk
        MOVEP #>$0400,X:<<IPRI                  ; IntPrior HST=1 SCI=0 SSI=0
        MOVEP A,X:<<STRD                        ; SSI clear data reg
        MOVE                     A,Y:<$04       ; clear PLAY/RECORD flags
        MOVEP #>$150003,X:<<PLLC
        MOVEP #>$000B,Y:<<SRCR
        MOVEP #>$008F,Y:<<VOLC
        MOVEC  A,SR                             ; enable interrupts
        MOVE                     #>$1000,A      ; NPTS = 4096
        MOVE                     A,Y:<$02       ; initialize NPTS
IDLUP   
        JSSET #$02,Y:<$04,>PLAY
;WAD0    JCLR #$06,X:<<SSSR,>WAD0
;        MOVEP X:<<STRD,B
;        MOVE                     B,X:>STRD
        JMP <IDLUP
;
PLAY
SYNC0   MOVEP #>$0000,X:<<STRD                  ; output zero to DAC
SYNC1   JCLR #$06,X:<<SSSR,>SYNC1               ; wait for txd empty
        JSET #$02,X:<<SSSR,>SYNC0               ; loop if no frame-sync occurred
SYNC2   MOVEP #>$0000,X:<<STRD                  ; output zero to DAC
SYNC3   JCLR #$06,X:<<SSSR,>SYNC3               ; wait for txd empty
        JSET #$02,X:<<SSSR,>SYNC2               ; loop if frame-sync occurred
PLAYL   JCLR #$02,Y:<$04,>PLAYX
        BCLR #$0A,Y:<$04
        JSSET #$0D,Y:<$04,>PLAB1
        JSSET #$0F,Y:<$04,>PLAB2
        JSET #$0A,Y:<$04,>PLAYL
        DO #$002,>PLAZL
WDAZ    JCLR #$06,X:<<SSSR,>WDAZ
        MOVEP #>$0000,X:<<STRD
PLAZL
        JMP <PLAYL
PLAYX   
        MOVEP #>$0000,X:<<STRD
WHPX    JCLR #$01,X:<<HOSR,>WHPX
        MOVEP #>$00000,X:<<HOTR
        RTS
;
PLAB1   MOVE                     #>$4000,R0
        JSR >PLABUF
PWHO1   JCLR #$01,X:<<HOSR,>PWHO1
        MOVEP #>$10000,X:<<HOTR
        BCLR #$0D,Y:<$04
        RTS
;
PLAB2   MOVE                     #>$6000,R0
        JSR >PLABUF
PWHO2   JCLR #$01,X:<<HOSR,>PWHO2
        MOVEP #>$30000,X:<<HOTR
        BCLR #$0F,Y:<$04
        RTS
;
PLABUF  
        MOVE                     Y:<$02,X0  ; load NPTS
        DO X0,>PBLUP
        JSET #$02,Y:<$04,>PBBEG
        ENDDO
        JMP PBLUP
PBBEG
        MOVE X:<SCLF1,Y1                        ; put SCLF1 in Y1
PBWA1   JCLR #$07,X:<<SSSR,>PBWA1
        MOVE X:(R0),Y0                          ; put DAC buf value in Y0
        MPY Y1,Y0,A                             ; multipy Y1*Y0 => A
        MOVEP A1,X:<<STRD                       ; output scaled value
        MOVEP X:<<STRD,Y:(R0)+
        MOVE X:<SCLF2,Y1                        ; put SCLF2 in Y1
PBWA2   JCLR #$07,X:<<SSSR,>PBWA2
        MOVE X:(R0),Y0                          ; put DAC buf value in Y0
        MPY Y1,Y0,A                             ; multipy Y1*Y0 => A
        MOVEP A1,X:<<STRD                       ; output scaled value
        MOVEP X:<<STRD,Y:(R0)+
PBLUP
        BSET #$0A,Y:<$04
        RTS
;
; interrupt routine [14] - host command 'RATE'
;
SETRATE
        JCLR #$00,X:<<HOSR,>SETRATE             ; wait for host data
        MOVEP X:<<HOTR,A                        ; read host data into reg A
        BSET #$00,Y:<<CLKE                      ; enable sample clock
        MOVEP A,Y:<<SRCR                        ; set sample clock rate
        RTI                                     ; return from SETRATE
;
; interrupt routine [14] - host command 'NPTS'
;
SETNPTS
        JCLR #$00,X:<<HOSR,>SETNPTS             ; wait for host data
        MOVEP X:<<HOTR,A                        ; read host data into reg A
        MOVE                     A,Y:<$02       ; save NPTS
        RTI                                     ; return from SETRATE
;
; interrupt routine [1A] - host command 'SETPOT'
;
SETPOT   
        JCLR #$00,X:<<HOSR,>SETPOT              ; wait for host data
        MOVEP X:<<HOTR,A1                       ; read host data into reg A1
                                                ; A1=LLRRPP: left,right,POT
        JCLR #$00,A1,>EVEN                      ; check pot-select bit 0
        JCLR #$01,A1,>POT1                      ; select POT1
POT3    JMP <SPOT3                              ; select POT3
POT2    MOVE                     #>$008B,X0     ; POT2 & AMPS bits
        JMP <SPOT1
EVEN    JCLR #$01,A1,>POT0
POT1    MOVE                     #>$008D,X0     ; POT1 & AMPS bits
        JMP <SPOT1
POT0    MOVE                     #>$008E,X0     ; POT0 & AMPS bits
SPOT1   MOVEP X0,Y:<<VOLC                       ; enable AMPS and selected pot
        DO #$010,>SPOT3                         ; loop over LLRR bits
        ROL A                                   ; rotate out leading bit
        JCC >SPOT2                              ; if CARRY=0, then pot-bit=0
        BSET #$05,X0                            ; otherwise vol-bit=1
SPOT2   MOVEP X0,Y:<<VOLC                       ; output vol bit w/ lo clock 
        BSET #$06,X0                            ; set clock bit
        MOVEP X0,Y:<<VOLC                       ; output vol bit w/ hi clock
        BCLR #$05,X0                            ; clear clock bit
        BCLR #$06,X0                            ; clear vol bit
        MOVEP X0,Y:<<VOLC                       ; output lo vol & clock bits
SPOT3
        MOVEP #>$008F,Y:<<VOLC                  ; enable AMPS
        RTI                                     ; return from SETPOT
;
; interrupt routine [16] - host command 'SCALE1'
;
SCALE1
	JCLR #$00,X:<<HOSR,>SCALE1              ; wait for host data
        MOVEP X:<<HOTR,A                        ; read host data into reg A
	MOVE A,X:<SCLF1                         ; store SCLF1
        RTI
;
; interrupt routine [17] - host command 'SCALE2'
;
SCALE2
	JCLR #$00,X:<<HOSR,>SCALE2              ; wait for host data
        MOVEP X:<<HOTR,A                        ; read host data into reg A
	MOVE A,X:<SCLF2                         ; store SCLF2
        RTI
