/*  All possible calls to 64-bit functions */
#ifndef __FAKELIB_H
#define __FAKELIB_H

/* exit provided by crossld */
_Noreturn void exit(int);

/* printout string */
void print(char *);

/* as crossld might not have proper exit() implementation, in some tests we provide our own */
_Noreturn void fake_exit (int);

/* see multicall-{32,64}.c */
int get_int(void);

/* Return 64-bit ptr, should cause error */
void *get_ptr64(void);

/* validation of argument passing */
long long validate(int, long, long long, unsigned int, unsigned long,
		unsigned long long, int, long, long long, unsigned int,
		unsigned long, unsigned long long);

/* stack alignment validation */
void *fun0(int t1, int t2, int t3, int t4, int t5, int t6);
void *fun4(int t1, int t2, int t3, int t4, int t5, int t6, int x);
void *fun8(int t1, int t2, int t3, int t4, int t5, int t6, int x, int y);
void *fun12(int t1, int t2, int t3, int t4, int t5, int t6, int x, int y, int z);
void *fun16(int t1, int t2, int t3, int t4, int t5, int t6, int x, int y, int z, int w);

#endif
