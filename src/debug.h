#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
#define DBG(...) printf(__VA_ARGS__); fflush(stdout);
#else
#define DBG(...)
#endif

#endif /* DEBUG_H */
