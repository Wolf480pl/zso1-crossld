section .text


load_edi:
    nop
    nop
    nop
    nop
    mov edi, [ebp+0x55]

load_rdi:
    nop
    nop
    nop
    mov rdi, [ebp+0x55]

load_esi:
    nop
    nop
    nop
    nop
    mov esi, [ebp+0x55]

load_rsi:
    nop
    nop
    nop
    mov rsi, [ebp+0x55]

load_edx:
    nop
    nop
    nop
    nop
    mov edx, [ebp+0x55]

load_rdx:
    nop
    nop
    nop
    mov rdx, [ebp+0x55]

load_ecx:
    nop
    nop
    nop
    nop
    mov ecx, [ebp+0x55]

load_rcx:
    nop
    nop
    nop
    mov rcx, [ebp+0x55]

load_r8:
    nop
    nop
    nop
    mov r8,  [ebp+0x55]

load_r8d:
    nop
    nop
    nop
    mov r8d, [ebp+0x55]

load_r9:
    nop
    nop
    nop
    mov r9,  [ebp+0x55]

load_r9d:
    nop
    nop
    nop
    mov r9d, [ebp+0x55]

push32_A:
    push word [ebp+0x57]
    push word [ebp+0x55]

push32_B:
    sub esp, 4
    mov r10d, [ebp+0x55]
    mov [esp], r10d

push64:
    push qword [ebp+0x55]

cpy:
    mov cl, 42
    lea rdi, [esp-1]
    sub rsp, rcx
    lea rsi, [rbp+0x55]
    std
    rep movsb
