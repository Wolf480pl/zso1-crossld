struct arg_hunk {
    unsigned char insn[3];
    unsigned char depth;
};

struct ret_hunk {
    unsigned char insn[8];
};

extern char crossld_hunks;
extern char crossld_call64_trampoline_start;
extern char crossld_call64_trampoline_mid;
extern size_t crossld_jump32_offset;
extern size_t crossld_call64_dst_addr_mid_offset;
extern size_t crossld_call64_retconv_mid_offset;
extern size_t crossld_call64_panic_addr_mid_offset;
extern size_t crossld_call64_panic_ctx_addr_mid_offset;
extern size_t crossld_call64_out_addr_mid_offset;
extern size_t crossld_hunks_len;
extern size_t crossld_call64_trampoline_len1;
extern size_t crossld_call64_trampoline_len2;
extern size_t crossld_call64_out_offset;
extern size_t crossld_exit_offset;
extern size_t crossld_exit_ctx_addr_offset;
extern struct arg_hunk crossld_hunk_array[7][3];
extern struct arg_hunk crossld_push_rax;
extern struct ret_hunk crossld_check_u32;
extern struct ret_hunk crossld_check_s32;
extern struct ret_hunk crossld_pass_u64;

typedef int (*crossld_jump32_t)(void *stack, void *func, struct crossld_ctx *ctx);
// GCC doesn't like _Noreturn here, so we use attribute...
typedef __attribute__((__noreturn__)) void (*crossld_exit_t)(int status);

