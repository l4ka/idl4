/*
 * Copyright (c) 2000 University of Karlsruhe
 *
 * This file is part of ChacmOS (http://www.l4ka.org/)
 *
 * ChacmOS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ChacmOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ChacmOS; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place #330, Boston, MA 02111, USA.
 */

	.text
	.global _start
	.global _stext

_stext:	
_start:
	leal	__stack, %esp	/* load the stack pointer */
	
	/* push arguments to main on stack */
	pushl	%ebx	/* mbi ptr	*/
	pushl	%eax	/* mb magic	*/
	/* push a return address to catch main ending */
	pushl	$___return_from_main
	jmp	main /* call main() */


/* this is the multiboot "magic number" needed for grub to load/run this 
	program */	
#if 1
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
#endif

/* main() returns here, simply loop calling the kernel debugger */
___return_from_main:
	int $3
	jmp 1f
	.ascii "System stopped."
1:	jmp ___return_from_main
		

/* allocate 4*1024 bytes for the stack */
	.bss
	.space	4096
__stack:   /* this marks the top of the stack */
