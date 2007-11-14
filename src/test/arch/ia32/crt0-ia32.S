	.text
	.global _start
	.global _stext

_stext:	
_start:
	/* load the stack pointer */
	leal	__stack, %esp
        
        /* stack boundary checking */
        movl	$0xC01AD05E, -8192(%esp)
        pushl	$0xC01AD05E
	
	/* push arguments to main on stack */
	pushl	%ebx	/* mbi ptr	*/
	pushl	%eax	/* mb magic	*/
        
	/* push a return address to catch main ending */
	pushl	$___return_from_main
        
        /* call main() */
	jmp	main

/* this is the multiboot "magic number" needed for grub to load/run this */	

	.align 16, 0x90
__mb_header:
	.long   0x1BADB002;
	.long   0x00010000;
	.long   - 0x00010000 - 0x1BADB002;
	.long   __mb_header;
	.long   _start;
	.long   _edata;
	.long   _end;
	.long   _start;

/* main() returns here, simply loop calling the kernel debugger */
___return_from_main:
	cmpl	$0xC01AD05E, 8(%esp)
        jne	2f
        cmpl	$0xC01AD05E, -8180(%esp)
        jne	2f
	int $3
	jmp 1f
	.ascii "System stopped"
1:	jmp _start
2:	int $3
	jmp 1f
        .ascii "STACK OVERFLOW DETECTED"
1:	jmp 2b

/* allocate 16kB for the stack */
	.bss
	.space	16384
__stack:   /* this marks the top of the stack */
