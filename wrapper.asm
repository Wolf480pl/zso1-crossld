global crossld_call64_in_fake
;global crossld_call64_in
;global crossld_jump32
global crossld_jump32_offset
global crossld_hunks
global crossld_hunks_len

extern crossld_call64

section .text

[bits 32]
crossld_hunks:
crossld_call64_in_fake:
crossld_call64_in:
    push ebp
    mov ebp, esp
    push ebx
    push edi
    push esi

           ; [ebp+4] ; return address
    lea edi, [ebp+8] ; original args

    push 0x33
;    nop
;    nop
;    nop
;    push crossld_call64_mid
    call .eip
.eip:
    add dword [esp], crossld_call64_mid - $
    retf

[bits 64]

crossld_call64_mid:
    mov rax, crossld_call64
    call rax
;    push crossld_call64_out ; yeah, it pushes 8 bytes, even though we want 4
    lea r10, [rel crossld_call64_out]
    push r10                ; yeah, it pushes 8 bytes, even though we want 4
    mov dword [rsp+4], 0x23 ; so we overwrite the upper 4 bytes with segment selector :D
    retf

[bits 32]
crossld_call64_out:
    sub ebp,12
    mov esp, ebp
    pop esi
    pop edi
    pop ebx
    pop ebp
    ret

crossld_jump32_out:
    push 0x2b
    pop ds
    push 0x2b
    pop es
    jmp esi

[bits 64]
crossld_jump32:
    mov rsp, rdi
    sub rsp, 8
    mov dword [rsp+4], 0x23
    lea rax, [rel crossld_jump32_out]
    mov [rsp], eax
    retf
crossld_jump32_offset: equ crossld_jump32 - crossld_hunks

align 8
crossld_load_edi:
    nop
    nop
    nop
    nop
    mov edi, [ebp+0x55]

crossld_load_rdi:
    nop
    nop
    nop
    mov rdi, [ebp+0x55]

crossld_load_esi:
    nop
    nop
    nop
    nop
    mov esi, [ebp+0x55]

crossld_load_rsi:
    nop
    nop
    nop
    mov rsi, [ebp+0x55]

crossld_load_edx:
    nop
    nop
    nop
    nop
    mov edx, [ebp+0x55]

crossld_load_rdx:
    nop
    nop
    nop
    mov rdx, [ebp+0x55]

crossld_load_ecx:
    nop
    nop
    nop
    nop
    mov ecx, [ebp+0x55]

crossld_load_rcx:
    nop
    nop
    nop
    mov rcx, [ebp+0x55]

crossld_load_r8:
    nop
    nop
    nop
    mov r8,  [ebp+0x55]

crossld_load_r8d:
    nop
    nop
    nop
    mov r8d, [ebp+0x55]

crossld_load_r9:
    nop
    nop
    nop
    mov r9,  [ebp+0x55]

crossld_load_r9d:
    nop
    nop
    nop
    mov r9d, [ebp+0x55]

crossld_push:
    lea rdi, [esp-1]
    mov cl, 42
    sub rsp, rcx
    lea rsi, [rbp+0x55]
    std
    rep movsb
crossld_hunks_len:
    dq $ - crossld_hunks
