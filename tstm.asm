; tstm.asm - test TB Monterey/Tahiti soundcard
;
; I/O register equates
;
VOLC    EQU  $FFC0    ; volume control register
PDAD    EQU  $FFC1    ; power down ADC ???
SRCA    EQU  $FFC2    ; sample rate control register ADC
SRCD    EQU  $FFC3    ; sample rate control register DAC
CLKE    EQU  $FFC4    ; sample clock enable ???
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
PABC    EQU  $FFFE    ; port A bus control register
IPRI    EQU  $FFFF    ; interrupt priority register
;
        ORG P:$0000
; interrupt vector [00] - hardware reset
        JMP <START
;
        ORG P:$0024
; interrupt vector [12] - host command 'STOP'
        BCLR #$02,Y:<$04
        NOP             
; interrupt vector [13] - host command 'PLAY'
        BSET #$02,Y:<$04  
        NOP               
; interrupt vector [14] - host command 'RATE'
        JSR >SETRATE
; interrupt vector [15] - host command 'NPTS'
        JSR >SETNPTS
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
        MOVEP #>$3330,X:<<PABC                  ; set X,Y,P mem 3 wait states
        MOVEP #>$0001,X:<<PBCR                  ; Port-B set periph
        MOVEP #>$0004,X:<<HOCR                  ; Host cmd interrupt enable
        MOVEP A,X:<<PCCR                        ; Port-C clear control reg
        MOVEP #>$0004,X:<<PCDD                  ; Port-C cfg pin2 output
        MOVEP #>$FFFFFB,X:<<PCCR                ; Port-C periph except pin2 
        MOVEP A,X:<<STRD                        ; SSI clear data reg
        MOVEP #>$6100,X:<<SSCA                  ; SSI 2wrd/frm, 24bit/wrd
        MOVEP #>$3800,X:<<SSCB                  ; SSI T/R enbl, net.mode
                                                ; cont.clk, sync, fsl=1
        MOVEP #>$0400,X:<<IPRI                  ; IntPrior HST=1 SCI=0 SSI=0
        MOVEP A,X:<<STRD                        ; SSI clear data reg
        MOVE                     A,Y:<$04       ; clear PLAY/RECORD flags
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
        JSET #$02,Y:<$04,>PBWD1
        ENDDO
        JMP <PBLUP
PBWD1   JCLR #$06,X:<<SSSR,>PBWD1
        MOVEP X:(R0),X:<<STRD
PBWA1   JCLR #$07,X:<<SSSR,>PBWA1
        MOVEP X:<<STRD,A
        MOVE                     A,Y:(R0)+
PBWD2   JCLR #$06,X:<<SSSR,>PBWD2
        MOVEP X:(R0),X:<<STRD
PBWA2   JCLR #$07,X:<<SSSR,>PBWA2
        MOVEP X:<<STRD,B
        MOVE                     B,Y:(R0)+
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
        MOVEP A,Y:<<SRCD                        ; set DAC clock rate
        MOVEP A,Y:<<SRCA                        ; set ADC clock rate
        MOVEP #>$0003,Y:<<PDAD                  ; power down ADC
        MOVEP #>$0000,Y:<<PDAD                  ; power up ADC
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
        MOVE                     #>$0082,X0     ; select 'AMPS' & 'IN pot'
        JCLR #$00,A1,>SPOT1                     ; check bit 0: IN=0,AUX=1
        MOVE                     #>$0084,X0     ; select 'AMPS' & 'AUX pot'
SPOT1   MOVEP X0,Y:<<VOLC                       ; enable AMPS and selected pot
        DO #$010,>SPOT3                         ; loop over LLRR bits
        ROL A                                   ; rotate out leading bit
        JCC >SPOT2                              ; if CARRY=0, then pot-bit=0
        BSET #$04,X0                            ; otherwise vol-bit=1
SPOT2   MOVEP X0,Y:<<VOLC                       ; output vol bit w/ lo clock
        BSET #$03,X0                            ; set clock bit
        MOVEP X0,Y:<<VOLC                       ; output vol bit w/ hi clock
        BCLR #$03,X0                            ; clear clock bit
        BCLR #$04,X0                            ; clear vol bit
        MOVEP X0,Y:<<VOLC                       ; output lo vol & clock bits
SPOT3
        MOVEP #>$0080,Y:<<VOLC                  ; enable AMPS
        RTI                                     ; return from SETPOT
