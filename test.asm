global test32

extern crossld_call64_in_fake

section .rodata

str_ok:
    db 'OK!!',10
str_fail:
    db 'FAIL',10

section .text

[bits 32]
test32:
    push ebp
    mov ebp, esp

    call crossld_call64_in_fake

    mov ecx, str_fail 
    cmp eax, 42
    jne .endif
    mov ecx, str_ok
.endif:
    mov ebx, 1
    mov edx, 5
    mov eax, 4
    int 0x80

    mov ebx, 0
    mov eax, 1
    int 0x80
