        .global _start
        .align  3
        .ent    _start
        
_start:	dla	$29, init_stack
	daddu	$29, 8192
	
	jal	__L4_Init
	
	jal	main

1:
	b	1b
        
        .end    _start

.lcomm init_stack, 8192
