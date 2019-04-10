global crossld_call64_dst_addr_mid_offset
global crossld_call64_retconv_mid_offset
global crossld_call64_panic_addr_mid_offset
global crossld_call64_out_addr_mid_offset
global crossld_call64_out_offset
;global crossld_jump32
global crossld_jump32_offset
global crossld_hunks
global crossld_hunks_len
global crossld_call64_trampoline_start
global crossld_call64_trampoline_mid
global crossld_call64_trampoline_len1
global crossld_call64_trampoline_len2
global crossld_hunk_array
global crossld_check_u32
global crossld_check_s32
global crossld_pass_u64

section .rodata
dummy:
    db 0

align 8
crossld_call64_trampoline_len1:
    dq crossld_call64_trampoline_mid - crossld_call64_trampoline_start
crossld_call64_trampoline_len2:
    dq crossld_call64_trampoline_end - crossld_call64_trampoline_mid

crossld_call64_dst_addr_mid_offset:
    dq crossld_call64_dst_mov + 2 - crossld_call64_trampoline_mid
crossld_call64_retconv_mid_offset:
    dq crossld_call64_retconv - crossld_call64_trampoline_mid
crossld_call64_panic_addr_mid_offset:
    dq crossld_call64_panic_mov + 2 - crossld_call64_trampoline_mid
crossld_call64_out_addr_mid_offset:
    dq crossld_call64_out_mov + 2 - crossld_call64_trampoline_mid

crossld_call64_out_offset:
    dq crossld_call64_out - crossld_hunks

crossld_hunks_len:
    dq crossld_hunks_end - crossld_hunks

crossld_jump32_offset:
    dq crossld_jump32 - crossld_hunks

section .text

[bits 32]
crossld_call64_trampoline:
crossld_call64_trampoline_start:
    push ebp
    mov ebp, esp
    push ebx
    push edi
    push esi

    push 0x33
    call .eip
.eip:
    add dword [esp], crossld_call64_mid - $
    retf

[bits 64]

align 8
crossld_call64_trampoline_mid:
crossld_call64_mid:

crossld_call64_dst_mov:
    mov rax, dummy ; callee goes here
    call rax
crossld_call64_retconv:
    ; will patch this place with ret value conversion hunk
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
crossld_call64_nopanic:
crossld_call64_out_mov:
    mov r10, dummy ; crossld_call64_out goes here

    push r10                ; yeah, it pushes 8 bytes, even though we want 4
    mov dword [rsp+4], 0x23 ; so we overwrite the upper 4 bytes with segment selector :D
    retf
crossld_call64_panic:
    mov rdi, rax
crossld_call64_panic_mov:
    mov rax, dummy ; exit goes here
    call rax

crossld_call64_panic_jump_offset: equ crossld_call64_panic - crossld_call64_nopanic

align 8
crossld_call64_trampoline_end:

[bits 32]
crossld_hunks:
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

align 8
crossld_hunks_end:

align 8
crossld_check_u32:
    mov edi, eax
    test rdi, rax
    jnz short 1 + crossld_call64_panic_jump_offset

crossld_check_s32:
    movsx rdi, eax
    test rdi, rax
    jnz short 1 + crossld_call64_panic_jump_offset

crossld_pass_u64:
    nop
    shl rdx, 32
    or rax, rdx

align 8
crossld_hunk_array:
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

crossld_load_rdi_signed:
    nop
    nop
    nop
    movsx rdi, dword [ebp+0x55]

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

crossld_load_rsi_signed:
    nop
    nop
    nop
    movsx rsi, dword [ebp+0x55]

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

crossld_load_rdx_signed:
    nop
    nop
    nop
    movsx rdx, dword [ebp+0x55]

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

crossld_load_rcx_signed:
    nop
    nop
    nop
    movsx rcx, dword [ebp+0x55]

crossld_load_r8d:
    nop
    nop
    nop
    mov r8d, [ebp+0x55]

crossld_load_r8:
    nop
    nop
    nop
    mov r8,  [ebp+0x55]

crossld_load_r8_signed:
    nop
    nop
    nop
    movsx r8, dword [ebp+0x55]

crossld_load_r9d:
    nop
    nop
    nop
    mov r9d, [ebp+0x55]

crossld_load_r9:
    nop
    nop
    nop
    mov r9,  [ebp+0x55]

crossld_load_r9_signed:
    nop
    nop
    nop
    movsx r9, dword [ebp+0x55]

crossld_push:
    lea rdi, [esp-1]
    mov cl, 42
    sub rsp, rcx
    lea rsi, [rbp+0x55]
    std
    rep movsb

