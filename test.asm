global test32
global crossld_call64_in_fake_ptr

section .rodata

str_ok:
    db 'OK!!',10
str_fail:
    db 'FAIL',10

section .data
crossld_call64_in_fake_ptr:
    dq 0

section .text

[bits 32]
test32:
    push ebp
    mov ebp, esp

    push 480

    mov eax, [crossld_call64_in_fake_ptr]
    call eax

    mov ecx, str_fail 
    cmp eax, 42
    jne .endif

    pop eax
    cmp eax, 480
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
