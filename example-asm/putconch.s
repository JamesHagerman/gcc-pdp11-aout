        .TITLE putconch: send a byte to the system console
        .IDENT "V01.00"

        .GLOBAL _putconch

        XCSR    = 0177564
        XBUF    = 0177566
        TXRDY   = 0x0080
        NRETRY  = 5000

        .text

_putconch:
        mov     r1,-(sp)
        mov     r2,-(sp)
        mov     $NRETRY, r1
10$:
        mov     XCSR,r2
        bit     r2, $TXRDY
        bne     20$
        dec     r1
        bne     10$
        mov     $2,r0
        jmp     999$

20$:    movb    r0,XBUF
        mov     $NRETRY, r1
30$:    mov     XCSR,r2
        bit     r2, $TXRDY
        bne     40$
        dec     r1
        bne     30$
        mov     $2, r0
        jmp     999$

40$:    mov     $0, r0
999$:
        mov (sp)+, r2
        mov (sp)+, r1
        rts     pc

        .end _putconch    

