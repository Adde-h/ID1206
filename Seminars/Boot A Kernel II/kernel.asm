bits 32
	global start
	
	section .text

start:
	mov dword [0xb8000], 0x2f41		; print 'A' to screen
	mov dword [0xb8002], 0x2f452f44		; print 'DE' to screen
	mov dword [0xb8006], 0x2f4c2f45		; print 'EL' to screen	
	hlt
