#define _POSIX_C_SOURCE 200112L

#include <signal.h>

int interrupted = 0;

/*
 * Save the signal number into the interrupted global variable
 * only if the program wasn't interrupted before.
 **/
static void int_handler(int signum) {
	if (!interrupted)
		interrupted = signum;
}

int setup_signal_handlers() {
	struct sigaction sa;
	sigfillset(&sa.sa_mask); /* do not interrupt a sig handler */
	sa.sa_flags = 0;
	sa.sa_handler = int_handler;

	if (sigaction(SIGINT, &sa, 0) == -1)
		return -1;

	if (sigaction(SIGTERM, &sa, 0) == -1)
		return -1;

	sa.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &sa, 0) == -1)
		return -1;

	return 0;
}
