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

static int initialize_block_all_sigset(sigset_t *set) {
	if (sigfillset(set) != -1 \
			&& sigdelset(set, SIGBUS) != -1  \
			&& sigdelset(set, SIGFPE) != -1  \
			&& sigdelset(set, SIGILL) != -1  \
			&& sigdelset(set, SIGSEGV) != -1 \
			&& sigdelset(set, SIGCONT) != -1 \
			&& sigdelset(set, SIGTSTP) != -1 \
			&& sigdelset(set, SIGTTIN) != -1 \
			&& sigdelset(set, SIGTTOU) != -1) {
		return 0;
	}
	else {
		return -1;
	}
}

int block_all_signals() {
	sigset_t set;
	if (initialize_block_all_sigset(&set) != -1 \
			&& sigprocmask(SIG_SETMASK, &set, 0) != -1) {
		return 0;
	}
	else {
		return -1;
	}
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

int initialize_interrupt_sigset(sigset_t *set) {
	if (initialize_block_all_sigset(set) != -1 \
			    && sigdelset(set, SIGINT) != -1 \
			    && sigdelset(set, SIGQUIT) != -1 \
			    && sigdelset(set, SIGTERM) != -1 \
			    && sigdelset(set, SIGPIPE) != -1) {
		return 0;
	}
	else {
		return -1;
	}
}
