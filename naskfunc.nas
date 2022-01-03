; naskfunc
; TAB=4

[FORMAT "WCOFF"]		
[INSTRSET "i486p"]				; 告诉编译器,使用486以上的编译模式
[BITS 32]						; 32位

; 本文件中定义的函数
[FILE "naskfunc.nas"]			

		GLOBAL	_io_hlt,_write_mem8	


[SECTION .text]	

_io_hlt:	; void io_hlt(void);
		HLT
		RET

_write_mem8:		;void write_mem8(int addr,int data);
		MOV		ECX,[ESP+4]		;[ESP+4]中存放的是地址,将其读入ECX
		MOV		AL,[ESP+8]		;[ESP+8]中存放的数据,将其读入AL
		MOV		[ECX],AL		;将数据写入到ECX所指向的内存
		RET						
