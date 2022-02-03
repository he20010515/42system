[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "a_nask.nas"]

        GLOBAL _api_putchar,_api_end,_api_putstring,_api_openwin

[SECTION .text]

_api_putchar: ;void api_putchar(int c)
        MOV     EDX,1
        MOV     AL,[ESP +4] ;c
        INT     0x40
        RET
_api_putstring: ;void api_putstring(char *s)
        PUSH    EBX
        MOV     EDX,2
        MOV     EBX,[ESP +8]
        INT     0x40
        POP     EBX
        RET

_api_openwin: ;int api_openwin(char *buf,int xsize,int ysize,int col_inv,char *title);
        PUSH    EDI
        PUSH    ESI
        PUSH    EBX
        MOV     EDX,5
        MOV     EBX,[ESP+16] ;buf
        MOV     ESI,[ESP+20] ;xsize        
        MOV     EDI,[ESP+24] ;ysize
        MOV     EAX,[ESP+28] ;col_inv
        MOV     ECX,[ESP+32] ;title
        INT     0x40
        POP     EBX
        POP     ESI
        POP     EDI
        RET
_api_end:     ;void api_end(void)
        MOV     EDX,4
        INT     0x40
