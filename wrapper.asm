global crossld_call64_in_fake
global crossld_call64_in
global crossld_jump32

extern crossld_call64

section .text

[bits 32]
crossld_call64_in_fake:
    push 0
    push 0x55555555
crossld_call64_in:
    push ebp
    mov ebp, esp
    push ebx
    push edi
    push esi

    mov edi, [ebp+4]  ; got[1]
    mov esi, [ebp+8]  ; got entry number
           ; [ebp+12] ; return address
    lea edx, [ebp+16] ; original args

    push 0x33
    push crossld_call64_mid
    retf
crossld_call64_out:
    sub ebp,12
    mov esp, ebp
    pop esi
    pop edi
    pop ebx
    pop ebp
    add esp, 8
    ret

crossld_jump32_out:
    push 0x2b
    pop ds
    push 0x2b
    pop es
    jmp esi

[bits 64]
crossld_call64_mid:
    call crossld_call64
    push crossld_call64_out ; yeah, it pushes 8 bytes, even though we want 4
    mov dword [rsp+4], 0x23 ; so we overwrite the upper 4 bytes with segment selector :D
    retf

crossld_jump32:
    mov rsp, rdi
    sub rsp, 8
    mov dword [rsp+4], 0x23
    mov rax, crossld_jump32_out
    mov [rsp], eax
    retf
