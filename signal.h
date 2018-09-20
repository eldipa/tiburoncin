#ifndef SIGNAL_H_
#define SIGNAL_H_

/*
 * Global variable (initialized to 0) that signals
 * when the program was interrupted by the user and
 * it should be closed.
 *
 * When the value is other than 0, it will have the value
 * of the signal received.
 *
 * */
extern int interrupted;

/*
 * EINTR_RETRY wraps a given expression into a do { } while(c) loop
 * where the while condition says that the expresion should be re evaluated
 * (the do-while keeps looping) if:
 *	- the expression returns -1 and
 *	- the global variable errno equals to EINTR and
 *	- the global variable interrupted is zero
 *
 * This macro requires that:
 *	- errno is declared and
 *	- interrupted is declared and
 *	- a local variable 's' is defined in the current scope as int
 *	- the expression can be called multiple times without side effects
 *
 * After the completion of this macro, the variable 's' should have the
 * last value of the expression.
 *
 * Example:
 *	#include <errno.h>
 *	#include "signal.h"
 *
 *	int read256(int fd, char* buf) {
 *		int s;
 *		EINTR_RETRY(read(fd, buf, 256));
 *
 *		return s;
 *	}
 *
 * */
#define EINTR_RETRY(expr)						\
		do {							\
			s = (expr);					\
                } while (s == -1 && errno == EINTR && !interrupted)

/*
 * Setup the signal handlers:
 *	- SIGINT (Interrupt / Ctrl-C): set interrupted variable to nonzero
 *	- SIGTERM (Termination): set interrupted variable to nonzero
 *	- SIGPIPE (Broken Pipe): ignore the signal
 *
 * Other signals are left to their default handlers.
 *
 * On error, return -1 and errno is set appropriately; return 0 on success.
 *
 * */
int setup_signal_handlers();

#endif
