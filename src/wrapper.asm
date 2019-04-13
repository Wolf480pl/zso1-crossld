global crossld_call64_dst_addr_mid_offset:data hidden
global crossld_call64_retconv_mid_offset:data hidden
global crossld_call64_panic_addr_mid_offset:data hidden
global crossld_call64_panic_ctx_addr_mid_offset:data hidden
global crossld_call64_out_addr_mid_offset:data hidden
global crossld_call64_out_offset:data hidden
global crossld_jump32_offset:data hidden
global crossld_hunks:data hidden
global crossld_hunks_len:data hidden
global crossld_call64_trampoline_start:data hidden
global crossld_call64_trampoline_mid:data hidden
global crossld_call64_trampoline_len1:data hidden
global crossld_call64_trampoline_len2:data hidden
global crossld_hunk_array:data hidden
global crossld_push_rax:data hidden
global crossld_check_u32:data hidden
global crossld_check_s32:data hidden
global crossld_pass_u64:data hidden
global crossld_exit_ctx_addr_offset:data hidden
global crossld_exit_offset:data hidden

section .rodata
dummy:
    db 0

; ----- global constants holding offsets and lengths of things in this file

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
crossld_call64_panic_ctx_addr_mid_offset:
    dq crossld_call64_panic_ctx_mov + 2 - crossld_call64_trampoline_mid
crossld_call64_out_addr_mid_offset:
    dq crossld_call64_out_mov + 2 - crossld_call64_trampoline_mid
crossld_call64_out_offset:
    dq crossld_call64_out - crossld_hunks

crossld_hunks_len:
    dq crossld_hunks_end - crossld_hunks

crossld_jump32_offset:
    dq crossld_jump32 - crossld_hunks

crossld_exit_offset:
    dq crossld_do_exit - crossld_hunks

crossld_exit_ctx_addr_offset:
    dq crossld_exit_ctx_mov + 2 - crossld_hunks

section .text

; ----- the function wrapper trampoline (will be copied a lot)

[bits 32]
; start of the function wrapper trampoline

crossld_call64_trampoline:
crossld_call64_trampoline_start:
    push ebp
    mov ebp, esp
    push ebx
    push edi
    push esi
    ; align the stack
    and esp, 0xfffffff0

    push 0x33
    call .eip
.eip:
    add dword [esp], crossld_call64_mid - $
    retf

[bits 64]

; argument conversions get injected here

; tail of the function wrapper trampoline
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
    mov rax, dummy ; panic goes here
crossld_call64_panic_ctx_mov:
    mov rsi, dummy ; panic ctx goes here
    call rax

crossld_call64_panic_jump_offset: equ crossld_call64_panic - crossld_call64_nopanic

align 8
crossld_call64_trampoline_end:


; ----- shared pieces of code (will be copied once)

[bits 32]
crossld_hunks:

; shared epilogue to which 64-bit trampoline code far-returns
crossld_call64_out:
    sub ebp,12
    mov esp, ebp
    pop esi
    pop edi
    pop ebx
    pop ebp
    ret

; these two switch us to the crossld stack
crossld_jump32_out:
    push 0x2b
    pop ds
    push 0x2b
    pop es
    jmp esi

[bits 64]
crossld_jump32:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov [rdx], rsp
    mov rsp, rdi
    sub rsp, 8
    mov dword [rsp+4], 0x23
    lea rax, [rel crossld_jump32_out]
    mov [rsp], eax
    retf

; this gets us back to the normal stack
; (it's called as a wrapped 64-bit function)
crossld_do_exit:
    mov rax, rdi ; exit code
crossld_exit_ctx_mov:
    mov rdi, dummy
    mov rsp, [rdi]
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

align 8
crossld_hunks_end:

; ----- a toolbox of hunks that will get copied and concatenated
;       in various ways, dependent on what needs to be done

; 8-byte hunks for return value conversion
; they get copied into the nop sled in the 64-bit part of trampoline 
; and expect some panic code at a certain offset after their end

align 8
crossld_check_u32:
    nop
    mov edi, eax
    cmp rdi, rax
    jnz short 1 + crossld_call64_panic_jump_offset

crossld_check_s32:
    movsx rdi, eax
    cmp rdi, rax
    jnz short 1 + crossld_call64_panic_jump_offset

crossld_pass_u64:
    nop
    mov rdx, rax
    shr rdx, 32

; An array of 4-byte arg conversion hunks
; that get inserted in the middle of the trampoline.
; See wrapper.h for the structure of the array.
; The 4th byte of each element needs to be rbp-relative depth on the stack
; and will be overwriten.

align 4
crossld_hunk_array:
.load_edi:
    mov edi, [ebp+0x55]

.load_rdi:
    mov rdi, [rbp+0x55]

.load_rdi_signed:
    movsx rdi, dword [rbp+0x55]

.load_esi:
    mov esi, [ebp+0x55]

.load_rsi:
    mov rsi, [rbp+0x55]

.load_rsi_signed:
    movsx rsi, dword [rbp+0x55]

.load_edx:
    mov edx, [ebp+0x55]

.load_rdx:
    mov rdx, [rbp+0x55]

.load_rdx_signed:
    movsx rdx, dword [rbp+0x55]

.load_ecx:
    mov ecx, [ebp+0x55]

.load_rcx:
    mov rcx, [rbp+0x55]

.load_rcx_signed:
    movsx rcx, dword [rbp+0x55]

.load_r8d:
    mov r8d, [rbp+0x55]

.load_r8:
    mov r8,  [rbp+0x55]

.load_r8_signed:
    movsx r8, dword [rbp+0x55]

.load_r9d:
    mov r9d, [rbp+0x55]

.load_r9:
    mov r9,  [rbp+0x55]

.load_r9_signed:
    movsx r9, dword [rbp+0x55]

.load_eax:
    mov eax, [ebp+0x55]

.load_rax:
    mov rax, [rbp+0x55]

.load_rax_signed:
    movsxd rax, [rbp+0x55]

.end

; a 4-byte hunk for pushing rax, also copied into the arg conversion part

crossld_push_rax:
    push rax
    nop
    nop
    nop
