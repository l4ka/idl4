	.text
	.global _trampoline
        .global _m1
        .global _m2
        .align  4096

_trampoline:	
        jmp     _loop
        .space  64
        .align  4
        
_loop:	movl    $64, %esp

        movl    $0xFFFFFFFF, %eax
        movl    $0, %ecx
        movl    $0, %ebp
_m1:    movl    $0x4040001, %esi
        int     $0x30

        cmp     $0, %edx
        je      _rcv
        cmp     $1, %edx
        je      _zero
        cmp     $2, %edx
        je      _pager
        cmp     $3, %edx
        je      _lnch
        jmp     _loop

_pager: movl    %ebx, %esi
        movl    $0, %eax
        movl    $0xFFFFFFFF, %ecx
        movl    $0xFFFFFFFF, %edx
        movl    $0xFFFFFFFF, %ebx
        int     $0x35
        jmp     _loop
        
_zero:  movl    %edi, %ecx
        addl    $3, %ecx
        shr     $2, %ecx
        movl    %ebx, %edi
        xor     %eax, %eax
        rep
        stosl
        jmp     _loop
        
_lnch:  mov     %esi, %eax
        and     $0xFFFF03FF, %eax
        jmp     *%edi
        
_rcv:   pushl   %ebx
        pushl   %edi
        pushl   $0
        pushl   $0
        
        pushl   $0
        pushl   $0
        pushl   $0

        pushl   $0
        pushl   $0x6100
        pushl   $0

        movl    $0xFFFFFFFF, %eax
        movl    $0, %ecx
        movl    %esp, %ebp
_m2:    movl    $0x4040001, %esi
        int     $0x30
        jmp     _loop
