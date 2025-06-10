#ifndef __LIB_DEBUG_H
#define __LIB_DEBUG_H

/* GCC lets us add "attributes" to functions, function
 * parameters, etc. to indicate their properties.
 * See the GCC manual for details. */
#define UNUSED __attribute__ ((unused))
#define NO_RETURN __attribute__ ((noreturn))
#define NO_INLINE __attribute__ ((noinline))
#define PRINTF_FORMAT(FMT, FIRST) __attribute__ ((format (printf, FMT, FIRST)))

/* Halts the OS, printing the source file name, line number, and
 * function name, plus a user-specific message. */
#define PANIC(...) debug_panic (__FILE__, __LINE__, __func__, __VA_ARGS__)

void debug_panic (const char *file, int line, const char *function,
		const char *message, ...) PRINTF_FORMAT (4, 5) NO_RETURN;
void debug_backtrace (void);

#endif



/* This is outside the header guard so that debug.h may be
 * included multiple times with different settings of NDEBUG. */
#undef ASSERT
#undef NOT_REACHED

#ifndef NDEBUG
#define ASSERT(CONDITION)                                       \
	if ((CONDITION)) { } else {                             \
		PANIC ("assertion `%s' failed.", #CONDITION);   \
	}
#define NOT_REACHED() PANIC ("executed an unreachable statement");
#else
#define ASSERT(CONDITION) ((void) 0)
#define NOT_REACHED() for (;;)
#endif /* lib/debug.h */

// #define DEBUG_PRINT

#ifdef DEBUG_PRINT
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...) ((void)0)
#endif

// #define DEBUG_PRINTA 

#ifdef DEBUG_PRINTA
#define dprintfa(...) printf(__VA_ARGS__)
#else
#define dprintfa(...) ((void)0)
#endif
////////////////////////
// #define DEBUG_PRINTB

#ifdef DEBUG_PRINTB
#define dprintfb(...) printf(__VA_ARGS__)
#else
#define dprintfb(...) ((void)0)
#endif
////////////////////////
// #define DEBUG_PRINTC

#ifdef DEBUG_PRINTC
#define dprintfc(...) printf(__VA_ARGS__)
#else
#define dprintfc(...) ((void)0)
#endif
////////////////////////
// #define DEBUG_PRINTD

#ifdef DEBUG_PRINTD
#define dprintfd(...) printf(__VA_ARGS__)
#else
#define dprintfd(...) ((void)0)
#endif
////////////////////////
// #define DEBUG_PRINTE

#ifdef DEBUG_PRINTE
#define dprintfe(...) printf(__VA_ARGS__)
#else
#define dprintfe(...) ((void)0)
#endif
////////////////////////
// #define DEBUG_PRINTFF

#ifdef DEBUG_PRINTFF
#define dprintff(...) printf(__VA_ARGS__)
#else
#define dprintff(...) ((void)0)
#endif
////////////////////////
// #define DEBUG_PRINTFG

#ifdef DEBUG_PRINTFG
#define dprintfg(...) printf(__VA_ARGS__)
#else
#define dprintfg(...) ((void)0)
#endif
////////////////////////
// #define DEBUG_PRINTFI

#ifdef DEBUG_PRINTFI
#define dprintfi(...) printf(__VA_ARGS__)
#else
#define dprintfi(...) ((void)0)
#endif
////////////////////////
// #define DEBUG_PRINTFJ

#ifdef DEBUG_PRINTFJ
#define dprintfj(...) printf(__VA_ARGS__)
#else
#define dprintfj(...) ((void)0)
#endif
////////////////////////
// #define DEBUG_PRINTFK

#ifdef DEBUG_PRINTFK
#define dprintfk(...) printf(__VA_ARGS__)
#else
#define dprintfk(...) ((void)0)
#endif
////////////////////////