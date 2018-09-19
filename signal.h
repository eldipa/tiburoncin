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
